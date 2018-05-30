// Memory layout

#define EXTMEM 0x100000            // Start of extended memory
#define TOP_PHYSICAL 0xE000000     // Top physical memory
#define DEVICE_SPACE 0xFE000000    // Other devices are at high addresses

// Key addresses for address space layout (see kmap in vm.c for layout)
#define KERNBASE 0x80000000             // First kernel virtual address
#define KERNEND (KERNBASE + EXTMEM)    // Address where kernel is linked

#define V2P(a) (((uint)(a)) - KERNBASE)
#define P2V(a) ((( void* )(a)) + KERNBASE)

#define ASM_V2P(x) (( x )-KERNBASE)    // same as V2P, but without casts
#define ASM_P2V(x) ((x) + KERNBASE)    // same as P2V, but without casts
