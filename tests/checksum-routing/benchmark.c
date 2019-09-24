#include "benchmark_helper.h"
#include "benchmark.h"
#include "defaults.h"

void benchmark_config_init(struct benchmark_t *bench) {
    struct pipeline_t *pipe = 0;

    bench->pipeline = pipe = pipeline_create();

    pipeline_element_add(pipe, el_checksum_create(CHECKSUM_BUFFER_SIZE));
    pipeline_element_add(pipe, el_routing_create_with_file(ROUTING_BUFFER_SIZE, "data/boza_rtr_route.lpm"));
    pipeline_element_add(pipe, el_drop_create(DROP_BUFFER_SIZE));
}

void bp_benchmark_config_init(struct benchmark_t *bench) {
    struct pipeline_t *pipe = 0;

    bench->pipeline = pipe = pipeline_create();

    pipeline_element_add(pipe, el_bp_checksum_create(CHECKSUM_BUFFER_SIZE));
    pipeline_element_add(pipe, el_bp_routing_create_with_file(ROUTING_BUFFER_SIZE, "data/boza_rtr_route.lpm"));
    pipeline_element_add(pipe, el_drop_create(DROP_BUFFER_SIZE));
}

void bpp_benchmark_config_init(struct benchmark_t *bench) {
    struct pipeline_t *pipe = 0;

    bench->pipeline = pipe = pipeline_create();

    pipeline_element_add(pipe, el_bpp_checksum_create(CHECKSUM_BUFFER_SIZE));
    pipeline_element_add(pipe, el_bpp_routing_create_with_file(ROUTING_BUFFER_SIZE, "data/boza_rtr_route.lpm"));
    pipeline_element_add(pipe, el_drop_create(DROP_BUFFER_SIZE));
}

void naive_benchmark_config_init(struct benchmark_t *bench) {
    struct pipeline_t *pipe = 0;

    bench->pipeline = pipe = pipeline_create();

    pipeline_element_add(pipe, el_naive_checksum_create(CHECKSUM_BUFFER_SIZE));
    pipeline_element_add(pipe, el_naive_routing_create_with_file(ROUTING_BUFFER_SIZE, "data/boza_rtr_route.lpm"));
    pipeline_element_add(pipe, el_drop_create(DROP_BUFFER_SIZE));
}
