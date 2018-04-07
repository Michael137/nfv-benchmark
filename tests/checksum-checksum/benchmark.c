#include "benchmark_helper.h"
#include "benchmark.h"
#include "defaults.h"

void benchmark_config_init(struct benchmark_t *bench) {
    struct pipeline_t *pipe = 0;
    bench->pipeline = pipe = pipeline_create();

    pipeline_element_add(pipe, el_checksum_create(MOD_BUFFER_SIZE_1));
    pipeline_element_add(pipe, el_checksum_create(MOD_BUFFER_SIZE_2));
    pipeline_element_add(pipe, el_drop_create(DROP_BUFFER_SIZE));
}
