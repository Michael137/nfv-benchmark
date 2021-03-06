#include "benchmark_helper.h"
#include "benchmark.h"
#include "defaults.h"

void benchmark_config_init(struct benchmark_t *bench) {
    struct pipeline_t *pipe = 0;
    bench->pipeline = pipe = pipeline_create();

    pipeline_element_add(pipe, el_measurement_create_with_size(MOD_BUFFER_SIZE_1, (10 * 1<<20)));
    pipeline_element_add(pipe, el_drop_create(MOD_BUFFER_SIZE_2));
}

void batching_benchmark_config_init(struct benchmark_t *bench) {
    struct pipeline_t *pipe = 0;
    bench->pipeline = pipe = pipeline_create();

    pipeline_element_add(pipe, el_batching_measurement_create_with_size(MOD_BUFFER_SIZE_1, (10 * 1<<20)));
    pipeline_element_add(pipe, el_drop_create(MOD_BUFFER_SIZE_2));
}

void bp_benchmark_config_init(struct benchmark_t *bench) {
    struct pipeline_t *pipe = 0;
    bench->pipeline = pipe = pipeline_create();

    pipeline_element_add(pipe, el_bp_measurement_create_with_size(MOD_BUFFER_SIZE_1, (10 * 1<<20)));
    pipeline_element_add(pipe, el_drop_create(MOD_BUFFER_SIZE_2));
}

void bpp_benchmark_config_init(struct benchmark_t *bench) {
    struct pipeline_t *pipe = 0;
    bench->pipeline = pipe = pipeline_create();

    pipeline_element_add(pipe, el_bpp_measurement_create_with_size(MOD_BUFFER_SIZE_1, (10 * 1<<20)));
    pipeline_element_add(pipe, el_drop_create(MOD_BUFFER_SIZE_2));
}

void naive_benchmark_config_init(struct benchmark_t *bench) {
    struct pipeline_t *pipe = 0;
    bench->pipeline = pipe = pipeline_create();

    pipeline_element_add(pipe, el_naive_measurement_create_with_size(1, (10 * 1<<20)));
    pipeline_element_add(pipe, el_drop_create(MOD_BUFFER_SIZE_2));
}
