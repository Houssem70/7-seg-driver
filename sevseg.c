// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/cdev.h>

dev_t dev_number;
struct cdev chr_dev;
struct class * sevseg_class;
 struct device * sevseg_device;

static int sevseg_open (struct inode *device_file, struct file *instance)
{
    printk("%s function was called\n", __FUNCTION__);
    return 0;
}

static int sevseg_release (struct inode *device_file, struct file *instance)
{
    printk("%s function was called\n", __FUNCTION__);
    return 0;
}

const struct file_operations fops={
    .owner = THIS_MODULE,
    .open = sevseg_open,
    .release = sevseg_release
};

static int __init sevseg_init(void)
{
    int return_val;
    
    return_val = alloc_chrdev_region(&dev_number, 0, 1, "7-segment");

    if(return_val == 0)
    {
        printk(KERN_INFO "7-segment device number was created successfully. Major: %d, Minor: %d\n", MAJOR(dev_number), MINOR(dev_number));
    }
    else
    {
        printk(KERN_ERR "%s: Failed to register 7-segment device number\n", __func__);
        return -1;
    }
    
    cdev_init(&chr_dev, &fops);
    return_val = cdev_add(&chr_dev, dev_number, 1);

    if(return_val != 0)
    {   
        unregister_chrdev_region(dev_number, 1);
        printk(KERN_ERR "%s: Failed to add 7-segment device\n", __func__);
        return -1;
    }
    
    sevseg_class = class_create(THIS_MODULE, "sevseg_class");
     
    if(sevseg_class == NULL)
    {
        class_destroy(sevseg_class);
        cdev_del(&chr_dev);
        unregister_chrdev_region(dev_number, 1);
        printk(KERN_ERR "%s: Failed to create 7-segment class\n", __func__);
        return -1;
    }

    sevseg_device = device_create(sevseg_class, NULL, dev_number, NULL, "sevseg_device");

    if(sevseg_device == NULL)
    {
        device_destroy(sevseg_class, dev_number);
        class_destroy(sevseg_class);
        cdev_del(&chr_dev);
        unregister_chrdev_region(dev_number, 1);
        printk(KERN_ERR "%s: Failed to create 7-segment class\n", __func__);
        return -1;
    }

    printk(KERN_INFO "7-segment driver loaded.\n");
    printk(KERN_INFO "Device created successfully.\n");

DEV_REGISTER_ERROR:
    unregister_chrdev_region(dev_number, 1);
    printk(KERN_ERR "%s: Failed to add 7-segment device\n", __func__);
    return -1;

    return 0;
}

static void __exit sevseg_exit(void)
{
    device_destroy(sevseg_class, dev_number);
    class_destroy(sevseg_class);
    cdev_del(&chr_dev);
    unregister_chrdev_region(dev_number, 1);
    printk(KERN_INFO "Device destroyed successfully\n");
    printk(KERN_INFO "7-segment driver unloaded\n");
}

module_init(sevseg_init);
module_exit(sevseg_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Houssem Eddine Marzouk");
MODULE_DESCRIPTION("7-segment driver");