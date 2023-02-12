#include "print.h"
#include "sbi.h"

void puts(char *s) {
    while(*s) {
        sbi_ecall(0x01, 0, *s, 0, 0, 0, 0, 0);
        s++;
    }
}

void puti(int x) {
    if(x < 0) {
        sbi_ecall(0x01, 0, '-', 0, 0, 0, 0, 0);
        x = -x;
    }

    int bit = 1;
    while(bit * 10 <= x) {
        bit = bit * 10;
    }

    while(bit > 0) {
        int val = x / bit;
        char cur_char = val + '0';
        sbi_ecall(0x01, 0, cur_char, 0, 0, 0, 0, 0);
        x = x % bit;
        bit = bit / 10;
    }

    
}
