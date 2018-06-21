#ifndef _OPERATIONS_H
#define _OPERATIONS_H

#include "types.h"

#define MYMODULE_IOC_MAGIC 'w'
#define MYMODULE_IOCRESET    _IO  (MYMODULE_IOC_MAGIC, 0)
// S -- set: through a ptr
#define MYMODULE_IOCSQUANTUM _IOW (MYMODULE_IOC_MAGIC, 1, int)
#define MYMODULE_IOCSQSET    _IOW (MYMODULE_IOC_MAGIC, 2, int)
// T -- tell: directly with the argument value
#define MYMODULE_IOCTQUANTUM _IO  (MYMODULE_IOC_MAGIC, 3)
#define MYMODULE_IOCTQSET    _IO  (MYMODULE_IOC_MAGIC, 4)
// G -- get: reply by setting through a pointer
#define MYMODULE_IOCGQUANTUM _IOR (MYMODULE_IOC_MAGIC, 5, int)
#define MYMODULE_IOCGQSET    _IOR (MYMODULE_IOC_MAGIC, 6, int)
// Q -- query: reply with return value
#define MYMODULE_IOCQQUANTUM _IO  (MYMODULE_IOC_MAGIC, 7)
#define MYMODULE_IOCQQSET    _IO  (MYMODULE_IOC_MAGIC, 8)
// X -- exchange: switch G and S atomically
#define MYMODULE_IOCXQUANTUM _IOWR(MYMODULE_IOC_MAGIC, 9, int)
#define MYMODULE_IOCXQSET    _IOWR(MYMODULE_IOC_MAGIC, 10, int)
// H -- shift: switch T and Q atomically
#define MYMODULE_IOCHQUANTUM _IO  (MYMODULE_IOC_MAGIC, 11)
#define MYMODULE_IOCHQSET    _IO  (MYMODULE_IOC_MAGIC, 12)

#define MYMODULE_IOC_MAXNR 14

void mymodule_setup_cdev(struct mymodule_dev* m_dev, int index);
int mymodule_trim(struct mymodule_dev* dev);

#endif
