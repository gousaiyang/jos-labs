# Lab 4: Preemptive Multitasking

## Design Details

- For the address mapping of all the physical memory at `KERNBASE`, switch to small pages to avoid trouble with memory-mapped I/O.
- In `sys_exofork()`, the `env_break` pointer should also be copied.
- We should reconstruct our code to accept system calls both from trap `T_SYSCALL` and the `sysenter` instruction. We should take care of locking the big kernel lock at the appropriate time, passing the fifth parameter of `syscall()` and the trapframe.
- We should set all `istrap` fields in `SETGATE()` to 0 to disable interrupt when handling traps/syscalls/interrupts in kernel.
- We should enable interrupt before the `sysexit` instruction.

## Exercise & Question Answers

Q1: Compare `kern/mpentry.S` side by side with `boot/boot.S`. Bearing in mind that `kern/mpentry.S` is compiled and linked to run above `KERNBASE` just like everything else in the kernel, what is the purpose of the macro `MPBOOTPHYS`? Why is it necessary in `kern/mpentry.S` but not in `boot/boot.S`? In other words, what could go wrong if it were omitted in `kern/mpentry.S`?
Hint: recall the differences between the link address and the load address that we have discussed in Lab 1.

A1: `kern/mpentry.S` needs `MPBOOTPHYS` to calculate the absolute physical address of its GDT (because paging on this AP has not been turned on yet), which cannot be done by the linker.

Q2: It seems that using the big kernel lock guarantees that only one CPU can run the kernel code at a time. Why do we still need separate kernel stacks for each CPU? Describe a scenario in which using a shared kernel stack will go wrong, even with the protection of the big kernel lock.

A2: Suppose CPU 0 and CPU 1 are both executing user programs. Later CPU 0 enters the kernel mode (and locks the big kernel lock), and then the kernel stack will be in use. But after that an interrupt may happen on CPU 1, pushing several arguments onto the kernel stack, without checking the big kernel lock. This would damage some information that CPU 0 was using.

Q3: In your implementation of `env_run()` you should have called `lcr3()`. Before and after the call to `lcr3()`, your code makes references (at least it should) to the variable `e`, the argument to `env_run()`. Upon loading the `%cr3` register, the addressing context used by the MMU is instantly changed. But a virtual address (namely `e`) has meaning relative to a given address context--the address context specifies the physical address to which the virtual address maps. Why can the pointer `e` be dereferenced both before and after the addressing switch?

A3: Because `e` is in kernel address space (kernel stack), the mapping of which remains the same after the page table reload.

## Challenge

I choose to implement the challenge about IPC: to reimplement the IPC system calls so that `ipc_send()` will not have to loop.

To start with, some new fields should be added to the `Env` structure to store some necessary information:

```c
envid_t env_ipc_pending_envid;	// dst envid of the pending message to send of this env (0 -> none) (for challenge)
uint32_t env_ipc_pending_value;	// value of the pending message to send of this env (for challenge)
struct Page *env_ipc_pending_page;	// page of the pending message to send of this env (for challenge)
int env_ipc_pending_perm;	// perm of the pending message to send of this env (for challenge)
```

Generally, the new implementation works as follows:

- If the sender recognizes that the receiver is ready, it directly sends information into the receiver's `Env` structure, wakes up the receiver and returns.
- If the sender recognizes that the receiver is not ready, it records necessary information in its own `Env` structure and `yield`s to wait for the receiver to receive the message.
- If the receiver finds a pending message sent to it, it receives the message, wakes up the sender and returns.
- If the receiver finds no pending message sent to it, it `yield`s to wait for a message.

NOTE: We should be careful to clear the `env_ipc_pending_envid` when a new environment is allocated in `env_alloc()`.
