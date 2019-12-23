#!/bin/bash

cp patches/dpdk-18.02/common_base ../dpdk-18.02/config/common_base

HOSTNAME=$(hostname)
AUTO_CMD="./${HOSTNAME}_dpdk_auto_setup.exp"

${AUTO_CMD}

cd ../dpdk-18.02/x86_64-native-linuxapp-gcc
make install DESTDIR=$PWD/../build
