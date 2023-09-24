#!/bin/bash

# Cross compile the kernel, modules, and dtbs for ARM64. Configure the kernel
# on first build and optionally thereafter. Note, the default configuration is
# the bcm2711_defconfig which is what is recommended for the Raspberry Pi Model
# 3b+.

source config.sh

ConfigKernel()
{
    pushd $ELDD_KERNEL_DIR > /dev/null
    make KERNEL=kernel8 \
        ARCH=arm64 \
        CC=clang \
        CROSS_COMPILE=aarch64-linux-gnu- \
        bcm2711_defconfig && \
        make ARCH=arm64 \
        CC=clang \
        CROSS_COMPILE=aarch64-linux-gnu- \
        nconfig
    popd
}

BuildKernel()
{
    pushd $ELDD_KERNEL_DIR > /dev/null
    make ARCH=arm64 \
        CC=clang \
        CROSS_COMPILE=aarch64-linux-gnu- \
        -j$(nproc) \
        Image modules dtbs
    popd > /dev/null
}

Main()
{
    if [[ ! -f "${ELDD_KERNEL_DIR}/.config" ]]; then
        # Missing kernel config, create one.
        ConfigKernel
    else
        # A .config already exists. Prompt the User in case they want to
        # create a new config with this build.
        read -p "Do you want to generate a new kernel .config? [y/n] " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            ConfigKernel
        fi
    fi

    BuildKernel
}

Main
