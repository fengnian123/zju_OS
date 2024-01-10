//arch/riscv/kernel/proc.c
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "rand.h"
#include "printk.h"
#include "test.h"
#include "string.h"
#include "elf.h"
//arch/riscv/kernel/proc.c

extern void __dummy();

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

/**
 * new content for unit test of 2023 OS lab2
*/
extern uint64 task_test_priority[]; // test_init 后, 用于初始化 task[i].priority 的数组
extern uint64 task_test_counter[];  // test_init 后, 用于初始化 task[i].counter  的数组
extern void __switch_to(struct task_struct* prev, struct task_struct* next);
extern void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);
void switch_to(struct task_struct* next) {
    /* YOUR CODE HERE */
    //判断下一个执行的线程 next 与当前的线程 current 是否为同一个线程，如果是同一个线程，则无需做任何处理，否则调用 __switch_to 进行线程切换。
    if(current->pid != next->pid){
        struct task_struct*tmp = current;
        current = next ;
        //printk("current->pgd = %lx\n", current->pgd);
        __switch_to(tmp, next);
    }
}
extern char _sramdisk[];//心得体会
extern char _eramdisk[];
extern char _sbss[];
extern unsigned long swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

static uint64_t load_program(struct task_struct* task) {
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)_sramdisk;

    uint64_t phdr_start = (uint64_t)ehdr + ehdr->e_phoff;
    int phdr_cnt = ehdr->e_phnum;
    
    uint64* new_pgtbl = (uint64*)alloc_page();
    //将内核页表 （ swapper_pg_dir ） 复制到每个进程的页表中
    memcopy(new_pgtbl, swapper_pg_dir, PGSIZE);
    task->pgd = new_pgtbl;
    Elf64_Phdr* phdr;
    int load_phdr_cnt = 0;
    for (int i = 0; i < phdr_cnt; i++) {
        phdr = (Elf64_Phdr*)(phdr_start + sizeof(Elf64_Phdr) * i);
        if (phdr->p_type == PT_LOAD) {
            // alloc space and copy content
            int page_cnt;
            if(phdr->p_memsz % PGSIZE == 0){
                page_cnt = phdr->p_memsz / PGSIZE;
            }
            else{
                page_cnt = phdr->p_memsz / PGSIZE + 1;
            }
            uint64* new_space = (uint64 *)alloc_pages(page_cnt);
            uint64* src = (uint64 *)(_sramdisk);
            memcopy(new_space , src, phdr->p_memsz);
            // do mapping
            uint64 va = phdr->p_vaddr;
            uint64 pa = (uint64)new_space - PA2VA_OFFSET;
            //printk("p_offset = %lx\n",phdr->p_offset);
            // printk("phdr->p_memsz = %|x\n",phdr->p_memsz);
            // printk("phdr->p_filesz = %|x\n",phdr->p_filesz);
            do_mmap(task, phdr->p_vaddr, phdr->p_memsz,14 , phdr->p_offset, phdr->p_filesz);
        }
    }

    // allocate user stack and do mapping
    // code...
    // following code has been written for you
    // set user stack
    // pc for the user program
    //设置用户态栈,通过 alloc_page 接口申请一个空的页面来作为用户态栈，并映射到进程的页表中
    task->user_sp = alloc_page();
    uint64 va = USER_END - PGSIZE;
    uint64 pa = task->user_sp - PA2VA_OFFSET;
    uint64 sz = PGSIZE;
    uint64 perm = 0b10111;
    do_mmap(task, USER_END - PGSIZE, PGSIZE, 7 , 0, 0);
    task->satp = (csr_read(satp) & 0xfffff00000000000) | (((uint64)new_pgtbl - PA2VA_OFFSET) >> 12);
    //将 sepc 设置为 ehdr->e_entry
    task->thread.sepc = ehdr->e_entry;
    // printk("sepc= %|x",ehdr->e_entry);
    //配置 sstatus 中的 SPP（使得 sret 返回至 U-Mode）， SPIE （sret 之后开启中断）， SUM（S-Mode 可以访问 User 页面）
    uint64 sstatus = csr_read(sstatus);
    sstatus |= 0x0000000000040020;
    sstatus &= 0xfffffffffffffeff;
    task->thread.sstatus = sstatus;
    //将 sscratch 设置为 U-Mode 的 sp，其值为 USER_END （即，用户态栈被放置在 user space 的最后一个页面）
    task->thread.sscratch = USER_END;
}

void task_init() {
    test_init(NR_TASKS);
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    idle = kalloc();
    // 2. 设置 state 为 TASK_RUNNING;
    idle->state = TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    idle->counter = 0;
    idle->priority = 0;
    // 4. 设置 idle 的 pid 为 0
    idle->pid = 0;
    // 5. 将 current 和 task[0] 指向 idle
    current = idle;
    task[0] = idle;
    /* YOUR CODE HERE */

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, 此外，为了单元测试的需要，counter 和 priority 进行如下赋值：
    //      task[i].counter  = task_test_counter[i];
    //      task[i].priority = task_test_priority[i];
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址,  `sp` 设置为 该线程申请的物理页的高地址
    for(int i = 1 ; i < NR_TASKS ; i++){
        task[i]  = kalloc();
        task[i]->pid = i;
        task[i]->counter = task_test_counter[i];
        task[i]->priority = 3;
        task[i]->state = TASK_RUNNING;
        uint64 ra_address = &__dummy;
        uint64 sp_address = (uint64)task[i] + PGSIZE;
        task[i]->thread.ra = ra_address;
        task[i]->thread.sp = sp_address;
        task[i]->kernel_sp = sp_address;
        task[i]->vma_cnt = 0;
        load_program(task[i]);
        for(int j = 0 ; j < task[i]->vma_cnt ; j++){
            printk("task[%d]->vmas[%d]->start_va=%lx  task[%d]->vmas[%d]->vm_end=%lx  task[%d]->vmas[%d]->flags=%d\n",
            i, j, task[i]->vmas[j].vm_start, 
            i, j, task[i]->vmas[j].vm_end,
            i, j, task[i]->vmas[j].vm_flags);
        }
//        //对于每个进程，创建属于它自己的页表
//        uint64* new_pgtbl = (uint64*)alloc_page();
//        //将内核页表 （ swapper_pg_dir ） 复制到每个进程的页表中
//        for(unsigned long i = 0;i < PGSIZE;i++){
//            new_pgtbl[i] = swapper_pg_dir[i];
//        }
//        //将 uapp 所在的页面映射到每个进行的页表中
//        //二进制文件需要先被 拷贝 到一块某个进程专用的内存之后再进行映射，防止所有的进程共享数据
//        uint64* new_addrress = (uint64*)alloc_page();
//        for(unsigned long i = 0;i < PGSIZE;i++){
//            new_addrress[i]  = _sramdisk[i];
//        }
//        uint64 va = USER_START;
//        uint64 pa = (uint64)new_addrress - PA2VA_OFFSET;
//        uint64 sz = PGSIZE;
//        int perm = 0b11111;
//        printk("task[i]->uapp %|x %|x %|x %d\n",va,pa,sz,perm);
//        create_mapping(new_pgtbl, va, pa, sz, perm);
//
//        //设置用户态栈,通过 alloc_page 接口申请一个空的页面来作为用户态栈，并映射到进程的页表中
//        task[i]->user_sp = alloc_page();
//        va = USER_END - PGSIZE;
//        pa = task[i]->user_sp - PA2VA_OFFSET;
//        sz = PGSIZE;
//        perm = 0b10111;
//        printk("task[i]->user %|x %|x %|x %d\n",va,pa,sz,perm);
//        create_mapping(new_pgtbl, va, pa, sz, perm);
//        task[i]->satp = (csr_read(satp) & 0xfffff00000000000) | (((uint64)new_pgtbl - PA2VA_OFFSET) >> 12);
//        //将 sepc 设置为 USER_START
//        task[i]->thread.sepc = USER_START;
//        //配置 sstatus 中的 SPP（使得 sret 返回至 U-Mode）， SPIE （sret 之后开启中断）， SUM（S-Mode 可以访问 User 页面）
//        uint64 sstatus = csr_read(sstatus);
//        sstatus |= 0x0000000000040020;
//        sstatus &= 0xfffffffffffffeff;
//        task[i]->thread.sstatus = sstatus;
//        //将 sscratch 设置为 U-Mode 的 sp，其值为 USER_END （即，用户态栈被放置在 user space 的最后一个页面）
//        task[i]->thread.sscratch = USER_END;

    }
    /* YOUR CODE HERE */

    printk("...proc_init done!\n");
}

void do_mmap(struct task_struct *task, uint64 addr, uint64 length, uint64 flags,
             uint64 vm_content_offset_in_file, uint64 vm_content_size_in_file){
    uint64 vm_start = addr/PGSIZE *PGSIZE;
    //printk("vm_start= %|x\n",vm_start);
    uint64 vm_end;
    if((addr + length) % PGSIZE != 0){
        vm_end = ((addr + length) / PGSIZE + 1) * PGSIZE;
    }
    else{
        vm_end  = ((addr + length) / PGSIZE) * PGSIZE;
    }
    int judge = 0;
    for(uint64 i = 0 ; i < vm_end - vm_start ; i += PGSIZE){
        if(find_vma(task, vm_start + i)){
            judge = 1;
            break;
        }
    }
        struct vm_area_struct *vma_new = (struct vm_area_struct *)alloc_page();
        vma_new->vm_start = vm_start;
        vma_new->vm_end = vm_end;
        vma_new->vm_flags = flags;
        vma_new->vm_content_offset_in_file = vm_content_offset_in_file;
        vma_new->vm_content_size_in_file = vm_content_size_in_file;
        task->vmas[task->vma_cnt] = *vma_new;
        task->vma_cnt++;
        // printk("task->vmas->start_va= %|x task->vmas->vm_end= %|x task->vmas->vma_cnt= %d\n", task->vmas[task->vma_cnt-1].vm_start, 
        // task->vmas[task->vma_cnt-1].vm_end, task->vma_cnt-1);
        return;
    // else{
        // uint64 page_cnt;
        // if(length % PGSIZE == 0){
        //     page_cnt = length / PGSIZE;
        // }
        // else{
        //     page_cnt = length / PGSIZE + 1;
        // }
        // for(vm_start = USER_START ; vm_start < USER_END ; vm_start += PGSIZE){
        //     uint64 cnt = 0;
        //     uint64 temp_vm_start = vm_start;
        //     for (; cnt < page_cnt; cnt++) {
        //         if (find_vma(task, vm_start + cnt * PGSIZE)) {
        //             temp_vm_start = temp_vm_start + cnt * PGSIZE;
        //             break;
        //         }
        //     }
        //     if (cnt == page_cnt) {
        //         vm_start = temp_vm_start;
        //         break;
        //     }
        // }
        // if((vm_start + length) % PGSIZE != 0){
        //     vm_end = ((vm_start + length) / PGSIZE + 1) * PGSIZE;
        // }
        // else{
        //     vm_end  = ((vm_start + length) / PGSIZE) * PGSIZE;
        // }
        // struct vm_area_struct *vma_new = (struct vm_area_struct *)alloc_page();
        // vma_new->vm_start = vm_start;
        // vma_new->vm_end = vm_end;
        // vma_new->vm_flags = flags;
        // vma_new->vm_content_offset_in_file = vm_content_offset_in_file;
        // vma_new->vm_content_size_in_file = vm_content_size_in_file;
        // task->vmas[task->vma_cnt] = *vma_new;
        // task->vma_cnt++;
        // // printk("task->vmas->start_va= %|x task->vmas->vm_end= %|x task->vmas->vma_cnt= %d\n", task->vmas[task->vma_cnt-1].vm_start, 
        // // task->vmas[task->vma_cnt-1].vm_end, task->vma_cnt-1);
        // return;
    // }
}

struct vm_area_struct *find_vma(struct task_struct *task, uint64 addr){
    struct vm_area_struct *vmas = &(task->vmas[0]);
    int judge = 0;
    for(int i = 0 ; i < task->vma_cnt ; i++){
        if(addr >= vmas[i].vm_start){
            if(addr < vmas[i].vm_end){
                judge = 1;
                return &(vmas[i]);
            }
        }
    }
    if(judge == 0){
        return NULL;
    }
}
// arch/riscv/kernel/proc.c
void dummy() {
    schedule_test();
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if ((last_counter == -1 || current->counter != last_counter) && current->counter > 0) {
            if(current->counter == 1){
                --(current->counter);   // forced the counter to be zero if this thread is going to be scheduled
            }                           // in case that the new counter is also 1, leading the information not printed.
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
        }
    }
}

void do_timer(void) {
    // 1. 如果当前线程是 idle 线程 直接进行调度
    // 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回 否则进行调度
    if(current == idle){
        schedule();
    }
    else{
        current->counter--;
        if(current->counter){
            return;
        }
        else{
            schedule();
        }
    }
}

void schedule(void) {
    //如果所有运行状态下的线程运行剩余时间都为0，则对 task[1] ~ task[NR_TASKS-1] 的运行剩余时间重新赋值 （使用 rand()） ，之后再重新进行调度。
    //遍历线程指针数组task（不包括 idle ，即 task[0] ）， 在所有运行状态 （TASK_RUNNING） 下的线程运行剩余时间最少的线程作为下一个执行的线程
#ifdef SJF
    int judge_all_0 = 1;
    int select_task = 0;
    for(int i = 1 ; i < NR_TASKS ; i++){
        if(task[i]->counter){
            judge_all_0 = 0;
        }
        if((task[i]->counter < task[select_task]->counter || select_task == 0) && task[i]->state == TASK_RUNNING && task[i]->counter){
            select_task = i;
        }
    }
    if(judge_all_0 == 1){
        for(int i = 0 ; i < NR_TASKS ; i++){
            task[i]->counter = rand();
            printk("SET [PID = %d COUNTER = %d]\n",task[i]->pid,task[i]->counter);
        }
        schedule();
    }
    else{
        printk("\nswitch to [PID = %d COUNTER = %d]\n",task[select_task]->pid,task[select_task]->counter);
        switch_to(task[select_task]);
    }
#else
    int judge_all_0 = 1;
    int select_task = 0;
    for( ; ; ){
        for(int i = NR_TASKS-1 ; i >=1  ; i--){
            if(task[i]->counter){
                judge_all_0 = 0;
            }
            if((task[i]->counter > task[select_task]->counter || select_task == 0) && task[i]->state == TASK_RUNNING && task[i]->counter){
                select_task = i;
            }
        }
        if(select_task){
            break;
        }
        for(int i = NR_TASKS-1 ; i >=1  ; i--){
            if (task[i]){
                task[i]->counter = (task[i]->counter >> 1) +task[i]->priority;
            }
        }   
    }
    if(judge_all_0 == 1){
        for(int i = 0 ; i < NR_TASKS ; i++){
            task[i]->counter = rand();
            printk("SET [PID = %d COUNTER = %d]\n",task[i]->pid,task[i]->counter);
        }
        schedule();
    }
    else{
        printk("\nswitch to [PID = %d COUNTER = %d PRIORITY = %d]\n",task[select_task]->pid,task[select_task]->counter,task[select_task]->priority);
        switch_to(task[select_task]);
    }
#endif
}

