//arch/riscv/kernel/proc.c
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "rand.h"
#include "printk.h"
#include "test.h"
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

void switch_to(struct task_struct* next) {
    /* YOUR CODE HERE */
    //判断下一个执行的线程 next 与当前的线程 current 是否为同一个线程，如果是同一个线程，则无需做任何处理，否则调用 __switch_to 进行线程切换。
    if(current->pid != next->pid){
        struct task_struct*tmp = current;
        current = next ;
        __switch_to(tmp, next);
    }
}
void task_init() {
    test_init(NR_TASKS);
    mm_init();
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
        task[i]->priority = task_test_priority[i];
        task[i]->state = TASK_RUNNING;
        uint64 ra_address = &__dummy;
        uint64 sp_address = (uint64)task[i] + PGSIZE;
        task[i]->thread.ra = ra_address;
        task[i]->thread.sp = sp_address;
    }
    /* YOUR CODE HERE */

    printk("...proc_init done!\n");
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