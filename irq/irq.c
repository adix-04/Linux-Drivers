#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/interrupt.h>

/*statuc method for tasklet creation idk about the parmeter may its new*/
static void tasklet_fun(struct tasklet_struct* t);   
/*declaration step for a tasklet*/
DECLARE_TASKLET(tasklet,tasklet_fun);
/**
 *@brief tasklet definitiion
 */
static void tasklet_fun(struct tasklet_struct* t){
    printk(KERN_DEBUG "IRQ : Executing tasklet function|data ");
}

static int major;
static char text [65]; 

static ssize_t myread (struct file *filep, char __user *userbuff, size_t ln, loff_t *off){
    int not_copied,delta,to_copy = (ln + *off ) < sizeof(text) ? ln : (sizeof(text) - *off);
    printk(KERN_INFO "Cdevops2:read called. . . .\n");
    if(*off >= sizeof(text))
            return 0;
    
    not_copied = copy_to_user(userbuff , &text[*off] , to_copy);
    delta = to_copy - not_copied;
    if(not_copied)
            printk(KERN_WARNING "Cdevops2 :only copied %d bytes",delta);
    *off += delta;
   
    return delta;
}
static ssize_t mywrite (struct file *filep, const char __user *userbuff, size_t ln, loff_t *off){
    int not_copied,delta,to_copy = (ln + *off ) < sizeof(text) ? ln : (sizeof(text) - *off);
    printk(KERN_INFO "Cdevops2:write called. . . .\n");
    if(*off >= sizeof(text))
            return 0;
    
    not_copied = copy_from_user( &text[*off] , userbuff , to_copy);
    delta = to_copy - not_copied;
    if(not_copied)
            printk(KERN_WARNING "Cdevops2 :only copied %d bytes",delta);
    *off += delta;
    return delta;
}
static struct  file_operations fops={
    .read = myread,
    .write = mywrite

};
/**
 * @brief this is the interupt handler function with return type irqreturn_t  this represents the top half of ISR the bottom can be implemented with workqueue or taslet or ..
 * @param ,irq number , device id
 */
static irqreturn_t irq_handler(int irq ,void* dev_id){
    printk(KERN_INFO "IRQ : interrupt occured .. . \n");
    /*we schedule the task inside the first half*/
    tasklet_schedule(&tasklet);
    return IRQ_HANDLED ;
}
static int __init myinit(void)
{
   major = register_chrdev(0,"char device",&fops);
   if(major < 0){
    printk("Cdev:error registering device. . . .\n");
    return major;
   }
   printk("IRQ :entering kernal major number = %d. . . . .\n",major);
   

   /**
    * @brief intrrupt registering method
    * @param irq number , handler function , flags , name for interrupt , device id for sharing interupt handler
    */
   if(request_irq(10,irq_handler,IRQF_SHARED,"irq",(void*)(irq_handler)))
   {
    printk(KERN_ALERT "IRQ :  intrrupt failed to init \n");
   }

   return 0;
}

static void __exit myexit(void)
{
    unregister_chrdev(major,"Cdev...\n");
    printk("IRQ :exiting  .. . . . ..\n");
    /**
     * @brief interrupt removing function
     * @param interrupt number , device id
     */
    free_irq(10 , (void*)(irq_handler));
}
module_init(myinit);
module_exit(myexit);

MODULE_AUTHOR("adin");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("IRQ device module");