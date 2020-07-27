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
 * \ingroup ROLE
 * \addtogroup ROLE_TAF
 * \{
 *****************************************************************************/

#ifndef _TEST_TAF_H_
#define _TEST_TAF_H_

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <hls_stream.h>

#include "../src/tcp_app_flash.hpp"

using namespace hls;
using namespace std;

//------------------------------------------------------
//-- TESTBENCH DEFINES
//------------------------------------------------------
#define MAX_SIM_CYCLES  500
#define VALID           true
#define UNVALID         false
#define DONE            true
#define NOT_YET_DONE    false

#define ENABLED         (ap_uint<1>)1
#define DISABLED        (ap_uint<1>)0

#define DEFAULT_SESS_ID 42

#define STARTUP_DELAY   25

//------------------------------------------------------
// UTILITY PROTOTYPE DEFINITIONS
//------------------------------------------------------
//OBSOLETE_20200727 bool writeAxiWordToFile(AxiWord  *tcpWord, ofstream &outFileStream);
//OBSOLETE_20200727 int  writeTcpWordToFile(AxiWord  *tcpWord, ofstream &outFile);

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
unsigned int    gSimCycCnt    = 0;
bool            gTraceEvent   = false;
bool            gFatalError   = false;
unsigned int    gMaxSimCycles = MAX_SIM_CYCLES;

#endif

/*! \} */
