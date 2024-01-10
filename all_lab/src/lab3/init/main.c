#include "printk.h"
#include "sbi.h"
extern void test();
int start_kernel() {
    printk("2023");
    printk(" Hello RISC-V\n");
    test(); // DO NOT DELETE !!!

	return 0;
}
