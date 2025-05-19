#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/ioctl.h>

#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)
int32_t value = 0;
static int major;
static ssize_t myread (struct file *, char __user *, size_t, loff_t *){
    printk("Cdev:read called. . . .\n");
    return 0;
}
static long int ioctl(struct file*filep ,unsigned cmd,unsigned long arg)
{
   switch(cmd){
    case WR_VALUE :
            printk(KERN_INFO "write value....\n");
            if(copy_from_user(&value,(int32_t*)arg,sizeof(value))){
                printk(KERN_ERR"DATA WRITE ERROR . . . \n");
            }
            printk(KERN_DEBUG "Vlaue is %d",value);
            break;
    case RD_VALUE:
           printk(KERN_INFO "READ VALUE ....\n");
           if(copy_to_user((int32_t*)arg,&value,sizeof(value))){
            printk(KERN_ERR"ERROR READING...\n");
           }
           break;
    default:
           printk(KERN_INFO"DEFAULT...\n");
           break;
   }
   return 0;
}

static struct  file_operations fops={
    .owner = THIS_MODULE,
    .read = myread,
    .unlocked_ioctl = ioctl

};
static int __init myinit(void)
{
   major = register_chrdev(0,"Cdev",&fops);
   if(major < 0){
    printk(KERN_ERR "Cdev:error registering device. . . .\n");
    return major;
   }
   printk(KERN_INFO "Cdev :entering kernal major number = %d. . . . .\n",major);
   return 0;
}

static void __exit myexit(void)
{
    unregister_chrdev(major,"Cdev...\n");
    printk(KERN_INFO"Cdev:exiting  .. . . . ..\n");
}
module_init(myinit);
module_exit(myexit);

MODULE_AUTHOR("adin");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Char device module");