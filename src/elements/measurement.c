#include "log.h"
#include "memory.h"
#include "murmur3.h"
#include "packets.h"
#include "rte_prefetch.h"
#include "util.h"
#include "types.h"

#include "elements/measurement.h"

#define MM_SIZE 16
#define MM_SIZE_HALF (MM_SIZE >> 1)
#define MM_PREFETCH
#define MM_PREFETCH_INST

//undef MM_PREFETCH
#undef MM_PREFETCH_INST

void measurement_resize(struct measurement_t *self, size_t size) {
    size = ge_pow2_64(size);
    self->tbl_size = size;
    self->tbl = mem_realloc(self->tbl, sizeof(uint32_t) * size);
}

struct measurement_t *measurement_create(void) {
    struct measurement_t *measurement = (struct measurement_t *)mem_alloc(sizeof(struct measurement_t));

    measurement->element.process = measurement_process_prefetching;
    measurement->element.release = measurement_release;
    measurement->element.report = measurement_report;
    measurement->element.connect = element_connect;
    measurement->element.disconnect = element_disconnect;
    measurement->element.hop_at = element_hop_at;
    measurement->tbl_size = 0;
    measurement->tbl = 0;

    return measurement;
}

void measurement_process_prefetching(struct element_t *ele, struct packet_t **pkts, packet_index_t size) {
    struct measurement_t *self = (struct measurement_t *)ele;
    struct packet_t *pkt;
    struct packet_t *p[MM_SIZE];
    uint32_t hashes[MM_SIZE_HALF];
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

        //XXX: There is a major difference in the way that the compiler
        // compiles the following code (look at the union definition):
        // IP.ports.src = *(pkt + 12);
        // IP.ports.dst = *(pkt + 14);
        //
        // vs.
        //
        // IP.ports.srcdst_port = *(pkt + 12);
        //
        // In reality, both of them are doing the same thing (well depending
        // on the endiness).  But the compiler can generate a much more
        // efficient code for one implementation vs. another:
        //
        // one mov vs. multiple moves + masking.
        //
    } ip;

    uint32_t out;

    // XXX: Help the compiler with unrolling the loops by forcefully processing the
    // packets in smaller batches of FIXED size.  Normally this would look pretty simple:
    //
    // for PKT_BATCH_OF_SIZE_8 in PKTS {
    //  for PKT in PKT_BATCH_OF_SIZE_8 {
    //    ...
    //  }
    // }
    //
    // But considering the note from bp_measurement.c, you prob want to be
    // processing multiple smaller batches together:  For example, in the case
    // of measurement module, you may want to prefetch content of packets while
    // computing the hash (or updating the hash table) in another batch.
    for (int j = 0; j < MM_SIZE_HALF; ++j) {
        p[j] = pkts[j];
        rte_prefetch0(p[j]->hdr + 26);
    }

    int i = MM_SIZE_HALF;
    size_t ss = self->tbl_size - 1;
    for (;i < size - MM_SIZE_HALF; i += MM_SIZE_HALF) {
        for (int j = 0; j < MM_SIZE_HALF && i + j < size; ++j) {
            p[j + MM_SIZE_HALF] = pkts[i + j];
            // Prefetch the next set of packets
            rte_prefetch0(p[j + MM_SIZE_HALF]->hdr + 26);
        }

#ifdef MM_PREFETCH
        for (int j = 0; j < MM_SIZE_HALF; ++j) {
            pkt = p[j];
            ip.src = *((ipv4_t*)(pkt->hdr+ 14 + 12));
            ip.dst = *((ipv4_t*)(pkt->hdr+ 14 + 12 + 4));
            ip.ports.src = *((uint16_t*)(pkt->hdr+ 14 + 12 + 8));
            ip.ports.dst = *((uint16_t*)(pkt->hdr+ 14 + 12 + 10));

            out = util_hash_ret(&ip, sizeof(ip));
            out &= ss;
            hashes[j] = out;
#ifdef MM_PREFETCH_INST
            rte_prefetch0(self->tbl + out);
#endif
        }

        for (int j = 0; j < MM_SIZE_HALF; ++j) {
            self->tbl[hashes[j]]++;
            p[j] = p[j + MM_SIZE_HALF];
        }
#else
        for (int j = 0; j < MM_SIZE_HALF; ++j) {
            pkt = p[j];
            ip.src = *((ipv4_t*)(pkt->hdr+ 14 + 12));
            ip.dst = *((ipv4_t*)(pkt->hdr+ 14 + 12 + 4));
            ip.ports.src = *((uint16_t*)(pkt->hdr+ 14 + 12 + 8));
            ip.ports.dst = *((uint16_t*)(pkt->hdr+ 14 + 12 + 10));

            out = util_hash_ret(&ip, sizeof(ip));
            out &= ss;
            self->tbl[out]++;
            p[j] = p[j + MM_SIZE_HALF];
        }
#endif //MM_PREFETCH
    }

    i -= MM_SIZE_HALF;
#ifdef MM_PREFETCH
    for (int j = i; j < size; ++j) {
        pkt = pkts[j];
        ip.src = *((ipv4_t*)(pkt->hdr+ 14 + 12));
        ip.dst = *((ipv4_t*)(pkt->hdr+ 14 + 12 + 4));
        ip.srcdst_port = *((uint32_t*)(pkt->hdr+ 14 + 12 + 8));

        out = util_hash_ret(&ip, sizeof(ip));
        out &= ss;
        hashes[j - i] = out;
#ifdef MM_PREFETCH_INST
        rte_prefetch0(self->tbl + out);
#endif //MM_PREFETCH_INST
    }

    for (int j = i; j < size; ++j) {
        self->tbl[hashes[j - i]]++;
    }
#else
    for (int j = i; j < size; ++j) {
        pkt = pkts[j];
        ip.src = *((ipv4_t*)(pkt->hdr+ 14 + 12));
        ip.dst = *((ipv4_t*)(pkt->hdr+ 14 + 12 + 4));
        ip.srcdst_port = *((uint32_t*)(pkt->hdr+ 14 + 12 + 8));

        out = util_hash_ret(&ip, sizeof(ip));
        out &= ss;
        self->tbl[out]++;
    }
#endif

    element_dispatch(ele, 0, pkts, size);
}


void measurement_process_no_prefetching(struct element_t *ele, struct packet_t **pkts, packet_index_t size) {
    struct measurement_t *self = (struct measurement_t *)ele;
    struct __attribute__((packed)) {
        ipv4_t src;
        ipv4_t dst;
        uint16_t src_port;
        uint16_t dst_port;
    } ip;

    uint32_t out;
    size_t size_minus_one = self->tbl_size - 1;

    for (packet_index_t i = 0; i < size; ++i) {
        ip.src = *((ipv4_t*)(pkts[i]->hdr+ 14 + 12));
        ip.dst = *((ipv4_t*)(pkts[i]->hdr+ 14 + 12 + 4));
        ip.src_port = *((uint16_t*)(pkts[i]->hdr+ 14 + 20 + 0));
        ip.dst_port = *((uint16_t*)(pkts[i]->hdr+ 14 + 20 + 2));

        out = util_hash_ret(&ip, sizeof(ip));

        // Make sure that the tbl_size is a multiple of two
        self->tbl[out & size_minus_one]++;
    }
    element_dispatch(ele, 0, pkts, size);
}

void measurement_release(struct element_t *ele) {
    struct measurement_t *self = (struct measurement_t *)ele;
    if (self->tbl) {
        printf("Printing measurement moduel stats\n");
        size_t size_minus_one = self->tbl_size - 1;
        size_t total_unique = 0;
        size_t total_count = 0;
        for (size_t i = 0; i < size_minus_one; ++i) {
            total_count += self->tbl[i];
            total_unique += (self->tbl[i] != 0);
        }
        printf("Total unique: %ld, total count: %ld\n", total_unique, total_count);
        mem_release(self->tbl);
    }
    mem_release(self);
}

void measurement_report(__attribute__((unused)) struct element_t *_) {
    // VOID
}
