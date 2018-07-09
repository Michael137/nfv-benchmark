#ifndef _BENCHMARK_HELPER_H_
#define _BENCHMARK_HELPER_H_

#include "types.h"
#include "dataplane.h"
#include "packets.h"

struct element_t;

struct element_t *el_only_merged_fastpass_create(char const *fname, size_t tbl_size);
struct element_t *el_only_merged_create(char const *fname, size_t tbl_size);
struct element_t *el_only_fastpass_create(struct rx_packet_stream *);
struct element_t *el_only_identity_create(void);
struct element_t *el_only_timer_create(void);
struct element_t *el_only_drop_create(void);
struct element_t *el_only_checksum_create(void);
struct element_t *el_only_routing_create(void);
struct element_t *el_only_routing_create_with_file(char const *fname);
struct element_t *el_only_measurement_create_with_size(size_t tbl_size);
struct element_t *el_only_drop_mbuf_create(struct rx_packet_stream *);

struct element_t *el_bypass_merged_fastpass_create(packet_index_t size, char const *fname, size_t tbl_size);
struct element_t *el_bypass_merged_create(packet_index_t size, char const *fname, size_t tbl_size);
struct element_t *el_bypass_fastpass_create(packet_index_t, struct rx_packet_stream *);
struct element_t *el_bypass_identity_create(packet_index_t size);
struct element_t *el_bypass_timer_create(packet_index_t size);
struct element_t *el_bypass_drop_create(packet_index_t size);
struct element_t *el_bypass_checksum_create(packet_index_t size);
struct element_t *el_bypass_routing_create(packet_index_t size);
struct element_t *el_bypass_routing_create_with_file(packet_index_t size, char const *fname);
struct element_t *el_bypass_measurement_create_with_size(packet_index_t size, size_t tbl_size);
struct element_t *el_bypass_drop_mbuf_create(packet_index_t size, struct rx_packet_stream *);

struct element_t *el_merged_fastpass_create(packet_index_t size, char const *fname, size_t tbl_size);
struct element_t *el_merged_create(packet_index_t size, char const *fname, size_t tbl_size);
struct element_t *el_fastpass_create(packet_index_t, struct rx_packet_stream *);
struct element_t *el_identity_create(packet_index_t size);
struct element_t *el_timer_create(packet_index_t size);
struct element_t *el_drop_create(packet_index_t size);
struct element_t *el_checksum_create(packet_index_t size);
struct element_t *el_routing_create(packet_index_t size);
struct element_t *el_routing_create_with_file(packet_index_t size, char const *fname);
struct element_t *el_measurement_create_with_size(packet_index_t size, size_t tbl_size);
struct element_t *el_drop_mbuf_create(packet_index_t size, struct rx_packet_stream *);

#endif // _BENCHMARK_HELPER_H_
