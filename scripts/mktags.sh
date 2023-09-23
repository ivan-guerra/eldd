#!/bin/bash

# This script generates the cscope and tags files for the kernel source tree's
# arm64 sources.
# https://stackoverflow.com/questions/33676829/vim-configuration-for-linux-kernel-development

source config.sh

MakeCScopeFiles()
{
    echo "creating kernel cscope.files for arm64, this may take a minute..."

    # Create cscope.files for generic kernel sources and sources found under
    # arch/arm64.
    find $ELDD_KERNEL_DIR                                      \
        -path "$ELDD_KERNEL_DIR/arch*"            -prune -o    \
        -path "$ELDD_KERNEL_DIR/tmp*"             -prune -o    \
        -path "$ELDD_KERNEL_DIR/Documentation*"   -prune -o    \
        -path "$ELDD_KERNEL_DIR/scripts*"         -prune -o    \
        -path "$ELDD_KERNEL_DIR/tools*"           -prune -o    \
        -path "$ELDD_KERNEL_DIR/include/config*"  -prune -o    \
        -path "$ELDD_KERNEL_DIR/usr/include*"     -prune -o    \
        -type f                                       \
        -not -name '*.mod.c'                          \
        -name "*.[chsS]" -print > cscope.files
    find $ELDD_KERNEL_DIR/arch/arm64                             \
        -path "$ELDD_KERNEL_DIR/arch/arm64/configs" -prune -o    \
        -path "$ELDD_KERNEL_DIR/arch/arm64/kvm"     -prune -o    \
        -path "$ELDD_KERNEL_DIR/arch/arm64/xen"     -prune -o    \
        -type f                                       \
        -not -name '*.mod.c'                          \
        -name "*.[chsS]" -print >> cscope.files
}

MakeCScopeIndex()
{
    # -b Build cross-reference only.
    # -q Enable fast symbol lookup via an inverted index.
    # -k Don't index the C standard library.
    cscope -b -q -k
}

MakeCTags()
{
    # Speeds things up to use the existing cscope.files.
    ctags -L cscope.files
}

InstallIndices()
{
    # Move indices to the root of the project.
    mv cscope.in.out cscope.out cscope.po.out $ELDD_PROJECT_PATH
    mv tags $ELDD_PROJECT_PATH
}

Main()
{
    # Verify cscope is installed.
    if ! command -v cscope --help &> /dev/null
    then
        echo "${LRED}error cscope is not installed${NC}"
        exit 1
    fi

    # Verify ctags is installed.
    if ! command -v ctags --help &> /dev/null
    then
        echo "${LRED}error 'ctags' is not installed${NC}"
        exit 1
    fi

    MakeCScopeFiles
    MakeCScopeIndex
    MakeCTags
    InstallIndices

    # Cleanup, we don't need to keep the cscope.files around.
    rm cscope.files
}

Main
