.extern start_kernel
.extern _traps
    .section .text.init
    .globl _start
_start:
    la sp, boot_stack_top

    # set stvec = _traps
    la t0, _traps
    csrw stvec, t0
    # ------------------

    # set sie[STIE] = 1
    # STIE=5
    li t0, 0x20
    csrs sie, t0
    # ------------------

    # set first time interrupt
    rdtime t0
    li t1, 10000000
    add t1, t1, t0
    li a7, 0
    li a6, 0
    mv a0, t1
    li a1, 0
    li a2, 0
    li a3, 0
    li a4, 0
    li a5, 0
    ecall
    # ------------------

    # set sstatus[SIE] = 1
    # SIE = 2
    li t0, 0x2
    csrs sstatus, t0
    # ------------------

    # ------------------
    j start_kernel
    # ------------------

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top: