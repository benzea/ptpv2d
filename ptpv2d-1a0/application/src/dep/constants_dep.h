/* src/dep/constants_dep.h */
/* Target specific constant definitions for PTP */
/* Copyright (c) 2005-2007 Kendall Correll */

/****************************************************************************/
/* Begin additional copyright and licensing information, do not remove      */
/*                                                                          */
/* This file (constants_dep.h) contains Modifications (updates, corrections */
/* comments and addition of initial support for IEEE 1588 version 1, IEEE   */
/* version 2 and IEEE 802.1AS PTP) and other features by Alan K. Bartky     */
/*                                                                          */
/* Modifications Copyright (c) 2007-2010 by Alan K. Bartky, all rights      */
/* reserved.                                                                */
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
/*     \/  \/      email: alan@bartky.net                                   */
/*      \  /       Web: http://www.bartky.net                               */
/*       \/                                                                 */
/*                                                                          */
/* End Alan K. Bartky additional copyright notice: Do not remove            */
/****************************************************************************/

#ifndef CONSTANTS_DEP_H
#define CONSTANTS_DEP_H

/* platform dependent */

#if !defined(linux) && !defined(__NetBSD__) && !defined(__FreeBSD__)
#error Not ported to this architecture, please update.
#endif

#ifdef	linux
#include<netinet/in.h>
#include<net/if.h>
#include<net/if_arp.h>
#define IFACE_NAME_LENGTH         IF_NAMESIZE
#define NET_ADDRESS_LENGTH        INET_ADDRSTRLEN

#define IFCONF_LENGTH 10

#include <netpacket/packet.h>
#include <net/ethernet.h>       /* the L2 protocols */

#include<endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define PTPD_LSBF
#elif __BYTE_ORDER == __BIG_ENDIAN
#define PTPD_MSBF
#endif
#endif /* linux */


#if defined(__NetBSD__) || defined(__FreeBSD__)
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <net/if.h>
# include <net/if_dl.h>
# include <net/if_types.h>
# if defined(__FreeBSD__)
#  include <net/ethernet.h>
#  include <sys/uio.h>
# else
#  include <net/if_ether.h>
# endif
# include <ifaddrs.h>
# define IFACE_NAME_LENGTH         IF_NAMESIZE
# define NET_ADDRESS_LENGTH        INET_ADDRSTRLEN

# define IFCONF_LENGTH 10

# define adjtimex ntp_adjtime

# include <machine/endian.h>
# if BYTE_ORDER == LITTLE_ENDIAN
#   define PTPD_LSBF
# elif BYTE_ORDER == BIG_ENDIAN
#   define PTPD_MSBF
# endif
#endif


#ifdef CONFIG_MPC831X
#define ADJ_FREQ_MAX  524288
#else
#define ADJ_FREQ_MAX  512000
#endif

/* UDP/IPv4 dependent */

#define SUBDOMAIN_ADDRESS_LENGTH  4
#define PORT_ADDRESS_LENGTH       2

#define PACKET_SIZE  384  // AKB: Big enough for all packets and nice round binary number

#define PTP_EVENT_PORT    319  // 0x013F
#define PTP_GENERAL_PORT  320  // 0x0140

#define DEFAULT_PTP_DOMAIN_ADDRESS     "224.0.1.129"  // 0xE0-00-01-81
#define ALTERNATE_PTP_DOMAIN1_ADDRESS  "224.0.1.130"  // 0xE0-00-01-82
#define ALTERNATE_PTP_DOMAIN2_ADDRESS  "224.0.1.131"  // 0xE0-00-01-83
#define ALTERNATE_PTP_DOMAIN3_ADDRESS  "224.0.1.132"  // 0xE0-00-01-84
#define DEFAULT_PTP_PDELAY_ADDRESS     "224.0.0.107"  // 0xE0-00-00-6B

#define HEADER_LENGTH             40
#define SYNC_PACKET_LENGTH        124
#define DELAY_REQ_PACKET_LENGTH   124
#define FOLLOW_UP_PACKET_LENGTH   52
#define DELAY_RESP_PACKET_LENGTH  60
#define MANAGEMENT_PACKET_LENGTH  136

#define MM_STARTING_BOUNDARY_HOPS  0x7fff

/* others */

#define SCREEN_BUFSZ  256     // AKB: Increased to handle more stats (may cause screen wrap)
#define SCREEN_MAXSZ  144

#endif

// eof constants_dep.h
