#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"

extern char data[];    // defined by kernel.ld
pde_t*      kpgdir;    // for use in scheduler()

// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void seginit(void) {
    struct cpu* c;

    // Map "logical" addresses to virtual addresses using identity map.
    // Cannot share a CODE descriptor for both kernel and user
    // because it would have to have DPL_USR, but the CPU forbids
    // an interrupt from CPL=0 to DPL=3.
    c                       = &cpus[cpuid()];
    c->gdt[SEG_KCODE_INDEX] = MAKE_SEG_DESCRIPTOR_32(APP_SEG_TYPE_X | APP_SEG_TYPE_R, 0, 0xffffffff, 0);
    c->gdt[SEG_KDATA_INDEX] = MAKE_SEG_DESCRIPTOR_32(APP_SEG_TYPE_W, 0, 0xffffffff, 0);
    c->gdt[SEG_UCODE_INDEX] = MAKE_SEG_DESCRIPTOR_32(APP_SEG_TYPE_X | APP_SEG_TYPE_R, 0, 0xffffffff, DPL_USER);
    c->gdt[SEG_UDATA_INDEX] = MAKE_SEG_DESCRIPTOR_32(APP_SEG_TYPE_W, 0, 0xffffffff, DPL_USER);
    lgdt(c->gdt, sizeof(c->gdt));
}

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
static pte_t* walkpgdir(pde_t* pgdir, const void* va, int alloc) {
    pde_t* pde;
    pte_t* pgtab;

    pde = &pgdir[PAGE_DIR_INDEX(va)];
    if (*pde & PTE_P) {
        pgtab = ( pte_t* )P2V(PTE_ADDR(*pde));
    } else {
        if (!alloc || (pgtab = ( pte_t* )kalloc()) == 0)
            return 0;
        // Make sure all those PTE_P bits are zero.
        memset(pgtab, 0, PAGE_SIZE);
        // The permissions here are overly generous, but they can
        // be further restricted by the permissions in the page table
        // entries, if necessary.
        *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
    }
    return &pgtab[PAGE_TABLE_INDEX(va)];
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
static int mappages(pde_t* pgdir, void* va, uint size, uint pa, int perm) {
    char * a, *last;
    pte_t* pte;

    a    = ( char* )PAGE_ROUNDDOWN(( uint )va);
    last = ( char* )PAGE_ROUNDDOWN((( uint )va) + size - 1);
    for (;;) {
        if ((pte = walkpgdir(pgdir, a, 1)) == 0)
            return -1;
        if (*pte & PTE_P)
            panic("remap");
        *pte = pa | perm | PTE_P;
        if (a == last)
            break;
        a += PAGE_SIZE;
        pa += PAGE_SIZE;
    }
    return 0;
}

// There is one page table per process, plus one that's used when
// a CPU is not running any process (kpgdir). The kernel uses the
// current process's page table during system calls and interrupts;
// page protection bits prevent user code from using the kernel's
// mappings.
//
// alloc_kvm_pgdir() and exec() set up every page table like this:
//
//   0..KERNAL_SPACE_BASE: user memory (text+data+stack+heap), mapped to
//                phys memory allocated by the kernel
//   KERNAL_SPACE_BASE..KERNAL_SPACE_BASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
//   KERNAL_SPACE_BASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
//                for the kernel's instructions and r/o data
//   data..KERNAL_SPACE_BASE+TOP_PHYSICAL: mapped to V2P(data)..TOP_PHYSICAL,
//                                  rw data + free physical memory
//   0xfe000000..0: mapped direct (devices such as ioapic)
//
// The kernel allocates physical memory for its heap and for user memory
// between V2P(end) and the end of physical memory (TOP_PHYSICAL)
// (directly addressable from end..P2V(TOP_PHYSICAL)).

// This table defines the kernel's mappings, which are present in
// every process's page table.
static struct kmap {
    void* virt;
    uint  phys_start;
    uint  phys_end;
    int   perm;
} kmap[] = {
    { ( void* )KERNAL_SPACE_BASE, 0, EXTMEM, PTE_W },                          // I/O space
    { ( void* )KERNAL_LINKED_BASE, V2P(KERNAL_LINKED_BASE), V2P(data), 0 },    // kern text+rodata
    { ( void* )data, V2P(data), TOP_PHYSICAL, PTE_W },                         // kern data+memory
    { ( void* )DEVICE_SPACE, DEVICE_SPACE, 0, PTE_W },                         // more devices
};

// Set up kernel part of a page table.
pde_t* alloc_kvm_pgdir(void) {
    pde_t*       pgdir;
    struct kmap* k;

    if ((pgdir = ( pde_t* )kalloc()) == 0)
        return 0;
    memset(pgdir, 0, PAGE_SIZE);
    if (P2V(TOP_PHYSICAL) > ( void* )DEVICE_SPACE)
        panic("TOP_PHYSICAL too high");
    for (k = kmap; k < &kmap[SIZEOF_ARRAY(kmap)]; k++)
        if (mappages(pgdir, k->virt, k->phys_end - k->phys_start, ( uint )k->phys_start, k->perm) < 0) {
            freevm(pgdir);
            return 0;
        }
    return pgdir;
}

// Allocate one page table for the machine for the kernel address
// space for scheduler processes.
void init_kvm_pgdir(void) {
    kpgdir = alloc_kvm_pgdir();    // 分配生成完整的页目录表 和页表  并保存为全局变量 kpgdir
}

// Switch h/w page table register to the kernel-only page table,
// for when no process is running.
void switch2kvm(void) {
    lcr3(V2P(kpgdir));    // switch to the kernel page table
}

// Switch TSS and h/w page table to correspond to process p.
void switch2uvm(struct proc* p)    // set ltr  and  lcr3
{
    if (p == 0)
        panic("switch2uvm: no process");
    if (p->kstack == 0)
        panic("switch2uvm: no kstack");
    if (p->pgdir == 0)
        panic("switch2uvm: no pgdir");

    pushcli();
    mycpu()->gdt[SEG_TSS_INDEX]   = MAKE_SEG_DESCRIPTOR_16(SYS_SEG_TYPE_T32A, &mycpu()->tss, sizeof(mycpu()->tss) - 1, 0);    // 初始化 tss
    mycpu()->gdt[SEG_TSS_INDEX].s = 0;
    mycpu()->tss.ss0              = SEG_KDATA_INDEX << 3;
    mycpu()->tss.esp0             = ( uint )p->kstack + KSTACK_SIZE;    // 特权级的栈 用户空间发生中断的时候用
    // setting IOPL=0 in eflags *and* iomb beyond the tss segment limit
    // forbids I/O instructions (e.g., inb and outb) from user space
    mycpu()->tss.iomb = ( ushort )0xFFFF;    // 没有io开放
    ltr(SEG_TSS_INDEX << 3);                 // 任务标志tss 加载到 tr中
    lcr3(V2P(p->pgdir));                     // switch to process's address space  // 加载cr3 用户的页目录表
    popcli();
}

// Load the initcode into address 0 of pgdir.
// sz must be less than a page.
void init_initcode_uvm(pde_t* pgdir, char* init, uint sz) {
    // 把 initcode.bin 从高地址copy到0地址因为 initcode.bin 的vstart是0
    char* mem;

    if (sz >= PAGE_SIZE)    //不能大于4k
        panic("init_initcode_uvm: more than a page");
    mem = kalloc();
    memset(mem, 0, PAGE_SIZE);
    mappages(pgdir, 0, PAGE_SIZE, V2P(mem), PTE_W | PTE_U);
    memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int loaduvm(pde_t* pgdir, char* addr, struct inode* ip, uint offset, uint sz) {
    uint   i, pa, n;
    pte_t* pte;

    if (( uint )addr % PAGE_SIZE != 0)
        panic("loaduvm: addr must be page aligned");
    for (i = 0; i < sz; i += PAGE_SIZE) {
        if ((pte = walkpgdir(pgdir, addr + i, 0)) == 0)
            panic("loaduvm: address should exist");
        pa = PTE_ADDR(*pte);
        if (sz - i < PAGE_SIZE)
            n = sz - i;
        else
            n = PAGE_SIZE;
        if (readi(ip, P2V(pa), offset + i, n) != n)
            return -1;
    }
    return 0;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int allocuvm(pde_t* pgdir, uint oldsz, uint newsz) {
    char* mem;
    uint  a;

    if (newsz >= KERNAL_SPACE_BASE)
        return 0;
    if (newsz < oldsz)
        return oldsz;

    a = PAGE_ROUNDUP(oldsz);
    for (; a < newsz; a += PAGE_SIZE) {
        mem = kalloc();
        if (mem == 0) {
            cprintf("allocuvm out of memory\n");
            deallocuvm(pgdir, newsz, oldsz);
            return 0;
        }
        memset(mem, 0, PAGE_SIZE);
        if (mappages(pgdir, ( char* )a, PAGE_SIZE, V2P(mem), PTE_W | PTE_U) < 0) {
            cprintf("allocuvm out of memory (2)\n");
            deallocuvm(pgdir, newsz, oldsz);
            kfree(mem);
            return 0;
        }
    }
    return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int deallocuvm(pde_t* pgdir, uint oldsz, uint newsz) {
    pte_t* pte;
    uint   a, pa;

    if (newsz >= oldsz)
        return oldsz;

    a = PAGE_ROUNDUP(newsz);
    for (; a < oldsz; a += PAGE_SIZE) {
        pte = walkpgdir(pgdir, ( char* )a, 0);
        if (!pte)
            a = PGADDR(PAGE_DIR_INDEX(a) + 1, 0, 0) - PAGE_SIZE;
        else if ((*pte & PTE_P) != 0) {
            pa = PTE_ADDR(*pte);
            if (pa == 0)
                panic("kfree");
            char* v = P2V(pa);
            kfree(v);
            *pte = 0;
        }
    }
    return newsz;
}

// Free a page table and all the physical memory pages
// in the user part.
void freevm(pde_t* pgdir) {
    uint i;

    if (pgdir == 0)
        panic("freevm: no pgdir");
    deallocuvm(pgdir, KERNAL_SPACE_BASE, 0);
    for (i = 0; i < PAGE_DIR_TABLE_ENTRY_SIZE; i++) {
        if (pgdir[i] & PTE_P) {
            char* v = P2V(PTE_ADDR(pgdir[i]));
            kfree(v);
        }
    }
    kfree(( char* )pgdir);
}

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void clearpteu(pde_t* pgdir, char* uva) {
    pte_t* pte;

    pte = walkpgdir(pgdir, uva, 0);
    if (pte == 0)
        panic("clearpteu");
    *pte &= ~PTE_U;
}

// Given a parent process's page table, create a copy
// of it for a child.
pde_t* copyuvm(pde_t* pgdir, uint sz) {
    pde_t* d;
    pte_t* pte;
    uint   pa, i, flags;
    char*  mem;

    if ((d = alloc_kvm_pgdir()) == 0)
        return 0;
    for (i = 0; i < sz; i += PAGE_SIZE) {
        if ((pte = walkpgdir(pgdir, ( void* )i, 0)) == 0)
            panic("copyuvm: pte should exist");
        if (!(*pte & PTE_P))
            panic("copyuvm: page not present");
        pa    = PTE_ADDR(*pte);
        flags = PTE_FLAGS(*pte);
        if ((mem = kalloc()) == 0)
            goto bad;
        memmove(mem, ( char* )P2V(pa), PAGE_SIZE);
        if (mappages(d, ( void* )i, PAGE_SIZE, V2P(mem), flags) < 0)
            goto bad;
    }
    return d;

bad:
    freevm(d);
    return 0;
}

//PAGEBREAK!
// Map user virtual address to kernel address.
char* uva2ka(pde_t* pgdir, char* uva) {
    pte_t* pte;

    pte = walkpgdir(pgdir, uva, 0);
    if ((*pte & PTE_P) == 0)
        return 0;
    if ((*pte & PTE_U) == 0)
        return 0;
    return ( char* )P2V(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int copyout(pde_t* pgdir, uint va, void* p, uint len) {
    char *buf, *pa0;
    uint  n, va0;

    buf = ( char* )p;
    while (len > 0) {
        va0 = ( uint )PAGE_ROUNDDOWN(va);
        pa0 = uva2ka(pgdir, ( char* )va0);
        if (pa0 == 0)
            return -1;
        n = PAGE_SIZE - (va - va0);
        if (n > len)
            n = len;
        memmove(pa0 + (va - va0), buf, n);
        len -= n;
        buf += n;
        va = va0 + PAGE_SIZE;
    }
    return 0;
}
