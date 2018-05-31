#ifndef __XV6_MEMLAYOUT_H__
#define __XV6_MEMLAYOUT_H__

#include "types.h"
// Memory layout

#define PHY_EXTMEM_BASE 0x100000      // 1m Start of extended memory
#define PHY_TOP_LIMIT 0xE000000       // Top physical memory
#define PHY_DEVICE_BASE 0xFE000000    // Other devices are at high addresses

// Key addresses for address space layout (see kmap in vm.c for layout)
#define VA_KERNAL_SPACE_BASE 0x80000000                                   // First kernel virtual address
#define VA_KERNAL_LINKED_BASE (VA_KERNAL_SPACE_BASE + PHY_EXTMEM_BASE)    // Address where kernel is linked

#define C_V2P(a) (((uint)(a)) - VA_KERNAL_SPACE_BASE)
#define C_P2V(a) ((( void* )(a)) + VA_KERNAL_SPACE_BASE)

#define ASM_V2P(x) (( x )-VA_KERNAL_SPACE_BASE)    // same as C_V2P, but without casts
#define ASM_P2V(x) ((x) + VA_KERNAL_SPACE_BASE)    // same as C_P2V, but without casts

#endif    // #ifndef __XV6_MEMLAYOUT_H__
