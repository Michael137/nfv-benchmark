// This file was automatically generated at 04/11/2019 06:12:05
#ifndef MERGED_IN_H
#define MERGED_IN_H
#include "elements/routing.h"
#include "element.h"
struct packet_t;
struct merged_t {
/* This block should match routing_t's structure */
struct element_t element;
uint64_t _tmp_2;
struct _routing_tbl_256_t fst_tbl;
/* until here */
// Measurement
size_t tbl_size;
uint32_t _tmp[MEASUREMENT_BUFFER_SIZE];
uint32_t* tbl;
// Checksum
uint64_t checksum_count;
uint64_t port_count;
// Misc.
uint32_t a, b, c;
uint64_t slowpass_count;
};
struct merged_t* merged_create(void);
void merged_process(struct element_t*, struct packet_t**, packet_index_t);
void merged_release(struct element_t*);
void merged_report(struct element_t*);
#endif // MERGED_IN_H
