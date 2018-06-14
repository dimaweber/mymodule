#ifndef _OPERATIONS_H
#define _OPERATIONS_H

#include <linux/fs.h>
#include "types.h"

loff_t mymodule_llseek(struct file*, loff_t, int); 
ssize_t mymodule_read(struct file*, char __user*, size_t, loff_t*);
ssize_t mymodule_write(struct file*, const char __user*, size_t, loff_t*);
long mymodule_ioctl(struct file*, unsigned int, unsigned long);
int mymodule_open(struct inode*, struct file*);
int mymodule_release(struct inode*, struct file*);

int mymodule_trim(struct mymodule_dev* dev);

#endif
