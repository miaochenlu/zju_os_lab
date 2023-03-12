#include "syscall.h"
#include "defs.h"
#include "printk.h"
#include "proc.h"

extern struct task_struct* current;

void sys_write(unsigned int fd, const char* buf, uint64 count) {
    printk("%s", buf);
}
uint64 sys_getpid() {
    return current->pid;
}