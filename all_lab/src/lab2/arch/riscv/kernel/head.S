.extern start_kernel

    .section .text.init
    .globl _start
_start:
    #将栈顶指针放入sp
    la sp, boot_stack_top 
    jal task_init
    jal test_init
    # ------------------

        # set stvec = _traps
        la t0, _traps
        csrw stvec, t0

    # ------------------

        # set sie[STIE] = 1
        addi t0, x0, 1
        slli t0, t0, 5
        csrr t1, sie
        or t2, t0, t1
        csrw sie, t2

    # ------------------

        # set first time interrupt
        li t0, 10000000
        and a0, x0, x0
        and a1, x0, x0
        rdtime a2
        add a2, a2, t0
        and a3, x0, x0
        and a4, x0, x0
        and a5, x0, x0
        and a6, x0, x0
        and a7, x0, x0
        jal sbi_ecall

    # ------------------

        # set sstatus[SIE] = 1
        addi t0, x0, 1
        slli t0, t0, 1
        csrr t1, sstatus
        or t2, t0, t1
        csrw sstatus, t2

    # ------------------
    
    jal start_kernel      #跳转到开始
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096

    .globl boot_stack_top
boot_stack_top: