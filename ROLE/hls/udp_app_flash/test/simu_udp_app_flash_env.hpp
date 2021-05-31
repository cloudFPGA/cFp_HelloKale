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
 * @file       : simu_udp_app_flash.hpp
 * @brief      : Simulation environment for the UDP Application Flash (UAF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE/TcpApplicationFlash (UAF)
 * Language    : Vivado HLS
 *
 * \ingroup ROLE_UAF
 * \addtogroup ROLE_UAF_TEST
 * \{
 ******************************************************************************/

#ifndef _SIMU_UAF_H_
#define _SIMU_UAF_H_

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <hls_stream.h>

#include "../src/udp_app_flash.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/SimNtsUtils.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/SimAppData.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/SimUdpDatagram.hpp"

//------------------------------------------------------
//-- TESTBENCH DEFINES
//------------------------------------------------------
#define TB_MAX_CYCLES    500
#define TB_GRACE_TIME    500  // Give the TB some time to finish
#define VALID       true
#define UNVALID     false
#define DEBUG_TRACE true

#define ENABLED     (ap_uint<1>)1
#define DISABLED    (ap_uint<1>)0


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
bool readDatagramFromFile(
        const char        *myName,
        SimUdpDatagram    &appDatagram,
        ifstream          &ifsData,
        SocketPair        &sockPair,
        queue<UdpAppMetb> &udpMetaQueue,
        int               &inpChunks,
        int               &inpDgrms,
        int               &inpBytes);

int createGoldenTxFiles(
        EchoCtrl          tbMode,
        string            inpData_FileName,
        queue<UdpAppMetb> &udpMetaQueue,
        string            outData_GoldName,
        string            outMeta_GoldName,
        string            outDLen_GoldName);

int createUdpRxTraffic(
        stream<AxisApp>    &ssData, 
        const string       ssDataName,
        stream<UdpAppMetb> &ssMeta, 
        const string       ssMetaName,
        string             datFile,
        queue<UdpAppMetb>  &metaQueue,
        int                &nrFeededChunks);

bool drainUdpMetaStreamToFile(
        stream<UdpAppMetb> &ss,
        string             ssName,
        string             datFile,
        int                &nrChunks,
        int                &nrFrames,
        int                &nrBytes);

bool drainUdpDLenStreamToFile(
        stream<UdpAppDLen> &ss, 
        string             ssName,
        string             datFile, 
        int                &nrChunks, 
        int                &nrFrames, 
        int                &nrBytes);

#endif

/*! \} */





