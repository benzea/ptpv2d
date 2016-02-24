<a href='Hidden comment: 
Copyright (c) 2010 Alan K. Bartky, all rights reserved
'></a>
# Debug Print Message Examples #

In this section are some sample excerpts showing examples of the extensive debugging capabilities of the ptpv2d software stack. This example shows excerpts of the debug output of ptpv2d running
in command mode with full debug (all three bits: 1, 2 and 4 true for Basic, Verbose and Message).  This gives 8 possible debug combinations of:

| **-z 0** | No debug output |
|:---------|:----------------|
| **-z 1** | Basic Debug outpu only |
| **-z 2** | Verbose Debug output only |
| **-z 3** | Basic and Verbose debug output|
| **-z 4** | PTP Message debug output only |
| **-z 5** | Basic and PTP Message debug output |
| **-z 6** | Verbose and PTP Message output |
| **-z 7** | Basic, Verbose and PTP message output (i.e. all messages output) |

To reduce code size, all debug code can be compiled out of the final object code simply by changing makefile flags to disable inclusion of the debug code.

Note that the debugging output is contains output information on whether the debug message itself is basic (ptp debug), verbose (ptp debugV) or message information/parsing (ptp debugM).

Also the actual C function is listed to indicate where in the code the debug output is coming from.

In additional the output is limited to 80 characters or less per line to make it easier to read on a terminal or via a text editor. When running the software under a POSIX based system such as Linux, this data by deafulat is output to the console or alternately it can be stored to a file using the “-f” option from the command line (set by the C module startup.c).

Below is an example of starting the ptpv2d daemon under command mode (-c) with full debug print messages enbaled (-z 7):

`~ # ptpv2d -c -z 7`

Example excerpt of startup and initialization code running based on example command line entered above:
```
(ptp debug) allocatePtpdMemory:
(ptp debug) allocated 1720 bytes for protocol engine data
(ptp debug) allocated 1040 bytes for foreign master data
(ptp debugV) currentPtpdClockData: 0x1003f008, Port ID: 1
(ptp debug) ptpdStartup: not running as daemon
(ptp debug) ptpdStartup: completed OK
(ptp debug) protocol: event POWERUP
(ptp debug) toState: entering state PTP_INITIALIZING
(ptp debugV) timerStop: timer index 0 stopped
(ptp debug) doInit: manufacturerIdentity: SNAP_Networks;evaluation_license
(ptp debug) netInit: entering
(ptp debugV) netInit: ptpClock: 0x1003f008, Port ID: 1
(ptp debug) netInit: Setting up sockets
(ptp debugV) netInit: created event socket 4
(ptp debugV) netInit: created general socket 5
(ptp debugV) netInit: raw socket create skipped
(ptp debugV) netInit: rtOpts->ifaceName port id: 1 is specified as eth1
(ptp debugV) findIface: specified interface: eth1
(ptp debugV) findIface: communication technology OK (1)
(ptp debugV) findIface: returning, got IP address OK
(ptp debug) netInit: socket reuse set OK
(ptp debugV) netInit: event and general socket binds complete
(ptp debugV) netInit: skipping bind of raw socket
(ptp debug) netInit: exiting OK
(ptp debug) initData
(ptp debugV) doInit: sync_interval = 0, initTimer 1 second/tick
(ptp debug) initTimer: 1 seconds, 0 microseconds
(ptp debugV) doInit: setting Clockperiod 7 nanoseconds
(ptp debugV) mpc831x_set_period: SOURCE1, requested period 7ns
(ptp debugV) mpc831x_set_period: period 7 nanoseconds
(ptp debugV) mpc831x_set_period: add value = 0xDB6DB6DB (3681400539)
(ptp debugV) mpc831x_set_period: period set as 0x00000007 (7)
(ptp debugV) mpc831x_set_addend: 0xdb6db6db (3681400539)
(ptp debugV) mpc831x_set_period: base timer add value = 0xDB6DB6DB (3681400539)
(ptp debug) initClock:
(ptp debug) initClockVars:
(ptp debugV) mpc831x_adj_addend: parameter passed 0x00000000 (0)
(ptp debugV) mpc831x_adj_addend: base add value 0xdb6db6db (3681400539)
(ptp debugV) mpc831x_adj_addend: new add value 0xdb6db6db (3681400539)
(ptp debugV) mpc831x_set_addend: same addend 0xdb6db6db (3681400539), returning
(ptp debug) mpc831x_set_curr_time board_time = 1205042240649637000 Hex 0x10b92b8885432c88)
(ptp debug) mpc831x_set_curr_time 1205042240.649637000`
```
Here is an excerpt of the same debug output later where an actual PTP inbound message is processed showing the detail available for message parsing and processing. This example shows a version 1 PTP Sync and Follow-Up message being received and processed by the PTP software stack.  First part of the trace is the receipt of a Sync Message:
```
(ptp debugV) handle: event Receipt of Message
(ptp debugV) handle: type..... 0
(ptp debugV) handle: uuid..... 00:e0:0c:00:7e:21
(ptp debugV) handle: isFrom self is FALSE
(ptp debugV) mpc831x_get_rx_time: interface: eth1, socket: 4
(ptp debugV) mpc831x_get_rx_time: ioctl command: 35312, data: 0x000001ea
(ptp debugV) mpc831x_get_rx_time: board_time = 1205042242192150063
(ptp debugV) mpc831x_get_rx_time: 1205042242.192150063
(ptp debugV) handle: time..... 1205042242s 192150063ns
(ptp debugV) handle: SYNC_MESSAGE, length: 124
(ptp debugV) handleSync: length = 124
(ptp debugV) handleSync: looking for uuid 00:e0:0c:00:7e:21
(ptp debugV) addForeign: add or update record
(ptp debugV) addForeign: updated record 0
(ptp debugM) msgUnpackHeader:
(ptp debugM) versionPTP.................... 1
(ptp debugM) versionNetwork................ 1
(ptp debugM) subdomain..................... _DFLT
(ptp debugM) messageType................... 1
(ptp debugM) sourceCommTechnology.......... 1
(ptp debugM) sourceUuid.................... 00:e0:0c:00:7e:21
(ptp debugM) sourcePortId.................. 1
(ptp debugM) sequenceId.................... 490
(ptp debugM) control....................... 0
(ptp debugM) flags......................... 00 08
(ptp debugM) msgUnpackSync:
(ptp debugM) originTimestamp.seconds....... 1205042229
(ptp debugM) originTimestamp.nanoseconds... 471370353
(ptp debugM) epochNumber................... 0
(ptp debugM) currentUTCOffset.............. 33
(ptp debugM) grandmasterCommTechnology..... 1
(ptp debugM) grandmasterClockUuid.......... 00:e0:0c:00:7e:21
(ptp debugM) grandmasterPortId............. 0
(ptp debugM) grandmasterSequenceId......... 490
(ptp debugM) grandmasterClockStratum....... 4
(ptp debugM) grandmasterClockIdentifier.... DFLT
(ptp debugM) grandmasterClockVariance...... -1000
(ptp debugM) grandmasterPreferred.......... 0
(ptp debugM) grandmasterIsBoundaryClock.... 0
(ptp debugM) syncInterval.................. 0
(ptp debugM) localClockVariance............ -1000
(ptp debugM) localStepsRemoved............. 0
(ptp debugM) localClockStratum............. 4
(ptp debugM) localClockIdentifer........... DFLT
(ptp debugM) parentCommunicationTechnology. 1
(ptp debugM) parentUuid.................... 00:e0:0c:00:7e:21
(ptp debugM) parentPortField............... 0
(ptp debugM) estimatedMasterVariance....... 0
(ptp debugM) estimatedMasterDrift.......... 0
(ptp debugM) utcReasonable................. 0
(ptp debugV) s1:
(ptp debugV) handleSync: SYNC_RECEIPT_TIMER reset
(ptp debugV) timerStart: set timer index 0 to 10
(ptp debugV) handleSync: MASTER, LISTENING, PASSIVE states
(ptp debugV) handleSync: Version 1 message
(ptp debugV) handleSync: Version 1 Communication technology OK
(ptp debugV) addForeign: add or update record
(ptp debugV) addForeign: updated record 0
```
Now here is the trace excerpt for the receipt of the subsequent follow up message:
```
(ptp debugV) handle: event Receipt of Message
(ptp debugV) handle: type..... 2
(ptp debugV) handle: uuid..... 00:e0:0c:00:7e:21
(ptp debugV) handle: sequence. 484
(ptp debugV) handle: isFrom self is FALSE
(ptp debugV) handle: time..... 0s 0ns
(ptp debugV) handle: FOLLOWUP_MESSAGE, length: 52
(ptp debugV) handleFollowUp: looking for uuid 00:e0:0c:00:7e:21
(ptp debugM) msgUnpackFollowUp:
(ptp debugM) associatedSequenceId.......... 490
(ptp debugM) preciseOriginTimestamp.secs... 1205042229
(ptp debugM) preciseOriginTimestamp.nsecs.. 471431806
(ptp debugV) handleFollowUp: expected and OK
(ptp debugV) toInternalTime: external:1205042229s 471431806ns
(ptp debugV) to internal:1205042229s 471431806ns
(ptp debugV) updateOffset:
(ptp debugV) updateClock:
(ptp debug) updateClock: Storing T1: 1205042229s 471431806ns
(ptp debug) updateClock: Storing T2: 1205042242s 192150063ns
(ptp debugV) timerExpired: Checking index 0
(ptp debugV) protocol: message_activity was TRUE
(ptp debugV) netSelect: NULL timeout pointer, wait for event
(ptp debugV) netSelect: return: 1
(ptp debugV) doState: Port state: 8
(ptp debugV) checkTxCompletions: no tx timestamps pending
(ptp debugV) handle:
(ptp debugV) netSelect: timeout requested 0s 0ns
```

---

Copyright (c) 2010 Alan K. Bartky, all rights reserved


---
