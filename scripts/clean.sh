#!/bin/bash

source config.sh

# Remove all driver build artefacts.
if [ -d $ELDD_LAB_DIR ]
then
    pushd $ELDD_LAB_DIR > /dev/null
        echo "cleaning '$ELDD_LAB_DIR'..."
        make KERNEL_DIR=$ELDD_KERNEL_DIR clean
        echo "done"
    popd > /dev/null
fi
