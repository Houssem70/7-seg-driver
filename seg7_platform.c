// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define NUM_SEGMENTS 8

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

struct seg7_dev {
    struct cdev          cdev;
    dev_t                devt;
    struct class        *class;
    struct device       *device;
    struct gpio_desc    *gpios[NUM_SEGMENTS];
    uint8_t              current_digit;
};

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
    .write = seg_write,
    .llseek  = default_llseek
};

/******************************* Character device file operations *******************************/
/*
** Called when the device file is opened.
*/
static int seg_open(struct inode *inode, struct file *file)
{
    struct seg7_dev *sdev;

    sdev = container_of(inode->i_cdev, struct seg7_dev, cdev);
    file->private_data = sdev;
    pr_info("seg7: device opened\n");
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
static ssize_t seg_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    struct seg7_dev *sdev = file->private_data;
    char kbuf[2];

    if (*off > 0)
        return 0;

    kbuf[0] = sdev->current_digit + '0';
    kbuf[1] = '\n';

    if (len < 2)
        return -EINVAL;

    if (copy_to_user(buf, kbuf, 2))
    {
        pr_err("%s: Failed to copy data to user\n", __func__);
        return -EFAULT;
    }

    *off += 2;
    return 2;
}

/*
** Triggered when the user performs a write operation on the device file.
*/
static ssize_t seg_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    struct seg7_dev *sdev = file->private_data;
    char kbuf;

    if (copy_from_user(&kbuf, buf, 1))
    {
        pr_err("%s: Failed to copy data from user\n", __func__);
        return -EFAULT;
    }

    if (kbuf < '0' || kbuf > '9')
    {
        pr_err("%s: Invalid input from user\n", __func__);
        return -EINVAL;
    }
    
    sdev->current_digit = kbuf - '0';
    for (int i = 0; i < NUM_SEGMENTS; i++)
        gpiod_set_value(sdev->gpios[i], (segment_digits[sdev->current_digit] >> i) & 1);
    
    pr_info("Write function was called\n");
    return 1;
}

/******************************* Sysfs attribute for 'value' *******************************/
static ssize_t value_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    struct seg7_dev *sdev = dev_get_drvdata(dev);
    return scnprintf(buf, PAGE_SIZE, "%u\n", sdev->current_digit);
}

static ssize_t value_store(struct device *dev,
                           struct device_attribute *attr,
                           const char *buf, size_t count)
{
    struct seg7_dev *sdev = dev_get_drvdata(dev);
    unsigned long val;

    if (kstrtoul(buf, 0, &val) || val > 9)
        return -EINVAL;

    sdev->current_digit = val;
    for (int i = 0; i < NUM_SEGMENTS; i++)
        gpiod_set_value(sdev->gpios[i], (segment_digits[sdev->current_digit] >> i) & 1);

    return count;
}
static DEVICE_ATTR_RW(value);

/* OF match table */
static const struct of_device_id seg7_of_match[] = {
    { .compatible = "rpi,seg7" },
    {}
};
MODULE_DEVICE_TABLE(of, seg7_of_match);

/******************************* Probe and remove *******************************/
static int seg7_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct seg7_dev *sdev;
    int ret;

    sdev = devm_kzalloc(dev, sizeof(*sdev), GFP_KERNEL);
    if (!sdev)
        return -ENOMEM;

    platform_set_drvdata(pdev, sdev);

    /* Request and configure GPIOs */
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        sdev->gpios[i] = devm_gpiod_get_index(dev, NULL, i, GPIOD_OUT_LOW);
        if (IS_ERR(sdev->gpios[i])) {
            dev_err(dev, "failed to get GPIO %d\n", i);
            return PTR_ERR(sdev->gpios[i]);
        }
    }

    /* Allocate char device number */
    ret = alloc_chrdev_region(&sdev->devt, 0, 1, "sevenseg");
    if (ret)
    {
        pr_err("%s: Failed to register 7-segment device number (%d)\n", __func__, ret);
        return ret;
    }
        
    pr_info("7-segment device number was created successfully. Major: %d, Minor: %d\n", MAJOR(sdev->devt), MINOR(sdev->devt));

    //Creating cdev structure
    cdev_init(&sdev->cdev, &fops);
    sdev->cdev.owner = THIS_MODULE;

    //Adding character device to the system
    ret = cdev_add(&sdev->cdev, sdev->devt, 1);
    if (ret)
    {
        pr_err("%s: Failed to add 7-segment device to the system (%d)\n", __func__, ret);
        goto unregister_chrdev;
    }
        
    //Creating struct class
    sdev->class = class_create("sevenseg");
    if (IS_ERR(sdev->class)) {
        pr_err("%s: Failed to create 7-segment class (%d)\n", __func__, ret);
        ret = PTR_ERR(sdev->class);
        goto del_cdev;
    }

    //Creating device
    sdev->device = device_create(sdev->class, dev, sdev->devt, NULL, "sevenseg");
    if (IS_ERR(sdev->device)) {
        pr_err("%s: Failed to create 7-segment device (%d)\n", __func__, ret);
        ret = PTR_ERR(sdev->device);
        goto destroy_class;
    }

    dev_set_drvdata(sdev->device, sdev);

    //Creating device file under sysfs
    ret = device_create_file(sdev->device, &dev_attr_value);
    if (ret)
    {    
        pr_warn("%s: Failed to create sysfs attribute (%d)\n", __func__, ret);
        goto destroy_device;
    }

    dev_info(dev, "seg7 platform driver probed\n");
    return 0;

destroy_device:
    device_destroy(sdev->class, sdev->devt);
destroy_class:
    class_destroy(sdev->class);
del_cdev:
    cdev_del(&sdev->cdev);
unregister_chrdev:
    unregister_chrdev_region(sdev->devt, 1);
    return ret;
}

static int seg7_remove(struct platform_device *pdev)
{
    struct seg7_dev *sdev = platform_get_drvdata(pdev);

    device_remove_file(sdev->device, &dev_attr_value);
    device_destroy(sdev->class, sdev->devt);
    class_destroy(sdev->class);
    cdev_del(&sdev->cdev);
    unregister_chrdev_region(sdev->devt, 1);

    dev_info(&pdev->dev, "seg7 platform driver removed\n");
    return 0;
}

static struct platform_driver seg7_driver = {
    .probe  = seg7_probe,
    .remove = seg7_remove,
    .driver = {
        .name           = "seg7",
        .of_match_table = of_match_ptr(seg7_of_match),
    },
};

module_platform_driver(seg7_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Houssem Eddine Marzouk");
MODULE_DESCRIPTION("Platform driver for common cathode 7-segment display");