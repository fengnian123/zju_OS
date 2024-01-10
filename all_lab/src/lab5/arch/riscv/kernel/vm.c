// arch/riscv/kernel/vm.c

#include "defs.h"
#include "types.h"
#include "printk.h"
#include "mm.h"
#include "print.h"
/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
unsigned long  early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm(void) {
    /*
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略dfd
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    uint64 index = PHY_START >> 30;
    index = index & 0x1ff;
    early_pgtbl[index] = PHY_START >> 2 | 15;

    index = (PHY_START + PA2VA_OFFSET) >> 30;
    index = index & 0x1ff;
    early_pgtbl[index] = PHY_START >> 2 | 15;
}

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
char _stext[];
char _srodata[];
char _sdata[];
char _ekernel[];
void setup_vm_final(void) {

    // mapping kernel text X|-|R|V
    uint64 va_text = (uint64)_stext;
    uint64 pa_text = (uint64)_stext - PA2VA_OFFSET;
    uint64 sz_text = (uint64)_srodata - (uint64)_stext;
    int perm_text = 0b1011;
    create_mapping(swapper_pg_dir, va_text,pa_text,sz_text, perm_text);

    // mapping kernel rodata -|-|R|V
    uint64 va_rodata = (uint64)_srodata;
    uint64 pa_rodata = (uint64)_srodata - PA2VA_OFFSET;
    uint64 sz_rodata = (uint64)_sdata - (uint64)_srodata;
    int perm_rodata = 0b0011;
    create_mapping(swapper_pg_dir, va_rodata, pa_rodata, sz_rodata, perm_rodata);

    // mapping other memory -|W|R|V
    uint64 va_orther = (uint64)_sdata;
    uint64 pa_orther = (uint64)_sdata - PA2VA_OFFSET;
    uint64 sz_orther = PHY_SIZE - OPENSBI_SIZE - ((uint64)_sdata - (uint64)_stext);
    int perm_orther = 0b0111;
    create_mapping(swapper_pg_dir, va_orther, pa_orther, sz_orther, perm_orther);
    printk("finish\n");

    // set satp with swapper_pg_dir

     uint64 ppn = ((uint64)swapper_pg_dir - PA2VA_OFFSET) >> 12;
     uint64 mode = 0x8000000000000000;
     uint64 satp_change = ppn | mode;
     csr_write(satp, satp_change);

    // flush TLB
    asm volatile("sfence.vma zero, zero");

    // flush icache
    asm volatile("fence.i");
    return;
}

/* 创建多级页表映射关系 */
create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm){

    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
    uint64 offset[3];
    uint64 page[3];
    unsigned long *pgt_next = NULL;
    unsigned long *pgt_nnext = NULL;
    for (uint64 i = 0; i < sz; i += PGSIZE){
        offset[2] = (va >> 30) & 0x1FF;
        offset[1] = (va >> 21) & 0x1FF;
        offset[0] = (va >> 12) & 0x1FF;
        if (!(pgtbl[offset[2]] & 0x1)){
            pgt_next = (unsigned long *)kalloc();
            pgt_nnext = (unsigned long *)kalloc();
            page[2] = (uint64)pgt_next;
            page[2] -= PA2VA_OFFSET;
            page[2] = page[2] >> 12;
            page[2] = page[2] << 10;
            pgtbl[offset[2]] = page[2] | 0x0000000000000001;
        }
        else {
            pgt_next = (uint64 *)(((pgtbl[offset[2]] & 0xfffffffffffffe00) << 2) + PA2VA_OFFSET);
            if(va == 0){
                printk("123");
            }
        }
        if (!(pgt_next[offset[1]] & 0x1)){
            pgt_nnext = (unsigned long *)kalloc();
            page[1] = (uint64)pgt_nnext;
            page[1] -= PA2VA_OFFSET;
            page[1] = page[1] >> 12;
            page[1] = page[1] << 10;
            pgt_next[offset[1]] = (page[1]) | 0x0000000000000001;
        }
        else {
            pgt_nnext = (uint64 *)(((pgt_next[offset[1]] & 0xfffffffffffffe00) << 2) + PA2VA_OFFSET);
            if(va == 0){
                printk("456");
            }
        }
        page[0] = pa >> 12;
        page[0] = page[0] <<10;
        pgt_nnext[offset[0]] = page[0] | perm;
        va += PGSIZE;
        pa += PGSIZE;
    }
}

