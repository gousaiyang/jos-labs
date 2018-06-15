#include <kern/e1000.h>

// LAB 6: Your driver code here

struct e1000_tx_desc tx_queue[E1000_NTX] __attribute__((aligned(16)));
struct e1000_rx_desc rx_queue[E1000_NRX] __attribute__((aligned(16)));
char tx_bufs[E1000_NTX][TX_PKT_SIZE];
char rx_bufs[E1000_NRX][RX_PKT_SIZE];

int
e1000_attach(struct pci_func *pcif)
{
	pci_func_enable(pcif);
	boot_map_region(kern_pgdir, E1000_BASE, pcif->reg_size[0], pcif->reg_base[0], PTE_PCD | PTE_PWT | PTE_W);
	e1000_addr = (uint32_t *)E1000_BASE;
	cprintf("E1000 status = 0x%08x\n", e1000_addr[E1000_STATUS >> 2]);

	memset(tx_queue, 0, sizeof(tx_queue));
	memset(tx_bufs, 0, sizeof(tx_bufs));
	memset(rx_queue, 0, sizeof(rx_queue));
	memset(rx_bufs, 0, sizeof(rx_bufs));

	int i;
	for (i = 0; i < E1000_NTX; ++i) {
		tx_queue[i].buffer_addr = PADDR(tx_bufs[i]);
		tx_queue[i].upper.data |= E1000_TXD_STAT_DD;
	}
	for (i = 0; i < E1000_NRX; ++i)
		rx_queue[i].buffer_addr = PADDR(rx_bufs[i]);

	e1000_addr[E1000_TDBAL >> 2] = PADDR(tx_queue);
	e1000_addr[E1000_TDBAH >> 2] = 0;
	e1000_addr[E1000_TDLEN >> 2] = sizeof(tx_queue);
	e1000_addr[E1000_TDH >> 2] = 0;
	e1000_addr[E1000_TDT >> 2] = 0;
	e1000_addr[E1000_TCTL >> 2] |= E1000_TCTL_EN | E1000_TCTL_PSP | 0x40100;
	e1000_addr[E1000_TCTL >> 2] &= ~E1000_TCTL_CT & ~E1000_TCTL_COLD;
	e1000_addr[E1000_TIPG >> 2] = 0x60100a;

	e1000_addr[E1000_RAL0 >> 2] = 0x12005452;
	e1000_addr[E1000_RAH0 >> 2] = 0x5634 | E1000_RAH_AV;
	e1000_addr[E1000_RDBAL >> 2] = PADDR(rx_queue);
	e1000_addr[E1000_RDBAH >> 2] = 0;
	e1000_addr[E1000_RDLEN >> 2] = sizeof(rx_queue);
	e1000_addr[E1000_RDH >> 2] = 1;
	e1000_addr[E1000_RDT >> 2] = 0;
	e1000_addr[E1000_RCTL >> 2] = E1000_RCTL_EN;
	e1000_addr[E1000_RCTL >> 2] &= ~E1000_RCTL_LPE & ~E1000_RCTL_LBM_MASK & ~E1000_RCTL_RDMTS_MASK & ~E1000_RCTL_MO_MASK
		& ~E1000_RCTL_BSEX & ~E1000_RCTL_SZ_MASK;
	e1000_addr[E1000_RCTL >> 2] |= E1000_RCTL_BAM | E1000_RCTL_SZ_2048 | E1000_RCTL_SECRC;

	return 0;
}

int
e1000_transmit(char *data, unsigned len)
{
	if (!data || len > TX_PKT_SIZE)
		return -E_INVAL;

	uint32_t tdt = e1000_addr[E1000_TDT >> 2];
	if (!(tx_queue[tdt].upper.data & E1000_TXD_STAT_DD))
		return -E_TX_FULL;

	memset(tx_bufs[tdt], 0, sizeof(tx_bufs[tdt]));
	memmove(tx_bufs[tdt], data, len);
	tx_queue[tdt].lower.flags.length = len;
	tx_queue[tdt].lower.data |= E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
	tx_queue[tdt].upper.data &= ~E1000_TXD_STAT_DD;
	e1000_addr[E1000_TDT >> 2] = (tdt + 1) % E1000_NTX;

	return 0;
}

int
e1000_receive(char *data)
{
	if (!data)
		return -E_INVAL;

	uint32_t rdt = (e1000_addr[E1000_RDT >> 2] + 1) % E1000_NRX;

	if (!(rx_queue[rdt].status & E1000_RXD_STAT_DD))
		return -E_RX_EMPTY;
	if (!(rx_queue[rdt].status & E1000_RXD_STAT_EOP))
		return -E_RX_LONG;

	while (rdt == e1000_addr[E1000_RDH >> 2]);

	uint32_t len = rx_queue[rdt].length;
	memmove(data, rx_bufs[rdt], len);
	rx_queue[rdt].status &= ~E1000_RXD_STAT_DD & ~E1000_RXD_STAT_EOP;
	e1000_addr[E1000_RDT >> 2] = rdt;
	return len;
}
