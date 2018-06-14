#include "operations.h"
#include "types.h"

#include <linux/init.h>
#include <linux/version.h> 
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("Dual BSD/GPL");

#define DFLT_DEVICE_COUNT 10
#define DFLT_DEVICE_MAJOR 0
#define DEVICE_STATIC_MAJOR 32
#define DFLT_DEVICE_MINOR 0

#define DEVICE_NAME_PREFIX "mym"

static int device_count = DFLT_DEVICE_COUNT;
static int device_major = DFLT_DEVICE_MAJOR;
static int device_first_minor = DFLT_DEVICE_MINOR;
int mymodule_quantum = 4096;
int mymodule_qset = 1024;

module_param(device_count, int, S_IRUGO);
module_param(device_major, int, S_IRUGO);
module_param(device_first_minor, int, S_IRUGO);
module_param(mymodule_quantum, int, S_IRUGO);
module_param(mymodule_qset, int, S_IRUGO);

dev_t device;

struct file_operations mymodule_fops = {
	.owner   = THIS_MODULE,
//	.llseek  = mymodule_llseek,
	.read    = mymodule_read,
	.write   = mymodule_write,
//	.unlocked_ioctl   = mymodule_ioctl,
	.open    = mymodule_open,
	.release = mymodule_release, 
};

static void mymodule_setup_cdev(struct mymodule_dev* m_dev, int index)
{
	int err;
	int devno = MKDEV(device_major, device_first_minor + index);
	
	cdev_init(&m_dev->cdev, &mymodule_fops);
	m_dev->cdev.owner = THIS_MODULE;
	m_dev->cdev.ops   = &mymodule_fops;
	err = cdev_add(&m_dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding mym%i", err, index);
} 

struct mymodule_dev* mymodule_devices = NULL;
struct proc_dir_entry* proc_debug_entry = NULL;

int mymodule_read_proc (char* buf, char** start, off_t offset, int count, int* eof, void* data)
{
	int i, j, len = 0;
	int limit = count - 80;
	for (i=0; i< device_count && len <= limit; i++)
	{
		struct mymodule_dev* d = mymodule_devices + i;
		struct mymodule_qset* qs = d->data;
		if (mutex_lock_interruptible(&d->mutex))
			return -ERESTARTSYS;
		len += sprintf(buf+len, "\nDevice %i: qset %i, q %i, sz %li\n",
						   i, d->qset, d->quantum, d->size);
		for(;qs && len <= limit; qs = qs->next)
		{
			len += sprintf(buf+len, "   item @ %p, qset @ %p\n", qs, qs->data);
			if (qs->data  && !qs->next)
			{
				for (j=0; j<d->qset; j++)
				{
					if(qs->data[j])
						len += sprintf(buf + len, "      %4i: %8p\n", j, qs->data[j]);
				}
			}
		}
		mutex_unlock(&d->mutex);
	}
	*eof = 1;
	return len;
}

static void  mymodule_exit(void)
{
	int i;
	if (mymodule_devices)
	{
		for (i=0; i<device_count; i++)
		{
			mymodule_trim(mymodule_devices + i);
			cdev_del(&mymodule_devices[i].cdev);
		}
		kfree(mymodule_devices);
	}
	if (device_major)
    		unregister_chrdev_region(device, device_count);

	proc_remove(proc_debug_entry);
	
	printk(KERN_ALERT "Goodbye, cruel world\n");
}

static int __init mymodule_init(void)
{
    	int ret = 0;
	int i;

    	printk(KERN_ALERT "Hello world\n");
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


//	proc_debug_entry = create_proc_read_entry("driver/mym_debug", 0, NULL, mymodule_read_proc, NULL);
	return 0;        
	
	fail:
	mymodule_exit();
	return ret;
}

module_init(mymodule_init);
module_exit(mymodule_exit);
