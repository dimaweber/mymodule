#include "operations.h"
#include "types.h"
#include "proc.h"

#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/mutex.h>

MODULE_LICENSE("Dual BSD/GPL");

int device_count = DFLT_DEVICE_COUNT;
int device_major = DFLT_DEVICE_MAJOR;
int device_first_minor = DFLT_DEVICE_MINOR;
int mymodule_quantum = DFLT_MYMODULE_QUANTUM;
int mymodule_qset = DFLT_MYMODULE_QSET;

module_param(device_count, int, S_IRUGO);
module_param(device_major, int, S_IRUGO);
module_param(device_first_minor, int, S_IRUGO);
module_param(mymodule_quantum, int, S_IRUGO);
module_param(mymodule_qset, int, S_IRUGO);

dev_t device;

struct mymodule_dev* mymodule_devices = NULL;

static void  mymodule_exit(void)
{
    int i;
    mymodule_proc_unrgister();
    if (mymodule_devices)
    {
        for (i=0; i<device_count; i++)
        {
            mutex_lock(&mymodule_devices[i].mutex);
            mymodule_trim(mymodule_devices + i);
            mutex_unlock(&mymodule_devices[i].mutex);
            cdev_del(&mymodule_devices[i].cdev);
        }
        kfree(mymodule_devices);
    }
    if (device_major)
            unregister_chrdev_region(device, device_count);

    printk(KERN_INFO "Goodbye, cruel world\n");
}

static int __init mymodule_init(void)
{
        int ret = 0;
    int i;

        printk(KERN_INFO "Hello world\n");
        printk(KERN_INFO  "the process is \"%s\" (pid %i). the kernel version module compiled with is %x\n", current->comm, current->pid, LINUX_VERSION_CODE);

        if (device_major != DFLT_DEVICE_MAJOR)
        {
            device = MKDEV(DEVICE_STATIC_MAJOR, device_first_minor);
            ret = register_chrdev_region(device, device_count, DEVICE_NAME_PREFIX);
        }
    else
    {
        ret = alloc_chrdev_region(&device, device_first_minor, device_count, DEVICE_NAME_PREFIX);
    }

    if(ret)
    {
        printk(KERN_WARNING "mymodule: can't get major %d device number\n", device_major);
            return ret;
    }

    device_major = MAJOR(device);
    printk(KERN_INFO "major number %d registered for mym devices\n", device_major);


    mymodule_devices = kmalloc (sizeof(struct mymodule_dev) * device_count, GFP_KERNEL);
    if (!mymodule_devices)
    {
        ret = -ENOMEM;
        goto fail;
    }
    memset(mymodule_devices, 0, sizeof(struct mymodule_dev) * device_count);

    for (i=0; i<device_count; i++)
    {
        mymodule_devices[i].quantum = mymodule_quantum;
        mymodule_devices[i].qset = mymodule_qset;
        mutex_init(&mymodule_devices[i].mutex);
        mymodule_setup_cdev(&mymodule_devices[i], i);
    }

    mymodule_proc_register();

    return 0;

    fail:
    mymodule_exit();
    return ret;
}

module_init(mymodule_init);
module_exit(mymodule_exit);
