

#include "contiki.h"
#include "hlmac-table.h"
//#include <stdio.h> //For sprintf()
#include <stdlib.h> //For malloc()
#include "lib/list.h"
#include "sys/log.h"

/*---------------------------------------------------------------------------*/

#define LOG_MODULE "IoTorii-HLMAC-Table"
#define LOG_LEVEL LOG_LEVEL_MAC

LIST(hlmac_table_entry_list);

/*---------------------------------------------------------------------------*/

uint8_t number_of_hlmac_addresses;

void hlmac_table_init(void)
{
	number_of_hlmac_addresses = 0;
}


uint8_t hlmactable_add (const hlmacaddr_t addr, uint64_t timestamp)
{
	if (number_of_hlmac_addresses>0) //COMPRUEBA SI HAY UNA ENTRADA PARA COMPROBAR QUÉ TIMESTAMP ES MÁS RECIENTE
	{
		hlmac_table_entry_t* entry = list_head (hlmac_table_entry_list);
		if (timestamp > entry->timestamp)
			hlmactable_clean(); //ELIMINA TODA LA TABLA
	}

	if (number_of_hlmac_addresses >= 255) //SI LA TABLA ESTÁ LLENA (255 DIRECCIONES)
	{ 
		#if LOG_DBG_DEVELOPER == 1
		LOG_DBG("Number of HLMAC addresses: %d, table is full.\n", number_of_hlmac_addresses);
		#endif
		return 0;
	}

#if HLMAC_ADD_HLMAC == HLMAC_FIRST_HLMAC
	if ( (HLMAC_MAX_HLMAC == -1) || ((HLMAC_MAX_HLMAC != -1)&&(number_of_hlmac_addresses < HLMAC_MAX_HLMAC)) ) //SI NO SE HA SUPERADO LA CAPACIDAD DE LA TABLA
	{
		hlmac_table_entry_t* entry = (hlmac_table_entry_t*) malloc (sizeof(hlmac_table_entry_t)); //SE RESERVA ESPACIO
		entry->address = addr; //SE ASIGNA LA DIRECCIÓN DADA
		
		entry->timestamp=timestamp;
		
		list_add (hlmac_table_entry_list, entry);

		number_of_hlmac_addresses++; //SE AUMENTA LA VARIABLE

		#if LOG_DBG_DEVELOPER == 1 || LOG_DBG_STATISTIC == 1
		
		char *addr_str = hlmac_addr_to_str(addr); //SE CONVIERTE LA DIRECCION A STRING PARA EL PRINTF
		// printf("Periodic Statistics: HLMAC address: %s saved to HLMAC table.\n\r", addr_str);			///////
		
		free(addr_str); //SE LIMPIA LA VARIABLE
		addr_str = NULL;

		LOG_DBG("Number of HLMAC address: %d saved to HLMAC table.\n", number_of_hlmac_addresses);
		#endif

		return 1;
    }	
	else //SI SE HA SUPERADO LA CAPACIDAD DE LA TABLA Y NO SE PUEDE GUARDAR LA DIRECCIÓN
	{

		#if LOG_DBG_DEVELOPER == 1
		char *addr_str = hlmac_addr_to_str(addr); //SE CONVIERTE LA DIRECCION A STRING PARA EL PRINTF
		LOG_DBG("HLMAC address %s is not saved to HLMAC table, MAX_HLMAC: %d, number of entries: %d, \n", addr_str, HLMAC_MAX_HLMAC, number_of_hlmac_addresses);
		free(addr_str);
		addr_str = NULL;

		LOG_DBG("Number of HLMAC address: %d saved to HLMAC table.\n", number_of_hlmac_addresses);
		#endif

		return 0;
	}
#else
	//CÓDIGO PARA AÑADIR SHORTEST HLMAC RECIBIDA
#endif
}

uint8_t hlmactable_clean (void)
{
	hlmac_table_entry_t* entry = list_head (hlmac_table_entry_list), *entry_remove = list_head (hlmac_table_entry_list);
	while(entry != NULL){
		entry = entry->next;
		free(entry_remove);
		entry_remove = entry;
	}
	*hlmac_table_entry_list = NULL;
	number_of_hlmac_addresses = 0;
	return 1;
}

uint8_t hlmactable_chop (void)
{
	free(list_chop (hlmac_table_entry_list));
	number_of_hlmac_addresses--; //SE DISMINUYE LA VARIABLE
	return 1;
}

uint8_t hlmactable_has_loop (const hlmacaddr_t addr)
{
	hlmacaddr_t *longest_prefix = hlmactable_get_longest_matchhed_prefix(addr); //ENCUENTRA EL PREFIJO MAS LARGO PARA LA DIRECCIÓN

	#if LOG_DBG_DEVELOPER == 1
	char *addr_str = hlmac_addr_to_str(addr);
	char *pref_str = hlmac_addr_to_str(*longest_prefix);
	LOG_DBG("Check Loop: HLMAC address: %s, Longest Prefix: %s \n", addr_str, pref_str);
	free(pref_str);
	pref_str = NULL;
	#endif


	if (hlmac_is_unspecified_addr(*longest_prefix)) //SI NO CREA BUCLE
	{
		#if LOG_DBG_DEVELOPER == 1
		LOG_DBG("HLMAC address %s doesn't create a loop\n", addr_str);
		free(addr_str);
		addr_str = NULL;
		#endif
		
		free(longest_prefix->address);
		longest_prefix->address = NULL;
		
		free(longest_prefix);
		longest_prefix = NULL;
		return 0;
	}
	else //SI CREA BUCLE
	{
		#if LOG_DBG_DEVELOPER == 1
		LOG_DBG("HLMAC address %s creates a loop\n", addr_str);
		free(addr_str);
		addr_str = NULL;
		#endif
		
		free(longest_prefix->address);
		longest_prefix->address = NULL;
		
		free(longest_prefix);
		longest_prefix = NULL;
		return 1;
	}
}


hlmacaddr_t* hlmactable_get_longest_matchhed_prefix (const hlmacaddr_t address)
{
	//if (list_length(hlmac_table_entry_list) == 0) //return UNSPECIFIED_HLMAC_ADDRESS;

	hlmacaddr_t* addr = (hlmacaddr_t*) malloc (sizeof(hlmacaddr_t));
	addr->address = (uint8_t*) malloc (sizeof(uint8_t)* address.len);
	//memcpy(addr->address, address.address, address.len);
	
	uint8_t i;
	
	for (i = 0; i < address.len; i++)
	{
		addr->address[i] = address.address[i]; //SE ASIGNA LA DIRECCION DADA 
	}
	
	addr->len = address.len;

	hlmac_remove_Last_id(addr); //LAS DIRECCIONES NO PUEDEN SER PREFIJO DE SÍ MISMAS

	hlmac_table_entry_t *table_entry;

	while (hlmac_get_len(*addr) > 0) //SE BUSCA LA DIRECCIÓN EN LA TABLA MIENTRAS LA LONGITUD NO SEA NULA
	{
		for (table_entry = list_head(hlmac_table_entry_list); table_entry != NULL; table_entry = table_entry->next)
		{
			if (hlmac_cmp(table_entry->address, *addr) == 0) //SI SON IGUALES
				return addr;
		}
		hlmac_remove_Last_id(addr); //SE BORRA
	}

	//return UNSPECIFIED_HLMAC_ADDRESS;
	return addr; //PUEDE DEVOLVER LA DIRECCION O "UNSPECIFIED"
}


#if LOG_DBG_STATISTIC == 1
int hlmactable_calculate_sum_hop (void)
{
	int sum = 0;
	hlmac_table_entry_t *table_entry;
	
	for (table_entry = list_head(hlmac_table_entry_list); table_entry != NULL; table_entry = table_entry->next)
	{
		sum += table_entry->address.len; //SE AÑADE AL NÚMERO DE SALTOS LA LONGITUD
	}
	
	return sum;
}
#endif
