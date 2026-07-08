#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

/*Module's init entry point */
static int __init helloworld_init(void)
{
  printk(KERN_INFO "Hello World! Module loaded.\n");
  //pr_info("Hello world\n");
  return 0;
}

/*Module's cleanup entry point */
static void __exit helloworld_cleanup(void)
{
  printk(KERN_INFO "Goodbye World! Module unloaded.\n");
  //pr_info("Good bye world\n");
}

module_init(helloworld_init);
module_exit(helloworld_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zihan Lin");
MODULE_DESCRIPTION("A simple hello world LKM");
MODULE_VERSION("0.1");
