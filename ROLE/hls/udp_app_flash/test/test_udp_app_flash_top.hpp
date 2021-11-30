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
 * @file       : test_udp_app_flash_top.hpp
 * @brief      : Testbench for toplevel of the UDP Application Flash.
 *
 * System:     : cloudFPGA
 * Component   : cFp_HelloKale/ROLE/UdpApplicationFlash (UAF)
 * Language    : Vivado HLS
 *
 * \ingroup udp_app_flash
 * \addtogroup udp_app_flash
 * \{
 *******************************************************************************/

#ifndef _TEST_UAF_TOP_H_
#define _TEST_UAF_TOP_H_

#include "../src/udp_app_flash_top.hpp"
#include "./simu_udp_app_flash_env.hpp"

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
unsigned int    gSimCycCnt    = 0;
bool            gTraceEvent   = false;
bool            gFatalError   = false;
unsigned int    gMaxSimCycles = TB_MAX_CYCLES + TB_GRACE_TIME;

#endif

/*! \} */





