# Makefile for ptp2d
#
# Based on makefile from ptpd version rc1
#
# Modifications by Alan K. Bartky to also add support
# for both Generic PC linux and also MPC8313E-RDB board linux
#


RM = rm -f
#
# Linux PC flags, tested with Debian Linux
#
CFLAGS = -Wall -DPTPD_DBGV -Dlinux -DSOCKET_TIMESTAMPING
#
# Realtime clock library needed for functions such as clock_gettime
# This is included using -lrt flags for the linker
#
LDFLAGS = -lrt

#
# Commented out flags below for No deamon option
#
#CPPFLAGS = -DPTPD_DBG -DPTPD_NO_DAEMON

# In case this is an MPC831X system, define CONFIG_MPC831X
# CONFIG_MPC831X=1

#
# PROG options creates program called "ptpv2d"
#
PROG = ptpv2d

#
# Objects in both main directory (fully portable) and also
# system dependent in the "dep" directory
#
OBJ  = ptpv2d.o arith.o bmc.o probe.o protocol.o v2utils.o v2bmc.o\
	dep/msg.o dep/net.o dep/servo.o dep/startup.o dep/sys.o dep/timer.o dep/ledlib.o
#
# Header files:
#
HDR  = ptpd.h constants.h datatypes.h \
	dep/ptpd_dep.h dep/constants_dep.h dep/datatypes_dep.h 

#
# Conditional compile for MPC831X family. If compiled under the LTIB
# builder provided by freescale the CONFIG_MPC831X environment variable
# will be set, so if using that build option, the symbol should be
# automatic and not necessary as a define in this makefile
# On other compilers/environments it may be necessary to add
#
ifeq ($(CONFIG_MPC831X), 1)

# -DCONFIG_MPC831X is defined to use MPC831X timestamping, 
# otherwise system timer is used
#
CFLAGS = -Wall -DPTPD_DBGV -DCONFIG_MPC831X
LDFLAGS = -lrt  
OBJ += mpc831x.o
HDR += mpc831x.h
#
# Makefile from original patch for ptpd version 1b5 used CC 
# as a specific gcc filename spec of powerpc-e300c3-linux-gcc.  
# This does not work# with the LTIB environment, 
# where you just use gcc (LTIB maps gcc to the right type and
# version of the compiler for the target.
#
# original patch from Freescale was:      CC=powerpc-e300c3-linux-gcc
# changed to work in an LTIB environment: CC=gcc
#
CC=gcc

#
# End MPC831X specific Make control information
#
endif

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

all: $(PROG)

$(PROG): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

$(OBJ): $(HDR)

clean:
	$(RM) $(PROG) $(OBJ)
