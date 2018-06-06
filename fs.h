#ifndef __XV6_FS_H__
#define __XV6_FS_H__
// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOT_INODE 1      // root i-number
#define BLOCK_SIZE 512    // block size

// Disk layout:
// [ boot block | super block | log | inode blocks | free bit map | data blocks]
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
    uint size;            // Size of file system image (blocks)
    uint block_num;       // Number of data blocks
    uint inode_num;       // Number of inodes.
    uint log_num;         // Number of log blocks
    uint log_start;       // Block number of first log block
    uint inode_start;     // Block number of first inode block
    uint bitmap_start;    // Block number of first free map block
};

#define DIRECT_NUM 12
#define FIRST_INDIRECT_NUM (BLOCK_SIZE / sizeof(uint))
#define SECOND_INDIRECT_NUM (FIRST_INDIRECT_NUM * FIRST_INDIRECT_NUM)
#define THIRD_INDIRECT_NUM (4 * FIRST_INDIRECT_NUM * FIRST_INDIRECT_NUM * FIRST_INDIRECT_NUM)
#define FILE_MAX_BLOCKS (DIRECT_NUM + FIRST_INDIRECT_NUM + SECOND_INDIRECT_NUM + THIRD_INDIRECT_NUM)

#define INODE_VERSION_32 0
#define INODE_VERSION_64 1
// On-disk inode structure
struct dinode {
    short version;                  // version type
    short type;                     // File type
    short major;                    // Major device number (T_DEV only)
    short minor;                    // Minor device number (T_DEV only)
    uint  nlink;                    // Number of links to inode in file system
    uint  size;                     // Size of file (bytes)
    uint  creattime;                // timestamp
    uint  visittime;                // timestamp
    uint  writetime;                // timestamp
    uint  user;                     // user
    uint  group;                    // group
    uint  permission;               // permission
    uint  addrs[DIRECT_NUM + 6];    // Data block addresses
    // uint  first_in_addrs[1];     // first block addresses
    // uint  second_in_addrs[1];    // second block addresses
    // uint  third_in_addrs[4];     // third block addresses
    uint reserved[4];
    //  DIRECT_NUM个直接块  1、2索引各一个、3级索引4个 每个文件最多可以有 FILE_MAX_BLOCKS*512≈4GB
};

// Inodes per block.
#define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(struct dinode))

// Block containing inode i
#define INODE_TO_BLOCK(i, sb) ((i) / INODES_PER_BLOCK + sb.inode_start)

// Bitmap bits per block
#define BITMAP_BITS_PER_BLOCK (BLOCK_SIZE * 8)

// Block of free map containing bit for block b
#define BITMAP_TO_BLOCK(b, sb) (b / BITMAP_BITS_PER_BLOCK + sb.bitmap_start)

// Directory is a file containing a sequence of direntry structures.
#define DIR_NAME_MAX_SIZE 252

struct direntry {
    uint inum;
    char name[DIR_NAME_MAX_SIZE];
};

#endif
