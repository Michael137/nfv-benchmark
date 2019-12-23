# nfv-benchmark

Adopted to continue project on NFV

## Instructions
- Setup DPDK in sibling directory ([zip](https://github.com/DPDK/dpdk/archive/v18.02.zip), ([docs](https://dpdk.readthedocs.io/en/v2.2.0/linux_gsg/intro.html)))
### Manual Installation
* edit config/common_base
  + CONFIG_RTE_BUILD_SHARED_LIB=y
  + CONFIG_RTE_EAL_PMD_PATH="./build/pmdlib" (use the absolute path of DPDK project instead of .)
  + (for MLX4 support)
  + CONFIG_RTE_LIBRTE_MLX4_PMD=y
  + CONFIG_RTE_LIBRTE_MLX4_DEBUG=y
  + CONFIG_RTE_LIBRTE_MLX4_DLOPEN_DEPS=y
* `make config T=x86_64-native-linuxapp-gcc`
* `make`
* in build, copy the drivers from lib to pmdlib, not all should be copied. see pmlib.txt for a list that works.
  + E.g., using this command: cd dpdk-18.02/build; mkdir pmdlib; awk '{print $1"\n"$2}' ../../nfv-benchmark/pmdlib.txt | xargs -I {} cp lib/{} pmdlib
As root:
* `sudo ./setup_hugepages.sh`
* `cat /proc/meminfo | grep Huge` should show some Free pages, e.g.:
```
AnonHugePages:         0 kB
HugePages_Total:    1024
HugePages_Free:     1024
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
```
* If not:
* `sudo umount -a -t hugetlbfs`
* `sudo mount -t hugetlbfs nodev /mnt/huge`
* can also do `sudo rm -rf /mnt/huge/*` to free once mounted

- Configure NIC:
  * on zu: sudo modprobe -a ib_uverbs mlx4_en mlx4_core mlx4_ib
- Edit Makefile:
  * Pick whether you need MLX4 support
  * Pick PORT_NAME
- Shared Libraries (DPDK):
  * Add dpdk-18.02/build/lib to your LD_LIBRARY_PATH in case the librte_*.so* libraries can't be located by ld
    + run `ldconfig dpdk-18.02/build/lib` if needed

### Installation Using DPDK Usertools
* Edit `config/common_base`:
  + set `CONFIG_RTE_MAX_MEMSEG=4096`
  + set `CONFIG_RTE_EAL_IGB_UIO=y`
* `cd dpdk-18.02/usertools`
* `sudo ./dpdk-setup.sh`
* Build application (14)
  + pick the `x86_64-native-linuxapp-gcc` target
* Insert the `igb_uio` module
* Setup huge pages for non-numa machine
  + Select **4096**
* Bind the ports to dpdk (23)
  + The PCI address is the port number to bind (e.g. 0000:89:00.0)
* Exit the setup utility
* `cd dpdk-18.02/x86_64-native-linuxapp-gcc`
* `make install DESTDIR=$PWD/../build`
  + Adjust makefile to point to correct headers and libraries

### Installation Using Automation Scripts
* This expects a dpdk-18.02 directory in the same parent directory as nfv-benchmark
* From the root directory (nfv-benchmark) run: `./setup.sh`
* Internally the setup script runs following [expect](https://likegeeks.com/expect-command/#Expect-Command) scripts depending on the server:
  * `./rumi_dpdk_auto_setup.exp`
  * or
  * `./dori_dpdk_auto_setup.exp`

## Troubleshooting
### Using **testpmd**
* `cd·dpdk-18.02/x86_64-native-linuxapp-gcc/app`
* `sudo ./testpmd -c 0xff -n 4 -- -i`
  + Try adding `-w 0000:89:00.0` to the testpmd arguments (i.e., add the port number bound with the dpdk-setup utility)

### Check whether one machine can ping the other
* Identify ethernet interface using `ifconfig`
  + If it doesn't show up check using `ifconfig -a` and enable the interface first using e.g. `sudo ifconfig enp137s0f0 up`
* Assign a suitable unused IP address to the interface. E.g., `sudo ifconfig enp137s0f0 172.16.0.1/16`
* On the other machine do the same (change the IP slightly, e.g., 172.16.0.2/16)
* Now ping one machine from the other. E.g., on the machine with IP setup to 172.16.0.2/16 run: `ping 172.16.0.1`
  + If the above does not succeed then their possibly exists a connectivity issue between the machines

### Check whether one machine can ping the other
* Identify ethernet interface using `ifconfig`
  + If it doesn't show up check using `ifconfig -a` and enable the interface first using e.g. `sudo ifconfig enp137s0f0 up`
* Assign a suitable unused IP address to the interface. E.g., `sudo ifconfig enp137s0f0 172.16.0.1/16`
* On the other machine do the same (change the IP slightly, e.g., 172.16.0.2/16)
* Now ping one machine from the other. E.g., on the machine with IP setup to 172.16.0.2/16 run: `ping 172.16.0.1`
  + If the above does not succeed then their possibly exists a connectivity issue between the machines

## Build & Run
* On machine 1 run
  + `make clean all rxer`
  + `sudo ./bin/rxer -l1-3 -n4`
* On machine 2 run
  + `make clean all txer`
  + `sudo ./bin/txer`

## TODOs
### Infrastructure
- [x] Build and run anything
  - [x] Fix gcc warnings
- [x] Run benchmarks
- [x] Automate dpdk setup
- [x] Merge rxer-test.c into rxer.c
- [ ] Refactor NF creation logic and rxer/txer
### Language
- [ ] Initial language design
### Experiments
- [ ] Analyze benchmarks
  - [ ] Reproduce behaviour from paper
    - [ ] changing workload effects optimizations significantly
      - [ ] use traffic generator, e.g., iperf for benchmarks
    - [x] hand-written optimizations outperform naive (and compiler optimized) versions
- [ ] Include more types of optimizations
- [ ] Experiment with more packet distributions
- [ ] Be able to switch on/off different pipeline/optimization configurations
  - [x] Write initial framework (optgen.py)
  - [ ] Run experiments
  - [ ] Include more complex optimizations/pipelines other than the ones from the merged benchmark
