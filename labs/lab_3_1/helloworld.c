#include <linux/module.h>

static int __init hello_init(void);
static void __exit hello_exit(void);

static int __init hello_init(void)
{
	pr_info("hello world init\n");
	return 0;
}

static void __exit hello_exit(void)
{
	pr_info("hello world exit\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Guerra");
MODULE_DESCRIPTION("A Hello World module");
