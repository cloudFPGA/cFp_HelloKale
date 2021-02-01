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
 * @file       : test_udp_app_flash.cpp
 * @brief      : Testbench for toplevel of the UDP Application Flash (UAF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_Monolithic / ROLE
 * Language    : Vivado HLS
 *
 * \ingroup ROLE_UAF
 * \addtogroup ROLE_UAF_TEST
 * \{
 *******************************************************************************/

#include "test_udp_app_flash_top.hpp"

using namespace hls;
using namespace std;

#define THIS_NAME "TB_UAF_TOP"

/*******************************************************************************
 * @brief Increment the simulation counter
 *******************************************************************************/
void stepSim() {
    gSimCycCnt++;
    if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
        printInfo(THIS_NAME, "-- [@%4.4d] -----------------------------\n", gSimCycCnt);
        gTraceEvent = false;
    }
    else if (0) {
        printInfo(THIS_NAME, "------------------- [@%d] ------------\n", gSimCycCnt);
    }
}

/*******************************************************************************
 * @brief Main function for the test of the UDP Application Flash (UAF) TOP.
 *******************************************************************************/
int main(int argc, char *argv[]) {

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    stream<AxisRaw>     ssUSIF_UAF_Data  ("ssUSIF_UAF_Data");
    stream<UdpAppMeta>  ssUSIF_UAF_Meta  ("ssUSIF_UAF_Meta");
    stream<AxisRaw>     ssUAF_USIF_Data  ("ssUAF_USIF_Data");
    stream<UdpAppMeta>  ssUAF_USIF_Meta  ("ssUAF_USIF_Meta");
    stream<UdpAppDLen>  ssUAF_USIF_DLen  ("ssUAF_USIF_DLen");

    printInfo(THIS_NAME, "#####################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH STARTS HERE                           ##\n");
    printInfo(THIS_NAME, "#####################################################\n");
    printInfo(THIS_NAME, "  FYI - This testbench does nothing. \n");
    printInfo(THIS_NAME, "        It is just provided here for compilation purpose.\n\n");

    //-- Run DUT simulation
    int tbRun = 42;
    while (tbRun) {
    	udp_app_flash_top(
    			//-- SHELL / Mmio / Configuration Interfaces
    			//[NOT_USED] sSHL_UAF_Mmio_EchoCtrl,
    			//[NOT_USED] sSHL_UAF_Mmio_PostPktEn,
    			//[NOT_USED] sSHL_UAF_Mmio_CaptPktEn,
    			//-- USIF / Rx Data Interfaces
    			ssUSIF_UAF_Data,
				ssUSIF_UAF_Meta,
				//-- USIF / Tx Data Interfaces
				ssUAF_USIF_Data,
				ssUAF_USIF_Meta,
				ssUAF_USIF_DLen);
    	tbRun--;
    	stepSim();
    }

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_udp_app_flash_top' ENDS HERE (RC=0)                    ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    stepSim();

    return 0;  // Always

}

/*! \} */

