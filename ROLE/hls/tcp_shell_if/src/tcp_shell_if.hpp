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

//-------------------------------------------------------------------
//-- DEFAULT LOCAL-FPGA AND FOREIGN-HOST SOCKETS
//--  By default, the following sockets and port numbers will be used
//--  by the TcpShellInterface (unless user specifies new ones via TBD).
//--  Default listen ports:
//--  --> 5001 : Traffic received on this port is [TODO-TBD].
//--             It is used to emulate IPERF V2.
//--  --> 5201 : Traffic received on this port is [TODO-TBD].
//--             It is used to emulate IPREF V3.
//--  --> 8800 : Traffic received on this port is systematically
//--             dumped. It is used to test the Rx part of TOE.
//--  --> 8801 : A message received on this port triggers the
//--             transmission of 'nr' bytes from the FPGA to the host.
//--             It is used to test the Tx part of TOE.
//--  --> 8802 : Traffic received on this port is forwarded to the TCP
//--             test application which will loop and echo it back to
//--             the sender in store-and-forward mode.
//--  --> 8803 : Traffic received on this port is forwarded to the TCP
//--             test application which will loop and echo it back to
//--             the sender in path-through mode.
//-------------------------------------------------------------------
#define RECV_MODE_LSN_PORT      8800        // 0x2260
#define XMIT_MODE_LSN_PORT      8801        // 0x2261
#define ECHO_MOD2_LSN_PORT      8802        // 0x2262
#define ECHO_MODE_LSN_PORT      8803        // 0x2263
#define IPERF_LSN_PORT          5001        // 0x1389
#define IPREF3_LSN_PORT         5201        // 0x1451

#define FIXME_DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // HOST's IP Address      = 10.12.200.50
#define FIXME_DEFAULT_HOST_LSN_PORT   8803+0x8000 // HOST   listens on port = 41571

//-------------------------------------------------------------------
//-- DEFAULT XMIT STRING
//--  By default, the string 'Hi from FMKU60\n' is sent out as two
//--  alternating chunks during the transmission test, which turns
//--  into the two following chunks:
//-------------------------------------------------------------------
#define GEN_CHK0    0x48692066726f6d20  // 'Hi from '
#define GEN_CHK1    0x464d4b553630210a  // 'FMKU60\n'

enum DropCode {
    NOP=0,  // No Operation
    GEN     // Generate traffic towards producer
};

/*******************************************************************************
 * Forward Command
 *  Indicates if a received stream with session Id 'sessId' must be forwarded
 *  or dropped. If the action is 'CMD_DROP' the field 'DropCOde' is meaningful,
 *  otherwise void.
 *******************************************************************************/
class ForwardCmd {
  public:
    SessionId   sessId;
    TcpDatLen   datLen;
    CmdBit      action;
    DropCode    dropCode;
    ForwardCmd() {}
    ForwardCmd(SessionId _sessId, TcpDatLen _datLen, CmdBit _action, DropCode _dropCode) :
            sessId(_sessId), datLen(_datLen), action(_action), dropCode(_dropCode) {}
};

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
        stream<AxisRaw>       &siTAF_Data,  // [FIXME-TcpAppData]
        stream<TcpSessId>     &siTAF_SessId,
        stream<TcpDatLen>     &siTAF_DatLen,

        //------------------------------------------------------
        //-- TAF / Tx Data Interface
        //------------------------------------------------------
        stream<AxisRaw>       &soTAF_Data,  // [FIXME-TcpAppData]
        stream<TcpSessId>     &soTAF_SessId,
        stream<TcpDatLen>     &soTAF_DatLen,

        //------------------------------------------------------
        //-- SHELL / Rx Data Interfaces
        //------------------------------------------------------
        stream<TcpAppNotif>   &siSHL_Notif,
        stream<TcpAppRdReq>   &soSHL_DReq,
        stream<AxisRaw>       &siSHL_Data,  // [FIXME-TcpAppData]
        stream<TcpAppMeta>    &siSHL_Meta,

        //------------------------------------------------------
        //-- SHELL / Listen Interfaces
        //------------------------------------------------------
        stream<TcpAppLsnReq>  &soSHL_LsnReq,
        stream<TcpAppLsnRep>  &siSHL_LsnRep,

        //------------------------------------------------------
        //-- SHELL / Tx Data Interfaces
        //------------------------------------------------------
        stream<AxisRaw>       &soSHL_Data,  // [FIXME-TcpAppData]
        stream<TcpAppSndReq>  &soSHL_SndReq,
        stream<TcpAppSndRep>  &siSHL_SndRep,

        //------------------------------------------------------
        //-- SHELL / Open Interfaces
        //------------------------------------------------------
        stream<TcpAppOpnReq>  &soSHL_OpnReq,
        stream<TcpAppOpnRep>  &siSHL_OpnRep,

        //------------------------------------------------------
        //-- SHELL / Close Interfaces
        //------------------------------------------------------
        stream<TcpAppClsReq>  &soSHL_ClsReq
        //-- Not Used         &siSHL_ClsSts

);

#endif

/*! \} */
