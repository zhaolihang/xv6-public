// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "kextern_data.h"

static void freerange(void* vstart, void* vend);

struct linked_list_node {
    struct linked_list_node* next;
};

static struct {
    struct spinlock          lock;
    int                      use_lock;
    struct linked_list_node* freelist;
} kmem;

// Initialization happens in two phases.
// 1. main() calls init_kernel_mem_1() while still using entry_page_directory to place just
// the pages mapped by entry_page_directory on free list.
// 2. main() calls init_kernel_mem_2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void init_kernel_mem_1(void* vstart, void* vend) {
    initlock(&kmem.lock, "kmem");
    kmem.use_lock = 0;
    freerange(vstart, vend);
}

void init_kernel_mem_2(void* vstart, void* vend) {
    freerange(vstart, vend);
    kmem.use_lock = 1;
}

static void freerange(void* vstart, void* vend) {
    char* p = ( char* )PAGE_ROUNDUP(( uint )vstart);    //p2align
    for (; p + PAGE_SIZE <= ( char* )vend; p += PAGE_SIZE)
        kfree_page(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a call to kalloc_page().
// (The exception is when initializing the allocator; see kinit above.)
void kfree_page(char* v) {

    if (( uint )v % PAGE_SIZE || v < kernel_end || C_V2P(v) >= PHY_TOP_LIMIT)
        panic("kfree_page");

    struct linked_list_node* free_node = ( struct linked_list_node* )v;
    // memset(free_node, 1, PAGE_SIZE);    // Fill with junk to catch dangling refs.    // for performance

    if (kmem.use_lock)
        acquire(&kmem.lock);
    {
        free_node->next = kmem.freelist;
        kmem.freelist   = free_node;    // 插入到单项链表的开头
    }
    if (kmem.use_lock)
        release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char* kalloc_page(void) {
    struct linked_list_node* free_node;

    if (kmem.use_lock)
        acquire(&kmem.lock);
    {
        free_node = kmem.freelist;
        if (free_node)
            kmem.freelist = free_node->next;
    }
    if (kmem.use_lock)
        release(&kmem.lock);

    return ( char* )free_node;
}
