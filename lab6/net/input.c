#include "ns.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.

	int r, len;
	char buf[2048];

	while (1) {
		while ((r = sys_net_receive(buf)) < 0)
			if (r != -E_RX_EMPTY && r != -E_RX_LONG)
				panic("sys_net_receive: %e\n", r);

		len = r;

		if ((r = sys_page_alloc(0, &nsipcbuf, PTE_P | PTE_W | PTE_U)) < 0)
			panic("sys_page_alloc: %e\n", r);

		nsipcbuf.pkt.jp_len = len;
		memmove(nsipcbuf.pkt.jp_data, buf, len);

		if ((r = sys_ipc_try_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_P | PTE_W | PTE_U)) < 0)
			panic("sys_ipc_try_send: %e\n", r);
	}
}
