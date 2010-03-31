/* src/dep/sys.c */
/* Misc. system dependent functions, mainly time related and also contains
 * function to display statistics in printable format or for output 
 * in .csv format for export to spreadsheet programs
 */
/* Copyright (c) 2005-2007 Kendall Correll */

/****************************************************************************/
/* Begin additional copyright and licensing information, do not remove      */
/*                                                                          */
/* This file (sys.c) contains Modifications (updates, corrections           */
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
#ifdef CONFIG_MPC831X
#include "../mpc831x.h"
#endif

void displayStats(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
  static int  start = 1;
  static char sbuf[SCREEN_BUFSZ];
  char *      s;
  int         len = 0;
  
  if(start && rtOpts->csvStats)
  {
    start = 0;
#ifdef CONFIG_MPC831X
    printf("state, one way delay, offset from master, drift, timer add value, sync send time, sync receive time, master to slave delay\n");
#else
    printf("state, one way delay, offset from master, drift, sync send time, sync receive time, master to slave delay\n");
#endif
    fflush(stdout);
  }
  
  memset(sbuf, ' ', SCREEN_BUFSZ);
  
  switch(ptpClock->port_state)
  {
  case PTP_INITIALIZING:  s = "init";  break;
  case PTP_FAULTY:        s = "flt ";  break;
  case PTP_LISTENING:     s = "lstn";  break;
  case PTP_PASSIVE:       s = "pass";  break;
  case PTP_UNCALIBRATED:  s = "uncl";  break;
  case PTP_SLAVE:         s = "slv ";  break;
  case PTP_PRE_MASTER:    s = "pmst";  break;
  case PTP_MASTER:        s = "mst ";  break;
  case PTP_DISABLED:      s = "dsbl";  break;
  default:                s = "?   ";  break;
  }
  
  if (!rtOpts->csvStats)
  {
    len += sprintf(sbuf + len, "\rstate: ");
  }

  len += sprintf(sbuf + len, "%s", s);
  
  if(ptpClock->port_state == PTP_SLAVE)
  {

    len += sprintf(sbuf + len,
                   ", %s%s%d.%09d",
                   rtOpts->csvStats ? "" : "owd: ",
                   (ptpClock->one_way_delay.nanoseconds < 0) ? "-" : " ",
                   ptpClock->one_way_delay.seconds,
                   abs(ptpClock->one_way_delay.nanoseconds)
                  );

    len += sprintf(sbuf + len,
                   ", %s%s%d.%09d",
                   rtOpts->csvStats ? "" : "ofm: ",
                   (ptpClock->offset_from_master.nanoseconds < 0) ? "-" : " ",
                   ptpClock->offset_from_master.seconds,
                   abs(ptpClock->offset_from_master.nanoseconds)
                  );
    
    len += sprintf(sbuf + len, 
                   ", %s%d",
                   rtOpts->csvStats ? "" : "drift: ",
                   ptpClock->observed_drift
                  );

/* Variance not supported, comment statistic out
    len += sprintf(sbuf + len, 
                   ", %s%d",
                   rtOpts->csvStats ? "" : "var: ",
                   ptpClock->observed_v1_variance
                  );
*/


#ifdef CONFIG_MPC831X
    len += sprintf(sbuf + len,
                   ", %s%10.10u",
                   rtOpts->csvStats ? "" : "clock_add: ",
                   current_timer_add_value
                  );
#endif

    // AKB: Added additional stats for Sync Transmit and one way delay


    len += sprintf(sbuf + len,
                   ", %s%s%d.%09d",
                   rtOpts->csvStats ? "" : "sst: ",
                   (ptpClock->t1_sync_tx_time.nanoseconds < 0) ? "-" : " ",
                   ptpClock->t1_sync_tx_time.seconds,
                   abs(ptpClock->t1_sync_tx_time.nanoseconds)
                  );

    len += sprintf(sbuf + len,
                   ", %s%s%d.%09d",
                   rtOpts->csvStats ? "" : "srt: ",
                   (ptpClock->t2_sync_rx_time.nanoseconds < 0) ? "-" : " ",
                   ptpClock->t2_sync_rx_time.seconds,
                   abs(ptpClock->t2_sync_rx_time.nanoseconds)
                  );

    len += sprintf(sbuf + len,
                   ", %s%s%d.%09d",
                   rtOpts->csvStats ? "" : "msd: ",
                   (ptpClock->master_to_slave_delay.nanoseconds < 0) ? "-" : " ",
                   ptpClock->master_to_slave_delay.seconds,
                   abs(ptpClock->master_to_slave_delay.nanoseconds)
                  );


  }

  if (rtOpts->csvStats)
  {
    len += sprintf(sbuf + len, "\n");
  }

  
  write(1, sbuf, rtOpts->csvStats ? len : SCREEN_MAXSZ + 1);
}

Boolean nanoSleep(TimeInternal *t)
{
  struct timespec ts, tr;
  
  ts.tv_sec  = t->seconds;
  ts.tv_nsec = t->nanoseconds;
  
  if(nanosleep(&ts, &tr) < 0)
  {
    t->seconds     = tr.tv_sec;
    t->nanoseconds = tr.tv_nsec;
    return FALSE;
  }
  
  return TRUE;
}


void setPtpTimeFromSystem(Integer16 utc_offset)
{

#ifdef CONFIG_MPC831X
  struct timeval tv;
  TimeInternal   time;

  gettimeofday(&tv, 0);

  time.seconds     =  tv.tv_sec;
  time.nanoseconds =  tv.tv_usec*1000;

  /* PTP uses TAI, gettime of day is UTC, so adjust */
  time.seconds     += utc_offset;

  mpc831x_set_curr_time(&time);

  NOTIFY("setPtpTimeFromSystem to TAI: %ds %dns\n",
         time.seconds,
         time.nanoseconds
        );
#endif
}

void setSystemTimeFromPtp(Integer16 utc_offset)
{
#ifdef CONFIG_MPC831X
  struct timeval tv;
  TimeInternal   time;

  mpc831x_get_curr_time(&time);
  
  tv.tv_sec  =  time.seconds;
  tv.tv_usec =  time.nanoseconds/1000;

  /* PTP uses TAI, gettime of day is UTC, so adjust */
  tv.tv_sec  -= utc_offset;

  settimeofday(&tv, 0);

  NOTIFY("setSystemTimeFromPtp to TAI: %ds %dns\n",
         time.seconds,
         time.nanoseconds
        );
#endif
}





void getTime(TimeInternal *time, Integer16 utc_offset)
{
#ifdef CONFIG_MPC831X
  mpc831x_get_curr_time(time);
#else
  struct timeval tv;
  
  gettimeofday(&tv, 0);

  time->seconds     =  tv.tv_sec;
  time->nanoseconds =  tv.tv_usec*1000;

  /* PTP uses TAI, gettime of day is UTC, so adjust */
  time->seconds     += utc_offset;

#endif
}


/* AKB: 1/31/08, Changed set time to set both the MPC8313 HW clock and also the
 * Linux time of day clock 
 */
void setTime(TimeInternal *time, Integer16 utc_offset)
{
  struct timeval tv;

#ifdef CONFIG_MPC831X
  mpc831x_set_curr_time(time);
#endif

  
  tv.tv_sec  = time->seconds;
  tv.tv_usec = time->nanoseconds/1000;

  /* PTP uses TAI, gettime of day is UTC, so adjust */
  tv.tv_sec  -= utc_offset;
  settimeofday(&tv, 0);
  
  NOTIFY("setTime: resetting clock to TAI %ds %dns\n", time->seconds, time->nanoseconds);
}

UInteger16 getRand(UInteger32 *seed)
{
  return rand_r((unsigned int*)seed);
}

short temp_debug_max_adjustments=0;

Boolean adjFreq(Integer32 adj)
{
#ifndef CONFIG_MPC831X
  struct timex t;
#endif
  
  if(adj > ADJ_FREQ_MAX)
    adj = ADJ_FREQ_MAX;
  else if(adj < -ADJ_FREQ_MAX)
    adj = -ADJ_FREQ_MAX;
  
#ifdef CONFIG_MPC831X
  if (++temp_debug_max_adjustments < 10000)
    mpc831x_adj_addend(adj);
  return (TRUE);
#else
  t.modes = MOD_FREQUENCY;
  t.freq = adj*((1<<16)/1000);

  return !adjtimex(&t);
#endif
}

// eof sys.c

