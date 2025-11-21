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
#include <linux/ioctl.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

/* Metadata */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("anonymous");
MODULE_DESCRIPTION("Module 4: Virtual FIFO with IOCTL & Deferred Work");
MODULE_VERSION("0.4");

/* Module Parameter: Buffer Size */
static int buffer_size = 1024;
module_param(buffer_size, int, 0644);
MODULE_PARM_DESC(buffer_size, "Size of the internal FIFO buffer in bytes");

/* IOCTL Definitions */
#define VFIFO_IOC_MAGIC 'k'
#define VFIFO_CLEAR     _IO(VFIFO_IOC_MAGIC, 1)
#define VFIFO_SET_MODE  _IOW(VFIFO_IOC_MAGIC, 2, int) /* 0=Manual, 1=Auto-Generate */

/* Device Structure */
struct vfifo_dev {
    struct cdev cdev;
    unsigned char *buffer;
    int head;
    int tail;
    int size;
    
    struct mutex lock;
    wait_queue_head_t read_queue;
    wait_queue_head_t write_queue;

    /* Module 4: Deferred Work */
    struct timer_list data_timer;
    struct work_struct data_work;
    bool auto_generate;
};

/* Global Variables */
static dev_t dev_num;
static struct class *vfifo_class;
static struct vfifo_dev *vfifo_device;

/* Prototypes */
static int vfifo_open(struct inode *inode, struct file *filp);
static int vfifo_release(struct inode *inode, struct file *filp);
static ssize_t vfifo_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t vfifo_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static long vfifo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

static struct file_operations vfifo_fops = {
    .owner = THIS_MODULE,
    .open = vfifo_open,
    .release = vfifo_release,
    .read = vfifo_read,
    .write = vfifo_write,
    .unlocked_ioctl = vfifo_ioctl,
};

/* --- Deferred Work Implementation --- */

/* Workqueue Handler: Runs in process context, can sleep/lock */
static void vfifo_work_handler(struct work_struct *work)
{
    struct vfifo_dev *dev = container_of(work, struct vfifo_dev, data_work);
    char *gen_data = "AUTO ";
    int len = 5;
    int i;

    /* Lock the device */
    if (mutex_lock_interruptible(&dev->lock))
        return;

    /* Check space */
    if (buffer_size - dev->size >= len) {
        for (i = 0; i < len; i++) {
            dev->buffer[dev->head] = gen_data[i];
            dev->head = (dev->head + 1) % buffer_size;
            dev->size++;
        }
        printk(KERN_DEBUG "vfifo: Auto-generated data added\n");
        wake_up_interruptible(&dev->read_queue);
    } else {
        printk(KERN_DEBUG "vfifo: Buffer full, skipping auto-gen\n");
    }

    mutex_unlock(&dev->lock);
}

/* Timer Handler: Runs in atomic context (SoftIRQ), cannot sleep/lock mutex */
static void vfifo_timer_func(struct timer_list *t)
{
    struct vfifo_dev *dev = container_of(t, struct vfifo_dev, data_timer);

    /* Schedule the work to run in process context */
    schedule_work(&dev->data_work);

    /* Restart timer if enabled */
    if (dev->auto_generate) {
        mod_timer(&dev->data_timer, jiffies + msecs_to_jiffies(1000)); /* Every 1 sec */
    }
}

/* --- File Operations --- */

static long vfifo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct vfifo_dev *dev = filp->private_data;
    int ret = 0;
    int val;

    switch (cmd) {
    case VFIFO_CLEAR:
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;
        dev->head = 0;
        dev->tail = 0;
        dev->size = 0;
        mutex_unlock(&dev->lock);
        printk(KERN_INFO "vfifo: Buffer cleared via IOCTL\n");
        break;

    case VFIFO_SET_MODE:
        if (copy_from_user(&val, (int __user *)arg, sizeof(val)))
            return -EFAULT;
        
        dev->auto_generate = (val != 0);
        if (dev->auto_generate) {
            printk(KERN_INFO "vfifo: Enabling auto-generation\n");
            mod_timer(&dev->data_timer, jiffies + msecs_to_jiffies(1000));
        } else {
            printk(KERN_INFO "vfifo: Disabling auto-generation\n");
            del_timer(&dev->data_timer);
        }
        break;

    default:
        return -ENOTTY;
    }
    return ret;
}

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

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    while (dev->size == 0) {
        mutex_unlock(&dev->lock);
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;
        if (wait_event_interruptible(dev->read_queue, (dev->size > 0)))
            return -ERESTARTSYS;
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;
    }

    if (count > dev->size)
        count = dev->size;

    for (i = 0; i < count; i++) {
        if (copy_to_user(buf + i, &dev->buffer[dev->tail], 1)) {
            ret = -EFAULT;
            goto out;
        }
        dev->tail = (dev->tail + 1) % buffer_size;
        dev->size--;
    }
    ret = count;
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
    while (free_space == 0) {
        mutex_unlock(&dev->lock);
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;
        if (wait_event_interruptible(dev->write_queue, (buffer_size - dev->size) > 0))
            return -ERESTARTSYS;
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;
        free_space = buffer_size - dev->size;
    }

    if (count > free_space)
        count = free_space;

    for (i = 0; i < count; i++) {
        if (copy_from_user(&dev->buffer[dev->head], buf + i, 1)) {
            ret = -EFAULT;
            goto out;
        }
        dev->head = (dev->head + 1) % buffer_size;
        dev->size++;
    }
    ret = count;
    wake_up_interruptible(&dev->read_queue);

out:
    mutex_unlock(&dev->lock);
    return ret;
}

/* --- Init and Exit --- */

static int __init vfifo_init(void)
{
    int ret;

    printk(KERN_INFO "vfifo: Initializing Module 4 (Control & Deferred Work)...\n");

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

    mutex_init(&vfifo_device->lock);
    init_waitqueue_head(&vfifo_device->read_queue);
    init_waitqueue_head(&vfifo_device->write_queue);

    /* Init Timer and Workqueue */
    timer_setup(&vfifo_device->data_timer, vfifo_timer_func, 0);
    INIT_WORK(&vfifo_device->data_work, vfifo_work_handler);
    vfifo_device->auto_generate = false;

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
    /* Ensure timer is stopped before cleaning up */
    del_timer_sync(&vfifo_device->data_timer);
    cancel_work_sync(&vfifo_device->data_work);

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
