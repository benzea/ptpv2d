/* maapd_startup.c */
/* Startup and initialization functions for 
 * preliminary experimental code to provide a base to implement a daemon
 * to support IEEE 1722 Multicast Address Allocation Protocol (MAAP)
 */

/****************************************************************************/
/* Begin copyright and licensing information, do not remove                 */
/*                                                                          */
/* This file (maapd_startup.c) contains original work by Alan K. Bartky     */
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

#include "../maapd.h"

MaapData *maapData;

int      maapd_output_fd;            // AKB: Added file descriptor so output file closed on exit

static void catch_close(int sig)
{
  char *s;
  
  maapdShutdown();
  
  switch(sig)
  {
  case SIGINT:
    s = "interrupt";
    break;
    
  case SIGTERM:
    s = "terminate";
    break;
    
  case SIGHUP:
    s = "hangup";
    break;
    
  default:
    s = "?";
  }
  
  NOTIFY("catch_close: shutdown on %s signal\n", s);

  /* Close output file if there is one open before exiting */

  if (output_fd != 0)
  {
     close(output_fd);
  }
  all_leds(FALSE);
  exit(0);
}

void freeMaapdMemory ()
{
  int i;
  MaapData * currentMaapdData;

  currentMaapdData = maapData;

  for (i=0; i<MAX_MAAP_PORTS; i++)
  {
    /* Free port specific data structures */
  }
  free(maapData);

}

void maapdShutdown()
{
  netShutdown(&maapData->netPath);
  freeMaapdMemory();
  all_leds(FALSE);
}


int maapParseCommandLineArguments (int           argc,
                                   char **       argv,
                                   Integer16 *   ret,
                                   MaapdRunTimeOpts * rtOpts
                              )
{
  int c;             // Current command option
  int fd = -1;       // File descriptor

  output_fd = 0;
  rtOpts->nonDaemon = FALSE; // Assume we are running in Daemon mode unless set otherwise
  rtOpts->noClose = 0;       // noclose option for daemon function (send output to file option
                             // sets this variable to 1)
  
  /* parse command line arguments */
  while( (c = getopt(argc, argv, "?cf:dDb:u:l:y:Y:m:n:k:rz:")) != -1 )
  {
    switch(c) {
    case '?':
      printf(
"\nUsage:  maapd [OPTION]\n\n"
"-?                show this page\n"
"\n"
"-c                run in command line (non-daemon) mode\n"
"-f FILE           send output to FILE\n"
"-d                display stats\n"
"-D                display stats in .csv format\n"
#ifdef MAAPD_DBG
"-z                debug level (0=none or bit mask 1:basic, 2:verbose, 4:message)\n"
#endif
"\n"
"-b NAME           bind MAAP to network interface NAME\n"
"\n"
"-y NUMBER         specify xxx interval in 2^NUMBER sec\n"
"-Y NUMBER         specify yyy interval in 2^NUMBER sec\n"
"\n"
      );
      *ret = 0;
      return 0;  // If we are here, then Help was requested and printed, return done
      
    case 'c':
      // Command mode requested, set as non daemon mode
      rtOpts->nonDaemon = TRUE;
      break;
      
    case 'f':
      // Force console output to user specified filename 
      if((fd = creat(optarg,    // filename from command line
                     S_IRUSR    // Set to read only file, user class
                    )
          ) != -1
        )
      {
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        output_fd = fd; /* Save file descriptor to close later on terminate */
        rtOpts->noClose = 1;
      }
      else
      {
        PERROR("maapdStartup: could not open output file");
      }
      break;
      
    case 'd':
//ifndef MAAPD_DBG
      // Display statisitics
      rtOpts->displayStats = TRUE;
//endif
      break;
      
    case 'D':
//ifndef MAAPD_DBG
      // Display statistics in .csv format
      rtOpts->displayStats = TRUE;
      rtOpts->csvStats     = TRUE;
//endif
      break;
      
      
    case 'b':
      // User specified bind to a specific interface
      // Clear ifaceName and copy user specified name to it
      memset( rtOpts->ifaceName, 0,      IFACE_NAME_LENGTH);
      strncpy(rtOpts->ifaceName, optarg, IFACE_NAME_LENGTH);
      break;
      
    case 'y':
      // xxx interval in 2^NUMBER seconds (message per # seconds)
      rtOpts->xxxInterval = strtol(optarg, 0, 0);
      DBGV("startup: xxxInterval = %d\n",
           rtOpts->xxxInterval
          );
      break;
      
    case 'Y':
      // yyy interval in 2^NUMBER seconds (message per # seconds)
      rtOpts->yyyInterval = strtol(optarg, 0, 0);
      DBGV("startup: yyyInterval = %d\n",
           rtOpts->announceInterval
          );
      break;


#ifdef MAAPD_DBG
    case 'z':
      // Debug level
      debugLevel = strtol(optarg, 0, 0);
      break;
#endif

    default:
      // Unknown option
      ERROR("maapParseCommandLineArguments: Unknown option: %c\n",c);
      *ret = 1;
      return 0;
    }
  }
  return 1;
}


int allocateMaapdMemory (Integer16 *ret, MaapdRunTimeOpts *rtOpts)
{
  int i;
  MaapData * currentMaapdClockData;
  DBG("allocateMaapdMemory:\n");

   // Allocate memory for maapData structure(s)
  
  maapData = (MaapData*)calloc(MAX_MAAP_PORTS, sizeof(MaapData));

  if(!maapData)
  {
    PERROR("allocateMaapdMemory: failed to allocate memory for protocol engine data");
    if (maapd_output_fd != 0)
    {
       close(maapd_output_fd);
    }
    *ret = 2;
    return 0;
  }
    DBG(" allocated %d bytes for protocol engine data\n",
        (int)sizeof(MaapData)
       );

  currentMaapdClockData = maapData;

  for (i=0; i<MAX_MAAP_PORTS; i++)
  {
    currentMaapdClockData->port_id_field = i+1;

    // Allocate space for per port record(s)

    // Skip to next Maapd main data structure

    currentMaapdClockData++;
  }
  *ret = 0;  // Return all OK :-)
  return 1;
  
}

MaapData * maapdStartup(int argc, char **argv, Integer16 *ret, MaapdRunTimeOpts *rtOpts)
{

#ifdef MAAPD_DBG
#ifdef _POSIX_TIMERS
#if _POSIX_TIMERS > 0


struct timespec     ts;
int i;

/*
                 1         2         3         4         5         6         7         8
        12345678901234567890123456789012345678901234567890123456789012345678901234567890
*/
printf("maapd: **********************************************************************\n");
printf("maapd: SNAP Networks IEEE 1722 Annex C Multicast Address Acquisition Protocol\n");
printf("maapd: Copyright(c) 2008-2010 by Alan K. Bartky, all rights reserved\n");
printf("maapd: Licensed licensed under the terms of the GNU General public license\n");
printf("maapd: version 2 as published by the Free Software Foundation\n");
printf("maapd: **********************************************************************\n");
printf("maapd: For command line option help type: maapd -?\n");
printf("maapd: **********************************************************************\n");


/*

i = clock_getres(CLOCK_REALTIME, &ts);

printf("maapd: System clock resolution is %ld seconds, %ld nanoseconds\n",
       (long int) ts.tv_sec, 
       (long int) ts.tv_nsec
      );

*/

      /* Call time */
printf("maapd: Linux System time function returns %ld seconds\n",(long int) time(NULL));
       /* Call clock_gettime */


clock_gettime(CLOCK_REALTIME, &ts);
printf("maapd: Linux System clock_gettime function returns:\n");
printf("maapd: %ld seconds and %ld nanoseconds\n", (long int) ts.tv_sec, (long int) ts.tv_nsec);


#endif
#endif
#endif


  if (!maapdParseCommandLineArguments(argc, argv, ret, rtOpts))
  {
      return 0;
  }

  // Dump command line arguments if -f option specified
  if (maapd_output_fd !=0)
  {
     for (i=0; i<argc; i++)
     {
       printf("%s%c", argv[i], (i<argc-1) ? ' ' : '\n');
     }
  }
  // Allocate memory for maapd

  if (!allocateMaapdMemory(ret, rtOpts))
  {
      return 0;
  }
  
  // If we are here in the code, then all the parsing above was OK
  // and we have allocated primary data structures in memory.
  //
  // Check if we are to run in daemon mode or not.  If so,
  // then call daemon to force the code to run in background

#ifndef MAAPD_NO_DAEMON
  if(rtOpts->nonDaemon == FALSE)
  {
    // nonDaemon is FALSE, call daemon function to run in background
    if(daemon(0,        // nochdir fixed at 0, so working directory will change to root (/)
              rtOpts->noClose   // if 0 then redirect output to /dev/null
             ) == -1)
    {
      PERROR("maapdStartup: failed to start as daemon");
      freeMaapdMemory();
      if (maapd_output_fd != 0)
      {
         close(maapd_output_fd);
      }
      *ret = 3;
      return 0;
    }
    DBG("maapdStartup: running as daemon\n");
  }
  else
  {
    DBG("maapdStartup: not running as daemon\n");
  }
#endif

  // Setup handling of termination signals
  // All are handled by the catch_close function above
  
  signal(SIGINT,  catch_close);  // Interrupt: (such as control-c) 
  signal(SIGTERM, catch_close);  // Terminate: (request to terminate)
  signal(SIGHUP,  catch_close);  // Hang-up:   (user session terminated)
  
  *ret = 0;
  DBG("maapdStartup: completed OK\n");
  return maapData;
}

// eof maapd_startup.c
