<a href='Hidden comment: 
Copyright (c) 2010 Alan K. Bartky, all rights reserved
'></a>

# PTP C Code Overview #

## PTP software structure ##

The PTP software stack source is grouped into source files that are system dependent (under the **_src/dep/_** directory) and those that are not (under the **_src/_** directory). This allows easier portation to other operating systems and environments that such that the non system dependent software should require little or no changes and are all coded in ANSI C. This section documents the major C source code modules for the PTP software stack.

In general, the PTP software stack source is divided into platform-independent and platform-dependent code.

Platform-independent code is located in the top level of the PTP stack source tree, and platform-dependent code is located the **_dep/_** sub-directory.

The system dependent modules are written in using standard POSIX operating system functions so they can be easily ported to other operating systems besides the Linux version as shipped.

The hardware timestamping software as shipped supports the Freescale MPC831X family and is split between user mode code and kernel code. The functions in this module are all abstracted into get and set time functions so they could be replaced by an alternate HW timestamping software interface.

## PTP software components ##

The PTP stack software major source code components are as follows:

### Protocol Engine, _protocol.c_ ###
This module contains main protocol state machines as defined in IEEE 1588 version 1, IEEE 1588 version 2 and IEEE 802.1AS specifications. The state machine is implemented as a forever loop with cases for each state and variations in the state machines handled by run time option variables and also alternate functions being called depending on whether the version of PTP running.  It is called with the **_protocol()_** fucntion after start-up and normal execution runs as a forever loop waiting on message reception or timeouts to occur to process the associated message or timeout. The primary states are handled by the function **_doState()_** and state transitions are handled by **_toState()_**. These transitions occur primarily due to the results of timeout or message events in association with the Best Master Clock (BMC) algorithm. Message events are processed by the **_handle()_** function and actions are taken based on PTP state, parsed message data and other run time and dynamic variables. The primary actions are message sends, timer resets, regular runs of the BMC algorithm, foreign master data updates, and system clock servo updates after PTP sync and follow-up message receipts.

### Version 1 Best Master Clock, _bmc.c_ ###
This module contains the software to support the Best Master Clock (BMC) algorithm defined by the 1588 version 1 specification. It is called with **_bmc()_**, and it returns the proper state, master or slave, based on the reported clock data of other PTP clocks that have sent Sync messages.

### Version 2 Best Master Clock, _v2bmc.c_ ###
This module contains the software to support the Best Master Clock (BMC) algorithm defined by the 1588 version 2 and 802.1AS specification. It is called with **_v2bmc()_**, and it returns the proper state, master or slave, based on the reported clock data of other PTP clocks that have sent Announce messages.

### Clock Servo, _dep/servo.c_ ###
The clock servo computes the offset-from-master from the master-to-slave delay and slave-to-master delays. It uses the offset-from-master calculation to compute clock rate adjustments to minimize the offset-from-master. The clock servo also performs filtering to remove jitter from its input.

### Message Packer/Unpacker, _dep/msg.c_ ###
The message packer/unpacker gathers data into and extracts data from PTP messages as defined by IEEE 1588 version 1, version 2 and IEEE 802.1AS. This module is written in ANSI C assuming no buffer structure and supports both big-endian and little endian formats by using macros to get, set or flip the bytes as needed. When running in debug mode, this module also contains both hexadecimal and parsed field printing for decoding of all PTP messages.

### Network Layer, _dep/net.c_ ###
Initializes connections, sends, and receives data to and from the local area network. All functions are written using standard POSIX IPv4 UDP sockets for IEEE 1588 and POSIX raw sockets for IEEE 802.1AS making it easily portable to most POSIX compliant systems. For non-POSIX compliant systems, this module can easily be recoded to support the other operating system without having to change the other modules in that all network calls are abstracted from the main protocol.c module.

### Hardware Timestamping support _mpc831x.c_ ###
This module contains abstracted functions called by protocol.c to initialize, control, configure and to get timestamps from the hardware timestamping engine (specifically for the delivered software, the Freescale MPC831X family of chips).

### Timer, _dep/timer.c_ ###
Low resolution interval timers used in the protocol engine. The timers control periodic timer events such as Sync message sends by masters, Delay Request sends by slaves, periodic runs of the BMC (state change events), Sync receive timeouts, etc.

### Startup, _dep/startup.c_ ###
This module sets up the program's execution state, allocates memory as needed, retrieves
run-time options from the user and returns execution back to the main module.


---

Copyright (c) 2010 Alan K. Bartky, all rights reserved


---
