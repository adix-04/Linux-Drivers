/**
 * The goal of this program is to show the usage of spinlock in a multithreaded code
 * the shared variable accessed through spinlock,unlock mechanism by threads and top half of irq and bottom half
 * from the logs i can see irq is registered by running cat /proc/interupts but i dont know if a interupt has been invoking
 */
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/module.h>
#include<linux/kthread.h>
#include<linux/delay.h>
#include<linux/sched.h>
#include<linux/interrupt.h>
#include<linux/slab.h>
int major;
unsigned long globalVar = 0;
static struct task_struct *thread2 ;
static struct task_struct *thread1 ;
struct tasklet_struct* tasklet = NULL;
/*defining spinlock */
DEFINE_SPINLOCK(spinlock);
/*tasklet function for bottom half*/
static void tasklet_fun(unsigned long var);
/*file operations for the char device, it is needed beacuse linux treats evrything as a file*/
static struct file_operations fops ={
    .owner = THIS_MODULE
};
/**
 * @brief tasklet function to be executed
 * @param unsigned long  variable 
 */
static void tasklet_fun(unsigned long var)
{
  spin_lock(&spinlock);
  printk(KERN_INFO "Thread : spinlocked and the value in tasklet is : %ld \n",globalVar);
  globalVar ++ ;
  spin_unlock(&spinlock);
}
/**
 * @brief thread function to be executed by thread
 * @param void pointer 
 */
int threadfunction(void* thread_nr)
{
  /*execute till the thread stops*/
 while(!kthread_should_stop()){
  if(!spin_is_locked(&spinlock)){
    printk(KERN_INFO "Thread : spin lock is not locked inside function 1\n");
  } 
  spin_lock(&spinlock);
  if(spin_is_locked(&spinlock)){
    printk(KERN_INFO "Thread : spin lock is locked inside function 1\n"); 
  }
  major ++ ;
  printk(KERN_DEBUG "Thread : Value in function 1 : %d \n",major);
  spin_unlock(&spinlock);
  msleep(3000);
 }
 printk(KERN_INFO "Thread 1 finished execution with :%d\n",major);  
 return 0;        
}
int threadfunction2(void* thread_nr)
{
 
 while(!kthread_should_stop()){
  if(!spin_is_locked(&spinlock)){
    printk(KERN_INFO "Thread : spin lock is not locked inside function 2\n");
  } 
  spin_lock(&spinlock);
  if(spin_is_locked(&spinlock)){
    printk(KERN_INFO "Thread : spin lock is locked inside function 2\n"); 
  }
  major ++ ;
  printk(KERN_DEBUG "Thread : Value in function 2 : %d \n",major);
  spin_unlock(&spinlock);
  msleep(3000);
 }
 printk(KERN_INFO "Thread 2 finished execution with  :%d\n",major);  
 return 0;        
}
/**
 * @brief this is how the irq handler function should be with a specific return type and arguments, i am also using spinlocks to lock the shared resource
 */
static irqreturn_t irqhandler(int irq , void* dev_id){
  spin_lock_irq(&spinlock);
  globalVar ++;
  printk(KERN_DEBUG "Thread : value in irqhandler : %ld \n",globalVar);
  spin_unlock_irq(&spinlock);
  tasklet_schedule(tasklet);
  return IRQ_HANDLED;
}
/**
 * @brief init function is called when the kernal module is loaded
 */
static int __init myInit(void){
  printk(KERN_INFO "Thread init kernal module\n");
  /*initialze some memory space for the tasklet then initialize it through dynamic method*/
  tasklet = kmalloc(sizeof(struct tasklet_struct),GFP_KERNEL);
  tasklet_init(tasklet,tasklet_fun,0);
  /**
   * thread creation and execution
   * i am just gonna stick to this part not kthread_run beacuse i love my computer
  */
  thread1 = kthread_create(threadfunction , NULL , "thread 1");
  if(thread1 != NULL){
    /*start the thread if the ceation was suucess full*/
    wake_up_process(thread1);
    printk(KERN_INFO " Thread 1 is created");
  } else {
    printk(KERN_ERR "Error creating thread");
  }
  thread2 = kthread_run(threadfunction2, NULL , "thread 2");
  
  // thread2 = kthread_create(threadfunction2 , NULL , "thread 2");
  if(thread2 != NULL){
    /*start the thread if the ceation was suucess full*/
    printk(KERN_INFO " Thread 2 is created");
  } else {
    printk(KERN_ERR "Error creating thread");
  }
  major = register_chrdev(0,"Thread",&fops);
  if(major < 0){
    printk(KERN_ERR "error registering device\n");
    return major;
  } 
  /*irq number , handler function , flags, name of irq , void pointer something*/
  if(request_irq(10,irqhandler,IRQF_SHARED,"spin lock irq",(void*)(irqhandler))){
    printk(KERN_INFO"Thread : Irq registred \n");
  }
  printk(KERN_INFO "registering device with major num %d \n",major);
  return 0 ;
}
/**
 * @brief exit function is called when the module is exited
 */
static void __exit myExit(void){
 printk(KERN_INFO "Thread : Exit kernal module\n");
 /*stopping the threads*/
 kthread_stop(thread1);
 kthread_stop(thread2);
 /*killing the tasklet*/
 tasklet_kill(tasklet);
 /*freeing the irq*/
 free_irq(10,(void*)(irqhandler));
}

module_init(myInit);
module_exit(myExit);

MODULE_AUTHOR("ADIN");
MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION("Threads and spinlock in linux kernal module"); 


