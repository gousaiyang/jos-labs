// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line
#define MAX_HEX_LINE 20 // Maximum hex bytes to display in a line.


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display stack backtrace", mon_backtrace},
	{ "time", "Measure CPU cycles that a command costs", mon_time},
	{ "vmmap", "Display memory mapping information", mon_vmmap},
	{ "setpgperm", "Set page permissions", mon_setpgperm},
	{ "pgdir", "Display page directory address", mon_pgdir},
	{ "cr", "Display control registers", mon_cr},
	{ "dumpv", "Dump data at given virtual memory", mon_dumpv},
	{ "dumpp", "Dump data at given physical memory", mon_dumpp},
	{ "loadv", "Modify a byte at given virtual memory", mon_loadv},
	{ "loadp", "Modify a byte at given physical memory", mon_loadp},
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

unsigned read_eip();

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		(end-entry+1023)/1024);
	return 0;
}

// Lab1 only
// read the pointer to the retaddr on the stack
static uint32_t
read_pretaddr() {
    uint32_t pretaddr;
    __asm __volatile("leal 4(%%ebp), %0" : "=r" (pretaddr));
    return pretaddr;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	cprintf("Stack backtrace:\n");

	uint32_t* ebp = (uint32_t *)read_ebp();
	unsigned eip = read_eip();
	struct Eipdebuginfo info;

	while (ebp) {
		cprintf("  eip %08x  ebp %08x  args %08x %08x %08x %08x %08x\n", eip, ebp, ebp[2], ebp[3], ebp[4], ebp[5], ebp[6]);
		debuginfo_eip(eip, &info);
		cprintf("         %s:%d: %s+%d\n", info.eip_file, info.eip_line, info.eip_fn_name, eip - info.eip_fn_addr);
		ebp = (uint32_t *)ebp[0]; // Upper level ebp.
		eip = ebp[1]; // Upper level eip.
	}

    cprintf("Backtrace success\n");
	return 0;
}

int
mon_time(int argc, char **argv, struct Trapframe *tf)
{
	if (argc < 2) {
		cprintf("No command specified\n");
		return 0;
	}

	int (*func)(int argc, char** argv, struct Trapframe* tf);
	func = NULL;

	int i;
	for (i = 0; i < NCOMMANDS; i++)
		if (strcmp(argv[1], commands[i].name) == 0)
			func = commands[i].func;

	if (!func) {
		cprintf("Unknown command '%s'\n", argv[1]);
		return 0;
	}

	uint64_t start_time = read_tsc();
	int ret = func(argc - 1, argv + 1, tf);
	uint64_t end_time = read_tsc();
	cprintf("%s cycles: %llu\n", argv[1], end_time - start_time);

	return ret;
}

// Check whether a virtual address has been mapped.
static int
va_mapped(uintptr_t va)
{
	pte_t *pte = pgdir_walk_large(kern_pgdir, (void *)va, 0);
	return pte && (*pte & PTE_P);
}

// Return start address of the virtual page containing va.
static uintptr_t
page_start(uintptr_t va)
{
	pte_t *pte = pgdir_walk_large(kern_pgdir, (void *)va, 0);
	if (!pte || !(*pte & PTE_P)) // Mapping not present, regard as small page.
		return ROUNDDOWN(va, PGSIZE);

	return ROUNDDOWN(va, *pte & PTE_PS ? PTSIZE : PGSIZE);
}

// Return end address of the virtual page containing va.
static uintptr_t
page_end(uintptr_t va)
{
	pte_t *pte = pgdir_walk_large(kern_pgdir, (void *)va, 0);
	if (!pte || !(*pte & PTE_P)) // Mapping not present, regard as small page.
		return ROUNDUP(va, PGSIZE);

	return ROUNDUP(va, *pte & PTE_PS ? PTSIZE : PGSIZE);
}

// Return size of the virtual page containing va.
static size_t
current_page_size(uintptr_t va)
{
	pte_t *pte = pgdir_walk_large(kern_pgdir, (void *)va, 0);
	if (!pte || !(*pte & PTE_P)) // Mapping not present, regard as small page.
		return PGSIZE;

	return *pte & PTE_PS ? PTSIZE : PGSIZE;
}

// Print mapping information of a virtual page starting at va.
static void
print_mapping(uintptr_t va)
{
	pte_t *pte = pgdir_walk_large(kern_pgdir, (void *)va, 0);
	if (!pte || !(*pte & PTE_P))
		return;

	size_t size = *pte & PTE_PS ? PTSIZE : PGSIZE;

	cprintf("%08p-%08p -> %08p-%08p ", va, va + size - 1, PTE_ADDR(*pte), PTE_ADDR(*pte) + size - 1);

	if (size == PTSIZE)
		cprintf("4M");
	else
		cprintf("4K");

	cprintf(" ");

	if (*pte & PTE_W)
		cprintf("rw");
	else
		cprintf("r-");

	if (*pte & PTE_U)
		cprintf("u");
	else
		cprintf("s");

	cprintf("\n");
}

int
mon_vmmap(int argc, char **argv, struct Trapframe *tf)
{
	uintptr_t va, vastart, vaend;

	if (argc == 1) {
		va = 0;
		do {
			print_mapping(va);
			va += current_page_size(va);
		} while (va);
	} else if (argc == 2) {
		va = (uintptr_t)strtol(argv[1], NULL, 0);
		print_mapping(page_start(va));
	} else if (argc == 3) {
		// Get input parameters.
		vastart = (uintptr_t)strtol(argv[1], NULL, 0);
		vaend = (uintptr_t)strtol(argv[2], NULL, 0);

		// Check valid virtual address range.
		if (vastart > vaend && vaend) {
			cprintf("Invalid virtual address range\n");
			return 0;
		}

		// Extend input range to page boundary.
		vastart = page_start(vastart);
		vaend = page_end(vaend);

		// Print mapping information of pages in range.
		va = vastart;
		do {
			print_mapping(va);
			va += current_page_size(va);
		} while (va < vaend || (!vaend && va));
	} else {
		cprintf("Usage: vmmap                    show all vmmaps\n");
		cprintf("       vmmap [va]               show vmmap of virtual page containing va\n");
		cprintf("       vmmap [vastart] [vaend]  show all vmmaps in range [vastart, vaend)\n");
	}

	return 0;
}

int
mon_setpgperm(int argc, char **argv, struct Trapframe *tf)
{
	if (argc == 3) {
		uintptr_t va = (uintptr_t)strtol(argv[1], NULL, 0);

		// Check valid virtual address mapping.
		pte_t *pte = pgdir_walk_large(kern_pgdir, (void *)va, 0);
		if (!pte || !(*pte & PTE_P)) {
			cprintf("Virtual address %08p is not mapped\n", va);
			return 0;
		}

		// Check new perm.
		if (strlen(argv[2]) != 1 || (argv[2][0] != 'u' && argv[2][0] != 's' && argv[2][0] != 'r' && argv[2][0] != 'w')) {
			cprintf("Invalid perm, should be one of u/s/r/w\n");
			return 0;
		}

		// Change perm.
		switch (argv[2][0]) {
			case 'u':
				*pte |= PTE_U;
				break;
			case 's':
				*pte &= ~PTE_U;
				break;
			case 'r':
				*pte &= ~PTE_W;
				break;
			case 'w':
				*pte |= PTE_W;
				break;
		}
	} else {
		cprintf("Usage: setpgperm [va] [u|s|r|w]\n");
	}

	return 0;
}

int
mon_pgdir(int argc, char **argv, struct Trapframe *tf)
{
	cprintf("Page directory virtual address = %08p, physical address = %08p\n", kern_pgdir, PADDR(kern_pgdir));
	return 0;
}

int
mon_cr(int argc, char **argv, struct Trapframe *tf)
{
	cprintf("cr0 = 0x%08x, cr2 = 0x%08x, cr3 = 0x%08x, cr4 = 0x%08x\n", rcr0(), rcr2(), rcr3(), rcr4());
	return 0;
}

int
mon_dumpv(int argc, char **argv, struct Trapframe *tf)
{
	uintptr_t va, vastart, vaend;
	size_t length;
	int i;

	if (argc == 3) {
		// Get input parameters.
		vastart = (uintptr_t)strtol(argv[1], NULL, 0);
		length = (size_t)strtol(argv[2], NULL, 0);
		vaend = vastart + length;

		// Check valid virtual address range.
		if (vastart > vaend && vaend) {
			cprintf("Range overflow\n");
			return 0;
		}

		// Check valid virtual address mapping.
		vastart = page_start(vastart);
		vaend = page_end(vaend);

		va = vastart;
		do {
			if (!va_mapped(va)) {
				cprintf("Virtual address %08p is not mapped\n", va);
				return 0;
			}
			va += current_page_size(va);
		} while (va < vaend || (!vaend && va));

		// Print hex dump.
		for (i = 0; i < length; ++i) {
			cprintf("%02x ", *(unsigned char *)(vastart + i));
			if ((i + 1) % MAX_HEX_LINE == 0)
				cprintf("\n");
		}

		if (length % MAX_HEX_LINE != 0)
			cprintf("\n");
	} else {
		cprintf("Usage: dumpv [start] [length]\n");
	}

	return 0;
}

int
mon_dumpp(int argc, char **argv, struct Trapframe *tf)
{
	if (argc == 3) {
		// Get input parameters.
		physaddr_t pastart = (physaddr_t)strtol(argv[1], NULL, 0);
		size_t length = (size_t)strtol(argv[2], NULL, 0);

		// Check valid physical address range.
		if (pastart >= npages * PGSIZE || pastart + length > npages * PGSIZE || pastart + length < pastart) {
			cprintf("Range exceeds physical memory\n");
			return 0;
		}

		// Print hex dump.
		int i;
		for (i = 0; i < length; ++i) {
			cprintf("%02x ", *((unsigned char *)KADDR(pastart) + i));
			if ((i + 1) % MAX_HEX_LINE == 0)
				cprintf("\n");
		}

		if (length % MAX_HEX_LINE != 0)
			cprintf("\n");
	} else {
		cprintf("Usage: dumpp [start] [length]\n");
	}

	return 0;
}

int
mon_loadv(int argc, char **argv, struct Trapframe *tf)
{
	if (argc == 3) {
		// Get input parameters.
		uintptr_t va = (uintptr_t)strtol(argv[1], NULL, 0);
		unsigned char data = (unsigned char)strtol(argv[2], NULL, 0);

		// Check valid virtual address mapping.
		if (!va_mapped(va)) {
			cprintf("Virtual address %08p is not mapped\n", va);
			return 0;
		}

		// Modify byte data.
		*(unsigned char *)va = data;
	} else {
		cprintf("Usage: loadv [va] [byte]\n");
	}

	return 0;
}

int
mon_loadp(int argc, char **argv, struct Trapframe *tf)
{
	if (argc == 3) {
		// Get input parameters.
		physaddr_t pa = (physaddr_t)strtol(argv[1], NULL, 0);
		unsigned char data = (unsigned char)strtol(argv[2], NULL, 0);

		// Check valid physical address.
		if (pa >= npages * PGSIZE) {
			cprintf("Address exceeds physical memory\n");
			return 0;
		}

		// Modify byte data.
		*(unsigned char *)KADDR(pa) = data;
	} else {
		cprintf("Usage: loadp [pa] [byte]\n");
	}

	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

// return EIP of caller.
// does not work if inlined.
// putting at the end of the file seems to prevent inlining.
unsigned
read_eip()
{
	uint32_t callerpc;
	__asm __volatile("movl 4(%%ebp), %0" : "=r" (callerpc));
	return callerpc;
}
