//arch/riscv/kernel/proc.c
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "rand.h"
#include "printk.h"

extern char uapp_start[];
extern char uapp_end[];
extern pte_t swapper_pg_dir[512];
extern void __dummy();
extern void __switch_to(struct task_struct* prev, struct task_struct* next);
extern void create_mapping(pagetable_t pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

void task_init() {
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle

    /* YOUR CODE HERE */
    idle = (struct task_struct*)kalloc();
    idle->state = TASK_RUNNING; 
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;

    current = idle;
    task[0] = idle;

    for(int i = 1; i < NR_TASKS; i++) {
        task[i] = (struct task_struct*)kalloc();
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = rand();
        task[i]->pid = i;
        task[i]->thread.ra = (uint64)__dummy;
        task[i]->thread.sp = (uint64)(task[i]) + PGSIZE;
        // task[i]->thread.sp = (uint64)(task[i] + PGSIZE); 好离谱的错误呀，我真的没看出来 
        
        uint64* user_stack = (uint64*)alloc_page();

        pte_t* user_pgtable = (pte_t*)alloc_page();
        printk("userpage: %d\n", user_pgtable);
        for(int i = 0; i < 512; i++) {
            user_pgtable[i] = swapper_pg_dir[i];
        }
        printk("userpage: %d\n", user_pgtable);
        create_mapping(user_pgtable, USER_START, (uint64)uapp_start - PA2VA_OFFSET, 
                        uapp_end - uapp_start, PTE_V | PTE_R | PTE_W | PTE_X | PTE_U);
        create_mapping(user_pgtable, USER_END-PGSIZE, (uint64)user_stack - PA2VA_OFFSET, 
                        PGSIZE, PTE_V | PTE_R | PTE_W | PTE_U);
        
        uint64 sstatus = csr_read(sstatus);
        task[i]->thread.sstatus     = sstatus | 0x40020; // 40020
        csr_write(sstatus, task[i]->thread.sstatus);
        task[i]->thread.sepc        = USER_START;
        task[i]->thread.sscratch    = USER_END;
        // task[i]->pgd                = (uint64)user_pgtable - PA2VA_OFFSET;
        // uint64 satp = csr_read(satp);
        // printk("original satp: %lx\n", satp);
        // uint64 PPN = (uint64)((uint64)user_pgtable - PA2VA_OFFSET) >> 12;
        // satp = (satp >> 44) << 44;
        // satp |= PPN;
        task[i]->pgd = (uint64)user_pgtable - PA2VA_OFFSET;
        // printk("i: %d PPN: %lx satp: %lx\n===\n", i, PPN, satp);

    }

    printk("...proc_init done!\n");
}

void dummy() {
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. thread space begin at 0x%lx\n", current->pid, current);
        }
    }
}

void switch_to(struct task_struct* next) {
    if(next && current->pid != next->pid) {
        struct task_struct* cur_tmp = current;
        current = next;
        __switch_to(cur_tmp, next);
    }
}

void do_timer(void) {
    // 1. 如果当前线程是 idle 线程 直接进行调度
    // 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回 否则进行调度

    /* YOUR CODE HERE */
    if(current->pid == idle->pid) {
        schedule();
    } else {
        current->counter -= 1;
        if(current->counter > 0) return;
        else schedule();
    }
}

#ifdef SJF
void schedule(void) {
    int sjf = 0xfff;
    int next_index = -1;
    for(int i = 1; i < NR_TASKS; i++) {
        if(task[i]->counter < sjf && task[i]->counter > 0) {
            sjf = task[i]->counter;
            next_index = i;
        }
    }

    if(next_index == -1) {
        for(int i = 1; i < NR_TASKS; i++) {
            task[i]->counter = rand();
            printk("SET [PID = %d COUNTER = %d]\n", i, task[i]->counter);
        }
        schedule();
    } else{
        printk("switch to [PID = %d COUNTER = %d ]\n", next_index, task[next_index]->counter);
        switch_to(task[next_index]);
    }
}
#endif

#ifdef PRIORITY
void schedule(void) {
    printk("start to schedule\n");
    int max_priority = 0;
    int next_index = -1;

    for(int i = 1; i < NR_TASKS; i++) {
            if(task[i]->counter > 0 && task[i]->priority > max_priority) {
            max_priority = task[i]->priority;
            next_index = i;
        }
    }

    if(next_index == -1) {
        for(int i = 1; i < NR_TASKS; i++) {
            task[i]->counter = rand();
            printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n", 
                i, task[i]->priority, task[i]->counter);
        }
        schedule();
    } else {
        printk("switch to [PID = %d PRIORITY = %d COUNTER = %d ]\n", 
            next_index, task[next_index]->priority, task[next_index]->counter);
        switch_to(task[next_index]);
    }
}
#endif