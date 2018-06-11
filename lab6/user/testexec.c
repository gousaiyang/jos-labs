#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int r;

	cprintf("testexec: Hello, world!\n");

	cprintf("testexec: going to exec /init\n");
	if ((r = execl("/init", "init", "initarg1", "initarg2", (char*)0)) < 0)
		panic("testexec: exec /init: %e\n", r);

	cprintf("testexec: Oh no!!! I should have already died!!!\n");
}
