#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "rte_ethdev.h"
#include "rte_malloc.h"

#include "benchmark.h"
#include "benchmark_helper.h"
#include "dataplane.h"
#include "pipeline.h"

struct pipeline_t *pipeline_build(struct rx_packet_stream *);

struct pipeline_t *pipeline_build(struct rx_packet_stream *stream) {
    struct pipeline_t *pipe = pipeline_create();
    pipeline_element_add(pipe, el_bypass_timer_create(MOD_BUFFER_SIZE_1));
    pipeline_element_add(pipe, el_bypass_bpp_measurement_create_with_size(MOD_BUFFER_SIZE_2, 
        1 << 18));
    pipeline_element_add(pipe, el_bypass_bpp_routing_create_with_file(MOD_BUFFER_SIZE_3, 
        "data/boza_rtr_route.lpm"));
    pipeline_element_add(pipe, el_bypass_bp_checksum_create(MOD_BUFFER_SIZE_4));
    pipeline_element_add(pipe, el_bypass_drop_mbuf_create(MOD_BUFFER_SIZE_DROP, stream));
    return pipe;
}
