PWD := $(shell pwd)

kbuild:
	${MAKE} -C ${KERNEL_DIR} \
		ARCH=arm64 \
		CROSS_COMPILE=aarch64-linux-gnu- \
		M=${PWD}

clean:
	${MAKE} -C ${KERNEL_DIR} M=${PWD} SUBDIRS=${PWD} clean
