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
    size_t bulk_out_size;
    unsigned char *bulk_in_buffer;
    unsigned char *bulk_out_buffer;
    struct urv *bulk_in_urb;
    __u8 bulk_in_endpointAddr;
    __u8 bulk_out_endpointAddr;
    int errors;
    struct mutex io_mutex;
};

/**
 * @brief calls when the usb device is opened for read or write ops
 * subminor and interface and device structure should be verified to ok state
 */
static int usb_open (struct inode *inode, struct file *file)
{
    printk(KERN_INFO "USB : open fops called \n");
    struct usb_skel *udev;
    struct usb_interface* iface;
    int subminor;
    subminor = iminor(inode);
    pr_debug("USB : Subminor number %d \n",subminor);
    iface = usb_find_interface(&usb_dev_driver,subminor);
    if(!iface){
        pr_info("USB : ERROR with interface here");
        return -ENODEV ;
    }

    udev = usb_get_intfdata(iface);
    file->private_data = udev;
    return 0 ;
}
/**
 * @brief close fun calls when the ops is completed read/write
 * everything should be cleared at this stage
 */
static int usb_close (struct inode *inode, struct file *file){
    printk(KERN_ALERT "USB  : File close ops called \n");
    return 0;
}
/**
 * @brief read fuction to read from the usb device
 */
static ssize_t usb_read (struct file *file,char *buffer, size_t count,loff_t *ppos){
    printk(KERN_INFO "USB : Read called \n");
    struct usb_skel* dev = file->private_data;
    int retval ;
    int read_cnt;
    if(!dev)
        return -ENODEV;
    /*adjusting the value of count to match our bulk size*/
    if(count > dev->bulk_in_size){
        pr_info("USB : %ld is the value of count \n",count);
        count = dev->bulk_in_size;
        pr_info("USB : %ld is the value of count afterwards \n",count);
    }
    pr_info("USB : going to use usb_bulk_msg from  0x%02x endpoint with size of %ld \n",dev->bulk_in_endpointAddr,count);
    /**
     * Builds a bulk USB request block, sends it off and waits for completion 
     * @return should be 0 otherwise failed
     */
    retval = usb_bulk_msg(dev->usbdev,
                        usb_rcvbulkpipe(dev->usbdev,dev->bulk_in_endpointAddr),
                        dev->bulk_in_buffer,
                        count,
                        &read_cnt,5000);

    if(retval){
        pr_err("USB : bulk msg ERR   \n");
        return retval; 
        } 
    else {
        pr_debug("USB : bulk_msgs retured OK code . . \n");
    }
    /*then we copy the bulk buffer to userspace*/
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
/**
 * @brief write function to write data off to the usb device
 * 
 */
static ssize_t usb_write(struct file *file, const char *buffer, size_t count,loff_t *ppos){
    struct usb_skel *dev = file->private_data;
    int retval,write_cnt;
    if (count > dev->bulk_out_size)
        count = dev->bulk_out_size;
    char *kbuf;
    if(!dev){
        dev_err(&dev->interface->dev,"USB : error in usb device \n");
        return -ENODEV;
    } 
    pr_info("USB : Writing %zu bytes to endpoint 0x%02x\n", count, dev->bulk_out_endpointAddr);
    /*storing the userbuffer based on our size to the buffer alllocated*/
    kbuf = memdup_user(buffer , count);
    if(IS_ERR(kbuf)){
        dev_err(&dev->interface->dev,"USB : error in storing kernel buffer \n");
        return PTR_ERR(kbuf);
    }
     /**
      * Builds a bulk USB request block, sends it off and waits for completion 
      * @return should be 0 otherwise failed
     */
    retval = usb_bulk_msg(dev->usbdev,
                          usb_sndbulkpipe(dev->usbdev,dev->bulk_out_endpointAddr),
                          kbuf,
                          count,
                          &write_cnt,
                          5000);
    pr_debug("USB : retval is %d \n",retval);
    if(retval){
         pr_err("USB : bulk msg ERR   \n");
         return retval; 
        } 
    else {
        pr_debug("USB : bulk_msgs retured OK code . . \n");
    }
    printk(KERN_INFO "USB : write called\n");
    /*freeing mem afterwards*/
    kfree(kbuf);
    return write_cnt;
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
static int usb_dev_probe (struct usb_interface *intf,const struct usb_device_id *id)
{
    /*structure for our usb_host_interface beacuse usb may have multiple endpoints and descriptors this structure represents all of them*/
    struct usb_host_interface* usb_interface_desc ;
    /*this structure defines one endpoint not all ot them so we can find the right one for read and write*/
    struct usb_endpoint_descriptor *endpoint_in,*endpoint_out;
    struct usb_skel *dev;
    /*this allocats the buffer for our device structure beacause this function goes first upon plugin*/
    dev = kzalloc(sizeof(*dev) , GFP_KERNEL);
    if(!dev)
        return -ENOMEM;
    
    int i, ret ;
    kref_init(&dev->kref);
    mutex_init(&dev->io_mutex);
    /*increments the reference count of the usb device structure*/
    dev->usbdev = usb_get_dev(interface_to_usbdev(intf));
    /*increments the reference count of the usb interface structure*/
    dev->interface = usb_get_intf(intf);
    /*getting the default interface setting*/
    usb_interface_desc = intf->cur_altsetting ;
    printk(KERN_INFO "USB : usb probed %d :(%04X:%04X)\n" , usb_interface_desc->desc.bInterfaceNumber , id->idVendor,id->idProduct);
    printk(KERN_INFO "USB : usb end points : %02x\n",usb_interface_desc->desc.bNumEndpoints);
    printk(KERN_INFO "USB : usb interface class : %02x\n",usb_interface_desc->desc.bInterfaceClass);
    /**
     * @brief we are getting the common endpoints here like endpoint_in and endpoint_out in this case
     * @return should be zero no other numbers
     */
    int retval = usb_find_common_endpoints(intf->cur_altsetting,&endpoint_in,&endpoint_out,NULL,NULL);
    if(retval){
        dev_err(&intf->dev,"USB : could not find endpoints IN and OUT");
        usb_set_intfdata(intf,NULL);
        printk(KERN_ERR "USB : err in finding common endpoits . . .\n");
        return retval;
    }else {
        pr_info("USB : common endpoints found \n");
    }
    /*afte we can get all the size buffer and endpoint address*/
    dev->bulk_in_size = usb_endpoint_maxp(endpoint_in);
    dev->bulk_in_endpointAddr = endpoint_in->bEndpointAddress;
    dev->bulk_in_buffer = kmalloc(dev->bulk_in_size,GFP_KERNEL);
    dev->bulk_out_size = usb_endpoint_maxp(endpoint_out);
    dev->bulk_out_endpointAddr = endpoint_out->bEndpointAddress;
    dev->bulk_out_buffer = kmalloc(dev->bulk_out_size , GFP_KERNEL);
    pr_info("USB : bulk in size : %ld , bulk_in_endpoint addr : 0x%02x \n",dev->bulk_in_size,dev->bulk_in_endpointAddr);
    pr_info("USB : bulk out size :%ld ,bulk_out_endpoint_address : 0x%02x \n" ,dev->bulk_out_size,dev->bulk_out_endpointAddr);
    printk("USB : flag 1");
    if(!dev->bulk_in_buffer){
       dev_err(&intf->dev, "Failed to allocate bulk_in or bulk_in_buffer\n");
       usb_put_dev(dev->usbdev);
       kfree(dev);
       return -ENOMEM;
    }
    if(!dev->bulk_out_buffer){
       dev_err(&intf->dev, "Failed to allocate bulk_out or bulk_out buffer\n");
       usb_put_dev(dev->usbdev);
       kfree(dev);
       return -ENOMEM;
    }
    /*we store the interface details so we can use it later*/
    usb_set_intfdata(intf,dev);
    printk("USB : flag 2");
    /*then we register our usb device */
    ret = usb_register_dev(intf,&usb_driver);
    if(ret){
       printk(KERN_ERR "USB : Error with registering and getting minor number \n");
    } else {
        printk(KERN_DEBUG "USB : Registered and got minor number : %d \n",intf->minor);
    }
    return 0;
}
/**
 * @brief disconnected calls when we remove our usb device
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

