/* src/dep/datatypes_dep.h */
/* Target specific data structures, typedefs, etc. definitions for PTP */
/* Copyright (c) 2005-2007 Kendall Correll */

/****************************************************************************/
/* Begin additional copyright and licensing information, do not remove      */
/*                                                                          */
/* This file (datattypes_dep.h) contains Modifications (updates, corrections*/
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

#ifndef DATATYPES_DEP_H
#define DATATYPES_DEP_H

typedef enum {FALSE=0, TRUE} Boolean;
typedef char                 Octet;
typedef signed char          Integer8;
typedef signed short         Integer16;
typedef signed int           Integer32;
typedef signed long long     Integer64;   // AKB: added
typedef unsigned char        UInteger8;
typedef unsigned short       UInteger16;
typedef unsigned int         UInteger32;
typedef unsigned long long   UInteger64;  // AKB: added

// AKB: Added enumeration types

typedef unsigned char        Enumeration8;
typedef unsigned short       Enumeration16;
typedef unsigned int         Enumeration32;
typedef unsigned long long   Enumeration64;

typedef struct {
  Integer32  nsec_prev;
  Integer32  y;
} offset_from_master_filter;

typedef struct {
  Integer32  nsec_prev;
  Integer32  y;
  Integer32  s_exp;
} one_way_delay_filter;

typedef struct {
  Integer32     eventSock;                  /* Event port UDP socket */
  Integer32     generalSock;                /* General port UDP socket */
  Integer32     rawSock;                    /* Raw Socket for 802.1AS operation */
  Integer32     multicastAddr;              /* IP multicast address */
  Integer32     unicastAddr;                /* Optional IP unicast destination address */
  Integer32     pdelayMulticastAddr;        /* Address for V2 PDelay messages */
  UInteger32    rawIfIndex;                 /* Interface Index of raw socket */
  char          ifName[IFNAMSIZ];           /* Interface name (e.g. "eth0") */
  unsigned char portMacAddress[6];          /* Local Hardware Port MAC address */
} NetPath;

#endif

// eof datatypes_dep.h

