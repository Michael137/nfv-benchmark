#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "rte_ethdev.h"

#include "benchmark.h"
#include "dataplane.h"
#include "jit.h"
#include "log.h"
#include "packets.h"
#include "pipeline.h"

#include "rte_cycles.h"
#include "rte_prefetch.h"

int datapath_init(int argc, char **argv, struct dataplane_port_t **port);
void datapath_teardown(struct dataplane_port_t *port);
/* TODO:
 * Normal vs. DDOS distribution packet size distribution
 */

void test_benchmark();
void test_benchmark() {
    uint32_t packet_count = 1<<10;
    struct packet_pool_t *pool = packets_pool_create(packet_count, PACKET_SIZE);

    // Create a zipfian distribution for source/destination ip address
    packets_pool_zipfian(pool, 0, packet_count - 1, 26, 8, 0.5);

    // Compile and load the checksum-drop module
    struct jit_t jit = {0};
    jit_test_load(&jit, "checksum-routing");

    for (int i=0; i<=3; i++) {
    // Benchmark the running time of the jitted test
    // Put a memory barrier for benchmarks
    uint32_t repeat = 40;
    asm volatile ("mfence" ::: "memory");
    uint64_t cycles = rte_get_tsc_cycles();
    (*jit.entry.test)(pool, repeat, i);
    asm volatile ("mfence" ::: "memory");
    printf("num cycles per packet (%.2f)\n", (float)(rte_get_tsc_cycles() - cycles)/(float)(packet_count * repeat));
    }

    // Unload once done
    jit_test_unload(&jit);
    packets_pool_delete(&pool);
}

int datapath_init(int argc, char **argv, struct dataplane_port_t **port) {
    *port = 0;
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Failed to initialize the EAL.");

    const char port_name[] = PORT_NAME;
    log_info_fmt("Num available dpdk ports (i.e., number of usable ethernet devices): %d", rte_eth_dev_count());

    struct dataplane_port_t *pport = 0;
    ret = port_configure(port_name, &pport);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Failed to configure port %s", port_name);
    log_info_fmt("Port [%s] configured successfully.", port_name);
    *port = pport;
    return ret;
}

void datapath_teardown(struct dataplane_port_t *port) {
    port_release(port);
    rte_eal_cleanup();
}

int main(int argc, char **argv) {
    // Deterministic experiments are the best experiments - one can only hope.
    srand(0);

    struct dataplane_port_t *port = 0;
    int ret = datapath_init(argc, argv, &port);
    argc -= ret;
    argv += ret;

    if (!port) 
        return 0;

    test_benchmark();

    datapath_teardown(port);
    return 0;
}
