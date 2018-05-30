#include "asm.h"
#include "memlayout.h"
#include "mmu.h"
	
# Each non-boot CPU ("AP") is started up in response to a STARTUP
# IPI from the boot CPU.  Section B.4.2 of the Multi-Processor
# Specification says that the AP will start in real mode with CS:IP
# set to XY00:0000, where XY is an 8-bit value sent with the
# STARTUP. Thus this code must start at a 4096-byte boundary.
#
# Because this code sets DS to zero, it must sit
# at an address in the low 2^16 bytes.
#
# Startothers (in main.c) sends the STARTUPs one at a time.
# It copies this code (start) at 0x7000.  It puts the address of
# a newly allocated per-core stack in start-4,the address of the
# place to jump to (mpenter) in start-8, and the physical address
# of entry_page_directory in start-12.
#
# This code combines elements of bootasm.S and entry.S.

.code16           
.globl start
start:
  cli #  ap cpu 相当于刚刚上电

  # Zero data segment registers DS, ES, and SS.
  xorw    %ax,%ax
  movw    %ax,%ds
  movw    %ax,%es
  movw    %ax,%ss

  # Switch from real to protected mode.  Use a bootstrap GDT that makes
  # virtual addresses map directly to physical addresses so that the
  # effective memory map doesn't change during the transition.
  lgdt    gdtdesc
  movl    %cr0, %eax
  orl     $CR0_PE, %eax
  movl    %eax, %cr0

  # Complete the transition to 32-bit protected mode by using a long jmp
  # to reload %cs and %eip.  The segment descriptors are set up with no
  # translation, so that the mapping is still the identity mapping.
  ljmpl    $(SEG_KCODE<<3), $(start32) #  刷新cs 寄存器 转到执行32位的代码

//PAGEBREAK!
.code32  # Tell assembler to generate 32-bit code now.
start32:
  # Set up the protected-mode data segment registers
  movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
  movw    %ax, %ds                # -> DS: Data Segment
  movw    %ax, %es                # -> ES: Extra Segment
  movw    %ax, %ss                # -> SS: Stack Segment
  movw    $0, %ax                 # Zero segments not ready for use
  movw    %ax, %fs                # -> FS
  movw    %ax, %gs                # -> GS   fs gs 这两个寄存器不使用

  # Turn on page size extension for 4Mbyte pages
  movl    %cr4, %eax
  orl     $(CR4_PSE), %eax # 开启4m的页
  movl    %eax, %cr4
  # Use entry_page_directory as our initial page table
  movl    (start-12), %eax   #    公共的entry_page_directory 的物理地址是由bsp 设置在物理地址 (0x7000-12) 处 
  movl    %eax, %cr3
  # Turn on paging.
  movl    %cr0, %eax
  orl     $(CR0_PE|CR0_PG|CR0_WP), %eax
  movl    %eax, %cr0 #  开启分页

  # Switch to the stack allocated by startothers()
  movl    (start-4), %esp  #  设置这个cpu的单独栈
  # Call mpenter()  in main.c
  call	 *(start-8)   #   mpenter函数 的虚拟地址是由bsp 设置在物理地址 (0x7000-8)处，  mpenter在内核的高地址处 这样就调转到了高地址处了

  movw    $0x8a00, %ax
  movw    %ax, %dx
  outw    %ax, %dx
  movw    $0x8ae0, %ax
  outw    %ax, %dx
spin:
  jmp     spin

.p2align 2
gdt:    #  在 bootasm.S for bsp  中有同样的数据
  ASM_SEG_NULL
  ASM_MAKE_SEG(APP_SEG_TYPE_X|APP_SEG_TYPE_R, 0, 0xffffffff)
  ASM_MAKE_SEG(APP_SEG_TYPE_W, 0, 0xffffffff)


gdtdesc:
  .word   (gdtdesc - gdt - 1)
  .long   gdt

