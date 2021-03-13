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
 * @file       : test_tcp_shell_if.hpp
 * @brief      : Testbench for the TCP Shell Interface (TSIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE
 * Language    : Vivado HLS
 *
 * \ingroup ROLE
 * \addtogroup ROLE_TSIF
 * \{
 *****************************************************************************/

#ifndef _TEST_TSIF_H_
#define _TEST_TSIF_H_

#include <hls_stream.h>
#include <iostream>
#include <fstream>
#include <map>
#include <set>

#include "../src/tcp_shell_if.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts_utils.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/SimNtsUtils.hpp"

//------------------------------------------------------
//-- TESTBENCH DEFINITIONS
//------------------------------------------------------
const int cSimToeStartupDelay = 1000;
const int cSimToeRxStartDelay =  100;
const int cSimToeTxStartDelay =  100;
const int cGraceTime          = 2500;

const int cNrSegToSend  = 5;
const int cNrSessToSend = 2;

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
unsigned int    gSimCycCnt    = 0;
bool            gTraceEvent   = false;
bool            gFatalError   = false;
unsigned int    gMaxSimCycles = cSimToeStartupDelay + cSimToeRxStartDelay +
                                cSimToeTxStartDelay + cGraceTime;

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

//OBSOLETE_20210305 #define DEFAULT_SESSION_ID      42
#define DEFAULT_SESSION_ID       0
#define DEFAULT_SESSION_LEN     32

#endif

/*! \} */
