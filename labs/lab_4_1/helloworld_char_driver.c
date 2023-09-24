#include <linux/export.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#define MY_MAJOR_NUM 202

static int __init hello_init(void);
static void __exit hello_exit(void);
static int my_dev_open(struct inode *inode, struct file *file);
static int my_dev_close(struct inode *inode, struct file *file);
static long my_dev_ioctl(struct file *file, unsigned int cmd,
			 unsigned long arg);

static struct cdev my_dev;

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

	/* Get first device identifier. */
	dev_t dev = MKDEV(MY_MAJOR_NUM, 0);
	pr_info("hello world init\n");

	/* Allocate device numbers. */
	ret = register_chrdev_region(dev, 1, "my_char_device");
	if (ret < 0) {
		pr_info("Unable to allocate major number %d\n", MY_MAJOR_NUM);
		return ret;
	}

	/* Initialize the cdev structure and add it to kernel space. */
	cdev_init(&my_dev, &my_dev_fops);
	ret = cdev_add(&my_dev, dev, 1);
	if (ret < 0) {
		unregister_chrdev_region(dev, 1);
		pr_info("Unable to add cdev\n");
		return ret;
	}

	return 0;
}

static void __exit hello_exit(void)
{
	pr_info("hello world exit\n");
	cdev_del(&my_dev);
	unregister_chrdev_region(MKDEV(MY_MAJOR_NUM, 0), 1);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Guerra");
MODULE_DESCRIPTION("A Hello World char driver");
