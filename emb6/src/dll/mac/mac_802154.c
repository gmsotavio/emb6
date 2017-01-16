/*
 * emb6 is licensed under the 3-clause BSD license. This license gives everyone
 * the right to use and distribute the code, either in binary or source code
 * format, as long as the copyright license is retained in the source code.
 *
 * The emb6 is derived from the Contiki OS platform with the explicit approval
 * from Adam Dunkels. However, emb6 is made independent from the OS through the
 * removal of protothreads. In addition, APIs are made more flexible to gain
 * more adaptivity during run-time.
 *
 * The license text is:
 *
 * Copyright (c) 2015,
 * Hochschule Offenburg, University of Applied Sciences
 * Laboratory Embedded Systems and Communications Electronics.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*============================================================================*/

/**
 * @file    mac_802154.c
 * @date    19.10.2015
 * @author  PN
 */


/*
********************************************************************************
*                                   INCLUDES
********************************************************************************
*/
#include "emb6.h"

#include "evproc.h"
#include "framer_802154.h"
#include "packetbuf.h"
#include "random.h"
#include "rt_tmr.h"


#define     LOGGER_ENABLE        LOGGER_MAC
#include    "logger.h"


/*
********************************************************************************
*                               LOCAL MACROS
********************************************************************************
*/
/* CSMA unit backoff coefficient */
#define MAC_CSMA_UNIT_BACKOFF_COE               ( 20u )


/*
********************************************************************************
*                          LOCAL FUNCTION DECLARATIONS
********************************************************************************
*/
static void mac_init(void *p_netstk, e_nsErr_t *p_err);
static void mac_on(e_nsErr_t *p_err);
static void mac_off(e_nsErr_t *p_err);
static void mac_send(uint8_t *p_data, uint16_t len, e_nsErr_t *p_err);
static void mac_recv(uint8_t *p_data, uint16_t len, e_nsErr_t *p_err);
static void mac_ioctl(e_nsIocCmd_t cmd, void *p_val, e_nsErr_t *p_err);
static void mac_csma(e_nsErr_t *p_err);


/*
********************************************************************************
*                               LOCAL VARIABLES
********************************************************************************
*/
static s_ns_t          *pmac_netstk;
static void            *pmac_cbTxArg;
static nsTxCbFnct_t     mac_cbTxFnct;


/*
********************************************************************************
*                               GLOBAL VARIABLES
********************************************************************************
*/
const s_nsMAC_t mac_driver_802154 =
{
 "MAC 802154",
  mac_init,
  mac_on,
  mac_off,
  mac_send,
  mac_recv,
  mac_ioctl,
};

extern uip_lladdr_t uip_lladdr;


/*
********************************************************************************
*                           LOCAL FUNCTION DEFINITIONS
********************************************************************************
*/

/**
 * @brief   Initialize driver
 *
 * @param   p_netstk    Pointer to netstack structure
 * @param   p_err       Pointer to a variable storing returned error code
 */
void mac_init(void *p_netstk, e_nsErr_t *p_err)
{
#if NETSTK_CFG_ARG_CHK_EN
  if (p_err == NULL) {
    return;
  }

  if (p_netstk == NULL) {
    *p_err = NETSTK_ERR_INVALID_ARGUMENT;
    return;
  }
#endif

  /* initialize local variables */
  pmac_netstk = (s_ns_t *) p_netstk;
  mac_cbTxFnct = 0;
  pmac_cbTxArg = NULL;

  /*
   * Configure stack address
   */
  memcpy(&uip_lladdr.addr, &mac_phy_config.mac_address, 8);
  linkaddr_set_node_addr((linkaddr_t *) mac_phy_config.mac_address);

  /* initialize MAC PIB attributes */
  packetbuf_attr_t macAckWaitDuration;
  packetbuf_attr_t macUnitBackoffPeriod;
  packetbuf_attr_t phyTurnaroundTime;
  packetbuf_attr_t phySHRDuration;
  packetbuf_attr_t phySymbolsPerOctet;
  packetbuf_attr_t phySymbolPeriod;

  phySHRDuration = packetbuf_attr(PACKETBUF_ATTR_PHY_SHR_DURATION);
  phyTurnaroundTime = packetbuf_attr(PACKETBUF_ATTR_PHY_TURNAROUND_TIME);
  phySymbolsPerOctet = packetbuf_attr(PACKETBUF_ATTR_PHY_SYMBOLS_PER_OCTET);
  phySymbolPeriod = packetbuf_attr(PACKETBUF_ATTR_PHY_SYMBOL_PERIOD);

  macUnitBackoffPeriod = 20 * phySymbolPeriod;
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_UNIT_BACKOFF_PERIOD, macUnitBackoffPeriod);

  /* compute and set macAckWaitDuration attribute, see IEEE Std. 802.15.4(g) */
  macAckWaitDuration = macUnitBackoffPeriod + phyTurnaroundTime + phySHRDuration;
#if (NETSTK_CFG_MR_FSK_PHY_EN == TRUE)
  macAckWaitDuration += 9 * phySymbolsPerOctet * phySymbolPeriod;
#else
  macAckWaitDuration += 6 * phySymbolsPerOctet * phySymbolPeriod;
#endif /* NETSTK_CFG_MR_FSK_PHY_EN */

  packetbuf_set_attr(PACKETBUF_ATTR_MAC_ACK_WAIT_DURATION, macAckWaitDuration);

  /* set returned error */
  *p_err = NETSTK_ERR_NONE;
}


/**
 * @brief   Turn driver on
 *
 * @param   p_err       Pointer to a variable storing returned error code
 */
void mac_on(e_nsErr_t *p_err)
{
#if NETSTK_CFG_ARG_CHK_EN
  if (p_err == NULL) {
    return;
  }
#endif

  pmac_netstk->phy->on(p_err);
}


/**
 * @brief   Turn driver off
 *
 * @param   p_err       Pointer to a variable storing returned error code
 */
void mac_off(e_nsErr_t *p_err)
{
#if NETSTK_CFG_ARG_CHK_EN
  if (p_err == NULL) {
    return;
  }
#endif

  pmac_netstk->phy->off(p_err);
}


/**
 * @brief   Frame transmission handler
 *
 * @param   p_data      Pointer to buffer holding frame to send
 * @param   len         Length of frame to send
 * @param   p_err       Pointer to a variable storing returned error code
 */
void mac_send(uint8_t *p_data, uint16_t len, e_nsErr_t *p_err)
{
#if NETSTK_CFG_ARG_CHK_EN
  if (p_err == NULL) {
    return;
  }

  if ((len == 0) || (p_data == NULL)) {
    *p_err = NETSTK_ERR_INVALID_ARGUMENT;
    return;
  }
#endif

  uint8_t isTxDone;
  uint8_t numTxRetries;
  uint8_t numRetriesMax;
  uint8_t isAckReq;

  /*
   * Transmission handling
   */
  isTxDone = FALSE;
  numTxRetries = 0;
  numRetriesMax = 4;
  isAckReq = packetbuf_attr(PACKETBUF_ATTR_MAC_ACK);
  trace_printf("MAC_TX: %02x %d %d", p_data[2], isAckReq, numRetriesMax);

  /* iterates retransmission as long as number of retries does not exceed
  * the maximum retries and transmission is not completed */
  while ((numTxRetries < numRetriesMax) && (isTxDone == FALSE)) {
    /* issue transmission request following successful CSMA-CA */
    mac_csma(p_err);
    if (*p_err == NETSTK_ERR_NONE) {
      /* then attempt to transmit the packet */
      pmac_netstk->phy->send(p_data, len, p_err);
    }

    /* retransmission? */
    if ((*p_err == NETSTK_ERR_TX_COLLISION) ||
        ((isAckReq == TRUE) && (*p_err != NETSTK_ERR_NONE))) {
      /* then increase number of retries */
      numTxRetries++;
    }
    else {
      isTxDone = TRUE;
    }
  }
  trace_printf("MAC_TX: done e=-%d r=%d", *p_err, numTxRetries);

  /* was transmission callback function set? */
  if (mac_cbTxFnct) {
    /* then signal the upper layer of the result of transmission process */
    mac_cbTxFnct(pmac_cbTxArg, p_err);
  }
}


/**
 * @brief   Frame reception handler
 *
 * @param   p_data      Pointer to buffer holding frame to receive
 * @param   len         Length of frame to receive
 * @param   p_err       Pointer to a variable storing returned error code
 */
void mac_recv(uint8_t *p_data, uint16_t len, e_nsErr_t *p_err)
{
#if NETSTK_CFG_ARG_CHK_EN
  if (p_err == NULL) {
    return;
  }

  if ((len == 0) || (p_data == NULL)) {
    *p_err = NETSTK_ERR_INVALID_ARGUMENT;
    return;
  }
#endif

  int hdrlen;
  frame802154_t frame;

  /* set returned error code to default */
  *p_err = NETSTK_ERR_NONE;

  /* was packet length larger than size of packet buffer? */
  if (len > PACKETBUF_SIZE) {
    /* then discard the packet to avoid buffer overflow when using memcpy to
    * store the frame into the packet buffer
    */
    *p_err = NETSTK_ERR_INVALID_FRAME;
    TRACE_LOG_ERR("MAC_RX: invalid length");
    return;
  }

  /* Parsing but not reducing header as that will be then handled by DLLC */
  hdrlen = frame802154_parse(p_data, len, &frame);
  if (hdrlen == 0) {
    *p_err = NETSTK_ERR_INVALID_FRAME;
    TRACE_LOG_ERR("MAC_RX: bad format");
    return;
  }

  switch (frame.fcf.frame_type) {
    case FRAME802154_DATAFRAME:
    case FRAME802154_CMDFRAME:
      /* signal upper layer of the received packet */
      pmac_netstk->dllc->recv(p_data, len, p_err);
      break;

    case FRAME802154_ACKFRAME:
      /* silently discard unwanted ACK */
      *p_err = NETSTK_ERR_NONE;
      break;

    default:
      *p_err = NETSTK_ERR_INVALID_FRAME;
      break;
  }
  LOG_INFO("MAC_RX: Received %d bytes.", len);
}


/**
 * @brief    Miscellaneous commands handler
 *
 * @param   cmd         Command to be issued
 * @param   p_val       Pointer to a variable related to the command
 * @param   p_err       Pointer to a variable storing returned error code
 */
void mac_ioctl(e_nsIocCmd_t cmd, void *p_val, e_nsErr_t *p_err)
{
#if NETSTK_CFG_ARG_CHK_EN
  if (p_err == NULL) {
    return;
  }
#endif

  *p_err = NETSTK_ERR_NONE;
  switch (cmd) {
    case NETSTK_CMD_TX_CBFNCT_SET:
      if (p_val == NULL) {
        *p_err = NETSTK_ERR_INVALID_ARGUMENT;
      } else {
        mac_cbTxFnct = (nsTxCbFnct_t) p_val;
      }
      break;

    case NETSTK_CMD_TX_CBARG_SET:
      pmac_cbTxArg = p_val;
      break;

    default:
      pmac_netstk->phy->ioctrl(cmd, p_val, p_err);
      break;
  }
}


/**
 * @brief   This function performs CSMA-CA mechanism.
 *
 * @param   p_err   Pointer to a variable storing returned error code
 */
static void mac_csma(e_nsErr_t *p_err)
{
  uint32_t delay = 0;
  uint32_t max_random;
  uint32_t unit_backoff;
  uint8_t nb;
  uint8_t be;

  /* initialize CSMA variables */
  nb = 0;
  be = NETSTK_CFG_CSMA_MIN_BE;
  unit_backoff = MAC_CSMA_UNIT_BACKOFF_COE * packetbuf_attr(PACKETBUF_ATTR_PHY_SYMBOL_PERIOD);
  *p_err = NETSTK_ERR_NONE;

  /* perform CCA maximum MaxBackoff time */
  while (nb <= NETSTK_CFG_CSMA_MAX_BACKOFF) {
    /* delay for random (2^BE - 1) unit backoff periods */
    max_random = (1 << be) - 1;
    delay = bsp_getrand(0, max_random) * unit_backoff;
    bsp_delayUs(delay);

    /* perform CCA */
    pmac_netstk->phy->ioctrl(NETSTK_CMD_RF_CCA_GET, 0, p_err);
    /* was channel free or was the radio busy? */
    if (*p_err == NETSTK_ERR_NONE) {
      /* channel free */
      break;
    }
    else {
      /* then increase number of backoff by one */
      nb++;
      /* be = MIN((be + 1), MaxBE) */
      be = ((be + 1) < NETSTK_CFG_CSMA_MAX_BE) ? (be + 1) : (NETSTK_CFG_CSMA_MAX_BE);
    }
  }
  LOG_INFO("MAC_TX: NB %d.", nb);
}


/*
********************************************************************************
*                               END OF FILE
********************************************************************************
*/
