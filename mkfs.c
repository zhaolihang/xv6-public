#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#define stat xv6_stat    // avoid clash with host struct stat
#include "types.h"
#include "fs.h"
#include "stat.h"
#include "param.h"

#ifndef static_assert
#define static_assert(a, b) do { switch (0) case 0 : case (a):; } while (0)
#endif

#define INODE_NUM 200

// Disk layout:
// [ boot block | sb block | log | inode blocks | free bit map | data blocks ]

int bitmap_num     = FSSIZE / (BLOCK_SIZE * 8) + 1;
int inodeblock_num = INODE_NUM / INODES_PER_BLOCK + 1;
int log_num        = LOGSIZE;
int meta_num;     // Number of meta blocks (boot, sb, log_num, inode, bitmap)
int block_num;    // Number of data blocks

int               fsfd;
struct superblock sb;
char              zeroes[BLOCK_SIZE];
uint              free_inode_index = 1;
uint              free_block_index;


void bitmap_alloc(int);
void write_sector(uint, void*);
void write_inode(uint, struct dinode*);
void read_inode(uint inum, struct dinode* ip);
void read_sector(uint sec, void* buf);
uint inode_alloc(ushort type);
void append_data_to_inode(uint inum, void* p, int n);

// convert to intel byte order
ushort xshort(ushort x) {
    ushort y;
    uchar* a = ( uchar* )&y;
    a[0]     = x;
    a[1]     = x >> 8;
    return y;
}

uint xint(uint x) {
    uint   y;
    uchar* a = ( uchar* )&y;
    a[0]     = x;
    a[1]     = x >> 8;
    a[2]     = x >> 16;
    a[3]     = x >> 24;
    return y;
}

bool str_start_with(const char* for_search, const char* search) {
    int for_search_size = strlen(for_search);
    int search_size     = strlen(search);
    if (for_search_size <= search_size) {
        return false;
    } else {
        int i;
        for (i = 0; i < search_size; i++) {
            if (*(for_search + i) != *(search + i)) {
                return false;
            }
        }
        return true;
    }
}

int main(int argc, char* argv[]) {
    int           i, count, fd;
    uint          root_inode, inum, off;
    struct direntry de;
    char          buf[BLOCK_SIZE];
    struct dinode disk_inode;


    static_assert(sizeof(int) == 4, "Integers must be 4 bytes!");

    if (argc < 2) {
        fprintf(stderr, "Usage: mkfs fs.img files...\n");
        exit(1);
    }

    assert((BLOCK_SIZE % sizeof(struct dinode)) == 0);
    assert((BLOCK_SIZE % sizeof(struct direntry)) == 0);

    fsfd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fsfd < 0) {
        perror(argv[1]);
        exit(1);
    }

    // 1 fs block = 1 disk sector
    meta_num  = 2 + log_num + inodeblock_num + bitmap_num;
    block_num = FSSIZE - meta_num;

    sb.size         = xint(FSSIZE);
    sb.block_num    = xint(block_num);
    sb.inode_num    = xint(INODE_NUM);
    sb.log_num      = xint(log_num);
    sb.log_start    = xint(2);
    sb.inode_start  = xint(2 + log_num);
    sb.bitmap_start = xint(2 + log_num + inodeblock_num);

    char* format = "meta_num %d (boot, super, log blocks %u inode blocks %u, bitmap blocks %u) blocks %d total %d\n";
    printf(format, meta_num, log_num, inodeblock_num, bitmap_num, block_num, FSSIZE);

    free_block_index = meta_num;    // the first free block that we can allocate

    for (i = 0; i < FSSIZE; i++)
        write_sector(i, zeroes);

    memset(buf, 0, sizeof(buf));
    memmove(buf, &sb, sizeof(sb));
    write_sector(1, buf);

    root_inode = inode_alloc(T_DIR);
    assert(root_inode == ROOT_INODE);

    bzero(&de, sizeof(de));
    de.inum = xshort(root_inode);
    strcpy(de.name, ".");
    append_data_to_inode(root_inode, &de, sizeof(de));

    bzero(&de, sizeof(de));
    de.inum = xshort(root_inode);
    strcpy(de.name, "..");
    append_data_to_inode(root_inode, &de, sizeof(de));    //  [.]  [..]  都指向自身

    for (i = 2; i < argc; i++) {
        assert(index(argv[i], '/') == 0);    // 不能包含 '/'

        if ((fd = open(argv[i], 0)) < 0) {
            perror(argv[i]);
            exit(1);
        }

        // Skip leading exec_ in name when writing to file system.
        // The binaries are named exec_rm, exec_cat, etc. to keep the
        // build operating system from trying to execute them
        // in place of system binaries like rm and cat.
        // exec_ 开头的文件
        if (str_start_with(argv[i], "exec_")) {
            argv[i] += 5;
        }

        inum = inode_alloc(T_FILE);

        bzero(&de, sizeof(de));
        de.inum = xshort(inum);
        strncpy(de.name, argv[i], DIR_NAME_MAX_SIZE);
        append_data_to_inode(root_inode, &de, sizeof(de));    // 在根目录下创建文件

        while ((count = read(fd, buf, sizeof(buf))) > 0)    // 追加文件内容
            append_data_to_inode(inum, buf, count);

        close(fd);
    }

    if (false) {
        // fix size of root inode dir    根节点为什么要对齐???
        read_inode(root_inode, &disk_inode);
        off             = xint(disk_inode.size);
        off             = ((off / BLOCK_SIZE) + 1) * BLOCK_SIZE;
        disk_inode.size = xint(off);
        write_inode(root_inode, &disk_inode);
    }

    bitmap_alloc(free_block_index);

    exit(0);
}

void write_sector(uint sec, void* buf) {
    if (lseek(fsfd, sec * BLOCK_SIZE, 0) != sec * BLOCK_SIZE) {
        perror("lseek");
        exit(1);
    }
    if (write(fsfd, buf, BLOCK_SIZE) != BLOCK_SIZE) {
        perror("write");
        exit(1);
    }
}

void write_inode(uint inum, struct dinode* ip) {
    char           buf[BLOCK_SIZE];
    uint           bn;
    struct dinode* dip;

    bn = INODE_TO_BLOCK(inum, sb);
    read_sector(bn, buf);
    dip  = (( struct dinode* )buf) + (inum % INODES_PER_BLOCK);
    *dip = *ip;
    write_sector(bn, buf);
}

void read_inode(uint inum, struct dinode* ip) {
    char           buf[BLOCK_SIZE];
    uint           bn;
    struct dinode* dip;

    bn = INODE_TO_BLOCK(inum, sb);
    read_sector(bn, buf);
    dip = (( struct dinode* )buf) + (inum % INODES_PER_BLOCK);
    *ip = *dip;
}

void read_sector(uint sec, void* buf) {
    if (lseek(fsfd, sec * BLOCK_SIZE, 0) != sec * BLOCK_SIZE) {
        perror("lseek");
        exit(1);
    }
    if (read(fsfd, buf, BLOCK_SIZE) != BLOCK_SIZE) {
        perror("read");
        exit(1);
    }
}

uint inode_alloc(ushort type) {
    uint          inum = free_inode_index++;
    struct dinode disk_inode;

    bzero(&disk_inode, sizeof(disk_inode));
    disk_inode.type  = xshort(type);
    disk_inode.nlink = xshort(1);
    disk_inode.size  = xint(0);
    write_inode(inum, &disk_inode);
    return inum;
}

void bitmap_alloc(int used) {
    uchar buf[BLOCK_SIZE];
    int   i;

    printf("bitmap_alloc: first %d blocks have been allocated\n", used);
    assert(used < BLOCK_SIZE * 8);
    bzero(buf, BLOCK_SIZE);
    for (i = 0; i < used; i++) {
        buf[i / 8] = buf[i / 8] | (0x1 << (i % 8));
    }
    printf("bitmap_alloc: write bitmap block at sector %d\n", sb.bitmap_start);
    write_sector(sb.bitmap_start, buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

void append_data_to_inode(uint inum, void* data, int n) {
    char*         data_p = ( char* )data;
    uint          file_block_num, off, count;
    struct dinode disk_inode;
    char          buf[BLOCK_SIZE];
    uint          indirect[FIRST_INDIRECT_NUM];
    uint          block_index;

    read_inode(inum, &disk_inode);
    off = xint(disk_inode.size);
    // printf("append inum %d at off %d sz %d\n", inum, off, n);
    while (n > 0) {
        file_block_num = off / BLOCK_SIZE;
        assert(file_block_num < FILE_MAX_BLOCKS);
        if (file_block_num < DIRECT_NUM) {
            if (xint(disk_inode.addrs[file_block_num]) == 0) {
                disk_inode.addrs[file_block_num] = xint(free_block_index++);
            }
            block_index = xint(disk_inode.addrs[file_block_num]);
        } else {
            if (xint(disk_inode.addrs[DIRECT_NUM]) == 0) {
                disk_inode.addrs[DIRECT_NUM] = xint(free_block_index++);
            }
            read_sector(xint(disk_inode.addrs[DIRECT_NUM]), ( char* )indirect);
            if (indirect[file_block_num - DIRECT_NUM] == 0) {
                indirect[file_block_num - DIRECT_NUM] = xint(free_block_index++);
                write_sector(xint(disk_inode.addrs[DIRECT_NUM]), ( char* )indirect);
            }
            block_index = xint(indirect[file_block_num - DIRECT_NUM]);
        }
        count = min(n, (file_block_num + 1) * BLOCK_SIZE - off);
        read_sector(block_index, buf);
        bcopy(data_p, buf + off - (file_block_num * BLOCK_SIZE), count);
        write_sector(block_index, buf);
        n -= count;
        off += count;
        data_p += count;
    }
    disk_inode.size = xint(off);
    write_inode(inum, &disk_inode);
}
