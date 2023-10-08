# Linux Driver Development for Embedded Processors

This project contains a subset of the lab modules introduced throughout the
[Linux Driver Development for Embedded Processors][1] text. The examples in
this repo target the [Raspberry Pi 3 Model B+][2]. All the labs were built and
tested using the [rpi-6.1.y][3] kernel.

### Checking Out the Project

This project includes the `rpi-6.1.y` raspberrypi Linux kernel branch as a
submodule. There's a lot of code and a lot of git history that comes along with
the `rpi-6.1.y` submodule. To speed up the checkout process and save space on
your PC, you may want to follow these steps when cloning the repo:
```
$ git clone git@github.com:ivan-guerra/eldd.git
$ cd eldd
$ git submodule init
$ git submodule update --depth 1
```
You can change the argument to `--depth` to include however much history you
like or remove it completey for the full history.

### Building and Deploying the Kernel

First, install the kernel dependencies and cross compilation tools required to
build the Linux kernel. These are defined in the ["Cross Compiling the
Kernel"][5] section of the Rapsberry Pi docs.

Build and optionally configure the kernel using the included
`scripts/kernel_build.sh` script. `kernel_build.sh` will apply the
`bcm2711_defconfig` on first run.   

> **Note** 
> You must manually apply the kernel configs described in Chapter 1 pg. 52 for
> all the lab drivers to function properly!

To deploy your kernel, mount the boot and rootfs partitions of your Pi's SD card:

```bash
mount /dev/sdb1 /mnt/fat32
mount /dev/sdb2 /mnt/ext4
```

**Be sure to pick partition and mount point names that make sense for your system!**

You can use the `scripts/kernel_install.sh` script to install the kernel image,
dtbs, and modules to the appropriate partitions. The `kernel_install.sh` script
expects two arguments: the boot mount point followed by the rootfs mount point
(in that order). For example,

```bash
sudo ./kernel_install /mnt/boot /mnt/ext4
```

### Building and Deploying the Labs 

You can build all the lab modules by running the `scripts/lab_build.sh` script.
For the build to succeed, you must have successfully followed the instructions
in ["Building and Deploying the Kernel"](#building-and-deploying-the-kernel).
Post build, a `*.ko` kernel module will be present in each
`lab_<CHAPTER_NUM>_<LAB_NUM>`. You can secure copy `*.ko` modules in bulk from
host to RPI using `scripts/lab_deploy.sh`. See the comments in
[`lab_deploy.sh`](scripts/lab_deploy.sh) for more info.

### Device Tree Overlays

Throughout the text, the author directly modifies the BCM2711 dts files. You
can do the same if you like. Optionally, you can apply the included Device Tree
Overlays to achieve the same effect and not litter the original dts files.

Relevant lab overlays are included in [`overlays/`](overlays/). To apply an
overlay, copy the overlay's `*.dts` file to
`linux/arch/arm64/boot/dts/overlays` and then edit the `Makefile` to include
your overlay.

Follow the instructions in ["Building and Deploying the
Kernel"](#building-and-deploying-the-kernel) to build and install your new
overlay to the SD card.

Edit the `config.txt` in the boot directory of your Pi's SD card to include a `dtoverlay` entry specifying your new overlay. For example:

```txt
dtoverlay=my-overlay
```

Boot your Pi off the SD card and your new overlay will automatically be applied during boot.

### Source Material

The author has made these and many other labs freely availably on his GitHub
page. Included in his repo are lab drivers for processors other than the RPI's
BCM2837: [linux_book_2nd_edition][4]

[1]: https://www.amazon.com/Linux-Driver-Development-Embedded-Processors/dp/1729321828
[2]: https://www.raspberrypi.com/products/raspberry-pi-3-model-b-plus/
[3]: https://github.com/raspberrypi/linux/tree/rpi-6.1.y
[4]: https://github.com/ALIBERA/linux_book_2nd_edition
[5]: https://www.raspberrypi.com/documentation/computers/linux_kernel.html#cross-compiling-the-kernel
