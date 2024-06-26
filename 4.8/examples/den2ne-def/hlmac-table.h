

#ifndef HLMACTABLE_H_
#define HLMACTABLE_H_

#include "contiki.h"
#include "hlmacaddr.h"
//#include "lib/list.h"
//#include <stdio.h>

#ifndef HLMAC_CONF_ADD_HLMAC
#define HLMAC_ADD_HLMAC HLMAC_FIRST_HLMAC //HLMAC_FIRST_HLMAC O HLMAC_SHORTEST_HLMAC
#else
#define HLMAC_ADD_HLMAC HLMAC_CONF_ADD_HLMAC
#endif

/*---------------------------------------------------------------------------*/

struct hlmac_table_entry
{
	struct hlmac_table_entry *next;
	hlmacaddr_t address;
	uint64_t timestamp;
};

typedef struct hlmac_table_entry hlmac_table_entry_t;


/*---------------------------------------------------------------------------*/


//INICIALIZA LA TABLA DE DIRECCIONES HLMAC
void hlmac_table_init (void);

//AÑADE UNA DIRECCIÓN HLMAC AL FINAL DE LA TABLA
uint8_t hlmactable_add (const hlmacaddr_t addr, uint64_t timestamp);

//ELIMINA TODAS LAS DIRECCIONES HLMAC DE LA TABLA
uint8_t hlmactable_clean (void);

//ELIMINA LA DIRECCIÓN HLMAC AL FINAL DE LA TABLA
uint8_t hlmactable_chop (void);

//COMPRUEBA SI LA DIRECCION HLMAC DADA CREA UN BUCLE O NO
//DEVUELVE UN 1 SI HAY BUCLE Y 0 SI NO
uint8_t hlmactable_has_loop (const hlmacaddr_t addr);

//ENCUENTRA EL PREFIJO MAS LARGO DADO PARA UNA DIRECCIÓN HLMAC
//DEVUELVE EL PREFIJO SI EXISTE Y SI NO "UNSPECIFIED"
hlmacaddr_t* hlmactable_get_longest_matchhed_prefix (const hlmacaddr_t addr);


//DEVUELVE EL NÚMERO DE SALTOS
#if LOG_DBG_STATISTIC == 1
int hlmactable_calculate_sum_hop (void);
#endif

#endif /* HLMACTABLE_H_ */
