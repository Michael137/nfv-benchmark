#!/bin/bash

HOSTNAME=$(hostname)
AUTO_CMD="./${HOSTNAME}_dpdk_auto_setup.exp"

${AUTO_CMD}

cd ../dpdk-18.02/x86_64-native-linuxapp-gcc
make install DESTDIR=$PWD/../build
