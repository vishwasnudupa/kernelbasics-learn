#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/sched.h>

/* Metadata */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("anonymous");
MODULE_DESCRIPTION("Module 3: Virtual FIFO with Concurrency");
MODULE_VERSION("0.3");

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
    
    struct mutex lock;              /* Protects buffer, head, tail, size */
    wait_queue_head_t read_queue;   /* Wait here if buffer is empty */
    wait_queue_head_t write_queue;  /* Wait here if buffer is full */
};

/* Global Variables */
static dev_t dev_num;
static struct class *vfifo_class;
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
    filp->private_data = dev;
    return 0;
}

static int vfifo_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t vfifo_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct vfifo_dev *dev = filp->private_data;
    int i;
    int ret = 0;

    /* Acquire lock to check status */
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    /* Loop until data is available */
    while (dev->size == 0) {
        /* Release lock before sleeping */
        mutex_unlock(&dev->lock);

        /* Check for non-blocking mode */
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;

        /* Sleep until data arrives or signal */
        printk(KERN_DEBUG "vfifo: Read blocked (empty)\n");
        if (wait_event_interruptible(dev->read_queue, (dev->size > 0)))
            return -ERESTARTSYS; /* Signal caught */
        
        /* Re-acquire lock */
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;
    }

    /* Limit count */
    if (count > dev->size)
        count = dev->size;

    /* Read data */
    for (i = 0; i < count; i++) {
        if (copy_to_user(buf + i, &dev->buffer[dev->tail], 1)) {
            ret = -EFAULT;
            goto out;
        }
        dev->tail = (dev->tail + 1) % buffer_size;
        dev->size--;
    }
    ret = count;

    /* Wake up writers */
    wake_up_interruptible(&dev->write_queue);

out:
    mutex_unlock(&dev->lock);
    return ret;
}

static ssize_t vfifo_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct vfifo_dev *dev = filp->private_data;
    int free_space;
    int i;
    int ret = 0;

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    free_space = buffer_size - dev->size;

    /* Loop until space is available */
    while (free_space == 0) {
        mutex_unlock(&dev->lock);

        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;

        printk(KERN_DEBUG "vfifo: Write blocked (full)\n");
        if (wait_event_interruptible(dev->write_queue, (buffer_size - dev->size) > 0))
            return -ERESTARTSYS;

        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;
        
        free_space = buffer_size - dev->size;
    }

    /* Limit count */
    if (count > free_space)
        count = free_space;

    /* Write data */
    for (i = 0; i < count; i++) {
        if (copy_from_user(&dev->buffer[dev->head], buf + i, 1)) {
            ret = -EFAULT;
            goto out;
        }
        dev->head = (dev->head + 1) % buffer_size;
        dev->size++;
    }
    ret = count;

    /* Wake up readers */
    wake_up_interruptible(&dev->read_queue);

out:
    mutex_unlock(&dev->lock);
    return ret;
}

/* --- Init and Exit --- */

static int __init vfifo_init(void)
{
    int ret;

    printk(KERN_INFO "vfifo: Initializing Module 3 (Concurrency)...\n");

    ret = alloc_chrdev_region(&dev_num, 0, 1, "vfifo");
    if (ret < 0) return ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    vfifo_class = class_create("vfifo_class");
#else
    vfifo_class = class_create(THIS_MODULE, "vfifo_class");
#endif
    if (IS_ERR(vfifo_class)) {
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(vfifo_class);
    }

    vfifo_device = kzalloc(sizeof(struct vfifo_dev), GFP_KERNEL);
    if (!vfifo_device) {
        class_destroy(vfifo_class);
        unregister_chrdev_region(dev_num, 1);
        return -ENOMEM;
    }

    vfifo_device->buffer = kzalloc(buffer_size, GFP_KERNEL);
    if (!vfifo_device->buffer) {
        kfree(vfifo_device);
        class_destroy(vfifo_class);
        unregister_chrdev_region(dev_num, 1);
        return -ENOMEM;
    }

    /* Initialize Synchronization Primitives */
    mutex_init(&vfifo_device->lock);
    init_waitqueue_head(&vfifo_device->read_queue);
    init_waitqueue_head(&vfifo_device->write_queue);

    cdev_init(&vfifo_device->cdev, &vfifo_fops);
    vfifo_device->cdev.owner = THIS_MODULE;

    ret = cdev_add(&vfifo_device->cdev, dev_num, 1);
    if (ret < 0) {
        kfree(vfifo_device->buffer);
        kfree(vfifo_device);
        class_destroy(vfifo_class);
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

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
