# Context switch
#
#   void swtch(struct context **old, struct context *new);
# 
# Save the current registers on the stack, creating
# a struct context, and save its address in *old.
# Switch stacks to new and pop previously-saved registers.

# void swtch(struct context **old, struct context *new)
# push new 
# push old
# push eip   #call调用段内转移压入eip
.globl swtch
swtch:
  movl 4(%esp), %eax #  &(c->scheduler)
  movl 8(%esp), %edx #   p->context

  # Save old callee-save registers
  pushl %ebp
  pushl %ebx
  pushl %esi
  pushl %edi # 压入内核栈中 构建一个context结构体

  # Switch stacks
  movl %esp, (%eax) # c->scheduler = esp 设置scheduler函数的context
  movl %edx, %esp   # esp = p->context 切换到用户进程的内核栈(0特权级栈) p->context 最高处包含进程当前的eip

  # Load new callee-save registers
  popl %edi
  popl %esi
  popl %ebx
  popl %ebp
  ret      # popl %eip; 利用ret进行eip切换, 进程首次会返回到forkret函数
