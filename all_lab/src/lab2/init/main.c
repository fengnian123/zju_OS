#include "printk.h"
#include "sbi.h"
extern void test();

int start_kernel() {
    printk("2022");
    printk(" Hello RISC-V\n");
    task_init();
    test(); // DO NOT DELETE !!!

	return 0;
}