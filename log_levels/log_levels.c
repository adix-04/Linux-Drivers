#include<linux/module.h>
#include<linux/init.h>

static int __init myinit(void)
{
   printk(KERN_INFO " kernal info. . . . .\n");
   printk(KERN_ALERT " kernal alert. . . . .\n");
   printk(KERN_ERR " kernal error. . . . .\n");
   printk(KERN_CRIT "entering kernal. . . . .\n");

   return 0;
}

static void __exit myexit(void)
{
    printk(KERN_EMERG "exiting kernal. . . . . ..\n");
    printk(KERN_DEBUG"value while exiting : %d\n",45);
}

module_init(myinit);
module_exit(myexit);

MODULE_AUTHOR("adin");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("test module");