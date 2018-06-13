#include <linux/init.h>
#include <linux/version.h> 
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>

MODULE_LICENSE("Dual BSD/GPL");

#define DFLT_DEVICE_COUNT 10
#define DFLT_DEVICE_MAJOR 0
#define DEVICE_STATIC_MAJOR 32
#define DFLT_DEVICE_MINOR 0

#define DEVICE_NAME_PREFIX "mym"

static int device_count = DFLT_DEVICE_COUNT;
static int device_major = DFLT_DEVICE_MAJOR;
static int device_first_minor = DFLT_DEVICE_MINOR;

module_param(device_count, int, S_IRUGO);
module_param(device_major, int, S_IRUGO);
module_param(device_first_minor, int, S_IRUGO);

dev_t device;

static int __init mymodule_init(void)
{
    	int ret = 0;

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
	return 0;        
}

static void __exit mymodule_exit(void)
{
    if (device_major)
    	unregister_chrdev_region(device, device_count);
    printk(KERN_ALERT "Goodbye, cruel world\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);
