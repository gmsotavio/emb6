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
/**
 * \addtogroup bsp
 * @{
 * \addtogroup if    PHY interfaces
 * @{ */

/**
 * \defgroup tcpip_radio PC based fake radio transceiver library
 *
 * The PC based radio library provides function for sending
 * and receiving packets via UDP ports on a loopback interface
 * of linux.
 *
 * @{
 */
/*
 * tcpip.h
 *
 *  Created on: Aug, 2015
 *      Author: Artem Yushev artem.yushev@hs-offenburg.de
 */

#ifndef TCPIP_RADIO_H_
#define TCPIP_RADIO_H_

#include "emb6.h"
#include "evproc.h"

#define FRADIO_S_IP                            "127.255.255.255"
#define FRADIO_C_IP                            "127.255.255.255"
#define FRADIO_OUTPORT                        "40001"
#define FRADIO_INPORT                        "40002"
#define FRADIO_OUTPORT_CLIENT                FRADIO_OUTPORT
#define FRADIO_INPORT_CLIENT                FRADIO_INPORT
#define UDPDEV_LLADDR_CLIENT                "1"
#define FRADIO_OUTPORT_SERVER                FRADIO_INPORT
#define FRADIO_INPORT_SERVER                FRADIO_OUTPORT
#define UDPDEV_LLADDR_SERVER                "2"


#endif /* TCPIP_RADIO_H_ */
/** @} */
/** @} */
/** @} */
