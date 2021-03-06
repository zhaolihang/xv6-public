#ifndef __XV6_PROC_H__
#define __XV6_PROC_H__

#include "mmu.h"
#include "types.h"
#include "param.h"
#include "x86.h"
#include "file.h"

struct proc;
// Per-CPU state
struct cpu {
    uchar            apicid;       // Local APIC ID
    struct context*  scheduler;    // swtch() here to enter scheduler   // scheduler函数中的context 寄存器状态
    struct taskstate tss;          // Used by x86 to find stack for interrupt  tss 在用户程序中发生中断的时候需要找到0特权级的栈
    struct segdesc   gdt[SEG_SIZE];    // x86 global descriptor table
    volatile uint    started;          // Has the CPU started?
    int              ncli;             // Depth of pushcli nesting.
    int              intena;           // Were interrupts enabled before pushcli?
    struct proc*     proc;             // The process running on this cpu or null // cpu 正在运行的进程
};

// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
    uint edi;    // 低地址 在栈顶
    uint esi;
    uint ebx;
    uint ebp;
    uint eip;    // 高地址 在栈底
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
    uint              sz;               // Size of process memory (bytes)
    pgtabe_t*         pgdir;            // Page directory table
    char*             kstack;           // Bottom of kernel stack for this process
    enum procstate    state;            // Process state
    int               pid;              // Process ID
    struct proc*      parent;           // Parent process
    struct trapframe* tf;               // Trap frame for current syscall
    struct context*   context;          // swtch() here to run process
    void*             chan;             // If non-zero, sleeping on chan
    int               killed;           // If non-zero, have been killed
    struct file*      ofile[NOFILE];    // Open files
    struct inode*     cwd;              // Current directory
    char              name[16];         // Process name (debugging)
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

#endif
