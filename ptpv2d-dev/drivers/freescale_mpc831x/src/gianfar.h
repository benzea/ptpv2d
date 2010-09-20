/*
 * drivers/net/gianfar.h
 *
 * Gianfar Ethernet Driver
 * Driver for FEC on MPC8540 and TSEC on MPC8540/MPC8560
 * Based on 8260_io/fcc_enet.c
 *
 * Author: Andy Fleming
 * Maintainer: Kumar Gala
 *
 * Copyright (c) 2002-2004 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 *  Still left to do:
 *      -Add support for module parameters
 *	-Add patch for ethtool phys id
 */

/****************************************************************************/
/* Begin copyright and licensing information, do not remove                 */
/*                                                                          */
/* This file (gianfar.h) contains Modifications (updates, corrections       */
/* comments and addition of initial support for IEEE 1588 version 1, IEEE   */
/* version 2 and IEEE 802.1AS PTP) by Alan K. Bartky <alan@bartky.net>      */
/*                                                                          */
/* Copyright (c) 2007-2008 by Alan K. Bartky                                */
/*                                                                          */
/* These modifications and their associated software algorithms are under   */
/* copyright and for this file are licensed under the terms of the GNU      */
/* General Public License as published by the Free Software Foundation;     */
/* either version 2 of the License, or (at your option) any later version.  */
/*                                                                          */
/*     /\        This file and/or data from this file is copyrighted and    */
/*    /| \       is provided under a software license.                      */
/*   / | /\                                                                 */
/*  /__|/  \     This notice is to be included in all derivative works      */
/*  \  /\  /\                                                               */
/*   \/  \/  \   For copyright and alternate licensing information contact: */
/*    \  /\  /     Alan K. Bartky                                           */
/*     \/  \/      Bartky Networks                                          */
/*      \  /       Web: http://www.bartky.net                               */
/*       \/        email: alan@bartky.net                                   */
/*                                                                          */
/* End Bartky Networks copyright notice: Do not remove                      */
/****************************************************************************/


#ifndef __GIANFAR_H
#define __GIANFAR_H

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/mii.h>
#include <linux/phy.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/crc32.h>
#include <linux/workqueue.h>
#include <linux/ethtool.h>
#include <linux/netdevice.h>
#include <linux/fsl_devices.h>
#include "gianfar_mii.h"

/* The maximum number of packets to be handled in one call of gfar_poll */
#define GFAR_DEV_WEIGHT 16

/* Length for FCB */
#define GMAC_FCB_LEN 8

/* Default padding amount */
#define DEFAULT_PADDING 2

/* Number of bytes to align the rx bufs to */
#define RXBUF_ALIGNMENT 64
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
#define GFAR_SKB_USER_OPT_HEADROOM 16
#endif

/* The number of bytes which composes a unit for the purpose of
 * allocating data buffers.  ie-for any given MTU, the data buffer
 * will be the next highest multiple of 512 bytes. */
#define INCREMENTAL_BUFFER_SIZE 512


#define MAC_ADDR_LEN 6

#define PHY_INIT_TIMEOUT 100000
#define GFAR_PHY_CHANGE_TIME 2

#ifndef CONFIG_GFAR_SKBUFF_RECYCLING
#define DEVICE_NAME "%s: Gianfar Ethernet Controller Version 1.2, "
#else
#define DEVICE_NAME "%s: Gianfar Ethernet Controller Version 1.4, "
#endif
#define DRV_NAME "gfar-enet"
extern const char gfar_driver_name[];
extern const char gfar_driver_version[];

/* These need to be powers of 2 for this driver */
#ifdef CONFIG_GFAR_NAPI
#define DEFAULT_TX_RING_SIZE	64
#define DEFAULT_RX_RING_SIZE	64
#else
#define DEFAULT_TX_RING_SIZE    64
#define DEFAULT_RX_RING_SIZE    64
#endif

#define GFAR_RX_MAX_RING_SIZE   256
#define GFAR_TX_MAX_RING_SIZE   256

#define GFAR_MAX_FIFO_THRESHOLD 511
#define GFAR_MAX_FIFO_STARVE	511
#define GFAR_MAX_FIFO_STARVE_OFF 511

#define DEFAULT_RX_BUFFER_SIZE  1536
#define TX_RING_MOD_MASK(size) (size-1)
#define RX_RING_MOD_MASK(size) (size-1)
#define JUMBO_BUFFER_SIZE 9728
#define JUMBO_FRAME_SIZE 9600

#define DEFAULT_FIFO_TX_THR 0x80
#define DEFAULT_FIFO_TX_STARVE 0x40
#define DEFAULT_FIFO_TX_STARVE_OFF 0x80
#define DEFAULT_BD_STASH 1
#define DEFAULT_STASH_LENGTH	64
#define DEFAULT_STASH_INDEX	0

#ifdef CONFIG_GFAR_PTP
#define PTP_GET_RX_TIMESTAMP_SYNC       SIOCDEVPRIVATE
#define PTP_GET_RX_TIMESTAMP_DEL_REQ    (SIOCDEVPRIVATE + 1)
#define PTP_GET_RX_TIMESTAMP_PDEL_REQ   (SIOCDEVPRIVATE + 2) // AKB: Changed for version 2
#define PTP_GET_RX_TIMESTAMP_PDEL_RESP  (SIOCDEVPRIVATE + 3) // AKB: Changed for version 2
#define PTP_GET_TX_TIMESTAMP            (SIOCDEVPRIVATE + 4)
#define PTP_SET_CNT                     (SIOCDEVPRIVATE + 5)
#define PTP_GET_CNT                     (SIOCDEVPRIVATE + 6)
#define PTP_ADJ_FREQ                    (SIOCDEVPRIVATE + 7)
#define PTP_ADJ_ADDEND                  (SIOCDEVPRIVATE + 8)
#define PTP_ADJ_ADDEND_IXXAT            (SIOCDEVPRIVATE + 9)
#define PTP_GET_ADDEND                  (SIOCDEVPRIVATE + 10)
#define PTP_SET_ALARM1			(SIOCDEVPRIVATE + 11)
#define PTP_SET_FIPER1			(SIOCDEVPRIVATE + 12)
#define PTP_SET_PERIOD			(SIOCDEVPRIVATE + 13) // AKB: Added to change clock period
#define DEFAULT_PTP_RX_BUF_SZ           2000
#define GFAR_PTP_MSG_TYPE_SYNC          0x0
#define GFAR_PTP_MSG_TYPE_DEL_REQ       0x1
#define GFAR_PTP_MSG_TYPE_FOLLOWUP      0x2
#define GFAR_PTP_MSG_TYPE_DEL_RESP      0x3
#define GFAR_PTP2_MSG_TYPE_PDEL_REQ     0x2	// AKB: Version 2 path delay request
#define GFAR_PTP2_MSG_TYPE_PDEL_RESP    0x3	// AKB: Version 2 path delay response
#endif

/* The number of Exact Match registers */
#define GFAR_EM_NUM	15

/* Latency of interface clock in nanoseconds */
/* Interface clock latency , in this case, means the
 * time described by a value of 1 in the interrupt
 * coalescing registers' time fields.  Since those fields
 * refer to the time it takes for 64 clocks to pass, the
 * latencies are as such:
 * GBIT = 125MHz => 8ns/clock => 8*64 ns / tick
 * 100 = 25 MHz => 40ns/clock => 40*64 ns / tick
 * 10 = 2.5 MHz => 400ns/clock => 400*64 ns / tick
 */
#define GFAR_GBIT_TIME  512
#define GFAR_100_TIME   2560
#define GFAR_10_TIME    25600

#define DEFAULT_TX_COALESCE 1
#define DEFAULT_TXCOUNT	24
#define DEFAULT_TXTIME	64

#define DEFAULT_RX_COALESCE 1
#define DEFAULT_RXCOUNT	2
#define DEFAULT_RXTIME	64

#define TBIPA_VALUE		0x1f
#define MIIMCFG_INIT_VALUE	0x00000007
#define MIIMCFG_RESET           0x80000000
#define MIIMIND_BUSY            0x00000001

/* TBI register addresses (not defined in linux/mii.h) */
#define MII_TBICON		0x11

/* TBICON register bit fields */
#define TBICON_CLK_SELECT	0x0020

/* MAC register bits */
#define MACCFG1_SOFT_RESET	0x80000000
#define MACCFG1_RESET_RX_MC	0x00080000
#define MACCFG1_RESET_TX_MC	0x00040000
#define MACCFG1_RESET_RX_FUN	0x00020000
#define	MACCFG1_RESET_TX_FUN	0x00010000
#define MACCFG1_LOOPBACK	0x00000100
#define MACCFG1_RX_FLOW		0x00000020
#define MACCFG1_TX_FLOW		0x00000010
#define MACCFG1_SYNCD_RX_EN	0x00000008
#define MACCFG1_RX_EN		0x00000004
#define MACCFG1_SYNCD_TX_EN	0x00000002
#define MACCFG1_TX_EN		0x00000001

#define MACCFG2_INIT_SETTINGS	0x00007205
#define MACCFG2_FULL_DUPLEX	0x00000001
#define MACCFG2_IF              0x00000300
#define MACCFG2_MII             0x00000100
#define MACCFG2_GMII            0x00000200
#define MACCFG2_HUGEFRAME	0x00000020
#define MACCFG2_LENGTHCHECK	0x00000010
#define MACCFG2_MPEN		0x00000008 // Magic Packet Enable, 831x only

#define ECNTRL_INIT_SETTINGS	0x00001000
#define ECNTRL_TBI_MODE         0x00000020
#define ECNTRL_REDUCED_MODE	0x00000010
#define ECNTRL_R100		0x00000008
#define ECNTRL_REDUCED_MII_MODE	0x00000004
#define ECNTRL_SGMII_MODE	0x00000002

#define MRBLR_INIT_SETTINGS	DEFAULT_RX_BUFFER_SIZE

#define MINFLR_INIT_SETTINGS	0x00000040

/* Init to do tx snooping for buffers and descriptors */
#define DMACTRL_INIT_SETTINGS   0x000000c3
#define DMACTRL_GRS             0x00000010
#define DMACTRL_GTS             0x00000008

#define TSTAT_CLEAR_THALT       0x80000000

/* Interrupt coalescing macros */
#define IC_ICEN			0x80000000
#define IC_ICFT_MASK		0x1fe00000
#define IC_ICFT_SHIFT		21
#define mk_ic_icft(x)		\
	(((unsigned int)x << IC_ICFT_SHIFT)&IC_ICFT_MASK)
#define IC_ICTT_MASK		0x0000ffff
#define mk_ic_ictt(x)		(x&IC_ICTT_MASK)

#define mk_ic_value(count, time) (IC_ICEN | \
				mk_ic_icft(count) | \
				mk_ic_ictt(time))

#define RCTRL_TS_ENABLE 	0x01000000
#define RCTRL_PADB_SIZE		(0x8 << 16)
#define RCTRL_PAL_MASK		0x001f0000
#define RCTRL_VLEX		0x00002000
#define RCTRL_FSQEN		0x00000800
#define RCTRL_FILREN		0x00001000
#define RCTRL_GHTX		0x00000400
#define RCTRL_IPCSEN		0x00000200
#define RCTRL_TUCSEN		0x00000100
#define RCTRL_PRSDEP_MASK	0x000000c0
#define RCTRL_PRSDEP_INIT	0x000000c0
#define RCTRL_PROM		0x00000008
#define RCTRL_EMEN		0x00000002
#define RCTRL_CHECKSUMMING	(RCTRL_IPCSEN \
		| RCTRL_TUCSEN | RCTRL_PRSDEP_INIT)
#define RCTRL_EXTHASH		(RCTRL_GHTX)
#define RCTRL_VLAN		(RCTRL_PRSDEP_INIT)
#define RCTRL_PADDING(x)	((x << 16) & RCTRL_PAL_MASK)


#define RSTAT_CLEAR_RHALT       0x00800000

#define TCTRL_IPCSEN		0x00004000
#define TCTRL_TUCSEN		0x00002000
#define TCTRL_VLINS		0x00001000
#define TCTRL_INIT_CSUM		(TCTRL_TUCSEN | TCTRL_IPCSEN)

#define IEVENT_INIT_CLEAR	0xffffffff
#define IEVENT_BABR		0x80000000
#define IEVENT_RXC		0x40000000
#define IEVENT_BSY		0x20000000
#define IEVENT_EBERR		0x10000000
#define IEVENT_MSRO		0x04000000
#define IEVENT_GTSC		0x02000000
#define IEVENT_BABT		0x01000000
#define IEVENT_TXC		0x00800000
#define IEVENT_TXE		0x00400000
#define IEVENT_TXB		0x00200000
#define IEVENT_TXF		0x00100000
#define IEVENT_LC		0x00040000
#define IEVENT_CRL		0x00020000
#define IEVENT_XFUN		0x00010000
#define IEVENT_RXB0		0x00008000
#define IEVENT_MAG		0x00000800 // 831x only
#define IEVENT_GRSC		0x00000100
#define IEVENT_RXF0		0x00000080
#define IEVENT_FIR		0x00000008
#define IEVENT_FIQ		0x00000004
#define IEVENT_DPE		0x00000002
#define IEVENT_PERR		0x00000001
#define IEVENT_RX_MASK          (IEVENT_RXB0 | IEVENT_RXF0)
#define IEVENT_TX_MASK          (IEVENT_TXB | IEVENT_TXF)
#define IEVENT_ERR_MASK         \
(IEVENT_RXC | IEVENT_BSY | IEVENT_EBERR | IEVENT_MSRO | \
 IEVENT_BABT | IEVENT_TXC | IEVENT_TXE | IEVENT_LC \
 | IEVENT_CRL | IEVENT_XFUN | IEVENT_DPE | IEVENT_PERR | \
 IEVENT_MAG)

#define IMASK_INIT_CLEAR	0x00000000
#define IMASK_BABR              0x80000000
#define IMASK_RXC               0x40000000
#define IMASK_BSY               0x20000000
#define IMASK_EBERR             0x10000000
#define IMASK_MSRO		0x04000000
#define IMASK_GRSC              0x02000000
#define IMASK_BABT		0x01000000
#define IMASK_TXC               0x00800000
#define IMASK_TXEEN		0x00400000
#define IMASK_TXBEN		0x00200000
#define IMASK_TXFEN             0x00100000
#define IMASK_LC		0x00040000
#define IMASK_CRL		0x00020000
#define IMASK_XFUN		0x00010000
#define IMASK_RXB0              0x00008000
#define IMASK_MAG		0x00000800
#define IMASK_GTSC              0x00000100
#define IMASK_RXFEN0		0x00000080
#define IMASK_FIR		0x00000008
#define IMASK_FIQ		0x00000004
#define IMASK_DPE		0x00000002
#define IMASK_PERR		0x00000001
#define IMASK_RX_DISABLED ~(IMASK_RXFEN0 | IMASK_BSY)
#define IMASK_DEFAULT  (IMASK_TXEEN | IMASK_TXFEN | IMASK_TXBEN | \
		IMASK_RXFEN0 | IMASK_BSY | IMASK_EBERR | IMASK_BABR | \
		IMASK_XFUN | IMASK_RXC | IMASK_BABT | IMASK_DPE \
		| IMASK_PERR)
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
#define IMASK_NAPI_DISABLED ~(IMASK_RXFEN0 | IMASK_BSY | IMASK_TXFEN)
#endif

/* Fifo management */
#define FIFO_TX_THR_MASK	0x01ff
#define FIFO_TX_STARVE_MASK	0x01ff
#define FIFO_TX_STARVE_OFF_MASK	0x01ff

/* Attribute fields */

/* This enables rx snooping for buffers and descriptors */
#define ATTR_BDSTASH		0x00000800

#define ATTR_BUFSTASH		0x00004000

#define ATTR_SNOOPING		0x000000c0
#define ATTR_INIT_SETTINGS      ATTR_SNOOPING

#define ATTRELI_INIT_SETTINGS   0x0
#define ATTRELI_EL_MASK		0x3fff0000
#define ATTRELI_EL(x) (x << 16)
#define ATTRELI_EI_MASK		0x00003fff
#define ATTRELI_EI(x) (x)


/* TxBD status field bits */
#define TXBD_READY		0x8000
#define TXBD_PADCRC		0x4000
#define TXBD_WRAP		0x2000
#define TXBD_INTERRUPT		0x1000
#define TXBD_LAST		0x0800
#define TXBD_CRC		0x0400
#define TXBD_DEF		0x0200
#define TXBD_HUGEFRAME		0x0080
#define TXBD_LATECOLLISION	0x0080
#define TXBD_RETRYLIMIT		0x0040
#define	TXBD_RETRYCOUNTMASK	0x003c
#define TXBD_UNDERRUN		0x0002
#define TXBD_TOE		0x0002

/* Tx FCB param bits */
#define TXFCB_VLN		0x80
#define TXFCB_IP		0x40
#define TXFCB_IP6		0x20
#define TXFCB_TUP		0x10
#define TXFCB_UDP		0x08
#define TXFCB_CIP		0x04
#define TXFCB_CTU		0x02
#define TXFCB_NPH		0x01
#define TXFCB_DEFAULT 		(TXFCB_IP|TXFCB_TUP|TXFCB_CTU|TXFCB_NPH)

/* RxBD status field bits */
#define RXBD_EMPTY		0x8000
#define RXBD_RO1		0x4000
#define RXBD_WRAP		0x2000
#define RXBD_INTERRUPT		0x1000
#define RXBD_LAST		0x0800
#define RXBD_FIRST		0x0400
#define RXBD_MISS		0x0100
#define RXBD_BROADCAST		0x0080
#define RXBD_MULTICAST		0x0040
#define RXBD_LARGE		0x0020
#define RXBD_NONOCTET		0x0010
#define RXBD_SHORT		0x0008
#define RXBD_CRCERR		0x0004
#define RXBD_OVERRUN		0x0002
#define RXBD_TRUNCATED		0x0001
#define RXBD_STATS		0x01ff

/* Rx FCB status field bits */
#define RXFCB_VLN		0x8000
#define RXFCB_IP		0x4000
#define RXFCB_IP6		0x2000
#define RXFCB_TUP		0x1000
#define RXFCB_CIP		0x0800
#define RXFCB_CTU		0x0400
#define RXFCB_EIP		0x0200
#define RXFCB_ETU		0x0100
#define RXFCB_CSUM_MASK		0x0f00
#define RXFCB_PERR_MASK		0x000c
#define RXFCB_PERR_BADL3	0x0008

/* 1588 Module Registers bits */
#ifdef CONFIG_GFAR_PTP

/* AKB 200-03-14: Added Raw Timer Control bits from spec */

#define TMR_CTRL_ALM1P		0x80000000	/* 00:    Alarm1 output polarity  */
#define TMR_CTRL_ALM2P		0x40000000	/* 01:    Alarm2 output polarity  */
#define TMR_CTRL_RSVD1		0x20000000	/* 02:    Reserved   */
#define TMR_CTRL_FS		0x10000000	/* 03:    FIPER start indication  */
#define TMR_CTRL_RSVD2		0x0C000000	/* 04-05: Reserved   */
#define TMR_CTRL_TCLK_PERIOD	0x03FF0000	/* 06-15: Timer reference clock period   */
#define TMR_CTRL_RSVD3		0x0000FC00	/* 16-21: Reserved   */
#define TMR_CTRL_ETEP2		0x00000200	/* 22:    External trigger 2 edge polarity */
#define TMR_CTRL_ETEP1		0x00000100	/* 23:    External trigger 1 edge polarity */
#define TMR_CTRL_COPH		0x00000080	/* 24:    Generated clock output phase */
#define TMR_CTRL_CIPH		0x00000040	/* 25:    External oscillator input phase */
#define TMR_CTRL_TMSR		0x00000020	/* 26:    Timer soft reset */
#define TMR_CTRL_DBG		0x00000010	/* 27:    Enable Debug mode */
#define TMR_CTRL_BYP		0x00000008	/* 28:    Bypass drift compensated clock */
#define TMR_CTRL_TE		0x00000004	/* 29:    Timer enable */
#define TMR_CTRL_CKSEL		0x00000003	/* 30-31: Timer reference clock source select */

#define TMR_CTRL_ENABLE		0x00000004	/* 29:    TE */
#define TMR_CTRL_RTC_CLK	0x00000003	/* 30-31: CKSEL: 11 RTC Clock oscillator */
#define TMR_CTRL_EXT_CLK	0x00000000	/* 30-31: CKSEL: 00 External (TSEC_TMR_CLK) */
#define TMR_CTRL_SYS_CLK	0x00000001	/* 30-31: CKSEL: 01 eTSEC system clock */

#define TMR_CTRL_TCLK_PRD	0x00640000	/* 06-15: Default Nominal Freq = 10 MHz */

#define TMR_CTRL_TCLK_MASK	0x03ff0000	/* 06-15: Mask for writing TCLK PERIOD */

/* Time add value */
#define TMR_ADD_VAL		CONFIG_GFAR_PTP_TMR_ADD  /* Constant from build process */

#define TMR_PEVENT_TXP2		0x00000200
#define TMR_PEVENT_TXP1		0x00000100
#define TMR_PEVENT_RXP		0x00000001
#define TMR_PTPD_MAX_FREQ	0x80000
#define TMR_CTRL_FIPER_START    0x10000000
#define TMR_PRSC                0x2
#define TMR_MAX_DAC_V		0xaf		/* We have a 8 bit DAC which */
						/* saturates at 3.3V */
#endif


struct txbd8
{
	u16	status;	/* Status Fields */
	u16	length;	/* Buffer length */
	u32	bufPtr;	/* Buffer Pointer */
};

struct txfcb {
	u8	flags;
	u8	ptp;	/* Least Significant bit for enabling Tx TimeStamping */
	u8	l4os;	/* Level 4 Header Offset */
	u8	l3os; 	/* Level 3 Header Offset */
	u16	phcs;	/* Pseudo-header Checksum */
	u16	vlctl;	/* VLAN control word */
};

struct rxbd8
{
	u16	status;	/* Status Fields */
	u16	length;	/* Buffer Length */
	u32	bufPtr;	/* Buffer Pointer */
};

struct rxfcb {
	u16	flags;
	u8	rq;	/* Receive Queue index */
	u8	pro;	/* Layer 4 Protocol */
	u16	reserved;
	u16	vlctl;	/* VLAN control word */
};

struct rmon_mib
{
	u32	tr64;	/* 0x.680 - Transmit and Receive 64-byte Frame Counter */
	u32	tr127;	/* 0x.684 - Transmit and Receive 65-127 byte Frame Counter */
	u32	tr255;	/* 0x.688 - Transmit and Receive 128-255 byte Frame Counter */
	u32	tr511;	/* 0x.68c - Transmit and Receive 256-511 byte Frame Counter */
	u32	tr1k;	/* 0x.690 - Transmit and Receive 512-1023 byte Frame Counter */
	u32	trmax;	/* 0x.694 - Transmit and Receive 1024-1518 byte Frame Counter */
	u32	trmgv;	/* 0x.698 - Transmit and Receive 1519-1522 byte Good VLAN Frame */
	u32	rbyt;	/* 0x.69c - Receive Byte Counter */
	u32	rpkt;	/* 0x.6a0 - Receive Packet Counter */
	u32	rfcs;	/* 0x.6a4 - Receive FCS Error Counter */
	u32	rmca;	/* 0x.6a8 - Receive Multicast Packet Counter */
	u32	rbca;	/* 0x.6ac - Receive Broadcast Packet Counter */
	u32	rxcf;	/* 0x.6b0 - Receive Control Frame Packet Counter */
	u32	rxpf;	/* 0x.6b4 - Receive Pause Frame Packet Counter */
	u32	rxuo;	/* 0x.6b8 - Receive Unknown OP Code Counter */
	u32	raln;	/* 0x.6bc - Receive Alignment Error Counter */
	u32	rflr;	/* 0x.6c0 - Receive Frame Length Error Counter */
	u32	rcde;	/* 0x.6c4 - Receive Code Error Counter */
	u32	rcse;	/* 0x.6c8 - Receive Carrier Sense Error Counter */
	u32	rund;	/* 0x.6cc - Receive Undersize Packet Counter */
	u32	rovr;	/* 0x.6d0 - Receive Oversize Packet Counter */
	u32	rfrg;	/* 0x.6d4 - Receive Fragments Counter */
	u32	rjbr;	/* 0x.6d8 - Receive Jabber Counter */
	u32	rdrp;	/* 0x.6dc - Receive Drop Counter */
	u32	tbyt;	/* 0x.6e0 - Transmit Byte Counter Counter */
	u32	tpkt;	/* 0x.6e4 - Transmit Packet Counter */
	u32	tmca;	/* 0x.6e8 - Transmit Multicast Packet Counter */
	u32	tbca;	/* 0x.6ec - Transmit Broadcast Packet Counter */
	u32	txpf;	/* 0x.6f0 - Transmit Pause Control Frame Counter */
	u32	tdfr;	/* 0x.6f4 - Transmit Deferral Packet Counter */
	u32	tedf;	/* 0x.6f8 - Transmit Excessive Deferral Packet Counter */
	u32	tscl;	/* 0x.6fc - Transmit Single Collision Packet Counter */
	u32	tmcl;	/* 0x.700 - Transmit Multiple Collision Packet Counter */
	u32	tlcl;	/* 0x.704 - Transmit Late Collision Packet Counter */
	u32	txcl;	/* 0x.708 - Transmit Excessive Collision Packet Counter */
	u32	tncl;	/* 0x.70c - Transmit Total Collision Counter */
	u8	res1[4];
	u32	tdrp;	/* 0x.714 - Transmit Drop Frame Counter */
	u32	tjbr;	/* 0x.718 - Transmit Jabber Frame Counter */
	u32	tfcs;	/* 0x.71c - Transmit FCS Error Counter */
	u32	txcf;	/* 0x.720 - Transmit Control Frame Counter */
	u32	tovr;	/* 0x.724 - Transmit Oversize Frame Counter */
	u32	tund;	/* 0x.728 - Transmit Undersize Frame Counter */
	u32	tfrg;	/* 0x.72c - Transmit Fragments Frame Counter */
	u32	car1;	/* 0x.730 - Carry Register One */
	u32	car2;	/* 0x.734 - Carry Register Two */
	u32	cam1;	/* 0x.738 - Carry Mask Register One */
	u32	cam2;	/* 0x.73c - Carry Mask Register Two */
};

struct gfar_extra_stats {
	u64 kernel_dropped;
	u64 rx_large;
	u64 rx_short;
	u64 rx_nonoctet;
	u64 rx_crcerr;
	u64 rx_overrun;
	u64 rx_bsy;
	u64 rx_babr;
	u64 rx_trunc;
	u64 eberr;
	u64 tx_babt;
	u64 tx_underrun;
	u64 rx_skbmissing;
	u64 tx_timeout;
};

#define GFAR_RMON_LEN ((sizeof(struct rmon_mib) - 16)/sizeof(u32))
#define GFAR_EXTRA_STATS_LEN (sizeof(struct gfar_extra_stats)/sizeof(u64))

/* Number of stats in the stats structure (ignore car and cam regs)*/
#define GFAR_STATS_LEN (GFAR_RMON_LEN + GFAR_EXTRA_STATS_LEN)

#define GFAR_INFOSTR_LEN 32

struct gfar_stats {
	u64 extra[GFAR_EXTRA_STATS_LEN];
	u64 rmon[GFAR_RMON_LEN];
};


struct gfar {
	u32	tsec_id;	/* 0x.000 - Controller ID register */
	u8	res1[12];
	u32	ievent;		/* 0x.010 - Interrupt Event Register */
	u32	imask;		/* 0x.014 - Interrupt Mask Register */
	u32	edis;		/* 0x.018 - Error Disabled Register */
	u8	res2[4];
	u32	ecntrl;		/* 0x.020 - Ethernet Control Register */
	u32	minflr;		/* 0x.024 - Minimum Frame Length Register */
	u32	ptv;		/* 0x.028 - Pause Time Value Register */
	u32	dmactrl;	/* 0x.02c - DMA Control Register */
	u32	tbipa;		/* 0x.030 - TBI PHY Address Register */
	u8	res3[88];
	u32	fifo_tx_thr;	/* 0x.08c - FIFO transmit threshold register */
	u8	res4[8];
	u32	fifo_tx_starve;	/* 0x.098 - FIFO transmit starve register */
	u32	fifo_tx_starve_shutoff;	/* 0x.09c - FIFO transmit starve shutoff register */
	u8	res5[4];
	u32	fifo_rx_pause;	/* 0x.0a4 - FIFO receive pause threshold register */
	u32	fifo_rx_alarm;	/* 0x.0a8 - FIFO receive alarm threshold register */
	u8	res6[84];
	u32	tctrl;		/* 0x.100 - Transmit Control Register */
	u32	tstat;		/* 0x.104 - Transmit Status Register */
	u32	dfvlan;		/* 0x.108 - Default VLAN Control word */
	u32	tbdlen;		/* 0x.10c - Transmit Buffer Descriptor Data Length Register */
	u32	txic;		/* 0x.110 - Transmit Interrupt Coalescing Configuration Register */
	u32	tqueue;		/* 0x.114 - Transmit queue control register */
	u8	res7[40];
	u32	tr03wt;		/* 0x.140 - TxBD Rings 0-3 round-robin weightings */
	u32	tr47wt;		/* 0x.144 - TxBD Rings 4-7 round-robin weightings */
	u8	res8[52];
	u32	tbdbph;		/* 0x.17c - Tx data buffer pointer high */
	u8	res9a[4];
	u32	tbptr0;		/* 0x.184 - TxBD Pointer for ring 0 */
	u8	res9b[4];
	u32	tbptr1;		/* 0x.18c - TxBD Pointer for ring 1 */
	u8	res9c[4];
	u32	tbptr2;		/* 0x.194 - TxBD Pointer for ring 2 */
	u8	res9d[4];
	u32	tbptr3;		/* 0x.19c - TxBD Pointer for ring 3 */
	u8	res9e[4];
	u32	tbptr4;		/* 0x.1a4 - TxBD Pointer for ring 4 */
	u8	res9f[4];
	u32	tbptr5;		/* 0x.1ac - TxBD Pointer for ring 5 */
	u8	res9g[4];
	u32	tbptr6;		/* 0x.1b4 - TxBD Pointer for ring 6 */
	u8	res9h[4];
	u32	tbptr7;		/* 0x.1bc - TxBD Pointer for ring 7 */
	u8	res9[64];
	u32	tbaseh;		/* 0x.200 - TxBD base address high */
	u32	tbase0;		/* 0x.204 - TxBD Base Address of ring 0 */
	u8	res10a[4];
	u32	tbase1;		/* 0x.20c - TxBD Base Address of ring 1 */
	u8	res10b[4];
	u32	tbase2;		/* 0x.214 - TxBD Base Address of ring 2 */
	u8	res10c[4];
	u32	tbase3;		/* 0x.21c - TxBD Base Address of ring 3 */
	u8	res10d[4];
	u32	tbase4;		/* 0x.224 - TxBD Base Address of ring 4 */
	u8	res10e[4];
	u32	tbase5;		/* 0x.22c - TxBD Base Address of ring 5 */
	u8	res10f[4];
	u32	tbase6;		/* 0x.234 - TxBD Base Address of ring 6 */
	u8	res10g[4];
	u32	tbase7;		/* 0x.23c - TxBD Base Address of ring 7 */
#ifndef CONFIG_GFAR_PTP
	u8	res10[192];
#else				/*New Registers added for Tx Timer modules */
	u8	 res10h[64];
	u32	tmr_txts1_id;	/* 0x.280 Tx time stamp identification*/
	u32	tmr_txts2_id;	/* 0x.284 Tx time stamp Identification*/
	u8	res10i[56];
	u32	tmr_txts1_h;	/* 0x.2c0 Tx time stamp high*/
	u32	tmr_txts1_l;	/* 0x.2c4 Tx Time Stamp low*/
	u32	tmr_txts2_h;	/* 0x.2c8 Tx time stamp high*/
	u32	tmr_txts2_l;	/*0x.2cc  Tx Time Stamp low */
	u8	res10j[48];
#endif

	u32	rctrl;		/* 0x.300 - Receive Control Register */
	u32	rstat;		/* 0x.304 - Receive Status Register */
	u8	res12[8];
	u32	rxic;		/* 0x.310 - Receive Interrupt Coalescing Configuration Register */
	u32	rqueue;		/* 0x.314 - Receive queue control register */
	u8	res13[24];
	u32	rbifx;		/* 0x.330 - Receive bit field extract control register */
	u32	rqfar;		/* 0x.334 - Receive queue filing table address register */
	u32	rqfcr;		/* 0x.338 - Receive queue filing table control register */
	u32	rqfpr;		/* 0x.33c - Receive queue filing table property register */
	u32	mrblr;		/* 0x.340 - Maximum Receive Buffer Length Register */
	u8	res14[56];
	u32	rbdbph;		/* 0x.37c - Rx data buffer pointer high */
	u8	res15a[4];
	u32	rbptr0;		/* 0x.384 - RxBD pointer for ring 0 */
	u8	res15b[4];
	u32	rbptr1;		/* 0x.38c - RxBD pointer for ring 1 */
	u8	res15c[4];
	u32	rbptr2;		/* 0x.394 - RxBD pointer for ring 2 */
	u8	res15d[4];
	u32	rbptr3;		/* 0x.39c - RxBD pointer for ring 3 */
	u8	res15e[4];
	u32	rbptr4;		/* 0x.3a4 - RxBD pointer for ring 4 */
	u8	res15f[4];
	u32	rbptr5;		/* 0x.3ac - RxBD pointer for ring 5 */
	u8	res15g[4];
	u32	rbptr6;		/* 0x.3b4 - RxBD pointer for ring 6 */
	u8	res15h[4];
	u32	rbptr7;		/* 0x.3bc - RxBD pointer for ring 7 */
	u8	res16[64];
	u32	rbaseh;		/* 0x.400 - RxBD base address high */
	u32	rbase0;		/* 0x.404 - RxBD base address of ring 0 */
	u8	res17a[4];
	u32	rbase1;		/* 0x.40c - RxBD base address of ring 1 */
	u8	res17b[4];
	u32	rbase2;		/* 0x.414 - RxBD base address of ring 2 */
	u8	res17c[4];
	u32	rbase3;		/* 0x.41c - RxBD base address of ring 3 */
	u8	res17d[4];
	u32	rbase4;		/* 0x.424 - RxBD base address of ring 4 */
	u8	res17e[4];
	u32	rbase5;		/* 0x.42c - RxBD base address of ring 5 */
	u8	res17f[4];
	u32	rbase6;		/* 0x.434 - RxBD base address of ring 6 */
	u8	res17g[4];
	u32	rbase7;		/* 0x.43c - RxBD base address of ring 7 */
#ifndef CONFIG_GFAR_PTP
	u8	res17[192];
#else
	u8	res17h[128];
	u32	tmr_rxts_h;	/* 0x.4c0 Rx Time Stamp high*/
	u32	tmr_rxts_l;	/* 0x.4c4 Rx Time Stamp low */
	u8	res17i[56];
#endif

	u32	maccfg1;	/* 0x.500 - MAC Configuration 1 Register */
	u32	maccfg2;	/* 0x.504 - MAC Configuration 2 Register */
	u32	ipgifg;		/* 0x.508 - Inter Packet Gap/Inter Frame Gap Register */
	u32	hafdup;		/* 0x.50c - Half Duplex Register */
	u32	maxfrm;		/* 0x.510 - Maximum Frame Length Register */
	u8	res18[12];
	u8	gfar_mii_regs[24];	/* See gianfar_phy.h */
	u8	res19[4];
	u32	ifstat;		/* 0x.53c - Interface Status Register */
	u32	macstnaddr1;	/* 0x.540 - Station Address Part 1 Register */
	u32	macstnaddr2;	/* 0x.544 - Station Address Part 2 Register */
	u32	mac01addr1;	/* 0x.548 - MAC exact match address 1, part 1 */
	u32	mac01addr2;	/* 0x.54c - MAC exact match address 1, part 2 */
	u32	mac02addr1;	/* 0x.550 - MAC exact match address 2, part 1 */
	u32	mac02addr2;	/* 0x.554 - MAC exact match address 2, part 2 */
	u32	mac03addr1;	/* 0x.558 - MAC exact match address 3, part 1 */
	u32	mac03addr2;	/* 0x.55c - MAC exact match address 3, part 2 */
	u32	mac04addr1;	/* 0x.560 - MAC exact match address 4, part 1 */
	u32	mac04addr2;	/* 0x.564 - MAC exact match address 4, part 2 */
	u32	mac05addr1;	/* 0x.568 - MAC exact match address 5, part 1 */
	u32	mac05addr2;	/* 0x.56c - MAC exact match address 5, part 2 */
	u32	mac06addr1;	/* 0x.570 - MAC exact match address 6, part 1 */
	u32	mac06addr2;	/* 0x.574 - MAC exact match address 6, part 2 */
	u32	mac07addr1;	/* 0x.578 - MAC exact match address 7, part 1 */
	u32	mac07addr2;	/* 0x.57c - MAC exact match address 7, part 2 */
	u32	mac08addr1;	/* 0x.580 - MAC exact match address 8, part 1 */
	u32	mac08addr2;	/* 0x.584 - MAC exact match address 8, part 2 */
	u32	mac09addr1;	/* 0x.588 - MAC exact match address 9, part 1 */
	u32	mac09addr2;	/* 0x.58c - MAC exact match address 9, part 2 */
	u32	mac10addr1;	/* 0x.590 - MAC exact match address 10, part 1*/
	u32	mac10addr2;	/* 0x.594 - MAC exact match address 10, part 2*/
	u32	mac11addr1;	/* 0x.598 - MAC exact match address 11, part 1*/
	u32	mac11addr2;	/* 0x.59c - MAC exact match address 11, part 2*/
	u32	mac12addr1;	/* 0x.5a0 - MAC exact match address 12, part 1*/
	u32	mac12addr2;	/* 0x.5a4 - MAC exact match address 12, part 2*/
	u32	mac13addr1;	/* 0x.5a8 - MAC exact match address 13, part 1*/
	u32	mac13addr2;	/* 0x.5ac - MAC exact match address 13, part 2*/
	u32	mac14addr1;	/* 0x.5b0 - MAC exact match address 14, part 1*/
	u32	mac14addr2;	/* 0x.5b4 - MAC exact match address 14, part 2*/
	u32	mac15addr1;	/* 0x.5b8 - MAC exact match address 15, part 1*/
	u32	mac15addr2;	/* 0x.5bc - MAC exact match address 15, part 2*/
	u8	res20[192];
	struct rmon_mib	rmon;	/* 0x.680-0x.73c */
	u32	rrej;		/* 0x.740 - Receive filer rejected packet counter */
	u8	res21[188];
	u32	igaddr0;	/* 0x.800 - Indivdual/Group address register 0*/
	u32	igaddr1;	/* 0x.804 - Indivdual/Group address register 1*/
	u32	igaddr2;	/* 0x.808 - Indivdual/Group address register 2*/
	u32	igaddr3;	/* 0x.80c - Indivdual/Group address register 3*/
	u32	igaddr4;	/* 0x.810 - Indivdual/Group address register 4*/
	u32	igaddr5;	/* 0x.814 - Indivdual/Group address register 5*/
	u32	igaddr6;	/* 0x.818 - Indivdual/Group address register 6*/
	u32	igaddr7;	/* 0x.81c - Indivdual/Group address register 7*/
	u8	res22[96];
	u32	gaddr0;		/* 0x.880 - Group address register 0 */
	u32	gaddr1;		/* 0x.884 - Group address register 1 */
	u32	gaddr2;		/* 0x.888 - Group address register 2 */
	u32	gaddr3;		/* 0x.88c - Group address register 3 */
	u32	gaddr4;		/* 0x.890 - Group address register 4 */
	u32	gaddr5;		/* 0x.894 - Group address register 5 */
	u32	gaddr6;		/* 0x.898 - Group address register 6 */
	u32	gaddr7;		/* 0x.89c - Group address register 7 */
	u8	res23a[352];
	u32	fifocfg;	/* 0x.a00 - FIFO interface config register */
	u8	res23b[252];
	u8	res23c[248];
	u32	attr;		/* 0x.bf8 - Attributes Register */
	u32	attreli;	/* 0x.bfc - Attributes Extract Length and Extract Index Register */
#ifndef CONFIG_GFAR_PTP
	u8	res24[1024];
#else				/* 1588 Timer Module Registers */
	u8	res24[512];
	u32	tmr_ctrl;	/* 0x.e00 - Timer Control Register */
	u32	tmr_tevent;	/* 0x.e04 - Timer stamp event register */
	u32	tmr_temask;	/* 0x.e08 - Timer event mask register */
	u32	tmr_pevent;	/* 0x.e0c - Timer stamp event register */
	u32	tmr_pemask;	/* 0x.e10 - Timer event mask register */
	u32	tmr_stat;	/* 0x.e14 - Timer stamp status register */
	u32	tmr_cnt_h;	/* 0x.e18 - Timer counter high register */
	u32	tmr_cnt_l;	/* 0x.e1c - Timer counter low register */
	u32	tmr_add;	/* 0x.e20 - Timer dirft compensation addend register */
	u32	tmr_acc;	/* 0x.e24 - Timer accumulator register */
	u32	tmr_prsc;	/* 0x.e28 - Timer prescale register */
	u8	res24a[4];	/* 0x.e2c - 0x.e2f reserved */
	u32	tmr_off_h;	/* 0x.e30 - Timer offset high register */
	u32	tmr_off_l;	/* 0x.e34 - Timer offset low register */
	u8	res24b[8];	/* 0x.e38 - 0x.e3f reserved */
	u32	tmr_alarm1_h;	/* 0x.e40 - Timer alarm 1 high register */
	u32	tmr_alarm1_l;	/* 0x.e44 - Timer alarm 1 low register */
	u32	tmr_alarm2_h;	/* 0x.e48 - Timer alarm 2 high register */
	u32	tmr_alarm2_l;	/* 0x.e4c - Timer alarm 2 low register */
	u8	res24c[48];	/* 0x.e50 - 0x.e7f reserved */
	u32	tmr_fiper1;	/* 0x.e80 - Timer fixed period register 1 */
	u32	tmr_fiper2;	/* 0x.e84 - Timer fixed period register 2 */
	u32	tmr_fiper3;	/* 0x.e88 - Timer fixed period register 3 */
	u8	res24d[20];	/* 0x.e8c - 0x.ebf reserved */
	u32	tmr_etts1_h;	/* 0x.ea0 - Timer stamp high of general purpose external trigger 1*/
	u32	tmr_etts1_l;	/* 0x.ea4 - Timer stamp low of general purpose external trigger 1*/
	u32	tmr_etts2_h;	/* 0x.ea8 - Timer stamp high of general purpose external trigger 2 */
	u32	tmr_etts2_l;	/* 0x.eac - Timer stamp low of general purpose external trigger 2*/
	u8	res25[336];	/* 0x.eb0 - 0x.fff */
#endif
};

#ifdef CONFIG_GFAR_PTP
/* Structure for PTP Time Stamp */
struct gfar_ptp_time {
	u32	high;
	u32	low;
};

struct gfar_ptp_data_t {
	int			key;
	struct gfar_ptp_time	item;
};

struct gfar_ptp_circular_t {
	int			front;
	int			end;
	int			size;
	struct gfar_ptp_data_t	*data_buf;
};

#endif

/* Struct stolen almost completely (and shamelessly) from the FCC enet source
 * (Ok, that's not so true anymore, but there is a family resemblence)
 * The GFAR buffer descriptors track the ring buffers.  The rx_bd_base
 * and tx_bd_base always point to the currently available buffer.
 * The dirty_tx tracks the current buffer that is being sent by the
 * controller.  The cur_tx and dirty_tx are equal under both completely
 * empty and completely full conditions.  The empty/ready indicator in
 * the buffer descriptor determines the actual condition.
 */
struct gfar_private {
	/* Fields controlled by TX lock */
	spinlock_t txlock;

	/* Pointer to the array of skbuffs */
	struct sk_buff ** tx_skbuff;

	/* next free skb in the array */
	u16 skb_curtx;

	/* First skb in line to be transmitted */
	u16 skb_dirtytx;

	/* Configuration info for the coalescing features */
	unsigned char txcoalescing;
	unsigned short txcount;
	unsigned short txtime;

	/* Buffer descriptor pointers */
	struct txbd8 *tx_bd_base;	/* First tx buffer descriptor */
	struct txbd8 *cur_tx;	        /* Next free ring entry */
	struct txbd8 *dirty_tx;		/* First buffer in line
					   to be transmitted */
	unsigned int tx_ring_size;

	/* RX Locked fields */
	spinlock_t rxlock;

	/* skb array and index */
	struct sk_buff ** rx_skbuff;
	u16 skb_currx;

	/* RX Coalescing values */
	unsigned char rxcoalescing;
	unsigned short rxcount;
	unsigned short rxtime;

	unsigned long txic;
	unsigned long rxic;
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	/* Buffer size for Advanced SKB Handling */
	unsigned long skbuff_truesize;
	unsigned long skbuff_buffsize;
#endif
	struct rxbd8 *rx_bd_base;	/* First Rx buffers */
	struct rxbd8 *cur_rx;           /* Next free rx ring entry */

	/* RX parameters */
	unsigned int rx_ring_size;
	unsigned int rx_buffer_size;
	unsigned int rx_stash_size;
	unsigned int rx_stash_index;

	struct vlan_group *vlgrp;

	/* Unprotected fields */
	/* Pointer to the GFAR memory mapped Registers */
	struct gfar __iomem *regs;

	/* Hash registers and their width */
	u32 __iomem *hash_regs[16];
	int hash_width;

	/* global parameters */
	unsigned int fifo_threshold;
	unsigned int fifo_starve;
	unsigned int fifo_starve_off;

	/* Bitfield update lock */
	spinlock_t bflock;

	unsigned char vlan_enable:1,
		rx_csum_enable:1,
		extended_hash:1,
		wol_en:1, // Wake-on-LAN enabled
		suspended:1,
#ifdef CONFIG_GFAR_PTP
		bd_stash_en:1,
		ptp_timestamping:1;
#else
		bd_stash_en:1;
#endif
	unsigned short padding;

	unsigned int interruptTransmit;
	unsigned int interruptReceive;
	unsigned int interruptError;

	/* info structure initialized by platform code */
	struct gianfar_platform_data *einfo;

	u32 saved_sleep_reg;

	/* PHY stuff */
	struct phy_device *phydev;
	struct mii_bus *mii_bus;
	int oldspeed;
	int oldduplex;
	int oldlink;

	uint32_t msg_enable;

	/* Network Statistics */
	struct net_device_stats stats;
	struct gfar_extra_stats extra_stats;
#ifdef CONFIG_GFAR_PTP
	struct gfar_ptp_circular_t rx_time_sync;
	struct gfar_ptp_circular_t rx_time_del_req;
	struct gfar __iomem *regs_1588;
#endif

};

static inline u32 gfar_read(volatile unsigned __iomem *addr)
{
	u32 val;
	val = in_be32(addr);
	return val;
}

static inline void gfar_write(volatile unsigned __iomem *addr, u32 val)
{
	out_be32(addr, val);
}
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
#define GFAR_RECYCLE_MAX 64
struct gfar_skb_handler {
	spinlock_t		lock;
	long int		recycle_max;
	long int		recycle_count; /* should be atomic */
	struct sk_buff		*recycle_queue;
};

DECLARE_PER_CPU(struct gfar_skb_handler, gfar_skb_handler);

#define GFAR_KFREE_SKB(skb,size) gfar_kfree_skb( skb, size )

#else /*CONFIG_GFAR_SKBUFF_RECYCLING*/
/* use dev_kfree_skb_irq directly */
#define GFAR_KFREE_SKB(skb,arg...) dev_kfree_skb_irq( skb )
#endif  /*CONFIG_GFAR_SKBUFF_RECYCLING*/

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
extern irqreturn_t gfar_receive(int irq, void *dev_id, struct pt_regs *regs);
#else
extern irqreturn_t gfar_receive(int irq, void *dev_id);
#endif
extern int startup_gfar(struct net_device *dev);
extern void stop_gfar(struct net_device *dev);
extern void gfar_halt(struct net_device *dev);
#ifdef CONFIG_GFAR_PTP
extern void gfar_1588_start(struct net_device *dev);
extern void gfar_1588_stop(struct net_device *dev);

extern char ad5301_set_voltage(char);
extern char ad5301_get_voltage(void);

extern int gfar_ptp_init(struct gfar_private *priv,
				struct gianfar_platform_data *einfo);
extern void gfar_ptp_cleanup(struct gfar_private *priv);
extern int gfar_ptp_do_txstamp(struct sk_buff *skb);
extern void gfar_ptp_store_rxstamp(struct net_device *dev, struct sk_buff *skb);

extern int gfar_1588_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
#endif

extern void gfar_phy_test(struct mii_bus *bus, struct phy_device *phydev,
		int enable, u32 regnum, u32 read);
void gfar_init_sysfs(struct net_device *dev);

#endif /* __GIANFAR_H */
