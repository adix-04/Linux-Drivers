#include<linux/init.h>
#include<linux/fs.h>
#include<linux/module.h>
#include<linux/kthread.h>
#include<linux/delay.h>
#include<linux/sched.h>
#include<linux/string.h>
#include<linux/ioctl.h>
/*#define RD_DATA __IOR("magic number","command number","argument type")*/
#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)

static int  major;
char buff[65];
int32_t data = 0;

/**
 * @brief write function to write to the kernal buffer
 * @param fileops,userbuff(must be const only reading),size or length , offset variable
 */
static ssize_t mywrite (struct file *fileop, const char __user *userbuf, size_t l, loff_t * off){
    printk(KERN_INFO "write called\n");
    /*calculating the length of how much bytes needed to be written*/
    int to_copy = (l + *off) < (sizeof(buff)) ? l : (sizeof(buff) - * off);
    if(*off >= sizeof(buff)){
      return 0;
    }
 /**
  * @brief function to write to the kernal buffer
  * @param kernal_buffer,userbuffer (where the data is) , length 
  */
   int result = copy_from_user(&buff[*off],userbuf,to_copy);
  /*calculating how much we copied */
  int delta = to_copy - result ;
  /*updating the offset data*/
  *off += delta;
  return delta;
}
/**
 * @brief Read function to read from  the kernal buffer
 * @param fileops,userbuff(no const we are writing to it),size or length , offset variable
 */
static ssize_t myRead(struct file *fileop,  char __user *userbuf, size_t l, loff_t * off){
    printk(KERN_INFO "read called\n");
    int result , to_copy = (l + *off) < (sizeof(buff)) ? l : (sizeof(buff) - *off);
    if(*off >= sizeof(buff)){
      printk(KERN_INFO "done or nothing to  read\n");
      return 0;
    }
    int rs = copy_to_user(userbuf,&buff[*off],to_copy);
    int len = to_copy - rs ;
    *off +=len ;
    return len;
}
/**
 * @brief file open function
 */
static int myOpen (struct inode *filenode, struct file *fileop){
    printk(KERN_INFO "IOCTL :file  open called \n");
    return 0;
}
/**
 * @brief file close function
 */
static int myClose (struct inode *filenode, struct file *fileop){
    printk(KERN_ALERT "IOCTL :File closed \n");
    return 0;
}

static long int ioctl (struct file *fileop, unsigned cmd, unsigned long arg)
{
  switch(cmd){
    case WR_VALUE:
          printk(KERN_INFO "IOCTL : ioctl write \n");
          if(copy_from_user(&data , (int32_t*)arg , sizeof(data))){
            printk(KERN_ERR "IOCTL :Data Write : ERR \n");
          } 
          printk(KERN_DEBUG "IOCTL :The value is :%d \n",data);
          break;
    case RD_VALUE:
          printk(KERN_INFO "IOCTL :ioctl read \n");
         if(copy_to_user((int32_t*)arg , &data , sizeof(data))){
          printk(KERN_ERR "IOCTL :Data Read : ERR \n");
         } 
         printk(KERN_DEBUG "IOCTL :The value is : %d \n",data) ;
         break; 
    default:
         printk(KERN_INFO "IOCTL : default\n ");
         break; 
  }
  return 0 ;
}

/*file operations for the char device, it is needed beacuse linux treats evrything as a file*/
static struct file_operations fops ={
  .owner = THIS_MODULE,
  .read = myRead,
  .write = mywrite,
  .open = myOpen ,
  .release = myClose,
  .unlocked_ioctl = ioctl

};
/**
 * @brief init function is called when the kernal module is loaded
 */
static int __init myInit(void){
    printk(KERN_INFO "IOCTL : Thread init kernal module\n");
    major = register_chrdev(0,"chardevice",&fops);
    if(major < 0){
     printk(KERN_ERR " IOCTL :error registering device\n");
     return major;
    } 
    printk(KERN_INFO "IOCTL : registering device with major num %d \n",major);
    return 0 ;
}
/**
 * @brief exit function is called when the module is exited
 */
static void __exit myExit(void){
  printk(KERN_INFO "Exit kernal module\n");
  unregister_chrdev(major,"chardevice");
}

module_init(myInit);
module_exit(myExit);

MODULE_AUTHOR("ADIN");
MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION("Threads in linux kernal module"); 


