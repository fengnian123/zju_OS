    .section .text.entry
    .align 2
    .globl _traps 
_traps:
    # YOUR CODE HERE
    # -----------

        # 1. save 32 registers and sepc to stack
        addi sp, sp, -256 #留256的空间保存
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
        csrr a0, scause
        sd x10, 80(sp) #寄存器a0
        csrr a1, sepc 
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


    # -----------

        # 2. call trap_handler
        jal trap_handler

    # -----------

        # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
        #取sepc
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
        addi sp, sp, 256
    # -----------

        # 4. return from trap
        sret

    # -----------
    .global __dummy
__dummy:
    # YOUR CODE HERE
    la t0, dummy 
    csrw sepc, t0 #在__dummy 中将 sepc 设置为 dummy() 的地址
    sret #从中断中返回

     .globl __switch_to
__switch_to:
    # save state to prev process
    # YOUR CODE HERE
    addi t0, a0, 40
    sd ra, 8(t0)
    sd sp, 16(t0)
    sd s0, 24(t0)
    sd s1, 32(t0)
    sd s2, 40(t0)
    sd s3, 48(t0)
    sd s4, 56(t0)
    sd s5, 64(t0)
    sd s6, 72(t0)
    sd s7, 80(t0)
    sd s8, 88(t0)
    sd s9, 96(t0)
    sd s10, 104(t0)
    sd s11, 112(t0)
    # restore state from next process
    # YOUR CODE HERE
    addi t0, a1, 40
    ld ra, 8(t0)
    ld sp, 16(t0)
    ld s0, 24(t0)
    ld s1, 32(t0)
    ld s2, 40(t0)
    ld s3, 48(t0)
    ld s4, 56(t0)
    ld s5, 64(t0)
    ld s6, 72(t0)
    ld s7, 80(t0)
    ld s8, 88(t0)
    ld s9, 96(t0)
    ld s10, 104(t0)
    ld s11, 112(t0)

    ret
