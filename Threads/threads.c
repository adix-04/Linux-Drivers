#include<linux/init.h>
#include<linux/fs.h>
#include<linux/module.h>
#include<linux/kthread.h>
#include<linux/delay.h>
#include<linux/sched.h>

static int  major;
static struct task_struct *thread1 ;
static struct task_struct *thread2 ;
static int t1, t2 ;



/*file operations for the char device, it is needed beacuse linux treats evrything as a file*/
static struct file_operations fops ={
    .owner = THIS_MODULE
};
/**
 * @brief thread function to be executed
 * @param thread_nr pointer to the thread number
 */
int threadfunction(void* thread_nr)
{
 unsigned int i = 0;
 int t_nr = *(int*)thread_nr;
 while(!kthread_should_stop()){
    printk(KERN_INFO "Thread %d is executed and counter value : %d",t_nr,i++);
    msleep(t_nr * 10000);
 }
 printk(KERN_INFO "Thread %d finished execution with counter :%d",t_nr,i);  
 return 0;        
}
/**
 * @brief init function is called when the kernal module is loaded
 */
static int __init myInit(void){
  printk(KERN_INFO "Thread init kernal module\n");
  /*thread creation and execution*/
  thread1 = kthread_create(threadfunction , &t1 , "thread 1");
  if(thread1 != NULL){
    /*start the thread if the ceation was suucess full*/
    wake_up_process(thread1);
    printk(KERN_INFO " Thread 1 is created");
  } else {
    printk(KERN_ERR "Error creating thread");
  }
  /*thread creation and running in single command*/
  thread2 = kthread_run(threadfunction,&t2,"thread 2");
  if(thread2 != NULL){
    printk(KERN_INFO "Thread 2 is created");
  } else {
    kthread_stop(thread1);
    printk(KERN_ERR "Error creating thread");
  }


  major = register_chrdev(0,"chardevice",&fops);
  if(major < 0){
    printk(KERN_ERR " error registering device\n");
    return major;
  } 
  printk(KERN_INFO "registering device with major num %d \n",major);
  return 0 ;
}
/**
 * @brief exit function is called when the module is exited
 */
static void __exit myExit(void){
 printk(KERN_INFO "Exit kernal module\n");
 kthread_stop(thread1);
 kthread_stop(thread2);
}

module_init(myInit);
module_exit(myExit);

MODULE_AUTHOR("ADIN");
MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION("Threads in linux kernal module"); 


