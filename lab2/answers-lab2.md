# Lab 2: Memory Management

## Exercise & Question Answers

Q1: Assuming that the following JOS kernel code is correct, what type should variable `x` have, `uintptr_t` or `physaddr_t`?

```c
mystery_t x;
char* value = return_a_pointer();
*value = 10;
x = (mystery_t) value;
```

A1: It should be `uintptr_t` (representing a virtual address). All C programs deal with virtual addresses after we get into protected mode.

Q2: What entries (rows) in the page directory have been filled in at this point? What addresses do they map and where do they point? In other words, fill out this table as much as possible.

A2:

|Entry|Base Virtual Address|Points to (logically)|
|-|-|-|
|1023|0xffc00000|Page table for remapped whole physical memory|
|...|...|Page table for remapped whole physical memory|
|960|0xf0000000|Page table for remapped whole physical memory|
|959|0xef7c0000|?|
|958|0xef800000|Page table for kernel stack|
|957|0xef400000|Current page table|
|956|0xef000000|Page table for `struct Page`s|
|...|...|?|
|1|0x00400000|?|
|0|0x00000000|?|

Q3: We have placed the kernel and user environment in the same address space. Why will user programs not be able to read or write the kernel's memory? What specific mechanisms protect the kernel memory?

A3: We use permission bits in page tables to achieve this. Pages without the PTE_U bit set are not accessible by user programs.

Q4: What is the maximum amount of physical memory that this operating system can support? Why?

A4: 256 MB. JOS needs to map all physical memory at virtual memory [0xf0000000, 2^32) in order to manipulate them. There is only 256 MB space in this range.

Q5: How much space overhead is there for managing memory, if we actually had the maximum amount of physical memory? How is this overhead broken down?

A5:

- Physical memory management: Using the `struct Page` structure and `page_free_list`.
- Virtual memory management: Using page tables. Use multilevel page tables (i.e. page directories and page tables) to reduce overhead.

Q6: Revisit the page table setup in `kern/entry.S` and `kern/entrypgdir.c`. Immediately after we turn on paging, EIP is still a low number (a little over 1MB). At what point do we transition to running at an EIP above KERNBASE? What makes it possible for us to continue executing at a low EIP between when we enable paging and when we begin running at an EIP above KERNBASE? Why is this transition necessary?

A6: Transition point in `kern/entry.S`:

```assembly
# Now paging is enabled, but we're still running at a low EIP
# (why is this okay?).  Jump up above KERNBASE before entering
# C code.
mov	$relocated, %eax
jmp	*%eax
```

In `kern/entrypgdir.c`, `entry_pgdir` maps VA's [0, 4MB) to PA's [0, 4MB), and VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB). So when paging is enabled, the old physical address value is still a valid virtual address, which enables the entry program to run smoothly.

## Challenge - Extend the JOS kernel monitor with new commands

I've implemented some new monitor commands as follows.

### `vmmap`

Display memory mapping information.

```
Usage: vmmap                    show all vmmaps
       vmmap [va]               show vmmap of virtual page containing va
       vmmap [vastart] [vaend]  show all vmmaps in range [vastart, vaend)
```

```
K> vmmap
0xef000000-0xef000fff -> 0x0011b000-0x0011bfff 4K r-u
0xef001000-0xef001fff -> 0x0011c000-0x0011cfff 4K r-u
...
0xef3fe000-0xef3fefff -> 0x00519000-0x00519fff 4K r-u
0xef3ff000-0xef3fffff -> 0x0051a000-0x0051afff 4K r-u
0xef7bc000-0xef7bcfff -> 0x003fd000-0x003fdfff 4K rwu
0xef7bd000-0xef7bdfff -> 0x0011a000-0x0011afff 4K r-u
0xef7be000-0xef7befff -> 0x003fe000-0x003fefff 4K rwu
0xef7c0000-0xefbbffff -> 0x00000000-0x003fffff 4M rws
0xefbf8000-0xefbf8fff -> 0x0010f000-0x0010ffff 4K rws
0xefbf9000-0xefbf9fff -> 0x00110000-0x00110fff 4K rws
...
0xefbfe000-0xefbfefff -> 0x00115000-0x00115fff 4K rws
0xefbff000-0xefbfffff -> 0x00116000-0x00116fff 4K rws
0xf0000000-0xf03fffff -> 0x00000000-0x003fffff 4M rws
0xf0400000-0xf07fffff -> 0x00400000-0x007fffff 4M rws
...
0xff800000-0xffbfffff -> 0x0f800000-0x0fbfffff 4M rws
0xffc00000-0xffffffff -> 0x0fc00000-0x0fffffff 4M rws
K> vmmap 0xf0000001
0xf0000000-0xf03fffff -> 0x00000000-0x003fffff 4M rws
K> vmmap 0xf0000010 0xf0800001
0xf0000000-0xf03fffff -> 0x00000000-0x003fffff 4M rws
0xf0400000-0xf07fffff -> 0x00400000-0x007fffff 4M rws
0xf0800000-0xf0bfffff -> 0x00800000-0x00bfffff 4M rws
```

### `setpgperm`

Set page permissions.

```
Usage: setpgperm [va] [u|s|r|w]
```

```
K> vmmap 0xf0000000
0xf0000000-0xf03fffff -> 0x00000000-0x003fffff 4M rws
K> setpgperm 0xf0000000 u
K> vmmap 0xf0000000
0xf0000000-0xf03fffff -> 0x00000000-0x003fffff 4M rwu
K> setpgperm 0xf0000000 r
K> vmmap 0xf0000000
0xf0000000-0xf03fffff -> 0x00000000-0x003fffff 4M r-u
```

### `pgdir`

Display page directory address.

```
K> pgdir
Page directory virtual address = 0xf011a000, physical address = 0x0011a000
```

### `cr`

Display control registers.

```
K> cr
cr0 = 0x80050033, cr2 = 0x00000000, cr3 = 0x0011a000, cr4 = 0x00000010
```

### `dumpv`

Dump data at given virtual memory.

```
Usage: dumpv [start] [length]
```

```
K> dumpv 0xf0000000 30
53 ff 00 f0 53 ff 00 f0 c3 e2 00 f0 53 ff 00 f0 53 ff 00 f0
54 ff 00 f0 53 ff 00 f0 53 ff
```

### `dumpp`

Dump data at given physical memory.

```
Usage: dumpp [start] [length]
```

```
K> dumpp 0 30
53 ff 00 f0 53 ff 00 f0 c3 e2 00 f0 53 ff 00 f0 53 ff 00 f0
54 ff 00 f0 53 ff 00 f0 53 ff
```

### `loadv`

Modify a byte at given virtual memory.

```
Usage: loadv [va] [byte]
```

```
K> dumpv 0xf0000000 10
53 ff 00 f0 53 ff 00 f0 c3 e2
K> loadv 0xf0000000 0xcc
K> dumpv 0xf0000000 10
cc ff 00 f0 53 ff 00 f0 c3 e2
```

### `loadp`

Modify a byte at given physical memory.

```
Usage: loadp [pa] [byte]
```

```
K> dumpp 0 10
cc ff 00 f0 53 ff 00 f0 c3 e2
K> loadp 0 0x53
K> dumpp 0 10
53 ff 00 f0 53 ff 00 f0 c3 e2
```
