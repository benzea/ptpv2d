# News #

## 2010-11 ##

Updated the ptpv2d-dev source code repository with the following updates:

- Added support for Annex F (dual MAC addresses 01-80-C2-00-00-0E and 01-1B-19-00-00)

- Moved and modified system dependent constants from constants.h to constants\_dep.h, with separte values for Windows, MPC831X (HW timestamping) and other (e.g. Linux).

- Added separate constant for one step mode for regular and raw sockets.  This allows for mode for Linux  with looped backed timestamps on regular sockets and none on raw sockets, mode for Windows with no timestamping (always one step mode) and MPC831X HW timestamping mode always with timestamps.

- Added fix to set sa\_family to unspecified when adding multicast MAC address to raw socket.

- Added both multicast MAC addresses as specified in IEEE 1588 Annex F when opening raw sockets (i.e. always listens on both addresses, sending MAC address selected based on -F or -8 option at startup and whether packet is PDelay or not.

I've also added a .zip file so those without SVN can download and tryout the source code.  This includes the solution/project files for MS Visual C 2008.

## 2010-09 ##

As part of cleaning up the code base to support a non-hardware specific Linux version of ptpv2d, I've updated the code base for an initial portation to Microsoft Visual C to allow for experimentation, development and testing using standard Windows XP PCs.  This has currently been posted under a "dev" version. Note that I'm working with and learning Tortoise SVN and Google code SVN, so not all files are where I'd like them to be.  Hopefully I'll get better at this as I go.  If you are looking for the previos 1a0 version (which has been tested on the Freescale MPC8313E-RDB board), you'll need to request version 3 of the 1a0 branch using SVN as my latest upload also overwrote many files in that branch.

This updated code base now should be able to be compiled under the following variants:
- Standard Linux running on PCs running software based socket timestamping
- Freescale LTIB BSP Linux on the MPC831X chips with ioctl based hardware timestamping.
- Micosoft Windows32 console application, compilable, linkable, but not yet fully functional

I have done a quick tests on standard PCs using Debian Linux with gcc toolchain and also Visual Studio 2008 on Windows PC running as a console application.  I have not yet had the time to test the compile on LTIB for Freescale MPC831X hardware timestamping, but I did try to be careful with the modifications, so it should still compile and run.

Hopefully in the near future, I will have both the untouched original 1a0 and a "dev" tree once I figure out how to get the Tortoise SVN tool working properly with Google code and with the structure I am trying to setup.

## 2010-08 ##

<img src='http://groups.google.com/intl/en/images/logos/groups_logo_sm.gif' alt='Google Groups' height='30' width='140'>

ptpv2d now has a new Google Groups page and associated mailing list for developers interested in working on or with the code base.<br>
<br>
<a href='http://groups.google.com/group/ptpv2d'>Visit ptpv2d Software Google Group</a>

<a href='mailto:ptpv2d@googlegroups.com?subject=ptpv2d%20on%20Google%20Code'>Send email to ptpv2d Software Google Group</a>