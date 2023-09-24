#include <linux/device/class.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>

#define DEVICE_NAME "mydev"
#define CLASS_NAME "hello_class"

dev_t dev;
static struct cdev my_dev;
static struct class *hello_class;

static int __init hello_init(void);
static void __exit hello_exit(void);
static int my_dev_open(struct inode *inode, struct file *file);
static int my_dev_close(struct inode *inode, struct file *file);
static long my_dev_ioctl(struct file *file, unsigned int cmd,
			 unsigned long arg);

static const struct file_operations my_dev_fops = {
	.owner = THIS_MODULE,
	.open = my_dev_open,
	.release = my_dev_close,
	.unlocked_ioctl = my_dev_ioctl,
};

static int my_dev_open(struct inode *inode, struct file *file)
{
	pr_info("my_dev_open() is called.\n");
	return 0;
}

static int my_dev_close(struct inode *inode, struct file *file)
{
	pr_info("my_dev_close() is called.\n");
	return 0;
}

static long my_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	pr_info("my_dev_ioctl() is called. cmd = %d, arg = %ld\n", cmd, arg);
	return 0;
}

static int __init hello_init(void)
{
	int ret;

	dev_t dev_no;
	int major;
	struct device *hello_device;

	pr_info("hello world init\n");

	/* Allocate dynamically device numbers, only one in this driver */
	ret = alloc_chrdev_region(&dev_no, 0, 1, DEVICE_NAME);
	if (ret < 0) {
		pr_info("Unable to allocate major number\n");
		return ret;
	}

	/* 
   * Get the device identifiers using MKDEV. We are doing it for 
   * teaching purposes as we only use one identifier in this 
   * driver and dev_no could be used as parameter for cdev_add()
   * and device_create() without needing to use the MKDEV macro.
   */

	/* Get device identifiers. */
	major = MAJOR(dev_no);
	dev = MKDEV(major, 0);

	pr_info("Allocated correctly with major number %d\n", major);

	/* Initialize the cdev structure and add it to kernel space. */
	cdev_init(&my_dev, &my_dev_fops);
	ret = cdev_add(&my_dev, dev, 1);
	if (ret < 0) {
		unregister_chrdev_region(dev, 1);
		pr_info("Unable to add cdev\n");
		return ret;
	}

	/* Register the device class. */
	hello_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(hello_class)) {
		unregister_chrdev_region(dev, 1);
		cdev_del(&my_dev);
		pr_info("Failed to register device class\n");
		return PTR_ERR(hello_class);
	}

	/* Create a device node named DEVICE_NAME associated a dev. */
	hello_device = device_create(hello_class, NULL, dev, NULL, DEVICE_NAME);
	if (IS_ERR(hello_device)) {
		class_destroy(hello_class);
		cdev_del(&my_dev);
		unregister_chrdev_region(dev, 1);
		pr_info("Failed to create the device\n");
		return PTR_ERR(hello_device);
	}
	pr_info("The dvice is created correctly\n");

	return 0;
}

static void __exit hello_exit(void)
{
	device_destroy(hello_class, dev);
	class_destroy(hello_class);
	cdev_del(&my_dev);
	unregister_chrdev_region(dev, 1);
	pr_info("hello world exit\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Guerra");
MODULE_DESCRIPTION("A Hello World char driver using devtmpfs");
