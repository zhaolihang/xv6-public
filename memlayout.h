#ifndef __XV6_MEMLAYOUT_H__
#define __XV6_MEMLAYOUT_H__

#include "types.h"
// Memory layout

#define EXTMEM 0x100000            // Start of extended memory
#define TOP_PHYSICAL 0xE000000     // Top physical memory
#define DEVICE_SPACE 0xFE000000    // Other devices are at high addresses

// Key addresses for address space layout (see kmap in vm.c for layout)
#define KERNAL_SPACE_BASE 0x80000000                       // First kernel virtual address
#define KERNAL_LINKED_BASE (KERNAL_SPACE_BASE + EXTMEM)    // Address where kernel is linked

#define V2P(a) (((uint)(a)) - KERNAL_SPACE_BASE)
#define P2V(a) ((( void* )(a)) + KERNAL_SPACE_BASE)

#define ASM_V2P(x) (( x )-KERNAL_SPACE_BASE)    // same as V2P, but without casts
#define ASM_P2V(x) ((x) + KERNAL_SPACE_BASE)    // same as P2V, but without casts

#endif    // #ifndef __XV6_MEMLAYOUT_H__
