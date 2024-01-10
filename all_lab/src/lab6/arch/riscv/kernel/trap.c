#include"clock.h"
#include"printk.h"
#include "syscall.h"
#include "mm.h"
#include "defs.h"
extern struct task_struct *current;
extern char _sramdisk[];
#define SYS_WRITE 64
#define SYS_PID 172
#define SYS_CLONE 220

void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs *regs) {
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    if(scause & 0x8000000000000000){//trap 类型为interrupt
        if(scause == 0x8000000000000005){//timer interrupt
            //printk("[S] Supervisor Mode Timer Interrupt\n");
            clock_set_next_event();
            do_timer();
        }
    }
    else if(scause == 8){
        if(regs->reg[17] == SYS_WRITE){
            char *buffer = (char *)(regs->reg[11]);
            int cnt = sys_write(1, buffer, regs->reg[12]);
            regs->sepc += 4;
        }
        else if(regs->reg[17] == SYS_PID){
            uint64 pid = sys_getpid();
            regs->sepc += 4;
            regs->reg[10] = pid;
        }
        else if(regs->reg[17] == SYS_CLONE){
            regs->reg[10] = sys_clone(regs);
            regs->sepc += 4;
        }
    }
    else if(scause == 15 || scause == 12 || scause == 13){
        printk("[S] trap, ");
        printk("scause: %lx, ", scause);
        printk("stval: %lx, ", regs->stval);
        printk("sepc: %lx\n", regs->sepc);
        if(regs->sepc > 0xf00000000){
            while (1);
        }
        do_page_fault(regs);
        return;
    }
    else{
        printk("[S] unhandled trap, ");
        printk("scause: %lx, ", scause);
        printk("stval: %lx, ", regs->stval);
        printk("sepc: %lx\n", regs->sepc);
        while(1);
    }
}

void do_page_fault(struct pt_regs *regs) {
    //  for(int j = 0 ; j < current->vma_cnt ; j++){
    //         printk("current->vmas[%d]->start_va=%lx  current->vmas[%d]->vm_end=%lx  current->vmas[%d]->flags=%d\n",
    //         j, current->vmas[j].vm_start, 
    //         j, current->vmas[j].vm_end,
    //         j, current->vmas[j].vm_flags);
    //     }
    //通过 stval 获得访问出错的虚拟内存地址（Bad Address）
    uint64 stval = csr_read(stval);
    //通过 find_vma() 查找 Bad Address 是否在某个 vma 中
    struct vm_area_struct * vma = find_vma(current, stval);
    if(vma->vm_flags & VM_ANONYM){
        uint64* new_page = (uint64*)alloc_page();
        uint64 va = (stval / PGSIZE) * PGSIZE;
        uint64 pa = (uint64)new_page - PA2VA_OFFSET;
        uint64 sz = PGSIZE;
        uint64 perm;
        memset(new_page, 0, PGSIZE);
        create_mapping(current->pgd, va, pa, sz, 23);
        
    }
    else{
        //拷贝 uapp 中的内容
        uint64* new_space = (uint64 *)alloc_page();
        // do mapping
        uint64 va = (stval / PGSIZE) * PGSIZE;
        uint64 pa = (uint64)new_space - PA2VA_OFFSET;
        create_mapping(current->pgd, va, pa, PGSIZE,31);
        uint64* src = (uint64 *)((_sramdisk) + (stval - (vma->vm_start)) / PGSIZE * PGSIZE);
        memcopy(new_space , src, PGSIZE);
        uint64 offset[3];
        offset[2] = (va >> 30) & 0x1FF;
        offset[1] = (va >> 21) & 0x1FF;
        offset[0] = (va >> 12) & 0x1FF;
        uint64 *pgt_next = (uint64 *)(((current->pgd[offset[2]] & 0xfffffffffffffc00) << 2) + PA2VA_OFFSET);
    }
}