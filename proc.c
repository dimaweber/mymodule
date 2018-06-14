#include "types.h"
#include "proc.h"
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

void* mymodule_seq_start(struct seq_file* sfile, loff_t* pos)
{
    if (*pos >= device_count)
        return NULL;
    return mymodule_devices + *pos;
}

void* mymodule_seq_next(struct seq_file* sfile, void* iter, loff_t* pos)
{
    (*pos)++;
    return mymodule_seq_start(sfile, pos);
}

void mymodule_seq_stop(struct seq_file* sfile, void* v)
{
    return;
}

int mymodule_seq_show(struct seq_file* sfile, void* v)
{
    struct mymodule_dev* dev = (struct mymodule_dev*)v;
    struct mymodule_qset* d;
    int i;

    if(mutex_lock_interruptible(&dev->mutex))
        return -ERESTARTSYS;
    seq_printf(sfile, "\nDevice %i: qset %i, q %i, sz %li\n",
               (int)(dev - mymodule_devices), dev->qset, dev->quantum, dev->size);
    for(d = dev->data; d; d=d->next)
    {
        seq_printf(sfile, "\titem @ %p, qset @ %p\n",d, d->data);
        if (d->data && !d->next)
            for(i=0;i<dev->qset;i++)
                if (d->data[i])
                    seq_printf(sfile, "\t\t%4i: %8p\n",i, d->data[i]);
    }
    mutex_unlock(&dev->mutex);
    return 0;
}

static struct seq_operations mymodule_seq_ops = {
    .start = mymodule_seq_start,
    .next  = mymodule_seq_next,
    .stop  = mymodule_seq_stop,
    .show  = mymodule_seq_show
};

static int mymodule_proc_open(struct inode* inode, struct file* file)
{
    return seq_open(file, &mymodule_seq_ops);
}

static struct file_operations mymodule_proc_ops = {
    .owner = THIS_MODULE,
    .open = mymodule_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release
};

struct proc_dir_entry* proc_debug_entry = NULL;
void mymodule_proc_register(void)
{
    proc_debug_entry = proc_create("driver/mym_debug", 0, NULL, &mymodule_proc_ops);
}

void mymodule_proc_unrgister(void)
{
    if (proc_debug_entry)
        proc_remove(proc_debug_entry);
}
