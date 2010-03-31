/* src/dep/ledlib.c */
/* Simple library to set/clear MPC8313E-RDB LEDs from user mode */
/* NOTE: These functions could be used as a basis to turn on/off
 * LEDs on other target boards based on PTP status
 */

/****************************************************************************/
/* Begin copyright and licensing information, do not remove                 */
/*                                                                          */
/* This file (ledlib.c) contains original work by Alan K. Bartky            */
/* Copyright (c) 2008-2010 by Alan K. Bartky, all rights reserved           */
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

/* The MPC8313E-RDB board has 8 LEDs where the least significant 6 bits are green
 * (0-63), and the upper 2 bits are yellow and red where red is the most signifcant
 * bit of the byte.
 *
 * Setting a bit to 0 turns the LED on, setting it to 1 turns the LED off
 *
 * Based on that, there are these main functions
 *
 * init_led
 *
 * yellow_alarm     Set Yellow LED On or Off
 * red_alarm        Set Red    LED On or Off
 * For both the "alarm" functions Setting TRUE turns on the LED, FALSE turns it off
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <memory.h>

// Per 8313EVB_INIT.cfg, LED/Status buffer is memory mapped @ 0xfa000000, size 32 KB

#define MEMSIZE         0x00008000
#define MEM_ADDR        0xfa000000

#define LED_BIT_7       0x80
#define LED_BIT_6       0x40
#define LED_BIT_5       0x20
#define LED_BIT_4       0x10
#define LED_BIT_3       0x08
#define LED_BIT_2       0x04
#define LED_BIT_1       0x02
#define LED_BIT_0       0x01


#define LEDS_ALL_ON     0x00
#define LEDS_ALL_OFF    0xFF

#define RED_LED         7
#define RED_LED_ON      0x7F
#define RED_LED_OFF     0x80

#define YELLOW_LED      6
#define YELLOW_LED_ON   0xBF
#define YELLOW_LED_OFF  0x40

#define GREEN_LED       5
#define GREEN_LED_ON    0xDF
#define GREEN_LED_OFF   0x20

#define STATUS_LED      4
#define STATUS_LED_ON   0xEF
#define STATUS_LED_OFF  0x10

/* Global vars */
volatile unsigned char * led_ptr;
unsigned char            led_value;


/*
 * Usage:
 *   volatile void *p = ioremap(MY_HARD_REG_ADDR, 4096);
 *   ...
 *   out_8(p, state ^= 0x1);
 *
 *
 *  Copyright (C) 2003 Stephane Fillod
 */

#ifdef __PPC__
inline void out_8(volatile unsigned char *addr, unsigned val)
{
        __asm__ __volatile__("stb%U0%X0 %1,%0; eieio" : "=m" (*addr) : "r" (val));
}
/* etc., cf asm/io.h */
#else
inline void out_8(volatile unsigned char *addr, unsigned val)
{
        *addr = val & 0xff;
}
#endif

volatile void * ioremap(unsigned long physaddr, unsigned size)
{
    static int axs_mem_fd = -1;
    unsigned long page_addr, ofs_addr, reg, pgmask;
    void* reg_mem = NULL;

    /*
     * looks like mmap wants aligned addresses?
     */
    pgmask = getpagesize()-1;
    page_addr = physaddr & ~pgmask;
    ofs_addr  = physaddr & pgmask;

    /*
     * Don't forget O_SYNC, esp. if address is in RAM region.
     * Note: if you do know you'll access in Read Only mode,
     *    pass O_RDONLY to open, and PROT_READ only to mmap
     */
    if (axs_mem_fd == -1) {
        axs_mem_fd = open("/dev/mem", O_RDWR|O_SYNC);
        if (axs_mem_fd < 0) {
                perror("AXS: can't open /dev/mem");
                return NULL;
        }
    }

    /* memory map */
    reg_mem = mmap(
        (caddr_t)reg_mem,
        size+ofs_addr,
        PROT_READ|PROT_WRITE,
        MAP_SHARED,
        axs_mem_fd,
        page_addr
    );
    if (reg_mem == MAP_FAILED) {
        perror("AXS: mmap error");
        close(axs_mem_fd);
        return NULL;
    }

    reg = (unsigned long )reg_mem + ofs_addr;
    return (volatile void *)reg;
}

int iounmap(volatile void *start, size_t length)
{
    unsigned long ofs_addr;
    ofs_addr = (unsigned long)start & (getpagesize()-1);

    /* do some cleanup when you're done with it */
    return munmap((unsigned char*)start-ofs_addr, length+ofs_addr);
}


void all_leds (unsigned char on_or_off)
{
   if (on_or_off)
   {
      led_value = LEDS_ALL_ON;
   }
   else
   {
      led_value = LEDS_ALL_OFF;
   }
   out_8(led_ptr,led_value);
}


void red_alarm (unsigned char on_or_off)
{
   if (on_or_off)
   {
      /* Turn LED on, turn bit off */
      led_value &= RED_LED_ON;
      led_value |= GREEN_LED_OFF;
      led_value |= YELLOW_LED_OFF;
   }
   else
   {
      led_value |= RED_LED_OFF;
   }
   out_8(led_ptr,led_value);
}

void yellow_alarm (unsigned char on_or_off)
{
   if (on_or_off)
   {
      /* Turn LED on, turn bit off */
      led_value &= YELLOW_LED_ON;
      led_value |= GREEN_LED_OFF;
   }
   else
   {
      led_value |= YELLOW_LED_OFF;
   }
   out_8(led_ptr,led_value);
}


void green_alarm (unsigned char on_or_off)
{
   if (on_or_off)
   {
      /* Turn LED on, turn bit off */
      led_value &= GREEN_LED_ON;
      led_value |= YELLOW_LED_OFF;
   }
   else
   {
      led_value |= GREEN_LED_OFF;
   }
   out_8(led_ptr,led_value);
}

/* Function to use least significant 4 bits of LED register to act
 * like a VU meter that goes up or down depending on passed value
 */

void led_meter(unsigned char value)
{
  unsigned char led_meter_value;

  led_meter_value = 0;  /* Assume VU meter min value */

  if (value <= 80)
  {
    led_meter_value |= 1;
    if (value <= 40)
    {
      led_meter_value |= 2;
      if (value <= 20)
      {
         led_meter_value |= 4;
         if (value <= 10)
         {
           led_meter_value |= 8;
         }
      }
    }
  }

  /* Clear led value lower 4 bits */

  led_value &= 0xF0;

  led_meter_value ^= 0x0F;  /* Invert bits for setting LED */

  led_value |= led_meter_value;

  out_8(led_ptr,led_value);

}


void init_leds(void)
{

	/* Create user mode pointer to LED register */

	led_ptr = (unsigned char *) ioremap(MEM_ADDR, MEMSIZE);

        /* Turn off all LEDs except the green status (running) for initial status */

        led_value = STATUS_LED_ON;

        out_8(led_ptr,led_value);
}

// eof ledlib.c


