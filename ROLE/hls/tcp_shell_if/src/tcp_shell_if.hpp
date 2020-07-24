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

/*******************************************************************************
 * @file       : tcp_shell_if.hpp
 * @brief      : TCP Shell Interface (TSIF)
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp / ROLE
 * Language    : Vivado HLS
 *
 * \ingroup ROLE
 * \addtogroup ROLE_TSIF
 * \{
 *******************************************************************************/

#ifndef _TSIF_H_
#define _TSIF_H_

#include <hls_stream.h>
#include "ap_int.h"

#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts_utils.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/AxisApp.hpp"

using namespace hls;

//-------------------------------------------------------------------
//-- DEFAULT LOCAL-FPGA AND FOREIGN-HOST SOCKETS
//--  By default, the following sockets and port numbers will be used
//--  by the TcpRoleInterface (unless user specifies new ones via TBD).
//--  Default listen ports:
//--  --> 5001 : Traffic received on this port is systematically
//--             dumped. It is used to emulate IPERF V2.
//--  --> 5201 : Traffic received on this port is systematically
//--             dumped. It is used to emulate IPREF V3.
//--  --> 8803 : Traffic received on this port is looped backed and
//--             echoed to the sender.
//-------------------------------------------------------------------
#define ECHO_LSN_PORT           8803        // 0x2263
#define IPERF_LSN_PORT          5001        // 0x1389
#define IPREF3_LSN_PORT         5201        // 0x1451

#define FIXME_DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // HOST's IP Address      = 10.12.200.50
#define FIXME_DEFAULT_HOST_LSN_PORT   8803+0x8000 // HOST   listens on port = 41571

/*************************************************************************
 *
 * ENTITY - TCP SHELL INTERFACE (TSIF)
 *
 *************************************************************************/
void tcp_shell_if(

        //------------------------------------------------------
        //-- SHELL / Mmio Interface
        //------------------------------------------------------
        CmdBit                *piSHL_Mmio_En,

        //------------------------------------------------------
        //-- TAF / Rx Data Interface
        //------------------------------------------------------
        stream<TcpAppData>    &siTAF_Data,
        stream<TcpAppMeta>    &siTAF_Meta,

        //------------------------------------------------------
        //-- TAF / Tx Data Interface
        //------------------------------------------------------
        stream<TcpAppData>    &soTAF_Data,
        stream<TcpAppMeta>    &soTAF_Meta,

        //------------------------------------------------------
        //-- SHELL / Rx Data Interfaces
        //------------------------------------------------------
        stream<TcpAppNotif>   &siSHL_Notif,
        stream<TcpAppRdReq>   &soSHL_DReq,
        stream<TcpAppData>    &siSHL_Data,
        stream<TcpAppMeta>    &siSHL_Meta,

        //------------------------------------------------------
        //-- SHELL / Listen Interfaces
        //------------------------------------------------------
        stream<TcpAppLsnReq>  &soSHL_LsnReq,
        stream<TcpAppLsnRep>  &siSHL_LsnRep,

        //------------------------------------------------------
        //-- SHELL / Tx Data Interfaces
        //------------------------------------------------------
        stream<TcpAppData>    &soSHL_Data,
        stream<TcpAppMeta>    &soSHL_Meta,
        stream<TcpAppWrSts>   &siSHL_DSts,

        //------------------------------------------------------
        //-- SHELL / Open Interfaces
        //------------------------------------------------------
        stream<TcpAppOpnReq>  &soSHL_OpnReq,
        stream<TcpAppOpnRep>  &siSHL_OpnRep,

        //------------------------------------------------------
        //-- SHELL / Close Interfaces
        //------------------------------------------------------
        stream<TcpAppClsReq>  &soSHL_ClsReq,
        //-- Not Used         &siSHL_ClsSts,

        //------------------------------------------------------
        //-- TAF / Session Connect Id Interface
        //------------------------------------------------------
        SessionId             *poTAF_SConId

);

#endif

/*! \} */
