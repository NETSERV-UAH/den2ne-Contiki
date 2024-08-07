

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

//#define LOG_CONF_LEVEL_MAC                         LOG_LEVEL_DBG
//#define RTIMER_CONF_CLOCK_SIZE 4

/* Configuring the developer debug log */
#define LOG_CONF_DBG_DEVELOPER 0

/* Configuring the statistics debug log */
#define LOG_CONF_DBG_STATISTIC 1

/*---------------------------------------------------------------------------*/

//CONFIGURACIÓN DEL NÚMERO MAX DE DIRECCIONES HLMAC QUE PUEDE TENR UN NODO
//POR DEFECTO ES 1 Y CON -1 SE INDICA ILIMITADO
#define HLMAC_CONF_MAX_HLMAC 1 //3

#define IOTORII_CONF_NODE_TYPE 1 //1 = SE DEFINE COMO NODO ROOT

/*---------------------------------------------------------------------------*/

/*
//DELAY (S) DESDE QUE SE INICIALIZA UN NODO HASTA QUE SE ENVÍAN LOS MENSAJES HELLO A LOS VECINOS
//RANGO = [IOTORII_CONF_HELLO_START_TIME/2 IOTORII_CONF_HELLO_START_TIME]
#define IOTORII_CONF_HELLO_START_TIME 4

//DELAY (S) DESDE QUE SE INICIALIZA EL NODO ROOT HASTA QUE SE ENVÍA EL PRIMER MENSAJE SETHLMAC A LOS VECINOS
//SE IMPRIMER LOS PRIMEROS LOGS DE ESTADÍSTICAS
#define IOTORII_CONF_SETHLMAC_START_TIME 5

//DELAY DESDE QUE SE INICIALIZA UN NODO COMÚN HASTA QUE SE ENVÍA MENSAJE SETHLMAC A LOS VECINOS
//RANGO = [IOTORII_CONF_SETHLMAC_DELAY/2 IOTORII_CONF_SETHLMAC_DELAY]
#define IOTORII_CONF_SETHLMAC_DELAY 5

//DELAY DE PRIMERAS STADÍSTICAS 
#define IOTORII_CONF_STATISTICS1_TIME 10

//DELAY DE LAS DEMÁS ESTADÍSTICAS
#define IOTORII_CONF_STATISTICS2_TIME 5
*/

/*---------------------------------------------------------------------------*/

/* Configure the csma_driver for netstack.h */
#ifndef IOTORII_CONF_NODE_TYPE
#define IOTORII_CONF_NODE_TYPE 1 //POR DEFECTO NODO ROOT
#endif

#if IOTORII_CONF_NODE_TYPE > 0 //PARA NODO COMÚN O ROOT
#define NETSTACK_CONF_MAC      iotorii_csma_driver

#elif IOTORII_CONF_NODE_TYPE == 0 //SI OPERACIÓN MAC TRADICIONAL
#define NETSTACK_CONF_MAC      csma_driver
#endif

// #ifndef IOTORII_CONF_IPV6
//     #define IOTORII_CONF_IPV6 0 //POR DEFECTO NO USA IPv6
//     // #include "net/ipv6/multicast/uip-mcast6-engines.h"
//     // #ifndef UIP_MCAST6_CONF_ENGINE
//     //     #define UIP_MCAST6_CONF_ENGINE UIP_MCAST6_ENGINE_MPL
//     // #endif
// #endif

#ifndef IOTORII_CONF_NRF52840_LOG
    #define IOTORII_CONF_NRF52840_LOG 1 //POR DEFECTO USA LOG PARA NRF52840-DK
#endif

/*---------------------------------------------------------------------------*/
/* Configure the routing_driver for netstack.h */
//#define NETSTACK_CONF_ROUTING      nullrouting_driver
/*---------------------------------------------------------------------------*/

#endif /* PROJECT_CONF_H_ */
