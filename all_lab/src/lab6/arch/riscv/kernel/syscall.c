//
// Created by Administrator on 2023/12/8.
//
#include "syscall.h"
extern struct task_struct *current;
extern char _sramdisk[];
extern struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此
extern void __ret_from_fork();
extern unsigned long swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
int sys_write(unsigned int fd, const char *buf, size_t count){
    int write_size;
    for(int i = 0;i < count ;i++){
        printk("%c", buf[i]);
        write_size++;
    }
    return write_size;
}

unsigned long sys_getpid(){
    return (unsigned long)(current->pid);
}

unsigned long sys_clone(struct pt_regs *regs) {
    //参考 task_init 创建一个新的 task，将的 parent task 的整个页复制到新创建的task_struct 页上
    struct task_struct *child_task = (struct task_struct *)alloc_page();
    memcopy(child_task, current, PGSIZE);
    //将 thread.ra 设置为__ret_from_fork，并正确设置 thread.sp
    child_task->thread.ra = __ret_from_fork;
    child_task->thread.sp = (uint64)child_task + PGSIZE - ((uint64)(current) + PGSIZE - (uint64)(regs));
//    printk("sp = %lx\n",child_task->thread.sp);
    uint64 child_pid;
    for(uint64 i = 0 ; i < NR_TASKS ; i++){
        if(!task[i]){
            child_pid = i;
            task[i] = child_task;
            break;
        }
    }
    child_task->pid = child_pid;
//    printk("child_task->pid = %lx\n", child_task->pid);
    //利用参数 regs 来计算出 child task 的对应的 pt_regs 的地址，并将其中的 a0, sp, sepc 设置成正确的值
    uint64 *addr_a0 = child_task->thread.sp + 80;
    *(addr_a0) = 0;
    uint64 *addr_sp = child_task->thread.sp + 16;
//    printk("p_sp=%lx  c_sp=%lx\n", regs->reg[2], *(addr_sp));
    *(addr_sp) = child_task->thread.sp;
//    printk("p_sp=%lx  c_sp=%lx\n", regs->reg[2], *(addr_sp));
    uint64 *addr_sepc = child_task->thread.sp + 32*8;
    *(addr_sepc) = regs->sepc + 4;


    //为 child task 申请 user stack，并将 parent task 的 user stack数据复制到其中
    child_task->user_sp = (uint64 *)alloc_page();
    memcopy(child_task->user_sp, current->user_sp, PGSIZE);
    //为 child task 分配一个根页表，并仿照 setup_vm_final 来创建内核空间的映射
    uint64 *new_pgt = (uint64 *)alloc_page();
    memcopy(new_pgt, swapper_pg_dir, PGSIZE);
    child_task->pgd = new_pgt;
    child_task->satp = (csr_read(satp) & 0xfffff00000000000) | (((uint64)new_pgt - PA2VA_OFFSET) >> 12);
//    printk("task->thread.sscratch = %lx\n", child_task->thread.sepc);
    //根据 parent task 的页表和 vma 来分配并拷贝 child task 在用户态会用到的内存
    for(uint64 i = 0 ; i < current->vma_cnt ; i++){
        uint64 offset[3];
        uint64 page[3];
        uint64 va = current->vmas[i].vm_start;
        uint64 va_end = current->vmas[i].vm_end;
//        printk("va = %lx\n", va);
        while(va < va_end){
            offset[2] = (va >> 30) & 0x1FF;
            offset[1] = (va >> 21) & 0x1FF;
            offset[0] = (va >> 12) & 0x1FF;
            if (!(current->pgd[offset[2]])){
                va += PGSIZE;
                continue;
            }
            else{
                uint64 *pgt_next = (uint64 *)(((current->pgd[offset[2]] & 0xfffffffffffffc00) << 2) + PA2VA_OFFSET);
//                printk("pgt_next = %lx\n",pgt_next);
                if(!(pgt_next[offset[1]])){
                    va += PGSIZE;
                    continue;
                }
                else{
                    uint64 *pgt_nnext = (uint64 *)(((pgt_next[offset[1]] & 0xfffffffffffffc00) << 2) + PA2VA_OFFSET);
//                    printk("pgt_nnext = %lx\n",pgt_nnext[offset[0]]);
                    if(!(pgt_nnext[offset[0]])){
                        va += PGSIZE;
                        continue;
                    }
                    else if(pgt_nnext[offset[0]] & 1){
//                        printk("pgt_nnext[offset[0]] = %lx\n", pgt_nnext[offset[0]]);
                        uint64* new_space = (uint64 *)alloc_page();
                        // do mapping
                        uint64 pa = (uint64)new_space - PA2VA_OFFSET;
                        create_mapping(child_task->pgd, va, pa, PGSIZE,31);
                        uint64* src = (uint64 *)(va);
                        memcopy(new_space , src, PGSIZE);
//                        printk("in\n");
                    }
                }
            }
            va += PGSIZE;
//            printk("va = %lx\n", va);
        }
    }
    //返回子 task 的 pid
    return child_task->pid;

    /*
     1. 参考 task_init 创建一个新的 task，将的 parent task 的整个页复制到新创建的
        task_struct 页上(这一步复制了哪些东西?）。将 thread.ra 设置为
        __ret_from_fork，并正确设置 thread.sp
        (仔细想想，这个应该设置成什么值?可以根据 child task 的返回路径来倒推)

     2. 利用参数 regs 来计算出 child task 的对应的 pt_regs 的地址，
        并将其中的 a0, sp, sepc 设置成正确的值(为什么还要设置 sp?)

     3. 为 child task 申请 user stack，并将 parent task 的 user stack
        数据复制到其中。 (既然 user stack 也在 vma 中，这一步也可以直接在 5 中做，无需特殊处理)

     3.1. 同时将子 task 的 user stack 的地址保存在 thread_info->
        user_sp 中，如果你已经去掉了 thread_info，那么无需执行这一步

     4. 为 child task 分配一个根页表，并仿照 setup_vm_final 来创建内核空间的映射

     5. 根据 parent task 的页表和 vma 来分配并拷贝 child task 在用户态会用到的内存

     6. 返回子 task 的 pid
    */
}

