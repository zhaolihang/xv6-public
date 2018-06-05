#ifndef __XV6_PARAM_H__
#define __XV6_PARAM_H__

#define NPROC           64      // maximum number of processes
#define KSTACK_SIZE     4096    // size of per-process kernel stack
#define MAX_CPU         8   // maximum number of CPUs
#define NOFILE          16  // open files per process
#define NFILE           100 // open files per system
#define NINODE          50  // maximum number of active i-nodes
#define NDEV            10  // maximum major device number
#define ROOT_DEVICE         1   // device number of file system root disk
#define MAXARG          32  // max exec arguments
#define MAXOPBLOCKS     10  // max # of blocks any FS op writes
#define LOGSIZE         (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define BUFFER_SIZE     (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE          1000    // size of file system in blocks

#endif