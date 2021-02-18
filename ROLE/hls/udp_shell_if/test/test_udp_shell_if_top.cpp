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
 * @file       : test_udp_shell_if_top.cpp
 * @brief      : Testbench for the toplevel of the UDP Shell Interface (USIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_Monolithic / ROLE
 * Language    : Vivado HLS
 *
 * \ingroup ROLE
 * \addtogroup ROLE_USIF
 * \{
 *****************************************************************************/

#include "test_udp_shell_if_top.hpp"

using namespace hls;
using namespace std;

#define THIS_NAME "TB_USIF_TOP"

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
 * @brief Main function for the test of the TOP of UDP Shell Interface (USIF).
 *******************************************************************************/
int main(int argc, char *argv[]) {

    //------------------------------------------------------
    //-- TESTBENCH GLOBAL VARIABLES
    //------------------------------------------------------
    gSimCycCnt = 0;  // Simulation cycle counter as a global variable

    //------------------------------------------------------
    //-- DUT SIGNAL INTERFACES
    //------------------------------------------------------
    //-- SHL / Mmio Interface
    CmdBit              sMMIO_USIF_Enable;
    //-- UOE->MMIO / Ready Signal
    StsBit              sUOE_MMIO_Ready;
    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------
    //-- UAF->USIF / UDP Tx Data Interface
    stream<AxisRaw>       ssUAF_USIF_Data     ("ssUAF_USIF_Data");
    stream<UdpAppMeta>    ssUAF_USIF_Meta     ("ssUAF_USIF_Meta");
    stream<UdpAppDLen>    ssUAF_USIF_DLen     ("ssUAF_USIF_DLen");
    //-- USIF->UOE / UDP Tx Data Interface
    stream<AxisRaw>       ssUSIF_UOE_Data     ("ssUSIF_UOE_Data");
    stream<UdpAppMeta>    ssUSIF_UOE_Meta     ("ssUSIF_UOE_Meta");
    stream<UdpAppDLen>    ssUSIF_UOE_DLen     ("ssUSIF_UOE_DLen");
    //-- UOE->USIF / UDP Rx Data Interface
    stream<AxisRaw>       ssUOE_USIF_Data     ("ssUOE_USIF_Data");
    stream<UdpAppMeta>    ssUOE_USIF_Meta     ("ssUOE_USIF_Meta");
    //-- USIF->UAF / UDP Rx Data Interface
    stream<AxisRaw>       ssUSIF_UAF_Data     ("ssUSIF_UAF_Data");
    stream<UdpAppMeta>    ssUSIF_UAF_Meta     ("ssUSIF_UAF_Meta");
    //-- UOE / Control Port Interfaces
    stream<UdpPort>       ssUSIF_UOE_LsnReq   ("ssUSIF_UOE_LsnReq");
    stream<StsBool>       ssUOE_USIF_LsnRep   ("ssUOE_USIF_LsnRep");
    stream<UdpPort>       ssUSIF_UOE_ClsReq   ("ssUSIF_UOE_ClsReq");
    stream<StsBool>       ssUOE_USIF_ClsRep   ("ssUOE_USIF_ClsRep");

    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    int         nrErr = 0;   // Total number of testbench errors

    printInfo(THIS_NAME, "#################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_udp_shell_top' STARTS HERE  ##\n");
    printInfo(THIS_NAME, "#################################################\n\n");
    printInfo(THIS_NAME, "  FYI - This testbench does nothing. \n");
    printInfo(THIS_NAME, "        It is just provided here for compilation purpose.\n");

    //-- Run DUT simulation
    int tbRun = 10;
    while (0) {
        udp_shell_if_top(
            //-- SHELL / Mmio Interface
            &sMMIO_USIF_Enable,
            //-- SHELL / Control Port Interfaces
            ssUSIF_UOE_LsnReq,
            ssUOE_USIF_LsnRep,
            ssUSIF_UOE_ClsReq,
            ssUOE_USIF_ClsRep,
            //-- SHELL / Rx Data Interfaces
            ssUOE_USIF_Data,
            ssUOE_USIF_Meta,
            //-- SHELL / Tx Data Interfaces
            ssUSIF_UOE_Data,
            ssUSIF_UOE_Meta,
            ssUSIF_UOE_DLen,
            //-- UAF / Tx Data Interfaces
            ssUAF_USIF_Data,
            ssUAF_USIF_Meta,
            ssUAF_USIF_DLen,
            //-- UAF / Rx Data Interfaces
            ssUSIF_UAF_Data,
            ssUSIF_UAF_Meta);
        tbRun--;
        stepSim();
    }

    printInfo(THIS_NAME, "#########################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_udp_shell_if_top' ENDS HERE (RC=0)  ##\n");
    printInfo(THIS_NAME, "#########################################################\n\n");

    return 0;  // Always

}

/*! \} */
