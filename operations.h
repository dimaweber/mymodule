#ifndef _OPERATIONS_H
#define _OPERATIONS_H

#include "types.h"

void mymodule_setup_cdev(struct mymodule_dev* m_dev, int index);
int mymodule_trim(struct mymodule_dev* dev);

#endif
