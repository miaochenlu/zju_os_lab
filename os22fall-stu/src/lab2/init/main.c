#include "printk.h"
#include "sbi.h"

extern void test();

int start_kernel() {
    printk("hello world\n");

    test();

	return 0;
}
