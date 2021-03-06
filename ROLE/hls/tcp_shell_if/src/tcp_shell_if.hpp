/*
 * Copyright 2016 -- 2021 IBM Corporation
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
 * Component   : cFp_HelloKale / ROLE
 * Language    : Vivado HLS
 *
 * \ingroup tcp_shell_if
 * \addtogroup tcp_shell_if
 * \{
 *******************************************************************************/

#ifndef _TSIF_H_
#define _TSIF_H_

#include <hls_stream.h>
#include "ap_int.h"

#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts_utils.hpp"

//-------------------------------------------------------------------
//-- CONSTANTS FOR THE INTERNAL STREAM DEPTHS
//-------------------------------------------------------------------
const int cDepth_IRbToRDp_Data       = 256;  // SizeOf input data read buffer (in #chunks)
const int cDepth_IRbToRDp_Meta       = 256;  // SizeOf input meta read buffer (in #metadata)

const int cDepth_RNhToRRh_Notif      =  64;  // SizeOf input notif buffer (in #notifications)
const int cDepth_RRhToRDp_FwdCmd     =   8;  // SizeOf output data request buffer (in #requests)
const int cDepth_RRhToRRm_DReq       = cDepth_RRhToRDp_FwdCmd;

const int cDepth_RDpToRRh_Dequeue    =   4;
const int cDepth_RDpToCOn_OpnSockReq =   2;
const int cDepth_RDpToCOn_TxCountReq = cDepth_RDpToCOn_OpnSockReq;

const int cDepth_COnToWRp_TxBytesReq =   2;
const int cDepth_COnToWRp_TxSessId   = cDepth_COnToWRp_TxBytesReq;

//-------------------------------------------------------------------
//-- CONSTANTS FOR THE TCP SHELL INTERFACE
//-------------------------------------------------------------------
const int cMaxSessions   = TOE_MAX_SESSIONS;

const int cIBuffBytes    = cDepth_IRbToRDp_Data * (ARW/8);
const int cMinDataReqLen = 128;  // The minimum DReq length we want to generate

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

//=========================================================
//== Forward Command
//==  Indicates if a received stream must be forwarded or
//==  dropped. If the action is 'CMD_DROP' the field
//==  'DropCOde' is meaningful, otherwise void.
//=========================================================
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

//=========================================================
//== Interrupt Table Entry
//==  The interrupt table keeps track of the received
//==  notifications. It maintains a counter of received
//==  bytes per sessions as well as the TCP destination
//==  port number.
//=========================================================
class InterruptEntry {
  public:
    TcpDatLen   byteCnt;
    TcpPort     dstPort;
    InterruptEntry() {}
    InterruptEntry(TcpDatLen _byteCnt, TcpPort _dstPort) :
            byteCnt(_byteCnt), dstPort(_dstPort) {}
};

enum QueryCmd {
    GET=0,  // Retrieve a table entry
    PUT,    // Modify an entry of the table
    POST,   // Create a new entry in the table
};

//=========================================================
//== Interrupt Table Query
//=========================================================
class InterruptQuery {
  public:
    SessionId       sessId;
    InterruptEntry  entry;
    QueryCmd        action;
    InterruptQuery () {}
    InterruptQuery(SessionId _sessId, QueryCmd _action=GET) : // GET Query
        sessId(_sessId), action(GET) {}
    InterruptQuery(SessionId _sessId, TcpDatLen _byteCnt) : // PUT Query: 'byteCnt'
        sessId(_sessId), entry(_byteCnt, 0), action(PUT) {}
    InterruptQuery(SessionId _sessId, InterruptEntry _entry, QueryCmd _action=POST) : // POST Query
        sessId(_sessId), entry(_entry), action(POST) {}
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
        stream<TcpAppData>    &siTAF_Data,
        stream<TcpSessId>     &siTAF_SessId,
        stream<TcpDatLen>     &siTAF_DatLen,

        //------------------------------------------------------
        //-- TAF / Tx Data Interface
        //------------------------------------------------------
        stream<TcpAppData>    &soTAF_Data,
        stream<TcpSessId>     &soTAF_SessId,
        stream<TcpDatLen>     &soTAF_DatLen,

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
        stream<TcpAppClsReq>  &soSHL_ClsReq,
        //-- Not Used         &siSHL_ClsSts,

        //------------------------------------------------------
        //-- DEBUG Probes
        //------------------------------------------------------
        stream<ap_uint<32> >  &soDBG_SinkCnt,
        stream<ap_uint<16> >  &soDBG_InpBufSpace

);

#endif

/*! \} */
