    .section .text.entry
    .align 2
    .globl _traps
_traps:
    # YOUR CODE HERE
    # -----------
        #在 _trap 的首尾我们都需要做类似的操作(判断是否在内核态)
        csrr t0, sscratch
        addi t1, x0, 0
        beq t0, t1, Next
        #交换对应的寄存器的值
        csrr t1, sscratch
        mv t2, sp
        mv sp, t1
        csrw sscratch, t2
        # 1. save 32 registers and sepc to stack
    Next:
        addi sp, sp, -296
        #存入32个寄存器
        sd x0, 0(sp)
        sd x1, 8(sp)
        sd x2, 16(sp)
        sd x3, 24(sp)
        sd x4, 32(sp)
        sd x5, 40(sp)
        sd x6, 48(sp)
        sd x7, 56(sp)
        sd x8, 64(sp)
        sd x9, 72(sp)
        sd x10, 80(sp) #寄存器a0
        sd x11, 88(sp) #寄存器a1
        sd x12, 96(sp)
        sd x13, 104(sp)
        sd x14, 112(sp)
        sd x15, 120(sp)
        sd x16, 128(sp)
        sd x17, 136(sp)
        sd x18, 144(sp)
        sd x19, 152(sp)
        sd x20, 160(sp)
        sd x21, 168(sp)
        sd x22, 176(sp)
        sd x23, 184(sp)
        sd x24, 192(sp)
        sd x25, 200(sp)
        sd x26, 208(sp)
        sd x27, 216(sp)
        sd x28, 224(sp)
        sd x29, 232(sp)
        sd x30, 240(sp)
        sd x31, 248(sp)
        #存sepc
        csrr t0, sepc
        sd t0, 256(sp)
        csrr t0, sstatus
        sd t0, 264(sp)
        csrr t0, sscratch
        sd t0, 272(sp)
        csrr t0, stval
        sd t0, 280(sp)
        csrr t0, scause
        sd t0, 288(sp)



    # -----------

        # 2. call trap_handler
        csrr a0, scause
        csrr a1, sepc
        mv a2, sp #寄存器a2
        jal trap_handler

    # -----------
        .global __ret_from_fork
    __ret_from_fork:
        ld t0, 288(sp)
        csrw scause, t0
        ld t0, 280(sp)
        csrw stval, t0
        ld t0, 272(sp)
        csrw sscratch, t0
        ld t0, 264(sp)
        csrw sstatus, t0
        ld t0, 256(sp)
        csrw sepc, t0
        ld x0, 0(sp)
        ld x1, 8(sp)
        ld x2, 16(sp)
        ld x3, 24(sp)
        ld x4, 32(sp)
        ld x5, 40(sp)
        ld x6, 48(sp)
        ld x7, 56(sp)
        ld x8, 64(sp)
        ld x9, 72(sp)
        ld x10, 80(sp) #寄存器a0
        ld x11, 88(sp) #寄存器a1
        ld x12, 96(sp)
        ld x13, 104(sp)
        ld x14, 112(sp)
        ld x15, 120(sp)
        ld x16, 128(sp)
        ld x17, 136(sp)
        ld x18, 144(sp)
        ld x19, 152(sp)
        ld x20, 160(sp)
        ld x21, 168(sp)
        ld x22, 176(sp)
        ld x23, 184(sp)
        ld x24, 192(sp)
        ld x25, 200(sp)
        ld x26, 208(sp)
        ld x27, 216(sp)
        ld x28, 224(sp)
        ld x29, 232(sp)
        ld x30, 240(sp)
        ld x31, 248(sp)

        addi sp, sp, 296
    # -----------
        #在 _trap 的首尾我们都需要做类似的操作(判断是否在内核态)
        csrr t0, sscratch
        addi t1, x0, 0
        beq t0, t1, Return
        #交换对应的寄存器的值
        csrr t1, sscratch
        mv t2, sp
        mv sp, t1
        csrw sscratch, t2
    Return:
        # 4. return from trap
        sret

    # -----------
    .global __dummy
__dummy:
    # YOUR CODE HERE
    #交换对应的寄存器的值
    csrr t1, sscratch
    mv t2, sp
    mv sp, t1
    csrw sscratch, t2
    sret #从中断中返回

     .globl __switch_to
__switch_to:
    #加入保存/恢复 sepc sstatus sscratch 以及 切换页表的逻辑
    csrr t1, sepc
    csrr t2, sstatus
    csrr t3, sscratch
    csrr t4, satp
    # save state to prev process
    # YOUR CODE HERE
    addi t0, a0, 40
    sd ra, 0(t0)
    sd sp, 8(t0)
    sd s0, 16(t0)
    sd s1, 24(t0)
    sd s2, 32(t0)
    sd s3, 40(t0)
    sd s4, 48(t0)
    sd s5, 56(t0)
    sd s6, 64(t0)
    sd s7, 72(t0)
    sd s8, 80(t0)
    sd s9, 88(t0)
    sd s10, 96(t0)
    sd s11, 104(t0)
    sd t1, 112(t0)
    sd t2, 120(t0)
    sd t3, 128(t0)
    sd t4, 136(t0)
    # restore state from next process
    # YOUR CODE HERE
    # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
    addi t0, a1, 40
    ld ra, 0(t0)
    ld sp, 8(t0)
    ld s0, 16(t0)
    ld s1, 24(t0)
    ld s2, 32(t0)
    ld s3, 40(t0)
    ld s4, 48(t0)
    ld s5, 56(t0)
    ld s6, 64(t0)
    ld s7, 72(t0)
    ld s8, 80(t0)
    ld s9, 88(t0)
    ld s10, 96(t0)
    ld s11, 104(t0)
    ld t1, 112(t0)
    ld t2, 120(t0)
    ld t3, 128(t0)
    ld t4, 136(t0)
    csrw sepc, t1
    csrw sstatus, t2
    csrw sscratch, t3
    csrw satp, t4
    # flush tlb
    sfence.vma zero, zero
    # flush icache
    fence.i
    ret

