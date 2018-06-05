#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "kextern_data.h"

static void start_others(void);
static void mp_main(void) __attribute__((noreturn));

// Bootstrap processor starts running C code here.
// Allocate a real stack and switch to it, first
// doing some setup required for memory allocator to work.
int main(void) {
    // phys page allocator 将 kernel_end~0x80000000+4*1024*1024 的内存每4k加入到内核内存空闲列表中 现在只有不到4m的内存
    init_kernel_mem_1(kernel_end, C_P2V(4 * 1024 * 1024));
    init_kvm_pgdir();    // 分配 kernel page directory table
    switch2kvm();        // 换到内核页表 lcr3

    mpinit();    // detect other processors  smp架构获取其他cpu的信息

    lapicinit();    // interrupt controller  配置本地中断控制器
    seginit();      // segment descriptors  设置当前cpu的gdt

    picinit();        // disable pic  禁用8259中断
    ioapicinit();     // another interrupt controller  配置io中断控制器
    consoleinit();    // console hardware  绑定 读写函数指针 开启键盘中断
    uartinit();       // serial port  打开串口 可能是显示器的数据口
    pinit();          // process table 初始化进程表的锁
    tvinit();         // trap vectors  初始化设置中断向量表,cpu并没有加载到idtr中
    binit();          // buffer cache  初始化io的环形缓冲区
    fileinit();       // file table  初始化文件表的锁
    ideinit();        // disk   初始化硬盘,并且打开ide中断

    start_others();    // start other processors   启动其他cpu 并进入scheduler 函数

    // must come after start_others()  初始化其他的空闲内存
    init_kernel_mem_2(C_P2V(4 * 1024 * 1024), C_P2V(PHY_TOP_LIMIT));

    userinit();    // first user process 在进程表中加入第一个用户进程  很重要!!!!!!!!!!!!!!!
    mp_main();     // finish this processor's setup  finish bsp cpu 然后执行scheduler 函数
}

// Other CPUs jump here from entryother.S.
static void mpenter(void)    // entryother.S 调用这里 但是使用了临时的 page_dirctory_table
{
    // init_kvm_pgdir(); 整个内核使用一份内存映射数据 在上面已经初始化过了
    switch2kvm();    // 换到内核页表 lcr3
    seginit();       // 初始化当前cpu的gdt
    lapicinit();     // 初始化当前cpu的lapic
    mp_main();
}

// Common CPU setup code.
static void mp_main(void) {
    cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
    idt_init();                      // lidt load idt register 加载中断向量表
    xchg(&(mycpu()->started), 1);    //  tell bsp cpu's start_others() we're up
    scheduler();                     // start running processes  // 当前cpu的中断 会在scheduler里面开启
}


// Start the non-boot (AP) processors.
static void start_others(void) {
    // ld 链接二进制文件 会生成3个symbol: _binary_***_start _binary_***_end _binary_***_size
    // 是ld中把entryother二进制文件放在内核文件标号_binary_entryother_start处
    // Google: ld embedded binary
    // https://www.linuxjournal.com/content/embedding-file-executable-aka-hello-world-version-5967
    // https://balau82.wordpress.com/2012/02/19/linking-a-binary-blob-with-gcc/
    // http://gareus.org/wiki/embedding_resources_in_executables
    uchar*      code;
    struct cpu* c;
    char*       stack;

    // Write entry code to unused memory at 0x7000.
    // The linker has placed the image of entryother.S in
    code = C_P2V(0x7000);
    // entryother.bin 这段代码写到物理内存0x7000处 因为 entryother.bin 的vstart是0x7000
    memmove(code, _binary_entryother_start, ( uint )_binary_entryother_size);

    for (c = cpus; c < cpus + cpu_count; c++) {
        if (c == mycpu())    // We've started already.
            continue;

        // Tell entryother.S what stack to use, where to enter, and what
        // pgdir to use. We cannot use kernel_page_dir yet, because the AP processor
        // is running in low  memory, so we use entry_page_directory for the APs too.
        stack                  = kalloc_page();                           //  分配4K的栈  stack是栈最低地址处
        *( int** )(code - 4)   = ( void* )C_V2P(entry_page_directory);    // ap cpu 初始的页目录表物理地址
        *( void** )(code - 8)  = stack + PAGE_SIZE;                       // ap使用在内存中分配的栈 ，最高地址处即当前的栈顶
        *( void** )(code - 12) = mpenter;                                 // ap cpu的高地址c代码

        lapicstartap(c->apicid, C_V2P(code));    // bsp cpu send message to ap cpu by IPI , CPU之间通信

        // wait for cpu to finish mp_main()
        while (c->started == 0)    //等待该核心启动完成后继续初始化下一个核心
            ;
    }
}
