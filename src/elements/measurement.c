#include "log.h"
#include "memory.h"
#include "murmur3.h"
#include "packets.h"
#include "rte_prefetch.h"
#include "util.h"
#include "types.h"

#include "elements/measurement.h"

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

#ifdef MEASUREMENT_POPULAR
    measurement->pop_count = 0;
    measurement->pop_sig[0] = 1933683066;
    measurement->pop_sig[1] = 1769104744;
    measurement->pop_sig[2] = 265260270;
#endif

    return measurement;
}

void measurement_process_prefetching(struct element_t *ele, struct packet_t **pkts, packet_index_t size) {
    struct measurement_t *self = (struct measurement_t *)ele;
    struct __attribute__((packed)) {
        ipv4_t src;
        ipv4_t dst;
        uint16_t src_port;
        uint16_t dst_port;
    } ip;

    packet_index_t idx = size;
    uint32_t out;
    size_t size_minus_one = self->tbl_size - 1;
    struct packet_t **pkt_ptr = pkts;

    ELEMENT_TIME_START(pkts, size);

    while (idx > MEASUREMENT_BUFFER_SIZE) {
        for (packet_index_t i = 0; i < MEASUREMENT_BUFFER_SIZE; ++i) {
            rte_prefetch0(pkt_ptr[i]->hdr + 26);
        }

        for (packet_index_t i = 0; i < MEASUREMENT_BUFFER_SIZE; ++i) {
            ip.src = *((ipv4_t*)(pkt_ptr[i]->hdr + 14 + 12));
            ip.dst = *((ipv4_t*)(pkt_ptr[i]->hdr+ 14 + 12 + 4));
            ip.src_port = *((uint16_t*)(pkt_ptr[i]->hdr+ 14 + 20 + 0));
            ip.dst_port = *((uint16_t*)(pkt_ptr[i]->hdr+ 14 + 20 + 2));

            util_hash(&ip, sizeof(ip), &out);
            out &= size_minus_one;
            rte_prefetch0(self->tbl + out);
            self->_tmp[i] = out;
        }

        for (packet_index_t i = 0; i < MEASUREMENT_BUFFER_SIZE; ++i) {
            // Make sure that the tbl_size is a multiple of two
            self->tbl[self->_tmp[i]]++;
        }

        idx -= MEASUREMENT_BUFFER_SIZE;
        pkt_ptr += MEASUREMENT_BUFFER_SIZE;
    }

    if (idx > 0) {
        for (packet_index_t i = 0; i < idx; ++i) {
            rte_prefetch0(pkt_ptr[i]->hdr + 26);
        }

        for (packet_index_t i = 0; i < idx; ++i) {
#ifdef MEASUREMENT_POPULAR
            if (//memcmp(pkt_ptr[i]->hdr + 14 + 12, &self->pop_sig, 12) == 0) {
                (*((uint32_t*)(pkt_ptr[i]->hdr + 14 + 12)) == self->pop_sig[0]) &&
                (*((uint32_t*)(pkt_ptr[i]->hdr + 14 + 16)) == self->pop_sig[1]) &&
                (*((uint32_t*)(pkt_ptr[i]->hdr + 14 + 20)) == self->pop_sig[2])) {
               //self->pop_count++;
               self->_tmp[i] = 0;
            } else {
#endif // MEASUREMENT_POPULAR
                ip.src = *((ipv4_t*)(pkt_ptr[i]->hdr+ 14 + 12));
                ip.dst = *((ipv4_t*)(pkt_ptr[i]->hdr+ 14 + 12 + 4));
                ip.src_port = *((uint16_t*)(pkt_ptr[i]->hdr+ 14 + 20 + 0));
                ip.dst_port = *((uint16_t*)(pkt_ptr[i]->hdr+ 14 + 20 + 2));

                util_hash(&ip, sizeof(ip), &out);
                out &= size_minus_one;
                rte_prefetch0(self->tbl + out);
                self->_tmp[i] = out;
#ifdef MEASUREMENT_POPULAR
            }
#endif // MEASUREMENT_POPULAR
        }

        for (packet_index_t i = 0; i < idx; ++i) {
            // Make sure that the tbl_idx is a multiple of two
            self->tbl[self->_tmp[i]]++;
        }
    }

    ELEMENT_TIME_END(pkts, size);
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

        util_hash(&ip, sizeof(ip), &out);

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
        for (int i = 0; i < size_minus_one; ++i) {
            total_count += self->tbl[i];
            total_unique += (self->tbl[i] != 0);
        }
        printf("Total unique: %d, total count: %d\n", total_unique, total_count);
#ifdef MEASUREMENT_POPULAR
        printf("Measurement popular: %d\n", self->pop_count);
#endif // MEASUREMENT_POPULAR

        mem_release(self->tbl);
    }
    mem_release(self);
}

void measurement_report(__attribute__((unused)) struct element_t *_) {
    // VOID
}
