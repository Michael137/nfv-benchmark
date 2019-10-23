#include "checksum.h"
#include "element.h"
#include "log.h"
#include "memory.h"
#include "murmur3.h"
#include "packets.h"
#include "rte_prefetch.h"
#include "types.h"
#include "util.h"

#include "elements/routing.h"
#include "elements/measurement.h"

#include "elements/merged_fastpass.h"

#define MPF_SIZE 16
#define MPF_SIZE_HALF (MPF_SIZE >> 1)
#define MPF_TBL_SIZE (1<<18)

struct merged_fastpass_t *merged_fastpass_create(uint32_t a, uint32_t b, uint32_t c) {
    struct merged_fastpass_t *merged = (struct merged_fastpass_t *)mem_alloc(sizeof(struct merged_fastpass_t));
    memset(merged, 0, sizeof(struct merged_fastpass_t));

    merged->element.process = merged_fastpass_process;
    merged->element.release = merged_fastpass_release;
    merged->element.report = merged_fastpass_report;
    merged->element.connect = element_connect;
    merged->element.disconnect = element_disconnect;
    merged->element.hop_at = element_hop_at;

    // Setup routing pool
    // nop;

    // Setup measurement pool
    merged->tbl_size = ge_pow2_64(MPF_TBL_SIZE);
    merged->tbl = mem_alloc(sizeof(uint32_t) * merged->tbl_size);
    memset(merged->tbl, 0, sizeof(uint32_t) * merged->tbl_size);

    // Setup checksum
    // nop;

    merged->a = a; // src?
    merged->b = b; // dst?
    merged->c = c; // srcdst_port?
    merged->_tmp_2 = 0;
    merged->slowpass_count = 0;

    return merged;
}

void merged_fastpass_process(struct element_t *ele, struct packet_t **pkts, packet_index_t size) {
    struct merged_fastpass_t *self = (struct merged_fastpass_t*)ele;
    // Use the two/two prefetching pattern
    struct packet_t *pkt;
    struct packet_t *p[MPF_SIZE];
    uint32_t hashes[MPF_SIZE_HALF];
    struct __attribute__((packed)) {
        ipv4_t src;
        ipv4_t dst;
        union {
            struct __attribute__((packed)) {
                uint16_t src;
                uint16_t dst;
            } ports;
            uint32_t srcdst_port;
        };
    } ip;

    for (int j = 0; j < MPF_SIZE_HALF; ++j) {
        p[j] = pkts[j];
        rte_prefetch0(p[j]->hdr + 26);
    }

    uint32_t a, b, c;
    uint32_t fastpass_count = 0;
    a = self->a; b = self->b; c = self->c;

    struct packet_t *slow[MPF_SIZE];
    uint32_t out = 0;

    int i = MPF_SIZE_HALF;
    for (;i < size - MPF_SIZE_HALF; i += MPF_SIZE_HALF) {
        for (int j = 0; j < MPF_SIZE_HALF && i + j < size; ++j) {
            p[j + MPF_SIZE_HALF] = pkts[i + j];
            // Prefetch the next set of packets
            rte_prefetch0(p[j + MPF_SIZE_HALF]->hdr + 26);
        }

        uint32_t sidx = 0;
        for (int j = 0; j < MPF_SIZE_HALF; ++j) {
            pkt = p[j];
            ip.src = *((ipv4_t*)(pkt->hdr+ 14 + 12));
            ip.dst = *((ipv4_t*)(pkt->hdr+ 14 + 12 + 4));
            ip.srcdst_port = *((uint32_t*)(pkt->hdr+ 14 + 12 + 8));

            slow[sidx++] = pkt;


//            uint32_t is_fast = \
//                               (ip.src == a) &&
//                               (ip.dst == b) &&
//                               (ip.srcdst_port == c) ||
//                               (ip.src == 1361913446 &&
//                                ip.dst == 2103858746 &&
//                                ip.srcdst_port == 1513629304) ||
//                               (ip.src == 198540573 &&
//                                ip.dst == 249629719 &&
//                                ip.srcdst_port == 847046559);
//
//            uint32_t is_fast = \
//                               (ip.src == a) || 
//                               (ip.src == 1361913446) ||
//                               (ip.src == 198540573);

            uint32_t is_fast = \
                               (ip.src == a) &&
                               (ip.dst == b) &&
                               (ip.srcdst_port == c);
            sidx -= (is_fast);
            fastpass_count += is_fast;
            self->checksum_count += checksum(pkt->hdr, pkt->size);
        }

        for (uint32_t j = 0; j < sidx; ++j) {
            pkt = slow[j];
            ip.src = *((ipv4_t*)(pkt->hdr+ 14 + 12));
            ip.dst = *((ipv4_t*)(pkt->hdr+ 14 + 12 + 4));
            ip.srcdst_port = *((uint32_t*)(pkt->hdr+ 14 + 12 + 8));

            out = util_hash_ret(&ip, sizeof(ip));
            out &= ((MPF_TBL_SIZE) - 1);
            hashes[j] = out;
            //rte_prefetch0(self->tbl + out);

            struct _routing_tbl_entry_t *ent = routing_entry_find(self, ip.dst);
            if (ent) {
                self->port_count += ent->port;
                ent->count ++;
            }
        }

        for (uint32_t j = 0; j < sidx; ++j) {
            self->tbl[hashes[j]]++;
        }

        self->slowpass_count += sidx;

        for (int j = 0; j < MPF_SIZE_HALF; ++j) {
            p[j] = p[j + MPF_SIZE_HALF];
        }
    }

    i -= MPF_SIZE_HALF;
    uint32_t sidx = 0;
    for (int j = i; j < size; ++j) {
        pkt = pkts[j];
        ip.src = *((ipv4_t*)(pkt->hdr+ 14 + 12));
        ip.dst = *((ipv4_t*)(pkt->hdr+ 14 + 12 + 4));
        ip.srcdst_port = *((uint32_t*)(pkt->hdr+ 14 + 12 + 8));

        slow[sidx++] = pkt;
        uint32_t is_fast = \
                               (ip.src == a) &&
                               (ip.dst == b) &&
                               (ip.srcdst_port == c);

//        uint32_t is_fast = \
//                               (ip.src == a) &&
//                               (ip.dst == b) &&
//                               (ip.srcdst_port == c) ||
//                               (ip.src == 1361913446 &&
//                                ip.dst == 2103858746 &&
//                                ip.srcdst_port == 1513629304) ||
//                               (ip.src == 198540573 &&
//                                ip.dst == 249629719 &&
//                                ip.srcdst_port == 847046559);
//        uint32_t is_fast = \
//                           (ip.src == a) || 
//                           (ip.src == 1361913446) ||
//                           (ip.src == 198540573);

        sidx -= (is_fast);
        fastpass_count += is_fast;
        self->checksum_count += checksum(pkt->hdr, pkt->size);
    }

    for (uint32_t j = 0; j < sidx; ++j) {
        pkt = slow[j];
        ip.src = *((ipv4_t*)(pkt->hdr+ 14 + 12));
        ip.dst = *((ipv4_t*)(pkt->hdr+ 14 + 12 + 4));
        ip.srcdst_port = *((uint32_t*)(pkt->hdr+ 14 + 12 + 8));

        out = util_hash_ret(&ip, sizeof(ip));
        out &= ((MPF_TBL_SIZE) - 1);
        //rte_prefetch0(self->tbl + out);
        hashes[j] = out;

        struct _routing_tbl_entry_t *ent = routing_entry_find(self, ip.dst);
        if (ent) {
            self->port_count += ent->port;
            ent->count ++;
        }
    }

    for (uint32_t j = 0; j < sidx; ++j) {
        self->tbl[hashes[j]]++;
    }
    self->slowpass_count += sidx;

    self->_tmp_2 += fastpass_count;
    element_dispatch(ele, 0, pkts, size);
}

void merged_fastpass_release(struct element_t *ele) {
    struct merged_fastpass_t *self = (struct merged_fastpass_t *)ele;

    uint64_t total = 0;
    for (size_t i = 0 ; i < self->tbl_size; ++i) {
        total += self->tbl[i];
    }
    printf("Total number of packets processed: %lu\n", total);
    printf("Total number of fastpass packets: %lu\n", self->_tmp_2);
    printf("Total number of slowpass packets: %lu\n", self->slowpass_count);

    if (self->tbl) {
        mem_release(self->tbl);
    }
    mem_release(ele);
}

void merged_fastpass_report(__attribute__((unused)) struct element_t *_) {
    // VOID
}
