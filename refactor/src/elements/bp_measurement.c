#include "log.h"
#include "memory.h"
#include "murmur3.h"
#include "packets.h"
#include "rte_prefetch.h"
#include "util.h"
#include "types.h"

#include "elements/bp_measurement.h"

void bp_measurement_resize(struct bp_measurement_t *self, size_t size) {
    size = ge_pow2_64(size);
    self->tbl_size = size;
    self->tbl = mem_realloc(self->tbl, sizeof(uint32_t) * size);
}

struct bp_measurement_t *bp_measurement_create(void) {
    struct bp_measurement_t *measurement = (struct bp_measurement_t *)mem_alloc(sizeof(struct bp_measurement_t));

    measurement->element.process = bp_measurement_process;
    measurement->element.release = bp_measurement_release;
    measurement->element.report = bp_measurement_report;
    measurement->element.connect = element_connect;
    measurement->element.disconnect = element_disconnect;
    measurement->element.hop_at = element_hop_at;
    measurement->tbl_size = 0;
    measurement->tbl = 0;

    return measurement;
}

void bp_measurement_process(struct element_t *ele, struct packet_t **pkts, packet_index_t size) {
    struct bp_measurement_t *self = (struct bp_measurement_t *)ele;
    struct __attribute__((packed)) {
        ipv4_t src;
        ipv4_t dst;
        uint16_t src_port;
        uint16_t dst_port;
    } ip;

    uint32_t out;
    size_t size_minus_one = self->tbl_size - 1;
    uint32_t tmp[256];

    // XXX: Breaking down data-structure pull and update operations can give us
    // "enough" time to pull the data of the DS into the cache.  In this
    // specific case (measurement), I have broken down the read and update
    // operations for updating the hash table.  Note that we are using more
    // instructions than "{naive,batching}_measurement.c" because we are
    // iterating over packets twice; however, we have a much better cache
    // locality because we can prefetch the data into the cache and avoid
    // bubbles in the instruction pipeline.
    //
    // In an ideal world, you would have hoped that the super-scalar
    // architecture of the CPU handles this at some level.  E.g., the CPU or
    // the compiler would somehow start processing packets, it would see the
    // bubble and go to the next packet (if there is no dependencies between
    // packet operations), i.e., they are not from the same flow.  But this
    // doesn't happen, and you need to manually do it.
    for (packet_index_t i = 0; i < size; ++i) {
        char *hdr = pkts[i]->hdr;
        ip.src = *((ipv4_t*)(hdr + 14 + 12));
        ip.dst = *((ipv4_t*)(hdr + 14 + 12 + 4));
        ip.src_port = *((uint16_t*)(hdr + 14 + 20 + 0));
        ip.dst_port = *((uint16_t*)(hdr + 14 + 20 + 2));

        //util_hash(&ip, sizeof(ip), &out);
        out = util_hash_ret(&ip, sizeof(ip)) & size_minus_one;

        // Make sure that the tbl_size is a multiple of two
        rte_prefetch0(self->tbl + out);
        tmp[i] = out; 
    }

    for (packet_index_t i = 0; i < size; ++i) {
        self->tbl[tmp[i]]++;
    }

    element_dispatch(ele, 0, pkts, size);
}

void bp_measurement_release(struct element_t *ele) {
    struct bp_measurement_t *self = (struct bp_measurement_t *)ele;
    if (self->tbl) {
        printf("Printing measurement module stats\n");
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

void bp_measurement_report(__attribute__((unused)) struct element_t *_) {
    // VOID
}
