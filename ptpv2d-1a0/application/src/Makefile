# Makefile for ptpd

RM = rm -f
CFLAGS = -Wall
#CPPFLAGS = -DPTPD_DBG -DPTPD_NO_DAEMON

# In case this is an MPC831X system, define CONFIG_MPC831X
CONFIG_MPC831X=1


# Nothing to be changed below this line

PROG = ptpv2d
OBJ  = ptpv2d.o arith.o bmc.o probe.o protocol.o v2utils.o v2bmc.o\
	dep/msg.o dep/net.o dep/servo.o dep/startup.o dep/sys.o dep/timer.o 
HDR  = ptpd.h constants.h datatypes.h \
	dep/ptpd_dep.h dep/constants_dep.h dep/datatypes_dep.h 

ifeq ($(CONFIG_MPC831X), 1)
#
# for full  debug messages define -DPTPD_DBGV below
# for basic debug messages define -DPTPD_DBG  below
# for no    debug messages remove the debug define
#
# -DCONFIG_MPC831X is defined to use MPC831X timestamping, 
# otherwise system timer is used
#
CFLAGS = -Wall -DPTPD_DBGV -DCONFIG_MPC831X
LDFLAGS = -lrt  
OBJ += mpc831x.o
OBJ += dep/ledlib.o
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
endif

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

all: $(PROG)

$(PROG): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

$(OBJ): $(HDR)

clean:
	$(RM) $(PROG) $(OBJ)
