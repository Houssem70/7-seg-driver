// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>

#define NUM_SEGMENTS 8

static int gpio_pins[NUM_SEGMENTS];
dev_t dev_num = 0;
struct cdev seg_cdev;
struct class * seg_class;
struct device * seg_device;

// Segment encoding for digits 0–9
static const uint8_t segment_digits[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};

static int __init seg_driver_init(void);
static void __exit seg_driver_exit(void);

/*************** Driver functions **********************/
static int     seg_open(struct inode *inode, struct file *file);
static int     seg_close (struct inode *inode, struct file *file);
static ssize_t seg_read (struct file *file, char __user * usr_buff, size_t len, loff_t *off);
static ssize_t seg_write(struct file *file, const char __user *usr_buff, size_t len, loff_t *off);
/******************************************************/

//File operation structure 
const struct file_operations fops={
    .owner = THIS_MODULE,
    .open = seg_open,
    .release = seg_close,
    .read = seg_read,
    .write = seg_write
};

/*
** Called when the device file is opened.
*/
static int seg_open(struct inode *inode, struct file *file)
{   
    pr_info("%s: 7-segment device file opened\n", __func__);
    return 0;
}

/*
** Called when the device file is closed.
*/
static int seg_close (struct inode *inode, struct file *file)
{
    pr_info("%s: 7-segment device file closed\n", __func__);
    return 0;
}

/*
** Triggered when the user performs a read operation on the device file.
*/
static ssize_t seg_read (struct file *file, char __user * usr_buff, size_t len, loff_t *off)
{
    pr_info("%s: 7-segment device file read function called\n", __func__);
    return 0;
}

/*
** Triggered when the user performs a write operation on the device file.
*/
static ssize_t seg_write(struct file *file, const char __user *usr_buff, size_t len, loff_t *off)
{
    pr_info("%s: 7-segment device file write function called\n", __func__);
    return 0;
}

/********************* Init function **************************/
static int __init seg_driver_init(void)
{
    int ret;
    
    //Allocating Major number
    ret = alloc_chrdev_region(&dev_num, 0, 1, "7-segment");
    if(ret)
    {
        pr_err("%s: Failed to register 7-segment device number\n", __func__);
        return ret;

    }
    else
    {
        pr_info("7-segment device number was created successfully. Major: %d, Minor: %d\n", MAJOR(dev_num), MINOR(dev_num));
    }

    //Creating cdev structure
    cdev_init(&seg_cdev, &fops);
    seg_cdev.owner = THIS_MODULE;

    //Adding character device to the system
    ret = cdev_add(&seg_cdev, dev_num, 1);

    if(ret)
    {           
        pr_err("%s: Failed to add 7-segment device\n", __func__);
        goto unregister;
    }

    //Creating struct class
    seg_class = class_create("sevenseg_class");
     
    if(IS_ERR(seg_class))
    {
        pr_err("%s: Failed to create 7-segment class\n", __func__);
        ret = PTR_ERR(seg_class);
        goto del_cdev;
    }

    //Creating device
    seg_device = device_create(seg_class, NULL, dev_num, NULL, "sevenseg_device");

    if(IS_ERR(seg_device))
    {
        pr_err("%s: Failed to create 7-segment class\n", __func__);
        ret = PTR_ERR(seg_device);
        goto destroy_class;
    }

    pr_info("7-segment driver loaded\n");
    return 0;

destroy_class:
    class_destroy(seg_class);
del_cdev:
    cdev_del(&seg_cdev);
unregister:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

/********************* Exit function **************************/
static void __exit seg_driver_exit(void)
{
    device_destroy(seg_class, dev_num);
    class_destroy(seg_class);
    cdev_del(&seg_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("7-segment driver unloaded\n");
}

module_init(seg_driver_init);
module_exit(seg_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Houssem Eddine Marzouk");
MODULE_DESCRIPTION("A simple common cathode 7-segment display driver");