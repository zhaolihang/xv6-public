#ifndef __XV6_KEXTERN_DATA_H__
#define __XV6_KEXTERN_DATA_H__
#include "types.h"
#include "param.h"
#include "proc.h"
#include "file.h"


// The boot page table used in entry.S and entryother.S.
// Page directories (and page tables) must start on page boundaries,
// hence the __aligned__ attribute.
// PTE_PS in a page directory entry enables 4Mbyte pages.
extern pgtabe_t entry_page_directory[];    // For entry.S

extern uchar _binary_entryother_start[], _binary_entryother_size[];
extern char  kernel_end[];    // defined by  kernel.ld
extern char  data_start[];
extern uint  vectors[];    // in vectors.S: array of 256 entry pointers  vectors.S由脚本vectors.pl生成
//  _binary_initcode_start  _binary_initcode_size 是ld中把initcode.S文件编译成的二进制放在内核文件标号_binary_initcode_start处
extern char _binary_initcode_start[], _binary_initcode_size[];

extern struct cpu       cpus[MAX_CPU];
extern int              cpu_count;
extern struct device_rw device_rw[];    // 设备读写指针表

#endif
