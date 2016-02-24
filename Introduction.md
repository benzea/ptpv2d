# Introduction #

<a href='Hidden comment: 
<!-- **************************************************************************** -->
<!-- Edited 2010-03-31 by Alan K. Bartky, Copyright (c) 2008, all rights reserved -->
<!-- **************************************************************************** -->
'></a>

## Precision Time Protocol Software ##

The ptpv2d Precision Time Protocol daemon offers the following features:

  * A User mode application that runs under standard LINUX, with a modular design that ensures easy portation to additional operating systems.
  * An extensive optional print-to-console debug message function.  This message functionality can be added or omitted at compile time.  If it is included, then the output level is command-line selectable. Debug messages are uniformly formatted for quicker, easier analysis.
  * Use of POSIX socket calls for all messages.  Message handling is contained in a separate module to ensure easy portation to any desired operating system.
  * Freescale MPC8313E Ethernet port driver with full support for PTP version 1 and version 2 message hardware time stamping.
  * Ability to adjust the Hardware clock very precisely (as precise as one part per billion).  Software algorithm for fine (as small as one part per billion) adjustment of the Hardware clock
  * Ability to report detected variances from Grandmaster clock in parts per million
  * Command line ability to set four different debug output levels: none, basic, verbose, and Message
  * Ability to set Sync/Follow-up transmit time values of less than one second.  This allows for multiple Sync/Follow-ups per second, which provides enhanced accuracy and tracking of the "slave" system to the "grandmaster" system.
  * Additional run time options and support code include:
    * Selection of PTP version 1 or version 2 encapsulation and protocol
    * Selection of IEEE 1588 PTP over IPv4/UDP, IEEE P1588 PTP over Ethernet (draft 2.2)or IEEE P802.1AS encapsulation and protocol(draft 1.0)
    * User specified clock period to synthesize other base clocks, not just at 10 MHz.

These features have gone through extensive testing using dual MPC8313E-RDB boards running both PTP and Ping traffic.  On a Freescale MPC8313E-RDB board running eight Sync/Follow-up Messages per second, it will support a plus or minus 50 nanosecond offset from master more than 99.9% of the time. At 128 Sync/Follow-ups per seconds, the system will synchronize within 16 nanoseconds of the "grandmaster" system.

## PTPV2D Daemon help file example: ##

```

# ptpv2d -?
Usage:  ptpv2d [OPTION]

-?                show this page

-c                run in command line (non-daemon) mode
-f FILE           send output to FILE
-d                display stats
-D                display stats in .csv format
-z                debug level (0=none or bit mask 1:basic, 2:verbose, 4:message)

-x                do not reset the clock if off by more than one second
-t                do not adjust the system clock
-a NUMBER,NUMBER  specify clock servo P and I attenuations
-w NUMBER         specify one way delay filter stiffness
-H                specify hardware clock period in nanoseconds
-A                specify base value for clock frequency adjustment
-R                remember adjust value for slave to master transition

-b NAME           bind PTP to network interface NAME
-u ADDRESS        also send uni-cast to ADDRESS
-2                run in PTP version 2 mode instead of version 1
-8                run in IEEE 802.1AS PTP Layer 2 mode instead of IP/UDP
-F                run in 1588 Annex F PTP Layer 2 mode instead of IP/UDP
-P                run Pdelay Req/Resp mechanism instead of Delay Resp/Req
-l NUMBER,NUMBER  specify inbound, outbound latency in nsec

-o NUMBER         specify current UTC offset
-e NUMBER         specify epoch NUMBER
-h                specify half epoch

-y NUMBER         specify sync interval in 2^NUMBER sec
-Y NUMBER         specify announce interval in 2^NUMBER sec
-m NUMBER         specify max number of foreign master records

-g                run as slave only
-p                make this a preferred clock
-s NUMBER         specify system clock stratum
-i NAME           specify system clock identifier
-v NUMBER         specify system clock allen variance

-n NAME           specify PTP subdomain name (not related to IP or DNS)

-k NUMBER,NUMBER  send a management message of key, record, then exit

```

## Sample charts generated from .csv statistics output feature ##

Here is a diagram showing ptpv2d in action running on a Freescale MPC8313E with full hardware timestamping support.  It shows a slave MPC8313E-RDB board quickly synchronizing to a grandmaster.  The the slave is then intentionally disconnected from the grandmaster for 2 minutes and then reconnected.  On the second time reconnecting to the grandmaster, the ptpv2d code has optional holdover running so it quickly resynchronizes with the grandmaster.

![http://www.bartky.net/Media/ptpv2d_sync_chart.png](http://www.bartky.net/Media/ptpv2d_sync_chart.png)

Here is a diagram showing ptpv2d in action in the same test run after the clock has stabilized.  This chart shows the flexibility of the ptpv2d software in that the software is configurable to run on the MPC8313E PTP Hardware clock with user configurable reference frequency period.  On the MPC8313E-RDB board, this can be set to as low as 8 nanoseconds.  It also shows that the software ban be configured for multiple sync/follow-up messages per second so you can trade off between clock accuracy and network load.

![http://www.bartky.net/Media/ptpv2d_scatter_example_chart.png](http://www.bartky.net/Media/ptpv2d_scatter_example_chart.png)


---

<p align='center'>
Copyright (c) 2010 by Alan K. Bartky, all rights reserved<br>
</p>

---
