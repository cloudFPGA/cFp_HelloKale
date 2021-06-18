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
 * @file       : simu_udp_shell_if_env.hpp
 * @brief      : Simulation environment for the UDP Shell Interface (USIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_Monolithic/ROLE/UdpShellInterface (USIF)
 * Language    : Vivado HLS
 *
 * \ingroup ROLE_USIF
 * \addtogroup ROLE_USIF_TEST
 * \{
 *******************************************************************************/

#ifndef _SIMU_USIF_H_
#define _SIMU_USIF_H_

#include <cstdlib>
#include <hls_stream.h>
#include <iostream>

#include "../src/udp_shell_if.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/SimNtsUtils.hpp"

//------------------------------------------------------
//-- TESTBENCH DEFINITIONS
//------------------------------------------------------
const int cUoeInitCycles = 100;  // FYI - It takes 0xFFFF cycles to initialize UOE.
const int cGraceTime     = 500;  // Give the TB some time to finish

//---------------------------------------------------------
//-- DEFAULT LOCAL FPGA AND FOREIGN HOST SOCKETS
//--  By default, the following sockets will be used by the
//--  testbench, unless the user specifies new ones via one
//--  of the test vector files.
//---------------------------------------------------------
#define DEFAULT_FPGA_IP4_ADDR   0x0A0CC801  // FPGA's local IP Address   = 10.12.200.01
#define DEFAULT_FPGA_LSN_PORT   0x2263      // UDP-ROLE listens on port  = 8803
#define DEFAULT_FPGA_SND_PORT   0xA263      // UDP-ROLE sends on port    = 41571
#define DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // HOST's foreign IP Address = 10.12.200.50
#define DEFAULT_HOST_LSN_PORT   0x80        // HOST listens on port      = 128
#define DEFAULT_HOST_SND_PORT   0x8080      // HOST sends on port        = 32896

#define DEFAULT_DATAGRAM_LEN    32


/*******************************************************************************
 * SIMULATION UTILITY HELPERS
 ********************************************************************************/
void stepSim();
void increaseSimTime(unsigned int cycles);

/******************************************************************************
 * SIMULATION ENVIRONMENT FUNCTIONS
 *******************************************************************************/
void pUAF(
        //-- USIF / Rx Data Interface
        stream<UdpAppData>  &siUSIF_Data,
        stream<UdpAppMeta>  &siUSIF_Meta,
        //-- USIF / Tx Data Interface
        stream<UdpAppData>  &soUSIF_Data,
        stream<UdpAppMeta>  &soUSIF_Meta,
        stream<UdpAppDLen>  &soUSIF_DLen);

void pMMIO(
        //-- SHELL / Ready Signal
        StsBit      *piSHL_Ready,
        //-- MMIO / Enable Layer-7 (.i.e APP alias ROLE)
        CmdBit      *poUSIF_Enable);

void pUOE(
        int                   &nrErr,
        ofstream              &dataGoldFile,
        ofstream              &dataFile,
        ofstream              &metaGoldFile,
        ofstream              &metaFile,
        int                    echoDgrmLen,
        SockAddr               testSock,
        int                    testDgrmLen,
        //-- MMIO / Ready Signal
        StsBit                *poMMIO_Ready,
        //-- UOE->USIF / UDP Rx Data Interfaces
        stream<UdpAppData>    &soUSIF_Data,
        stream<UdpAppMeta>    &soUSIF_Meta,
        stream<UdpAppDLen>    &soUSIF_DLen,
        //-- USIF->UOE / UDP Tx Data Interfaces
        stream<UdpAppData>    &siUSIF_Data,
        stream<UdpAppMeta>    &siUSIF_Meta,
        stream<UdpAppDLen>    &siUSIF_DLen,
        //-- USIF<->UOE / Listen Interfaces
        stream<UdpPort>       &siUSIF_LsnReq,
        stream<StsBool>       &soUSIF_LsnRep,
        //-- USIF<->UOE / Close Interfaces
        stream<UdpPort>       &siUSIF_ClsReq);

#endif

/*! \} */





