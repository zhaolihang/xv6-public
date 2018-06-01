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

static void wait_disk(void);
static void read_sector(void*, uint);
static void read_seg(uchar*, uint, uint);


void bootmain(void)    // 加载内核到物理地址 0x10000(64k) 并进入内核
{
    struct elfhdr*  elf;
    struct proghdr *prog_header, *prog_header_end;
    void (*entry)(void);
    uchar* phy_addr;

    elf = ( struct elfhdr* )0x10000;

    // Read 1st page off disk
    read_seg(( uchar* )elf, 4096, 0);    // 读取kernel文件的前4k到 地址0x10000(64k)

    // Is this an ELF executable?
    if (elf->magic != ELF_MAGIC)
        return;    // return bootasm.S

    // Load each program segment (ignores prog_header flags).
    prog_header     = ( struct proghdr* )(( uchar* )elf + elf->phoff);
    prog_header_end = prog_header + elf->phnum;
    for (; prog_header < prog_header_end; prog_header++) {
        phy_addr = ( uchar* )prog_header->paddr;
        read_seg(phy_addr, prog_header->filesz, prog_header->off);    // 加载到elf文件规定的物理地址中
        if (prog_header->memsz > prog_header->filesz)                 // 初始化 预留的数据段或由于对齐 的空数据
            stosb(phy_addr + prog_header->filesz, 0, prog_header->memsz - prog_header->filesz);
    }

    // Call the entry point from the ELF header.  Does not return!
    entry = (void (*)(void))(elf->entry);    // entry.S 中的 entry_start
    entry();
}


// 扇区从0开始
// Read 'count' bytes at 'offset' from kernel into physical address 'phy'.
// Might copy more than asked.
static void read_seg(uchar* phy, uint count, uint offset) {
    uchar* phy_end;
    uint   sector_index;
    phy_end = phy + count;

    // Round down to sector boundary.
    phy -= offset % SECTOR_SIZE;    // 获取 下边界

    // Translate from bytes to sectors; kernel starts at sector 1.
    sector_index = (offset / SECTOR_SIZE) + 1;    // +1 因为内核文件是从第1号扇区开始的

    // phy 是要加载到的物理地址 sector_index是第几号扇区
    for (; phy < phy_end; phy += SECTOR_SIZE, sector_index++)
        read_sector(phy, sector_index);
}

// Read a single sector at offset into dst.
static void read_sector(void* phy_dst, uint sector_index) {
    // Issue command.
    wait_disk();
    outb(0x1F2, 1);    // count = 1
    outb(0x1F3, sector_index);
    outb(0x1F4, sector_index >> 8);
    outb(0x1F5, sector_index >> 16);
    outb(0x1F6, (sector_index >> 24) | 0xE0);
    outb(0x1F7, 0x20);    // cmd 0x20 - read sectors

    // Read data.
    wait_disk();
    insl(0x1F0, phy_dst, SECTOR_SIZE / 4);
}

static void wait_disk(void) {    // Wait for disk ready.
    while ((inb(0x1F7) & 0xC0) != 0x40)
        ;
}
