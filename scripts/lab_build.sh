#!/bin/bash

# This script assumes the kernel has been built within its source tree.
# See/run kernel_build.sh prior to running this script.

source config.sh

pushd $ELDD_LAB_DIR > /dev/null
make KERNEL_DIR=$ELDD_KERNEL_DIR -j$(nproc) all
popd > /dev/null
