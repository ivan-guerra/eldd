#include <linux/ktime.h>
#include <linux/module.h>

static int __init hello_init(void);
static void __exit hello_exit(void);

static time64_t start;
static time64_t end;

static int __init hello_init(void)
{
	start = ktime_get_seconds();
	return 0;
}

static void __exit hello_exit(void)
{
	end = ktime_get_seconds();
	pr_info("unloading module after %lld seconds\n", end - start);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Guerra");
MODULE_DESCRIPTION(
	"A Hello World module that prints the time since it was loaded");
