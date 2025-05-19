#include<linux/module.h>
#include<linux/init.h>
#include<linux/hrtimer.h>
#include<linux/jiffies.h>


static struct hrtimer tTimer;
u64 start_t;
static enum hrtimer_restart my_timer_handler(struct hrtimer *timer)
{
    u64 now_t = jiffies;
    printk(KERN_INFO "Start time - now time = %u \n",jiffies_to_msecs(now_t - start_t));

    return HRTIMER_NOmakekRESTART;
}
static int __init myinit(void)
{
   printk(KERN_INFO "Hello:entering kernal. . . . .\n");
   hrtimer_init(&tTimer,CLOCK_MONOTONIC , HRTIMER_MODE_REL);
   tTimer.function = &my_timer_handler;
   start_t = jiffies ;
   hrtimer_start(&tTimer , ms_to_ktime(100) , HRTIMER_MODE_REL);
   return 0;
}

static void __exit myexit(void)
{
    hrtimer_cancel(&tTimer);
    printk(KERN_INFO "Hello:exiting  .. . . . ..\n");
}


module_init(myinit);
module_exit(myexit);

MODULE_AUTHOR("adin");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("hr timer module");