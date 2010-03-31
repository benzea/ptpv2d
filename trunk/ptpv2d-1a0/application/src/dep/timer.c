/* sys/dep/timer.c */
/* System dependent Functions to handle PTP timers (such as protocol timers
 * to map them to system timer functions
 */
/* Copyright (c) 2005-2007 Kendall Correll */

/****************************************************************************/
/* Begin additional copyright and licensing information, do not remove      */
/*                                                                          */
/* This file (timer.c) contains Modifications (updates, corrections         */
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

#define TIMER_INTERVAL 1
int elapsed[MAX_PTP_PORTS];

#ifdef LIMIT_RUNTIME
/* Variables to allow for limited run time (i.e. terminate the
 * system after a certain amount of time since the daemon
 * was started
 */
int max_ticks=14400; /*14400 4 hours in seconds (60 *60 * 4) */ 
int license_tick_counter = 0;
#endif

void catch_alarm(int sig)
{
  int i;

  for (i=0; i<MAX_PTP_PORTS; i++)
  {
    elapsed[i] += TIMER_INTERVAL;
  }
  // NOTE: If you put in a debug print here, it will interleave with other messages
  // from the normal execution task.  So if you need to put one in, put it
  // in to verify it, but remove it once you verify timeout is occurring OK.
/* ifdef PTPD_DBG
  if (debugLevel >= 1) 
  {
    DBGV("\n(***tick***)\n");
    fflush(NULL);
  }
 endif
*/
  
  /*  fprintf(stderr, "tick: %d, %d\n",max_ticks,license_tick_counter); */

#ifdef LIMIT_RUNTIME
 /* Check Limit run timer */
 if (++license_tick_counter > max_ticks)
 {
    fprintf(stderr, "\nptpv2d: Evaluation license maximum run time reached, terminating!\n"); 
    raise(SIGTERM); // Time up, send a terminate signal to the ptpv2d task
 }
 /* End run time limit timer*/
#endif

}

void initTimer(Integer32  seconds,
               UInteger32 microseconds
              )
{
  struct itimerval itimer;
  
  DBG("initTimer: %d seconds, %u microseconds\n",
      seconds,
      microseconds
     );
  
  // Set Alarm clock signal to ignore
  signal(SIGALRM, SIG_IGN);

  int i;

  for (i=0; i<MAX_PTP_PORTS; i++)
  {
      elapsed[i] = 0;
  }

#ifdef LIMIT_RUNTIME
  // Adjust run time limit timer if timer less than one second per tick
  if (seconds == 0)
  {
    max_ticks *= (1000000/microseconds);
  }
#endif

  // Set interval timer values  
  itimer.it_value.tv_sec  = itimer.it_interval.tv_sec  = seconds;
  itimer.it_value.tv_usec = itimer.it_interval.tv_usec = microseconds;
  
  // Set Alarm clock signal to catch_alarm function above
  signal(SIGALRM, catch_alarm);

  // Set and start interval timer
  setitimer(ITIMER_REAL,  // Use real time clock (do not use time of day)
            &itimer,      // Pointer to new itimerval structure 
            0             // Pointer to old structure (none in this case)
           );  
}

void timerUpdate(IntervalTimer *itimer, int port_id)
{
  int i, delta;
  
  i = port_id -1;
  delta   = elapsed[i];
  elapsed[i] = 0;
  
  if(delta <= 0)
    return;
  
  for(i = 0; i < TIMER_ARRAY_SIZE; ++i)
  {
    if(itimer[i].interval > 0 && (itimer[i].left -= delta) <= 0)
    {
      itimer[i].left   = itimer[i].interval;
      itimer[i].expire = TRUE;
      DBGV("timerUpdate: timer index %u expired (interval=%d delta=%d)\n",
           i,
           itimer[i].interval,
           delta
          );
    }
  }
}

void timerStop(UInteger16 index, IntervalTimer *itimer)
{
  if(index >= TIMER_ARRAY_SIZE)
    return;
  
  itimer[index].interval = 0;
  DBGV("timerStop: timer index %u stopped\n", index);
}

void timerStart(UInteger16 index, UInteger16 interval, IntervalTimer *itimer)
{
  if(index >= TIMER_ARRAY_SIZE)
    return;
  
  itimer[index].expire   = FALSE;
  itimer[index].left     = interval;
  itimer[index].interval = itimer[index].left;
  
  DBGV("timerStart: set timer index %d to %d\n", index, interval);
}

Boolean timerExpired(UInteger16 index, IntervalTimer *itimer, int port_id)
{
  DBGV("timerExpired: Checking index %d\n", index);
  timerUpdate(itimer, port_id);
  
  if(index >= TIMER_ARRAY_SIZE)
    return FALSE;                // ERROR:index out of range, return false
  
  if(!itimer[index].expire)
    return FALSE;                // Timer not expired, return FALSE
  
  DBGV("timerExpired: timer index %d has expired\n", index);
  itimer[index].expire = FALSE;  // Clear expired flag
  return TRUE;                   // Return TRUE
}

// eof timer.c
