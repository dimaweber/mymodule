#ifndef _TYPES_H
#define _TYPES_H
#include <linux/cdev.h>

extern int mymodule_quantum;
extern int mymodule_qset;
extern int device_major;
extern int device_first_minor;

struct mymodule_qset {
    void** data;
    struct mymodule_qset* next;
};

struct mymodule_dev {
        struct mymodule_qset* data;
        int quantum;
        int qset;
        unsigned long size;
        unsigned int access_key;
        struct mutex mutex;
        struct cdev cdev;
};

extern int device_count;
extern struct mymodule_dev* mymodule_devices;
#endif
