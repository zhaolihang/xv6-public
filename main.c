#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void startothers(void);
static void mpmain(void)  __attribute__((noreturn));
extern pde_t *kpgdir;
extern char end[]; // first address after kernel loaded from ELF file  定义在 kernel.ld 中全局标签 代表整个内核文件的末尾

// Bootstrap processor starts running C code here.
// Allocate a real stack and switch to it, first
// doing some setup required for memory allocator to work.
int
main(void)
{
  kinit1(end, P2V(4*1024*1024)); // phys page allocator   将 end~0x80000000+4*1024*1024 的内存每4k加入到内核内存空闲列表中 现在只有不到4m的内存
  kvmalloc();      // kernel page table  // 分配并切换到内核页表
  mpinit();        // detect other processors
  lapicinit();     // interrupt controller 配置本地中断
  seginit();       // segment descriptors  // 设置当前cpu的gdt
  picinit();       // disable pic  禁用8259中断
  ioapicinit();    // another interrupt controller 配置io中断
  consoleinit();   // console hardware// 绑定 读写函数指针 开启键盘中断
  uartinit();      // serial port  // 打开 串口
  pinit();         // process table 初始化进程表的锁
  tvinit();        // trap vectors  初始化设置中断向量表,cpu并没有加载到idtr中
  binit();         // buffer cache  // 初始化io的环形缓冲区
  fileinit();      // file table  初始化文件表的锁
  ideinit();       // disk 初始化硬盘  并且打开ide中断
  startothers();   // start other processors   //启动其他cpu 并进入scheduler 函数
  kinit2(P2V(4*1024*1024), P2V(PHYSTOP)); // must come after startothers()  // 初始化其他的空闲内存
  userinit();      // first user process 在进程表中加入第一个用户进程  很重要！！！！！！！！！！！！！！！！！！！！！！！！！
  mpmain();        // finish this processor's setup  finish bsp cpu 然后执行scheduler 函数
}

// Other CPUs jump here from entryother.S.
static void
mpenter(void)// entryother.S 调用这里 但是使用了临时的pgdirtable
{
  //  kvmalloc();    不需要再分配了因为整个内核使用一份数据 在上面已经初始化过了
  switchkvm();// 切换到 内核的页目录表
  seginit();// 初始化当前cpu的gdt
  lapicinit();// 初始化当前cpu的lapic
  mpmain();
}

// Common CPU setup code.
static void
mpmain(void)
{
  cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
  idtinit();       // load idt register 加载中断向量表
  xchg(&(mycpu()->started), 1); //  tell bsp cpu's startothers() we're up
  scheduler();     // start running processes  // 当前cpu的中断 会在scheduler里面开启
}

pde_t entrypgdir[];  // For entry.S

// Start the non-boot (AP) processors.
static void
startothers(void)
{
  //  _binary_entryother_start  _binary_entryother_size 是ld中把entryother.S文件编译成的二进制放在内核文件标号_binary_entryother_start处
  extern uchar _binary_entryother_start[], _binary_entryother_size[];
  uchar *code;
  struct cpu *c;
  char *stack;

  // Write entry code to unused memory at 0x7000.
  // The linker has placed the image of entryother.S in
  code = P2V(0x7000);
  memmove(code, _binary_entryother_start, (uint)_binary_entryother_size);// entryother.bin 这段代码写到物理内存0x7000处

  for(c = cpus; c < cpus+ncpu; c++){
    if(c == mycpu())  // We've started already.
      continue;

    // Tell entryother.S what stack to use, where to enter, and what
    // pgdir to use. We cannot use kpgdir yet, because the AP processor
    // is running in low  memory, so we use entrypgdir for the APs too.
    stack = kalloc(); //  分配4K的栈  stack是栈最低地址处
    *(void**)(code-4) = stack + KSTACKSIZE;// ap使用在内存中分配的栈 ，最高地址处即当前的栈顶
    *(void**)(code-8) = mpenter;// ap cpu的高地址c代码
    *(int**)(code-12) = (void *) V2P(entrypgdir);// ap cpu 初始的页目录表物理地址

    lapicstartap(c->apicid, V2P(code));// bsp cpu send message to ap cpu by IPI , CPU之间通信

    // wait for cpu to finish mpmain()
    while(c->started == 0)//等待该核心启动完成后继续初始化下一个核心
      ;
  }
}

// The boot page table used in entry.S and entryother.S.
// Page directories (and page tables) must start on page boundaries,
// hence the __aligned__ attribute.
// PTE_PS in a page directory entry enables 4Mbyte pages.

__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
  // Map VA's [0, 4MB) to PA's [0, 4MB)
  [0] = (0) | PTE_P | PTE_W | PTE_PS,
  // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
  [KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};

//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.

