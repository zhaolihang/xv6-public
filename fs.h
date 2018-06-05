#ifndef __XV6_FS_H__
#define __XV6_FS_H__
// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOT_INODE 1         // root i-number
#define BLOCK_SIZE 512    // block size

// Disk layout:
// [ boot block | super block | log | inode blocks | free bit map | data blocks]
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
    uint size;          // Size of file system image (blocks)
    uint block_num;       // Number of data blocks
    uint inode_num;       // Number of inodes.
    uint log_num;          // Number of log blocks
    uint log_start;      // Block number of first log block
    uint inode_start;    // Block number of first inode block
    uint bitmap_start;     // Block number of first free map block
};

#define DIRECT_NUM 12
#define INDIRECT_NUM (BLOCK_SIZE / sizeof(uint))
#define FILE_MAX_BLOCKS (DIRECT_NUM + INDIRECT_NUM)

// On-disk inode structure
struct dinode {
    short type;                     // File type
    short major;                    // Major device number (T_DEV only)
    short minor;                    // Minor device number (T_DEV only)
    short nlink;                    // Number of links to inode in file system
    uint  size;                     // Size of file (bytes)
    uint  addrs[DIRECT_NUM + 1];    // Data block addresses  
    //  DIRECT_NUM个直接块  1个间接块  每个文件最多可以有 DIRECT_NUM+512/4  = 140  140*512=71680 bits (70k)
};

// Inodes per block.
#define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(struct dinode))

// Block containing inode i
#define INODE_TO_BLOCK(i, sb) ((i) / INODES_PER_BLOCK + sb.inode_start)

// Bitmap bits per block
#define BITMAP_BITS_PER_BLOCK (BLOCK_SIZE * 8)

// Block of free map containing bit for block b
#define BITMAP_TO_BLOCK(b, sb) (b / BITMAP_BITS_PER_BLOCK + sb.bitmap_start)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
    ushort inum;
    char   name[DIRSIZ];
};

#endif
