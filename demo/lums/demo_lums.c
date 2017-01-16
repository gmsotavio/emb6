/*
 * --- License --------------------------------------------------------------*
 */
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

/*
 * --- Module Description ---------------------------------------------------*
 */
/**
 *  \file       demo_lums.c
 *  \author     Institute of reliable Embedded Systems
 *              and Communication Electronics
 *  \date       $Date$
 *  \version    $Version$
 *
 *  \brief      LUMS' demo application
 */

/*
 * --- Includes -------------------------------------------------------------*
 */
#include "emb6.h"
#include "bsp.h"
#include "evproc.h"
#include "tcpip.h"
#include "uip-udp-packet.h"
#include "rpl.h"
#include "packetbuf.h"
#include "ctimer.h"

#include "demo_lums.h"
#include "lums_menu.h"

#include "clp.h"

#ifndef DEMO_LUMS_SERVER
#define DEMO_LUMS_SERVER                    TRUE
#endif

/*
 *  --- Macros ------------------------------------------------------------- *
 */
/*
 * Common configurations
 */
#define DBG_PRINTF                          clp_output

#ifndef LUMS_UTEST_EN
#define LUMS_UTEST_EN                       FALSE
#endif

#ifndef LUMS_CFG_TIME_SYNC
#define LUMS_CFG_TIME_SYNC                  TRUE
#endif

/* message format: [type] [timestamp] [sensor1 value] [sensor2 value] */
#define LUMS_MSG_DEMILITER                  " "
#define LUMS_MSG_TYPE_LEN                   ( 20u )
#define LUMS_MSG_TYPE_DATA                  "data"
#define LUMS_MSG_TYPE_SYNC                  "sync"
#define LUMS_MSG_TYPE_INTERVALSET           "interval_set"
#define LUMS_MSG_TYPE_INTERVALGET           "interval_get"

#define LUMS_MSG_TIMESTAMP_FORMAT           "%d:%d:%d:%d:%d:%d"
#define LUMS_MSG_TIMESTAMP_PRINT            "%02d:%02d:%02d %02d/%02d/%02d "

#define LUMS_MSG_SENSOR_VALUE_FORMAT        "%d:%d"

#define LUMS_MSG_DATA_FORMAT                LUMS_MSG_TYPE_DATA LUMS_MSG_DEMILITER   \
                                            LUMS_MSG_TIMESTAMP_FORMAT LUMS_MSG_DEMILITER   \
                                            LUMS_MSG_SENSOR_VALUE_FORMAT

#define LUMS_MSG_SYNC_FORMAT                LUMS_MSG_TYPE_SYNC LUMS_MSG_DEMILITER   \
                                            LUMS_MSG_TIMESTAMP_FORMAT

#define LUMS_MSG_GET_INTERVAL_FORMAT        LUMS_MSG_TYPE_INTERVALGET LUMS_MSG_DEMILITER
#define LUMS_MSG_SET_INTERVAL_FORMAT        LUMS_MSG_TYPE_INTERVALSET LUMS_MSG_DEMILITER


/** first port used by the demo */
#ifndef DEMO_LUMS_PORTA
#define DEMO_LUMS_PORTA                     4211UL
#endif /* #ifndef DEMO_LUMS_PORTA */

#ifndef DEMO_LUMS_PORTB
/** second port used by the demo */
#define DEMO_LUMS_PORTB                     4233UL
#endif /* #ifndef DEMO_LUMS_PORTB */

/** specify output buffer size in octets */
#define LUMS_OBUF_SIZE                      ( 50u )

/** pointer to the UIP buffer structure */
#define LUMS_UIP_BUF                        ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])


#if (DEMO_LUMS_SERVER == TRUE)
/*
 * Server's configurations
 */
/** assign device port */
#define DEMO_UDP_DEVPORT                    DEMO_LUMS_PORTA
/** assign remote port */
#define DEMO_UDP_REMPORT                    DEMO_LUMS_PORTB

#define LUMS_CLIENT_TABLE_SIZE              ( 2u )
#define LUMS_CLIENT_ID_INVALID              ( 255u )

#else /* #if (DEMO_LUMS_SERVER == TRUE) */
/*
 * Client's configurations
 */
/** assign device port */
#define DEMO_UDP_DEVPORT                    DEMO_LUMS_PORTB
/** assign remote port */
#define DEMO_UDP_REMPORT                    DEMO_LUMS_PORTA

/** specifies the interval to send periodic data */
#ifndef LUMS_SENSOR_DATA_TX_INTERVAL
#define LUMS_SENSOR_DATA_TX_INTERVAL        ( 1u )
#endif /* #ifndef LUMS_SENSOR_DATA_TX_INTERVAL */

/** specifies the configuration timeout */
#ifndef DEMO_UDP_CONF_TIMEOUT
#define DEMO_UDP_CONF_TIMEOUT               ( 10u )
#endif /* #ifndef DEMO_UDP_CONF_TIMEOUT */

/** Sensor data reading intervals */
#define LUMS_INTERVAL_1_SEC                 ( bsp_getTRes() )
#define LUMS_INTERVAL_1_MIN                 ( 60 * bsp_getTRes() )

#define LUMS_SS1_RX_INTERVAL                ( 60u * LUMS_INTERVAL_1_SEC )
#define LUMS_SS2_RX_INTERVAL                ( 60u * LUMS_INTERVAL_1_SEC )
#define LUMS_SS_TX_INTERVAL                 ( 15u * LUMS_INTERVAL_1_SEC )

#define LUMS_SS1_UART_CHAN                  EN_HAL_UART_USER_1
#define LUMS_SS1_ID                         EN_HAL_PERIPHIRQ_USERUART1_RX

#define LUMS_SS2_UART_CHAN                  EN_HAL_UART_USER_2
#define LUMS_SS2_ID                         EN_HAL_PERIPHIRQ_USERUART2_RX

/** specify sensor's read buffer */
#ifndef LUMS_SS_RD_BUF_SIZE
#define LUMS_SS_RD_BUF_SIZE                 ( 5u )
#endif

#endif /* #if (DEMO_LUMS_SERVER == TRUE) */


/*
 *  --- Local Data Types --------------------------------------------------- *
 */
#if (DEMO_LUMS_SERVER == TRUE)
typedef struct s_lumsSensorCtx
{
    uip_ipaddr_t addr;
    uint32_t numRxMsg;
} s_lumsSensorCtx_t;

#else
typedef struct s_lumsSensorData
{
     void *p_uart;
     uint16_t value;
     struct ctimer tmr;
     uint8_t buf[LUMS_SS_RD_BUF_SIZE];
     uint8_t wrPtr;
     en_hal_periphirq_t id;
     pf_hal_irqCb_t rxCb;
} s_lumsSensorData_t;
#endif /* #if (DEMO_LUMS_SERVER == TRUE) */

typedef struct s_demoLumsCtx
{
  /* UDP connection */
  struct uip_udp_conn *p_udpConn;
  /* transmission timer */
  struct etimer txTimer;

  uint8_t obuf[LUMS_OBUF_SIZE];
  uint16_t olen;

  /*
   * RX
   */
  /* RX Counter */
  uint32_t rxCounter;
  /* last received sequence number */
  uint32_t rxLastSeqNum;

  /* TX packet length */
  uint32_t txPayloadLen;
  /* TX Counter */
  uint32_t txCounter;
  /* last sent sequence number */
  uint32_t txSeqNum;

  /* number of lost packets. */
  uint32_t numLostPackets;

  /*
   * Info-flash
   */
  void *p_confMem;

#if (DEMO_LUMS_SERVER == TRUE)
  s_lumsSensorCtx_t s_clients[LUMS_CLIENT_TABLE_SIZE];
#else /* #if (DEMO_LUMS_SERVER == TRUE) */
  /*
   * Sensor
   */
  s_lumsSensorData_t ss1;
  s_lumsSensorData_t ss2;
  struct ctimer ssTxTmr;
  uint32_t ssTxInterval;
  uint32_t ssNumTxMsg;
#endif /* #if (DEMO_LUMS_SERVER == TRUE) */

} s_lumsCtx_t;


/*
 *  --- Local Variables ---------------------------------------------------- *
 */
/** Demo UDP socket context */
static s_lumsCtx_t gs_lumsCtx;

/*
 *  --- Local Function Prototypes ------------------------------------------ *
 */
static void lums_rx(c_event_t c_event, p_data_t p_data);
static void lums_readTime(en_hal_rtc_t *p_rtc, char *p_str);

#if (DEMO_LUMS_SERVER == TRUE)
static void lums_cmdSyncTime(s_lumsCtx_t *p_ctx, uint8_t clientId);
static void lums_cmdGetInterval(s_lumsCtx_t *p_ctx, uint8_t clientId);
static void lums_cmdSetInterval(s_lumsCtx_t *p_ctx, uint8_t clientId, uint32_t newInterval);
static void lums_cmdRequestData(s_lumsCtx_t *p_ctx, uint8_t clientId);
static void lums_cmdHandler(c_event_t c_event, p_data_t p_data);

static uint8_t lums_serverSaveClientInfo(s_lumsCtx_t *p_ctx, uip_ipaddr_t *p_src);
static void lums_serverTx(s_lumsCtx_t *p_ctx, uip_ipaddr_t *p_dest);
static void lums_serverRx(s_lumsCtx_t *p_ctx, uint8_t *p_data, uint16_t len);
#endif /* #if (DEMO_LUMS_SERVER == TRUE) */

/*
 *  --- Local Functions ---------------------------------------------------- *
 */
static void lums_readTime(en_hal_rtc_t *p_rtc, char *p_time)
{
    int hour, min, sec, day, mon, year;

    sscanf(p_time, LUMS_MSG_TIMESTAMP_FORMAT, &hour, &min, &sec, &day, &mon, &year);
    p_rtc->uc_hour = hour;
    p_rtc->uc_min = min;
    p_rtc->uc_sec = sec;
    p_rtc->uc_day = day;
    p_rtc->uc_mon = mon;
    p_rtc->ui_year = year;
}

#if (DEMO_LUMS_SERVER == TRUE)
/*---------------------------------------------------------------------------*/
/*
* lums_serverTx()
*/
static void lums_serverTx(s_lumsCtx_t *p_ctx, uip_ipaddr_t *p_dest)
{
    DBG_PRINTF("TX: %s (%d)\n", p_ctx->obuf, p_ctx->olen);

    /* send response */
    uip_ipaddr_copy(&p_ctx->p_udpConn->ripaddr, p_dest);
    uip_udp_packet_send(p_ctx->p_udpConn, p_ctx->obuf, p_ctx->olen);
    uip_create_unspecified(&p_ctx->p_udpConn->ripaddr);
}

/*---------------------------------------------------------------------------*/
/*
* lums_serverSaveClientInfo()
*/
static uint8_t lums_serverSaveClientInfo(s_lumsCtx_t *p_ctx, uip_ipaddr_t *p_src)
{
    uint8_t i;
    uint8_t isFound;
    uint8_t clientId;
    uint8_t emptyEntry;

    /* look for the client's record */
    isFound = FALSE;
    emptyEntry = LUMS_CLIENT_ID_INVALID;
    for (i = 0; i < LUMS_CLIENT_TABLE_SIZE; i++) {
        if (p_ctx->s_clients[i].addr.u16[7] == p_src->u16[7]) {
            isFound = TRUE;
            clientId = i;
            break;
        }
        if ((p_ctx->s_clients[i].addr.u16[7] == 0) && (emptyEntry == LUMS_CLIENT_ID_INVALID)) {
            emptyEntry = i;
        }
    }

    if (isFound == FALSE) {
        /* the server doesn't have any records of the client yet */
        uip_ipaddr_copy(&p_ctx->s_clients[emptyEntry].addr, p_src);
        clientId = emptyEntry;
    }
    return clientId;
}


/*---------------------------------------------------------------------------*/
/*
* lums_serverRx()
*/
static void lums_serverRx(s_lumsCtx_t *p_ctx, uint8_t *p_data, uint16_t len)
{
    char *token;
    char buffer[128];
    char msgType[LUMS_MSG_TYPE_LEN];
    en_hal_rtc_t remTime;
    en_hal_rtc_t localTime;
    int ss1Value, ss2Value;
    uip_ipaddr_t srcAddr;
    uint8_t clientId;

    /* copy input string to an intermediate buffer */
    memcpy(buffer, p_data, len);

    /* read message type */
    token = strtok(buffer, LUMS_MSG_DEMILITER);
    sscanf(token, "%s", msgType);

    /* read sender's address */
    uip_ipaddr_copy(&srcAddr, &LUMS_UIP_BUF->srcipaddr);

    /* store client's info to the record table */
    clientId = lums_serverSaveClientInfo(p_ctx, &srcAddr);
    DBG_PRINTF("%s nodeId=%d [%x:%x:%x:%x:%x:%x:%x:%x] ",
            msgType,
            clientId,
            srcAddr.u16[0], srcAddr.u16[1], srcAddr.u16[2], srcAddr.u16[3],
            srcAddr.u16[4], srcAddr.u16[5], srcAddr.u16[6], srcAddr.u16[7]);

    /* process message */
    if (strcmp(msgType, LUMS_MSG_TYPE_DATA) == 0)
    {
        /* increase number of received messages from the client */
        p_ctx->s_clients[clientId].numRxMsg++;

        /* read timestamp */
        token = strtok(NULL, LUMS_MSG_DEMILITER);
        lums_readTime(&remTime, token);
        DBG_PRINTF(LUMS_MSG_TIMESTAMP_PRINT,
                remTime.uc_hour, remTime.uc_min, remTime.uc_sec,
                remTime.uc_day, remTime.uc_mon, remTime.ui_year);

        /* read sensor values */
        token = strtok(NULL, LUMS_MSG_DEMILITER);
        sscanf(token, LUMS_MSG_SENSOR_VALUE_FORMAT, &ss1Value, &ss2Value);

        if ((ss1Value < 300) && (ss2Value < 300)) {
            DBG_PRINTF("-SS1: NC -SS2: NC");
        }
        else if (ss1Value < 300) {
            DBG_PRINTF("-SS1: NC -SS2: %d mm", ss2Value);
        }
        else if (ss2Value < 300) {
            DBG_PRINTF("-SS1: %d mm SS2: CC", ss1Value);
        }
        DBG_PRINTF(" (%d)", p_ctx->s_clients[clientId].numRxMsg);
        DBG_PRINTF(" | %d", packetbuf_attr(PACKETBUF_ATTR_RSSI));
        DBG_PRINTF("\r\n");

#if (LUMS_CFG_TIME_SYNC == TRUE)
        /* synchronize time */
        bsp_rtcGetTime(&localTime);
        remTime.uc_sec = localTime.uc_sec;

        if (strncmp((const char *)&localTime, (const char *)&remTime, sizeof(localTime)) != 0) {
            DBG_PRINTF("synchronize time\n");
            lums_cmdSyncTime(p_ctx, clientId);
        }
#endif /* #if (LUMS_CFG_TIME_SYNC == TRUE) */
    }
    else if (strcmp(msgType, LUMS_MSG_TYPE_INTERVALGET) == 0)
    {
        uint32_t txInterval;

        /* read transmission interval */
        token = strtok(NULL, LUMS_MSG_DEMILITER);
        sscanf(token, "%ld", &txInterval);
        DBG_PRINTF("Sensor's transmission interval: %ld\n", txInterval);
    }
    else {
        DBG_PRINTF("Unexpected message\n");
    }
}
#else /* #if (DEMO_LUMS_SERVER == TRUE) */


/*---------------------------------------------------------------------------*/
/*
* lums_clientTx()
*/
static void lums_clientTx(s_lumsCtx_t *p_ctx)
{
    uip_ds6_addr_t *ps_src_addr;
    rpl_dag_t *ps_dag_desc;

    /* get server IP address */
    ps_src_addr = uip_ds6_get_global(ADDR_PREFERRED);
    ps_dag_desc = rpl_get_any_dag();
    if (ps_src_addr && ps_dag_desc) {
        /* set destination address */
        uip_ipaddr_copy(&p_ctx->p_udpConn->ripaddr, &ps_src_addr->ipaddr);
        memcpy(&p_ctx->p_udpConn->ripaddr.u8[8], &ps_dag_desc->dag_id.u8[8], 8);

        /* issue transmission request */
        uip_udp_packet_send(p_ctx->p_udpConn, p_ctx->obuf, p_ctx->olen);
        uip_create_unspecified(&p_ctx->p_udpConn->ripaddr);

        /* increase number of received messages from the client */
        p_ctx->ssNumTxMsg++;
        DBG_PRINTF("TX: %s (%d)\n", p_ctx->obuf, p_ctx->ssNumTxMsg);
    }
    else {
        DBG_PRINTF("Couldn't find server address\n");
    }
}


/*---------------------------------------------------------------------------*/
/*
* lums_clientReadSensor1Data()
*/
static void lums_clientReadSensor1Data( void *p_data )
{
    s_lumsSensorData_t *p_ss = &gs_lumsCtx.ss1;

    /* read sensor value */
    p_ss->buf[p_ss->wrPtr] = *((uint8_t *)p_data);

    /* FIXME update average sensor value */
    p_ss->value = 299;

    /* update write pointer */
    p_ss->wrPtr++;
    if (p_ss->wrPtr >= LUMS_SS_RD_BUF_SIZE) {
        p_ss->wrPtr = 0;

        /* unregister UART RX interrupt to stop reading sensor values */
        bsp_periphIRQRegister(p_ss->id, 0, NULL);
    }
}

/*---------------------------------------------------------------------------*/
/*
* lums_clientReadSensor2Data()
*/
static void lums_clientReadSensor2Data( void *p_data )
{
    s_lumsSensorData_t *p_ss = &gs_lumsCtx.ss2;

    /* read sensor value */
    p_ss->buf[p_ss->wrPtr] = *((uint8_t *)p_data);

    /* FIXME update average sensor value */
    p_ss->value = 299;

    /* update write pointer */
    p_ss->wrPtr++;
    if (p_ss->wrPtr >= LUMS_SS_RD_BUF_SIZE) {
        p_ss->wrPtr = 0;

        /* unregister UART RX interrupt to stop reading sensor values */
        bsp_periphIRQRegister(p_ss->id, 0, NULL);
    }
}

/*---------------------------------------------------------------------------*/
/*
* lums_clientReadSensorTimeout()
*/
static void lums_clientReadSensorTimeout( void *p_sensor )
{
    s_lumsSensorData_t *p_ss = (s_lumsSensorData_t *)p_sensor;

    /* restart reading timer */
    ctimer_restart(&p_ss->tmr);

    /* register UART RX interrupt for reading sensor values */
    bsp_periphIRQRegister(p_ss->id, p_ss->rxCb, p_ss);
}

/*---------------------------------------------------------------------------*/
/*
* lums_sendSensorData()
*/
static void lums_clientSendSensorData( void *p_appCtx )
{
    s_lumsCtx_t *p_ctx = (s_lumsCtx_t *)p_appCtx;
    en_hal_rtc_t rtc;

    /* restart timer */
    ctimer_restart(&p_ctx->ssTxTmr);

    /* clear output buffer */
    memset(p_ctx->obuf, 0, sizeof(p_ctx->obuf));

    /* prepare the response */
    bsp_rtcGetTime(&rtc);
    p_ctx->olen = sprintf((char *)p_ctx->obuf,
            LUMS_MSG_DATA_FORMAT,
            rtc.uc_hour, rtc.uc_min, rtc.uc_sec, rtc.uc_day, rtc.uc_mon, rtc.ui_year,
            p_ctx->ss1.value, p_ctx->ss2.value);

    /* TODO send the application payload */
    lums_clientTx(p_ctx);
}

/*---------------------------------------------------------------------------*/
/*
* lums_clientSyncTime()
*/
static void lums_clientSyncTime(char *p_time)
{
    en_hal_rtc_t remTime;
    en_hal_rtc_t localTime;

    /* TODO just for testing */
    bsp_rtcGetTime(&localTime);
    DBG_PRINTF("before: " LUMS_MSG_TIMESTAMP_PRINT "\n",
            localTime.uc_hour, localTime.uc_min, localTime.uc_sec,
            localTime.uc_day, localTime.uc_mon, localTime.ui_year);

    /* read remote time i.e., 18:21:00:01:10:17 */
    lums_readTime(&remTime, p_time);

    /* update local time */
    bsp_rtcSetTime(&remTime);

    /* TODO just for testing */
    bsp_rtcGetTime(&localTime);
    DBG_PRINTF("after: " LUMS_MSG_TIMESTAMP_PRINT "\n",
            localTime.uc_hour, localTime.uc_min, localTime.uc_sec,
            localTime.uc_day, localTime.uc_mon, localTime.ui_year);
}

/*---------------------------------------------------------------------------*/
/*
* lums_clientRx()
*/
static void lums_clientRx(s_lumsCtx_t *p_ctx, uint8_t *p_data, uint16_t len)
{
    char *token;
    char buffer[128];
    char msgType[15];

    /* copy input string to an intermediate buffer */
    memcpy(buffer, p_data, len);

    /* read message type */
    token = strtok(buffer, LUMS_MSG_DEMILITER);
    sscanf(token, "%s", msgType);
    DBG_PRINTF("received message type: %s\n", msgType);

    /* read message payload */
    token = strtok(NULL, "");
    DBG_PRINTF("received message payload: %s\n", token);

    /* process message */
    if (strcmp(msgType, LUMS_MSG_TYPE_SYNC) == 0)
    {
        /* sync local time to the remote time */
        lums_clientSyncTime(token);
    }
    else if (strcmp(msgType, LUMS_MSG_TYPE_INTERVALSET) == 0)
    {
        /* get the new transmission interval */
        sscanf(token, "%ld", &p_ctx->ssTxInterval);
        /* update transmission timer */
        ctimer_stop(&p_ctx->ssTxTmr);
        ctimer_set(&p_ctx->ssTxTmr, p_ctx->ssTxInterval, lums_clientSendSensorData, p_ctx);
    }
    else if (strcmp(msgType, LUMS_MSG_TYPE_INTERVALGET) == 0)
    {
        /* clear output buffer */
        memset(p_ctx->obuf, 0, sizeof(p_ctx->obuf));
        /* prepare the response */
        memcpy(msgType, LUMS_MSG_TYPE_INTERVALGET, strlen(LUMS_MSG_TYPE_INTERVALGET));
        p_ctx->olen = sprintf((char *)p_ctx->obuf, "%s %lu", msgType, p_ctx->ssTxInterval);
        /* commence transmission */
        lums_clientTx(p_ctx);
    }
}
#endif


/*---------------------------------------------------------------------------*/
/*
* lums_rx()
*/
static void lums_rx(c_event_t c_event, p_data_t p_data)
{
    uint8_t has_data = 0;
    uint8_t *p_payload = NULL;
    uint16_t payload_len = 0;
    s_lumsCtx_t *p_ctx = &gs_lumsCtx;

    /*
     * process input TCPIP packet
     */
    if (c_event == EVENT_TYPE_TCPIP) {
        has_data = uip_newdata();
        if (has_data != 0u) {
            p_payload = (uint8_t *) uip_appdata;
            payload_len = (uint16_t) uip_datalen();
        }
    }

    if (p_payload != NULL) {
        /* RX indicator */
        bsp_led(HAL_LED3, EN_BSP_LED_OP_BLINK);

#if (DEMO_LUMS_SERVER == TRUE)
        lums_serverRx(p_ctx, p_payload, payload_len);
#else
        lums_clientRx(p_ctx, p_payload, payload_len);
#endif /* #if (DEMO_LUMS_SERVER == TRUE) */
    }
}


#if (DEMO_LUMS_SERVER)
/*---------------------------------------------------------------------------*/
/*
* lums_cmdGetInterval()
*/
static void lums_cmdGetInterval(s_lumsCtx_t *p_ctx, uint8_t clientId)
{
    /* clear output buffer */
    memset(p_ctx->obuf, 0, sizeof(p_ctx->obuf));
    /* prepare the message to send */
    p_ctx->olen = sprintf((char *)p_ctx->obuf, LUMS_MSG_GET_INTERVAL_FORMAT);
    /* send message */
    lums_serverTx(p_ctx, &p_ctx->s_clients[clientId].addr);
}

/*---------------------------------------------------------------------------*/
/*
* lums_cmdSetInterval()
*/
static void lums_cmdSetInterval(s_lumsCtx_t *p_ctx, uint8_t clientId, uint32_t newInterval)
{
    /* clear output buffer */
    memset(p_ctx->obuf, 0, sizeof(p_ctx->obuf));
    /* prepare the message to send */
    p_ctx->olen = sprintf((char *)p_ctx->obuf, LUMS_MSG_SET_INTERVAL_FORMAT " %lu", newInterval);
    /* send message */
    lums_serverTx(p_ctx, &p_ctx->s_clients[clientId].addr);
}

/*---------------------------------------------------------------------------*/
/*
* lums_cmdSyncTime()
*/
static void lums_cmdSyncTime(s_lumsCtx_t *p_ctx, uint8_t clientId)
{
    en_hal_rtc_t localTime;

    /* read local time */
    bsp_rtcGetTime(&localTime);

    /* clear output buffer */
    memset(p_ctx->obuf, 0, sizeof(p_ctx->obuf));
    /* prepare the message to send */
    p_ctx->olen = sprintf((char *)p_ctx->obuf, LUMS_MSG_SYNC_FORMAT,
            localTime.uc_hour, localTime.uc_min, localTime.uc_sec, localTime.uc_day, localTime.uc_mon, localTime.ui_year);
    /* send message */
    lums_serverTx(p_ctx, &p_ctx->s_clients[clientId].addr);
}

/*---------------------------------------------------------------------------*/
/*
* lums_cmdRequestData()
*/
static void lums_cmdRequestData(s_lumsCtx_t *p_ctx, uint8_t clientId)
{

}

/*---------------------------------------------------------------------------*/
/*
* lums_cmdHandler()
*/
static void lums_cmdHandler(c_event_t c_event, p_data_t p_data)
{
    s_lumsCtx_t *p_ctx = &gs_lumsCtx;

    if ((c_event == NETSTK_APP_EVENT_TX) && (p_data != NULL)) {
        s_lums_cmd_t *p_cmd = (s_lums_cmd_t *)p_data;

        if (strcmp(p_cmd->type, "s") == 0) {
            lums_cmdSyncTime(p_ctx, p_cmd->param1);
        }
        else if (strcmp(p_cmd->type, "si") == 0) {
            lums_cmdSetInterval(p_ctx, p_cmd->param1, p_cmd->param2);
        }
        else if (strcmp(p_cmd->type, "gi") == 0) {
            lums_cmdGetInterval(p_ctx, p_cmd->param1);
        }
        else {
            DBG_PRINTF("unknown command %s\n", p_cmd->type);
        }
    }
}
#endif /* #if (DEMO_LUMS_SERVER) */


/*
 * --- Global Function Definitions ----------------------------------------- *
 */

/*---------------------------------------------------------------------------*/
/*
* demo_lumsInit()
*/
int8_t demo_lumsInit(void)
{
    s_lumsCtx_t *p_ctx = &gs_lumsCtx;

    /* set all attributes to zero */
    memset(p_ctx, 0x00, sizeof(*p_ctx));

    /* create a new UDP connection */
    p_ctx->p_udpConn = udp_new(NULL, UIP_HTONS(DEMO_UDP_REMPORT), NULL);
    EMB6_ASSERT_RET(p_ctx->p_udpConn != NULL, -1);

    /* bind the UDP connection to a local port*/
    udp_bind(p_ctx->p_udpConn, UIP_HTONS(DEMO_UDP_DEVPORT));

    /* set callback for event process */
    evproc_regCallback(EVENT_TYPE_TCPIP, lums_rx);

#if (DEMO_LUMS_SERVER == TRUE)
    /* register events for command handling */
    evproc_regCallback(NETSTK_APP_EVENT_TX, lums_cmdHandler);

    /* set local time */
    en_hal_rtc_t localTime = {
        .uc_hour = 17,
        .uc_min  = 53,
        .uc_sec  = 00,
        .uc_day  = 12,
        .uc_mon  = 01,
        .ui_year = 17,
    };
    bsp_rtcSetTime(&localTime);
#else /* #if (DEMO_LUMS_SERVER == TRUE) */
    /* setup sensors' attributes */
    p_ctx->ss1.id = LUMS_SS1_ID;
    p_ctx->ss1.rxCb = lums_clientReadSensor1Data;
    p_ctx->ss1.p_uart = bsp_uartInit(LUMS_SS1_UART_CHAN);
    ctimer_set(&p_ctx->ss1.tmr, LUMS_SS1_RX_INTERVAL, lums_clientReadSensorTimeout, &p_ctx->ss1);

    p_ctx->ss2.id = LUMS_SS2_ID;
    p_ctx->ss2.rxCb = lums_clientReadSensor2Data;
    p_ctx->ss2.p_uart = bsp_uartInit(LUMS_SS2_UART_CHAN);
    ctimer_set(&p_ctx->ss2.tmr, LUMS_SS2_RX_INTERVAL, lums_clientReadSensorTimeout, &p_ctx->ss2);

    /* initialize timer for periodic data transmission */
    p_ctx->ssTxInterval = LUMS_SS_TX_INTERVAL;
    ctimer_set(&p_ctx->ssTxTmr, p_ctx->ssTxInterval, lums_clientSendSensorData, p_ctx);
#endif /* #if (DEMO_LUMS_SERVER != TRUE) */

#if (LUMS_UTEST_EN == TRUE)
#if (DEMO_LUMS_SERVER == TRUE)
    /*
     * Test S1: Server receives a Data message
     */
    uint8_t dataMsg[] = LUMS_MSG_TYPE_DATA " 18:21:0:1:10:17 200:500";
    lums_serverRx(p_ctx, dataMsg, sizeof(dataMsg));

    /*
     * Test S2: Server receives a interval_get message
     */
    uint8_t intergetMsg[] = LUMS_MSG_TYPE_INTERVALGET " 123456";
    lums_serverRx(p_ctx, intergetMsg, sizeof(intergetMsg));
#else /* #if (DEMO_LUMS_SERVER == TRUE) */
    /*
     * Test C1: Client receives a SYNC message
     */
    uint8_t syncMsg[] = LUMS_MSG_TYPE_SYNC " 18:21:00:01:10:17";
    lums_clientRx(p_ctx, syncMsg, sizeof(syncMsg));

    /*
     * Test C2: Client receives a Interval message
     */
    uint8_t intersetMsg[] = LUMS_MSG_TYPE_INTERVALSET " 123456";
    lums_clientRx(p_ctx, intersetMsg, sizeof(intersetMsg));

    /*
     * Test C3: Client receives a Interget message
     */
    uint8_t intergetMsg[] = LUMS_MSG_TYPE_INTERVALGET " 18:21:00:01:10:17";
    lums_clientRx(p_ctx, intergetMsg, sizeof(intergetMsg));

    /*
     * Test C4: Client sends sensor data message
     */
    p_ctx->ss1.value = 200;
    p_ctx->ss2.value = 500;
    lums_clientSendSensorData(p_ctx);

#endif /* #if (DEMO_LUMS_SERVER == TRUE) */

    while (1)
        ;
#endif

    /* Always success */
    return 1;
} /* demo_lumsInit() */


/*---------------------------------------------------------------------------*/
/*
* demo_lumsConf()
*/
int8_t demo_lumsConf(s_ns_t *p_netstk)
{
  int8_t i_ret = -1;

  if (p_netstk != NULL) {
    if (p_netstk->c_configured == FALSE) {
      p_netstk->hc = &hc_driver_sicslowpan;
      p_netstk->frame = &framer_802154;
      p_netstk->dllsec = &dllsec_driver_null;
      i_ret = 1;
    } else {
      if ((p_netstk->hc == &hc_driver_sicslowpan) &&
          (p_netstk->frame == &framer_802154) &&
          (p_netstk->dllsec == &dllsec_driver_null)) {
        i_ret = 1;
      } else {
        p_netstk = NULL;
        i_ret = 0;
      }
    }

    /*
     * handles stack configuration via CLI
     */
    lums_menuInit(p_netstk);
    lums_menuRun();
  }

  return i_ret;
} /* demo_lumsConf() */

