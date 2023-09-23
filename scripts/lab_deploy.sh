#!/bin/bash

# This script copies all *.ko files to the Raspberry Pi via scp.  Set the
# variables at the top of the script to match your Pi username, hostname, etc.

source config.sh

RPI_USERNAME="rdev"
RPI_HOSTNAME="raspberrypi"
# Use of an SSH key allows for passwordless ssh, scp, etc. from the dev PC to
# the Raspberry Pi.
# https://alvinalexander.com/linux-unix/how-use-scp-without-password-backups-copy/
RPI_SSH_KEY="$HOME/.ssh/rpi_rsa"
MODULE_INSTALL_DIR="/home/rdev/"

pushd $ELDD_LAB_DIR > /dev/null
find . -name *.ko \
    -exec scp -i $RPI_SSH_KEY {} $RPI_USERNAME@$RPI_HOSTNAME:$MODULE_INSTALL_DIR \;
popd > /dev/null
