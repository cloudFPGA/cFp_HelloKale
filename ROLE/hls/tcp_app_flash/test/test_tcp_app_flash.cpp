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
 * @file       : test_tcp_app_flash.cpp
 * @brief      : Testbench for TCP Application Flash (TAF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_HelloKale/ROLE/TcpApplicationFlash (TAF)
 * Language    : Vivado HLS
 *
 *               +-----------------------+
 *               |  UdpApplicationFlash  |     +--------+
 *               |        (TAF)          <-----+  MMIO  |
 *               +-----/|\------+--------+     +--------+
 *                      |       |
 *                      |       ||
 *               +------+------\|/-------+
 *               |   UdpShellInterface   |
 *               |       (TSIF)          |
 *               +-----/|\------+--------+
 *                      |       |
 *                      |      \|/
 *
 * \ingroup tcp_app_flash
 * \addtogroup tcp_app_flash
 * \{
 *******************************************************************************/

#include "test_tcp_app_flash.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//---------------------------------------------------------
#define THIS_NAME "TB_TAF"

#ifndef __SYNTHESIS__
  extern unsigned int gSimCycCnt;
  extern unsigned int gMaxSimCycles;
#endif

/******************************************************************************
 * @brief Main function for the test of the TCP Application Flash (TAF).
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
  #if defined TAF_USE_NON_FIFO_IO
    ap_uint<2> sMMIO_TAF_EchoCtrl;
  #endif
    //[NOT_USED] CmdBit     sMMIO_TAF_PostSegEn;
    //[NOT_USED] CmdBit     sMMIO_TAF_CaptSegEn;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------
    //-- TSIF / TCP Data Interfaces
    stream<TcpAppData>  ssTSIF_TAF_Data   ("ssTSIF_TAF_Data");
    stream<TcpSessId>   ssTSIF_TAF_SessId ("ssTSIF_TAF_SessId");
    stream<TcpDatLen>   ssTSIF_TAF_DatLen ("ssTSIF_TAF_DatLen");
    stream<TcpAppData>  ssTAF_TSIF_Data   ("ssTAF_TSIF_Data");
    stream<TcpSessId>   ssTAF_TSIF_SessId ("ssTAF_TSIF_SessId");
    stream<TcpDatLen>   ssTAF_TSIF_DatLen ("ssTAF_TSIF_DatLen");

    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    int nrErr  = 0;
    int segCnt = 0;

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_tcp_app_flash' STARTS HERE                             ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "This testbench will be executed with the following parameters: \n");
    for (int i=1; i<argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
    }
    printf("\n\n");

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {
        //-------------------------------------------------
        //-- EMULATE TSIF
        //-------------------------------------------------
        pTSIF(
            nrErr,
            //-- MMIO / Configuration Interfaces
          #if defined TAF_USE_NON_FIFO_IO
            sMMIO_TAF_EchoCtrl,
          #endif
            //-- TAF / TCP Data Interfaces
            ssTSIF_TAF_Data,
            ssTSIF_TAF_SessId,
            ssTSIF_TAF_DatLen,
            //-- TAF / TCP Data Interface
            ssTAF_TSIF_Data,
            ssTAF_TSIF_SessId,
            ssTAF_TSIF_DatLen);

        //-------------------------------------------------
        //-- RUN DUT
        //-------------------------------------------------
        tcp_app_flash(
          #if defined TAF_USE_NON_FIFO_IO
            //-- MMIO / Configuration Interfaces
            sMMIO_TAF_EchoCtrl,
          #endif
            //-- TSIF / TCP Rx Data Interface
            ssTSIF_TAF_Data,
            ssTSIF_TAF_SessId,
            ssTSIF_TAF_DatLen,
            //-- TSIF / TCP Tx Data Interface
            ssTAF_TSIF_Data,
            ssTAF_TSIF_SessId,
            ssTAF_TSIF_DatLen);

        //------------------------------------------------------
        //-- INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        stepSim();

    } while ( (gSimCycCnt < gMaxSimCycles) and
             !gFatalError and
            (nrErr < 10) );

    //---------------------------------------------------------------
    //-- PRINT TESTBENCH STATUS
    //---------------------------------------------------------------
    printf("\n\n");
    if (argc > 1) {
        printInfo(THIS_NAME, "This testbench was executed with the following test-file: \n");
        printInfo(THIS_NAME, "\t==> %s\n\n", argv[1]);
    }

    if (nrErr) {
        printError(THIS_NAME, "###########################################################\n");
        printError(THIS_NAME, "#### TEST BENCH FAILED : TOTAL NUMBER OF ERROR(S) = %2d ####\n", nrErr);
        printError(THIS_NAME, "###########################################################\n\n");

        printInfo(THIS_NAME, "FYI - You may want to check for \'ERROR\' and/or \'WARNING\' alarms in the LOG file...\n\n");
    }
        else {
        printInfo(THIS_NAME, "#############################################################\n");
        printInfo(THIS_NAME, "####               SUCCESSFUL END OF TEST                ####\n");
        printInfo(THIS_NAME, "#############################################################\n");
    }

    return(nrErr);

}

/*! \} */
