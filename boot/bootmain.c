// Boot loader.
//
// Part of the boot block, along with bootasm.S, which calls bootmain().
// bootasm.S has put the processor into protected 32-bit mode.
// bootmain() loads an ELF kernel image from the disk starting at
// sector 1 and then jumps to the kernel entry routine.

#include "types.h"
#include "elf.h"
#include "x86.h"

#define SECTOR_SIZE 512

static void waitdisk(void);
static void readsector(void* dst, uint offset);
static void readseg(uchar* pa, uint count, uint offset);


void bootmain(void)    // 加载内核到物理地址 0x10000(64k) 并进入内核
{
    struct elfhdr*  elf;
    struct proghdr *ph, *ph_end;
    void (*entry)(void);
    uchar* phy_addr;

    elf = ( struct elfhdr* )0x10000;    // (64k)

    // Read 1st page off disk
    readseg(( uchar* )elf, 4096, 0);

    // Is this an ELF executable?
    if (elf->magic != ELF_MAGIC)
        return;    // let bootasm.S handle error

    // Load each program segment (ignores ph flags).
    ph     = ( struct proghdr* )(( uchar* )elf + elf->phoff);
    ph_end = ph + elf->phnum;
    for (; ph < ph_end; ph++) {
        phy_addr = ( uchar* )ph->paddr;
        readseg(phy_addr, ph->filesz, ph->off);
        if (ph->memsz > ph->filesz)    // 初始化 预留的数据段 或由于对齐 空着的无效数据
            stosb(phy_addr + ph->filesz, 0, ph->memsz - ph->filesz);
    }

    // Call the entry point from the ELF header.
    // Does not return!
    entry = (void (*)(void))(elf->entry);    // entry.S low address
    entry();
}

static void waitdisk(void) {    // Wait for disk ready.
    while ((inb(0x1F7) & 0xC0) != 0x40)
        ;
}

// Read a single sector at offset into dst.
static void readsector(void* dst, uint offset) {
    // Issue command.
    waitdisk();
    outb(0x1F2, 1);    // count = 1
    outb(0x1F3, offset);
    outb(0x1F4, offset >> 8);
    outb(0x1F5, offset >> 16);
    outb(0x1F6, (offset >> 24) | 0xE0);
    outb(0x1F7, 0x20);    // cmd 0x20 - read sectors

    // Read data.
    waitdisk();
    insl(0x1F0, dst, SECTOR_SIZE / 4);
}

// Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
// Might copy more than asked.
static void readseg(uchar* pa, uint count, uint offset) {
    uchar* epa;

    epa = pa + count;

    // Round down to sector boundary.
    pa -= offset % SECTOR_SIZE;

    // Translate from bytes to sectors; kernel starts at sector 1.
    offset = (offset / SECTOR_SIZE) + 1;

    // If this is too slow, we could read lots of sectors at a time.
    // We'd write more to memory than asked, but it doesn't matter --
    // we load in increasing order.
    for (; pa < epa; pa += SECTOR_SIZE, offset++)
        readsector(pa, offset);
}
