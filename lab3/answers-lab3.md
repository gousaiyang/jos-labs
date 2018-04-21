# Lab 3: User Environments

## Design Details

- In `env_setup_vm()`, we can copy page table entries above `UTOP` directly from `kern_pgdir`.
- In `load_icode()`, we should set `tf_eip` and `env_break` properly.
- In `env_create()`, we should load the program using the page directory of its environment.
- In the lib `syscall()` function:
  - We should store return `%esp` to `%ebp`, and store return pc to `%esi` before the `sysenter` instruction.
  - We should use `%=` to generate a unique label number to resolve assembler errors.
- In `sysenter_handler()`:
  - We push 0 as the 5th parameter of the kernel `syscall()` function.
  - We should set `%edx` to return pc (stored in `%esi`), and set `%ecx` to return `%esp` (stored in `%ebp`) before the `sysexit` instruction.
- While implementing `sys_sbrk()`:
  - We should add `env_break` field to `struct Env` to keep track of the brk pointer of an environment.
  - We should align `inc` to whole pages.
  - We should prevent heap address range from overflowing to kernel.
- In `trap_dispatch()`, we should dispatch both `T_DEBUG` and `T_BRKPT` to invoke the kernel monitor.
- In `mon_c()`, we should clear the trap flag in `EFLAGS`. In `mon_si()`, we should set the trap flag in `EFLAGS`. In both functions, we should return -1 to quit the kernel monitor.
- When handling page faults, `(tf->tf_cs & 0x3) == 0` indicates that we are in kernel mode.

## Exercise & Question Answers

Q1: What is the purpose of having an individual handler function for each exception/interrupt? (i.e. if all exceptions/interrupts were delivered to the same handler, what feature that exists in the current implementation could not be provided?)

A1: If all exceptions/interrupts were delivered to the same handler, we can no longer distinguish different kinds of them. But actually we need to perform different actions when different exceptions/interrupts occur, therefore we need individual handlers for each exception/interrupt.

Q2: Did you have to do anything to make the user/softint program behave correctly? The grade script expects it to produce a general protection fault (trap 13), but softint's code says `int $14`. Why should this produce interrupt vector 13? What happens if the kernel actually allows softint's `int $14` instruction to invoke the kernel's page fault handler (which is interrupt vector 14)?

A2: I did not have to do more to make the user/softint program behave correctly. Executing `int $14` in user mode will produce a general protection fault because user programs are not allowed to produce interrupt of number 14 (page fault) actively (in `trap_init()`, the `dpl` of `T_PGFLT` is set to 0). If the kernel actually allows this behavior, the environment will be destroyed (in the kernel page fault handler) and the user program will terminate.

Q3: The break point test case will either generate a break point exception or a general protection fault depending on how you initialized the break point entry in the IDT (i.e. your call to `SETGATE()` from `trap_init()`). Why? How do you need to set it up in order to get the breakpoint exception to work as specified above and what incorrect setup would cause it to trigger a general protection fault?

A3: In `trap_init()`, the `dpl` of `T_BRKPT` is set to 3. This allows user programs to produce breakpoint exceptions. In contrast, if it is set to 0, a general protection fault will be generated when user programs execute `int $3`.

Q4: What do you think is the point of these mechanisms, particularly in light of what the user/softint test program does?

A4: This helps to prevent bugs (or malicious behaviors) of user programs from causing kernel malfunctions, which makes the operating system more robust and secure.
