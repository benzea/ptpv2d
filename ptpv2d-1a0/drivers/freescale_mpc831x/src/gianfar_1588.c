/*
 * drivers/net/gianfar_1588.c
 *
 * Gianfar Ethernet Driver -- IEEE 1588 interface functionality
 *
 * Copyright (c) 2007 Freescale Semiconductor, Inc.
 *
 * Author: Anup Gangwar <anup.gangwar@freescale.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 */

/****************************************************************************/
/* Begin copyright and licensing information, do not remove                 */
/*                                                                          */
/* This file (gianfar_1588.c) contains Modifications (updates, corrections  */
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

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <asm/ocp.h>

#include <linux/io.h>
#include <linux/irq.h>
#include <linux/uaccess.h>

#include "gianfar.h"

static int gfar_ptp_init_circ(struct gfar_ptp_circular_t *buf);
static int gfar_ptp_is_empty(struct gfar_ptp_circular_t *buf);
static int gfar_ptp_nelems(struct gfar_ptp_circular_t *buf);
static int gfar_ptp_is_full(struct gfar_ptp_circular_t *buf);
static int gfar_ptp_insert(struct gfar_ptp_circular_t *buf,
			struct gfar_ptp_data_t *data);
static int gfar_ptp_find_and_remove(struct gfar_ptp_circular_t *buf,
			int key, struct gfar_ptp_data_t *data);

/*
 * Resource required for accessing 1588 Timer Registers. There are few 1588
 * modules registers which are present in eTSEC1 memory space only. The second
 * reg entry there in denotes the 1588 regs.
 */
int gfar_ptp_init(struct gfar_private *priv,
		struct gianfar_platform_data *einfo)
{
	priv->regs_1588 = ioremap(einfo->regs_1588, sizeof (struct gfar));
	if ((priv->regs_1588 == NULL) ||
			gfar_ptp_init_circ(&(priv->rx_time_sync)) ||
			gfar_ptp_init_circ(&(priv->rx_time_del_req))) {
		return 1;
	}

	return 0;
}

void gfar_ptp_cleanup(struct gfar_private *priv)
{
	if (priv->regs_1588 != NULL)
		iounmap(priv->regs_1588);
	if (priv->rx_time_sync.data_buf)
		vfree(priv->rx_time_sync.data_buf);
	if (priv->rx_time_del_req.data_buf)
		vfree(priv->rx_time_del_req.data_buf);
}

#define get_u8(ptr)  (*((u8*)ptr))
#define get_u16(ptr) (*((u16*)ptr))
#define get_u32(ptr) (*((u32*)ptr))

int gfar_ptp_do_txstamp(struct sk_buff *skb)
{

/* Old code
	u32 *multicast_addr;
	char *pkt_type;

	if (skb->len > 42) {
		pkt_type = (char *)(skb->data + 31);
		multicast_addr = (u32 *)(skb->data + 38);

		if ((*multicast_addr == 0xe0000181) && (*pkt_type == 0x11))
			return 1;
	}
*/


	/* New code, Alan K. Bartky, alan@bartky.net
	 * test for IEEE 1588 packets and IEEE 802.1AS PTP frames
	 * use hard coded pointers and do not copy to local
	 * variables (again for speed).
	 *
	 * Old code checked for multicast IP address, but unicast is allowed
	 * as well.
	 *
	 * For IEEE 1588 This code checks for IP Ethertype, Destination IP port
	 * and UDP port type (in that order for speed).  By checking for the
	 * IP port number, it automatically gets only those frames that
	 * need to be timestamped as non-timestamped frames use the 
	 * General socket.  In this way, we don't get any false positive
	 * timestamps (i.e. timestamping frames that don't need it). 
	 * 
	 * Note all frame offsets have a value of 8 added to them
	 *
	 * Returns -1 (0xFFFFFFFF) if not to be timestamped, else 
	 * PTP sequence number (0x0000 to 0xFFFF) which will be used to
	 * put into the ID field for later checking when the timestamp
	 * is fetched.
	 *
	 */

	// 1588, test Ethertype, address and protocol
	if (*((u16*)(skb->data + 20)) == 0x0800)    // IP Ethertype (12+8)
	{
		if  (*((u16*)(skb->data + 44)) != 0x013f) // Dest Event Port (36+8)
		{
			return -1;  // Not PTP Event UDP port
		}
		if  (*((u8*)(skb->data + 31)) != 0x11)  // UDP protocol (23+8)
		{
			return -1; // Not UDP protocol
		}
		// IP, UDP & PTP Event port, message type and LSB of timestamp needed 
		// In theory we don't have to also check for message type
		// or for v1 versus v2 as PTP specifies that timestamped
		// frames use the event socket (0x013F), whereas all other frames
		// use the general socket port number (0x0140).
		//
		if  (*((u8*)(skb->data + 51)) == 1)	// Version number (43+8)
		{
			/* Version 1, get message type from control field */
			return (
			         ((int)get_u8(skb->data+82)) << 8 // control field (74+8)
				|
			         ((int)get_u8(skb->data+81)) // LSB of sequence (73+8)
			       );

		}
		else
		{
			/* Version 2 or greater, message type in lowest 4 bits of first byte */

			return (
			         ((int)(get_u8(skb->data+50) & 0x0F)) << 8 // V2 msg field (42+8)
				|
			         ((int) get_u8(skb->data+81)) // LSB of sequence (73+8)
			       );
		}
	}


	// Not IEEE 1588 PTP Event Packet, test Ethertype for IEEE 1588 v2 
	// ethernet & 802.1AS Ethertype (offset 12) 
	// As this parsing is called for non-IP packets, there is no FCB
	// So frame offsets do not need to be adjusted by 8

	if (*((u16*)(skb->data + 12)) != 0x88F7) // IEEE 1588 v2 ethernet & 802.1AS Ethertype (12)
	{
		return -1; // Not PTP ethertype
	}
	// IEEE 1588 v2 ethernet & 802.1AS Ethertype:
	// Test if PTP message type should
	// be timestamped (least significant 4 bits of first byte
	// is less than 8)

	if  ( (*((u8 *)(skb->data + 14)) & 0x0F) >= 8) // PTPv2 message (14)
	{
		/* PTP V2 general message, no timestamp needed */
		return -1;
	}
	// V2 message needing timestamp, return message type and LSB of sequence
	return (
	         ((int)(get_u8(skb->data+14) & 0x0F)) << 8 // V2 msg field (14)
		|
	         ((int) get_u8(skb->data+45)) // LSB of sequence (45)
	       );
}

void gfar_ptp_store_rxstamp(struct net_device *dev, struct sk_buff *skb)
{
/* Old function header and initial code:
	int is_not_igmp, msg_type, seq_id;
	struct gfar_ptp_data_t tmp_rx_time;
	struct gfar_private *priv = netdev_priv(dev);  
	is_not_igmp = (*((char *)skb->data) + 32) != 0x2;

	msg_type = *((u8 *)(skb->data + 82));
*/
	/* New code, Alan K. Bartky alan@bartky.net
	 *
	 * Note: uses goto for speed as this function is called on every frame
	 *
	 * test for IEEE 1588 packets and IEEE 802.1AS PTP frames
	 * use hard coded pointers and do not copy to local
	 * variables (again for speed).
	 *
	 * This code in general will detect not to timestamp
	 * with just 2 compares (so other frames are processed
	 * faster)
	 *
	 * Note all frame offsets have 8 added to them as the
	 * the first 8 bytes of the socket buffer data contain the
	 * receive timestamp.
	 *
	 * This code also checks for PTP Event desination port ID
	 * as ones received on the general socket port ID don't need
	 * to be timestamped.  This should also work for IEEE 1588
	 * version 2 timestamping as the same event socket is
	 * used there and in theory any new messages needing timestamps
	 * should come via this port ID.
	 *
	 * It also has support for detecting both IEEE 1588 PTP over IP packets
	 * and IEEE 802.1AS PTP frames.
	 *
	 */
	int                    msg_type, seq_id;
	struct gfar_ptp_data_t tmp_rx_time;
	struct gfar_private *  priv;  

	/* Check if IEEE 1588 over IP packet
	 * test Ethertype, address and protocol
	 */

	if (*((u16*)(skb->data + 20)) == 0x0800)             /* IP Ethertype    (12+8) */
	{
		if  (*((u16*)(skb->data + 44)) != 0x013F)    /* Event Dest Port (36+8) */
		{
			/* IP packet, but not PTP event Port ID packet,
			 * no need to parse further, so return */
			return;
		}

		/* Probably a PTP packet, check protocol as UDP to be sure */
		if  (*((u8*)(skb->data + 31)) != 0x11)       /* UDP protocol    (23+8) */
		{
			/* Data at Port ID offset was OK, but protocol type
			 * was not UDP.  Not a valid thing to timestamp
			 * so return.
			 */
			return;
		}
		else
		{
			/* Parsing complete for IEEE 1588 PTP frame to timestamp
			 * get sequence ID and message type (control field)
			 * and jump to timestamping processing
			 */
			seq_id =  *((u16 *)(skb->data + 80));    /* get sequence #  (72+8) */
			if ((*((u8 *)(skb->data + 51)) & 0x0F) > 1)  /* get version (43+8) */
			{
				/* Version 2 or greater message 
				 * Message type is lower 4 bits at offset: 42+8
				 */
				msg_type = *((u8 *)(skb->data + 50)) & 0x0F;
			}
			else
			{
				/* Version 1 message
				 * Message type is at offset: 74+8
				 */
				msg_type = *((u8 *)(skb->data + 82));
			}
			goto rx_timestamp;
		}
	}

	/* Not IP packet Ethertype, test Ethertype for IEEE 1588 v2 over Ethernet
         * IEEE 802.1AS Ethertype
	 */

	else if (*((u16*)(skb->data + 20)) == 0x88F7) /* 802.1AS etype (12+8) */
	{
		/* IEEE 1588 v2 over Ethernet 802.1AS Ethertype, test if PTP message type should
		 * be timestamped (if least significant 4 bits is less than 8)
		 */
		msg_type = (*((u8 *)(skb->data + 22)) & 0x0F); // PTPv2 message (14+8)
		if  (msg_type  < 8) 
		{
			/* Parsing complete for IEEE 802.1AS PTP frame to timestamp
			 * get sequence ID and message type (control field)
			 * and jump to timestamping processing
			 */
			seq_id =   *((u16*)(skb->data + 52)); /* get sequence #  (44+8) */
			goto rx_timestamp;
		}
		else
		{
			/* Ethertype is OK for IEEE 802.1AS, but this is not
			 * a message we need to timestamp, so return.
			 */
			return;
		}
	}
	else
	{
		/* Is not an IEEE 1588 or 802.1AS PTP frame to 
		 * timestamp, no further parsing needed, so return
		 */
		return;
	}

rx_timestamp:

	/* Note: Freescale code commented out here as it was checking for a sequence
	 * ID of 0 which is a valid number as sequence ID can be any number
	 * from 0x0000 to 0xFFFF.  New code above does a return as quick as possible
	 * so if we are down here, we know it is OK to put the timestamp in
	 * with the sequence number (including 0)
	 */

/* Wrong, so commented out:
	if (seq_id != 0) {
 */

	tmp_rx_time.key = seq_id;
	tmp_rx_time.item.high = *((u32 *)skb->data);
	tmp_rx_time.item.low = *(((u32 *)skb->data) + 1);

	priv = netdev_priv(dev);
		switch (msg_type) {
	case GFAR_PTP_MSG_TYPE_SYNC: 
		gfar_ptp_insert(&(priv->rx_time_sync), &tmp_rx_time);
		break;
	case GFAR_PTP_MSG_TYPE_DEL_REQ: 
	case GFAR_PTP2_MSG_TYPE_PDEL_REQ:
	case GFAR_PTP2_MSG_TYPE_PDEL_RESP:
		gfar_ptp_insert(&(priv->rx_time_del_req), &tmp_rx_time);
		break;
	default:
		break;
	}
}

int gfar_ptp_init_circ(struct gfar_ptp_circular_t *buf)
{
	buf->data_buf = (struct gfar_ptp_data_t *)
				vmalloc((DEFAULT_PTP_RX_BUF_SZ+1) *
					sizeof(struct gfar_ptp_data_t));

	if (!buf->data_buf)
		return 1;
	buf->front = buf->end = 0;
	buf->size = (DEFAULT_PTP_RX_BUF_SZ + 1);

	return 0;
}

static inline int gfar_ptp_calc_index (int size, int curr_index, int offset)
{
	return ((curr_index + offset) % size);
}

int gfar_ptp_is_empty (struct gfar_ptp_circular_t *buf)
{
	return (buf->front == buf->end);
}

int gfar_ptp_nelems (struct gfar_ptp_circular_t *buf)
{
	const int front = buf->front;
	const int end = buf->end;
	const int size = buf->size;
	int n_items;

	if (end > front)
		n_items = end - front;
	else if (end < front)
		n_items = size - (front - end);
	else
		n_items = 0;

	return n_items;
}

int gfar_ptp_is_full (struct gfar_ptp_circular_t *buf)
{
	if (gfar_ptp_nelems(buf) == (buf->size - 1))
		return 1;
	else
		return 0;
}

int gfar_ptp_insert (struct gfar_ptp_circular_t *buf,
				struct gfar_ptp_data_t *data)
{
	struct gfar_ptp_data_t *tmp;

	if (gfar_ptp_is_full(buf))
		return 1;

	tmp = (buf->data_buf + buf->end);

	tmp->key = data->key;
	tmp->item.high = data->item.high;
	tmp->item.low = data->item.low;

	buf->end = gfar_ptp_calc_index(buf->size, buf->end, 1);

	return 0;
}

int gfar_ptp_find_and_remove (struct gfar_ptp_circular_t *buf,
			int key, struct gfar_ptp_data_t *data)
{
	int i;
	int size = buf->size, end = buf->end;

	if (gfar_ptp_is_empty(buf))
		return 1;

	i = buf->front;
	while (i != end) {
		if ((buf->data_buf + i)->key == key)
			break;
		i = gfar_ptp_calc_index(size, i, 1);
	}

	if (i == end) {
		buf->front = buf->end;
		return 1;
	}

	data->item.high = (buf->data_buf + i)->item.high;
	data->item.low = (buf->data_buf + i)->item.low;

	buf->front = gfar_ptp_calc_index(size, i, 1);

	return 0;
}

static void gfar_adj_freq(signed long freq)
{
	char new_v, old_v;
	const char max_v = TMR_MAX_DAC_V;
	const char center_v = max_v / 2;
	const long max_freq = TMR_PTPD_MAX_FREQ;

	old_v = ad5301_get_voltage();
	/* Linear interpolation for our DAC range */
	new_v = (freq * center_v) / max_freq + center_v;

	ad5301_set_voltage(new_v);
}

/* Set the 1588 timer counter registers */
static void gfar_set_1588cnt(struct net_device *dev,
			struct gfar_ptp_time *gfar_time)
{
	struct gfar_private *priv = netdev_priv(dev);
	u32 tempval;

	/* We must write the tmr_cnt_l register first */
	tempval = (u32)gfar_time->low;
	gfar_write(&(priv->regs_1588->tmr_cnt_l), tempval);
	tempval = (u32)gfar_time->high;
	gfar_write(&(priv->regs_1588->tmr_cnt_h), tempval);
}

/* Set Alarm1 register*/
static void gfar_set_alarm1(struct net_device *dev,
			struct gfar_ptp_time *gfar_time)
{
	struct gfar_private *priv = netdev_priv(dev);
	u32 tempval;
	
	/* We must write the tmr_alarm1_l register first */
	tempval = (u32)gfar_time->low;
	gfar_write(&(priv->regs_1588->tmr_alarm1_l), tempval);
	tempval = (u32)gfar_time->high;
	gfar_write(&(priv->regs_1588->tmr_alarm1_h), tempval);
}

/* AKB: Get the timestamp based on the sequence number
 * in the ioctl.  Read ID register and set time
 * if there is a match.  Otherwise time is set
 * to zero.
 */
static void gfar_get_tx_timestamp(struct ifreq *ifr,
			struct gfar __iomem *regs,
			struct gfar_ptp_time *tx_time)
{

/* Old code from Freescale
 * This is commented out and then re-written because
 * time can move both forwards and backwards
 * as when the Grandmaster takes over, its
 * time can be older than local wall clock
 * time.  This can cause the wrong TX timestamp
 * to be returned and start making bad calculations.
 *
 *
	struct gfar_ptp_time tx_set_1, tx_set_2;
	u32 tmp;

	* Read the low register first *
	tx_set_1.low = gfar_read(&regs->tmr_txts1_l);
	tx_set_1.high = gfar_read(&regs->tmr_txts1_h);

	tx_set_2.low = gfar_read(&regs->tmr_txts2_l);
	tx_set_2.high = gfar_read(&regs->tmr_txts2_h);

	tmp = 0;
	if ( tx_set_2.high > tx_set_1.high )
		tmp = 1;
	else if ( tx_set_2.high == tx_set_1.high )
		if ( tx_set_2.low > tx_set_1.low )
			tmp = 1;

	if (tmp == 0) {
		tx_time->low = tx_set_1.low;
		tx_time->high = tx_set_1.high;
	} else {
		tx_time->low = tx_set_2.low;
		tx_time->high = tx_set_2.high;
	}
*/

/* New code by Alan K. Bartky (alan@bartky.net)
 * get the time by reading the ID registers to find the
 * right one to use.
 */
	u32 timestamp_id;
	u32 sequence_number;

	sequence_number =  *((u32 *)ifr->ifr_data) & 0xFFFF;

        /*
	 * printk(KERN_INFO"\n**gfar_get_tx_timestamp: sequence: %8.8x\n",
	 *        sequence_number
	 *       );
	 */

	/* Get timestamp ID 1 from register, mask other bits and 
	 * check for a match, if there is no match, check
	 * ID 2.  If either match, read the timestamp values
	 * for that ID.  If neither match, return a timestamp
	 * of zero.
	 *
	 * The timestamp is set in the FCB at the time of
	 * transmission and for PTP is set to the 
	 * sequence number.  The user code then 
	 * asks for the correlated timestamp based 
	 * on passing the sequence number of interest in the
	 * ioctl.
	 */

	timestamp_id = gfar_read(&regs->tmr_txts1_id) & 0xFFFF;

	/*
	 * printk(KERN_INFO"\n**gfar_get_tx_timestamp: id1: %8.8x\n",
	 *        timestamp_id
	 *       );
	 */

	if      (timestamp_id == sequence_number) {
		/* Match for ID 1, get TX timestamp 1 values */
		tx_time->low  = gfar_read(&regs->tmr_txts1_l);
		tx_time->high = gfar_read(&regs->tmr_txts1_h);

		/*
		 * printk(KERN_INFO"\n**gfar_get_tx_timestamp: ts1: %8.8x%8.8x\n",
		 *        tx_time->high,
		 *        tx_time->low
		 *       );
		 */


		return;
	}
	else {
		timestamp_id = gfar_read(&regs->tmr_txts2_id) & 0xFFFF;

		/* 
		 * printk(KERN_INFO"\n**gfar_get_tx_timestamp: id2: %8.8x\n",
		 *        timestamp_id
		 *       );
		 */

		if (timestamp_id == sequence_number) {
			/* Match for ID 2, get TX timestamp 2 values */
			tx_time->low  = gfar_read(&regs->tmr_txts2_l);
			tx_time->high = gfar_read(&regs->tmr_txts2_h);

			/*
			 *printk(KERN_INFO"\n**gfar_get_tx_timestamp: ts2: %8.8x%8.8x\n",
			 *       tx_time->high,
			 *       tx_time->low
			 *      );
			 */

			return;
		}
	}
	/* No match for Timestamp, return time of zero */
	tx_time->low  = 0;
	tx_time->high = 0;

}

static void gfar_get_rx_time(struct gfar_private *priv, struct ifreq *ifr,
		struct gfar_ptp_time *rx_time, int mode)
{
	struct gfar_ptp_data_t tmp;
	int key, flag;

	key = *((int *)ifr->ifr_data);

	switch (mode) {
	case PTP_GET_RX_TIMESTAMP_SYNC:
		flag = gfar_ptp_find_and_remove(&(priv->rx_time_sync),
						key, &tmp);
		break;
	case PTP_GET_RX_TIMESTAMP_DEL_REQ:
	case PTP_GET_RX_TIMESTAMP_PDEL_REQ:	// AKB: Added for v2 support
	case PTP_GET_RX_TIMESTAMP_PDEL_RESP:	// AKB: Added for v2 support
		flag = gfar_ptp_find_and_remove(&(priv->rx_time_del_req),
						key, &tmp);
		break;
	default:
		flag = 1;
		printk(KERN_ERR "ERROR\n");
		break;
	}

	if (!flag) {
		rx_time->high = tmp.item.high;
		rx_time->low = tmp.item.low;
	} else {
		rx_time->high = rx_time->low = 0;
	}
}

static void gfar_get_curr_cnt(struct gfar __iomem *regs_1588,
			struct gfar_ptp_time *curr_time)
{
	curr_time->low = gfar_read(&(regs_1588->tmr_cnt_l));
	curr_time->high = gfar_read(&(regs_1588->tmr_cnt_h));
}


//
// gfar_adj_added: function to adjust the timer add register
// plus or minus from its default value
//
// AKB: Except for adding a test for zero to avoid doing a lot
// of math for the zero case,  I haven't otherwise modified
// this code as I've changed my PTP code to switch to directly
// I haven't modified this code as I've switched to directly 
// manipulating the add value from the PTP application user
// mode code, rather than using this function.
//
// However, based on my analysis of the code, it needs to be
// updated in how it calculates its "mid" value, in that 
// if you recompile different frequency and add values, you
// will not get the same relative clock adjustment from this call.
// Specific examples below.
//
// 10Mhz via 133Mhz sys clock examples (add value from actual ltib build is different):
// Base add value: 0x13333333
// mask =  (TMR_PTPD_MAX_FREQ-1) ..0x80000 - 1 .............0x7FFFF (524,287)
// mid = mask - (TMR_ADD_VAL & mask); 0x7FFFF - (0x33333) = 0x4CCCC (314,572)
// half_max = TMR_PTPD_MAX_FREQ............................ 0x80000 (524,288)
//
// passed value 0, 1, 10, 100, 1000, 10000 (decimal)
//
// tmp = 0      * 314,572 / 524,288 = 0     adj freq:  9,999,999.9938  (base)
// tmp = 1      * 314,572 / 524,288 = 0     adj freq:  9,999,999.9938  (no change)
// tmp = 10     * 314,572 / 524,288 = 5     adj freq: 10,000,000.1490  (+ ~0.015 PPM change)
// tmp = 100    * 314,572 / 524,288 = 59    adj freq: 10,000,001.8254  (+ ~0.183 PPM change)
// tmp = 1,000  * 314,572 / 524,288 = 599   adj freq: 10,000,018.5892  (+ ~1.860 PPM change)
// tmp = 10,000 * 314,572 / 524,288 = 5999  adj freq: 10,000,186.2273  (+ ~18.62 PPM change


// 125Mhz via 133Mhz sys clock examples:
// Base add value: 0xF0000000
// mask =  (TMR_PTPD_MAX_FREQ-1) ..0x80000 - 1 .............0x7FFFF (524,287)
// mid = mask - (TMR_ADD_VAL & mask); 0x7FFFF - (0x00000) = 0x7FFFF (524,827) (not really "mid")
// half_max = TMR_PTPD_MAX_FREQ............................ 0x80000 (524,288)
//
// passed value 0, 1, 10, 100, 1000, 10000 (decimal)
//
// tmp = 0      * 524,287 / 524,288 = 0     adj freq: 100,000,000.00000  (base)
// tmp = 1      * 524,287 / 524,288 = 0     adj freq: 100,000,000.00000  (no change)
// tmp = 10     * 524,287 / 524,288 = 9     adj freq: 100,000,000.27940  (+ ~0.0027 PPM change)
// tmp = 100    * 524,287 / 524,288 = 99    adj freq: 100,000,003.07336  (+ ~0.0307 PPM change)
// tmp = 1,000  * 524,287 / 524,288 = 999   adj freq: 100,000,031.01304  (+ ~0.3101 PPM change)
// tmp = 10,000 * 524,287 / 524,288 = 9999  adj freq: 100,000,310.40981  (+ ~3.1041 PPM change)


static void gfar_adj_addend(struct gfar __iomem *regs_1588, signed long adj)
{
	unsigned long	new;                               // 10Mhz via 133Mhz sys clock examples:
	const long	mask = (TMR_PTPD_MAX_FREQ-1);      // 0x7FFFF
	const long 	mid = mask - (TMR_ADD_VAL & mask); // 0x7FFFF - (0x33333) = 0x4CCCC
	const long	half_max = TMR_PTPD_MAX_FREQ;      // 0x80000
	signed long	tmp;

	new = (unsigned long)TMR_ADD_VAL;
	if (adj != 0)  // AKB: Added test for zero, no sense doing a lot of math for zero case
	{
		/* Do a linear interpolation */
		tmp = (((long long)adj) * ((long long)mid)) / ((long long)half_max);
		new += tmp;
	}

	gfar_write(&regs_1588->tmr_add, new );

}

static void gfar_set_period(struct gfar __iomem *regs_1588, unsigned long new_period)
{
	/*
	 * Alan K. Bartky, alan@bartky.net
	 * Function to allow user application to set the period
	 * to complement the function that allows direct setting
	 * of the addend.  This allows the user application to set
	 * other effective PTP timer frequencies besides 10 Mhz.
	 */

	unsigned long tmr_ctrl_tclk_prd;  // Variable instead of constant

	// Shift parameter passed to correct location and mask off other bits

        tmr_ctrl_tclk_prd = (new_period << 16) & TMR_CTRL_TCLK_MASK;

	// Read the current register, "or" in the new period and write back out

/* AKB: Was seeing timer anomalies so added a soft reset to the original Freescale code 
 *

	* AKB: Disable the clock 
	gfar_write(&(regs_1588->tmr_ctrl),
		(gfar_read(&(regs_1588->tmr_ctrl))
		 & ~TMR_CTRL_ENABLE));

	* AKB: Put timer engine in reset (TMSR bit on) *
	gfar_write(&(regs_1588->tmr_ctrl),
		(gfar_read(&(regs_1588->tmr_ctrl))
		 | TMR_CTRL_TMSR));
*/

	/* Set new timer clock period */
	gfar_write(&regs_1588->tmr_ctrl,            // MPC831X timer control register
	           (gfar_read(&regs_1588->tmr_ctrl) // Read current
	            & ~TMR_CTRL_TCLK_MASK           // Mask out current period value
                   )
	           | tmr_ctrl_tclk_prd              // Set new period value
	          );

/*
	* AKB: Release Timer from reset (TMSR bit ff) *
	gfar_write(&(regs_1588->tmr_ctrl),
		(gfar_read(&(regs_1588->tmr_ctrl))
		 & ~TMR_CTRL_TMSR));

*/

#if defined(CONFIG_GFAR_PTP_VCO)
/*
	* Select 1588 Timer source and enable module for starting Tmr Clock *
	gfar_write(&(regs_1588->tmr_ctrl),
		gfar_read(&(regs_1588->tmr_ctrl)) |
		TMR_CTRL_ENABLE | TMR_CTRL_EXT_CLK );
*/
#else
/*
	* Select 1588 Timer source and enable module for starting Tmr Clock *
	gfar_write(&(regs_1588->tmr_ctrl),
		gfar_read(&(regs_1588->tmr_ctrl)) |
		TMR_CTRL_ENABLE | TMR_CTRL_SYS_CLK );
*/
#endif

}

/* IOCTL Handler for PTP Specific IOCTL Commands coming from PTPD Application */
int gfar_1588_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = priv->regs;
	struct gfar_ptp_time *cnt, *alarm1_val;
	signed long *freq, *p_addend, *fiper1_val, addend;
	unsigned long *ulong_ptr;
	struct gfar_ptp_time rx_time, tx_time, curr_time;

	switch (cmd) {
	case PTP_GET_RX_TIMESTAMP_SYNC:
	case PTP_GET_RX_TIMESTAMP_DEL_REQ:
	case PTP_GET_RX_TIMESTAMP_PDEL_REQ:	// AKB: Added for v2
	case PTP_GET_RX_TIMESTAMP_PDEL_RESP:	// AKB: Added for v2
		gfar_get_rx_time(priv, ifr, &rx_time, cmd);
		copy_to_user(ifr->ifr_data, &rx_time, sizeof(rx_time));
		break;

	case PTP_GET_TX_TIMESTAMP:
		gfar_get_tx_timestamp(ifr, regs, &tx_time); /* AKB: Add ifr parameter */
		copy_to_user(ifr->ifr_data, &tx_time, sizeof(tx_time));
		break;

	case PTP_GET_CNT:
		gfar_get_curr_cnt(priv->regs_1588, &curr_time);
		copy_to_user(ifr->ifr_data, &curr_time, sizeof(curr_time));
		break;

	case PTP_SET_CNT:
		cnt = (struct gfar_ptp_time *)ifr->ifr_data;
		gfar_set_1588cnt(dev, cnt);
		break;

	case PTP_ADJ_FREQ:
		freq = (signed long *)ifr->ifr_data;
		gfar_adj_freq(*freq);
		break;

	case PTP_ADJ_ADDEND:
		p_addend = (signed long *)ifr->ifr_data;
		gfar_adj_addend(priv->regs_1588, *p_addend);
		break;

	case PTP_ADJ_ADDEND_IXXAT:
		p_addend = (signed long *)ifr->ifr_data;
		/* assign new value directly */
		gfar_write(&(priv->regs_1588->tmr_add), *p_addend);

		// AKB: TEMP FOR DEBUG: Make sure write to add register was OK
/*
		addend = gfar_read(&(priv->regs_1588->tmr_add));
		if (addend != *p_addend)
		{
			printk(KERN_ERR 
			       "gfar1588: ERROR in add register wrote %8.8lx, read: %8.8lx\n",
			       *p_addend,
			       addend
			      );
		}
*/
		break;

	case PTP_GET_ADDEND:
		addend = TMR_ADD_VAL;
		/* return initial timer base add value
		 * to calculate drift correction */
		copy_to_user(ifr->ifr_data, &addend, sizeof(addend));
		break;

	case PTP_SET_ALARM1:
		alarm1_val = (struct gfar_ptp_time *)ifr->ifr_data;
		gfar_set_alarm1(dev, alarm1_val);
		break;

	case PTP_SET_FIPER1:
		fiper1_val = (signed long *)ifr->ifr_data;
		gfar_write(&(priv->regs_1588->tmr_fiper1), *fiper1_val);
		break;
		
	case PTP_SET_PERIOD:
		/* AKB: Added the ability to change the period as well as the addend
		 * from the user application.
	         */
		ulong_ptr = (unsigned long *)ifr->ifr_data;
		gfar_set_period(priv->regs_1588, *ulong_ptr);
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

/* Function to initialize Filer Entry
 * far: Offset in Filer Table in SRAM
 * fcr: Filer Control register value corresponds to table entry
 * fpr: Filer Entry Value
 */
inline void gfar_write_filer(struct net_device *dev, unsigned int far,
			unsigned int fcr, unsigned int fpr)
{
	struct gfar_private *priv = netdev_priv(dev);
	gfar_write(&priv->regs->rqfar, far);
	gfar_write(&priv->regs->rqfcr, fcr);
	gfar_write(&priv->regs->rqfpr, fpr);
}

/* 1588 Module intialization and filer table populating routine*/
void gfar_1588_start(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);

	/* Alan K. Bartky, updated/corrected comments for original
	 * filer code from Freescale and added rule for 802.1AS
	 * PTP detection as well and changed to allow unicast
         * IPv4/UDP packets (removed address checkign) 
	 * as well as only checking for IPv4/UDP
         * that need to be timestamped.
	 */

	/* Begin, set filer rules for this port for queue 0 */
	/* Set Arbitrary Field register for IEEE 1588 version 1 PTP packet type offset */

	gfar_write(&priv->regs->rbifx,0xE8);

	/* First set of filer rules for IEEE 1588 over IP packets */

	/* File only on port number 0x13f which is the Event port address/socket.
         * General port address is not filed as those packets do not need to be timestamped
         */
	gfar_write_filer(dev,0x00,0x80,0xffff);   /* 16 bit Mask */
	gfar_write_filer(dev,0x01,0x8f,0x013f);   /* Port ID == 0x013F (319)*/

	/* File if Ethertype field is ipv4 packet */
	gfar_write_filer(dev,0x02,0x87,0x0800);   /* Ethertype == 0x0800 */

	/* File is protocol field in UDP 0x11 (17) */
	gfar_write_filer(dev,0x03,0x80,0xff); /* 8 bit mask */
	gfar_write_filer(dev,0x04,0x8b,0x11); /* IP protocol ID == 0x11 (17) for UDP */

	/* Extra validity check:
	 * Check PTP Packet type being less than 6 i.e. only SYNC, 
	 * Delay_Req, Follow-up, Delay-res, Management or Version 2 "All other"
	 * Packet types are filed with this rule.  
	 * This is the last rule of the set.
	 */
	gfar_write_filer(dev,0x05,0x62,0x06); /* Arbitrary field < 6, last rule of set */


	/* AKB: added filer rules set for 802.1as
	 * Check for proper Ethertype
	 */
	gfar_write_filer(dev,0x06,0x80,0xffff);  /* 16 bit mask */
	gfar_write_filer(dev,0x07,0x07,0x88F7);  /* Ethertype == 0x88F7, last rule of set */

	/*
	 * Default Rule to put all other frames in receive queue 1.
	 * Any receive frames filed in that queue will not even be
	 * checked to see if a timestamp is needed to be recorded
	 * and queued during the receive frame processing in gianfar.c
	 */

#define MAX_FILER_INDEX 0x8  /* Max value for clearing in gfar_1588_stop */

	gfar_write_filer(dev,MAX_FILER_INDEX,0x420,0x0);

	/* begin Original Freescale filer setup code *

	 * Arbitrary Field register *
	gfar_write(&priv->regs->rbifx, 0xE8);

	 * File on port number 13f to 141 *  THIS COMMENT IS INCORRECT SHOULD BE 13F TO 140
	gfar_write_filer(dev, 0x0, 0x80, 0xffff);
	gfar_write_filer(dev, 0x1, 0xaf, 0x13f);
	gfar_write_filer(dev, 0x2, 0xef, 0x141);
	 * File if type field is ipv4 packet *
	gfar_write_filer(dev, 0x3, 0x87, 0x800);

	 * File for mulitcast address in range from
	 * 224.0.1.129 to 224.0.1.132 *
	gfar_write_filer(dev, 0x4, 0x80, 0xffffffff);
	gfar_write_filer(dev, 0x5, 0xac, 0xe0000181);
	gfar_write_filer(dev, 0x6, 0xec, 0xe0000185);

	 * File is protocol field in UDP *
	gfar_write_filer(dev, 0x7, 0x80, 0xff);
	gfar_write_filer(dev, 0x8, 0x8b, 0x11);

	 * Check for PTP Packet type being less than 5 i.e. only SYNC,
	 * Delay_Req, Follow-up, Delay-res, Management Packet types are
	 * filed with this rule
	 *
	gfar_write_filer(dev, 0x9, 0x62, 0x5);
	gfar_write_filer(dev, 0xa, 0x80, 0xffff);
	gfar_write_filer(dev, 0xb, 0x87, 0x800);

	 * Accept IGMP query messages as well *
	gfar_write_filer(dev, 0xc, 0x80, 0xffffffff);
	gfar_write_filer(dev, 0xd, 0xc, 0xe0000001);
	 * Default Rule to file all non-PTP packets in Queue 1*
	gfar_write_filer(dev, 0xe, 0x420, 0x0);

	end original Freescale filer setup code */

	/* Set timer prescaler */
	/* commented out for test 
	gfar_write(&(priv->regs_1588->tmr_prsc), TMR_PRSC);
	*/

	/* Need to mask the TCLK bits as they are initialized with 1 */
	gfar_write(&(priv->regs_1588->tmr_ctrl),
		(gfar_read(&(priv->regs_1588->tmr_ctrl))
		 & ~TMR_CTRL_TCLK_MASK) | TMR_CTRL_TCLK_PRD );

	/* Set timer Add value to nominal default nominal frequence (see gianfar.h) */
	gfar_write(&(priv->regs_1588->tmr_add), TMR_ADD_VAL);

#if defined(CONFIG_GFAR_PTP_VCO)
	/* Select 1588 Timer source and enable module for starting Tmr Clock */
        /* AKB: Added FIPER_START from 8315E code */
	gfar_write(&(priv->regs_1588->tmr_ctrl),
		gfar_read(&(priv->regs_1588->tmr_ctrl)) |
		TMR_CTRL_ENABLE | TMR_CTRL_EXT_CLK /*| TMR_CTRL_FIPER_START*/);
#else
	/* Select 1588 Timer source and enable module for starting Tmr Clock */
        /* AKB: Added FIPER_START from 8315E code */
	gfar_write(&(priv->regs_1588->tmr_ctrl),
		gfar_read(&(priv->regs_1588->tmr_ctrl)) |
		TMR_CTRL_ENABLE | TMR_CTRL_SYS_CLK /*| TMR_CTRL_FIPER_START*/ );
#endif
}

/* Cleanup routine for 1588 module.
 * When PTP is disabled this routing is called */
void gfar_1588_stop(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	unsigned int filer_addr_reg;

	/* Alan K. Bartky, added MAX_FILER_INDEX instead of 0xe */
	for (filer_addr_reg = 0; filer_addr_reg < MAX_FILER_INDEX; filer_addr_reg++) {
		gfar_write_filer(dev, filer_addr_reg, 0, 0);
	}

	gfar_write(&(priv->regs_1588->tmr_ctrl),
		gfar_read(&(priv->regs_1588->tmr_ctrl))
		& ~TMR_CTRL_ENABLE);
}
