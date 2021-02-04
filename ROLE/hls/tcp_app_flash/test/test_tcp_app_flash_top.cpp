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
 * @file       : test_tcp_app_flash_top.cpp
 * @brief      : Testbench for for toplevel of the TCP Application Flash (TAF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_Monolithic /ROLE
 * Language    : Vivado HLS
 *
 *
 * \ingroup ROLE_TAF
 * \addtogroup ROLE_TAF_TEST
 * \{
 *******************************************************************************/

#include "test_tcp_app_flash_top.hpp"

using namespace hls;
using namespace std;

#define THIS_NAME "TB_TAF_TOP"

/******************************************************************************
 * @brief Increment the simulation counter
 ******************************************************************************/
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

/******************************************************************************
 * @brief Main function for the test of the TCP Application Flash (TAF) TOP.
 ******************************************************************************/
int main(int argc, char *argv[]) {

    //------------------------------------------------------
    //-- TESTBENCH GLOBAL VARIABLES
    //------------------------------------------------------
    gSimCycCnt = 0;  // Simulation cycle counter as a global variable

    //------------------------------------------------------
    //-- DUT SIGNAL INTERFACES
    //------------------------------------------------------
    //-- MMIO/ Configuration Interfaces
    ap_uint<2>          sMMIO_TAF_EchoCtrl;
    //[NOT_USED] CmdBit sMMIO_TAF_PostSegEn;
    //[NOT_USED] CmdBit sMMIO_TAF_CaptSegEn;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    stream<AxisRaw>     ssTSIF_TAF_Data   ("ssTSIF_TAF_Data");
    stream<TcpSessId>   ssTSIF_TAF_SessId ("ssTSIF_TAF_SessId");
    stream<TcpDatLen>   ssTSIF_TAF_DatLen ("ssTSIF_TAF_DatLen");
    stream<AxisRaw>     ssTAF_TSIF_Data   ("ssTAF_TSIF_Data");
    stream<TcpSessId>   ssTAF_TSIF_SessId ("ssTAF_TSIF_SessId");
    stream<TcpDatLen>   ssTAF_TSIF_DatLen ("ssTAF_TSIF_DatLen");

    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    int nrErr  = 0;
    int segCnt = 0;

    printInfo(THIS_NAME, "#######################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_tcp_app_flash_flash' STARTS HERE  ##\n");
    printInfo(THIS_NAME, "#######################################################\n");
    printInfo(THIS_NAME, "  FYI - This testbench does nothing. \n");
    printInfo(THIS_NAME, "        It is just provided here for compilation purpose.\n\n");

    //-- Run DUT simulation
    int tbRun = 42;
    while (tbRun) {
    	tcp_app_flash_top(
            //-- SHELL / MMIO / Configuration Interfaces
            &sMMIO_TAF_EchoCtrl,
            //-- TSIF / Rx Data Interfaces
            ssTSIF_TAF_Data,
            ssTSIF_TAF_SessId,
            ssTSIF_TAF_DatLen,
            //-- TSIF / Tx Data Interfaces
            ssTAF_TSIF_Data,
            ssTAF_TSIF_SessId,
            ssTAF_TSIF_DatLen);
    	tbRun--;
    	stepSim();
    }

    printError(THIS_NAME, "###########################################################\n");
    printError(THIS_NAME, "#### TESTBENCH 'test_tcp_app_flash_top' ENDS HERE (RC=0) ##\n");
    printError(THIS_NAME, "###########################################################\n\n");

    return 0;  // Always

}

/*! \} */
