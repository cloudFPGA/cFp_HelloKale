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
 * @file       : test_udp_shell_if.hpp
 * @brief      : Testbench for the UDP Shell Interface (USIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_HelloKale/ROLE
 * Language    : Vivado HLS
 *
 * \ingroup udp_shell_if
 * \addtogroup udp_shell_if
 * \{
 *******************************************************************************/

#ifndef _TEST_USIF_H_
#define _TEST_USIF_H_

#include <cstdlib>
#include <hls_stream.h>
#include <iostream>

#include "../src/udp_shell_if.hpp"
#include "./simu_udp_shell_if_env.hpp"

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
unsigned int    gSimCycCnt    = 0;
bool            gTraceEvent   = false;
bool            gFatalError   = false;
unsigned int    gMaxSimCycles = cUoeInitCycles + cGraceTime;

#endif

/*! \} */





