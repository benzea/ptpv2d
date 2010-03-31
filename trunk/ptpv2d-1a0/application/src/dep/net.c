/* src/dep/net.c */
/* PTP message sending and receiving and other misc. network related 
 * functions 
 */
/* Copyright (c) 2005-2007 Kendall Correll */

/* For IEEE 1588 version 1, see annex d */
/* For IEEE 1588 version 2, see clause (chapter) 13 */

/****************************************************************************/
/* Begin additional copyright and licensing information, do not remove      */
/*                                                                          */
/* This file (net.c) contains Modifications (updates, corrections           */
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

Boolean lookupSubdomainAddress(Octet *subdomainName, Octet *subdomainAddress)
{
  UInteger32 h;
  
  /* set multicast group address based on subdomainName */
  if     (!memcmp(subdomainName, 
                  DEFAULT_PTP_DOMAIN_NAME, 
                  PTP_SUBDOMAIN_NAME_LENGTH
                 )
         )
           memcpy(subdomainAddress,
                  DEFAULT_PTP_DOMAIN_ADDRESS,
                  NET_ADDRESS_LENGTH
                 );

  else if(!memcmp(subdomainName,
                  ALTERNATE_PTP_DOMAIN1_NAME,
                  PTP_SUBDOMAIN_NAME_LENGTH
                 )
         )
           memcpy(subdomainAddress,
                  ALTERNATE_PTP_DOMAIN1_ADDRESS,
                  NET_ADDRESS_LENGTH
                 );

  else if(!memcmp(subdomainName, 
                  ALTERNATE_PTP_DOMAIN2_NAME,
                  PTP_SUBDOMAIN_NAME_LENGTH
                 )
         )
           memcpy(subdomainAddress,
                  ALTERNATE_PTP_DOMAIN2_ADDRESS,
                  NET_ADDRESS_LENGTH
                 );

  else if(!memcmp(subdomainName, 
                  ALTERNATE_PTP_DOMAIN3_NAME,
                  PTP_SUBDOMAIN_NAME_LENGTH
                 )
         )
           memcpy(subdomainAddress,
                  ALTERNATE_PTP_DOMAIN3_ADDRESS,
                  NET_ADDRESS_LENGTH
                 );

  else
  {
    h = crc_algorithm(subdomainName, PTP_SUBDOMAIN_NAME_LENGTH) % 3;
    switch(h)
    {
    case 0:
      memcpy(subdomainAddress, 
             ALTERNATE_PTP_DOMAIN1_ADDRESS,
             NET_ADDRESS_LENGTH
            );
      break;
    case 1:
      memcpy(subdomainAddress,
             ALTERNATE_PTP_DOMAIN2_ADDRESS,
             NET_ADDRESS_LENGTH
            );
      break;
    case 2:
      memcpy(subdomainAddress,
             ALTERNATE_PTP_DOMAIN3_ADDRESS,
             NET_ADDRESS_LENGTH
            );
      break;
    default:
      ERROR("lookupSubdomainAddress: handle out of range for '%s'!\n",
            subdomainName
           );
      return FALSE;
    }
  }
  
  return TRUE;
}

UInteger8 lookupCommunicationTechnology(UInteger8 communicationTechnology)
{
#if defined(linux)
  
  switch(communicationTechnology)
  {
  case ARPHRD_ETHER:
  case ARPHRD_EETHER:
  case ARPHRD_IEEE802:
    return PTP_ETHER;
    
  default:
    break;
  }
  
#elif defined(BSD_INTERFACE_FUNCTIONS)
  
#endif
  
  return PTP_DEFAULT;
}



UInteger32 findIface(  /* Find interface, returns IP address of interface */
                     Octet *     ifaceName,               /* Name (e.g. eth0 or NULL ptr) */
                     UInteger8 * communicationTechnology, /* Communication technology */
                     Octet *     uuid,                    /* UUID (to copy MAC address to */
                     NetPath *   netPath                  /* NetPath (network info structure) */
                    )
{
#if defined(linux)

  /* depends on linux specific ioctls (see 'netdevice' man page) */
  int i, flags;
  struct ifconf data;
  struct ifreq device[IFCONF_LENGTH];
  
  data.ifc_len = sizeof(device);
  data.ifc_req = device;
  
  memset(data.ifc_buf,0,data.ifc_len);
  
  flags = IFF_UP|IFF_RUNNING|IFF_MULTICAST;
  
  /* look for an interface that is UP, RUNNING and MULTICAST
   * if none specified 
   */
  if(ifaceName[0] != '\0')
  {
    //
    // Interface was specified, copy to ifr_name
    //
    DBGV("findIface: specified interface: %s\n", ifaceName);
    i = 0;
    memcpy(device[i].ifr_name, ifaceName, IFACE_NAME_LENGTH);
    strncpy(netPath->ifName, ifaceName, IFNAMSIZ);
    
    //
    // Try and get the Hardware address for this interface name
    //
    if     (ioctl(netPath->eventSock, SIOCGIFHWADDR, &device[i]) < 0)
    {
      DBGV("findIface: failed to get hardware address\n");
    }
    else if((*communicationTechnology 
             = lookupCommunicationTechnology(device[i].ifr_hwaddr.sa_family)
            ) == PTP_DEFAULT
           )
    {
      DBGV("findIface: unsupported communication technology (%d)\n",
           *communicationTechnology
          );
    }
    else
    {
      DBGV("findIface: communication technology OK (%d)\n",
           *communicationTechnology
          );
      memcpy(uuid, device[i].ifr_hwaddr.sa_data, PTP_UUID_LENGTH);
      memcpy(netPath->portMacAddress, device[i].ifr_hwaddr.sa_data, 6);
    }
  }
  else
  {
    /* no iface specified */
    /* get list of network interfaces*/
    if(ioctl(netPath->eventSock, SIOCGIFCONF, &data) < 0)
    {
      PERROR("findIface: failed query network interfaces");
      return 0;
    }
    
    DBGV("findIface: eventSock %d Got Interface config data\n",
         netPath->eventSock
        );

    if(data.ifc_len >= sizeof(device))
    {
      DBG("findIface: device list may exceed allocated space\n");
    }
    
    /* search through interfaces */
    for(i=0; i < data.ifc_len/sizeof(device[0]); ++i)
    {
      DBGV("findIface: Searching index: %d ifr_name: %s address: %s\n",
           i,
           device[i].ifr_name,
           inet_ntoa(((struct sockaddr_in *)&device[i].ifr_addr)->sin_addr)
           );
      
      if     (ioctl(netPath->eventSock, SIOCGIFFLAGS, &device[i]) < 0)
      {
        DBGV("findIface: failed to get device flags\n");
      }
      else if((device[i].ifr_flags&flags) != flags)
      {
        DBGV("findIface does not meet requirements (got: %08x, requested: %08x)\n",
             device[i].ifr_flags,
             flags
            );
      }
      else if(ioctl(netPath->eventSock, SIOCGIFHWADDR, &device[i]) < 0)
      {
        DBGV("findIface: failed to get hardware address\n");
      }
      else if((*communicationTechnology 
               = lookupCommunicationTechnology(device[i].ifr_hwaddr.sa_family)
              ) == PTP_DEFAULT
             )
      {
        DBGV("findIface: unsupported communication technology (%d)\n",
             *communicationTechnology
            );
      }
      else
      {
        DBGV("findIface: found interface (%s)\n", device[i].ifr_name);
        
        memcpy(uuid, 
               device[i].ifr_hwaddr.sa_data,
               PTP_UUID_LENGTH
              );
        memcpy(netPath->portMacAddress,
               device[i].ifr_hwaddr.sa_data,
               6
              );
        memcpy(ifaceName,
               device[i].ifr_name,
               IFACE_NAME_LENGTH
              );
        memcpy(netPath->ifName,
               ifaceName,
               IFACE_NAME_LENGTH
              );
        
        break;
      }
    }
  }
  
  if(ifaceName[0] == '\0')
  {
    ERROR("findIface: failed to find a usable interface\n");
    return 0;
  }

  /* Get Interface Index and store into netPath if 
   * raw socket enabled
   */

  if (netPath->rawSock != -1)
  {
    if(ioctl(netPath->rawSock, // Socket ID
             SIOCGIFINDEX,     // Socket IO Command get interface index
             &device[i]
            ) == -1
      )
    {
       printf("findIface: get ifindex error, socket: %d, device: %s!\n",
              netPath->rawSock,
              device[i].ifr_name
             );
       return 0;
    }

    DBGV("findIface: %s, got interface index: %d\n",
           device[i].ifr_name,
           device[i].ifr_ifindex
        );

    /* Store Interface Index into netPath structure for 
     * later use
     */
    netPath->rawIfIndex = device[i].ifr_ifindex;

    /* Setup L2 Mulcast address for 802.1AS */

    device[i].ifr_hwaddr.sa_data[0] = 0x01;
    device[i].ifr_hwaddr.sa_data[1] = 0x80;
    device[i].ifr_hwaddr.sa_data[2] = 0xC2;
    device[i].ifr_hwaddr.sa_data[3] = 0x00;
    device[i].ifr_hwaddr.sa_data[4] = 0x00;
    device[i].ifr_hwaddr.sa_data[5] = 0x0E;

    if((ioctl(netPath->rawSock, // Socket ID
              SIOCADDMULTI,     // Socket IO Command Add multicast MAC address
              &device[i]        // Interface Request data structure
                  )) == -1)     // -1 if error
    {
       printf("findIface: set multicast MAC error device: %s, raw socket: %d!\n",
              device[i].ifr_name,
              netPath->rawSock
             );
       return 0;
    }
  }
  
  /* Get IP address from event socket */

  if(ioctl(netPath->eventSock, SIOCGIFADDR, &device[i]) < 0)
  {
    PERROR("findIface: failed to get ip address");
    return 0;
  }

  DBGV("findIface: returning, got IP address OK\n");

  return ((struct sockaddr_in *)&device[i].ifr_addr)->sin_addr.s_addr;

#elif defined(BSD_INTERFACE_FUNCTIONS)

  struct ifaddrs *if_list, *ifv4, *ifh;

  if (getifaddrs(&if_list) < 0)
  {
    PERROR("findIface(BSD): getifaddrs() failed");
    return FALSE;
  }

  /* find an IPv4, multicast, UP interface, right name(if supplied) */
  for (ifv4 = if_list; ifv4 != NULL; ifv4 = ifv4->ifa_next)
  {
    if ((ifv4->ifa_flags & IFF_UP) == 0)
      continue;
    if ((ifv4->ifa_flags & IFF_RUNNING) == 0)
      continue;
    if ((ifv4->ifa_flags & IFF_LOOPBACK))
      continue;
    if ((ifv4->ifa_flags & IFF_MULTICAST) == 0)
      continue;
    if (ifv4->ifa_addr->sa_family != AF_INET)  /* must have IPv4 address */
      continue;

    if (ifaceName[0] && strncmp(ifv4->ifa_name, ifaceName, IF_NAMESIZE) != 0)
      continue;

    break;
  }

  if (ifv4 == NULL)
  {
    if (ifaceName[0])
    {
      ERROR("findIface(BSD): interface \"%s\" does not exist, or is not appropriate\n",
            ifaceName
           );
      return FALSE;
    }
    ERROR("findIface(BSD): no suitable interfaces found!");
    return FALSE;
  }

  /* find the AF_LINK info associated with the chosen interface */
  for (ifh = if_list; ifh != NULL; ifh = ifh->ifa_next)
  {
    if (ifh->ifa_addr->sa_family != AF_LINK)
      continue;
    if (strncmp(ifv4->ifa_name, ifh->ifa_name, IF_NAMESIZE) == 0)
      break;
  }

  if (ifh == NULL)
  {
    ERROR("findIface(BSD): could not get hardware address for interface \"%s\"\n", ifv4->ifa_name);
    return FALSE;
  }

  /* check that the interface TYPE is OK */
  if ( ((struct sockaddr_dl *)ifh->ifa_addr)->sdl_type != IFT_ETHER )
  {
    ERROR("findIface(BSD): \"%s\" is not an ethernet interface!\n", ifh->ifa_name);
    return FALSE;
  }

  printf("findIface(BSD): ==> %s %s %s\n", ifv4->ifa_name,
       inet_ntoa(((struct sockaddr_in *)ifv4->ifa_addr)->sin_addr),
        ether_ntoa((struct ether_addr *)LLADDR((struct sockaddr_dl *)ifh->ifa_addr))
        );

  *communicationTechnology = PTP_ETHER;
  memcpy(ifaceName, ifh->ifa_name, IFACE_NAME_LENGTH);
  memcpy(uuid, LLADDR((struct sockaddr_dl *)ifh->ifa_addr), PTP_UUID_LENGTH);

  return ((struct sockaddr_in *)ifv4->ifa_addr)->sin_addr.s_addr;

#endif
}

/* start all of the Netwok UDP and Layer 2 raw stuff */
/* must specify 'subdomainName', optionally 'ifaceName', if not then pass ifaceName == "" */
/* returns other args */
/* on socket options, see the 'socket(7)' and 'ip' man pages */

Boolean netInit(NetPath *netPath, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
  int    temp, i;
  struct in_addr interfaceAddr, netAddr;
  struct sockaddr_in addr;    /* Internet Socket data */
  struct sockaddr_ll rawaddr; /* Link Layer raw socket data */
  struct ip_mreq imr;
  char   addrStr[NET_ADDRESS_LENGTH];
  char   interface_name[IFACE_NAME_LENGTH];
  char * s;
  
  DBG("netInit: entering\n");

      DBGV("netInit: ptpClock: %p, Port ID: %d\n",
           ptpClock,
           ptpClock->port_id_field
          );

  DBG("netInit: Setting up sockets\n");
  /* open event and general sockets for IEEE 1588 operation */

  if( (netPath->eventSock    = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0
    || (netPath->generalSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0 )
  {
    PERROR("netInit: failed to initalize event or general socket");
    return FALSE;
  }
  DBGV("netInit: created event socket   %d\n",
       netPath->eventSock
      );
  DBGV("netInit: created general socket %d\n",
       netPath->generalSock
      );

  /* open raw socket if 802.1AS PTP operation is enabled */

  if (rtOpts->ptp8021AS)
  {
     if( (netPath->rawSock = socket(PF_PACKET, SOCK_RAW, htons(0x88F7))
         ) < 0
       )

     {
       PERROR("netInit: failed to initalize raw socket");
       return FALSE;
     }
     DBGV("netInit: created raw socket     %d\n",
          netPath->rawSock
         );
  }
  else
  {
    /* Not 802.1AS PTP operation, make sure it isn't tried to
     * be used later
     */
    DBGV("netInit: raw socket create skipped\n");

    netPath->rawSock = -1;
  }

  /* validate user specified network interface,
   * or find and validate network interface 
   */

  // AKB: new code for multiple port support.  If not bound to an interface, use port
  // id number to specify ethernet interface to use eth0 + (port id - 1)
  //
  if (rtOpts->ifaceName[0] == '\0')
  {
      strncpy(interface_name, "eth0", IFACE_NAME_LENGTH);
      interface_name[3] += (ptpClock->port_id_field -1);
      DBGV("netInit: rtOpts->ifaceName port id: %d set to %s\n",
           ptpClock->port_id_field,
           interface_name
          );
  }
  else
  {
     strncpy(interface_name, rtOpts->ifaceName, IFACE_NAME_LENGTH);
     DBGV("netInit: rtOpts->ifaceName port id: %d is specified as %s\n",
           ptpClock->port_id_field,
           interface_name
          );
  }

  interfaceAddr.s_addr  
        = findIface(interface_name,
                    &ptpClock->port_communication_technology,
                    ptpClock->port_uuid_field,
                    netPath
                   );

  if (!interfaceAddr.s_addr)
  {
    PERROR("netInit: failed to get a valid interface (findIface failed)\n");    
    return FALSE;
  }  

  /* Interface found, for V2 support, copy MAC address UUID into the clock identity field
   * and format into an EUI-64 address.  EUI-48 to EUI-64 conversion consists of copying
   * OUI to first 3 bytes, then 0xFF and 0xFE in next 2 bytes and then copying last 3
   * bytes of MAC address to last 3 bytes of EUI-64 (for total of 8 bytes).
   */

  memcpy (ptpClock->port_clock_identity,        // Copy OUI field
          ptpClock->port_uuid_field,
          3
         );

  ptpClock->port_clock_identity[3] = 0xFF;      // IEEE EUI-48 (MAC address) to EUI-64
  ptpClock->port_clock_identity[4] = 0xFE;


  memcpy (&ptpClock->port_clock_identity[5],    // Copy remaining 3 bytes of MAC address
          &ptpClock->port_uuid_field[3],
          3
         );

  /* Set socket options for event and general sockets for address reuse */

  temp = 1;
  if(  setsockopt(netPath->eventSock,
                  SOL_SOCKET,
                  SO_REUSEADDR,
                  &temp,
                  sizeof(int)
                 ) < 0
    || setsockopt(netPath->generalSock,
                  SOL_SOCKET,
                  SO_REUSEADDR,
                  &temp,
                   sizeof(int)
                 ) < 0
    )
  {
    DBG("netInit: failed to set socket reuse\n");
  }
  DBG("netInit: socket reuse set OK\n");

  /* bind sockets */
  /* need INADDR_ANY to allow receipt of multi-cast and uni-cast messages */

  bzero(&addr, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port        = htons(PTP_EVENT_PORT);

  if(bind(netPath->eventSock,
          (struct sockaddr*)&addr,
          sizeof(struct sockaddr_in)
         ) < 0
    )
  {
    PERROR("netInit: failed to bind event socket");
    return FALSE;
  }
  
  addr.sin_port = htons(PTP_GENERAL_PORT);
  if(bind(netPath->generalSock,
          (struct sockaddr*)&addr,
          sizeof(struct sockaddr_in)
         ) < 0
    )
  {
    PERROR("netInit: failed to bind general socket");
    return FALSE;
  }
  DBGV("netInit: event and general socket binds complete\n");

  if (rtOpts->ptp8021AS)
  {

     /* For PTP 802.1AS operation, we need to bind the raw socket */

     bzero(&rawaddr, sizeof(rawaddr));
     rawaddr.sll_family   = AF_PACKET;
     rawaddr.sll_ifindex  = netPath->rawIfIndex;
     rawaddr.sll_protocol = htons(0x88F7); 

     if(bind(netPath->rawSock,
             (struct sockaddr*)&rawaddr,
             sizeof(struct sockaddr_ll)
            ) < 0
       )
     {
       PERROR("netInit: failed to bind raw socket");
       return FALSE;
     }
     DBGV("netInit: raw socket %d bind complete\n",
          netPath->rawSock
         );
     //
     // Raw socket now setup OK for 802.1AS
     // Prepare Output buffer with standard 802.1AS Multicast
     // address, the source MAC for this interface and the
     // Ethertype.  NOTE: Offsets have 2 added to them
     // so that the PTP message payload is on a 16 byte
     // aligned address.

     /* Setup L2 Mulcast address for 802.1AS */

     ptpClock->outputBuffer[2] = 0x01;
     ptpClock->outputBuffer[3] = 0x80;
     ptpClock->outputBuffer[4] = 0xC2;
     ptpClock->outputBuffer[5] = 0x00;
     ptpClock->outputBuffer[6] = 0x00;
     ptpClock->outputBuffer[7] = 0x0E;

     /* Debug code below for using broadcast instead of multicast,
        commented out

     memset(&ptpClock->outputBuffer[2],
            0xff,
            6);

     */

     memcpy(&ptpClock->outputBuffer[8],
            netPath->portMacAddress,
            6);

     ptpClock->outputBuffer[14] = 0x88; // Set Ethertype for PTP over 802.3 encapsulation
     ptpClock->outputBuffer[15] = 0xF7;
   

  }
  else
  {
     DBGV("netInit: skipping bind of raw socket\n");
  }


  
  /* set general and port address */
  *(Integer16*)ptpClock->event_port_address   = PTP_EVENT_PORT;
  *(Integer16*)ptpClock->general_port_address = PTP_GENERAL_PORT;
  
  /* AKB: Setup PDelay Multicast address for V2 support */

  if(!inet_aton(DEFAULT_PTP_PDELAY_ADDRESS, &netAddr))
  {
    ERROR("netInit: failed to encode PDelay multicast address: %s\n",
          DEFAULT_PTP_PDELAY_ADDRESS
         );
    return FALSE;
  }
  // PDelay multicast address parsed OK, set unicast address to user specified value    
  netPath->pdelayMulticastAddr = netAddr.s_addr;
  
  /* send a uni-cast address if specified (useful for testing) */

  if(rtOpts->unicastAddress[0])
  {
    if(!inet_aton(rtOpts->unicastAddress, &netAddr))
    {
      ERROR("netInit: failed to encode user specified uni-cast address: %s\n",
            rtOpts->unicastAddress
           );
      return FALSE;
    }
    // Unicast address parsed OK, set unicast address to user specified value    
    netPath->unicastAddr = netAddr.s_addr;
  }
  else
  {
    // Unicast address not asked for, set unicast address to zero
    netPath->unicastAddr = 0;
  }
  
  /* resolve PTP subdomain */
  if(!lookupSubdomainAddress(rtOpts->subdomainName, addrStr))
  {
    ERROR("netInit: failed lookup of multi-cast address: %s\n", addrStr);
    return FALSE;
  }

  if(!inet_aton(addrStr, &netAddr))
  {
    ERROR("netInit: failed to encode multi-cast address: %s\n", addrStr);
    return FALSE;
  }
  
  netPath->multicastAddr = netAddr.s_addr;
  
  s = addrStr;
  for(i = 0; i < SUBDOMAIN_ADDRESS_LENGTH; ++i)
  {
    ptpClock->subdomain_address[i] = strtol(s, &s, 0);
    
    if(!s)
      break;
    
    ++s;
  }
  
  /* multicast send only on specified interface */

  imr.imr_multiaddr.s_addr = netPath->multicastAddr;
  imr.imr_interface.s_addr = interfaceAddr.s_addr;

  if(  setsockopt(netPath->eventSock,
                  IPPROTO_IP,
                  IP_MULTICAST_IF,
                  &imr.imr_interface.s_addr,
                  sizeof(struct in_addr)
                 ) < 0
    || setsockopt(netPath->generalSock, 
                  IPPROTO_IP,
                  IP_MULTICAST_IF,
                  &imr.imr_interface.s_addr,
                  sizeof(struct in_addr)
                 ) < 0
    )
  {
    PERROR("netInit: failed to enable multi-cast on the interface");
    return FALSE;
  }
  
  /* join regular multicast group (for receiving) on specified interface */

  if(  setsockopt(netPath->eventSock,
                  IPPROTO_IP,
                  IP_ADD_MEMBERSHIP,
                  &imr,
                  sizeof(struct ip_mreq)
                 ) < 0
    || setsockopt(netPath->generalSock,
                  IPPROTO_IP,
                  IP_ADD_MEMBERSHIP,
                  &imr,
                  sizeof(struct ip_mreq)
                 ) < 0
    )
  {
    PERROR("netInit: failed to join the regular multi-cast group");
    return FALSE;
  }


  /* join PDelay multicast group (for receiving) on specified interface */
  imr.imr_multiaddr.s_addr = netPath->pdelayMulticastAddr;

  if(  setsockopt(netPath->eventSock,
                  IPPROTO_IP,
                  IP_ADD_MEMBERSHIP,
                  &imr,
                  sizeof(struct ip_mreq)
                 ) < 0
    || setsockopt(netPath->generalSock,
                  IPPROTO_IP,
                  IP_ADD_MEMBERSHIP,
                  &imr,
                  sizeof(struct ip_mreq)
                 ) < 0
    )
  {
    PERROR("netInit: failed to join the pdelay multi-cast group");
    return FALSE;
  }

  /* set socket time-to-live to 1 */
  temp = 1;
  if(  setsockopt(netPath->eventSock,
                  IPPROTO_IP,
                  IP_MULTICAST_TTL,
                  &temp,
                  sizeof(int)
                 ) < 0
    || setsockopt(netPath->generalSock,
                  IPPROTO_IP,
                  IP_MULTICAST_TTL,
                  &temp,
                  sizeof(int)
                 ) < 0
    )
  {
    PERROR("netInit: failed to set the multi-cast time-to-live");
    return FALSE;
  }
  

  /* Enable IP multicast loopback.  This is so for Software based timestamping,
   *  all multicast frames we send will be looped back to us with a time stamp
   * (as that is enabled later in this function).  
   *
   * That way, we get a reasonable software estimate for frames that 
   * we need a transmit time value, we can read the timestamp
   * of the loopbacked frame sent back to us (by checking to see 
   * if it is from self) and this option is supported by many operating systems
   */
#ifdef CONFIG_MPC831X
  temp = 0; // HW timestamping, no need for looping back messages, disable IP multicast loop
#else
  temp = 1; // SW timestamping, enable IP multicast loop
#endif
  if(  setsockopt(netPath->eventSock,
                  IPPROTO_IP,
                  IP_MULTICAST_LOOP,
                  &temp,
                  sizeof(int)
                 ) < 0
    || setsockopt(netPath->generalSock,
                  IPPROTO_IP,
                  IP_MULTICAST_LOOP,
                  &temp, 
                  sizeof(int)
                 ) < 0
    )
  {
    PERROR("netInit: failed to enable multi-cast loopback");
    return FALSE;
  }

  /* make timestamps available through recvmsg() */

  temp = 1;

  if(  setsockopt(netPath->eventSock,
                  SOL_SOCKET,
                  SO_TIMESTAMP,
                  &temp,
                  sizeof(int)
                 ) < 0
    || setsockopt(netPath->generalSock,
                  SOL_SOCKET,
                  SO_TIMESTAMP,
                  &temp,
                  sizeof(int)
                 ) < 0
    )
  {
    PERROR("netInit: failed to enable receive time stamps");
    return FALSE;
  }

  if (rtOpts->ptp8021AS)
  {
     if(  setsockopt(netPath->rawSock,
                     SOL_SOCKET,
                     SO_TIMESTAMP,
                     &temp,
                     sizeof(int)
                    ) < 0
       )
     {
       PERROR("netInit: failed to enable raw socket time stamps");
       return FALSE;
     }
  }

  DBG("netInit: exiting OK\n");
#ifdef CONFIG_MPC831X
  mpc831x_netPath = netPath;
#endif
  return TRUE;
}

/* shut down the UDP and raw socket stuff */

Boolean netShutdown(NetPath *netPath)
{
  struct ip_mreq imr;
  struct ifreq ifr;

  /* Setup Interface Request (raw socket)
   * and IP Multicast Request (UDP socket)
   * structures.
   */
  bzero(&ifr, sizeof(ifr));
  strncpy((char *)ifr.ifr_name, netPath->ifName, IFNAMSIZ);
  ifr.ifr_ifindex = netPath->rawIfIndex;

  imr.imr_multiaddr.s_addr = netPath->multicastAddr;
  imr.imr_interface.s_addr = htonl(INADDR_ANY);

  // Drop IP Multicast membership on both UDP sockets

  setsockopt(netPath->eventSock,
             IPPROTO_IP,
             IP_DROP_MEMBERSHIP,
             &imr,
             sizeof(struct ip_mreq)
            );
  setsockopt(netPath->generalSock,
             IPPROTO_IP,
             IP_DROP_MEMBERSHIP,
             &imr,
             sizeof(struct ip_mreq)
            );

  imr.imr_multiaddr.s_addr = netPath->pdelayMulticastAddr;

  setsockopt(netPath->eventSock,
             IPPROTO_IP,
             IP_DROP_MEMBERSHIP,
             &imr,
             sizeof(struct ip_mreq)
            );
  setsockopt(netPath->generalSock,
             IPPROTO_IP,
             IP_DROP_MEMBERSHIP,
             &imr,
             sizeof(struct ip_mreq)
            );
  
  netPath->multicastAddr       = 0;
  netPath->pdelayMulticastAddr = 0;
  netPath->unicastAddr         = 0;

  // Remove 802.1AS PTP multicast address

  // Close sockets if not already closed
  
  if(netPath->eventSock > 0)
  {
    close(netPath->eventSock);
  }
  netPath->eventSock = -1;
  
  if(netPath->generalSock > 0)
  {
    close(netPath->generalSock);
  }
  netPath->generalSock = -1;

  if(netPath->rawSock > 0)
  {
    /* Raw socket, Delete 802.1AS multicast Mac address from port 
     * and then close socket.
     */

    ifr.ifr_hwaddr.sa_data[0] = 0x01;
    ifr.ifr_hwaddr.sa_data[1] = 0x80;
    ifr.ifr_hwaddr.sa_data[2] = 0xC2;
    ifr.ifr_hwaddr.sa_data[3] = 0x00;
    ifr.ifr_hwaddr.sa_data[4] = 0x00;
    ifr.ifr_hwaddr.sa_data[5] = 0x0E;


    ioctl(netPath->rawSock, // Socket ID
          SIOCDELMULTI,     // Socket IO Command Add multicast MAC address
          &ifr              // Interface Request data structure
         );                 // -1 if error (not checked as we are closing anyway)


    close(netPath->rawSock);
  }
  netPath->rawSock = -1;
    
  return TRUE;
}

/*
 * netSelect:
 *
 * Function to check multiple sockets on a single port if anything is ready to receive
 * with an optional time to wait for event 
 */

int netSelect(TimeInternal *timeout, NetPath *netPath)
{
  int ret, nfds;
  fd_set readfds;
  struct timeval tv, *tv_ptr;
  
  if(timeout < 0) /* Make sure we have a non-negative timeout */
  {
    return FALSE;
  }
  
  /* Setup fd_set structure for select function */

  FD_ZERO(&readfds);

  if  (netPath->eventSock != -1)
  {
    FD_SET(netPath->eventSock,   &readfds);
  }

  if  (netPath->generalSock != -1)
  {
    FD_SET(netPath->generalSock, &readfds);
  }

  if  (netPath->rawSock != -1)
  {
    FD_SET(netPath->rawSock,     &readfds);
  }
  
  /* Set time to wait if any, else setup NULL pointer */

  if(timeout)
  {
    /* Pointer to timeout structure is not NULL */

    /* timeout argument is in seconds and nanoseconds.
     * select() function wants seconds and microseconds
     * convert and copy to local structure
     */
    tv.tv_sec  = timeout->seconds;
    tv.tv_usec = timeout->nanoseconds/1000;
    tv_ptr     = &tv;
    DBGV("netSelect: timeout requested %ds %dns\n",
         (unsigned int)timeout->seconds,
         (int) timeout->nanoseconds
        );
  }
  else
  {
    /* If the timeout argument is a NULL pointer, select() blocks until 
     * an event causes one of the masks to be returned with a valid
     * (non-zero) value or until a signal occurs that needs to be
     * delivered. 
     */
    DBGV("netSelect: NULL timeout pointer, wait for event\n");
    tv_ptr = 0;
  }

  /* Find highest Number Socket for select() function */
  
  if(netPath->eventSock > netPath->generalSock)
  {
    if (netPath->rawSock > netPath->eventSock)
    {
       nfds = netPath->rawSock;
    }
    else
    {
       nfds = netPath->eventSock;
    }
  }
  else
  {
    if (netPath->rawSock > netPath->generalSock)
    {
       nfds = netPath->rawSock;
    }
    else
    {
       nfds = netPath->generalSock;
    }
  }

  /* Call select function to check all receive sockets with optional timeout */

  ret = select(nfds + 1,  // nfds (highest socket number + 1)
               &readfds,  // readfds
               0,         // writefds
               0,         // exceptfds
               tv_ptr     // timeout structure pointer or NULL
               ) > 0;

  if(ret < 0)
  {
    if(errno == EAGAIN || errno == EINTR)
    {
      DBGV("netSelect: errno EAGAIN or EINTR (%d)\n",errno); 
      ret = 0;
    }
    DBG("netSelect: unexpected errno: %d, select returns %d\n", errno, ret);
  }
  DBGV("netSelect: return: %d\n",ret);  
  return ret;
}

/*
 * netSelectAll:
 *
 * Function to check multiple sockets on all ports if anything is ready to receive
 * with an optional time to wait for event 
 */

int netSelectAll(TimeInternal *timeout, PtpClock *ptpClock)
{
  int ret, nfds;
  fd_set readfds;
  struct timeval tv, *tv_ptr;
  int i;

  
  if(timeout < 0) /* Make sure we have a non-negative timeout */
  {
    return FALSE;
  }

  /* Setup fd_set structure for select function */
  /* Loop through all ports, to get receive sockets */
  /* Find highest Number Socket for select() function */

  FD_ZERO(&readfds);
  nfds = 0;

  for (i=0; i<MAX_PTP_PORTS; i++)
  {
    if  (ptpClock->netPath.eventSock != -1)
    {
      FD_SET(ptpClock->netPath.eventSock,   &readfds);
      if (ptpClock->netPath.eventSock > nfds)
      {
          nfds = ptpClock->netPath.eventSock;
      }
    }

    if  (ptpClock->netPath.generalSock != -1)
    {
      FD_SET(ptpClock->netPath.generalSock, &readfds);
      if (ptpClock->netPath.generalSock > nfds)
      {
          nfds = ptpClock->netPath.generalSock;
      }
    }

    if  (ptpClock->netPath.rawSock != -1)
    {
      FD_SET(ptpClock->netPath.rawSock,     &readfds);
      if (ptpClock->netPath.rawSock > nfds)
      {
          nfds = ptpClock->netPath.rawSock;
      }
    }

    ptpClock++;  // Get pointer to next port structure and continue

  } // for loop end
  
  /* Set time to wait if any, else setup NULL pointer */

  if(timeout)
  {
    /* Pointer to timeout structure is not NULL */

    /* timeout argument is in seconds and nanoseconds.
     * select() function wants seconds and microseconds
     * convert and copy to local structure
     */
    tv.tv_sec  = timeout->seconds;
    tv.tv_usec = timeout->nanoseconds/1000;
    tv_ptr     = &tv;
    DBGV("netSelectAll: timeout requested %ds %dns\n",
         (unsigned int)timeout->seconds,
         (int) timeout->nanoseconds
        );
  }
  else
  {
    /* If the timeout argument is a NULL pointer, select() blocks until 
     * an event causes one of the masks to be returned with a valid
     * (non-zero) value or until a signal occurs that needs to be
     * delivered. 
     */
    DBGV("netSelectAll: NULL timeout pointer, wait for event\n");
    tv_ptr = 0;
  }

  /* Call select function to check all receive sockets with optional timeout */

  ret = select(nfds + 1,  // nfds (highest socket number + 1)
               &readfds,  // readfds
               0,         // writefds
               0,         // exceptfds
               tv_ptr     // timeout structure pointer or NULL
               ) > 0;

  if(ret < 0)
  {
    if(errno == EAGAIN || errno == EINTR)
    {
      DBGV("netSelectAll: errno EAGAIN or EINTR (%d)\n",errno); 
      ret = 0;
    }
    DBG("netSelectAll: unexpected errno: %d, select returns %d\n",errno, ret); 
  }
  DBGV("netSelectAll: return: %d\n",ret);  
  return ret;
}

ssize_t netRecvEvent(Octet *buf, 
                     TimeInternal *time,
                     NetPath *netPath
                    )
{
  ssize_t ret;
  struct msghdr msg;
  struct iovec vec[1];
  struct sockaddr_in from_addr;
  union {
      struct cmsghdr cm;
      char control[CMSG_SPACE(sizeof(struct timeval))];
  } cmsg_un;
  struct cmsghdr *cmsg;
  struct timeval *tv;
  
  vec[0].iov_base = buf;
  vec[0].iov_len  = PACKET_SIZE;
  
  memset(&msg,       0, sizeof(msg));
  memset(&from_addr, 0, sizeof(from_addr));
  memset(buf,        0, PACKET_SIZE);
  memset(&cmsg_un,   0, sizeof(cmsg_un));
  
  msg.msg_name       = (caddr_t)&from_addr;
  msg.msg_namelen    = sizeof(from_addr);
  msg.msg_iov        = vec;
  msg.msg_iovlen     = 1;
  msg.msg_control    = cmsg_un.control;
  msg.msg_controllen = sizeof(cmsg_un.control);
  msg.msg_flags = 0;

  DBGV("netRecvEvent:   %s calling recvmsg,  socket: %d\n",
       netPath->ifName,
       netPath->eventSock
      );
  
  ret = recvmsg(netPath->eventSock, 
                &msg, 
                MSG_DONTWAIT
               );
  
  if(ret <= 0)
  {
    if(errno == EAGAIN || errno == EINTR)
    {
      DBGV("netRecvEvent:   No message (EAGAIN or EINTR)\n");
      return 0;
    }
    
    DBGV("netRecvEvent:   recvfrom error : %d\n",
         ret
        );
    return ret;
  }





  if(msg.msg_flags&MSG_TRUNC)
  {
    ERROR("netRecvEvent:   received truncated message\n");
    return 0;
  }
  
  /* get time stamp of packet */
  if(!time)
  {
    ERROR("netRecvEvent:   null time stamp argument\n");
    return 0;
  }
  
  if(msg.msg_flags&MSG_CTRUNC)
  {
    ERROR("netRecvEvent:   truncated ancillary data\n");
    return 0;
  }
  
  if(msg.msg_controllen < sizeof(cmsg_un.control))
  {
    ERROR("netRecvEvent:   short ancillary data (%d/%d)\n",
      msg.msg_controllen, (int)sizeof(cmsg_un.control));
    
    return 0;
  }
  
  tv = 0;
  for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg))
  {
    if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMP)
      tv = (struct timeval *)CMSG_DATA(cmsg);
  }
  
  if(tv)
#ifdef CONFIG_MPC831X
  {

/*  Original code from Freescale patch for versions prior to 2007-08-27 versions

    if (port_state == PTP_MASTER)
    {
      mpc831x_get_tx_time(time);
      DBGV("netRecvEvent:   HW tx time %9.9us %9.9dns\n", time->seconds, time->nanoseconds);
    }
    else
    {
      mpc831x_get_rx_time(time);
      DBGV("netRecvEvent:   HW rx time %9.9us %9.9dns\n", time->seconds, time->nanoseconds);
    }
*/

    /* Time is gotten later in protocol.c when the frame is parsed to get sequence number
     * and frame type.  This is because the newer Ethernet driver from Freescale
     * now has a queue for both and when you get the timestamp, you pass the message
     * type and sequence number for which you want to get the timestamp.
     *
     * As this is now done in protocol.c, we'll just return 0 in the timestamp.
     * This also allows us to not have to pass the state in this call.
     */
    time->seconds     = 0;
    time->nanoseconds = 0;
  }
#else
  {
    time->seconds     = tv->tv_sec;
    time->nanoseconds = tv->tv_usec*1000;
    DBGV("netRecvEvent:   recv time stamp %us %dns\n", time->seconds, time->nanoseconds);
  }
#endif
  else
  {
    /* do not try to get by with recording the time here, better to fail
       because the time recorded could be well after the message receive,
       which would put a big spike in the offset signal sent to the clock servo */
    DBG("netRecvEvent:   no receive time stamp\n");
    return 0;
  }

  DBGV("netRecvEvent:   %s length: %d\n",
       netPath->ifName,
       ret
      );

  /* Temp for debug: dump hex data *

  int i;

  if ((debugLevel & 4) == 4 )
  {
    for (i=0; i<ret; i++)
    {
      if (i % 16 == 0)
      {
         fprintf(stderr, "\nnetRecvEvent:   ");
         fprintf(stderr, "%4.4x:", i);
      }
      fprintf(stderr, " %2.2x", buf[i]);
    }
    fprintf(stderr,"\n\n");
  }
  * */

  return ret;
}

ssize_t netRecvGeneral(Octet *buf, NetPath *netPath)
{
  ssize_t ret;
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(struct sockaddr_in);
  
  DBGV("netRecvGeneral: %s calling recvfrom, socket: %d\n",
       netPath->ifName,
       netPath->generalSock
      );
 
  ret = recvfrom(netPath->generalSock,
                 buf,
                 PACKET_SIZE,
                 MSG_DONTWAIT,
                 (struct sockaddr *)&addr,
                 &addr_len
                );
  if(ret <= 0)
  {
    if(errno == EAGAIN || errno == EINTR)
    {
      DBGV("netRecvGeneral: No message (EAGAIN or EINTR)\n");
      return 0;
    }
    
    DBGV("netRecvGeneral: recvfrom error : %d\n",
         ret
        );
    return ret;
  }
  DBGV("netRecvGeneral: %s length: %d\n",
       netPath->ifName,
       ret
      );
  /* Temp for debug: dump hex data *

  int i;

  if ((debugLevel & 4) == 4 )
  {
    for (i=0; i<ret; i++)
    {
      if (i % 16 == 0)
      {
         fprintf(stderr, "\nnetRecvGeneral: ");
         fprintf(stderr, "%4.4x:", i);
      }
      fprintf(stderr, " %2.2x", buf[i]);
    }
    fprintf(stderr,"\n\n");
  }
  * */

  return ret;
}

ssize_t netRecvRaw(Octet *buf, NetPath *netPath)
{
  ssize_t ret;
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(struct sockaddr_in);
  
  DBGV("netRecvRaw:     %s calling recvfrom, socket: %d\n",
       netPath->ifName,
       netPath->rawSock
      );
 
  ret = recvfrom(netPath->rawSock,
                 buf,
                 PACKET_SIZE,
                 MSG_DONTWAIT,
                 (struct sockaddr *)&addr,
                 &addr_len
                );
  if(ret <= 0)
  {
    if(errno == EAGAIN || errno == EINTR)
    {
      DBGV("netRecvRaw:     No message (EAGAIN or EINTR)\n");
      return 0;
    }
    
    DBGV("netRecvRaw:     recvfrom error : %d\n",
         ret
        );
    return ret;
  }

  DBGV("netRecvRaw:     %s length: %d\n",
       netPath->ifName,
       ret
      );

  /* Temp for debug: dump hex data 

  int i;

  if ((debugLevel & 4) == 4 )
  {
    for (i=0; i<ret; i++)
    {
      if (i % 16 == 0)
      {
         fprintf(stderr, "\nnetRecvRaw:     ");
         fprintf(stderr, "%4.4x:", i);
      }
      fprintf(stderr, " %2.2x", buf[i]);
    }
    fprintf(stderr,"\n\n");
  }
  * */

  return ret;
}


ssize_t netSendEvent(Octet *buf, UInteger16 length, NetPath *netPath, Boolean pdelay)
{
  ssize_t ret;
  struct sockaddr_in addr;

  /* Temp for debug: dump hex data */

  int i;

  if ((debugLevel & 4) == 4 )
  {
    for (i=0; i<length; i++)
    {
      if (i % 16 == 0)
      {
         fprintf(stderr, "\nnetSendEvent:   ");
         fprintf(stderr, "%4.4x:", i);
      }
      fprintf(stderr, " %2.2x", buf[i]);
    }
    fprintf(stderr,"\n\n");
  }
  /* */
  
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(PTP_EVENT_PORT);

#ifdef CONFIG_MPC831X
  if (!netPath->unicastAddr)
  {
#endif
  if (pdelay)
  {
    addr.sin_addr.s_addr = netPath->pdelayMulticastAddr;
  }
  else
  {
    addr.sin_addr.s_addr = netPath->multicastAddr;
  }
  
  ret = sendto(netPath->eventSock,
               buf,
               length,
               0,
               (struct sockaddr *)&addr,
               sizeof(struct sockaddr_in)
              );

  if(ret <= 0)
  {
    DBG("netSendEvent: error sending multi-cast event message\n");
    return ret;
  }
#ifdef CONFIG_MPC831X
  }
#endif
  
  if(netPath->unicastAddr)
  {
    addr.sin_addr.s_addr = netPath->unicastAddr;
    
    ret = sendto(netPath->eventSock,
                 buf,
                 length,
                 0,
                 (struct sockaddr *)&addr,
                 sizeof(struct sockaddr_in)
                );
    if(ret <= 0)
    {
      DBG("netSendEvent: error sending uni-cast event message\n");
    }
  }
  
  DBGV("netSendEvent: %s requested: %d, sent: %d\n",
       netPath->ifName,
       length,
       ret
      );  
  return ret;
}

ssize_t netSendGeneral(Octet *buf, UInteger16 length, NetPath *netPath, Boolean pdelay)
{
  ssize_t ret;
  struct sockaddr_in addr;

  /* Temp for debug: dump hex data */

  int i;

  if ((debugLevel & 4) == 4 )
  {
    for (i=0; i<length; i++)
    {
      if (i % 16 == 0)
      {
         fprintf(stderr, "\nnetSendGeneral: ");
         fprintf(stderr, "%4.4x:", i);
      }
      fprintf(stderr, " %2.2x", buf[i]);
    }
    fprintf(stderr,"\n\n");
  }

  /* */
  
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(PTP_GENERAL_PORT);

#ifdef CONFIG_MPC831X
  if (!netPath->unicastAddr)
  {
#endif

  if (pdelay)
  {
    addr.sin_addr.s_addr = netPath->pdelayMulticastAddr;
  }
  else
  {
    addr.sin_addr.s_addr = netPath->multicastAddr;
  }
  
  ret = sendto(netPath->generalSock,
               buf,
               length,
               0,
               (struct sockaddr *)&addr,
               sizeof(struct sockaddr_in)
              );
  if(ret <= 0)
  {
    DBG("netSendGeneral: error sending multi-cast general message\n");
    return ret;
  }
  
#ifdef CONFIG_MPC831X
  }
#endif

  if(netPath->unicastAddr)
  {
    addr.sin_addr.s_addr = netPath->unicastAddr;
    
    ret = sendto(netPath->generalSock,  // was eventSock, is this in original code??
                 buf,
                 length,
                 0,
                 (struct sockaddr *)&addr,
                 sizeof(struct sockaddr_in)
                );

    if(ret <= 0)
    {
      DBG("netSendGeneral: error sending uni-cast general message\n");
    }
  }

  DBGV("netSendGeneral: %s requested: %d, sent: %d\n",
      netPath->ifName,
      length,
      ret
     );  
  return ret;
}

ssize_t netSendRaw(Octet *buf, UInteger16 length, NetPath *netPath)
{
  ssize_t ret;
  struct  sockaddr_ll rawaddr;

  DBGV("netSendRaw: buf %p, length: %d\n", buf, length);

  /* Temp for debug: dump hex data */

int i;

  if ((debugLevel & 4) == 4 )
  {
  
    for (i=0; i<length; i++)
    {
      if (i % 16 == 0)
      {
         fprintf(stderr, "\nnetSendRaw:     ");
         fprintf(stderr, "%4.4x:", i);
      }
      fprintf(stderr, " %2.2x", buf[i]);
    }
    fprintf(stderr,"\n\n");
  }

  /* */

  bzero(&rawaddr, sizeof(rawaddr));
  rawaddr.sll_family   = AF_PACKET;
  rawaddr.sll_ifindex  = netPath->rawIfIndex;
  rawaddr.sll_protocol = htons(0x88F7); 

  ret = sendto(netPath->rawSock,
               buf,
               length,
               0,
               (struct sockaddr *)&rawaddr,
               sizeof(struct sockaddr_ll)
              );
  if(ret <= 0)
  {
    DBG("netSendRaw: error %d sending raw frame\n",
        ret
       );
    return ret;
  }
  
  DBGV("netSendRaw: %s requested:%d, sent:%d\n",
      netPath->ifName,
      length,
      ret
     );
  
  return ret;
}

// eof net.c
