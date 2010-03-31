/* src/dep/startup.c */
/* System dependent Startup, initialization and shutdown functions for 
 * PTP daemon 
 */
/* Copyright (c) 2005-2007 Kendall Correll */

/****************************************************************************/
/* Begin additional copyright and licensing information, do not remove      */
/*                                                                          */
/* This file (startup.c) contains Modifications (updates, corrections       */
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
 
#include "../ptpd.h"

PtpClock *ptpClock;

int      output_fd;            // AKB: Added file descriptor so output file closed on exit

void catch_close(int sig)
{
  char *s;
  
  ptpdShutdown();
  
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

void freePtpdMemory ()
{
  int i;
  PtpClock * currentPtpdClockData;

  currentPtpdClockData = ptpClock;

  for (i=0; i<MAX_PTP_PORTS; i++)
  {
     if (currentPtpdClockData->foreign)
     {
        free(currentPtpdClockData->foreign);
     }
     currentPtpdClockData++;
  }
  free(ptpClock);

}

void ptpdShutdown()
{
  netShutdown(&ptpClock->netPath);
  freePtpdMemory();
  all_leds(FALSE);
}


int parseCommandLineArguments (int           argc,
                               char **       argv,
                               Integer16 *   ret,
                               RunTimeOpts * rtOpts
                              )
{
  int c;             // Current command option
  int fd = -1;       // File descriptor

  output_fd = 0;
  rtOpts->nonDaemon = FALSE; // Assume we are running in Daemon mode unless set otherwise
  rtOpts->noClose = 0;       // noclose option for daemon function (send output to file option
                             // sets this variable to 1)
  
  /* parse command line arguments */
  while( (c = getopt(argc, argv, "?cf:dDxta:w:b:u:l:o:e:hy:Y:m:gps:i:v:n:k:rz:28PH:A:R")) != -1 )
  {
    switch(c) {
    case '?':
      printf(
"\nUsage:  ptpv2d [OPTION]\n\n"
"-?                show this page\n"
"\n"
"-c                run in command line (non-daemon) mode\n"
"-f FILE           send output to FILE\n"
"-d                display stats\n"
"-D                display stats in .csv format\n"
#ifdef PTPD_DBG
"-z                debug level (0=none or bit mask 1:basic, 2:verbose, 4:message)\n"
#endif
"\n"
"-x                do not reset the clock if off by more than one second\n"
"-t                do not adjust the system clock\n"
"-a NUMBER,NUMBER  specify clock servo P and I attenuations\n"
"-w NUMBER         specify one way delay filter stiffness\n"
#ifdef CONFIG_MPC831X
"-H                specify hardware clock period in nanoseconds\n"
"-A                specify base value for clock frequency adjustment\n"
"-R                remember adjust value for slave to master transition\n"
#endif
"\n"
"-b NAME           bind PTP to network interface NAME\n"
"-u ADDRESS        also send uni-cast to ADDRESS\n"
"-2                run in PTP version 2 mode instead of version 1\n"
"-8                run in IEEE 802.1AS PTP Layer 2 mode instead of IP/UDP\n"
"-P                run Pdelay Req/Resp mechanism instead of Delay Resp/Req\n"
"-l NUMBER,NUMBER  specify inbound, outbound latency in nsec\n"
"\n"
"-o NUMBER         specify current UTC offset\n"
"-e NUMBER         specify epoch NUMBER\n"
"-h                specify half epoch\n"
"\n"
"-y NUMBER         specify sync interval in 2^NUMBER sec\n"
"-Y NUMBER         specify announce interval in 2^NUMBER sec\n"
"-m NUMBER         specify max number of foreign master records\n"
"\n"
"-g                run as slave only\n"
"-p                make this a preferred clock\n"
"-s NUMBER         specify system clock stratum\n"
"-i NAME           specify system clock identifier\n"
"-v NUMBER         specify system clock allen variance\n"
"\n"
"-n NAME           specify PTP subdomain name (not related to IP or DNS)\n"
"\n"
"-k NUMBER,NUMBER  send a management message of key, record, then exit\n"
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
        PERROR("ptpdStartup: could not open output file");
      }
      break;
      
    case 'd':
//ifndef PTPD_DBG
      // Display statisitics
      rtOpts->displayStats = TRUE;
//endif
      break;
      
    case 'D':
//ifndef PTPD_DBG
      // Display statistics in .csv format
      rtOpts->displayStats = TRUE;
      rtOpts->csvStats     = TRUE;
//endif
      break;
      
    case 'x':
      // Do not reset the system clock
      rtOpts->noResetClock = TRUE;
      break;
      
    case 't':
      // Do not adjust time
      rtOpts->noAdjust     = TRUE;
      break;
      
    case 'a':
      // clock servo P and I attenuations (ap & ai in rtOpts)
      rtOpts->ap = strtol(optarg, &optarg, 0);
      if(optarg[0])
        rtOpts->ai = strtol(optarg+1, 0, 0);
      break;
      
    case 'w':
      // one way delay filter stiffness (s in rtOpts)
      rtOpts->s = strtol(optarg, &optarg, 0);
      break;
      
    case 'b':
      // User specified bind to a specific interface
      // Clear ifaceName and copy user specified name to it
      memset( rtOpts->ifaceName, 0,      IFACE_NAME_LENGTH);
      strncpy(rtOpts->ifaceName, optarg, IFACE_NAME_LENGTH);
      break;
      
    case 'u':
      // User specified unicast IP adddress 
      strncpy(rtOpts->unicastAddress, optarg, NET_ADDRESS_LENGTH);
      break;
      
    case 'l':
      // User specified inbound and outbound latency
      rtOpts->inboundLatency.nanoseconds = strtol(optarg, &optarg, 0);
      if(optarg[0])
        rtOpts->outboundLatency.nanoseconds = strtol(optarg+1, 0, 0);
      break;
      
    case 'o':
      // Offset from UTC
      rtOpts->currentUtcOffset = strtol(optarg, &optarg, 0);
      break;
      
    case 'e':
      // Epoch number
      rtOpts->epochNumber      = strtoul(optarg, &optarg, 0);
      break;
      
    case 'h':
      // Half epoch
      rtOpts->halfEpoch = TRUE;
      break;
      
    case 'y':
      // Sync interval in 2^NUMBER seconds (message per # seconds)
      rtOpts->syncInterval = strtol(optarg, 0, 0);
      DBGV("startup: syncInterval = %d\n",
           rtOpts->syncInterval
          );
      break;
      
    case 'Y':
      // Sync interval in 2^NUMBER seconds (message per # seconds)
      rtOpts->announceInterval = strtol(optarg, 0, 0);
      DBGV("startup: syncInterval = %d\n",
           rtOpts->announceInterval
          );
      break;
      
    case 'm':
      // Maximum number of of foreign master records
      rtOpts->max_foreign_records = strtol(optarg, 0, 0);
      if(rtOpts->max_foreign_records < 1)
        rtOpts->max_foreign_records = 1;
      break;
      
    case 'g':
      // Slave only option
      rtOpts->slaveOnly = TRUE;
      break;
      
    case 'p':
      // Preferred (force to master) option
      rtOpts->clockPreferred = TRUE;
      break;
      
    case 's':
      // Clock stratum (number from 0 to 255 as specified in IEEE 1588)
      rtOpts->clockStratum = strtol(optarg, 0, 0);
      if(rtOpts->clockStratum <= 0)
        rtOpts->clockStratum = 255;
      break;
      
    case 'i':
      // Clock Identifier name (string)
      memset( rtOpts->clockIdentifier, 0,      PTP_CODE_STRING_LENGTH);
      strncpy(rtOpts->clockIdentifier, optarg, PTP_CODE_STRING_LENGTH);
      break;
      
    case 'v':
      // Clock variance
      rtOpts->clockVariance = strtol(optarg, 0, 0);
      break;
      
    case 'n':
      // PTP subdomain name
      memset( rtOpts->subdomainName, 0,      PTP_SUBDOMAIN_NAME_LENGTH);
      strncpy(rtOpts->subdomainName, optarg, PTP_SUBDOMAIN_NAME_LENGTH);
      break;
      
    case 'k':
      // send a management message of key, record, then exit
      rtOpts->probe = TRUE;
      
      rtOpts->probe_management_key = strtol(optarg, &optarg, 0);
      if(optarg[0])
        rtOpts->probe_record_key = strtol(optarg+1, 0, 0);
      
      rtOpts->nonDaemon = TRUE;   // Do not run as daemon to send management message,
                                  // run in command mode
      break;
      
    case 'r':
      ERROR("The '-r' option has been removed because it is now the default behaviour.\n");
      ERROR("Use the '-x' option to disable clock resetting.\n");
      *ret = 1;
      return 0;

#ifdef PTPD_DBG
    case 'z':
      // Debug level
      debugLevel = strtol(optarg, 0, 0);
      break;
#endif

    case '2':
      // run in 802.1AS mode instead of 1588 IP/UDP
      rtOpts->ptpv2 = TRUE;
      break;

    case '8':
      // run in 802.1AS mode instead of 1588 IP/UDP
      rtOpts->ptp8021AS = TRUE;
      break;


    case 'P':
      // run in PDelay Req/Resp/Follow instead of Delay Req/Resp mode
      rtOpts->pdelay = TRUE;
      break;

    case 'H':
#ifdef CONFIG_MPC831X
      // Set Hardware clock period in nanoseconds
      rtOpts->hwClockPeriod = strtoul(optarg, &optarg, 0);
#endif
      break;
      

    case 'A':
#ifdef CONFIG_MPC831X
      // Set Hardware clock period in nanoseconds
      rtOpts->baseAdjustValue = strtol(optarg, &optarg, 0);
#endif
      break;

    case 'R':
#ifdef CONFIG_MPC831X
      // Set Hardware clock period in nanoseconds
      rtOpts->rememberAdjustValue = TRUE;
#endif
      break;

    default:
      // Unknown option
      ERROR("parseCommandLineArguments: Unknown option: %c\n",c);
      *ret = 1;
      return 0;
    }
  }
  return 1;
}


int allocatePtpdMemory (Integer16 *ret, RunTimeOpts *rtOpts)
{
  int i;
  PtpClock * currentPtpdClockData;
  DBG("allocatePtpdMemory:\n");

   // Allocate memory for ptpClock structure(s)
  
  ptpClock = (PtpClock*)calloc(MAX_PTP_PORTS, sizeof(PtpClock));

  if(!ptpClock)
  {
    PERROR("allocatePtpdMemory: failed to allocate memory for protocol engine data");
    if (output_fd != 0)
    {
       close(output_fd);
    }
    *ret = 2;
    return 0;
  }
    DBG(" allocated %d bytes for protocol engine data\n",
        (int)sizeof(PtpClock)
       );

  currentPtpdClockData = ptpClock;

  for (i=0; i<MAX_PTP_PORTS; i++)
  {
    // Allocate space for foreign master record(s)

    currentPtpdClockData->foreign 
        = (ForeignMasterRecord*)calloc(rtOpts->max_foreign_records,
                                       sizeof(ForeignMasterRecord)
                                      );
    if(!currentPtpdClockData->foreign)
    {
      PERROR("allocatePtpdMemory: failed to allocate memory for foreign master data");
      *ret = 2;
      if (output_fd != 0)
      {
         close(output_fd);
      }
      freePtpdMemory();
      return 0;
    }
    else
    {
      DBG(" allocated %d bytes for foreign master data\n",
          (int)(rtOpts->max_foreign_records*sizeof(ForeignMasterRecord))
         );
      currentPtpdClockData->port_id_field = i+1;
      DBGV(" currentPtpdClockData: %p, Port ID: %d\n",
           currentPtpdClockData,
           currentPtpdClockData->port_id_field
          );
    }
    currentPtpdClockData++;
  }
  *ret = 0;  // Return all OK :-)
  return 1;
  
}

PtpClock * ptpdStartup(int argc, char **argv, Integer16 *ret, RunTimeOpts *rtOpts)
{

#ifdef PTPD_DBG
#ifdef _POSIX_TIMERS
#if _POSIX_TIMERS > 0


struct timespec     ts;
int i;

/*
                 1         2         3         4         5         6         7         8
        12345678901234567890123456789012345678901234567890123456789012345678901234567890
*/
printf("ptpv2d: **********************************************************************\n");
printf("ptpv2d: IEEE 1588 & IEE 802.1AS Precision Time Protocol Daemon\n");
printf("ptpv2d: Licensed licensed under the terms of the GNU General public license\n");
printf("ptpv2d: version 2 as published by the Free Software Foundation\n");
printf("ptpv2d: **********************************************************************\n");
printf("ptpv2d: For command line option help type: ptpv2d -?\n");
printf("ptpv2d: **********************************************************************\n");


/*

i = clock_getres(CLOCK_REALTIME, &ts);

printf("ptpv2d: System clock resolution is %ld seconds, %ld nanoseconds\n",
       (long int) ts.tv_sec, 
       (long int) ts.tv_nsec
      );

*/

      /* Call time */
printf("ptpv2d: Linux System time function returns %ld seconds\n",(long int) time(NULL));
       /* Call clock_gettime */


clock_gettime(CLOCK_REALTIME, &ts);
printf("ptpv2d: Linux System clock_gettime function returns:\n");
printf("ptpv2d: %ld seconds and %ld nanoseconds\n", (long int) ts.tv_sec, (long int) ts.tv_nsec);


#endif
#endif
#endif


  if (!parseCommandLineArguments(argc, argv, ret, rtOpts))
  {
      return 0;
  }

  // Dump command line arguments if -f option specified
  if (output_fd !=0)
  {
     for (i=0; i<argc; i++)
     {
       printf("%s%c", argv[i], (i<argc-1) ? ' ' : '\n');
     }
  }
  // Allocate memory for ptpd

  if (!allocatePtpdMemory(ret, rtOpts))
  {
      return 0;
  }
  
  // If we are here in the code, then all the parsing above was OK
  // and we have allocated primary data structures in memory.
  //
  // Check if we are to run in daemon mode or not.  If so,
  // then call daemon to force the code to run in background

#ifndef PTPD_NO_DAEMON
  if(rtOpts->nonDaemon == FALSE)
  {
    // nonDaemon is FALSE, call daemon function to run in background
    if(daemon(0,        // nochdir fixed at 0, so working directory will change to root (/)
              rtOpts->noClose   // if 0 then redirect output to /dev/null
             ) == -1)
    {
      PERROR("ptpdStartup: failed to start as daemon");
      freePtpdMemory();
      if (output_fd != 0)
      {
         close(output_fd);
      }
      *ret = 3;
      return 0;
    }
    DBG("ptpdStartup: running as daemon\n");
  }
  else
  {
    DBG("ptpdStartup: not running as daemon\n");
  }
#endif

  // Setup handling of termination signals
  // All are handled by the catch_close function above
  
  signal(SIGINT,  catch_close);  // Interrupt: (such as control-c) 
  signal(SIGTERM, catch_close);  // Terminate: (request to terminate)
  signal(SIGHUP,  catch_close);  // Hang-up:   (user session terminated)
  
  *ret = 0;
  DBG("ptpdStartup: completed OK\n");
  return ptpClock;
}

// eof startup.c

