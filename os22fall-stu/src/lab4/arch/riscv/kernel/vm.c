#include "defs.h"
#include "types.h"
#include "string.h"

extern char _stext[];
extern char _etext[];
extern char _srodata[];
extern char _erodata[];
extern char _sdata[];
extern char _edata[];

/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
pte_t early_pgtbl[512] __attribute__((__aligned__(0x1000)));
/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
pte_t swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
/*
在 C 语言中，变量和结构体的地址对齐是一个非常重要的概念。对齐是指内存中的数据在存储时的地址对齐方式，这个地址通常是数据类型的整数倍，比如 4 字节或 8 字节。在编译器中，通过设置属性 __attribute__((__aligned__(n))) 可以强制变量或结构体的对齐方式为 n，其中 n 是 2 的整数次幂，表示以字节为单位的对齐方式。这就是在本代码中使用 __attribute__((__aligned__(0x1000))) 来实现对齐的方法。
对于 early_pgtbl 数组，由于每个 entry 的大小为 8 字节，因此这个数组的总大小为 512*8=4096 字节，即 4KB。所以，将 __attribute__((__aligned__(0x1000))) 属性应用到 early_pgtbl 数组，意味着要将其地址对齐到 4KB 边界。这样做的好处是，在访问数组元素时，可以更高效地使用 CPU 的缓存机制，提高代码的性能。
在实际实现中，由于 early_pgtbl 数组大小为 4KB，因此它将占据内存中一个 4KB 的连续空间，这个空间的起始地址将被对齐到 4KB 边界。当程序访问 early_pgtbl 数组时，它将访问这个连续空间的不同位置，这些位置都被对齐到 8 字节边界。这样，就可以通过最少的内存读写操作来访问 early_pgtbl 数组，提高代码的执行效率。
*/

void setup_vm(void) {
    /* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    early_pgtbl[2] = (0x00000002 << 28) | 0xf; // 0x80000000 PPN[2] = 10, V|R|W|X都为1
    early_pgtbl[384] = (0x00000002 << 28) | 0xf; //0xffffffe000000000 PPN[2] = 10, V|R|W|X都为1
}

pte_t* walk(pagetable_t pgtbl, uint64 va, int alloc) {
    if(va > VM_END) {
        printk("walk: VA out of range\n");
        return 0;
    }

    for(int level = 2; level > 0; level--) {
        // 用这个level的9位索引去index pagetable
        pte_t* pte = &pgtbl[PX(level, va)];
        if(*pte & PTE_V) { // 如果是valid的话
            pgtbl = (pagetable_t)PTE2PA(*pte); // pte右移10位左移12位得到下一level的pagetable的地址
        } else {
            if(alloc == 0 || (pgtbl = (pde_t*)kalloc()) == 0) { //给pagetable分配内存
                return 0;
            }
            *pte = PA2PTE(pgtbl) | PTE_V;
        }
    }
    return &pgtbl[PX(0, va)];
}

/* 创建多级页表映射关系 */
void create_mapping(pagetable_t pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
    unsigned long end_va = va + sz;
    for (; va < end_va; va += PGSIZE, pa += PGSIZE) {
        pte_t *pte = walk(pgtbl, va, 1);
        if (!pte) {
            printk("create_mapping: pte should exist");
            return;
        }
        if (*pte & PTE_V) {
            printk("create_mapping: page already mapped");
            return;
        }
        *pte = PA2PTE(pa) | perm | PTE_V;
        printk("pte: %lx\n", *pte);
    }
}

void setup_vm_final(void) {
    printk("setup_vm_final\n");
    memset(swapper_pg_dir, 0x0, PGSIZE);

    printk("_etext = %lx\n",_etext);
	printk("_stext = %lx\n",_stext);
	printk("_erodata = %lx\n",_erodata);
	printk("_srodata = %lx\n",_srodata);
	printk("_edata = %lx\n",_edata);
	printk("_sdata = %lx\n",_sdata);

    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    printk("create mapping for text...\n");
    create_mapping(swapper_pg_dir, _stext, _stext - PA2VA_OFFSET, _etext - _stext, 
                PTE_X | PTE_R | PTE_V);
    

    // mapping kernel rodata -|-|R|V
    printk("create mapping for rodata...\n");
    create_mapping(swapper_pg_dir, _srodata, _srodata - PA2VA_OFFSET, _erodata - _srodata, 
                PTE_R | PTE_V);

    // mapping other memory -|W|R|V
    printk("create mapping for data...\n");
    create_mapping(swapper_pg_dir, _sdata, _sdata - PA2VA_OFFSET, _edata - _sdata, 
                PTE_W | PTE_R | PTE_V);

    // set satp with swapper_pg_dir
    int satp_val = ((uint64)0x1000) << 60 | 0 | ((uint64)swapper_pg_dir >> 12); // mode | satp | PPN
    csr_write(satp, satp_val);
    

    // flush TLB
    asm volatile("sfence.vma zero, zero");

    // flush icache
    asm volatile("fence.i");
    return;
}