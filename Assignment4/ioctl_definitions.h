#include <linux/ioctl.h>
//extern struct tuple;
#define MAGIC_NO 'k'
#define IOCTL_CMD_WRITE _IOW(MAGIC_NO, 1, int*)
#define IOCTL_CMD_READ _IOR(MAGIC_NO, 2, int*)
#define IOCTL_CMD_BOOK_READ _IOR(MAGIC_NO, 3, struct param*)