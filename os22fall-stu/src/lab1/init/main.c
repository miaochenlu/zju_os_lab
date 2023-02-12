#include "print.h"
#include "sbi.h"

extern void test();

int start_kernel() {
    // puts(" Hello RISC-V\n");
    puti(0);
    puts("\n");
    puti(1);
    puts("\n");
    // puti(2022);
    // puts("\n");
    puti(1000);
    puts("\n");
    puti(-9899);
    puts("\n");
    puti(1234);
    puts("\n");
    test(); // DO NOT DELETE !!!

	return 0;
}
