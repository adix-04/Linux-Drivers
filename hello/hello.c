#include<linux/module.h>
#include<linux/init.h>

static int __init myinit(void)
{
   printk("Hello:entering kernal. . . . .\n");
   return 0;
}

static void __exit myexit(void)
{
    printk("Hello:exiting  .. . . . ..\n");
}


module_init(myinit);
module_exit(myexit);

MODULE_AUTHOR("adin");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("test module");