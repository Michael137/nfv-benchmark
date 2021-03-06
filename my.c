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
#include "opt.h"

#include "rte_cycles.h"
#include "rte_prefetch.h"

#include "poisson.h"
#include "optgen.h"

int datapath_init(int argc, char **argv, struct dataplane_port_t **port);
void datapath_teardown(struct dataplane_port_t *port);
/* TODO:
 * Normal vs. DDOS distribution packet size distribution
 */

void test_benchmark(char const*);
void test_benchmark(char const *name) {
    printf("cycles for %s\n", name);
    uint32_t packet_count = 1<<10;
    struct packet_pool_t *pool = packets_pool_create(packet_count, PACKET_SIZE);

    // Compile and load the checksum-drop module
    struct jit_t jit = {0};
    jit_test_load(&jit, name);

    for (int j=0; j<2; j++) {
    for (int d=0; d<=2; d++) {
      if (d==0 && j==1) {
	continue;
      }
      if (d==1) {
	if (d==1) printf("cycles ZIPF\n");
	// Create a zipfian distribution for source/destination ip address
	packets_pool_zipfian(pool, 0, packet_count - 1, 26, 8, 0.5);
      }
      if (d==2) {
	printf("cycles UNIFORM DIST\n");
	packet_t *pkt_first = NULL;
	for (packet_t *pkt = packets_pool_first(pool); 
	     pkt < pool->end; pkt = (packet_t*)(pkt->data + pool->size)) {
	  if (pkt_first == NULL) {
	    pkt_first = pkt;
	  } else {
	    memcpy(pkt->data, pkt_first->data, pkt->size);
	  }
	}
      }
    for (int o=Naive; o<=Opt; o++) {
      // Benchmark the running time of the jitted test
      // Put a memory barrier for benchmarks
      uint32_t repeat = 40;
      asm volatile ("mfence" ::: "memory");
      uint64_t cycles = rte_get_tsc_cycles();
      (*jit.entry.test)(pool, repeat, o);
      asm volatile ("mfence" ::: "memory");
      printf("num cycles per packet (%.2f)\n", (float)(rte_get_tsc_cycles() - cycles)/(float)(packet_count * repeat));
    }
    }
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
//    srand(0);
//
//    struct dataplane_port_t *port = 0;
//    int ret = datapath_init(argc, argv, &port);
//    argc -= ret;
//    argv += ret;
//
//    if (!port) 
//        return 0;
//
//    test_benchmark("checksum-checksum");
//    test_benchmark("checksum-drop");
//    test_benchmark("checksum-rfile");
//    test_benchmark("checksum-routing");
//    test_benchmark("mea_checksum-rfile");
//    test_benchmark("mea_rfile-checksum");
//    test_benchmark("measurement-drop");
//    test_benchmark("rfile_checksum-mea");
//    test_benchmark("rfile-drop");
//    //test_benchmark("routing-drop");
//    
//    datapath_teardown(port);

	parse_spec();
    return 0;
}
