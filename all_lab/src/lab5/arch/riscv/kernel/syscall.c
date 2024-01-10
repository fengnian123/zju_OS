//
// Created by Administrator on 2023/12/8.
//
#include "syscall.h"
extern struct task_struct *current;
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

