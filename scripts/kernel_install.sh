#!/bin/bash

# This script installs the kernel image, dtbs, overlays, and modules to a RPI
# SD card. The script expects two arguments: the path to the mounted boot
# partition and the path to the mounted ext4 partition (in that order). You
# will need to run this script with root permissions. Example usage:
# sudo ./kernel_install /mnt/fat32 /mnt/ext4

source config.sh

InstallModules()
{
    ROOTFS_PART=$1
    pushd $ELDD_KERNEL_DIR > /dev/null
        make ARCH=arm64 \
             CROSS_COMPILE=aarch64-linux-gnu- \
             INSTALL_MOD_PATH=$ROOTFS_PART modules_install
    popd > /dev/null
}

InstallImageAndDtbs()
{
    BOOT_PART=$1

    pushd $ELDD_KERNEL_DIR > /dev/null
        cp -v arch/arm64/boot/Image $BOOT_PART/kernel-rpi.img
        cp -v arch/arm64/boot/dts/broadcom/*.dtb $BOOT_PART
        cp -v arch/arm64/boot/dts/overlays/*.dtb* $BOOT_PART/overlays/
        cp -v arch/arm64/boot/dts/overlays/README $BOOT_PART/overlays/
    popd > /dev/null
}

Main()
{
    BOOT_PART=$1
    if [[ ! -d $BOOT_PART ]]; then
        echo "error missing BOOT_PART: '$BOOT_PART' does not exist"
        exit 1
    fi

    ROOTFS_PART=$2
    if [[ ! -d $ROOTFS_PART ]]; then
        echo "error missing ROOTFS_PART: '$ROOTFS_PART' does not exist"
        exit 1
    fi

    InstallModules $ROOTFS_PART
    InstallImageAndDtbs $BOOT_PART

    read -p "Do you want to eject the SD card? [y/n] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        umount $BOOT_PART
        umount $ROOTFS_PART
    fi
}

if [[ $EUID > 0 ]]; then
    echo "error: you must run kernel_install.sh with root permissions"
    exit 1
fi

if [[ "$#" -ne 2 ]]; then
    echo "error: invalid argument count"
    echo "usage: sudo kernel_install.sh BOOT_PATH ROOTFS_PATH"
    exit 1
fi

Main $1 $2
