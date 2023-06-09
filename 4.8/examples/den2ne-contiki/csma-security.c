

#include "contiki.h"

// #include "net/mac/csma/csma.h"
// #include "net/mac/csma/anti-replay.h"
// #include "net/mac/csma/csma-security.h"
#include "iotorii-csma.h"
#include "anti-replay.h"
#include "csma-security.h"

#include "net/mac/framer/frame802154.h"
#include "net/mac/framer/framer-802154.h"
#include "net/mac/llsec802154.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "lib/ccm-star.h"
#include "lib/aes-128.h"
#include <stdio.h>
#include <string.h>
#include "ccm-star-packetbuf.h"

/* Log configuration */
#include "sys/log.h"

/*---------------------------------------------------------------------------*/

#define LOG_MODULE "CSMA"
#define LOG_LEVEL LOG_LEVEL_MAC

#if LOG_LEVEL == LOG_LEVEL_DBG
static const char * HEX = "0123456789ABCDEF";
#endif

#if LLSEC802154_USES_AUX_HEADER && LLSEC802154_USES_FRAME_COUNTER

#define MIC_LEN(level) LLSEC802154_MIC_LEN(level)

#if LLSEC802154_USES_EXPLICIT_KEYS
#define LLSEC_KEY_INDEX (FRAME802154_IMPLICIT_KEY == packetbuf_attr(PACKETBUF_ATTR_KEY_ID_MODE) \
                          ? 0 \
                          : packetbuf_attr(PACKETBUF_ATTR_KEY_INDEX))
#define LLSEC_KEY_MODE (packetbuf_attr(PACKETBUF_ATTR_KEY_ID_MODE))
#else
#define LLSEC_KEY_INDEX (0)
#define LLSEC_KEY_MODE (FRAME802154_IMPLICIT_KEY)
#endif /* LLSEC802154_USES_EXPLICIT_KEYS */

/*---------------------------------------------------------------------------*/

typedef struct //KEYS FOR LLSEC FOR CSMA
{
	uint8_t u8[16];
} aes_key_t;

static aes_key_t keys[CSMA_LLSEC_MAXKEYS];

/*---------------------------------------------------------------------------*/

int csma_security_set_key (uint8_t index, const uint8_t *key) //SE ASUME QUE ES DE 16 BYTES
{
	if(key != NULL && index < CSMA_LLSEC_MAXKEYS) 
	{
		memcpy(keys[index].u8, key, 16);
		return 1;
	}
	return 0;
}

#define N_KEYS (sizeof(keys) / sizeof(aes_key))


static int aead (uint8_t hdrlen, int forward)
{
	uint8_t totlen;
	uint8_t nonce[CCM_STAR_NONCE_LENGTH];
	uint8_t *m;
	uint8_t m_len;
	uint8_t *a;
	uint8_t a_len;
	uint8_t *result;
	
	/* Allocate for MAX level */
	uint8_t generated_mic[MIC_LEN(7)];
	uint8_t *mic;
	uint8_t key_index;
	aes_key_t *key;
	uint8_t with_encryption;

	key_index = LLSEC_KEY_INDEX;
	
	if (key_index >= CSMA_LLSEC_MAXKEYS) 
	{
		LOG_ERR("Key not available: %u\n", key_index);
		return 0;
	}

	key = &keys[key_index];

	ccm_star_packetbuf_set_nonce (nonce, forward);
	totlen = packetbuf_totlen();
	a = packetbuf_hdrptr();

	with_encryption = (packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) & 0x4) ? 1 : 0;

	if (with_encryption) 
	{
		a_len = hdrlen;
		m = a + a_len;
		m_len = totlen - hdrlen;
	} 
	else 
	{
		a_len = totlen;
		m = NULL;
		m_len = 0;
	}

	mic = a + totlen;
	result = forward ? mic : generated_mic;

	CCM_STAR.set_key(key->u8);
	CCM_STAR.aead(nonce, m, m_len, a, a_len, result, MIC_LEN(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) & 0x07), forward);

	if (forward) 
	{
		packetbuf_set_datalen(packetbuf_datalen() + MIC_LEN(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) & 0x07));
		return 1;
	} 
	else 
		return (memcmp(generated_mic, mic, MIC_LEN(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) & 0x07)) == 0);
}


int csma_security_create_frame (void)
{
	int hdr_len;

	packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);

	if (packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) > 0 && LLSEC_KEY_INDEX != 0xffff) 
		anti_replay_set_counter();

	hdr_len = NETSTACK_FRAMER.create();

	if (hdr_len < 0) 
		return hdr_len;

	if (packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) > 0) 
	{
		#if LOG_LEVEL == LOG_LEVEL_DBG
		int i = 0;
		uint8_t *p;
		
		LOG_DBG("  Payload before (%d):", packetbuf_totlen());
		p = packetbuf_hdrptr();
		
		for (i = 0; i < packetbuf_totlen(); i++) 
		{
			LOG_DBG_("%c%c", HEX[(p[i] >> 4) & 0x0f], HEX[p[i] & 0x0f]);
		}
		LOG_DBG("\n");
		#endif

		if (!aead(hdr_len, 1)) 
		{
			LOG_ERR("failed to encrypt packet to ");
			LOG_ERR_LLADDR (packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
			LOG_ERR_("\n");
			return FRAMER_FAILED;
		}
		
		LOG_INFO("LLSEC-OUT:");
		LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
		LOG_INFO_(" ");
		LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
		LOG_INFO_(" %u (%u) LV:%d, KEY:0x%02x\n", packetbuf_datalen(), packetbuf_totlen(), packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL), LLSEC_KEY_INDEX);

		#if LOG_LEVEL == LOG_LEVEL_DBG
		LOG_DBG("  Payload after: (%d)", packetbuf_totlen());
		p = packetbuf_hdrptr();
		
		for (i = 0; i < packetbuf_totlen(); i++) 
		{
			LOG_DBG_("%c%c", HEX[(p[i] >> 4) & 0x0f], HEX[p[i] & 0x0f]);
		}
		LOG_DBG_("\n");
		#endif
	}
	return hdr_len;
}


int csma_security_frame_len(void)
{
	if (packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) > 0 && LLSEC_KEY_INDEX != 0xffff) 
		return NETSTACK_FRAMER.length() + MIC_LEN(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) & 0x07);

	return NETSTACK_FRAMER.length();
}


int csma_security_parse_frame(void)
{
	int hdr_len;
	hdr_len = NETSTACK_FRAMER.parse();
	
	if (hdr_len < 0) 
		return hdr_len;

	if (packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) == 0) //SI NO HAY SEGURIDAD NO SE REQUIERE PROCESAMIENTO
		return hdr_len;

	LOG_INFO("LLSEC-IN: ");
	LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
	LOG_INFO_(" ");
	LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
	LOG_INFO_(" %d %u (%u) LV:%d KM:%d KEY:0x%02x\n", hdr_len, packetbuf_datalen(),	packetbuf_totlen(), packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL), LLSEC_KEY_MODE, LLSEC_KEY_INDEX);

	if (packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) != CSMA_LLSEC_SECURITY_LEVEL) //ERROR CON EL NIVEL DE SEGURIDAD
	{
		LOG_INFO("received frame with wrong security level (%u) from ", packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL));
		LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
		LOG_INFO_("\n");
		return FRAMER_FAILED;
	}

	if (LLSEC_KEY_MODE != CSMA_LLSEC_KEY_ID_MODE) 
	{
		LOG_INFO("received frame with wrong key id mode (%u) from ", LLSEC_KEY_MODE);
		LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
		LOG_INFO("\n");
		return FRAMER_FAILED;
	}

	if (linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_SENDER), &linkaddr_node_addr)) 
	{
		LOG_INFO("frame from ourselves\n");
		return FRAMER_FAILED;
	}

	if (packetbuf_datalen() <= MIC_LEN(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) & 0x07)) //POCO TAMAÑO
	{
		LOG_ERR("MIC error - too little data in frame!\n");
		return FRAMER_FAILED;
	}

	packetbuf_set_datalen(packetbuf_datalen() - MIC_LEN(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) & 0x07));
	
	if (!aead(hdr_len, 0)) 
	{
		LOG_INFO("received unauthentic frame %u from ", (unsigned int) anti_replay_get_counter());
		LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
		LOG_INFO_("\n");
		return FRAMER_FAILED;
	}

	return hdr_len; //ANTI REPLAY PROTECTION
}

#else //VERSIONES CREATE FRAME Y PARSE FRAME INSEGURAS -->

int csma_security_create_frame(void) 
{
	packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
	return NETSTACK_FRAMER.create();
}


int csma_security_parse_frame(void)
{
	return NETSTACK_FRAMER.parse();
}

#endif /* LLSEC802154_USES_AUX_HEADER && LLSEC802154_USES_FRAME_COUNTER */


