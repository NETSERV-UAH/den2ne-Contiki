

//#include "net/mac/csma/csma.h"
//#include "net/mac/csma/csma-output.h"
#include "csma-output.h"
#include "iotorii-csma.h"
#include "sys/ctimer.h"
#include "lib/list.h"
#include <stdlib.h> //For malloc()
#include "lib/random.h"
#include "hlmac-table.h"

#if LOG_DBG_STATISTIC == 1
	#include "sys/node-id.h" //For node_id
	#include "sys/rtimer.h" //For rtimer_clock_t, RTIMER_NOW()
#endif

#include "net/mac/mac-sequence.h"
#include "net/packetbuf.h"
#include "net/netstack.h"

/* Log configuration */

#include "sys/log.h"
#if IOTORII_NODE_TYPE == 0 //The traditional MAC operation
	#define LOG_MODULE "CSMA"
#elif IOTORII_NODE_TYPE > 0 // 1 => the root node, 2 => the common nodes
	#define LOG_MODULE "IoTorii-CSMA"
#endif
#define LOG_LEVEL LOG_LEVEL_MAC

/*---------------------------------------------------------------------------*/

//DELAY DESDE QUE SE INICIALIZA UN NODO HASTA QUE SE ENVÍAN LOS MENSAJES HELLO A LOS VECINOS
//RANGO = [IOTORII_CONF_HELLO_START_TIME/2 IOTORII_CONF_HELLO_START_TIME]
#ifdef IOTORII_CONF_HELLO_START_TIME
	#define IOTORII_HELLO_START_TIME IOTORII_CONF_HELLO_START_TIME
#else
	#define IOTORII_HELLO_START_TIME 15 //Default Delay is 60 s
#endif

#ifdef IOTORII_CONF_HELLO_IDLE_TIME
	#define IOTORII_HELLO_IDLE_TIME IOTORII_CONF_HELLO_IDLE_TIME
#else
	#define IOTORII_HELLO_IDLE_TIME 60 //Default Delay is 60 s
#endif

//DELAY DESDE QUE SE INICIALIZA EL NODO ROOT HASTA QUE SE ENVÍA EL PRIMER MENSAJE SETHLMAC A LOS VECINOS
//SE IMPRIMER LOS PRIMEROS LOGS DE ESTADÍSTICAS
#ifdef IOTORII_CONF_SETHLMAC_START_TIME
	#define IOTORII_SETHLMAC_START_TIME IOTORII_CONF_SETHLMAC_START_TIME
#else
	#define IOTORII_SETHLMAC_START_TIME 2 //Default Delay is 1 s
#endif

//DELAY DESDE QUE SE INICIALIZA UN NODO COMÚN HASTA QUE SE ENVÍA MENSAJE SETHLMAC A LOS VECINOS
//RANGO = [IOTORII_CONF_SETHLMAC_DELAY/2 IOTORII_CONF_SETHLMAC_DELAY]
#ifdef IOTORII_CONF_SETHLMAC_DELAY
	#define IOTORII_SETHLMAC_DELAY IOTORII_CONF_SETHLMAC_DELAY
#else
	#define IOTORII_SETHLMAC_DELAY 25 //BROADCAST
#endif

//DELAY DE PRIMERAS STADÍSTICAS
#ifdef IOTORII_CONF_STATISTICS_TIME
	#define IOTORII_STATISTICS_TIME IOTORII_CONF_STATISTICS_TIME
#else
	#define IOTORII_STATISTICS_TIME 20
#endif

//DELAY DE INFORMACIÓN DE LA CARGA QUE TIENE UN NODO A SUS VECINOS
#ifdef IOTORII_CONF_LOAD_TIME
	#define IOTORII_LOAD_TIME IOTORII_CONF_LOAD_TIME
#else
	#define IOTORII_LOAD_TIME 25
#endif

//DELAY DE TRASPASOS DE CARGAS
#ifdef IOTORII_CONF_SHARE_TIME
	#define IOTORII_SHARE_TIME IOTORII_CONF_SHARE_TIME
#else
	#define IOTORII_SHARE_TIME 25 //1 POR DEFECTO -> AUMENTAR EN CASO DE MAS DE 30 NODOS
#endif

//NÚMERO DE HELLOS HASTA QUE SE BORRE A ESE VECINO
#ifdef IOTORII_CONF_MAX_HELLO_DIFF
	#define IOTORII_MAX_HELLO_DIFF IOTORII_CONF_MAX_HELLO_DIFF
#else
	#define IOTORII_MAX_HELLO_DIFF 2
#endif

//MÁXIMO RETARDO ADICIONAL DE TRANSMISIÓN DE MENSAJES
#ifdef IOTORII_CONF_MAX_DELAY
	#define IOTORII_MAX_DELAY IOTORII_CONF_MAX_DELAY
#else
	#define IOTORII_MAX_DELAY 25 //UNICAST
#endif

/*CUANDO TERMINA EL PRIMER PASO DE ENVÍO DE CARGAS (NODOS EDGE) COMIENZA EL SHARE TIMER PARA LOS NODOS EDGE.
PARA QUE NO INTERFIERA CON EL SEGUNDO PASO DE ENVÍOS DE CARGA (NODOS NO EDGE) EL SHARE_TIME > 2*LOAD_TIME*/

/*---------------------------------------------------------------------------*/

#if IOTORII_NODE_TYPE == 1 //ROOT
	static struct ctimer sethlmac_timer;
#endif

#if IOTORII_NODE_TYPE > 0 //ROOT O NODO COMÚN
	static struct ctimer hello_timer;
	static struct ctimer send_sethlmac_timer;
	// static struct ctimer load_timer;
	// static struct ctimer share_timer;
	static struct ctimer statistic_timer;
	uint16_t number_of_hello_messages = 0;

	#if LOG_DBG_STATISTIC == 1
		uint16_t number_of_sethlmac_messages = 0;
		uint16_t number_times_converged = 0;
	#endif

#endif

#if IOTORII_NRF52840_LOG == 1
	static struct ctimer log_timer;
#endif

#if IOTORII_NODE_TYPE > 0 //ROOT O NODO COMÚN
	LIST(neighbour_table_entry_list); //SE CREA LA TABLA DEL VECINO

	struct payload_entry //ESTRUCTURA DE ENTRADA DE TABLA PARA DIRECCIONES DE RECEPTORES O PAYLOAD
	{
		struct payload_entry *next;
		uint8_t *payload;
		int data_len;
		#if IOTORII_HLMAC_CAST == 0
			addr_t dest;
		#endif
	};

	typedef struct payload_entry payload_entry_t;
	LIST(payload_entry_list);

	/*---------------------------------------------------------------------------*/

	struct this_node //ESTRUCTURA DE ENTRADA DE TABLA DE PRIORIDADES Y DE RELACIONES PADRE-HIJO
	{
		struct this_node *next;
		char* str_addr;
		char* str_top_addr; //ADDR PADRE
		uint8_t payload;
		uint8_t nhops;
		int load; //CARGA DE CADA NODO -> 16 BITS PORQUE CON 8 BITS A PARTIR DE 255 DE CARGA SE VUELVE 0
	};

	typedef struct this_node this_node_t;
	LIST(node_list); //aux

	void list_this_node_entry (payload_entry_t *a, hlmacaddr_t *addr); //GUARDA INFO EN this_node

	uint8_t n_hijos_share = 0; //CUENTA DEL NÚMERO DE NODOS HIJOS PARA EL ENVÍO DEL MENSAJE SHARE
	uint8_t n_hijos_load = 0; //CUENTA DEL NÚMERO DE NODOS HIJOS PARA EL ENVÍO DEL MENSAJE LOAD
	uint8_t count_hijos = 0; //CONTROL PARA QUE HAYA UNA ÚNICA CUENTA DE LOS HIJOS

	uint8_t edge = 0; //INDICA QUE EL NODO ES EDGE SI ES 1
	uint8_t sent_load = 0;
	uint8_t sent_share = 0;

	uint32_t has_converged = 0;

	short extra_load = 0; //INDICA CUANTA CARGA SOBRA SI UN NODO TIENE UNA CANTIDAD MAYOR A 100 
#endif


#if IOTORII_NRF52840_LOG == 1 //LOG EN NRF52840-DK
	uint8_t erase = 0x2, write=0x1, read=0x0;
	//int log = 0xF5000;
	int log = 0xA0000;
	int nvmc = 0x4001E000;
	int mem_counter = 0;
	//int page_mem_counter = 0xF4000;
	int page_mem_counter = log - 0x1000;
#endif

// #if IOTORII_HLMAC_CAST == 0
// #endif

/*---------------------------------------------------------------------------*/

#if IOTORII_NODE_TYPE == 1 //ROOT
	static void iotorii_handle_sethlmac_timer ();
#endif

void iotorii_handle_send_message_timer ();
static void iotorii_handle_statistic_timer ();

clock_time_t hello_start_time;
clock_time_t hello_idle_time;
int number_of_neighbours;
int number_of_neighbours_flag; //PARA COMPROBAR SI EL NODO ES EDGE
uint32_t last_timestamp = 0;

clock_time_t convergence_time;

static void init_sec (void)
{
	#if LLSEC802154_USES_AUX_HEADER
		if (packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) == PACKETBUF_ATTR_SECURITY_LEVEL_DEFAULT) 
			packetbuf_set_attr(PACKETBUF_ATTR_SECURITY_LEVEL, CSMA_LLSEC_SECURITY_LEVEL);
	#endif
}

static void send_packet (mac_callback_t sent, void *ptr) //ENVÍA PAQUETE
{
	init_sec();
	csma_output_packet(sent, ptr);
}


static int on (void)
{
    return NETSTACK_RADIO.on();
}


static int off (void)
{
    return NETSTACK_RADIO.off();
}


static int max_payload (void)
{
	int framer_hdrlen;

	init_sec();

	framer_hdrlen = NETSTACK_FRAMER.length();

	if (framer_hdrlen < 0) //FRAMING HA FALLADO Y SE ASUME LA LONGITUD MAX DE CABECERA
		framer_hdrlen = CSMA_MAC_MAX_HEADER;

	return CSMA_MAC_LEN - framer_hdrlen;
}

/*---------------------------------------------------------------------------*/

#if IOTORII_IPV6 == 1 //IPv6
	#include "net/netstack.h"
	#include "net/routing/routing.h"
	#include "net/ipv6/simple-udp.h"

	#include "net/ipv6/uip-ds6.h"
	#include "net/ipv6/uiplib.h"
	#include "net/ipv6/uip.h"

	void iotorii_handle_incoming_hello (addr_t *sender_addr); //PROCESA UN PAQUETE HELLO (DE DIFUSIÓN) RECIBIDO DE OTROS NODOS
	void iotorii_handle_incoming_sethlmac_or_load (addr_t *sender_address, uint8_t *packetbuf_ptr_head, uint16_t packetbuf_data_len); //PROCESA UN MENSAJE DE DIFUSIÓN SETHLMAC RECIBIDO DE OTROS NODOS

	bool send_correct;

	//MAX PAYLOAD DE LOS MENSAJES SETHLMAC IPv6
	#ifdef IPv6_MAX_PAYLOAD_CONF
		#define IPv6_MAX_PAYLOAD IPv6_MAX_PAYLOAD_CONF
	#else
		#define IPv6_MAX_PAYLOAD 128 //TODO: CAMBIAR EN UN FUTURO
	#endif

	#define UDP_PORT_LISTEN	8765
	#define UDP_PORT_SEND	5678
	static struct simple_udp_connection udp_conn;
	addr_t broad_ipaddr;
	static void
	udp_rx_callback(struct simple_udp_connection *c,
			const addr_t *sender_addr,
			uint16_t sender_port,
			const addr_t *receiver_addr,
			uint16_t receiver_port,
			const uint8_t *data,
			uint16_t datalen){

		addr_t *aux_addr = (addr_t*) malloc (sizeof(addr_t));
		memcpy(aux_addr, sender_addr, sizeof(addr_t)); //SE COPIA LOAD
		uint8_t *aux_data = (uint8_t*) malloc (sizeof(uint8_t) * datalen);
		memcpy(aux_data, data, datalen); //SE COPIA LOAD
		// printf("Mensaje de longitud %d\n\r", datalen);
		
		if(datalen==1)
			iotorii_handle_incoming_hello(aux_addr);
		else
			iotorii_handle_incoming_sethlmac_or_load(aux_addr, aux_data, datalen);

		free(aux_addr);
		free(aux_data);
	}
#endif

/*---------------------------------------------------------------------------*/

#if IOTORII_NRF52840_LOG == 1
	void read_log(){
		int mem_used, code;
		int aux_log, len_log;
		addr_t addr_log;

		printf("\n\r\t{\n\r" \
			"\t\t\"MAC\"      :\t\"");
		for(int i = 0; i < ADDR_SIZE; i++) {
			if(i > 0 && i % 2 == 0) {
				printf(".");
			}
			printf("%02x", linkaddr_node_addr.u8[i]);
		}
		printf("\",");
		#if IOTORII_NODE_TYPE == 1 //ROOT
			printf("\n\r\t\t\"Type\"     :\t\"Root\",");
		#elif IOTORII_NODE_TYPE == 2 //COMMON
			printf("\n\r\t\t\"Type\"     :\t\"Common\",");
		#endif
		printf("\n\r\t\t\"Message\"  :\t[");

		memcpy(&mem_used, page_mem_counter+0xFFC, sizeof(int));
		for(int counter = 0; counter < mem_used; counter=counter+sizeof(code)){
			printf("\n\r\t\t{");
			memcpy(&code, log+counter, 4);
			printf("\n\r\t\t\t\"Origin\"  :\t\"");
			switch (code)
			{
			case 0xFFFF1111: //HELLO ENVIADO
				for(int i = 0; i < ADDR_SIZE; i++) {
					if(i > 0 && i % 2 == 0) {
						printf(".");
					}
					printf("%02x", linkaddr_node_addr.u8[i]);
				}
				printf("\",");
				printf("\n\r\t\t\t\"Type\"    :\t\"Hello\"");
				memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log)); //AUX_LOG => NUMERO DE HELLO ENVIADO
				counter=counter+sizeof(aux_log);
				break;

			case 0xFFFF2222: //HELLO RECIBIDO
				memcpy(&addr_log, log+counter+sizeof(code), ADDR_SIZE);
				counter=counter+ADDR_SIZE;
				for(int i = 0; i < ADDR_SIZE; i++) {
					if(i > 0 && i % 2 == 0) {
						printf(".");
					}
					printf("%02x", addr_log.u8[i]);
				}
				printf("\",");
				printf("\n\r\t\t\t\"Type\"    :\t\"Hello\"");
				break;

			case 0xFFFF3333: //HLMAC ENVIADA
				for(int i = 0; i < ADDR_SIZE; i++) {
					if(i > 0 && i % 2 == 0) {
						printf(".");
					}
					printf("%02x", linkaddr_node_addr.u8[i]);
				}
				printf("\",");
				printf("\n\r\t\t\t\"Type\"    :\t\"SetHLMAC\",");
				printf("\n\r\t\t\t\"Content\" :\n\r\t\t\t{");
				memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log)); //AUX_LOG => TIMESTAMP
				counter=counter+sizeof(aux_log);
				printf("\n\r\t\t\t\t\"Timestamp\"  :\t\"%d\",", aux_log);
				memcpy(&len_log, log+counter+sizeof(code), sizeof(len_log)); //LEN_LOG => LONGITUD DIRECCIÓN PROPIA
				counter=counter+sizeof(len_log);
				memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log)); //AUX_LOG => INICIO DE DIRECCIÓN PROPIA
				counter=counter+sizeof(aux_log);
				printf("\n\r\t\t\t\t\"Prefix\"     :\t\"%2.2X", aux_log);
				for(int k=1; k<len_log; k++){
					memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log)); //AUX_LOG => CADA BYTE DE DIRECCIÓN PROPIA
					counter=counter+sizeof(aux_log);
					printf(".%2.2X", aux_log);
				}
				printf("\",");
				printf("\n\r\t\t\t\t\"HLMAC\"      :");
				memcpy(&len_log, log+counter+sizeof(code), sizeof(len_log)); //LEN_LOG => NÚMERO DE HLMACS ASIGNADAS
				counter=counter+sizeof(len_log);
				printf("\n\r\t\t\t\t\t{");
				for(int k=0; k<len_log-1; k++){
					memcpy(&addr_log, log+counter+sizeof(code), ADDR_SIZE);
					counter=counter+ADDR_SIZE;
					printf("\n\r\t\t\t\t\t\t\"");
					for(int i = 0; i < ADDR_SIZE; i++) {
						if(i > 0 && i % 2 == 0) {
							printf(".");
						}
						printf("%02x", addr_log.u8[i]);
					}
					memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log)); //AUX_LOG => ID DE CADA HLMAC ASIGNADA
					printf("\" :\t\"%2.2X\"", aux_log);
					counter=counter+sizeof(aux_log);
					if (k+1<len_log-1)
						printf(",");
				}
				printf("\n\r\t\t\t\t\t}");
				printf("\n\r\t\t\t}");
				break;

			case 0xFFFF4444: //HLMAC RECIBIDA
				memcpy(&addr_log, log+counter+sizeof(code), ADDR_SIZE);
				counter=counter+ADDR_SIZE;
				for(int i = 0; i < ADDR_SIZE; i++) {
					if(i > 0 && i % 2 == 0) {
						printf(".");
					}
					printf("%02x", addr_log.u8[i]);
				}
				printf("\",");
				printf("\n\r\t\t\t\"Type\"    :\t\"SetHLMAC\",");
				printf("\n\r\t\t\t\"Content\" :\n\r\t\t\t{");
				memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log)); //AUX_LOG => TIMESTAMP
				counter=counter+sizeof(aux_log);
				printf("\n\r\t\t\t\t\"Timestamp\"  :\t\"%d\",", aux_log);
				memcpy(&len_log, log+counter+sizeof(code), sizeof(len_log)); //LEN_LOG => LONGITUD DIRECCIÓN PROPIA
				counter=counter+sizeof(len_log);
				memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log)); //AUX_LOG => INICIO DE DIRECCIÓN PROPIA
				counter=counter+sizeof(aux_log);
				printf("\n\r\t\t\t\t\"Prefix\"     :\t\"%2.2X", aux_log);
				for(int k=1; k<len_log-1; k++){
					memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log)); //AUX_LOG => CADA BYTE DE DIRECCIÓN PROPIA
					counter=counter+sizeof(aux_log);
					printf(".%2.2X", aux_log);
				}
				printf("\",");
				printf("\n\r\t\t\t\t\"HLMAC\"      :");
				printf("\n\r\t\t\t\t\t{");
				printf("\n\r\t\t\t\t\t\t\"");
				for(int i = 0; i < ADDR_SIZE; i++) {
					if(i > 0 && i % 2 == 0) {
						printf(".");
					}
					printf("%02x", linkaddr_node_addr.u8[i]);
				}
				memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log)); //AUX_LOG => CADA BYTE DE DIRECCIÓN PROPIA
				counter=counter+sizeof(aux_log);
				printf("\" :\t\"%2.2X\"", aux_log);
				printf("\n\r\t\t\t\t\t}");
				printf("\n\r\t\t\t}");
				break;

			case 0xFFFF5555:  //LOAD ENVIADO
				for(int i = 0; i < ADDR_SIZE; i++) {
					if(i > 0 && i % 2 == 0) {
						printf(".");
					}
					printf("%02x", linkaddr_node_addr.u8[i]);
				}
				printf("\",");
				printf("\n\r\t\t\t\"Type\"    :\t\"Load\",");
				memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log));
				counter=counter+sizeof(aux_log);
				printf("\n\r\t\t\t\"Value\"   :\t\"%d\"", aux_log);
				break;

			case 0xFFFF6666: //LOAD RECIBIDO
				memcpy(&addr_log, log+counter+sizeof(code), ADDR_SIZE);
				counter=counter+ADDR_SIZE;
				for(int i = 0; i < ADDR_SIZE; i++) {
					if(i > 0 && i % 2 == 0) {
						printf(".");
					}
					printf("%02x", addr_log.u8[i]);
				}
				printf("\",");
				printf("\n\r\t\t\t\"Type\"    :\t\"Load\",");
				memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log)); //AUX_LOG => VALOR DE LOAD RECIBIDO
				counter=counter+sizeof(aux_log);
				printf("\n\r\t\t\t\"Value\"   :\t\"%d\"", aux_log);
				break;

			case 0xFFFF7777:  //SHARE ENVIADO
				for(int i = 0; i < ADDR_SIZE; i++) {
					if(i > 0 && i % 2 == 0) {
						printf(".");
					}
					printf("%02x", linkaddr_node_addr.u8[i]);
				}
				printf("\",");
				printf("\n\r\t\t\t\"Type\"    :\t\"Share\",");
				memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log));
				counter=counter+sizeof(aux_log);
				printf("\n\r\t\t\t\"Value\"   :\t\"%d\"", aux_log);
				break;

			case 0xFFFF8888: //SHARE RECIBIDO
				memcpy(&addr_log, log+counter+sizeof(code), ADDR_SIZE);
				counter=counter+ADDR_SIZE;
				for(int i = 0; i < ADDR_SIZE; i++) {
					if(i > 0 && i % 2 == 0) {
						printf(".");
					}
					printf("%02x", addr_log.u8[i]);
				}
				printf("\",");
				printf("\n\r\t\t\t\"Type\"    :\t\"Share\",");
				memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log)); //AUX_LOG => VALOR DE LOAD RECIBIDO
				counter=counter+sizeof(aux_log);
				printf("\n\r\t\t\t\"Value\"   :\t\"%d\"", aux_log);
				break;

			case 0xFFFF9999: //ES EDGE
				for(int i = 0; i < ADDR_SIZE; i++) {
					if(i > 0 && i % 2 == 0) {
						printf(".");
					}
					printf("%02x", linkaddr_node_addr.u8[i]);
				}
				printf("\",");
				printf("\n\r\t\t\t\"Type\"    :\t\"INFO\",");
				printf("\n\r\t\t\t\"Content\" :\t\"Edge\"");
				break;

			case 0xFFFFAAAA: //HLMAC AÑADIDA
				for(int i = 0; i < ADDR_SIZE; i++) {
					if(i > 0 && i % 2 == 0) {
						printf(".");
					}
					printf("%02x", linkaddr_node_addr.u8[i]);
				}
				printf("\",");
				printf("\n\r\t\t\t\"Type\"    :\t\"INFO\",");
				printf("\n\r\t\t\t\"Content\" :\t\"HLMAC added\",");
				memcpy(&len_log, log+counter+sizeof(code), sizeof(len_log)); //LEN_LOG => LONGITUD DIRECCIÓN PROPIA
				counter=counter+sizeof(len_log);
				memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log)); //AUX_LOG => INICIO DE DIRECCIÓN PROPIA
				counter=counter+sizeof(aux_log);
				printf("\n\r\t\t\t\"Value\"   :\t\"%2.2X", aux_log);
				for(int k=1; k<len_log; k++){
					memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log)); //AUX_LOG => CADA BYTE DE DIRECCIÓN PROPIA
					counter=counter+sizeof(aux_log);
					printf(".%2.2X", aux_log);
				}
				printf("\",");
				memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log)); //AUX_LOG => CÓDIGO TIMESTAMP DE LA ASIGNACIÓN
				counter=counter+sizeof(aux_log);
				printf("\n\r\t\t\t\"Timestamp\"   :\t\"%d\"", aux_log);
				break;

			case 0xFFFFBBBB:  //TIEMPO DE CONVERGENCIA
				for(int i = 0; i < ADDR_SIZE; i++) {
					if(i > 0 && i % 2 == 0) {
						printf(".");
					}
					printf("%02x", linkaddr_node_addr.u8[i]);
				}
				printf("\",");
				printf("\n\r\t\t\t\"Type\"    :\t\"INFO\",");
				printf("\n\r\t\t\t\"Content\" :\t\"Convergence\",");
				memcpy(&aux_log, log+counter+sizeof(code), sizeof(aux_log));
				counter=counter+sizeof(aux_log);
				printf("\n\r\t\t\t\"Value\"   :\t\"%d\"", aux_log);
				break;
			
			default:
				break;
			}
			if (counter+sizeof(code) >= mem_used)
				printf("\n\r\t\t}");
			else
				printf("\n\r\t\t},");
		}
		printf("\n\r\t\t]");
		printf("\n\r\t}\n\r");

		for (int c = 0; c <= (int) (mem_used/0x1000); c++){
			int erase_page = log + c * 0x1000;
			memcpy(nvmc+0x504, &erase, 4);
			memcpy(nvmc+0x508, &erase_page, 4); //ES NECESARIO BORRA LA PÁGINA DEL CONTADOR PARA VOLVER A ESCRIBIR
			memcpy(nvmc+0x504, &read, 4);
		}


		memcpy(nvmc+0x504, &erase, 4);
		memcpy(nvmc+0x508, &page_mem_counter, 4); //ES NECESARIO BORRA LA PÁGINA DEL CONTADOR PARA VOLVER A ESCRIBIR
		memcpy(nvmc+0x504, &read, 4);
	}

	bool check_write(){
		int ready, ready_next;
		memcpy(&ready, nvmc+0x400, 4);
		memcpy(&ready_next, nvmc+0x408, 4);
		return ready && ready_next;
	}
	
	void update_mem_counter(){
		memcpy(nvmc+0x504, &erase, 4);
		memcpy(nvmc+0x508, &page_mem_counter, 4); //ES NECESARIO BORRA LA PÁGINA DEL CONTADOR PARA VOLVER A ESCRIBIR
		memcpy(nvmc+0x504, &write, 4);
		memcpy(page_mem_counter+0xFFC, &mem_counter, 4); //DIRECCIÓN DÓNDE SE GUARDA EL TAMAÑO DEL LOG EN BYTES
		memcpy(nvmc+0x504, &read, 4);
	}

	void write_log(void *element){
		memcpy(log+mem_counter, &element, sizeof(element));
		mem_counter += sizeof(element);
	}

#endif


#if IOTORII_IPV6 == 1
	static void prepare_packet (void *data_ptr, size_t datalen, addr_t *addr){
		udp_conn.udp_conn->lport=UIP_HTONS(UDP_PORT_SEND);
		send_correct = (bool) simple_udp_sendto(&udp_conn, data_ptr, datalen, addr);
	}

#else
	static void prepare_packet (void *data_ptr, size_t datalen, addr_t *addr){
		packetbuf_clear(); //SE PREPARA EL BUFFER DE PAQUETES Y SE RESETEA
		memcpy(packetbuf_dataptr(), data_ptr, datalen); //SE COPIA LOAD  
		packetbuf_set_datalen(datalen);
		packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, addr);
		send_packet(NULL, NULL);
	}
#endif

extern clock_time_t transmission_delay;

/*---------------------------------------------------------------------------*/

#if IOTORII_NODE_TYPE > 0 //ROOT O NODO COMÚN

#if IOTORII_IPV6 == 1 
hlmacaddr_t *iotorii_extract_address (addr_t *sender_addr, uint8_t *packetbuf_ptr_head, uint16_t packetbuf_data_len) //SE EXTRAE LA DIRECCIÓN DEL NODO EMISOR A PARTIR DEL PAQUETE RECIBIDO
{
	addr_t node_addr;	//DIRECCIÓN IP DEL PROCESO NATIVO
	// uip_ds6_select_src(&node_addr, &broad_ipaddr);
	uip_ds6_select_src(&node_addr, sender_addr);

	addr_t link_address;
	uip_ip6addr(&link_address, 0,0,0,0,0,0,0,0);
#else
hlmacaddr_t *iotorii_extract_address (void) //SE EXTRAE LA DIRECCIÓN DEL NODO EMISOR A PARTIR DEL PAQUETE RECIBIDO
{
	int packetbuf_data_len = packetbuf_datalen();
	uint8_t *packetbuf_ptr_head = (uint8_t*) malloc (sizeof(uint8_t) * packetbuf_data_len);
	memcpy(packetbuf_ptr_head, packetbuf_dataptr(), packetbuf_data_len); //COPIA DEL BUFFER
	
	addr_t link_address;
	link_address = linkaddr_null;

	addr_t node_addr = linkaddr_node_addr;
#endif
	
	uint8_t *packetbuf_ptr = packetbuf_ptr_head; //PUNTERO A COPIA DEL BUFFER

	int datalen_counter = 0;
	uint8_t id = 0;
	hlmacaddr_t *prefix = NULL;

	//SE LEE EL PAYLOAD
	
	/* The payload structure:
	* +-----------+------------+--------+-----+------+-----+-----+------+
	* | Timestamp | Prefix len | Prefix | ID1 | MAC1 | ... | IDn | MACn |
	* +-----------+------------+--------+-----+------+-----+-----+------+ -------->*/
	
	packetbuf_ptr+=sizeof(uint32_t); //SE IGNORA EL CAMPO TIMESTAMP
	datalen_counter+=sizeof(uint32_t); //SE IGNORA EL CAMPO TIMESTAMP
	
	prefix = (hlmacaddr_t*) malloc (sizeof(hlmacaddr_t)); //SE RESERVA MEMORIA PARA EL PREFIJO
	memcpy(&prefix->len, packetbuf_ptr, 1); //COPIA LONGITUD PREFIJO DEL VECINO
	
	packetbuf_ptr++;
	datalen_counter++;

	prefix->address = (uint8_t*) malloc (sizeof(uint8_t) * prefix->len); //COPIA PREFIJO DEL VECINO
	memcpy(prefix->address, packetbuf_ptr, prefix->len);
	
	packetbuf_ptr += prefix->len;
	datalen_counter += prefix->len;

	// #if IOTORII_HLMAC_CAST == 1
		while (datalen_counter < packetbuf_data_len && !addr_cmp(&link_address, &node_addr))
		{
			memcpy(&id, packetbuf_ptr, 1); //COPIA ID DEL VECINO
			
			packetbuf_ptr++;
			datalen_counter++;

			memcpy(&link_address, packetbuf_ptr, ADDR_SIZE); //COPIA DIRECCIÓN MAC DEL VECINO
			
			packetbuf_ptr += ADDR_SIZE;
			datalen_counter += ADDR_SIZE;
		}

		#if IOTORII_IPV6 == 0
			free(packetbuf_ptr_head); //SE LIBERA MEMORIA
			packetbuf_ptr_head = NULL;
		#endif
		
		if (addr_cmp(&link_address, &node_addr)){

			hlmac_add_new_id(prefix, id); //SE AÑADE ID A LA DIRECCIÓN DEL PREFIJO
			return prefix;
		}
	// #else
	// 	if (packetbuf_data_len == sizeof(uint32_t) + (sizeof(uint8_t)*(1+prefix->len+1))){ //SI EL TAMAÑO DEL MENSAJES COINCIDE CON TIMESTAMP, LOGITUD DE PREFIJO, PREFIJO E ID
	// 		memcpy(&id, packetbuf_ptr, 1); //COPIA ID DEL VECINO

	// 		#if IOTORII_IPV6 == 0
	// 			free(packetbuf_ptr_head); //SE LIBERA MEMORIA
	// 			packetbuf_ptr_head = NULL;
	// 		#endif

	// 		hlmac_add_new_id(prefix, id); //SE AÑADE ID A LA DIRECCIÓN DEL PREFIJO
	// 		return prefix;
	// 	}
	// 	#if IOTORII_IPV6 == 0
	// 		free(packetbuf_ptr_head); //SE LIBERA MEMORIA
	// 		packetbuf_ptr_head = NULL;
	// 	#endif
	// #endif
	
	
	*prefix = UNSPECIFIED_HLMAC_ADDRESS; //SE "BORRA"
	return prefix;
}

#if IOTORII_IPV6 == 1 
uint32_t iotorii_extract_timestamp (uint8_t *packetbuf_ptr_head) //SE EXTRAE EL TIMESTAMP A PARTIR DEL PAQUETE RECIBIDO
{
#else
uint32_t iotorii_extract_timestamp (void) //SE EXTRAE EL TIMESTAMP A PARTIR DEL PAQUETE RECIBIDO
{
	int packetbuf_data_len = packetbuf_datalen();
	uint8_t *packetbuf_ptr_head = (uint8_t*) malloc (sizeof(uint8_t) * packetbuf_data_len);
	memcpy(packetbuf_ptr_head, packetbuf_dataptr(), packetbuf_data_len); //COPIA DEL BUFFER
#endif

    uint32_t timestamp;
	
	uint8_t *packetbuf_ptr = packetbuf_ptr_head; //PUNTERO A COPIA DEL BUFFER
	
	//SE LEE EL PAYLOAD
	memcpy(&timestamp, packetbuf_ptr, sizeof(uint32_t)); //COPIA EL TIMESTAMP

	/* The payload structure:
	* +-----------+------------+--------+-----+------+-----+-----+------+
	* | Timestamp | Prefix len | Prefix | ID1 | MAC1 | ... | IDn | MACn |
	* +-----------+------------+--------+-----+------+-----+-----+------+ -------->*/

#if IOTORII_IPV6 == 0
	free(packetbuf_ptr_head);
#endif	

	return timestamp;
}


void iotorii_handle_load_timer ()
{	
	this_node_t *node;
	node = list_head(node_list); 
	
	if (list_head(node_list)) //EXISTE 
	{
		
		#if IOTORII_NRF52840_LOG == 1
			if(check_write()){
				memcpy(nvmc+0x504, &write, 4);
				write_log(0xFFFF5555);
				write_log(node->load);
				memcpy(nvmc+0x504, &read, 4);
			}
			update_mem_counter();
		#endif

		
		#if IOTORII_HLMAC_CAST == 1
			// packetbuf_clear(); //SE PREPARA EL BUFFER DE PAQUETES Y SE RESETEA 
			
			// memcpy(packetbuf_dataptr(), &(node->load), sizeof(node->load)); //SE COPIA LOAD  
			// packetbuf_set_datalen(sizeof(node->load));
			// packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_null);

			neighbour_table_entry_t *nb;
			nb = list_head(neighbour_table_entry_list);
			for (nb = list_head(neighbour_table_entry_list); nb != NULL; nb = list_item_next(nb))
			{
				if (nb->flag == 1)
				{
					// printf("//INFO HANDLE LOAD//");
					// for(int i = 0; i < ADDR_SIZE; i++) {
					// 	if(i > 0 && i % 2 == 0) {
					// 		printf(".");
					// 	}
					// 	printf("%02x", nb->addr.u8[i]);
					// }
					// printf("\n\r");
					prepare_packet(&(node->load), sizeof(node->load), &(nb->addr));
					// packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &(nb->addr));
				}	
			}

			// send_packet(NULL, NULL, NULL);

		#else
			payload_entry_t *payload_entry = (payload_entry_t*) malloc (sizeof(payload_entry_t));
			payload_entry->next = NULL;
			payload_entry->payload = (uint8_t*) malloc (sizeof(int));
			payload_entry->data_len = sizeof(int);
			memcpy(payload_entry->payload, &(node->load), sizeof(node->load)); //SE COPIA LOAD  

			neighbour_table_entry_t *nb;
			nb = list_head(neighbour_table_entry_list);
			for (nb = list_head(neighbour_table_entry_list); nb != NULL; nb = list_item_next(nb))
			{
				if (nb->flag == 1)
				{
					// printf("//INFO HANDLE LOAD//");
					// for(int i = 0; i < ADDR_SIZE; i++) {
					// 	if(i > 0 && i % 2 == 0) {
					// 		printf(".");
					// 	}
					// 	printf("%02x", nb->addr.u8[i]);
					// }
					// printf("\n\r");
					payload_entry->dest = nb->addr;
					if(list_head(payload_entry_list) == NULL)
						ctimer_set(&send_sethlmac_timer, 10, iotorii_handle_send_message_timer, NULL);
					list_add(payload_entry_list, payload_entry);
				}	
			}

			// if(ctimer_expired(&send_sethlmac_timer))
			// 	ctimer_set(&send_sethlmac_timer, max_transmission_delay(), iotorii_handle_send_message_timer, NULL);
				// ctimer_restart(&send_sethlmac_timer);
				// ctimer_set(&send_sethlmac_timer, 10, iotorii_handle_send_message_timer, NULL);

		#endif
		#if LOG_DBG_STATISTIC == 1		
			// printf("//INFO HANDLE LOAD// carga enviada: %d de direccion %s\r\n", node->load, node->str_addr); 
		#endif
	}
}


void iotorii_handle_share_upstream_timer ()
{
	this_node_t *node;
	node = list_head(node_list); 
	
	neighbour_table_entry_t *nb;
	nb = list_head(neighbour_table_entry_list);
	
	
	//FLUJOS DE TRÁFICO UPWARD: DESDE LOS EDGE HACIA EL DODAG (NODO ROOT)
	//TODOS LOS NODOS SE QUEDAN CON UNA CARGA DE 100 
	if (node->load > 100)
	{
		extra_load = node->load - 100; //SE QUITA LA CARGA SOBRANTE Y SE ALMACENA 			
		// printf("//INFO HANDLE SHARE UP// En este nodo sobra una carga de %d\r\n", extra_load);		
	}
	else //node->load <= 100
	{
		if (node->load > 0)
			extra_load = node->load - 100; //SE QUITA LA CARGA SOBRANTE Y SE ALMACENA 
		else
		{
			extra_load = -node->load + 100;
			extra_load = -extra_load;
		}
		
		// printf("//INFO HANDLE SHARE UP// En este nodo permite una carga adicional de valor %d\r\n", -extra_load);
	}
	
	#if IOTORII_NODE_TYPE == 2
		node->load = 100; //SE ASIGNA COMO MÁXIMO UNA CANTIDAD DE 100 PARA NODOS COMUNES
	#endif
	
	
	#if IOTORII_HLMAC_CAST == 1
		// packetbuf_clear(); //SE PREPARA EL BUFFER DE PAQUETES Y SE RESETEA 
		// printf("//INFO HANDLE SHARE UP// Carga actualizada: %d\r\n", node->load);
		// printf("//INFO HANDLE SHARE UP// Carga informada: %d\r\n", extra_load);
		// memcpy(packetbuf_dataptr(), &(extra_load), sizeof(extra_load)); //SE COPIA LOAD  
		packetbuf_set_datalen(sizeof(extra_load));

		for (nb = list_head(neighbour_table_entry_list); nb != NULL; nb = list_item_next(nb))
		{
			if (nb->flag == 1)
			{
				nb->in_out = -extra_load;
										
				// packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_null);
				// packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &(nb->addr));
				prepare_packet(&(extra_load), sizeof(extra_load), &(nb->addr));
			}	
		}
		// send_packet(NULL, NULL, NULL);

	#else
		payload_entry_t *payload_entry = (payload_entry_t*) malloc (sizeof(payload_entry_t));
		payload_entry->next = NULL;

		for (nb = list_head(neighbour_table_entry_list); nb != NULL; nb = list_item_next(nb))
		{
			if (nb->flag == 1)
			{
				nb->in_out = -extra_load;
				#if IOTORII_HLMAC_CAST == 0
					payload_entry->dest = nb->addr;
				#endif
			}	
		}

		payload_entry->payload = (uint8_t*) malloc (sizeof(short));
		memcpy(payload_entry->payload, &extra_load, sizeof(short)); //SE COPIA SHARE
		payload_entry->data_len = sizeof(short);
		if(list_head(payload_entry_list) == NULL)
			ctimer_set(&send_sethlmac_timer, 10, iotorii_handle_send_message_timer, NULL);
		list_add(payload_entry_list, payload_entry);

		// if(ctimer_expired(&send_sethlmac_timer))
		// 	ctimer_set(&send_sethlmac_timer, max_transmission_delay(), iotorii_handle_send_message_timer, NULL);
			// ctimer_restart(&send_sethlmac_timer);
			// ctimer_set(&send_sethlmac_timer, 10, iotorii_handle_send_message_timer, NULL);
	#endif

	#if IOTORII_NRF52840_LOG == 1
		if(check_write()){
			memcpy(nvmc+0x504, &write, 4);
			write_log(0xFFFF7777);
			write_log(extra_load);
			memcpy(nvmc+0x504, &read, 4);
		}
		update_mem_counter();
	#endif
}


#if LOG_DBG_STATISTIC == 1

static void iotorii_handle_statistic_timer ()
{
	uint8_t load_null = 0;
	
	this_node_t *node;
	node = list_head(node_list); 
	neighbour_table_entry_t *nb;
	
	if(node!=NULL){
		// printf("Periodic Statistics: node_id: %u, n_hello: %d, n_sethlmac: %d, n_neighbours: %d\r\n", node_id, number_of_hello_messages, number_of_sethlmac_messages, number_of_neighbours);
		// printf("//INFO STATISTICS// n_hello: %d, n_sethlmac: %d\r\n", number_of_hello_messages, number_of_sethlmac_messages);
		printf("//INFO STATISTICS// Node %s has %d neighbours and has not received HLMAC from %d neighbours\n\r", node->str_addr, number_of_neighbours, number_of_neighbours_flag);
		
		#if IOTORII_NODE_TYPE > 1
			if (!number_of_neighbours_flag)
			{
				// printf("es edge\r\n");
				edge = 1;

				#if IOTORII_NRF52840_LOG == 1
					if(check_write()){
						memcpy(nvmc+0x504, &write, 4);
						write_log(0xFFFF9999);
						memcpy(nvmc+0x504, &read, 4);
					}
					update_mem_counter();
				#endif

				//ctimer_set(&load_timer, (random_rand() % IOTORII_LOAD_TIME) * CLOCK_SECOND, iotorii_handle_load_timer, NULL);
				
				// #if IOTORII_NODE_TYPE > 1
				// 	iotorii_handle_load_timer();
				// #endif
			}
			else
			{
				edge = 0;
				// printf("no es edge\r\n");
			}
		#endif
		
		printf("//INFO LOAD// Node's load: %d\r\n", node->load);

		n_hijos_share=0;
		n_hijos_load=0;
		for (nb = list_head(neighbour_table_entry_list); nb != NULL; nb = list_item_next(nb)) //LISTA DE VECINOS DEL NODO
		{		
			if (nb->flag == 1 || nb->flag == -1)
				nb->flag = nb->flag;
				// printf("--> Vecino padre (flag, carga, in_out) --> ");
			else
			{
				// printf("--> Vecino hijo  (flag, carga, in_out) --> ");	
				// if (count_hijos == 0){
					n_hijos_share++;
					n_hijos_load++;
				// }
			}

			// if (nb->flag == -1)
			// 	printf("%d, %d, (%d)\r\n", nb->flag, nb->load, nb->in_out);		
			// else
			// 	printf("%d, %d, %d\r\n", nb->flag, nb->load, nb->in_out);		
		}
		
		// count_hijos = 1; //SE HA REALIZADO LA CUENTA DE LOS HIJOS
		
		for (nb = list_head(neighbour_table_entry_list); nb != NULL; nb = list_item_next(nb))
		{
			if (nb->load == 0)
				load_null++; //SE CONTABILIZAN LOS VECINOS CON CARGA TODAVÍA DESCONOCIDA
		}	

		// if (load_null != 0) //SI UN NODO SABE YA TODAS LAS CARGAS DE SUS VECINOS PUEDE COMENZAR CON EL REPARTO DE CARGAS
		// 	printf("//INFO STATISTICS// Faltan %d nodos por conocer su carga\r\n", load_null);
		
		#if IOTORII_NODE_TYPE > 1
			if (edge == 1){ //SI SE HAN ENVIADO TODOS LOS MENSAJES DE CARGA (PRIMERA ACTUALIZACIÓN COMPLETA)		
				iotorii_handle_load_timer();
				iotorii_handle_share_upstream_timer();
				sent_load = 1;
				sent_share = 1;
				// ctimer_set(&load_timer,  (random_rand() % IOTORII_LOAD_TIME), iotorii_handle_load_timer, NULL);
				// ctimer_set(&share_timer, (random_rand() % IOTORII_SHARE_TIME) + IOTORII_LOAD_TIME, iotorii_handle_share_upstream_timer, NULL);
			}
		#endif
	}
}

//ELIMINA LOS VECINOS DE LOS CUALES NO SE HA RECIBIDO HELLO EN LOS ÚLTIMOS HELLO ENVIADOS
static void iotorii_check_neighbours_hello (void){
	neighbour_table_entry_t *new_nb = list_head(neighbour_table_entry_list), *rem_nb = NULL;
	while(new_nb != NULL)
	{
		if (number_of_hello_messages - new_nb->last_hello > IOTORII_MAX_HELLO_DIFF){
			rem_nb = new_nb;
			list_remove(neighbour_table_entry_list, new_nb);
			number_of_neighbours--;

			new_nb = new_nb->next;
			free(rem_nb);
		} else {
			new_nb = new_nb->next;
		}
	}
}

#endif

// PONE EL FLAG DE TODOS LOS VECINOS A 0
static void iotorii_neighbours_flag (void){
	neighbour_table_entry_t *nb;
	for (nb = list_head(neighbour_table_entry_list); nb != NULL; nb = list_item_next(nb))
	{
		nb->flag = 0; //Se resetea a 0 el flag de cada vecino para cada asignación
	}
}


static void iotorii_handle_hello_timer ()
{
	#if IOTORII_NRF52840_LOG == 1
		if(check_write()){
			memcpy(nvmc+0x504, &write, 4);
			write_log(0xFFFF1111);
			write_log(number_of_hello_messages);
			memcpy(nvmc+0x504, &read, 4);
		}
		update_mem_counter();
	#endif

	#if IOTORII_IPV6 == 1 //IPv6
		udp_conn.udp_conn->lport=UIP_HTONS(UDP_PORT_SEND);
		simple_udp_sendto(&udp_conn, "h", strlen("h"), &broad_ipaddr);
		LOG_DBG("Hello prepared to send\r\n");
		// printf("//INFO HANDLE HELLO// Mensaje Hello enviado con IPv6\r\n");
		
	#else
		int mac_max_payload = max_payload();
	
		if (mac_max_payload <= 0) //FRAMING HA FALLADO Y NO SE PUEDE CREAR HELLO
			LOG_WARN("output: failed to calculate payload size - Hello can not be created\r\n");
		else
		{
			packetbuf_clear(); //HELLO NO TIENE PAYLOAD
			packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_null);
			LOG_DBG("Hello prepared to send\r\n");

			send_packet(NULL, NULL);
			// printf("//INFO HANDLE HELLO// Mensaje Hello enviado sin IPv6\r\n");
		}
	#endif
	
	printf("//INFO HANDLE HELLO// Message Hello sent\r\n");
	#if LOG_DBG_STATISTIC == 1
		number_of_hello_messages++; //SE INCREMENTA EL NÚMERO DE MENSAJES HELLO
	#endif

	if (number_of_hello_messages <= 500){
	#if IOTORII_NODE_TYPE == 1 //ROOT
		//SE PLANIFICA MENSAJE SETHLMAC EN CASO DE SER ROOT
			ctimer_set(&sethlmac_timer, (random_rand() % IOTORII_SETHLMAC_START_TIME) + (IOTORII_SETHLMAC_START_TIME * CLOCK_SECOND), iotorii_handle_sethlmac_timer, NULL);
	#endif
	hello_idle_time = (IOTORII_HELLO_IDLE_TIME * CLOCK_SECOND - IOTORII_HELLO_IDLE_TIME) + (random_rand() % (IOTORII_HELLO_IDLE_TIME * 2));
	ctimer_set(&hello_timer, hello_idle_time, iotorii_handle_hello_timer, NULL);
	}
	// #if IOTORII_NODE_TYPE == 1
	// 	if (number_of_hello_messages == 501)
	// 		printf("500 iteraciones\n");
	// #endif

	iotorii_check_neighbours_hello();
}


void iotorii_handle_send_message_timer ()
{
	payload_entry_t *payload_entry;
	payload_entry = list_pop(payload_entry_list); //POP PACKETBUF DE LA COLA
	
	// if (payload_entry) //EXISTE LA ENTRADA PAYLOAD
	// {
		//SE PREPARA EL BUFFER DE PAQUETES		
		// packetbuf_clear(); //SE RESETEA EL BUFFER 
		
		// memcpy(packetbuf_dataptr(), payload_entry->payload, payload_entry->data_len); //SE COPIA PAYLOAD
		// packetbuf_set_datalen(payload_entry->data_len);
		// clock_time_t sethlmac_delay_time = 1000;
		transmission_delay = 1000;

		//Control info: the destination address, the broadcast address, is tagged to the outbound packet
		#if IOTORII_HLMAC_CAST == 1
			// packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_null);
			#if IOTORII_IPV6 == 1
				prepare_packet (payload_entry->payload, payload_entry->data_len, &broad_ipaddr);
			#else
				prepare_packet (payload_entry->payload, payload_entry->data_len, &linkaddr_null);
			#endif
		#else
			prepare_packet (payload_entry->payload, payload_entry->data_len, &(payload_entry->dest));
			// packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &(payload_entry->dest));
		#endif
		LOG_DBG("Queue: SetHLMAC prepared to send\r\n");
		
		// send_packet(NULL, NULL, &sethlmac_delay_time);
		// printf("Tiempo retransmision: %ld\n\r", transmission_delay);

		#if IOTORII_HLMAC_CAST == 1
			//SE LIBERA MEMORIA
			free(payload_entry->payload);
			payload_entry->payload = NULL;
			free(payload_entry);
			payload_entry = NULL;
		
			if (list_head(payload_entry_list)) //SI LA LISTA TIENE UNA ENTRADA 
			{
				//SE PLANIFICA EL SIGUIENTE MENSAJE
				// clock_time_t sethlmac_delay_time = IOTORII_SETHLMAC_DELAY/2 * (CLOCK_SECOND / 1000);
				//sethlmac_delay_time = sethlmac_delay_time + (random_rand() % sethlmac_delay_time);
				
				#if LOG_DBG_DEVELOPER == 1
					LOG_DBG("Scheduling a SetHLMAC message after %u ticks in the future\r\n", (unsigned)transmission_delay);
				#endif
							
				// ctimer_set(&send_sethlmac_timer, sethlmac_delay_time, iotorii_handle_send_message_timer, NULL);
				//iotorii_handle_send_message_timer();
				// ctimer_set(&send_sethlmac_timer, max_transmission_delay()+1, iotorii_handle_send_message_timer, NULL);
				ctimer_set(&send_sethlmac_timer, max_transmission_delay() + 10, iotorii_handle_send_message_timer, NULL);
			}
		#else

			#if IOTORII_IPV6 == 0
				if(transmission_delay == 1000){
					ctimer_set(&send_sethlmac_timer, max_transmission_delay() + random_rand() % IOTORII_MAX_DELAY, iotorii_handle_send_message_timer, NULL);
					list_push(payload_entry_list, payload_entry); //SI NO SE ALOJA BIEN EL MENSAJE SE VUELVE A AÑADIR A LA LISTA
				} else {
					if (list_head(payload_entry_list)) //SI LA LISTA TIENE UNA ENTRADA 
						ctimer_set(&send_sethlmac_timer, transmission_delay + random_rand() % IOTORII_MAX_DELAY, iotorii_handle_send_message_timer, NULL);
					//SE LIBERA MEMORIA
					free(payload_entry->payload);
					payload_entry->payload = NULL;
					free(payload_entry);
					payload_entry = NULL;
				}
			#else

				if(send_correct){
					ctimer_set(&send_sethlmac_timer, max_transmission_delay() + random_rand() % IOTORII_MAX_DELAY, iotorii_handle_send_message_timer, NULL);
					list_push(payload_entry_list, payload_entry); //SI NO SE ALOJA BIEN EL MENSAJE SE VUELVE A AÑADIR A LA LISTA
				
				} else {
					if (list_head(payload_entry_list)) //SI LA LISTA TIENE UNA ENTRADA 
						ctimer_set(&send_sethlmac_timer, max_transmission_delay() + random_rand() % IOTORII_MAX_DELAY, iotorii_handle_send_message_timer, NULL);
					free(payload_entry->payload);
					payload_entry->payload = NULL;
					free(payload_entry);
					payload_entry = NULL;
				}
			#endif
		#endif
	// }
}


void iotorii_send_sethlmac (hlmacaddr_t addr, addr_t sender_link_address, uint32_t timestamp)
{
	int mac_max_payload = max_payl();
	
	if (mac_max_payload <= 0) //FRAMING HA FALLADO Y SETHLMAC NO SE PUEDE CREAR
	   LOG_WARN("output: failed to calculate payload size - SetHLMAC can not be created\r\n");
	
	else
	{
		neighbour_table_entry_t *neighbour_entry;

		#if LOG_DBG_DEVELOPER == 1 
		LOG_DBG("Info before sending SetHLMAC: ");
		LOG_DBG("Number of neighbours: %d, mac_max_payload: %d, ADDR_SIZE: %d.\r\n", number_of_neighbours, mac_max_payload, ADDR_SIZE);
		#endif

		uint8_t number_of_neighbours_new = number_of_neighbours;

		for (neighbour_entry = list_head(neighbour_table_entry_list); neighbour_entry != NULL; neighbour_entry = list_item_next(neighbour_entry))
		{
			if (addr_cmp(&neighbour_entry->addr, &sender_link_address)) //SE BORRA EL NODO EMISOR DEL MENSAJE SETHLMAC RECIBIDO DEL NUEVO PAYLOAD
			{
				number_of_neighbours_new = number_of_neighbours - 1;
				
				#if LOG_DBG_DEVELOPER == 1
					LOG_DBG("Sender node is in neighbour list, number_of_neighbours: %u, number_of_neighbours_new: %u.\r\n", number_of_neighbours, number_of_neighbours_new);
				#endif
			}
		}

		if (number_of_neighbours_new > 0)
		{
			neighbour_table_entry_t **random_list = (neighbour_table_entry_t **) malloc(sizeof(neighbour_table_entry_t *) * number_of_neighbours_new);
			uint8_t j;
			unsigned short r;
			
			//SE CREA UNA SECUENCIA ALEATORIA PARA LOS VECINOS (ÚTIL SI EL PAYLOAD NO PUEDE ACOMODAR A TODOS LOS VECINOS)
			//TODOS LOS VECINOS TIENEN LA MISMA PRIORIDAD PARA SER ELEGIDOS
			
			for (j = 0; j < number_of_neighbours_new; j++)
			{
				random_list[j] = NULL;
			}
			
			for (neighbour_entry = list_head(neighbour_table_entry_list); neighbour_entry != NULL; neighbour_entry = list_item_next(neighbour_entry))
			{
				if (!addr_cmp(&neighbour_entry->addr, &sender_link_address)) //EL NODO EMISOR NO ESTÁ EN LA LISTA DE VECINOS
				{
					while (random_list[r = random_rand() % (number_of_neighbours_new)] != NULL); //CADA VECINO TIENE UNA r ÚNICA
					random_list[r] = neighbour_entry;
				   
					#if LOG_DBG_DEVELOPER == 1
						LOG_DBG("number_of_neighbours_new = %u Neighbor (ID = %u) gets random priority %u\r\n", number_of_neighbours_new, random_list[r]->id, r);
					#endif
				}
				
				else //EL NODO EMISOR ESTÁ EN LA LISTA DE VECINOS
				{
					#if LOG_DBG_DEVELOPER == 1
						LOG_DBG("Sender node is eliminated in random list!\r\n");
					#endif
				}
		    }
 
			uint8_t *packetbuf_ptr_head;
			uint8_t *packetbuf_ptr;

			int datalen_counter = 0;
			uint8_t i = 1;

			//SE ACOMODAN LAS DIRECCIONES SETHLMAC EN EL PAYLOAD
			//SI EL NODO TIENE VECINOS PARA ENVIAR MENSAJE SETHLMAC Y EL PAYLOAD ES MEÑOR AL MÁXIMO PAYLOAD
			if (random_list[i-1] && (mac_max_payload >= (addr.len + 2 + ADDR_SIZE))) //2 = LONGITUD DEL PREFIJO DE DIRECCIÓN HLMAC(1) + ID(1)
			{
				#if IOTORII_HLMAC_CAST == 0
				do
				{
				#endif
				//SE CREA UNA COPIA DE PACKETBUF ANTES DE EMPEZAR EL PROCESO PARA QUE NO SE SOBREESCRIBA AL ENVIAR MENSAJES SETHLMAC
				packetbuf_ptr_head = (uint8_t *) malloc (sizeof(uint8_t) * mac_max_payload);
				packetbuf_ptr = packetbuf_ptr_head;
				
				/* The payload structure:
				* +-----------+------------+--------+-----+------+-----+-----+------+
				* | Timestamp | Prefix len | Prefix | ID1 | MAC1 | ... | IDn | MACn |
				* +-----------+------------+--------+-----+------+-----+-----+------+ -------->*/
				
				memcpy(packetbuf_ptr, &timestamp, sizeof(uint32_t)); //COPIA TIMESTAMP
				
				packetbuf_ptr+=sizeof(uint32_t);
				datalen_counter=sizeof(uint32_t);
				
				memcpy(packetbuf_ptr, &(addr.len), 1); //COPIA LONGITUD PREFIJO DE VECINO
				
				packetbuf_ptr++;
				datalen_counter++;
				
				memcpy(packetbuf_ptr, addr.address, addr.len); //COPIA PREFIJO DE VECINO
				
				packetbuf_ptr += addr.len;
				datalen_counter += addr.len;

				#if IOTORII_HLMAC_CAST == 1
				do
				{
				#endif
					printf("//INFO HLMAC// HLMAC ");
					for(int k=1; k<addr.len; k++){
						printf(".%2.2X", addr.address[k]);
					}
					printf(".%2.2X assigned to node ", random_list[i-1]->number_id);
					for(int x = 0; x < ADDR_SIZE; x++) {
						if(x > 0 && x % 2 == 0) {
							printf(".");
						}
						printf("%02x", random_list[i-1]->addr.u8[x]);
					}
					printf("\n\r");
					memcpy(packetbuf_ptr, &(random_list[i-1]->number_id), 1); //COPIA ID DE VECINO
					
					packetbuf_ptr++;
					datalen_counter++;
					
					// #if IOTORII_HLMAC_CAST == 1
					memcpy(packetbuf_ptr, &(random_list[i-1]->addr), ADDR_SIZE); //COPIA DIRECCIÓN MAC DE VECINO
					
					packetbuf_ptr += ADDR_SIZE;
					datalen_counter += ADDR_SIZE;
					// #endif

					i++;
				#if IOTORII_HLMAC_CAST == 1
				} while (mac_max_payload >= (datalen_counter + ADDR_SIZE + 1) && i <= number_of_neighbours_new);
				#endif

				//SE CREA Y SE ASIGNAN VALORES A LA ENTRADA DE PAYLOAD
				payload_entry_t *payload_entry = (payload_entry_t*) malloc (sizeof(payload_entry_t));
				payload_entry->next = NULL;
				payload_entry->payload = packetbuf_ptr_head;
				payload_entry->data_len = datalen_counter;
				#if IOTORII_HLMAC_CAST == 0
					payload_entry->dest = random_list[i-2]->addr;
					// printf("Destino: ");
					// for(int c = 0; c < ADDR_SIZE; c++) {
					// 	if(c > 0 && c % 2 == 0) {
					// 		printf(".");
					// 	}
					// 	printf("%02x", payload_entry->dest.u8[c]);
					// }
					// printf("\n\r");
				#endif

				
				if (!list_head(payload_entry_list)) //ANTES DE AÑADIR LA ENTRADA DE PAYLOAD A LA LISTA SE COMPRUEBA SI LA LISTA ESTA VACÍA PARA CONFIGURAR EL TIEMPO
				{
					clock_time_t sethlmac_delay_time = 0; //EL DELAY ANTES DE ENVIAR EL PRIMER MENSAJE SETHLMAC (ENVIADO POR ROOT) ES 0
					
					#if IOTORII_NODE_TYPE > 1 //SI NODO COMÚN SE PLANIFICAN DELAYS ANTES DE ENVIAR LOS DEMÁS MENSAJES SETHLMAC
						// sethlmac_delay_time = IOTORII_SETHLMAC_DELAY/2; //* (CLOCK_SECOND / 1000);
						// sethlmac_delay_time = sethlmac_delay_time + (random_rand() % sethlmac_delay_time);
						sethlmac_delay_time = (random_rand() % (IOTORII_SETHLMAC_DELAY));
						// printf("Delay: %d\n\r", sethlmac_delay_time);

						#if LOG_DBG_DEVELOPER == 1
							LOG_DBG("Scheduling a SetHLMAC message by the root node after %u ticks in the future\r\n", (unsigned)sethlmac_delay_time);
						#endif
					#endif
					
					list_add(payload_entry_list, payload_entry); //SE AÑADE AL FINAL DE LA LISTA LA ENTRADA DE PAYLOAD
					// printf("Tiempo: %d\n\r", sethlmac_delay_time);

					#if IOTORII_NODE_TYPE == 1
						// printf("INICIO CONVERGENCIA\r\n");
						convergence_time = clock_time();
					#endif

					number_of_sethlmac_messages++; //SE INCREMENTA EL NÚMERO DE MENSAJES SETHLMAC
					ctimer_set(&send_sethlmac_timer, sethlmac_delay_time, iotorii_handle_send_message_timer, NULL); //SET TIMER
					//iotorii_handle_send_message_timer();
				}
				else //SI NO ESTÁ VACÍA NO SE CONFIGURA EL TIEMPO Y DIRECTAMENTE SE AÑADE LA ENTRADA DE PAYLOAD
					list_add(payload_entry_list, payload_entry);
				

				#if IOTORII_HLMAC_CAST == 0
				if (i==2)
				#endif
				list_this_node_entry (payload_entry, &addr); //RELLENA LA ESTRUCTURA	
				

				#if IOTORII_HLMAC_CAST == 0
				} while (mac_max_payload >= (datalen_counter + ADDR_SIZE + 1) && i <= number_of_neighbours_new);
				#endif

				

				char *neighbour_hlmac_addr_str = hlmac_addr_to_str(addr);
				LOG_DBG("SetHLMAC prefix (addr:%s) added to queue to advertise to %d nodes.\r\n", neighbour_hlmac_addr_str, i-1);
				free(neighbour_hlmac_addr_str);


				#if IOTORII_NRF52840_LOG == 1
					if(check_write()){
						int c=1;
						memcpy(nvmc+0x504, &write, 4);
						write_log(0xFFFF3333);
						write_log(timestamp);
						write_log(addr.len);
						for(int k = 0; k < addr.len; k++){
							write_log(addr.address[k]);
						}
						write_log(i);
						do
						{
							memcpy(log+mem_counter, &(random_list[c-1]->addr), sizeof(random_list[c-1]->addr));
							mem_counter = mem_counter + sizeof(random_list[c-1]->addr);
							write_log(random_list[c-1]->number_id);
							c++;
						} while (c < i);
						memcpy(nvmc+0x504, &read, 4);
					}
					update_mem_counter();
				#endif
			}			
			else //SI EL NODO NO TIENE VECINOS PARA ENVIAR MENSAJE SETHLMAC O EL PAYLOAD ES PEQUEÑO
				LOG_DBG("Node has not any neighbour (or payload is low) to send SetHLMAC.\r\n");


			//SE BORRA LA LISTA ALEATORIA DE VECINOS
			if (number_of_neighbours_new > 0) //SI SE HAN CREADO NUEVOS VECINOS
			{
				for (j = 0; j < number_of_neighbours_new; j++)
				{
					random_list[j] = NULL; 
				}
				
				free(random_list);
				random_list = NULL;
			}
		}//END if number_of_neighbours_new
		else { //SOLO TIENE 1 VECINO DEL QUE RECIBIÓ LA ASIGNACIÓN
			payload_entry_t *payload_entry = (payload_entry_t*) malloc (sizeof(payload_entry_t));
			uint8_t zero_payload = 0;
			int zero_length = 0;
			payload_entry->next = NULL;
			payload_entry->payload = &zero_payload;
			payload_entry->data_len = zero_length;
			list_this_node_entry (payload_entry, &addr); //RELLENA LA ESTRUCTURA
		}
	} // END else
	#endif

	//ctimer_set(&statistic_timer, IOTORII_STATISTICS_TIME * CLOCK_SECOND, iotorii_handle_statistic_timer, NULL); //SE MOSTRARÁN LAS ESTADÍSTICAS ACTUALIZADAS
	// clock_time_t sethlmac_delay_time = 0;
	// sethlmac_delay_time = (IOTORII_SETHLMAC_DELAY * (CLOCK_SECOND / 1000));
	
	#if IOTORII_HLMAC_CAST == 1
		ctimer_set(&statistic_timer, max_transmission_delay()+IOTORII_SETHLMAC_DELAY, iotorii_handle_statistic_timer, NULL); //SE MOSTRARÁN LAS ESTADÍSTICAS ACTUALIZADAS
	#endif
}



#if IOTORII_IPV6 == 1
void iotorii_handle_incoming_hello (addr_t *sender_addr) //PROCESA UN PAQUETE HELLO (DE DIFUSIÓN) RECIBIDO DE OTROS NODOS
{
#else
void iotorii_handle_incoming_hello () //PROCESA UN PAQUETE HELLO (DE DIFUSIÓN) RECIBIDO DE OTROS NODOS
{
	const addr_t *sender_addr = packetbuf_addr(PACKETBUF_ADDR_SENDER); //SE LEE EL BUFFER

	LOG_DBG("A Hello message received from ");
	LOG_DBG_LLADDR(sender_addr);
	LOG_DBG("\r\n");
#endif

	#if IOTORII_NRF52840_LOG == 1
		if(check_write()){
			memcpy(nvmc+0x504, &write, 4);
			write_log(0xFFFF2222);
			memcpy(log+mem_counter, sender_addr, ADDR_SIZE);
			mem_counter += ADDR_SIZE;
			memcpy(nvmc+0x504, &read, 4);
		}
		update_mem_counter();
	#endif

				// printf("Hello de: ");
				// for(int i = 0; i < ADDR_SIZE; i++) {
				// 	if(i > 0 && i % 2 == 0) {
				// 		printf(".");
				// 	}
				// 	printf("%02x", sender_addr->u8[i]);
				// }
				// printf("\n\r");


	if (number_of_neighbours < 256) //SI NO SE HA SUPERADO EL MÁXIMO DE ENTRADAS EN LA LISTA
	{
		neighbour_table_entry_t *new_nb;
		uint8_t address_is_in_table = 0;
		
		for (new_nb = list_head(neighbour_table_entry_list); new_nb != NULL; new_nb = new_nb->next)
		{
			if (addr_cmp(&(new_nb->addr), sender_addr)){
				address_is_in_table = 1; //SE PONE A 1 SI LA DIRECCIÓN DEL EMISOR SE ENCUENTRA EN LA LISTA
				#if LOG_DBG_STATISTIC == 1
					new_nb->last_hello=number_of_hello_messages; //Se actualiza el número de hellos enviados al recibir el hello
				#endif
			}
		}
		
		if (!address_is_in_table) //SI NO ESTÁ EN LA LISTA
		{

			new_nb = (neighbour_table_entry_t*) malloc (sizeof(neighbour_table_entry_t)); //SE RESERVA ESPACIO
			new_nb->number_id = ++number_of_neighbours;
			//number_of_neighbours_flag++;
			new_nb->addr = *sender_addr;
			new_nb->flag = 0;
			new_nb->load = 0; //CARGA INICIAL NULA
			new_nb->in_out = 0; //CARGA ENTRANTE O SALIENTE NULA AHORA
			new_nb->last_hello=number_of_hello_messages; //GUARDA EL NÚMERO DE HELLOS ENVIADOS
			
			list_add(neighbour_table_entry_list, new_nb); //SE AÑADE A LA LISTA
			
			#if IOTORII_IPV6 == 0 
				LOG_DBG("A new neighbour added to IoTorii neighbour table, address: ");
				LOG_DBG_LLADDR(sender_addr);
				LOG_DBG(", ID: %d\r\n", new_nb->number_id);
			#endif
		}
		else
		{
			#if IOTORII_IPV6 == 0 
				LOG_DBG("Address of hello (");
				LOG_DBG_LLADDR(sender_addr);
				LOG_DBG(") message received already!\r\n");
			#endif
		}
		printf("//INFO INCOMING HELLO// Message Hello received\r\n");
	}
	else //TABLA LLENA (256)
		LOG_WARN("The IoTorii neighbour table is full! \r\n");
}

#if IOTORII_IPV6 == 1 
void iotorii_handle_incoming_sethlmac_or_load (addr_t *sender_address, uint8_t *packetbuf_ptr_head, uint16_t packetbuf_data_len) //PROCESA UN MENSAJE DE DIFUSIÓN SETHLMAC RECIBIDO DE OTROS NODOS
{
#else
void iotorii_handle_incoming_sethlmac_or_load () //PROCESA UN MENSAJE DE DIFUSIÓN SETHLMAC RECIBIDO DE OTROS NODOS
{
		int packetbuf_data_len = packetbuf_datalen();
		uint8_t *packetbuf_ptr_head = packetbuf_dataptr();
#endif
	LOG_DBG("A message received from ");
	LOG_DBG_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
	LOG_DBG("\r\n");

	hlmacaddr_t *received_hlmac_addr;

	#if IOTORII_IPV6 == 1
		received_hlmac_addr = iotorii_extract_address(sender_address, packetbuf_ptr_head, packetbuf_data_len); //SE COGE LA DIRECCIÓN RECIBIDA
		uint32_t timestamp = iotorii_extract_timestamp(packetbuf_ptr_head); //SE COGE EL TIMESTAMP RECIBIDA
		addr_t sender_addr = *sender_address;
	#else
		received_hlmac_addr = iotorii_extract_address(); //SE COGE LA DIRECCIÓN RECIBIDA
		uint32_t timestamp = iotorii_extract_timestamp(); //SE COGE EL TIMESTAMP RECIBIDA
		addr_t sender_addr = *packetbuf_addr(PACKETBUF_ADDR_SENDER);
	#endif
	
	const addr_t *sender = &sender_addr;

				// printf("Mensaje de: ");
				// for(int i = 0; i < ADDR_SIZE; i++) {
				// 	if(i > 0 && i % 2 == 0) {
				// 		printf(".");
				// 	}
				// 	printf("%02x", sender_addr.u8[i]);
				// }
				// printf("\n\r");
	
	neighbour_table_entry_t *nb;
	this_node_t *node;
	node = list_head(node_list);

	if (hlmac_is_unspecified_addr(*received_hlmac_addr)) //SI NO SE ESPECIFICA DIRECCIÓN, NO HAY PARA EL NODO
	{
		if (packetbuf_data_len == 4) //LOAD
		{
			// printf("Load de: ");
			// for(int i = 0; i < ADDR_SIZE; i++) {
			// 	if(i > 0 && i % 2 == 0) {
			// 		printf(".");
			// 	}
			// 	printf("%02x", sender_addr.u8[i]);
			// }
			// printf("\n\r");
			
			int *p_load = (int *) malloc (sizeof(int));
			memcpy(p_load, packetbuf_ptr_head, packetbuf_data_len); //COPIA DEL BUFFER 

			char* str_sender = (char*) malloc (sizeof(char) * (ADDR_SIZE * (2 + 1) + 1));
			addr_to_str (str_sender, &sender_addr, SIZE);	//LENGTH = 1 PARA MOSTRAR SOLO EL ID

			#if IOTORII_NRF52840_LOG == 1
				if(check_write()){
					memcpy(nvmc+0x504, &write, 4);
					write_log(0xFFFF6666);
					memcpy(log+mem_counter, &sender_addr, ADDR_SIZE);
					mem_counter += ADDR_SIZE;
					write_log(*p_load);
					memcpy(nvmc+0x504, &read, 4);
				}
				update_mem_counter();
			#endif
			
			for (nb = list_head(neighbour_table_entry_list); nb != NULL; nb = list_item_next(nb))
			{
				if (addr_cmp(&nb->addr, sender)) //SE BUSCA EN LA LISTA DE VECINOS LA DIRECCIÓN QUE HA ENVIADO EL MENSAJE 
				{
					memcpy(&nb->load, packetbuf_ptr_head, packetbuf_data_len); //SE ACTUALIZA LA CARGA EN LA LISTA DE VECINOS						
					printf("//INFO INCOMING LOAD// Load received: %d from node 0x%s\r\n", *p_load, str_sender);

					// printf("Hijos LOAD: %d    Flag: %d\n\r", n_hijos_load, nb->flag);
					// printf("Origen: ");
					// for(int i = 0; i < ADDR_SIZE; i++) {
					// 	if(i > 0 && i % 2 == 0) {
					// 		printf(".");
					// 	}
					// 	printf("%02x", sender->u8[i]);
					// }
					// printf("\n\r");

					if (nb->flag == 0 && edge == 0 && n_hijos_load != 0)
					{
						n_hijos_load--;
					}

					#if IOTORII_NODE_TYPE == 1 //SI LLEGA AL ROOT HA TERMINADO EL INTERCAMBIO DE DATOS
						if (n_hijos_load == 0 && n_hijos_share == 0 && has_converged == 0){
							convergence_time = clock_time() - convergence_time;
							number_times_converged++;
							has_converged = 1;
							// printf("%ld\n", convergence_time);
							printf("//INFO CONVERGENCE// Cycles until convergence: %ld\n\r", convergence_time);
							// printf("Ciclos por segundo: %ld\n\r", CLOCK_SECOND);
							// printf("Ha realizado un total de %d asignaciones y ha convergido %d veces (%d%%)\n\r\n\r", number_of_sethlmac_messages, number_times_converged, (number_times_converged * 100)/number_of_sethlmac_messages);
							// printf("FIN CONVERGENCIA\r\n");

							#if IOTORII_NRF52840_LOG == 1
								if(check_write()){
									memcpy(nvmc+0x504, &write, 4);
									write_log(0xFFFFBBBB);
									write_log(convergence_time);
									memcpy(nvmc+0x504, &read, 4);
								}
								update_mem_counter();
							#endif
						}
					#else //SI NO ES ROOT ENVIA SU LOAD
						if(!edge && sent_load == 0 && n_hijos_load == 0){
							// ctimer_set(&load_timer, (random_rand() % IOTORII_LOAD_TIME), iotorii_handle_load_timer, NULL);
							iotorii_handle_load_timer();
							sent_load = 1;
						}
					#endif
				}
			}
			
			free(p_load);
			p_load = NULL;
			
			free(str_sender);
			str_sender = NULL;
		}
		
		else if (packetbuf_data_len == 2) //SHARE
		{			
			short *p_extra = (short *) malloc (sizeof(short));
			memcpy(p_extra, packetbuf_ptr_head, packetbuf_data_len); //COPIA DEL BUFFER LA CARGA SOBRANTE QUE HA ENVIADO EL NODO

			#if IOTORII_NRF52840_LOG == 1
				if(check_write()){
					memcpy(nvmc+0x504, &write, 4);
					write_log(0xFFFF8888);
					memcpy(log+mem_counter, &sender_addr, ADDR_SIZE);
					mem_counter += ADDR_SIZE;
					write_log(*p_extra);
					memcpy(nvmc+0x504, &read, 4);
				}
				update_mem_counter();
			#endif

			// printf("Share de: ");
			// for(int i = 0; i < ADDR_SIZE; i++) {
			// 	if(i > 0 && i % 2 == 0) {
			// 		printf(".");
			// 	}
			// 	printf("%02x", sender_addr.u8[i]);
			// }
			// printf("\n\r");

			for (nb = list_head(neighbour_table_entry_list); nb != NULL; nb = list_item_next(nb))
			{
				if (addr_cmp(&nb->addr, sender)) //SE BUSCA EN LA LISTA DE VECINOS LA DIRECCIÓN QUE HA ENVIADO EL MENSAJE 
				{	
					//NODO NO EDGE RECIBE CARGA DE SU HIJO O HIJOS
					if (nb->flag == 0 && edge == 0 && n_hijos_share != 0)
					{
						//if (*p_extra > 0) //LA CARGA RECIBIDA DEL HIJO ES POSITIVA Y HAY QUE MODIFICAR LA CARGA DEL NODO
						node->load = node->load + *p_extra;
						
						printf("//INFO INCOMING SHARE// node's load: %d\r\n", node->load);

						// printf("Hijos share: %d\n\r", n_hijos_share);
						// printf("Origen: ");
						// for(int i = 0; i < ADDR_SIZE; i++) {
						// 	if(i > 0 && i % 2 == 0) {
						// 		printf(".");
						// 	}
						// 	printf("%02x", sender->u8[i]);
						// }
						// printf("\n\r");
						
						n_hijos_share--; //SE RESTA EL HIJO DE LA CUENTA TOTAL DE HIJOS

						#if IOTORII_NODE_TYPE == 1 //SI LLEGA AL ROOT HA TERMINADO EL INTERCAMBIO DE DATOS
							if (n_hijos_load == 0 && n_hijos_share == 0 && has_converged == 0){
								convergence_time = clock_time() - convergence_time;
								number_times_converged++;
								has_converged = 1;
								// printf("%ld\n", convergence_time);
								printf("//INFO CONVERGENCE// Cycles until convergence: %ld\n\r", convergence_time);
								// printf("Ciclos por segundo: %ld\n\r", CLOCK_SECOND);
								// printf("Ha realizado un total de %d asignaciones y ha convergido %d veces (%d%%)\n\r\n\r", number_of_sethlmac_messages, number_times_converged, (number_times_converged * 100)/number_of_sethlmac_messages);
								// printf("FIN CONVERGENCIA\r\n");

								#if IOTORII_NRF52840_LOG == 1
									if(check_write()){
										memcpy(nvmc+0x504, &write, 4);
										write_log(0xFFFFBBBB);
										write_log(convergence_time);
										memcpy(nvmc+0x504, &read, 4);
									}
									update_mem_counter();
								#endif
							}
						#else //SI NO ES ROOT ENVIA SU SHARE
							// ctimer_set(&share_timer, (random_rand() % IOTORII_SHARE_TIME), iotorii_handle_share_upstream_timer, NULL);	
							if (n_hijos_share == 0 && sent_share == 0){
								iotorii_handle_share_upstream_timer();
								sent_share = 1;
							}
							//ctimer_set(&statistic_timer, IOTORII_STATISTICS_TIME * CLOCK_SECOND, iotorii_handle_statistic_timer, NULL); //SE MOSTRARÁN LAS ESTADÍSTICAS ACTUALIZADAS		
						#endif
					}
					
					if (*p_extra != 0 && nb->in_out == 0) //SI HA HABIDO ACTUALIZACIÓN DE LA CARGA, SE ACTUALIZA EL VALOR DEL "FLUJO DE CARGA"
						nb->in_out = *p_extra;
				}
			}
			
			free(p_extra);
			p_extra = NULL;	
		}
	}
	
	else //ESTÁ ESPECIFICADA
	{
		char *new_hlmac_addr_str = hlmac_addr_to_str(*received_hlmac_addr);
		printf("//INFO INCOMING HLMAC// HLMAC received: %s\r\n", new_hlmac_addr_str);
		free(new_hlmac_addr_str);
			// printf("Hlmac de: ");
			// for(int i = 0; i < ADDR_SIZE; i++) {
			// 	if(i > 0 && i % 2 == 0) {
			// 		printf(".");
			// 	}
			// 	printf("%02x", sender_addr.u8[i]);
			// }
			// printf("\n\r");

		#if IOTORII_NRF52840_LOG == 1
			if(check_write()){
				memcpy(nvmc+0x504, &write, 4);
				write_log(0xFFFF4444);
				memcpy(log+mem_counter, &sender_addr, ADDR_SIZE);
				mem_counter = mem_counter + ADDR_SIZE;
				write_log(timestamp);
				write_log(received_hlmac_addr->len);
				for(int k = 0; k < received_hlmac_addr->len; k++){
					write_log(received_hlmac_addr->address[k]);
				}
				memcpy(nvmc+0x504, &read, 4);
			}
			update_mem_counter();
		#endif

		if (timestamp > last_timestamp || (!hlmactable_has_loop(*received_hlmac_addr) && timestamp == last_timestamp)) //SI NO HAY BUCLE, SI SE LA MANDA AL HIJO
		{
			uint8_t is_added = hlmactable_add(*received_hlmac_addr, timestamp);

			if (is_added) //SI SE HA ASIGNADO HLMAC AL NODO
			{
				// printf("Longitud lista node: %d\n\r", list_length(node_list));
				if (last_timestamp < timestamp || last_timestamp == 0){
					last_timestamp = timestamp;
					iotorii_neighbours_flag();
					number_of_neighbours_flag = number_of_neighbours; //SE RESETEA PARA CADA ASIGNACIÓN DE HLMAC

					#if IOTORII_HLMAC_CAST == 0
						n_hijos_load = number_of_neighbours;
						n_hijos_share = number_of_neighbours;

						edge = 0;
					#endif

					//ELIMINAR TODAS LAS ENTRADAS DE LA LISTA
					this_node_t *new_address;
					while(list_head(node_list) != NULL){
						new_address = list_chop(node_list);
						free(new_address->str_addr);
						new_address->str_addr = NULL;
						free(new_address->str_top_addr);
						new_address->str_top_addr = NULL;
						free(new_address);
						new_address = NULL;
					}
	
					sent_load = 0;
					sent_share = 0;
				}

				#if IOTORII_NRF52840_LOG == 1
					if(check_write()){
						memcpy(nvmc+0x504, &write, 4);
						write_log(0xFFFFAAAA);
						write_log(received_hlmac_addr->len);
						for(int k = 0; k < received_hlmac_addr->len; k++){
							write_log(received_hlmac_addr->address[k]);
						}
						write_log(timestamp);
						memcpy(nvmc+0x504, &read, 4);
					}
					update_mem_counter();
				#endif


				LOG_DBG("New HLMAC address is assigned to the node.\r\n");
				LOG_DBG("New HLMAC address is sent to the neighbours.\r\n");
				iotorii_send_sethlmac(*received_hlmac_addr, sender_addr, timestamp); //SE ENVÍA A LOS DEMÁS NODOS
			
				free(received_hlmac_addr->address);
				received_hlmac_addr->address = NULL;
				free(received_hlmac_addr);
				received_hlmac_addr = NULL;
			}
			else //NO SE HA ASIGNADO
			{
				LOG_DBG("New HLMAC address not added to the HLMAC table, and memory is free.\r\n");
				
				free(received_hlmac_addr->address);
				received_hlmac_addr->address = NULL;
				free(received_hlmac_addr);
				received_hlmac_addr = NULL;
			}
			
			//BÚSQUEDA DE LA DIRECCIÓN DEL EMISOR EN LA LISTA DE VECINOS 
			for (nb = list_head(neighbour_table_entry_list); nb != NULL; nb = list_item_next(nb))
			{
				if (addr_cmp(&nb->addr, sender))
				{
					if (is_added)
						nb->flag = 1; //SE MARCA COMO PADRE ESE VECINO 
					else
						nb->flag = -1; //SE MARCA COMO PADRE ESE VECINO PERO SIN UNIÓN DE PADRE
				}
			}
			number_of_neighbours_flag--; //SE DECREMENTA EL NÚMERO DE VECINOS DE LOS QUE NO SE HA RECIBIDO HLMAC

			#if IOTORII_HLMAC_CAST == 0
				if (n_hijos_load > 0)
					n_hijos_load--;
				if (n_hijos_share > 0)
					n_hijos_share--;

				// printf("Faltan %d nodos load y %d nodos share para ser edge\n\r", n_hijos_load, n_hijos_share);
				
				if (!number_of_neighbours_flag)
				{
					// printf("es edge\r\n");
					edge = 1;

					#if IOTORII_NRF52840_LOG == 1
						if(check_write()){
							memcpy(nvmc+0x504, &write, 4);
							write_log(0xFFFF9999);
							memcpy(nvmc+0x504, &read, 4);
						}
						update_mem_counter();
					#endif

					iotorii_handle_load_timer();
					iotorii_handle_share_upstream_timer();
					sent_load = 1;
					sent_share = 1;
				}

				if(edge == 0 && n_hijos_load == 0 && sent_load == 0){
					iotorii_handle_load_timer();
					sent_load = 1;
				}
				if(edge == 0 && n_hijos_share == 0 && sent_share == 0){
					iotorii_handle_share_upstream_timer();
					sent_share = 1;
				}
			#endif
		}
		else
		{
			LOG_DBG("New HLMAC address not assigned to the node (loop), and memory is free.\r\n");
			
			free(received_hlmac_addr->address);
			received_hlmac_addr->address = NULL;
			free(received_hlmac_addr);
			received_hlmac_addr = NULL;

			#if IOTORII_HLMAC_CAST == 0
				if (n_hijos_load > 0)
					n_hijos_load--;
				if (n_hijos_share > 0)
					n_hijos_share--;

				if(edge == 0 && n_hijos_load == 0 && sent_load == 0){
					iotorii_handle_load_timer();
					sent_load = 1;
				}
				if(edge == 0 && n_hijos_share == 0 && sent_share == 0){
					iotorii_handle_share_upstream_timer();
					sent_share = 1;
				}
			#endif
		}

		#if IOTORII_NODE_TYPE == 1 //SI LLEGA AL ROOT HA TERMINADO EL INTERCAMBIO DE DATOS
			if (n_hijos_load == 0 && n_hijos_share == 0 && has_converged == 0){
				convergence_time = clock_time() - convergence_time;
				number_times_converged++;
				has_converged = 1;
				// printf("%ld\n", convergence_time);
				printf("//INFO CONVERGENCE// Cycles until convergence: %ld\n\r", convergence_time);
				// printf("Ciclos por segundo: %ld\n\r", CLOCK_SECOND);
				// printf("Ha realizado un total de %d asignaciones y ha convergido %d veces (%d%%)\n\r\n\r", number_of_sethlmac_messages, number_times_converged, (number_times_converged * 100)/number_of_sethlmac_messages);
				// printf("FIN CONVERGENCIA\r\n");

				#if IOTORII_NRF52840_LOG == 1
					if(check_write()){
						memcpy(nvmc+0x504, &write, 4);
						write_log(0xFFFFBBBB);
						write_log(convergence_time);
						memcpy(nvmc+0x504, &read, 4);
					}
					update_mem_counter();
				#endif
			}
		#endif
	}
	
	// #if LOG_DBG_STATISTIC == 1
	// 	printf("Periodic Statistics: node_id: %u, convergence_time_end\r\n", node_id);
	// #endif
}


#if IOTORII_IPV6 == 0 //No IPv6
void iotorii_operation (void)
{
	// printf("Longitud: %d\n\r", packetbuf_datalen());
	// if (packetbuf_holds_broadcast())
	// {
		if (packetbuf_datalen() == 0) //SI LA LONGITUD ES NULA, SE HA RECIBIDO UN PAQUETE HELLO
			iotorii_handle_incoming_hello(); //SE PROCESA PAQUETE HELLO
		else //SI NO ES NULA, SE HA RECIBIDO UN PAQUETE SETHLMAC
			iotorii_handle_incoming_sethlmac_or_load(); //SE PROCESA PAQUETE SETHLMAC
	// }
}
#endif



/*---------------------------------------------------------------------------*/

#if IOTORII_NODE_TYPE == 1 //ROOT

static void iotorii_handle_sethlmac_timer ()
{
	static uint32_t timestamp=-1;
	timestamp++;
	has_converged = 0;
	hlmacaddr_t root_addr; //SE CREA EL ROOT
	hlmac_create_root_addr(&root_addr, 1);
	hlmactable_add(root_addr, timestamp);

	#if IOTORII_NRF52840_LOG == 1
		if(check_write()){
			memcpy(nvmc+0x504, &write, 4);
			write_log(0xFFFFAAAA);
			write_log(1);
			for(int k = 0; k < 1; k++){
				write_log(root_addr.address[k]);
			}
			write_log(timestamp);
			memcpy(nvmc+0x504, &read, 4);
		}
		update_mem_counter();
	#endif
	
	// #if LOG_DBG_STATISTIC == 1
	// 	printf("Periodic Statistics: node_id: %u, convergence_time_start\r\n", node_id);
	// #endif
	
	addr_t node_addr;
	#if IOTORII_IPV6 == 1 
		uip_ds6_select_src(&node_addr, &broad_ipaddr);    //DIRECCIÓN IP DEL PROCESO NATIVO
	#else
		node_addr = linkaddr_node_addr;
	#endif

	iotorii_send_sethlmac(root_addr, node_addr, timestamp);
	free(root_addr.address);
	root_addr.address = NULL;
	
	//ESTADÍSTICAS
	#if LOG_DBG_STATISTIC == 1
		//ctimer_set(&statistic_timer, IOTORII_STATISTICS_TIME * CLOCK_SECOND, iotorii_handle_statistic_timer, NULL);
		clock_time_t sethlmac_delay_time = 0;
		sethlmac_delay_time = IOTORII_STATISTICS_TIME/2 * (CLOCK_SECOND / 128);
		sethlmac_delay_time = sethlmac_delay_time + (random_rand() % sethlmac_delay_time);
		#if IOTORII_HLMAC_CAST == 1
			ctimer_set(&statistic_timer, IOTORII_STATISTICS_TIME, iotorii_handle_statistic_timer, NULL);
		#else
			n_hijos_share=number_of_neighbours;
			n_hijos_load=number_of_neighbours;
		#endif
	#endif
	
	iotorii_neighbours_flag();
	number_of_neighbours_flag = number_of_neighbours; //SE RESETEA PARA CADA ASIGNACIÓN DE HLMAC
}

#endif


static void init (void)
{
	#if LLSEC802154_USES_AUX_HEADER
		#ifdef CSMA_LLSEC_DEFAULT_KEY0
			uint8_t key[16] = CSMA_LLSEC_DEFAULT_KEY0;
			csma_security_set_key(0, key);
		#endif
	#endif /* LLSEC802154_USES_AUX_HEADER */
	
	csma_output_init(); //INICIALIZA VECINO
	on();
	
	#if IOTORII_NODE_TYPE == 1 //INFORMA QUE ES ROOT
		LOG_INFO("This node operates as a root.\r\n");
	#endif
	
	#if IOTORII_NODE_TYPE > 1 //INFORMA QUE ES NODO COMÚN
		LOG_INFO("This node operates as a common node.\r\n ");
	#endif

	#if IOTORII_IPV6 == 1 //IPv6
  		uip_ip6addr(&broad_ipaddr, 0xFF03,0,0,0,0,0,0,0xFC);
		if(!simple_udp_register(&udp_conn, UDP_PORT_LISTEN, NULL, UDP_PORT_SEND, udp_rx_callback)){
			LOG_DBG("Error when registering callback function\r\n");
		}
		udp_conn.udp_conn->ttl=10;
	#endif
	
	#if IOTORII_NODE_TYPE > 0 //ROOT O NODO COMÚN
		unsigned short seed_number;
		uint8_t min_len_seed = sizeof(unsigned short) < ADDR_SIZE ?  sizeof(unsigned short) : ADDR_SIZE;
		
		memcpy(&seed_number, &linkaddr_node_addr, min_len_seed);
		random_init(seed_number); //SE INICIALIZA UN SEED NUMBER DIFERENTE PARA CADA NODO
		
		#if LOG_DBG_DEVELOPER == 1
			LOG_DBG("Seed is %2.2X (%d), sizeof(Seed) is %u\r\n", seed_number, seed_number, min_len_seed);
		#endif

		//SE PLANIFICA MENSAJE HELLO
		hello_start_time = (IOTORII_HELLO_START_TIME * CLOCK_SECOND - IOTORII_HELLO_START_TIME) + (random_rand() % (IOTORII_HELLO_START_TIME * 2));
		LOG_DBG("Scheduling a Hello message after %u ticks in the future\r\n", (unsigned)hello_start_time);
		ctimer_set(&hello_timer, hello_start_time, iotorii_handle_hello_timer, NULL);

		number_of_neighbours = 0;
		number_of_neighbours_flag = 0;
		hlmac_table_init(); //SE CREA LA TABLA DE VECINOS
		
		// int erase=0x1, erase_en=0x10, eras_dis=0x0;
		// memcpy(nvmc+0x504, &erase_en, 4);
		// memcpy(nvmc+0x50C, &erase, 4);
		// memcpy(nvmc+0x504, &erase_dis, 4);

		// int write=0x1, read=0x0;
		// int dis_prot = 0xFFFFFF00;
		// memcpy(nvmc+0x504, &write, 4);
		// memcpy(protection, &dis_prot, 4);
		// memcpy(nvmc+0x504, &read, 4);


		// memcpy(&check, nvmc+0x400, 4);
		// memcpy(&check2, nvmc+0x408, 4);
		// printf("Ready: %d\tNext: %d\n\r", check, check2);
		// if(check && check2){
		// 	memcpy(nvmc+0x504, &write, 4);
		// 	memcpy(log+0x100, &data, 8);
		// 	memcpy(nvmc+0x504, &read, 4);
		// }
		#if IOTORII_NRF52840_LOG == 1
			ctimer_set(&log_timer, hello_start_time/2, read_log, NULL);
		#endif
	#endif
}


static void input_packet (void) //SE RECIBE UN PAQUETE 
{
	#if CSMA_SEND_SOFT_ACK
		uint8_t ackdata[CSMA_ACK_LEN];
	#endif

	if (packetbuf_datalen() == CSMA_ACK_LEN) //SE IGNORAN PAQUETES ACK
	{
		LOG_DBG("ignored ack\r\n");
	} 
	else if (csma_security_parse_frame() < 0) //ERROR AL PARSEAR
	{
		LOG_ERR("failed to parse %u\r\n", packetbuf_datalen());
	} 
	else if (!linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &linkaddr_node_addr) && !packetbuf_holds_broadcast()) //NO COINCIDE LA DIRECCIÓN
	{
		LOG_WARN("not for us\r\n");
	} 
	else if (linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_SENDER), &linkaddr_node_addr)) //PAQUETE DEL PROPIO NODO
	{
		LOG_WARN("frame from ourselves\r\n");
	} 
	else 
	{
		int duplicate = 0;

		duplicate = mac_sequence_is_duplicate(); //SE COMPRUEBA SI ESTÁ DUPLICADO
		
		if (duplicate) //SI LO ESTÁ, SE BORRA
		{
			LOG_WARN("drop duplicate link layer packet from ");
			LOG_WARN_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
			LOG_WARN_(", seqno %u\r\n", packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO));
		} 
		else 
		{
		    mac_sequence_register_seqno(); //SE REGISTRA
		}

		#if CSMA_SEND_SOFT_ACK
			if (packetbuf_attr(PACKETBUF_ATTR_MAC_ACK)) 
			{
				ackdata[0] = FRAME802154_ACKFRAME;
				ackdata[1] = 0;
				ackdata[2] = ((uint8_t*) packetbuf_hdrptr())[2];
				NETSTACK_RADIO.send(ackdata, CSMA_ACK_LEN); //SE ENVÍA ACK
			}
		#endif /* CSMA_SEND_SOFT_ACK */
		
		if (!duplicate)  //SI NO ESTÁ DUPLICADO
		{
			LOG_INFO("received packet from ");
			LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
			LOG_INFO_(", seqno %u, len %u\r\n", packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO), packetbuf_datalen());

			#if IOTORII_IPV6 == 1
				NETSTACK_NETWORK.input();
			#elif IOTORII_IPV6 == 0
				iotorii_operation();
			#endif
		}
	}
}


#if IOTORII_NODE_TYPE == 0
	const struct mac_driver csma_driver = { "CSMA",
#elif IOTORII_NODE_TYPE > 0
	const struct mac_driver iotorii_csma_driver = { "IoTorii CSMA",
#endif

	init,
	send_packet,
	input_packet,
	on,
	off,
	max_payload,
};

void link_addr_to_str (char* buf, const addr_t addr, int length)
{
	char *s = buf;
	uint8_t i;
	
	for (i = 0; i < length; i++, s += 3)
		sprintf(s, "%2.2X.", (char)addr.u16[i]);

	*(s) = '\0'; //SE AÑADE '\0' AL FINAL
}


void list_this_node_entry (payload_entry_t *a, hlmacaddr_t *addr)
{
	payload_entry_t *aux = a;
	this_node_t *new_address;
	
	char* str_addr = hlmac_addr_to_str(*addr);
	char* str_top_addr = (char*) malloc ((addr->len-1)*3 * sizeof(char));

	if (addr->len > 1)
		strncpy(str_top_addr, str_addr, (addr->len-1)*3); //COPIA HASTA EL PENÚLTIMO ID
	else
		str_top_addr = "/";
	

	while (aux != NULL)
	{		
		new_address = (this_node_t*) malloc (sizeof(this_node_t)); //SE RESERVA ESPACIO
		
		new_address->str_addr = str_addr;		
		new_address->str_top_addr = str_top_addr;	
		
		new_address->payload = *aux->payload;
		new_address->nhops = hlmactable_calculate_sum_hop();
		new_address->load = random_rand() % 200; //CARGA ALEATORIA (SE HA IMPUESTO UN MAX DE 180)
		
		list_add(node_list, new_address); 
		// printf("[addr: %s | top_addr: %s | payload/hops: %d | node_load: %d]\r\n", new_address->str_addr, new_address->str_top_addr, new_address->nhops, new_address->load);
		aux = aux->next;
	}
}
