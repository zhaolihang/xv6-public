# Initial process execs /init.
# This code runs in user space.

#include "syscall.h"
#include "traps.h"


# exec(init, argv)
# push argv ;  push init ;  push eip ;
.globl start
start:
  pushl $argv   #  below      .long init   .long 0
  pushl $init   #  below     .string "/init\0"
  pushl $0      # where caller eip would be  // 这三句模拟c语言函数调用
  movl $SYS_exec, %eax #  功能号
  int $T_SYSCALL   # 系统调用 64

# for(;;) exit();
exit:
  movl $SYS_exit, %eax
  int $T_SYSCALL
  jmp exit

# char init[] = "/init\0"# 
init:
  .string "/init\0"

# char *argv[] = { init, 0 }# 
.p2align 2
argv:
  .long init
  .long 0

