/* src/maapd.c */
/* Preliminary experimental code to provide a base to implement a daemon
 * to support IEEE 1722 Multicast Address Allocation Protocol (MAAP)
 */

/****************************************************************************/
/* Begin copyright and licensing information, do not remove                 */
/*                                                                          */
/* This file (maapd.h) contains original work by Alan K. Bartky             */
/* Copyright (c) 2007-2010 by Alan K. Bartky, all rights reserved           */
/*                                                                          */
/* This source code and its associated software algorithms are under        */
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
/* End Alan K. Bartky copyright notice: Do not remove                       */
/****************************************************************************/

/* Multicast Address Acquisition Protocol (MAAP) Daemon (maapd) */

#include "maapd.h"

MaapdRunTimeOpts maapRtOpts;  /* statically allocated run-time configuration data */

#ifdef MAAPD_DBG
int maapDebugLevel;
#endif


int main(int argc, char **argv)
{
  MaapData *maapData;
  Integer16 ret;
  
  /* initialize run-time options to reasonable values */ 
  memset(&maapRtOpts, 0, sizeof(maapRtOpts));

#ifdef MAAPD_DBG
  maapDebugLevel = 0;
#endif

  // Multiport protocol not fully suported yet, so force default to eth1 interface 
  // which is the best port to use on the MPC8313E-RDB board

  memset( maapRtOpts.ifaceName, 0,      IFACE_NAME_LENGTH);
  strncpy(maapRtOpts.ifaceName, "eth1", IFACE_NAME_LENGTH);

  if( !(maapData = maapdStartup(argc, argv, &ret, &maapRtOpts)) )
  {
    // maapdStartup did not return a pointer to the maapData structure,
    // something went wrong
    // Return code set by maapdStartup function, pass
    // back to operating system.
    return ret;
  }

  if (maapRtOpts.ifaceName[0] == '\0')
  {
     /* do the multiple port protocol engine, if not bound to a single port */
     maapMultiPortProtocol(&maapRtOpts, maapData);
     MAAPD_NOTIFY("main: self shutdown, probably due to an error\n");
  }
  else
  {
     /* do the single port protocol engine */
     maapProtocol(&maapRtOpts, maapData);
     MAAPD_NOTIFY("main: self shutdown, probably due to an error\n");
  }
  
  maapdShutdown();
  
  return 1;
}

// eof maapd.c

