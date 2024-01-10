//
// Created by Administrator on 2023/12/8.
//
#include "proc.h"
#include "sbi.h"
#include "printk.h"
#include "mm.h"
#include "defs.h"

int sys_write(unsigned int fd, const char* buf, size_t count);

unsigned long sys_getpid();

unsigned long sys_clone(struct pt_regs *regs);

void fork_return();