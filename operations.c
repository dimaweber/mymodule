#include "operations.h"
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

struct mymodule_qset* mymodule_follow(struct mymodule_dev* dev, int item)
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

int mymodule_open(struct inode* inode, struct file* filp)
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

int mymodule_release(struct inode* inode, struct file* filp)
{
    return 0;
}

ssize_t mymodule_read(struct file* filp, char __user* buffer, size_t count, loff_t* f_pos)
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

ssize_t mymodule_write(struct file* filp, const char __user* buff, size_t count, loff_t* f_pos)
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
