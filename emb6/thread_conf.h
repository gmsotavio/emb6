/*
 * thread_conf.h
 *
 *  Created on: 23 May 2016
 *  Author: Lukas Zimmermann <lzimmer1@stud.hs-offenburg.de>
 */

#ifndef EMB6_THREAD_CONF_H_
#define EMB6_THREAD_CONF_H_

#include "emb6.h"

/*=============================================================================
                                    DEVICE TYPE
===============================================================================*/

#define THRD_DEV_TYPE			THRD_DEV_TYPE_ROUTER		// Router.
#define THRD_DEV_FUNC			THRD_DEV_FUNC_FFD			// FFD.

/**
 * Thread Device Types.
 */
typedef enum
{
	THRD_DEV_TYPE_NONE = 0x00,	 //!< DEV_TYPE_NONE
	THRD_DEV_TYPE_END = 0x01,   //!< DEV_TYPE_END
	THRD_DEV_TYPE_REED = 0x02,  //!< DEV_TYPE_REED
	THRD_DEV_TYPE_ROUTER = 0x03,//!< DEV_TYPE_ROUTER
	THRD_DEV_TYPE_LEADER = 0x04 //!< DEV_TYPE_LEADER
} thrd_dev_type_t;

/**
 * Thread Device Functionalty (RFD or FFD).
 */
typedef enum
{
	THRD_DEV_FUNC_RFD = 0,//!< THRD_DEV_RFD
	THRD_DEV_FUNC_FFD = 1 //!< THRD_DEV_FFD
} thrd_dev_funct_t;

/**
 * Thread Device Types and RFD/FFD.
 */
typedef struct
{
	uint8_t type;	// thrd_dev_type_t.
	uint8_t isFFD;
	uint8_t isRX_off_when_idle;
#if ( (THRD_DEV_TYPE == THRD_DEV_TYPE_ROUTER) || (THRD_DEV_TYPE == THRD_DEV_TYPE_REED) )
	uint8_t Router_ID;	// Router ID.
#endif
} thrd_dev_t;

/*! Thread Device Type Configuration. */
extern thrd_dev_t thrd_dev;

// ---------------- THREAD IPv6 ADRESSES ------------------------
#define THRD_CREATE_RLOC_ADDR(ipaddr, rloc16)	uip_ip6addr(ipaddr, 0xaaaa, 0x0000, 0x0000, 0x0000, 0x0000, 0x00ff, 0xfe00, rloc16);

#define THRD_LINK_LOCAL_ALL_NODES_ADDR(ipaddr)	uip_ip6addr(ipaddr, 0xff02, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001)
#define THRD_LINK_LOCAL_ALL_ROUTERS_ADDR(ipaddr)	uip_ip6addr(ipaddr, 0xff02, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0002)
#define THRD_REALM_LOCAL_ALL_NODES_ADDR(ipaddr)	uip_ip6addr(ipaddr, 0xff03, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001)
#define THRD_REALM_LOCAL_ALL_ROUTERS_ADDR(ipaddr)	uip_ip6addr(ipaddr, 0xff03, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0002)
#define THRD_REALM_LOCAL_ALL_MPL_FORWARDERS_ADDR(ipaddr)	uip_ip6addr(ipaddr, 0xff03, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00fc)

// ----------------- IANA CONSIDERATIONS ------------------------

#define THRD_MGMT_COAP_PORT				19789 // UIP_HTONS(19789)		// Thread Network Management (:MM).

/*=============================================================================
                                NETWORK LAYER SECTION
===============================================================================*/

// --------------- THREAD ROUTING PROTOCOL ----------------------


/** Routing table size */
#define THRD_CONF_MAX_ROUTES            5 // 10

/** Exponentially weighted moving average. */
#define EXP_WEIGHT_MOV_AVG_1_8			3	// 1/8 (shifting bits).
#define EXP_WEIGHT_MOV_AVG_1_16			4	// 1/16 (shifting bits).
/** Set the weight. */
#define THRD_EXP_WEIGHT_MOV_AVG			EXP_WEIGHT_MOV_AVG_1_8

/** Network Layer defines. */
#define	ADVERTISEMENT_I_MIN				1		// 1 sec.
#define	ADVERTISEMENT_I_MAX				5		// 2⁵ = 32.
#define	ID_REUSE_DELAY					100
#define	ID_SEQUENCE_PERIOD				10
#define	MAX_NEIGHBOR_AGE				100
#define	MAX_ROUTE_COST					16
#define	MAX_ROUTER_ID					5 // 62
#define	MAX_ROUTERS						5 // 32
#define	MIN_DOWNGRADE_NEIGHBORS			7
#define	NETWORK_ID_TIMEOUT				120
#define	PARENT_ROUTE_TO_LEADER_TIMEOUT	20
#define	ROUTER_SELECTION_JITTER			120
#define	ROUTER_DOWNGRADE_TRESHOLD		23
#define	ROUTER_UPGRADE_TRESHOLD			16
#define	INFINITE_COST_TIMEOUT			90
#define	REED_ADVERTISEMENT_INTERVAL		570
#define	REED_ADVERTISEMENT_MAX_JITTER	60

/** Network Layer TLVs. */
#define THRD_NET_TLV_MAX_NUM			10		// Maximum number of network layer TLVs.
#define THRD_NET_TLV_MAX_SIZE			50		// Maximum size of network layer TLVs.

// --------------- THREAD EID-to-RLOC Mapping -------------------

#define THRD_MAX_LOCAL_ADDRESSES		10		// Maximum number of local addresses (Local Address Set).
#define THRD_MAX_RFD_CHILD_ADDRESSES	10		// Maximum number of RFD Child Addresses (RFD Child Address Set).
#define THRD_MAX_ADDRESS_QUERIES		32		// Maximum number of Address Queries (Address Query Set).

// --------------- THREAD EID-to-RLOC Map Cache -----------------

#define THRD_MAX_EID_RLOC_MAP_CACHE_SIZE	10		// Maximum number of EID-to-RLOC Map Cache entries.

// --------------------------------------------------------------

#endif /* EMB6_THREAD_CONF_H_ */
