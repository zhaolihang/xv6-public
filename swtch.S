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
# push eip  call 调用段内转移压如eip
.globl swtch
swtch:
  movl 4(%esp), %eax #  &(c->scheduler)
  movl 8(%esp), %edx #   p->context

  # Save old callee-save registers
  pushl %ebp
  pushl %ebx
  pushl %esi
  pushl %edi # 压入内核栈中

  # Switch stacks
  movl %esp, (%eax) # c->scheduler = esp 保存内核的 scheduler() context
  movl %edx, %esp   # esp = mycpu->context  用户进程的内核栈 mycpu->context最高处包含eip

  # Load new callee-save registers
  popl %edi
  popl %esi
  popl %ebx
  popl %ebp
  ret # 直接返回就切换到了 context *new 新进程中去执行  利用ret 进行进程切换进程首次运行返回到 forkret 函数
