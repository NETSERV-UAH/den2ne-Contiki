

//#ifndef CSMA_H_
//#define CSMA_H_

#ifndef IOTORII_CSMA_H_
#define IOTORII_CSMA_H_

#include "hlmacaddr.h"
#include "hlmac-table.h"
#include "net/linkaddr.h"

#include "contiki.h"
#include "net/mac/mac.h"
#include "dev/radio.h"

#include "net/ipv6/uip.h"

/*---------------------------------------------------------------------------*/

#ifdef IOTORII_CONF_IPV6
	#define IOTORII_IPV6 IOTORII_CONF_IPV6
#else
	#define IOTORII_IPV6 0 //To use (1) or not (0) IPv6
#endif

//#if IOTORII_IPV6 == 1 
	//#define UIP_MCAST6_CONF_ENGINE UIP_MCAST6_ENGINE_MPL
//#endif

#ifdef IOTORII_CONF_NRF52840_LOG
	#define IOTORII_NRF52840_LOG IOTORII_CONF_NRF52840_LOG
#else
	#define IOTORII_NRF52840_LOG 1 //To log data (1) or not (0)
#endif

#ifdef IOTORII_CONF_NODE_TYPE
	#define IOTORII_NODE_TYPE IOTORII_CONF_NODE_TYPE
#else
	#define IOTORII_NODE_TYPE 1 //To support the traditional MAC operation
#endif

#ifdef IOTORII_CONF_HLMAC_CAST
	#define IOTORII_HLMAC_CAST IOTORII_CONF_HLMAC_CAST
#else
	#define IOTORII_HLMAC_CAST 0 //To send HLMACS through broadcast (1) or unicast (0)
#endif

#ifdef CSMA_CONF_SEND_SOFT_ACK
	#define CSMA_SEND_SOFT_ACK CSMA_CONF_SEND_SOFT_ACK
#else /* CSMA_CONF_SEND_SOFT_ACK */
	#define CSMA_SEND_SOFT_ACK 0
#endif /* CSMA_CONF_SEND_SOFT_ACK */

#ifdef CSMA_CONF_ACK_WAIT_TIME
	#define CSMA_ACK_WAIT_TIME CSMA_CONF_ACK_WAIT_TIME
#else /* CSMA_CONF_ACK_WAIT_TIME */
	#define CSMA_ACK_WAIT_TIME                      RTIMER_SECOND / 2500
#endif /* CSMA_CONF_ACK_WAIT_TIME */

#ifdef CSMA_CONF_AFTER_ACK_DETECTED_WAIT_TIME
	#define CSMA_AFTER_ACK_DETECTED_WAIT_TIME CSMA_CONF_AFTER_ACK_DETECTED_WAIT_TIME
#else /* CSMA_CONF_AFTER_ACK_DETECTED_WAIT_TIME */
	#define CSMA_AFTER_ACK_DETECTED_WAIT_TIME       RTIMER_SECOND / 1500
#endif /* CSMA_CONF_AFTER_ACK_DETECTED_WAIT_TIME */

#define CSMA_ACK_LEN 3

/* Default MAC len for 802.15.4 classic */
#ifdef  CSMA_MAC_CONF_LEN
	#define CSMA_MAC_LEN CSMA_MAC_CONF_LEN
#else
	#define CSMA_MAC_LEN 127 - 2
#endif

/*---------------------------------------------------------------------------*/

#if IOTORII_IPV6 == 1 
	#define addr_t uip_ipaddr_t
#else
	#define addr_t linkaddr_t
#endif

struct neighbour_table_entry //ESTRUCTURA DE ENTRADA DE TABLA
{
	struct neighbour_table_entry *next;
	addr_t addr;
	uint16_t last_hello; //NÚMERO DE HELLOS ENVIADOS AL RECIBIR EL ÚLTIMO HELLO DE ESTE VECINO
	uint8_t number_id;
	int flag; //FLAG PADRE/HIJO OUTPUT/INPUT
	int load; //CARGA DE CADA NODO
	short in_out; //PUERTOS DE CARGA IN/OUT
};

typedef struct neighbour_table_entry neighbour_table_entry_t;

// struct neighbour_table_entry_ipv6 //ESTRUCTURA DE ENTRADA DE TABLA
// {
//        struct neighbour_table_entry_ipv6 *next;
//        uip_ipaddr_t addr;
// 	   uint16_t last_hello; //NÚMERO DE HELLOS ENVIADOS AL RECIBIR EL ÚLTIMO HELLO DE ESTE VECINO
//        uint8_t number_id;
//        int flag; //FLAG PADRE/HIJO OUTPUT/INPUT
//        int load; //CARGA DE CADA NODO
//        short in_out; //PUERTOS DE CARGA IN/OUT
// };

// typedef struct neighbour_table_entry_ipv6 neighbour_table_entry_t_ipv6;

/* just a default - with LLSEC, etc */
#define CSMA_MAC_MAX_HEADER 21

#if IOTORII_NODE_TYPE == 0 //SI OPERACIÓN MAC TRADICIONAL
	extern const struct mac_driver csma_driver;
#elif IOTORII_NODE_TYPE > 0 //1 PARA NODO ROOT Y 2 PARA NODOS COMUNES
	extern const struct mac_driver iotori_csma_driver;
#endif

/*---------------------------------------------------------------------------*/

//VER CSMA-SECURITY.C -->

//FUNCIONES DE SEGURIDAD CSMA DE CABECERA
int csma_security_create_frame (void);
int csma_security_parse_frame (void);

//MANTENIMIENTO DE CLAVE PARA CSMA
int csma_security_set_key (uint8_t index, const uint8_t *key);

/*---------------------------------------------------------------------------*/

//adicional
char *link_addr_to_str (const linkaddr_t addr, int length);

#endif /* IOTORII_CSMA_H_ */

/*SECUENCIA DE ACCIONES QUE SIGUE EL ALGORITMO --->

init (void)
///////////
	--> csma_output_init()	...INICIALIZA MIEMBRO
	--> on()
	--> ctimer_set(&hello_timer, hello_start_time, iotorii_handle_hello_timer, NULL);
		--> max_payload()
		--> send_packet(NULL, NULL)
	--> ctimer_set(&statistic_timer, statistic_start_time, iotorii_handle_statistic_timer, NULL);
	--> //ROOT// ctimer_set(&sethlmac_timer, sethlmac_start_time, iotorii_handle_sethlmac_timer, NULL);
		--> hlmac_create_root_addr(&root_addr, 1)
		-->	hlmactable_add(root_addr) ...AÑADE LA DIRECCIÓN HLMAC ASIGNADA A LA TABLA DEL NODO SIEMPRE QUE NO SUPERE EL MAX
		--> iotorii_send_sethlmac(root_addr, linkaddr_node_addr)
			--> max_payload()
///////////



send_packet (mac_callback_t sent, void *ptr)
///////////
	-->	init_sec()
	-->	csma_output_packet(sent, ptr)	...PAQUETE DE SALIDA
		--> neighbor_queue_from_addr(addr) 
		--> schedule_transmission(n)
///////////



input_packet (void)	...SE RECIBE PAQUETE Y SE REALIZAN COMPROBACIONES
///////////
	--> iotorii_operation()
		--> //msg HELLO// iotorii_handle_incoming_hello() ...PROCESA HELLO Y AÑADE AL VECINO A SU TABLA DE VECINOS
		--> //msg setHLMAC// iotorii_handle_incoming_sethlmac_or_load()	...PROCESA setHLMAC
			--> iotorii_extract_address() ...SE COGE LA DIRECCIÓN DEL EMISOR
			--> hlmactable_has_loop(*received_hlmac_addr) ...COMPRUEBA QUE NO HAY BUCLE
			--> hlmactable_add(*received_hlmac_addr) ...AÑADE LA DIRECCIÓN HLMAC ASIGNADA A LA TABLA DEL NODO SIEMPRE QUE NO SUPERE EL MAX
		--> //msg LOAD// iotorii_handle_incoming_hello() ...PROCESA LOAD MESSAGE
///////////


on

off

max_payload
*/





/*
1) ROOT: iotorii_handle_sethlmac_timer()
   -> iotorii_send_sethlmac()
   
   COMÚN: iotorii_handle_sethlmac_timer()
   -> ctimer

*/
