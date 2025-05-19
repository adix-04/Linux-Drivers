#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
static int major;
static ssize_t myread (struct file *, char __user *, size_t, loff_t *){
    printk("Cdev:read called. . . .\n");
    return 0;
}
static int my_open (struct inode *inode, struct file *filep){
     printk(KERN_INFO "file major :%d minor :%d\n",imajor(inode),iminor(inode));
     printk(KERN_INFO "file position file->flags : %lld\n",filep->f_pos);
     printk(KERN_INFO "file permissions :0x%x\n",filep->f_mode);
     printk(KERN_INFO "file flags :0x%x\n",filep->f_flags);
     return 0;
}

static int my_close (struct inode *inode, struct file *filep){
    printk(KERN_INFO"file is closed  \n");
    return 0;
}
static struct  file_operations fops={
    .read = myread,
    .open = my_open,
    .release = my_close
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
    printk(KERN_INFO "Cdev:exiting  .. . . . ..\n");
}
module_init(myinit);
module_exit(myexit);

MODULE_AUTHOR("adin");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Char device module");