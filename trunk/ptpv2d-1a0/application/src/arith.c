/* src/arith.c */
/* General arithmetic routines for PTP */
/* Copyright (c) 2005-2007 Kendall Correll */

/****************************************************************************/
/* Begin additional copyright and licensing information, do not remove      */
/*                                                                          */
/* This file (arith.c) contains Modifications (updates, corrections         */
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

/* AKB: 2010-09-25: Added doxygen style comments */

/**
 * @file arith.c
 * Miscellaneous arithmetic subroutines and functions used
 * by ptpv2d software.
 *
 * @par Original Copyright
 * This file is a derivative work from ptpd.c
 * Copyright (c) 2005-2007 Kendall Correll 
 *
 * @par Modifications and enhancement Copyright
 * Modifications Copyright (c) 2007-2010 by Alan K. Bartky, all rights
 * reserved
 *
 * @par
 * This file (arith.c) contains Modifications (updates, corrections      
 * comments and addition of initial support for IEEE 1588 version 1, IEEE 
 * version 2 and IEEE 802.1AS PTP) and other features by Alan K. Bartky.
 * 
 * @par License
 * These modifications and their associated software algorithms are under 
 * copyright and for this file are licensed under the terms of the GNU   
 * General Public License as published by the Free Software Foundation;   
 * either version 2 of the License, or (at your option) any later version.
 */ 
#include <limits.h>
#include "ptpd.h"

#define NSEC 1000000000

#define	NSEC_TIME_TO_SECS(nsec_time)	\
	( (Integer32) (nsec_time/NSEC) )
	
#define REMAINDER_NSECS(nsec_time, sec_time)\
	( (Integer32) (nsec_time - (((Integer64)sec_time) * NSEC)) )

/** 
 * Function to calculate 32 bit CRC based on algorithm
 *  from annex C of the 1588 version 1 spec 
 *
 * @param[in] buf       Pointer to 8 bit data bytes to calculate CRC for
 * @param[in] length    Number of bytes to calculate CRC
 *
 * @return          
 * Returns 32 bit CRC as calulcated based on the pointer to the bytes
 * and number of bytes to calculate the CRC for.
 */
UInteger32 crc_algorithm(Octet *buf, Integer16 length)
{
  Integer16 i;
  UInteger8 data;
  UInteger32 polynomial = 0xedb88320, crc = 0xffffffff;
  
  while(length-- > 0)
  {
    data = *(UInteger8 *)(buf++);
    
    for(i=0; i<8; i++)
    {
      if((crc^data)&1)
      {
        crc = (crc>>1);
        crc ^= polynomial;
      }
      else
      {
        crc = (crc>>1);
      }
      data >>= 1;
    }
  }
  
  return crc^0xffffffff;
}

/**
 * Function to calculate the unsigned 32 bit sum of a sequence of 
 * unsigned 32 bit bytes.
 *
 * @param[in] buf       Pointer to 8 bit data bytes to calculate the sum for
 * @param[in] length    Number of bytes to calculate sum
 *
 * @return          
 * Returns 32 bit sum as calulcated based adding up all the 8 bit bytes specified
 * with the result being a 32 bit sum of the byte values.
 */
 
UInteger32 sum(Octet *buf, Integer16 length)
{
  UInteger32 sum = 0;
  
  while(length-- > 0)
    sum += *(UInteger8 *)(buf++);
  
  return sum;
}

/**
 * Function to convert time from internal time used by ptpv2d
 * software of signed seconds, signed nanoseconds, to 
 * PTP version 2 time representation of unsigned seconds,
 * signed seconds, Boolean of half epoch and 16 bit unsigned
 * integer of epoch.
 *
 * @param[in] internal       
 * Pointer to TimeInternal structure containing ptpv2d
 * code internal time representation of signed
 * seconds and signed nanoseconds which is to be converted
 * to PTP version 2 time format (external time)
 *
 * @param[out] external       
 * Pointer to V2TimeRepresentation structure containing PVP
 * version 2 message time representation of unsigned
 * seconds and signed nanoseconds which is to be converted
 * from ptpv2d software internal time format and stored in 
 * the structure
 *
 * @param[in] halfEpoch 
 * Boolean parameter, TRUE if current data is past year 
 * 2038, otherwise set to false
 *
 * @param[in] epoch
 * Unsigned 16 bit epoch number as specified in PTP version
 * 2 standard.  This is for dates even farther in the future
 * than 2038.
 *
 */ 
void v2FromInternalTime(TimeInternal         *internal, // signed secs,   signed nanoseconds
                        V2TimeRepresentation *external, // unsigned secs, signed nanoseconds
                        Boolean               halfEpoch,// current date past year 2038
                        UInteger16            epoch     // Epoch number (even further in future)
                     )
{
  external->epoch_number = epoch;

  external->seconds = labs(internal->seconds) + halfEpoch * INT_MAX;
  
  if(internal->seconds < 0 || internal->nanoseconds < 0)
  {
    external->nanoseconds = labs(internal->nanoseconds) | ~INT_MAX;
  }
  else
  {
    external->nanoseconds = labs(internal->nanoseconds);
  }

  
  DBGV("v2FromInternalTime: internal:%10ds %11dns\n",
       internal->seconds, internal->nanoseconds
      );
  DBGV("                 to external:%10ds %11dns\n",
       external->seconds, external->nanoseconds
      );
}

/**
 * Function to convert time from internal time used by ptpv2d
 * software of signed seconds, signed nanoseconds, to 
 * PTP version 1 time representation of unsigned seconds,
 * signed seconds and Boolean of half epoch.
 *
 * @param[in] internal       
 * Pointer to TimeInternal structure containing ptpv2d
 * code internal time representation of signed
 * seconds and signed nanoseconds which is to be converted
 * to PTP version 1 time format (external time)
 *
 * @param[out] external       
 * Pointer to TimeRepresentation structure containing PVP
 * version 1 message time representation of unsigned
 * seconds and signed nanoseconds which is to be converted
 * from ptpv2d software internal time format and stored in 
 * the structure
 *
 * @param[in] halfEpoch 
 * Boolean parameter, TRUE if current data is past year 
 * 2038, otherwise set to false
 *
 */ 
void fromInternalTime(TimeInternal       *internal, // signed secs,   signed nanoseconds
                      TimeRepresentation *external, // unsigned secs, signed nanoseconds
                      Boolean halfEpoch             // Half Epoch flag (for time past year 2038)
                     )
{
  external->seconds = labs(internal->seconds) + halfEpoch * INT_MAX;
  
  if(internal->seconds < 0 || internal->nanoseconds < 0)
  {
    external->nanoseconds = labs(internal->nanoseconds) | ~INT_MAX;
  }
  else
  {
    external->nanoseconds = labs(internal->nanoseconds);
  }

  
  DBGV("fromInternalTime: internal:%10ds %11dns\n",
       internal->seconds, internal->nanoseconds
      );
  DBGV("               to external:%10ds %11dns\n",
       external->seconds, external->nanoseconds
      );
}

/**
 * Function to convert time to internal time used by ptpv2d
 * software of signed seconds, signed nanoseconds, from
 * PTP version 1 time representation of unsigned seconds,
 * signed seconds and Boolean of half epoch.
 *
 * @param[out] internal       
 * Pointer to TimeInternal structure containing ptpv2d
 * code internal time representation of signed
 * seconds and signed nanoseconds which is to be converted
 * from PTP version 1 time format (external time)
 * and stored in the structure.
 *
 * @param[in] external       
 * Pointer to TimeRepresentation structure containing PTP
 * version 1 message time representation of unsigned
 * seconds and signed nanoseconds which is to be converted
 * to ptpv2d software internal time format
 *
 * @param[out] halfEpoch 
 * Boolean parameter, set to 0 if current data is not past year 
 * 2038, otherwise set to non-zero.
 *
 */ 
void toInternalTime(TimeInternal *internal, TimeRepresentation *external, Boolean *halfEpoch)
{
  *halfEpoch = external->seconds / INT_MAX;
  
  if(external->nanoseconds & ~INT_MAX)
  {
    internal->seconds     = -(external->seconds     % INT_MAX);
    internal->nanoseconds = -(external->nanoseconds & INT_MAX);
  }
  else
  {
    internal->seconds     = external->seconds % INT_MAX;
    internal->nanoseconds = external->nanoseconds;
  }
  
  DBGV("toInternalTime:   external:%10ds %11dns\n",
       external->seconds, external->nanoseconds
      );
  DBGV("               to internal:%10ds %11dns\n",
       internal->seconds, internal->nanoseconds
      );
}

/**
 * Function to convert time to internal time used by ptpv2d
 * software of signed seconds, signed nanoseconds, from
 * PTP version 2 time representation of unsigned seconds,
 * signed seconds.
 *
 * @param[out] internal       
 * Pointer to TimeInternal structure containing ptpv2d
 * code internal time representation of signed
 * seconds and signed nanoseconds which is to be converted
 * from PTP version 2 time format (external time)
 * and stored in the structure.
 *
 * @param[in] external       
 * Pointer to TimeRepresentation structure containing PTP
 * version 2 message time representation of unsigned
 * seconds and signed nanoseconds which is to be converted
 * to ptpv2d software internal time format
 */ 
void v2ToInternalTime(TimeInternal *internal, V2TimeRepresentation *external)
{
  
  if(external->nanoseconds & ~INT_MAX)
  {
    internal->seconds     = -(external->seconds     % INT_MAX);
    internal->nanoseconds = -(external->nanoseconds & INT_MAX);
  }
  else
  {
    internal->seconds     = external->seconds % INT_MAX;
    internal->nanoseconds = external->nanoseconds;
  }
  
  DBGV("V2ToInternalTime: external:%10ds %11dns\n",
       external->seconds, external->nanoseconds
      );
  DBGV("               to internal:%10ds %11dns\n",
       internal->seconds, internal->nanoseconds
      );
}

/**
 * Function to apply PTP version 2 Signed 64 bit correction
 * time to a current value of time represented
 * in ptpv2d software of signed seconds and signed nanoseconds
 *
 * @param[in,out] internal       
 * Pointer to TimeInternal structure containing ptpv2d
 * code internal time representation of signed
 * seconds and signed nanoseconds for which the PTP
 * version 2 message correction time is to be applied
 * to.
 *
 * @param[in] external       
 * Unsigned 64 bit PTP version 2 correction time value 
 * to be applied to the ptpv2d internal time value
 *
 */ 
void v2CorrectionToInternalTime(TimeInternal *internal, Integer64 external)
{
  /* This code is optimized for speed (avoid a divide if at all possible) */
  if (abs(external) == 0)
  {
    internal->seconds     = 0;
    internal->nanoseconds = 0;
    return;
  }
  if (external > 0ULL)
  {
    external >>= 16;
  }
  else
  {
    external /= 65536;
  }
  if (abs(external) < 1000000000ULL)
  {
    internal->seconds     = 0;
    internal->nanoseconds = (Integer32)external;
  }
  else
  {
    internal->seconds     = NSEC_TIME_TO_SECS(external);
    internal->nanoseconds = REMAINDER_NSECS(external, internal->seconds);
  }
 
  DBGV("V2CorrectionToInternalTime: external: %16.16llx\n",
       external
      );
  DBGV("                         to internal: %10.10ds.%9.9dns\n",
       internal->seconds, internal->nanoseconds
      );
}


/**
 * Function to do fixup of time after based
 * on calling other routines here in arith.c
 * This function "noramlizes" the time by making
 * sure that after time additions, subtractions
 * etc. that the nanoseconds field if between
 * -999,999,999 and +999,999,999 and as
 * necessary, the seconds field is adjusted if
 * the nanosecond time is outside of those
 * boundaries.
 *
 *
 * @param[in,out] result       
 * Pointer to TimeInternal structure containing ptpv2d
 * code internal time representation of signed
 * seconds and signed nanoseconds for which the 
 * time values are "normalized".
 *
 */ 
static void normalizeTime(TimeInternal *result)
{
  result->seconds     += result->nanoseconds/1000000000;
  result->nanoseconds -= result->nanoseconds/1000000000*1000000000;
  
  if(     result->seconds > 0 && result->nanoseconds < 0)
  {
    result->seconds     -= 1;
    result->nanoseconds += 1000000000;
  }
  else if(result->seconds < 0 && result->nanoseconds > 0)
  {
    result->seconds     += 1;
    result->nanoseconds -= 1000000000;
  }
}

/**
 * Function to add two ptpv2d time values
 * as represented by ptpv2d internal time
 * of signed seconds and signed nanosecond
 *
 * @param[out] result       
 * Pointer to TimeInternal structure containing ptpv2d
 * code internal time representation of signed
 * seconds and signed nanoseconds for which the 
 * add result (x+y) is stored to.
 *
 * @param[out] x       
 * Pointer to TimeInternal structure containing ptpv2d
 * code internal time representation of signed
 * seconds for "x".
 *
 * @param[out] y
 * Pointer to TimeInternal structure containing ptpv2d
 * code internal time representation of signed
 * seconds for "y".
 *
 * @see normalizeTime
 * @see subtractTime
 */ 
void addTime(TimeInternal *result, TimeInternal *x, TimeInternal *y)
{
  result->seconds     = x->seconds     + y->seconds;
  result->nanoseconds = x->nanoseconds + y->nanoseconds;
  
  normalizeTime(result);
}

/**
 * Function to subtract two ptpv2d time values
 * as represented by ptpv2d internal time
 * of signed seconds and signed nanosecond
 *
 * @param[out] result       
 * Pointer to TimeInternal structure containing ptpv2d
 * code internal time representation of signed
 * seconds and signed nanoseconds for which the 
 * subtract result (x-y) is stored to.
 *
 * @param[out] x       
 * Pointer to TimeInternal structure containing ptpv2d
 * code internal time representation of signed
 * seconds for "x".
 *
 * @param[out] y
 * Pointer to TimeInternal structure containing ptpv2d
 * code internal time representation of signed
 * seconds for "y".
 *
 * @see normalizeTime
 * @see addTime
 */ 
void subTime(TimeInternal *result, TimeInternal *x, TimeInternal *y)
{
  result->seconds     = x->seconds     - y->seconds;
  result->nanoseconds = x->nanoseconds - y->nanoseconds;
  
  normalizeTime(result);
}

void clearTime(TimeInternal *time)
{
  time->seconds     = 0;
  time->nanoseconds = 0;
}

void halveTime(TimeInternal *time)
{
  register Boolean seconds_positive;
  register Boolean seconds_odd;

  if (time->nanoseconds >= 0)
  {
    time->nanoseconds >>= 1;
  }
  else
  {
    time->nanoseconds /= 2;
  }
  if (time->seconds == 0)
  {
    return;
  }

  seconds_positive = (time->seconds > 0);
  seconds_odd      = ((((UInteger32)time->seconds) & 1) == 1);

  if (seconds_positive)
  {
    time->seconds >>= 1;
    if (seconds_odd)
    {
      time->nanoseconds += 500000000;
    }
    return;
  }
  else
  {
    time->seconds /=  2;
    if (seconds_odd)
    {
      time->nanoseconds -= 500000000;
    }
    return;
  }
}

void copyTime(TimeInternal *destination, TimeInternal *source)
{
  destination->seconds     = source->seconds;
  destination->nanoseconds = source->nanoseconds;
}

Integer32 getSeconds(TimeInternal *time)
{
  return (time->seconds);
}

Integer64 getNanoseconds(TimeInternal *time)
{
  return (
            ((Integer64)time->seconds * 1000000000)
          + time->nanoseconds
         );
}

/** 
 * Function to check TimeInternal representation time is
 * zero seconds and zero nanoseconds.
 *
 * @return Returns 0 if time is zero, else returns
 * non-zero
 */
Boolean isNonZeroTime(TimeInternal *time)
{
  return (   time->seconds     != 0 
          || time->nanoseconds != 0
         );
}

// eof src/arith.c
