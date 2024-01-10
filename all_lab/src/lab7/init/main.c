#include "printk.h"
#include "sbi.h"
extern void test();
extern void schedule();
int start_kernel() {
    printk("2023");
    printk(" Hello RISC-V\n");
    schedule();
    test(); // DO NOT DELETE !!!

	return 0;
}
