#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

/* Metadata */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Antigravity Driver Course");
MODULE_DESCRIPTION("Module 1: Virtual FIFO Driver Skeleton");
MODULE_VERSION("0.1");

/* Module Parameter: Buffer Size */
static int buffer_size = 1024;
module_param(buffer_size, int, 0644);
MODULE_PARM_DESC(buffer_size, "Size of the internal FIFO buffer in bytes");

/**
 * @brief Module Initialization Function
 * Called when the module is loaded (insmod).
 */
static int __init vfifo_init(void)
{
    printk(KERN_INFO "vfifo: Module loaded\n");
    printk(KERN_INFO "vfifo: Initialized with buffer_size = %d\n", buffer_size);
    
    /* 
     * In future modules, we will allocate memory and register 
     * the character device here.
     */

    return 0; // Return 0 means success
}

/**
 * @brief Module Exit Function
 * Called when the module is unloaded (rmmod).
 */
static void __exit vfifo_exit(void)
{
    printk(KERN_INFO "vfifo: Module unloaded\n");
    
    /* 
     * In future modules, we will free memory and unregister 
     * the character device here.
     */
}

/* Register entry/exit points */
module_init(vfifo_init);
module_exit(vfifo_exit);
