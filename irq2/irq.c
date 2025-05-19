/**
 * The reason for this code is simple to show the use of interupt and tasklet and how the bottom half and the top half of the ISR works ..
 * This code involves the manual activaton if tasklest other than the automatic
 */

#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/interrupt.h>
#include<linux/slab.h>
/*method for tasklet function  about the parmeter may be its new*/
static void tasklet_fun(unsigned long);   
/*declaration step for a tasklet*/
struct tasklet_struct* tasklet = NULL;
/**
 *@brief tasklet definitiion
 *@param unsigned long arguments
 */
static void tasklet_fun(unsigned long arg){
    printk(KERN_DEBUG "IRQ : Executing tasklet function|data = %ld \n",arg);
}

static int major;
static char text [65]; 

/**
 * @brief this is the interupt handler function with return type irqreturn_t  this represents the top half of ISR the bottom can be implemented with workqueue or taslet or ..
 * @param ,irq number , device id
 */
static irqreturn_t irq_handler(int irq ,void* dev_id){
    printk(KERN_INFO "IRQ : interrupt occured .. . \n");
    tasklet_schedule(tasklet);
    return IRQ_HANDLED ;
}
static int __init myinit(void)
{
   printk(KERN_INFO "IRQ : init module");
   /*since we are doing taslet dynamically we need to allocate memeory just like malloc we use kmalloc -> slab.h*/
   tasklet = kmalloc(sizeof(struct tasklet_struct),GFP_KERNEL);
   if(tasklet == NULL){
    printk(KERN_ERR "IRQ :Error allocating memmory\n");
   } else 
    printk(KERN_INFO "IRQ :TASKLET OK");
   /*intialize the tasklet function*/ 
   tasklet_init(tasklet,tasklet_fun,0);
   /**
    * @brief intrrupt registering method
    * @param irq number , handler function , flags , name for interrupt , device id for sharing interupt handler
    */
   if(request_irq(10,irq_handler,IRQF_SHARED,"irq",(void*)(irq_handler)))
     printk(KERN_ALERT "IRQ :  intrrupt failed to init \n") ;


   return 0;
}

static void __exit myexit(void)
{
    printk("IRQ :exiting  .. . . . ..\n");
    /**
     * @brief interrupt removing function
     * @param interrupt number , device id
     */
    tasklet_kill(tasklet);
    if(tasklet != NULL)
        kfree(tasklet);
    free_irq(10 , (void*)(irq_handler));
}
module_init(myinit);
module_exit(myexit);

MODULE_AUTHOR("adin");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("IRQ device module");