#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

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

static struct miscdevice helloworld_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mydev",
	.fops = &my_dev_fops,
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

	pr_info("hello world init\n");

	ret = misc_register(&helloworld_miscdevice);
	if (ret < 0) {
		pr_info("could not register the misc device");
		return ret;
	}

	pr_info("mydef: got minor %i\n", helloworld_miscdevice.minor);

	return 0;
}

static void __exit hello_exit(void)
{
	pr_info("hello world exit\n");
	misc_deregister(&helloworld_miscdevice);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Guerra");
MODULE_DESCRIPTION("A Hello World misc char driver");
