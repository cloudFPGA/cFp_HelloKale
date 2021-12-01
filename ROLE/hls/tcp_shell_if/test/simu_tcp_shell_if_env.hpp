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

/*****************************************************************************
 * @file       : simu_tcp_shell_if_env.hpp
 * @brief      : Simulation environment for the TCP Shell Interface (TSIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_HelloKale/ROLE/TcpShellInterface (TSIF)
 * Language    : Vivado HLS
 *
 * \ingroup tcp_shell_if
 * \addtogroup tcp_shell_if
 * \{
 *****************************************************************************/

#ifndef _SIMU_TSIF_H_
#define _SIMU_TSIF_H_

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <hls_stream.h>
#include <map>
#include <set>

#include "../src/tcp_shell_if.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/SimNtsUtils.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/SimAppData.hpp"

//------------------------------------------------------
//-- TESTBENCH DEFINES
//------------------------------------------------------
const int cSimToeStartupDelay = 1000;
const int cGraceTime          = 2500;

const int cNrSegToSend  = 5;
const int cNrSessToSend = 2;

const int cMinWAIT = cMaxSessions;  // To avoid that TSIF accumulates the byte counts of the Notifs

//---------------------------------------------------------
//-- DEFAULT LOCAL FPGA AND FOREIGN HOST SOCKETS
//--  By default, the following sockets will be used by the
//--  testbench, unless the user specifies new ones via one
//--  of the test vector files.
//---------------------------------------------------------
#define DEFAULT_FPGA_IP4_ADDR   0x0A0CC801  // TOE's local IP Address  = 10.12.200.01
#define DEFAULT_FPGA_LSN_PORT   0x0057      // TOE listens on port     = 87 (static  ports must be     0..32767)
#define DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // TB's foreign IP Address = 10.12.200.50
#define DEFAULT_HOST_TCP_SRC_PORT 0x80      // TB source port          = 128

#define DEFAULT_SESSION_ID       0
#define DEFAULT_SESSION_LEN     32


/*******************************************************************************
 * SIMULATION UTILITY HELPERS
 ********************************************************************************/
void stepSim();
void increaseSimTime(unsigned int cycles);
bool drainDebugSinkCounter(stream<ap_uint<32> > &ss, string ssName);
bool drainDebugSpaceCounter(stream<ap_uint<16> > &ss, string ssName);


/******************************************************************************
 * SIMULATION ENVIRONMENT FUNCTIONS
 *******************************************************************************/
void pTAF(
        ofstream           &ofTAF_Data,
        stream<TcpAppData> &siTSIF_Data,
        stream<TcpSessId>  &siTSIF_SessId,
        stream<TcpDatLen>  &siTSIF_DatLen,
        stream<TcpAppData> &soTAF_Data,
        stream<TcpSessId>  &soTAF_Meta,
        stream<TcpSessId>  &soTAF_DLen);

void pMMIO(
        StsBit *piSHL_Ready,
        CmdBit *poTSIF_Enable);

void pTOE(
        int                  &nrErr,
        ofstream             &ofTAF_Gold,
        ofstream             &ofTOE_Gold,
        ofstream             &ofTOE_Data,
        TcpDatLen             echoDatLen,
        SockAddr              testSock,
        TcpDatLen             testDatLen,
        //-- MMIO / Ready Signal
        StsBit               *poMMIO_Ready,
        //-- TSIF / Tx Data Interfaces
        stream<TcpAppNotif>  &soTSIF_Notif,
        stream<TcpAppRdReq>  &siTSIF_DReq,
        stream<TcpAppData>   &soTSIF_Data,
        stream<TcpAppMeta>   &soTSIF_Meta,
        //-- TSIF / Listen Interfaces
        stream<TcpAppLsnReq> &siTSIF_LsnReq,
        stream<TcpAppLsnRep> &soTSIF_LsnRep,
        //-- TSIF / Rx Data Interfaces
        stream<TcpAppData>   &siTSIF_Data,
        stream<TcpAppSndReq> &siTSIF_SndReq,
        stream<TcpAppSndRep> &soTSIF_SndRep,
        //-- TSIF / Open Interfaces
        stream<TcpAppOpnReq> &siTSIF_OpnReq,
        stream<TcpAppOpnRep> &soTSIF_OpnRep);

#endif

/*! \} */
