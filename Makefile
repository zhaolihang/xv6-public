## make参数“-n”或“--just-print”，那么其只是显示命令，但不会执行命令，这个功能很有利于我们调试我们的Makefile

# Using native tools (e.g., on X86 Linux)
#TOOLPREFIX = 

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if i386-jos-elf-objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1; \
	then echo 'i386-jos-elf-'; \
	elif objdump -i 2>&1 | grep 'elf32-i386' >/dev/null 2>&1; \
	then echo ''; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an i386-*-elf version of GCC/binutils." 1>&2; \
	echo "*** Is the directory with i386-jos-elf-gcc in your PATH?" 1>&2; \
	echo "*** If your i386-*-elf toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'i386-jos-elf-', set your TOOLPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

# If the makefile can't find QEMU, specify its path here
# QEMU = qemu-system-i386

# Try to infer the correct QEMU
ifndef QEMU
QEMU = $(shell if which qemu > /dev/null; \
	then echo qemu; exit; \
	elif which qemu-system-i386 > /dev/null; \
	then echo qemu-system-i386; exit; \
	elif which qemu-system-x86_64 > /dev/null; \
	then echo qemu-system-x86_64; exit; \
	else \
	qemu=/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu; \
	if test -x $$qemu; then echo $$qemu; exit; fi; fi; \
	echo "***" 1>&2; \
	echo "*** Error: Couldn't find a working QEMU executable." 1>&2; \
	echo "*** Is the directory containing the qemu binary in your PATH" 1>&2; \
	echo "*** or have you tried setting the QEMU variable in Makefile?" 1>&2; \
	echo "***" 1>&2; exit 1)
endif

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O2 -Wall -MD -m32 -Werror -fno-omit-frame-pointer -I. -nostdinc
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
ASFLAGS = -m32 -gdwarf-2 -Wa,-divide -I.
LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)

OBJS = \
	bio.o\
	console.o\
	exec.o\
	file.o\
	fs.o\
	ide.o\
	ioapic.o\
	kalloc.o\
	kbd.o\
	lapic.o\
	log.o\
	mp.o\
	picirq.o\
	pipe.o\
	proc.o\
	sleeplock.o\
	spinlock.o\
	string.o\
	swtch.o\
	syscall.o\
	sysfile.o\
	sysproc.o\
	trapasm.o\
	trap.o\
	uart.o\
	vectors.o\
	vm.o\
	entry/main.o\
	entry/entry.o\

#xv6.img
xv6.img: bootblock kernel fs.img   #执行make 会默认生成第一个目标
	dd if=/dev/zero of=xv6.img count=10000
	dd if=bootblock of=xv6.img conv=notrunc
	dd if=kernel of=xv6.img seek=1 conv=notrunc

#bootblock
bootblock: boot/bootasm.o boot/bootmain.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o boot/bootblock.o boot/bootasm.o boot/bootmain.o
	$(OBJDUMP) -S boot/bootblock.o > boot/bootblock.asm
	$(OBJCOPY) -S -O binary -j .text boot/bootblock.o bootblock
	./sign.pl bootblock

boot/bootmain.o: boot/bootmain.c
	$(CC) $(CFLAGS) -O -c boot/bootmain.c -o boot/bootmain.o # With ‘-O’, the compiler tries to reduce code size and execution time

boot/bootasm.o: boot/bootasm.S
	$(CC) $(CFLAGS) -c boot/bootasm.S -o boot/bootasm.o

#kernel
kernel: $(OBJS) entryother initcode kernel.ld 
	$(LD) $(LDFLAGS) -T kernel.ld -o kernel $(OBJS) -b binary entryother initcode
	$(OBJDUMP) -S kernel > kernel.asm
	$(OBJDUMP) -t kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym

vectors.S: vectors.pl
	perl vectors.pl > vectors.S

#entryother
entryother: entry/entryother.S
	$(CC) $(CFLAGS) -c entry/entryother.S -o entry/entryother.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o entry/bootblock_entryother.o entry/entryother.o
	$(OBJCOPY) -S -O binary -j .text entry/bootblock_entryother.o entryother
	$(OBJDUMP) -S entry/bootblock_entryother.o > entry/entryother.asm

#initcode
initcode: entry/initcode.S
	$(CC) $(CFLAGS) -c entry/initcode.S -o entry/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o entry/entry_initcode.o entry/initcode.o
	$(OBJCOPY) -S -O binary entry/entry_initcode.o initcode
	$(OBJDUMP) -S entry/initcode.o > entry/initcode.asm

ULIB = ulib.o usys.o printf.o umalloc.o

%_exec: %.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

# forktest has less library code linked in - needs to be small
# in order to be able to max out the proc table.
forktest_exec: forktest.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o forktest_exec forktest.o ulib.o usys.o
	$(OBJDUMP) -S forktest_exec > forktest.asm

UPROGS=\
	cat_exec\
	echo_exec\
	forktest_exec\
	grep_exec\
	init_exec\
	kill_exec\
	ln_exec\
	ls_exec\
	mkdir_exec\
	rm_exec\
	sh_exec\
	stressfs_exec\
	usertests_exec\
	wc_exec\
	zombie_exec\

mkfs: mkfs.c fs.h
	gcc -Werror -Wall -o mkfs mkfs.c

fs.img: mkfs README $(UPROGS)
	./mkfs fs.img README $(UPROGS)

# run in emulators
CPUS := 2
QEMUOPTS = -drive file=fs.img,index=1,media=disk,format=raw -drive file=xv6.img,index=0,media=disk,format=raw -smp $(CPUS) -m 512 $(QEMUEXTRA)
qemu: fs.img xv6.img
	$(QEMU) -serial mon:stdio $(QEMUOPTS)


-include *.d # 就算*.d 文件找不到也不报错 下次编译使用上次生成的.d文件加快依赖分析


clean: 
	rm -f \
	mkfs \
	*.o *.d *.asm *.sym *.log \
	*/*.o */*.d */*.asm */*.sym */*.log \
	bootblock entryother initcode kernel xv6.img fs.img \
	vectors.S \
	$(UPROGS)

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so that disk image changes after first build are persistent until clean
.PRECIOUS: %.o
# see http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html