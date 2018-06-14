#ifndef _TYPES_H
#define _TYPES_H
#include <linux/cdev.h>

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
#endif
