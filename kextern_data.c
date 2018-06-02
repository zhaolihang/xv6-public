#include "kextern_data.h"

#include "mmu.h"
#include "memlayout.h"



__attribute__((__aligned__(PAGE_SIZE))) pgtabe_t entry_page_directory[PAGE_DIR_TABLE_ENTRY_SIZE] = {
    // Map VA's [0, 4MB) to PA's [0, 4MB)
    [0] = (0) | PTE_P | PTE_W | PTE_PS,
    // Map VA's [VA_KERNAL_SPACE_BASE, VA_KERNAL_SPACE_BASE+4MB) to PA's [0, 4MB)
    [VA_KERNAL_SPACE_BASE >> PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};
