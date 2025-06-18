/**
 * Linux USB driver 
 * https://elixir.bootlin.com/linux/v6.15.2/source   => linux kernal
 */
#include<linux/init.h>
#include<linux/module.h>
#include<linux/usb.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/slab.h>
#include<linux/mutex.h>
/*define vendor id and product id (find it by lsusb -v) this way make it more easy*/
#define USB_VENDOR 0x14cd
#define USB_PRODUCT 0x1212
#define USB_MINOR 192


static struct usb_driver usb_dev_driver;
/**
 * @brief this structure is used store all data about our usb driver , like thi program consist of manystructures and many ops
 * so to keep track of everything we use this structure as a brain. for example we need the  bulk_in_size or bulk_in_buffer to 
 * know how much we want to read and write , we can get that from probe function and store 
 * it inside this struture , later we can access that values from this structure pointer
 * TL:DR this is only my understanding
 */
struct usb_skel{
    struct usb_device *usbdev;
    struct usb_class_driver *usbclassdrv;
    struct usb_interface* interface;
    struct kref kref;
    size_t bulk_in_size;
    unsigned char *bulk_in_buffer;
    struct urv *bulk_in_urb;
    __u8 bulk_in_endpointAddr;
    __u8 bulk_out_endpointAddr;
    int errors;
    struct mutex io_mutex;
};

/*this is the usb class driver struct there are things we need to add inside this structure used inside registering device*/


static int usb_open (struct inode *inode, struct file *file)
{
    printk(KERN_INFO "USB : open fops called \n");
    struct usb_skel *udev;
    struct usb_interface* iface;
    int subminor;
    subminor = iminor(inode);
    iface = usb_find_interface(&usb_dev_driver,subminor);

    if(!iface){
        return -ENODEV ;
    }

    udev = usb_get_intfdata(iface);
    file->private_data = udev;
    return 0 ;
}
static int usb_close (struct inode *inode, struct file *file){
    printk(KERN_ALERT "USB  : File close ops called \n");
    return 0;
}
static ssize_t usb_read (struct file *file, char *buffer, size_t count,loff_t *ppos){
    printk(KERN_INFO "USB : Read called \n");
    struct usb_skel* dev = file->private_data;
    int retval ;
    int read_cnt;
    if(!dev)
        return -ENODEV;
    
    if(count > dev->bulk_in_size){
        printk("USB : %ld is the value of count \n",count);
        count = dev->bulk_in_size;
        printk("USB : %ld is the value of count afterwards \n",count);
        
    }

    retval = usb_bulk_msg(dev->usbdev,
                        usb_rcvbulkpipe(dev->usbdev,dev->bulk_in_endpointAddr),
                        dev->bulk_in_buffer,
                        min(dev->bulk_in_size,count),
                        &read_cnt,5000);

    if(retval){
        pr_info("USB : bulk msg ERR   \n");
        return retval; 
        } 
    else {
        pr_err("USB : bulk_msgs retured OK code . . \n");
    }
    if(copy_to_user(buffer,dev->bulk_in_buffer,read_cnt))
        {
            pr_err("USB :  Error in copying from user. . \n");
            return -EFAULT;
        } 
    else {
        pr_info("USB : copied %d bytes . . .\n",read_cnt);
    }

    return read_cnt;
}
static ssize_t usb_write(struct file *file, const char __user *userbuf, size_t len, loff_t *off){

    printk(KERN_INFO " USB : write called\n");
    return 0;
}
static struct file_operations fops={
    .owner      = THIS_MODULE,
    .open       = usb_open,
    .read       = usb_read,
    .write      = usb_write,
    .release    = usb_close   
};
struct usb_class_driver usb_driver={
    .name           = "adix-usbDriver%d",
    .fops           = &fops,
    .minor_base     = USB_MINOR
};
/**
 * @brief this function called as the probe function when we load our usb,inside we need to add the interface
 * and register the usb device , careful about the print statements kinda tricky
 * issue 1 : not registering the usb and getting minor number probably due to other drivers . working on it 
 * @param usb_interface structure , device id structure
 */
/**
 * im kinda modifying the probe function beacuse we need to add some more things there like the struture which holds all usb related datas 
 * need to collect and store . so that we can do it from the probe functon we will get as much as things we need from here
 * so first we create all the structure pointers and proceed
 */
static int usb_dev_probe (struct usb_interface *intf,const struct usb_device_id *id)
{
    /*structure for our usb_host_interface beacuse usb may jhave multiple endpoints and descriptors this structure represents all of them*/
    struct usb_host_interface* usb_interface_desc ;
    /*this structure defines one endpoint not all ot them so we can find the right one for read and write*/
    struct usb_endpoint_descriptor *endpoint_in,*endpoint_out;
    /*our brain structure*/
    struct usb_skel *dev;
    dev = kzalloc(sizeof(*dev) , GFP_KERNEL);
    if(!dev)
        return -ENOMEM;
    
    int i, ret ;
    /**
     * init for the kref and mutex
     */
    kref_init(&dev->kref);
    mutex_init(&dev->io_mutex);
    /**
     * interface_to_usbdev(intf) gets you the struct usb_device * from the usb_interface 
     *usb_get_dev() increments the reference count of that USB device, so it doesn’t get deallocated while your driver is using it.
    */
    dev->usbdev = usb_get_dev(interface_to_usbdev(intf));
    dev->interface = usb_get_intf(intf);
    usb_interface_desc = intf->cur_altsetting ;

    printk(KERN_INFO "USB : usb probed %d :(%04X:%04X)\n" , usb_interface_desc->desc.bInterfaceNumber , id->idVendor,id->idProduct);
    printk(KERN_INFO "USB : usb end points : %02x\n",usb_interface_desc->desc.bNumEndpoints);
    printk(KERN_INFO "USB : usb interface class : %02x\n",usb_interface_desc->desc.bInterfaceClass);
    /**
     * @brief there may ne multiple endpoints available each with IN or OUT ops 
     * now we are checking for the IN that is why  we loop through all endpoints till we find the bulk_in one see the loop below
     */
    
 
/**
* inside the usb_skelton file they did something like this i dont know why they did that in the first place
* for now i am sticking with it .
* finding the common endpoits 
*/
/**
 * usb_find_common_endpoints() -- look up common endpoint descriptors
 * @alt:	alternate setting to search
 * @bulk_in:	pointer to descriptor pointer, or NULL
 * @bulk_out:	pointer to descriptor pointer, or NULL
 * @int_in:	pointer to descriptor pointer, or NULL
 * @int_out:	pointer to descriptor pointer, or NULL
 *
 * Search the alternate setting's endpoint descriptors for the first bulk-in,
 * bulk-out, interrupt-in and interrupt-out endpoints and return them in the
 * provided pointers (unless they are NULL).
 *
 * If a requested endpoint is not found, the corresponding pointer is set to
 * NULL.
 *
 * Return: Zero if all requested descriptors were found, or -ENXIO otherwise.
 */
    int retval = usb_find_common_endpoints(intf->cur_altsetting,&endpoint_in,&endpoint_out,NULL,NULL);
    if(retval){
        dev_err(&intf->dev,"USB : could not find endpoints IN and OUT");
        usb_set_intfdata(intf,NULL);
        printk(KERN_ERR "USB : err in finding common endpoits . . .\n");
        return retval;
    }
    /**
     * @brief hopefully if we got something from the endpoints we can find the bulk_size and buffer endpoint address
     */
    dev->bulk_in_size = usb_endpoint_maxp(endpoint_in);
    dev->bulk_in_endpointAddr = endpoint_in->bEndpointAddress;
    dev->bulk_in_buffer = kmalloc(dev->bulk_in_size,GFP_KERNEL);
    printk("USB : flag");

    if(!dev->bulk_in_buffer){
       printk(KERN_ERR "USB : ERROR Finding THE bulk in buffer area. . .\n");
       return -ENOMEM;
    }
    if(!dev->bulk_in_buffer){
        usb_put_dev(dev->usbdev);
        kfree(dev);
        return -ENOMEM;
    }
    dev->bulk_out_endpointAddr = endpoint_out->bEndpointAddress;

    usb_set_intfdata(intf,dev);
    printk("USB : flag");
    ret = usb_register_dev(intf,&usb_driver);
    if(ret){
       printk(KERN_ERR "USB : Error with registering and getting minor number \n");
    } else {
        printk(KERN_DEBUG "USB : Registered and got minor number : %d \n",intf->minor);
    }
    return 0;
}
/**
 * @brief function called when the usb device is disconnected we just deregister here
 * @param usb_interface structure
 */
static void usb_dev_disconnect (struct usb_interface *intf){
    usb_deregister_dev(intf,&usb_driver);
    printk(KERN_INFO "USB : disconnected usb device \n");
}

static int usb_dev_resume (struct usb_interface *intf){
   return 0;
}

/**
 * @brief usb device id structure holds the id table like vendor is and product id of our usb device
 * so our kernal or code knows which device should use this driver
 */
static struct usb_device_id usb_dev_idtable[] ={
    {
    USB_DEVICE(USB_VENDOR,USB_PRODUCT),
    },
    {}
};

/**
 * @brief this structure holds usb_driver functions we need to add all the driver operations over here
 * this will be used in usb_register operation not usb_register_dev op
 */
static struct usb_driver usb_dev_driver ={
    .name = "adix usb driver",
    .probe = usb_dev_probe,
    .disconnect = usb_dev_disconnect,
    .id_table = usb_dev_idtable,
    .supports_autosuspend  = 1,
    .resume = usb_dev_resume
}; 

static int __init usb_init(void){
    printk(KERN_INFO "USB : initializing driver \n");
    int result = usb_register(&usb_dev_driver);
    if(result < 0){
        pr_err("USB : register failed with ER :no %d .. \n",result);
        return -1;
    }
    return 0;
}
static void __exit usb_exit(void){
    printk(KERN_INFO "USB : module is exiting\n");
    usb_deregister(&usb_dev_driver);
}


module_init(usb_init);
module_exit(usb_exit);
MODULE_DEVICE_TABLE(usb,usb_dev_idtable);
MODULE_AUTHOR("Adin N S <adinnavakumar22@gmail.com>");
MODULE_DESCRIPTION("Simple linux driver module");
MODULE_LICENSE("GPL");


//    for(i = 0 ;i < usb_interface_desc->desc.bNumEndpoints ; i++){
//         endpoint_in = &usb_interface_desc->endpoint[i].desc;
        
//         if(usb_endpoint_is_bulk_in(endpoint_in)){
//             dev->bulk_in_size = usb_endpoint_maxp(endpoint_in);
//             dev->bulk_in_endpointAddr = endpoint_in->bEndpointAddress;
//             dev->bulk_in_buffer = kmalloc(dev->bulk_in_size,GFP_KERNEL);
//             if(!dev->bulk_in_buffer)
//                 return -ENOMEM;
//             break;
//         }
        
//     } 