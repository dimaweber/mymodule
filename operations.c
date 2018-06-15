#include "operations.h"
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
#  include <asm/uaccess.h>
#else
#  include <linux/uaccess.h>
#endif

extern int mymodule_quantum;
extern int mymodule_qset;
extern int device_major;
extern int device_first_minor;

static struct mymodule_qset* mymodule_follow(struct mymodule_dev* dev, int item)
{
    struct mymodule_qset* dptr = dev->data;
    if (!dptr)
    {
        dptr = dev->data = kmalloc(sizeof(struct mymodule_qset), GFP_KERNEL);
        if (!dptr)
            return NULL;
        memset(dptr, 0, sizeof(struct mymodule_qset));
    }
    while (item--)
    {
        if (!dptr->next)
        {
            dptr->next = kmalloc(sizeof(struct mymodule_qset), GFP_KERNEL);
            if (!dptr->next)
                return NULL;
            memset(dptr->next, 0, sizeof(struct mymodule_qset));
        }
        dptr = dptr->next;
    }
    return dptr;
}

int mymodule_trim(struct mymodule_dev* dev)
{
    struct mymodule_qset* next = dev->data;
    struct mymodule_qset* dptr = dev->data;
    int qset = dev->qset;
    int i;
    while (dptr)
    {
        next = dptr->next;
        if (dptr->data)
        {
            for (i=0; i<qset; i++)
            {
                kfree(dptr->data[i]);
            }
            kfree(dptr->data);
            dptr->data = NULL;
        }
        kfree(dptr);
        dptr=next;
    }
    dev->size = 0;
    dev->quantum = mymodule_quantum;
    dev->qset = mymodule_qset;
    dev->data = NULL;

    return 0;
}

static int mymodule_open(struct inode* inode, struct file* filp)
{
    if (!filp->private_data)
    {
        struct mymodule_dev* m_dev;
        m_dev = container_of(inode->i_cdev, struct mymodule_dev, cdev);
        filp->private_data = m_dev;
    }

    if ( (filp->f_flags & O_ACCMODE) == O_WRONLY)
    {
        mymodule_trim(filp->private_data);
    }

    return 0;
}

static int mymodule_release(struct inode* inode, struct file* filp)
{
    return 0;
}

static ssize_t mymodule_read(struct file* filp, char __user* buffer, size_t count, loff_t* f_pos)
{
    struct mymodule_dev* dev = filp->private_data;
    struct mymodule_qset* dptr = dev->data;
    int quantum = dev->quantum;
    int qset = dev->qset;
    int itemsize = quantum * qset;
    int item, s_pos, q_pos, rest;
    ssize_t retval = 0;

    if (mutex_lock_interruptible(&dev->mutex))
        return -ERESTARTSYS;

    if (dev->size <= *f_pos)
        goto out;

    if (*f_pos + count > dev->size)
        count = dev->size - *f_pos;

    item = (long)*f_pos / itemsize;
    rest = (long)*f_pos % itemsize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;

    dptr = mymodule_follow(dev, item);
    if (!dptr || !dptr->data || !dptr->data[s_pos])
        goto out;

    if (count > quantum - s_pos)
        count = quantum - s_pos;

    if (copy_to_user(buffer, dptr->data[item] + q_pos, count))
    {
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    retval = count;

    out:
    mutex_unlock(&dev->mutex);
    return retval;
}

static ssize_t mymodule_write(struct file* filp, const char __user* buff, size_t count, loff_t* f_pos)
{
    struct mymodule_dev* dev = filp->private_data;
    struct mymodule_qset* dptr;
    int quantum = dev->quantum;
    int qset = dev->qset;
    int itemsize = quantum * qset;
    int item, q_pos, s_pos, rest;
    ssize_t retval = -ENOMEM;

    if (mutex_lock_interruptible(&dev->mutex))
        return -ERESTARTSYS;

    item = (long)*f_pos / itemsize;
    rest = (long)*f_pos % itemsize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;

    dptr = mymodule_follow(dev, item);
    if (!dptr)
        goto out;

    if (!dptr->data)
    {
        dptr->data = kmalloc(qset * sizeof(char*), GFP_KERNEL);
        if (!dptr->data)
            goto out;
        memset(dptr->data, 0, qset * sizeof(char*));
    }

    if (!dptr->data[s_pos])
    {
        dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
        if (!dptr->data[s_pos])
            goto out;
    }

    if (count > quantum - q_pos)
        count = quantum - q_pos;

    if (copy_from_user(dptr->data[s_pos]+q_pos, buff, count))
    {
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    retval = count;

    if (dev->size < *f_pos)
        dev->size = *f_pos;

    out:
    mutex_unlock(&dev->mutex);
    return retval;
}

static loff_t mymodule_llseek(struct file * filp, loff_t off, int whence)
{
    struct mymodule_dev* dev = filp->private_data;
    loff_t newpos;
    switch (whence)
    {
        case 0:
            newpos = off;
            break;
        case 1:
            newpos = filp->f_pos + off;
            break;
        case 2:
            newpos = dev->size + off;
            break;
        default:
            return -EINVAL;
    }
    if (newpos < 0)
        return -EINVAL;
    filp->f_pos = newpos;
    return newpos;
}

struct file_operations mymodule_fops = {
    .owner   = THIS_MODULE,
    .llseek  = mymodule_llseek,
    .read    = mymodule_read,
    .write   = mymodule_write,
    //	.unlocked_ioctl   = mymodule_ioctl,
    .open    = mymodule_open,
    .release = mymodule_release,
};

void mymodule_setup_cdev(struct mymodule_dev* m_dev, int index)
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
