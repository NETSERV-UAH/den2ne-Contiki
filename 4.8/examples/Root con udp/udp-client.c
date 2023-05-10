#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdint.h>
#include <inttypes.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define IOTORII_HELLO_START_TIME		  (4 * CLOCK_SECOND)

static void iotorii_handle_hello_timer ();
static struct ctimer hello_timer;
clock_time_t hello_start_time = IOTORII_HELLO_START_TIME;
static struct simple_udp_connection udp_conn;
static uint32_t rx_count = 0;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{

  LOG_INFO("Received response '%.*s' from ", datalen, (char *) data);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");
  rx_count++;
}

static void
multicast_send(void)
{
  uint32_t id;

  id = uip_htonl(seq_id);
  memset(buf, 0, MAX_PAYLOAD_LEN);
  memcpy(buf, &id, sizeof(seq_id));

  PRINTF("Send to: ");
  PRINT6ADDR(&mcast_conn->ripaddr);
  PRINTF(" Remote Port %u,", uip_ntohs(mcast_conn->rport));
  PRINTF(" (msg=0x%08"PRIx32")", uip_ntohl(*((uint32_t *)buf)));
  PRINTF(" %lu bytes\n", (unsigned long)sizeof(id));

  seq_id++;
  uip_udp_packet_send(mcast_conn, buf, sizeof(id));
}

static void
prepare_mcast(void)
{
  uip_ipaddr_t ipaddr;

#if UIP_MCAST6_CONF_ENGINE == UIP_MCAST6_ENGINE_MPL
/*
 * MPL defines a well-known MPL domain, MPL_ALL_FORWARDERS, which
 *  MPL nodes are automatically members of. Send to that domain.
 */
  uip_ip6addr(&ipaddr, 0xFF03,0,0,0,0,0,0,0xFC);
#else
  /*
   * IPHC will use stateless multicast compression for this destination
   * (M=1, DAC=0), with 32 inline bits (1E 89 AB CD)
   */
  uip_ip6addr(&ipaddr, 0xFF1E,0,0,0,0,0,0x89,0xABCD);
#endif
  mcast_conn = udp_new(&ipaddr, UIP_HTONS(MCAST_SINK_UDP_PORT), NULL);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{

  PROCESS_BEGIN();
  ctimer_set(&hello_timer, hello_start_time, iotorii_handle_hello_timer, NULL);

    //if(NETSTACK_ROUTING.node_is_reachable() &&
        //NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {

      
    /*} else {
      LOG_INFO("Not reachable yet\n");
      if(tx_count > 0) {
        missed_tx_count++;
      }
    }*/

    /* Add some jitter */
    //etimer_set(&periodic_timer, SEND_INTERVAL
      //- CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  //}

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

static void iotorii_handle_hello_timer (){

  static char str[32];
  uip_ipaddr_t dest_ipaddr;
  static uint32_t tx_count;
  static uint32_t missed_tx_count;
  
  /* Set timer again for new transmission */
  ctimer_set(&hello_timer, hello_start_time, iotorii_handle_hello_timer, NULL);
  
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);
  
  /* Print statistics every 10th TX */
  if(tx_count % 10 == 0) {
        LOG_INFO("Tx/Rx/MissedTx: %" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
               tx_count, rx_count, missed_tx_count);
  }

  /* Send to DAG root */
  LOG_INFO("Sending request %"PRIu32" to ", tx_count);
  LOG_INFO_6ADDR(&dest_ipaddr);
  LOG_INFO_("\n");
  snprintf(str, sizeof(str), "", tx_count);
  simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
  multicast_send();
  tx_count++;
}
