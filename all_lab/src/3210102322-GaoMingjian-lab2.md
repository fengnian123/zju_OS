# Lab 2: RV64 内核线程调度

姓名：高铭健

学号：3210102322

日期：2023.11.4

## 一、实验目的和要求

### 1.1 实验目的：

- 了解线程概念，并学习线程相关结构体，并实现线程的初始化功能。
- 了解如何使用时钟中断来实现线程的调度。
- 了解线程切换原理，并实现线程的切换。
- 掌握简单的线程调度算法，并完成两种简单调度算法的实现。

### 1.2 实验环境：

- 计算机（Intel Core i5以上，4GB内存以上）系统 
- Ubuntu 22.04.2 LTS




## 二、实验过程和数据记录

### 2.1 准备工程：

- 从 `repo` 同步以下代码: `rand.h/rand.c`，`string.h/string.c`，`mm.h/mm.c`，`proc.h/proc.c`，`test.h/test_schedule.h`，`schedule_test.c` ，同步后代码结构如下：

  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105112101223.png" alt="image-20231105112101223" style="zoom: 50%;" />  

- 在 `_start` 中调用 `mm_init`（```jal mm_init```），来初始化内存管理系统

  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105112420932.png" alt="image-20231105112420932" style="zoom: 67%;" /> 

- 在初始化时用一些自定义的宏，需修改 `defs.h`，在 `defs.h` 添加如下内容：

  ```c
  #define PHY_START 0x0000000080000000
  #define PHY_SIZE  128 * 1024 * 1024 // 128MB，QEMU 默认内存大小
  #define PHY_END   (PHY_START + PHY_SIZE)
  
  #define PGSIZE 0x1000 // 4KB
  #define PGROUNDUP(addr) ((addr + PGSIZE - 1) & (~(PGSIZE - 1)))
  #define PGROUNDDOWN(addr) (addr & (~(PGSIZE - 1)))
  ```



### 2.2 线程调度功能实现

#### 2.2.1 线程初始化

- 为 `idle` 设置 `task_struct`。并将 `current`，`task[0]` 都指向 `idle`，代码如下所示：

  ```c
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
  ```

  

- 将 `task[1]` ~ `task[NR_TASKS - 1]`全部初始化

  - 调用`kalloc()` 为`task[i]`分配一个物理页
  - 每个线程的 `state` 为 `TASK_RUNNING`
  -  `ra` 设置为` __dummy` 的地址,  `sp` 设置为 该线程申请的物理页的高地址

  ```c
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
  ```

- 在 `_start` 中调用 `task_init`

  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105113906981.png" alt="image-20231105113906981" style="zoom: 67%;" />  



#### 2.2.2 `__dummy` 与 `dummy`

- 在 `proc.c` 添加 `dummy()`:


```c
void dummy() {
    schedule_test();
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if ((last_counter == -1 || current->counter != last_counter) && current->counter > 0) {
            if(current->counter == 1){
                --(current->counter);   // forced the counter to be zero if this thread is going to be scheduled
            }                           // in case that the new counter is also 1，leading the information not printed.
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
        }
    }
}
```

- 为线程 **第一次调度** 提供返回函数 `__dummy`，在 `entry.S` 添加 `__dummy`

  - 在`__dummy` 中将 sepc 设置为 `dummy()` 的地址
  - 使用 `sret` 从中断中返回。

  ```assembly
  __dummy:
      # YOUR CODE HERE
      la t0, dummy 
      csrw sepc, t0 #在__dummy 中将 sepc 设置为 dummy() 的地址
      sret #从中断中返回
  ```

  

#### 2.2.3 实现线程切换

- 判断下一个执行的线程 `next` 与当前的线程 `current` 是否为同一个线程，如果是同一个线程，则无需做任何处理，否则调用 `__switch_to` 进行线程切换。

  ```c
  void switch_to(struct task_struct* next) {
      /* YOUR CODE HERE */
      //判断下一个执行的线程 next 与当前的线程 current 是否为同一个线程，如果是同一个线程，则无需做任何处理，否则调用 __switch_to 进行线程切换。
      if(current->pid != next->pid){
          struct task_struct*tmp = current;
          current = next ;
          __switch_to(tmp, next);
      }
  }
  ```

- 在 `entry.S` 中实现线程上下文切换 `__switch_to`:

  - `__switch_to`接受两个 `task_struct` 指针作为参数，分别在`a0`和`a1`中
  - 保存当前线程的`ra`，`sp`，`s0~s11`到当前线程的 `thread_struct` （偏移量为`40`，因为`thread_struct` 有5个成员变量）
  - 将下一个线程的 `thread_struct` 中的相关数据载入到`ra`，`sp`，`s0~s11`中

  ```assembly
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
  ```

#### 2.2.4 实现调度入口函数

实现 `do_timer()`，并在 `时钟中断处理函数` 中调用。

-  如果当前线程是 idle 线程 直接进行调度 
- 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回 否则进行调度

```c
void do_timer(void) {
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
```

```c
void trap_handler(unsigned long scause, unsigned long sepc) {
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.3.4 节
    // 其他interrupt / exception 可以直接忽略
    if(scause & 0x8000000000000000){//trap 类型为interrupt
        if(scause == 0x8000000000000005){//timer interrupt
            printk("[S] Supervisor Mode Timer Interrupt\n");
            clock_set_next_event();
            do_timer();
        }
    }
    else{

    }
}
```



#### 2.2.5 实现线程调度

##### 2.2.5.1 短作业优先调度算法

- 遍历线程指针数组`task`（不包括 `idle` ，即 `task[0]` ）， 在所有运行状态 （`TASK_RUNNING`） 下的线程运行剩余时间最少的线程作为下一个执行的线程。
- 如果所有运行状态下的线程运行剩余时间都为0，则对 `task[1]` ~ `task[NR_TASKS-1]` 的运行剩余时间重新赋值 （使用 `rand()`） ，之后再重新进行调度。
- 打印设置和switch是的`pid`和`counter`信息

```c
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
```

##### 2.2.5.2 优先级调度算法

- 遍历线程指针数组`task`（不包括 `idle` ，即 `task[0]` ）， 第一轮在所有运行状态 （`TASK_RUNNING`） 下的线程运行剩余时间最多的线程作为下一个执行的线程，如果时间相同，选择优先级更高的线程
- 第二轮所有运行状态下的线程运行剩余时间都为0，将`(task[i]->counter >> 1) +task[i]->priority`的结果作为新的时间片，也就是`priority`与`counter`一致，再选择剩余时间最多的线程作为下一个执行的线程就相当于按优先级调度
- 打印设置和switch是的`pid`和`counter`信息

```c
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
```



### 2.3 编译及测试

- 在`proc.c`中使用 `#ifdef` ， `#endif` 来控制代码，用`#ifdef SJF` ,`#else`实现编译时的代码选择，完整的`shedule`函数如下：

  ```c
  void schedule(void) {
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
      for(int i = 1 ; i < NR_TASKS ; i++){
          if(task[i]->counter){
              judge_all_0 = 0;
          }
          if((task[i]->counter > task[select_task]->counter || select_task == 0) && task[i]->state == TASK_RUNNING && task[i]->counter){
              select_task = i;
          }
          if(task[i]->counter == task[select_task]->counter && task[i]->priority > task[select_task]->priority){
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
          printk("\nswitch to [PID = %d COUNTER = %d PRIORITY = %d]\n",task[select_task]->pid,task[select_task]->counter,task[select_task]->priority);
          switch_to(task[select_task]);
      }
  #endif
  }
  ```

  

- `NR_TASKS = 4` 情况下短作业优先调度算法的测试结果：

  先修改`makefile`:

  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105122608156.png" alt="image-20231105122608156" style="zoom:67%;" /> 

  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105122143078.png" alt="image-20231105122143078" style="zoom:50%;" /> 

  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105122233814.png" alt="image-20231105122233814" style="zoom: 50%;" /> 

  测试通过

- `NR_TASKS = 4` 情况下优先级调度算法的测试结果

  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105122424044.png" alt="image-20231105122424044" style="zoom:50%;" /> 
  
  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105122456204.png" alt="image-20231105122456204" style="zoom:50%;" /> 
  
  测试通过
  
- `NR_TASKS = 16` 情况下短作业优先调度算法的测试结果：

  | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105122940581.png" alt="image-20231105122940581" style="zoom: 67%;" /> | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105123012395.png" alt="image-20231105123012395" style="zoom: 67%;" /> |
  | ------------------------------------------------------------ | ------------------------------------------------------------ |
  | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105123043914.png" alt="image-20231105123043914" style="zoom: 67%;" /> | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105123103217.png" alt="image-20231105123103217" style="zoom: 67%;" /> |
  | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105123220994.png" alt="image-20231105123220994" style="zoom:67%;" /> | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105123235959.png" alt="image-20231105123235959" style="zoom:67%;" /> |
  | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105123249776.png" alt="image-20231105123249776" style="zoom:67%;" /> | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105123307711.png" alt="image-20231105123307711" style="zoom:67%;" /> |
  | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105123326703.png" alt="image-20231105123326703" style="zoom:67%;" /> | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105123345068.png" alt="image-20231105123345068" style="zoom:67%;" /> |
  
  测试通过
  
- `NR_TASKS = 16` 情况下优先级调度算法的测试结果：

  | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105123945231.png" alt="image-20231105123945231" style="zoom:67%;" /> | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105124008629.png" alt="image-20231105124008629" style="zoom:67%;" /> |
  | ------------------------------------------------------------ | ------------------------------------------------------------ |
  | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105124024181.png" alt="image-20231105124024181" style="zoom:67%;" /> | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105124048274.png" alt="image-20231105124048274" style="zoom:67%;" /> |
  | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105124103997.png" alt="image-20231105124103997" style="zoom:67%;" /> | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105124121561.png" alt="image-20231105124121561" style="zoom:67%;" /> |
  | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105124139979.png" alt="image-20231105124139979" style="zoom:67%;" /> | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231108235426791.png" alt="image-20231108235426791" style="zoom:67%;" /> |
  | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231108235442243.png" alt="image-20231108235442243" style="zoom:67%;" /> | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231108235513087.png" alt="image-20231108235513087" style="zoom:67%;" /> |
  | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231108235604935.png" alt="image-20231108235604935" style="zoom:67%;" /> | <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231109001508101.png" alt="image-20231109001508101" style="zoom:67%;" /> |

​	测试通过（从第二轮开始，完全按`priority`由高到低调度，此时`counter`与`priority`相同，所以从`priority`最大的线程2开始）



## 三、思考题

> 1. **在 RV64 中一共用 32 个通用寄存器，为什么 `context_switch` 中只保存了14个?**

答：因为`switch`函数调用时只需要保存`Callee Saved Register`和`ra`、`sp`，`Caller Saved Register`会被编译器自动保存在当前的栈上，也就是只需要保存`ra、sp、s0~s11`寄存器，



> 2. 当线程第一次调用时，其 `ra` 所代表的返回点是 `__dummy`。那么在之后的线程调用中 `context_switch` 中，`ra` 保存/恢复的函数返回点是什么呢? 请同学用 gdb 尝试追踪一次完整的线程切换流程，并关注每一次 `ra` 的变换 (需要截图)。

- 运行到第一次保存`ra`的地方，查看此时`ra`的值为0x802004c4（switch+92）

  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105130153122.png" alt="image-20231105130153122" style="zoom:50%;" /> 

- 运行到第一次恢复`ra`的地方，此时`ra`为`dummy`的地址

  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105130400335.png" alt="image-20231105130400335" style="zoom:50%;" /> 

- 运行到第二次保存`ra`的地方，查看此时`ra`的值还是0x802004c4（switch+92）

  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105130606539.png" alt="image-20231105130606539" style="zoom:50%;" /> 

- 之后都是第一次被调度，`ra`的变化一样

- 直到进程第二次被调度，此时恢复`ra`的时候，`ra`不再是为`dummy`的地址，而是0x802004c4（switch+92）

  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105131127274.png" alt="image-20231105131127274" style="zoom:50%;" /> 

- 之后无论保存还是恢复，`ra`的值均为0x802004c4（switch+92），不再变化

  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105131359930.png" alt="image-20231105131359930" style="zoom:50%;" /> 

  这个返回的地址（proc.c:31）就是调用switch_to的下一行

  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105131653574.png" alt="image-20231105131653574" style="zoom:50%;" /> 

  <img src="C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20231105131845362.png" alt="image-20231105131845362" style="zoom:50%;" /> 

## 四、遇到的问题与心得

  通过操作系统的第一次实验，我了解了线程的概念，并学习了线程相关结构体，最后实现了线程的调度，在实验过程中，由于对相关知识的不了解，遇到了一些问题，我在搜索了解相关知识以及自己尝试后终于解决了如下问题：

1. 没有在_start里调用初始化函数导致一直卡住，在反复调试后还是不知道出现什么问题，最后仔细看实验指导才发现问题

2. 对于优先级调度算法一开始理解错误，以为是先考虑优先级再考虑时间，导致结果错误

   





