/**
 * creating a file in proc file system and performing operations
 */
#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/proc_fs.h>

static int major;
static char text [65]; 
static ssize_t myread (struct file *filep, char __user *userbuff, size_t ln, loff_t *off){
    int not_copied,delta,to_copy = (ln + *off ) < sizeof(text) ? ln : (sizeof(text) - *off);
    printk(KERN_INFO "PROC : read called. . . .\n");
    if(*off >= sizeof(text))
            return 0;
    
    not_copied = copy_to_user(userbuff , &text[*off] , to_copy);
    delta = to_copy - not_copied;
    if(not_copied)
            printk(KERN_WARNING "PROC :only copied %d bytes",delta);
    *off += delta;
   
    return delta;
}
static ssize_t mywrite (struct file *filep, const char __user *userbuff, size_t ln, loff_t *off){
    int not_copied,delta,to_copy = (ln + *off ) < sizeof(text) ? ln : (sizeof(text) - *off);
    printk(KERN_INFO "PROC: write called. . . .\n");
    if(*off >= sizeof(text))
            return 0;
    
    not_copied = copy_from_user( &text[*off] , userbuff , to_copy);
    delta = to_copy - not_copied;
    if(not_copied)
            printk(KERN_WARNING "PROC : only copied %d bytes",delta);
    *off += delta;
    return delta;
}
static int open(struct inode* inode , struct file* fileop){
  printk(KERN_INFO "PROC : Opened file \n");
  return 0;
}
static int close(struct inode* inode , struct file* fileop){
  printk(KERN_ERR "PROC : Closed file \n");
  return 0;
}
static struct  proc_ops fops={
    .proc_open = open,
    .proc_release = close,
    .proc_read = myread,
    .proc_write = mywrite

};
static int __init myinit(void)
{
   //major = register_chrdev(0,"char device",&fops);
   if(major < 0){
    printk( "PROC :Cdev:error registering device. . . .\n");
    return major;
   }
   printk("Cdev :entering kernal major number = %d. . . . .\n",major);
   proc_create("demo_proc_file2",0600,NULL,&fops);
   return 0;
}

static void __exit myexit(void)
{
    //unregister_chrdev(major,"Cdev...\n");
    remove_proc_entry("demo_proc_file2",NULL);
    printk("Cdev:exiting  .. . . . ..\n");
}
module_init(myinit);
module_exit(myexit);

MODULE_AUTHOR("adin");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Char device module");