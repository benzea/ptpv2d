# ptpv2d Project Home Information #

## Summary ##

This software contains portable C code for supporting IEEE 1588 vesion 2.0 and IEEE 802.1AS precision timing protocol.  The software uses IP and raw standard sockets for packet/frame communication in conjunction with a timer tick call to service timer events.

## News ##

For news updated as of 2010-11-07 see [http://code.google.com/p/ptpv2d/wiki/News](http://code.google.com/p/ptpv2d/wiki/News)

## Copyright, Licensing and Derivative Works Policy ##
This code is based on the original revision rc1 of the BSD licensed **ptpd** PTP version 1 daemon hosted at: http://sourceforge.net/projects/ptpd/

This code's original modifications are by Alan K. Bartky and all new files and modifications from the original ptpd code base files are all copyright Alan K. Bartky, all rights reserved. This code is published as GPL by permission of Alan K. Bartky and SNAP Networks Private Limited.

This ptpv2d code base is being licensed under the GNU General Public License (GPL) Version 2, June 1991.

To fully encourage the open source nature of this project and per the goals of why the  GPL license was created, any and all contributions and derivative works also must be sub-licensed under terms of the GPL license.

If you are still interested in PTP but are looking for less restrictive terms, I recommend the orignal ptpd code base at sourceforge.net

## Differences from ptpd rc1 release code base ##

This code base has extensive modifications from the original ptpd code base version v1rc1.  Some key changes are listed below:

  * Added extensive optional debug print to console (compiled in Y/N, if compiled then command line selectable).

  * Added support for POSIX raw socket calls to accomodate direct ethernet encapsulation for IEEE 1588 and IEEE 802.1AS.

  * Freescale MPC831x Ethernet port driver with full support for PTP version 1 and version 2 message HW timestamping.  Software has been tested on both the MPC8313 and MPC8315.

  * Software Algorithm for fine (part per billion) adjustment of HW clock and reporting of detected variance from Grandmaster.

This code base also has run time options and support code for:

  * IEEE version 2 or IEEE version 1

  * IEEE 1588 IPv4/UDP, IEEE P1588 Ethernet (draft 2.2) or IEEE P802.1AS (draft 1.0)

  * User specified clock period to synthesize other base clocks besides 10 MHz

  * Ability to set bit mask debug print level from command line for 0 (none), 1 (basic), 2 (verbose), 4(messages).

  * Ability to set Sync/Follow-up transmit time value of less than one second(to allow for multiple Sync/Follow-ups per second for enhanced accuracy/tracking of the slave to the Grandmaster).


---


For general comments, questions, suggestions, etc. email [ptpv2d@googlegroups.com](mailto:ptpv2d@googlegroups.com?subject=ptpv2d%20on%20Google%20Code)

For specific comments, questions, suggestions, etc. to the orignal author and current maintainer of this site, email [alan@bartky.net](mailto:alan@bartky.net?subject=ptpv2d%20on%20Google%20Code)


---
