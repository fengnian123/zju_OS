#include "printk.h"
#include "defs.h"

// Please do not modify

void test() {
    //printk("kernel is running!\n");
    while (1){
        long long i = 0;
        for(i = 0 ;i < 200000000 ;i++){

        }
        printk("kernel is running!\n");
    }
}
