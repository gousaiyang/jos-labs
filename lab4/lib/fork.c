// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

// Assembly language pgfault entrypoint defined in lib/pfentry.S.
extern void _pgfault_upcall(void);

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *)utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.

	cprintf("pgfault: fault_va = %p by env %08x, eip = %p, esp = %p\n", addr, sys_getenvid(), utf->utf_eip, utf->utf_esp); // ###

	if (!(err & FEC_WR))
		panic("pgfault: faulting access was not a write\n");
	if (!(vpd[PDX(addr)] & PTE_P) || ((vpt[PGNUM(addr)] & (PTE_P | PTE_COW)) != (PTE_P | PTE_COW)))
		panic("pgfault: faulting access was no to a copy-on-write page\n");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.

	if ((r = sys_page_alloc(0, PFTEMP, PTE_P | PTE_U | PTE_W)) < 0)
		panic("sys_page_alloc: %e\n", r);

	addr = ROUNDDOWN(addr, PGSIZE);
	memmove(PFTEMP, addr, PGSIZE);

	if ((r = sys_page_map(0, PFTEMP, 0, addr, PTE_P | PTE_U | PTE_W)) < 0)
		panic("sys_page_map: %e\n", r);

	if ((r = sys_page_unmap(0, PFTEMP)) < 0)
		panic("sys_page_unmap: %e\n", r);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
// duppage(envid_t envid, unsigned pn)
duppage(envid_t dstenv, void *addr)
{
	// LAB 4: Your code here.

	// int r;
	// uintptr_t addr = pn * PGSIZE;
	//
	// if (vpt[pn] & (PTE_W | PTE_COW)) {
	// 	if ((r = sys_page_map(0, (void *)addr, envid, (void *)addr, PTE_P | PTE_U | PTE_COW)) < 0)
	// 		panic("sys_page_map: %e\n", r);
	//
	// 	if ((r = sys_page_map(0, (void *)addr, 0, (void *)addr, PTE_P | PTE_U | PTE_COW)) < 0)
	// 		panic("sys_page_map: %e\n", r);
	// } else {
	// 	if ((r = sys_page_map(0, (void *)addr, envid, (void *)addr, PTE_P | PTE_U)) < 0)
	// 		panic("sys_page_map: %e\n", r);
	// }
	//
	// return 0;
	int r;

	// This is NOT what you should do in your fork.
	if ((r = sys_page_alloc(dstenv, addr, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);
	if ((r = sys_page_map(dstenv, addr, 0, UTEMP, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_map: %e", r);
	memmove(UTEMP, addr, PGSIZE);
	if ((r = sys_page_unmap(0, UTEMP)) < 0)
		panic("sys_page_unmap: %e", r);

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.

	// envid_t envid;
	// uintptr_t addr;
	// int r;
	//
	// set_pgfault_handler(pgfault);
	//
	// envid = sys_exofork();
	// if (envid < 0)
	// 	panic("sys_exofork: %e\n", envid);
	//
	// if (envid == 0) { // Child
	// 	thisenv = &envs[ENVX(sys_getenvid())];
	// 	return 0;
	// }
	//
	// for (addr = 0; addr < UTOP; addr += PGSIZE)
	// 	if ((vpd[PDX(addr)] & PTE_P) && (vpt[PGNUM(addr)] & PTE_P) && addr != UXSTACKTOP - PGSIZE)
	// 		duppage(envid, PGNUM(addr));
	//
	// if ((r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W)) < 0)
	// 	panic("sys_page_alloc: %e\n", r);
	//
	// if ((r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall)) < 0)
	// 	panic("sys_env_set_pgfault_upcall: %e\n", r);
	//
	// if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
	// 	panic("sys_env_set_status: %e\n", r);
	//
	// return envid;

	envid_t envid;
	uint8_t *addr;
	int r;
	extern unsigned char end[];

	// Allocate a new child environment.
	// The kernel will initialize it with a copy of our register state,
	// so that the child will appear to have called sys_exofork() too -
	// except that in the child, this "fake" call to sys_exofork()
	// will return 0 instead of the envid of the child.
	envid = sys_exofork();
	if (envid < 0)
		panic("sys_exofork: %e", envid);
	if (envid == 0) {
		// We're the child.
		// The copied value of the global variable 'thisenv'
		// is no longer valid (it refers to the parent!).
		// Fix it and return 0.
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	// We're the parent.
	// Eagerly copy our entire address space into the child.
	// This is NOT what you should do in your fork implementation.
	for (addr = (uint8_t*) UTEXT; addr < end; addr += PGSIZE)
		duppage(envid, addr);

	// Also copy the stack we are currently running on.
	duppage(envid, ROUNDDOWN(&addr, PGSIZE));

	// Start the child environment running
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
