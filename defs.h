struct buf;
struct context;
struct file;
struct inode;
struct pipe;
struct proc;
struct rtcdate;
struct spinlock;
struct sleeplock;
struct stat;
struct superblock;

// bio.c
void        binit(void);
struct buf* bread(uint, uint);
void        brelse(struct buf*);
void        bwrite(struct buf*);

// console.c
void consoleinit(void);
void cprintf(char*, ...);
void consoleintr(int (*)(void));
void panic(char*) __attribute__((noreturn));

// exec.c
int exec(char*, char**);

// file.c
struct file* filealloc(void);
void         fileclose(struct file*);
struct file* filedup(struct file*);
void         fileinit(void);
int          fileread(struct file*, char*, int n);
int          filestat(struct file*, struct stat*);
int          filewrite(struct file*, char*, int n);

// fs.c
void          readsb(int dev, struct superblock* sb);
int           dirlink(struct inode*, char*, uint);
struct inode* dirlookup(struct inode*, char*, uint*);
struct inode* ialloc(uint, short);
struct inode* idup(struct inode*);
void          iinit(int dev);
void          ilock(struct inode*);
void          iput(struct inode*);
void          iunlock(struct inode*);
void          iunlockput(struct inode*);
void          iupdate(struct inode*);
int           namecmp(const char*, const char*);
struct inode* namei(char*);
struct inode* nameiparent(char*, char*);
int           readi(struct inode*, char*, uint, uint);
void          stati(struct inode*, struct stat*);
int           writei(struct inode*, char*, uint, uint);

// ide.c
void ideinit(void);
void ideintr(void);
void iderw(struct buf*);

// ioapic.c
void         ioapicenable(int irq, int cpu);
extern uchar ioapicid;
void         ioapicinit(void);

// kalloc_page.c
char* kalloc_page(void);
void  kfree_page(char*);
void  init_kernel_mem_1(void*, void*);
void  init_kernel_mem_2(void*, void*);

// kbd.c
void kbdintr(void);

// lapic.c
void                  cmostime(struct rtcdate* r);
int                   lapicid(void);
void                  lapiceoi(void);
void                  lapicinit(void);
void                  lapicstartap(uchar, uint);
void                  microdelay(int);
extern volatile uint* lapicaddr;
void                  init_lapicaddr(uint*);


// log.c
void initlog(int dev);
void log_write(struct buf*);
void begin_op();
void end_op();

// mp.c
void mpinit(void);

// picirq.c
void picenable(int);
void picinit(void);

// pipe.c
int  pipealloc(struct file**, struct file**);
void pipeclose(struct pipe*, int);
int  piperead(struct pipe*, char*, int);
int  pipewrite(struct pipe*, char*, int);

// proc.c
int          cpuid(void);
void         exit(void);
int          fork(void);
int          growproc(int);
int          kill(int);
struct cpu*  mycpu(void);
struct proc* myproc();
void         pinit(void);
void         procdump(void);
void         scheduler(void) __attribute__((noreturn));
void         sched(void);
void         setproc(struct proc*);
void         sleep(void*, struct spinlock*);
void         userinit(void);
int          wait(void);
void         wakeup(void*);
void         yield(void);

// swtch.S
void swtch(struct context**, struct context*);

// spinlock.c
void acquire(struct spinlock*);
void getcallerpcs(void*, uint*);
int  holding(struct spinlock*);
void initlock(struct spinlock*, char*);
void release(struct spinlock*);
void pushcli(void);
void popcli(void);

// sleeplock.c
void acquiresleep(struct sleeplock*);
void releasesleep(struct sleeplock*);
int  holdingsleep(struct sleeplock*);
void initsleeplock(struct sleeplock*, char*);

// string.c
int   memcmp(const void*, const void*, uint);
void* memmove(void*, const void*, uint);
void* memset(void*, int, uint);
char* safestrcpy(char*, const char*, int);
int   strlen(const char*);
int   strncmp(const char*, const char*, uint);
char* strncpy(char*, const char*, int);

// syscall.c
int  argint(int, int*);
int  argptr(int, char**, int);
int  argstr(int, char**);
int  fetchint(uint, int*);
int  fetchstr(uint, char**);
void syscall(void);

// timer.c
void timerinit(void);

// trap.c
void                   idt_init(void);
extern uint            ticks;
void                   tvinit(void);
extern struct spinlock tickslock;

// uart.c
void uartinit(void);
void uartintr(void);
void uartputc(int);

// vm.c
void      seginit(void);
void      init_kvm_pgdir(void);
pgtabe_t* alloc_kvm_pgdir(void);
char*     uva2ka(pgtabe_t*, char*);
int       allocuvm(pgtabe_t*, uint, uint);
int       deallocuvm(pgtabe_t*, uint, uint);
void      freevm(pgtabe_t*);
void      init_initcode_uvm(pgtabe_t*, char*, uint);
int       loaduvm(pgtabe_t*, char*, struct inode*, uint, uint);
pgtabe_t* copyuvm(pgtabe_t*, uint);
void      switch2uvm(struct proc*);
void      switch2kvm(void);
int       copyout(pgtabe_t*, uint, void*, uint);
void      clearpteu(pgtabe_t* pgdir, char* uva);

// number of elements in fixed-size array
#define SIZEOF_ARRAY(x) (sizeof(x) / sizeof((x)[0]))
