/*
 * Copyright 2016 -- 2020 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 * @file       : udp_shell_if.hpp
 * @brief      : UDP Shell Interface (USIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the
 *                   UDP shell interface.
 *
 * \ingroup ROL
 * \addtogroup ROL_USIF
 * \{
 *******************************************************************************/

#ifndef _USIF_H_
#define _USIF_H_

#include <hls_stream.h>
#include "ap_int.h"

#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts_utils.hpp"

using namespace hls;

//-------------------------------------------------------------------
//-- DEFAULT LISTENING PORTS
//--  By default, the following port numbers will be used by the
//--  UdpShellInterface (unless user specifies new ones via TBD).
//--  Default listen ports:
//--  --> 5001 : Traffic received on this port is [TODO-TBD].
//--             It is used to emulate IPERF V2.
//--  --> 5201 : Traffic received on this port is [TODO-TBD].
//--             It is used to emulate IPREF V3.
//--  --> 8800 : Traffic received on this port is systematically
//--             dumped. It is used to test the Rx part of UOE.
//--  --> 8801 : A message received on this port triggers the
//--             transmission of 'nr' bytes from the FPGA to the host.
//--             It is used to test the Tx part of UOE.
//--  --> 8802 : Traffic received on this port is forwarded to the UDP
//--             test application which will loop and echo it back to
//--             the sender in store-and-forward mode.
//--  --> 8803 : Traffic received on this port is forwarded to the UDP
//--             test application which will loop and echo it back to
//--             the sender in path-through mode.
//-------------------------------------------------------------------
#define RECV_MODE_LSN_PORT      8800        // 0x2260
#define XMIT_MODE_LSN_PORT      8801        // 0x2261
#define ECHO_MOD2_LSN_PORT      8802        // 0x2262
#define ECHO_MODE_LSN_PORT      8803        // 0x2263
#define IPERF_LSN_PORT          5001        // 0x1389
#define IPREF3_LSN_PORT         5201        // 0x1451


/*******************************************************************************
 *
 * ENTITY - UDP SHELL INTERFACE (USIF)
 *
 *******************************************************************************/
void udp_shell_if(

        //------------------------------------------------------
        //-- SHELL / Mmio Interface
        //------------------------------------------------------
        CmdBit                  *piSHL_Mmio_En,

        //------------------------------------------------------
        //-- SHELL / Control Port Interfaces
        //------------------------------------------------------
        stream<UdpAppLsnReq>    &soSHL_LsnReq,
        stream<UdpAppLsnRep>    &siSHL_LsnRep,
        stream<UdpAppClsReq>    &soSHL_ClsReq,
        stream<UdpAppClsRep>    &siSHL_ClsRep,

        //------------------------------------------------------
        //-- SHELL / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>      &siSHL_Data,
        stream<UdpAppMeta>      &siSHL_Meta,

        //------------------------------------------------------
        //-- SHELL / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>      &soSHL_Data,
        stream<UdpAppMeta>      &soSHL_Meta,
        stream<UdpAppDLen>      &soSHL_DLen,

        //------------------------------------------------------
        //-- UAF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>      &siUAF_Data,
        stream<UdpAppMeta>      &siUAF_Meta,
        stream<UdpAppDLen>      &siUAF_DLen,

        //------------------------------------------------------
        //-- UAF / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>      &soUAF_Data,
        stream<UdpAppMeta>      &soUAF_Meta

);

#endif

/*! \} */
