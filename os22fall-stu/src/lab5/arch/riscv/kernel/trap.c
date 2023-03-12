// trap.c 
#include "printk.h"
#include "clock.h"
#include "proc.h"
#include "syscall.h"

struct pt_regs {
    uint64 x[32];
    uint64 sepc;
    uint64 sstatus;
};

void trap_handler(uint64 scause, uint64 sepc, struct pt_regs *regs) {
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.5 节
    // 其他interrupt / exception 可以直接忽略

    unsigned long interrupt = (scause >> 63);
    unsigned long except_code = ((scause << 1) >> 1);

    if(interrupt) {
        printk("DEBUG: Receive interrupt\n");
        if(except_code == 5) {
            // supervisor timer interrupt
            printk("[S]: Supervisor timer interrupt\n");
            clock_set_next_event();
            do_timer();
        }
    } else {
        if(except_code == 8) {
            uint64 a0 = regs->x[10];
            uint64 a1 = regs->x[11];
            uint64 a2 = regs->x[12];
            uint64 a7 = regs->x[17];
            // printk("[U]: Environment call from U-mode\n");
            if(a7 == SYS_WRITE) {
                sys_write(a0, (const char*)a1, a2);
            } else if(a7 == SYS_GETPID) {
                regs->x[10] = sys_getpid();
            }
           regs->sepc =(unsigned long)(((unsigned long)regs->sepc) + (unsigned long)0x4);
        }
    }

}