#ifndef IOTORII_ROUTING_H
#define IOTORII_ROUTING_H


#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "lib/list.h"
#include "sys/ctimer.h"

#define uip_create_linklocal_iotorii_nodes_mcast(addr) uip_ip6addr((addr), 0xfd00, 0, 0, 0, 0, 0, 0, 0x001a)

#endif /* RPL_H */
