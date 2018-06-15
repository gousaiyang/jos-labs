#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <inc/error.h>
#include <inc/string.h>
#include <kern/pmap.h>
#include <kern/pci.h>

#define E1000_BASE KSTACKTOP
#define E1000_STATUS 0x8
#define E1000_TDBAL 0x3800
#define E1000_TDBAH 0x3804
#define E1000_TDLEN 0x3808
#define E1000_TDH 0x3810
#define E1000_TDT 0x3818
#define E1000_TCTL 0x400
#define E1000_TIPG 0x410
#define E1000_TCTL_EN 0x2
#define E1000_TCTL_PSP 0x8
#define E1000_TCTL_CT 0xff0
#define E1000_TCTL_COLD 0x3ff000
#define E1000_TXD_STAT_DD 0x1
#define E1000_TXD_CMD_EOP 0x1000000
#define E1000_TXD_CMD_RS 0x8000000
#define E1000_NTX 64
#define TX_PKT_SIZE 1518

#define E1000_RA 0x5400
#define E1000_RAL0 (E1000_RA + 0)
#define E1000_RAH0 (E1000_RA + 4)
#define E1000_RAH_AV 0x80000000
#define E1000_RDBAL 0x2800
#define E1000_RDBAH 0x2804
#define E1000_RDLEN 0x2808
#define E1000_RDH 0x2810
#define E1000_RDT 0x2818
#define E1000_RCTL 0x100
#define E1000_RCTL_EN 0x2
#define E1000_RCTL_LPE 0x20
#define E1000_RCTL_LBM_MASK 0xC0
#define E1000_RCTL_RDMTS_MASK 0x300
#define E1000_RCTL_MO_MASK 0x7000
#define E1000_RCTL_BAM 0x8000
#define E1000_RCTL_BSEX 0x2000000
#define E1000_RCTL_SZ_MASK 0x30000
#define E1000_RCTL_SZ_2048 0x0
#define E1000_RCTL_SECRC 0x4000000
#define E1000_RXD_STAT_DD 0x1
#define E1000_RXD_STAT_EOP 0x2
#define E1000_NRX 128
#define RX_PKT_SIZE 2048

volatile uint32_t *e1000_addr;

struct e1000_tx_desc {
	uint64_t buffer_addr;
	union {
		uint32_t data;
		struct {
			uint16_t length;
			uint8_t cso;
			uint8_t cmd;
		} flags;
	} lower;
	union {
		uint32_t data;
		struct {
			uint8_t status;
			uint8_t css;
			uint16_t special;
		} fields;
	} upper;
};

struct e1000_rx_desc {
	uint64_t buffer_addr;
	uint16_t length;
	uint16_t csum;
	uint8_t status;
	uint8_t errors;
	uint16_t special;
};

int e1000_attach(struct pci_func *pcif);
int e1000_transmit(char *data, unsigned len);
int e1000_receive(char *data);

#endif	// JOS_KERN_E1000_H
