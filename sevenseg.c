// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#define NUM_SEGMENTS 8

static int gpio_pins[NUM_SEGMENTS];
static uint8_t current_digit = 0;
dev_t dev_num = 0;
static struct cdev seg_cdev;
static struct class * seg_class;
static struct device * seg_device;

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
static int seg_parse_dt(void);

/*************** Driver functions **********************/
static int     seg_open(struct inode *inode, struct file *file);
static int     seg_close (struct inode *inode, struct file *file);
static ssize_t seg_read (struct file *file, char __user * usr_buff, size_t len, loff_t *off);
static ssize_t seg_write(struct file *file, const char __user *usr_buff, size_t len, loff_t *off);
static ssize_t value_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t value_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
/******************************************************/

//File operation structure 
const struct file_operations fops={
    .owner = THIS_MODULE,
    .open = seg_open,
    .release = seg_close,
    .read = seg_read,
    .write = seg_write
};

// Creates "value" file with read/write methods
static DEVICE_ATTR_RW(value); 

//Device Tree Parsing function
static int seg_parse_dt(void)
{
    struct device_node *np;
    int i, ret;

    np = of_find_compatible_node(NULL, NULL, "rpi,seg7");
    if (!np) {
        pr_err("seg7: DT node \"rpi,seg7\" not found\n");
        return -ENODEV;
    }

    for (i = 0; i < NUM_SEGMENTS; i++) {
        gpio_pins[i] = of_get_named_gpio(np, "gpios", i);
        if (!gpio_is_valid(gpio_pins[i])) {
            pr_err("seg7: invalid GPIO at index %d\n", i);
            ret = -EINVAL;
            goto put_node;
        }

        ret = gpio_request(gpio_pins[i], "seg7_gpio");
        if (ret) {
            pr_err("seg7: gpio_request(%d) failed: %d\n",
                   gpio_pins[i], ret);
            goto put_node;
        }

        ret = gpio_direction_output(gpio_pins[i], 0);
        if (ret) {
            pr_err("seg7: gpio_direction_output(%d) failed: %d\n",
                   gpio_pins[i], ret);
            goto free_gpio;
        }

        continue;

    free_gpio:
        gpio_free(gpio_pins[i]);
    put_node:
        of_node_put(np);
        return ret;
    }

    of_node_put(np);
    pr_info("%s: acquired %d GPIOs from DT\n", __func__, NUM_SEGMENTS);
    return 0;
}

// Digit display function 
static void seg_display_digit(uint8_t digit)
{
    int i;
    uint8_t val;

    current_digit = digit;
    val = segment_digits[digit];

    for(i=0; i<NUM_SEGMENTS; i++)
        gpio_set_value(gpio_pins[i], (val >> i) & 1);
}

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
static ssize_t seg_read(struct file *file, char __user *usr_buff, size_t len, loff_t *off)
{
    char kbuff[2];

    if (*off > 0)
        return 0;

    kbuff[0] = current_digit + '0';
    kbuff[1] = '\n';

    if (len < 2)
        return -EINVAL;

    if (copy_to_user(usr_buff, kbuff, 2))
    {
        pr_err("%s: Failed to copy data to user\n", __func__);
        return -EFAULT;
    }


    *off += 2;  // mark file position
    pr_info("%s: 7-segment device file read\n", __func__);

    return 2;   // number of bytes written
}

/*
** Triggered when the user performs a write operation on the device file.
*/
static ssize_t seg_write(struct file *file, const char __user *usr_buff, size_t len, loff_t *off)
{
    char kbuff[2];

    if(copy_from_user(kbuff, usr_buff, 1))
    {
        pr_err("%s: Failed to copy data from user\n", __func__);
        return -EFAULT;
    }

    if(kbuff[0] < '0' || kbuff[0] > '9')
    {
        pr_err("%s: Invalid input from user\n", __func__);
        return -EINVAL;
    }

    pr_info("%s: 7-segment device file write\n", __func__);
    seg_display_digit(kbuff[0] - '0');
    return 1;
}

/*
** Triggered when the user reads to the sysfs 'value' attribute.
 */
static ssize_t value_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "%u\n", current_digit);
}

/*
** Handle user write to sysfs 'value'; update 7-segment display.
 */
static ssize_t value_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long val;  
    
    if(kstrtoul(buf, 0, &val) || val > 9)
    {
        pr_err("%s: Invalid input from user\n", __func__);
        return -EINVAL;
    }

    seg_display_digit(val);
    return count;
}

/********************* Init function **************************/
static int __init seg_driver_init(void)
{
    int ret;
    
    //Allocating Major number
    ret = alloc_chrdev_region(&dev_num, 0, 1, "sevenseg");
    if(ret)
    {
        pr_err("%s: Failed to register 7-segment device number (%d)\n", __func__, ret);
        return ret;

    }

    pr_info("7-segment device number was created successfully. Major: %d, Minor: %d\n", MAJOR(dev_num), MINOR(dev_num));

    //Creating cdev structure
    cdev_init(&seg_cdev, &fops);
    seg_cdev.owner = THIS_MODULE;

    //Adding character device to the system
    ret = cdev_add(&seg_cdev, dev_num, 1);

    if(ret)
    {           
        pr_err("%s: Failed to add 7-segment device to the system (%d)\n", __func__, ret);
        goto unregister;
    }

    //Creating struct class
    seg_class = class_create("sevenseg");
     
    if(IS_ERR(seg_class))
    {
        pr_err("%s: Failed to create 7-segment class (%d)\n", __func__, ret);
        ret = PTR_ERR(seg_class);
        goto del_cdev;
    }

    //Creating device
    seg_device = device_create(seg_class, NULL, dev_num, NULL, "sevenseg");

    if(IS_ERR(seg_device))
    {
        pr_err("%s: Failed to create 7-segment device (%d)\n", __func__, ret);
        ret = PTR_ERR(seg_device);
        goto destroy_class;
    }

    //Creating device file under sysfs
    ret = device_create_file(seg_device, &dev_attr_value);
    if (ret)
    {    
        pr_warn("%s: Failed to create sysfs attribute (%d)\n", __func__, ret);
        goto destroy_device;
    }
    //Parsing the device tree to get GPIO pin assignments
    ret = seg_parse_dt();
    if (ret) {
        pr_err("%s: failed to parse DT (%d)\n", __func__, ret);
        goto remove_device_file;
    }

    pr_info("7-segment driver loaded\n");
    return 0;

remove_device_file:
    device_remove_file(seg_device, &dev_attr_value);
destroy_device:
    device_destroy(seg_class, dev_num);
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
    int i;
    for(i=0; i<NUM_SEGMENTS; i++)
        gpio_free(gpio_pins[i]);

    device_remove_file(seg_device, &dev_attr_value);
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