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
 * @file       : test_tcp_app_flash.hpp
 * @brief      : Testbench for TCP Application Flash.
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE/TcpApplicationFlash (TAF)
 * Language    : Vivado HLS
 *
 * \ingroup ROLE_TAF
 * \addtogroup ROLE_TAF_TEST
 * \{
 *****************************************************************************/

#ifndef _TEST_TAF_H_
#define _TEST_TAF_H_

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <hls_stream.h>

#include "../src/tcp_app_flash.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/SimNtsUtils.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/SimAppData.hpp"

//------------------------------------------------------
//-- TESTBENCH DEFINES
//------------------------------------------------------
#define MAX_SIM_CYCLES  500
#define TB_GRACE_TIME   1000  // Give the TB some time to finish
#define STARTUP_DELAY   25
#define VALID           true
#define UNVALID         false
#define DONE            true
#define NOT_YET_DONE    false

#define ENABLED         (ap_uint<1>)1
#define DISABLED        (ap_uint<1>)0

#define DEFAULT_SESS_ID         42
#define DEFAULT_DATAGRAM_LEN    32


//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
unsigned int    gSimCycCnt    = 0;
bool            gTraceEvent   = false;
bool            gFatalError   = false;
unsigned int    gMaxSimCycles = MAX_SIM_CYCLES + TB_GRACE_TIME;

//---------------------------------------------------------
//-- DEFAULT LOCAL FPGA AND FOREIGN HOST SOCKETS
//--  By default, the following sockets will be used by the
//--  testbench, unless the user specifies new ones via one
//--  of the test vector files.
//---------------------------------------------------------
#define DEFAULT_FPGA_IP4_ADDR   0x0A0CC801  // FPGA's local IP Address   = 10.12.200.01
#define DEFAULT_FPGA_LSN_PORT   0x2263      // TCP-ROLE listens on port  = 8803
#define DEFAULT_FPGA_SND_PORT   0xA263      // TCP-ROLE sends on port    = 41571
#define DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // HOST's foreign IP Address = 10.12.200.50
#define DEFAULT_HOST_LSN_PORT   0x80        // HOST listens on port      = 128
#define DEFAULT_HOST_SND_PORT   0x8080      // HOST sends on port        = 32896

#endif

/*! \} */
