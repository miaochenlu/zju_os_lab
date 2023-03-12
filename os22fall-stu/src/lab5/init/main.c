#include "printk.h"
#include "sbi.h"

extern void test();

int start_kernel() {
    schedule();

    test();

	return 0;
}
