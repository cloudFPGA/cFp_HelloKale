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
 * @file       : test_udp_shell_if.hpp
 * @brief      : Testbench for the UDP Shell Interface (USIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE
 * Language    : Vivado HLS
 *
 * \ingroup ROLE
 * \addtogroup ROLE_USIF
 * \{
 *******************************************************************************/

#ifndef _TEST_USIF_H_
#define _TEST_USIF_H_

#include <cstdlib>
#include <hls_stream.h>
#include <iostream>

#include "../src/udp_shell_if.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts_utils.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/SimNtsUtils.hpp"

//------------------------------------------------------
//-- TESTBENCH DEFINITIONS
//------------------------------------------------------
#define UOE_INIT_CYCLES  100  // FYI - It takes 0xFFFF cycles to initialize UOE.
#define GRACE_TIME       500  // Give the TB some time to finish

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
unsigned int    gSimCycCnt    = 0;
bool            gTraceEvent   = false;
bool            gFatalError   = false;
unsigned int    gMaxSimCycles = UOE_INIT_CYCLES + GRACE_TIME;

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

#endif

/*! \} */





