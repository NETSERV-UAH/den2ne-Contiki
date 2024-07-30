/*
 * Copyright (c) 2017, RISE SICS.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \addtogroup routing
 * @{
 *
 * \file
 *         A routing protocol that does nothing
 *
 * \author Simon Duquennoy <simon.duquennoy@ri.se>
 */

#include "net/routing/iotorii-routing/iotorii-routing.h"
#include "net/routing/routing.h"

#include "contiki.h"
#include "contiki-net.h"

#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "IOTORII"
#define LOG_LEVEL LOG_LEVEL_RPL
uip_ipaddr_t iotorii_multicast_addr;

/*---------------------------------------------------------------------------*/
static void
init(void)
{
  LOG_INFO("initializing\n");
  uip_create_linklocal_iotorii_nodes_mcast(&iotorii_multicast_addr);
  uip_ds6_maddr_add(&iotorii_multicast_addr);

  uip_sr_init();
}
/*---------------------------------------------------------------------------*/
static void
set_global_address(uip_ipaddr_t *prefix, uip_ipaddr_t *iid)
{
  static uip_ipaddr_t root_ipaddr;
  const uip_ipaddr_t *default_prefix;
  int i;
  uint8_t state;

  default_prefix = uip_ds6_default_prefix();

  //Assign a unique local address (RFC4193,http://tools.ietf.org/html/rfc4193).
  if(prefix == NULL) {
    uip_ip6addr_copy(&root_ipaddr, default_prefix);
  } else {
    memcpy(&root_ipaddr, prefix, 8);
  }
  if(iid == NULL) {
    uip_ds6_set_addr_iid(&root_ipaddr, &uip_lladdr);
  } else {
    memcpy(((uint8_t*)&root_ipaddr) + 8, ((uint8_t*)iid) + 8, 8);
  }

  uip_ds6_addr_add(&root_ipaddr, 0, ADDR_AUTOCONF);

  LOG_INFO("IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      LOG_INFO("-- ");
      LOG_INFO_6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      LOG_INFO_("\n");
    }
  }
}

void
iotorii_set_prefix(uip_ipaddr_t *prefix, uip_ipaddr_t *iid)
{
  static uint8_t initialized = 0;

  if(!initialized) {
    set_global_address(prefix, iid);
    initialized = 1;
  }
}
/*---------------------------------------------------------------------------*/

int
iotorii_root_start(void)
{
  /*struct uip_ds6_addr *root_if;
  int i;
  uint8_t state;
  uip_ipaddr_t *ipaddr = NULL;*/
  
  // NECESSARY FOR BROADCAST
  static uint8_t initialized = 0;

  if(!initialized) {
    set_global_address(NULL, NULL);
    initialized = 1;
  }

  /*for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       state == ADDR_PREFERRED &&
       !uip_is_addr_linklocal(&uip_ds6_if.addr_list[i].ipaddr)) {
      ipaddr = &uip_ds6_if.addr_list[i].ipaddr;
    }
  }

  root_if = uip_ds6_addr_lookup(ipaddr);
  if(ipaddr != NULL || root_if != NULL) {

    rpl_dag_init_root(RPL_DEFAULT_INSTANCE, ipaddr,
      (uip_ipaddr_t *)rpl_get_global_address(), 64, UIP_ND6_RA_FLAG_AUTONOMOUS);
    rpl_dag_update_state();

    LOG_INFO("created a new RPL DAG\n");
    return 0;
  } else {
    LOG_ERR("failed to create a new RPL DAG\n");
    return -1;
  }*/
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
node_is_root(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
get_root_ipaddr(uip_ipaddr_t *ipaddr)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
get_sr_node_ipaddr(uip_ipaddr_t *addr, const uip_sr_node_t *node)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
leave_network(void)
{
}
/*---------------------------------------------------------------------------*/
static int
node_has_joined(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
node_is_reachable(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
global_repair(const char *str)
{
}
/*---------------------------------------------------------------------------*/
static void
local_repair(const char *str)
{
}
/*---------------------------------------------------------------------------*/
static bool
ext_header_remove(void)
{
#if NETSTACK_CONF_WITH_IPV6
  return uip_remove_ext_hdr();
#else
  return true;
#endif /* NETSTACK_CONF_WITH_IPV6 */
}
/*---------------------------------------------------------------------------*/
static int
ext_header_update(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
ext_header_hbh_update(uint8_t *ext_buf, int opt_offset)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
ext_header_srh_update(void)
{
  return 0; /* Means SRH not found */
}
/*---------------------------------------------------------------------------*/
static int
ext_header_srh_get_next_hop(uip_ipaddr_t *ipaddr)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
link_callback(const linkaddr_t *addr, int status, int numtx)
{
}
/*---------------------------------------------------------------------------*/
static void
neighbor_state_changed(uip_ds6_nbr_t *nbr)
{
}
/*---------------------------------------------------------------------------*/
static void
drop_route(uip_ds6_route_t *route)
{
}
/*---------------------------------------------------------------------------*/
static uint8_t
is_in_leaf_mode(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct routing_driver rpl_lite_driver = {
  "Iotorii routing",
  init,
  iotorii_set_prefix,
  iotorii_root_start,
  node_is_root,
  get_root_ipaddr,
  get_sr_node_ipaddr,
  leave_network,
  node_has_joined,
  node_is_reachable,
  global_repair,
  local_repair,
  ext_header_remove,
  ext_header_update,
  ext_header_hbh_update,
  ext_header_srh_update,
  ext_header_srh_get_next_hop,
  link_callback,
  neighbor_state_changed,
  drop_route,
  is_in_leaf_mode,
};
/*---------------------------------------------------------------------------*/

/** @}*/


const linkaddr_t *
rpl_nbr_gc_get_worst(const linkaddr_t *lladdr1, const linkaddr_t *lladdr2)
{
  return NULL;
}
