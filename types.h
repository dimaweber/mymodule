#ifndef _TYPES_H
#define _TYPES_H
#include <linux/cdev.h>

#define DFLT_DEVICE_COUNT 10
#define DFLT_DEVICE_MAJOR 0
#define DEVICE_STATIC_MAJOR 32
#define DFLT_DEVICE_MINOR 0
#define DFLT_MYMODULE_QUANTUM 4096
#define DFLT_MYMODULE_QSET 1024

#define DEVICE_NAME_PREFIX "mym"

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
