#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h>

/* Metadata */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Antigravity Driver Course");
MODULE_DESCRIPTION("Module 2: Virtual FIFO Character Device");
MODULE_VERSION("0.2");

/* Module Parameter: Buffer Size */
static int buffer_size = 1024;
module_param(buffer_size, int, 0644);
MODULE_PARM_DESC(buffer_size, "Size of the internal FIFO buffer in bytes");

/* Device Structure */
struct vfifo_dev {
    struct cdev cdev;
    unsigned char *buffer;
    int head; /* Write pointer */
    int tail; /* Read pointer */
    int size; /* Current data size */
    /* We will add synchronization primitives in Module 3 */
};

/* Global Variables */
static dev_t dev_num;               /* Major/Minor number */
static struct class *vfifo_class;   /* Sysfs class for auto-creation in /dev */
static struct vfifo_dev *vfifo_device;

/* File Operations Prototypes */
static int vfifo_open(struct inode *inode, struct file *filp);
static int vfifo_release(struct inode *inode, struct file *filp);
static ssize_t vfifo_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t vfifo_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);

static struct file_operations vfifo_fops = {
    .owner = THIS_MODULE,
    .open = vfifo_open,
    .release = vfifo_release,
    .read = vfifo_read,
    .write = vfifo_write,
};

/* --- File Operations Implementation --- */

static int vfifo_open(struct inode *inode, struct file *filp)
{
    struct vfifo_dev *dev = container_of(inode->i_cdev, struct vfifo_dev, cdev);
    filp->private_data = dev; /* Store pointer to our device data */
    return 0;
}

static int vfifo_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t vfifo_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct vfifo_dev *dev = filp->private_data;
    int bytes_to_read;
    int i;
    
    /* Check if empty */
    if (dev->size == 0) {
        return 0; /* EOF */
    }

    /* Limit count to available data */
    if (count > dev->size) {
        count = dev->size;
    }

    /* Copy data to user space byte-by-byte for simplicity in circular buffer */
    /* Note: In production, we'd optimize this to 1 or 2 calls to copy_to_user */
    for (i = 0; i < count; i++) {
        if (copy_to_user(buf + i, &dev->buffer[dev->tail], 1)) {
            return -EFAULT;
        }
        dev->tail = (dev->tail + 1) % buffer_size;
        dev->size--;
    }

    return count;
}

static ssize_t vfifo_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct vfifo_dev *dev = filp->private_data;
    int free_space;
    int i;

    free_space = buffer_size - dev->size;

    /* Check if full */
    if (free_space == 0) {
        return -ENOSPC; /* No space left on device */
    }

    /* Limit count to available space */
    if (count > free_space) {
        count = free_space;
    }

    /* Copy data from user space */
    for (i = 0; i < count; i++) {
        if (copy_from_user(&dev->buffer[dev->head], buf + i, 1)) {
            return -EFAULT;
        }
        dev->head = (dev->head + 1) % buffer_size;
        dev->size++;
    }

    return count;
}

/* --- Init and Exit --- */

static int __init vfifo_init(void)
{
    int ret;

    printk(KERN_INFO "vfifo: Initializing Module 2...\n");

    /* 1. Allocate Major/Minor numbers */
    ret = alloc_chrdev_region(&dev_num, 0, 1, "vfifo");
    if (ret < 0) {
        printk(KERN_ERR "vfifo: Failed to allocate major number\n");
        return ret;
    }

    /* 2. Create Device Class (for /dev/vfifo0 creation) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    vfifo_class = class_create("vfifo_class");
#else
    vfifo_class = class_create(THIS_MODULE, "vfifo_class");
#endif
    if (IS_ERR(vfifo_class)) {
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(vfifo_class);
    }

    /* 3. Allocate Device Structure */
    vfifo_device = kzalloc(sizeof(struct vfifo_dev), GFP_KERNEL);
    if (!vfifo_device) {
        class_destroy(vfifo_class);
        unregister_chrdev_region(dev_num, 1);
        return -ENOMEM;
    }

    /* 4. Allocate Internal Buffer */
    vfifo_device->buffer = kzalloc(buffer_size, GFP_KERNEL);
    if (!vfifo_device->buffer) {
        kfree(vfifo_device);
        class_destroy(vfifo_class);
        unregister_chrdev_region(dev_num, 1);
        return -ENOMEM;
    }

    /* 5. Initialize cdev */
    cdev_init(&vfifo_device->cdev, &vfifo_fops);
    vfifo_device->cdev.owner = THIS_MODULE;

    /* 6. Add cdev to kernel */
    ret = cdev_add(&vfifo_device->cdev, dev_num, 1);
    if (ret < 0) {
        kfree(vfifo_device->buffer);
        kfree(vfifo_device);
        class_destroy(vfifo_class);
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    /* 7. Create Device Node /dev/vfifo0 */
    device_create(vfifo_class, NULL, dev_num, NULL, "vfifo0");

    printk(KERN_INFO "vfifo: Registered device with Major %d\n", MAJOR(dev_num));
    return 0;
}

static void __exit vfifo_exit(void)
{
    device_destroy(vfifo_class, dev_num);
    cdev_del(&vfifo_device->cdev);
    kfree(vfifo_device->buffer);
    kfree(vfifo_device);
    class_destroy(vfifo_class);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "vfifo: Module unloaded\n");
}

module_init(vfifo_init);
module_exit(vfifo_exit);
