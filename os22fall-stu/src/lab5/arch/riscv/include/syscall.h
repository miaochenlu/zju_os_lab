#include "types.h"

#define SYS_WRITE   64
#define SYS_GETPID  172

void sys_write(unsigned int fd, const char* buf, uint64 count);
uint64 sys_getpid();
