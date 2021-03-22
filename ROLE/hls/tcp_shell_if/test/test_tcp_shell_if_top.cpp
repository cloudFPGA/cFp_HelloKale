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

/******************************************************************************
 * @file       : test_tcp_shell_if_top.cpp
 * @brief      : Testbench for the toplevel of the TCP Shell Interface (TSIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_Monolithic / ROLE
 * Language    : Vivado HLS
 *
 * \ingroup ROLE
 * \addtogroup ROLE_TSIF_TEST
 * \{
 ******************************************************************************/

#include "test_tcp_shell_if_top.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//---------------------------------------------------------
#define THIS_NAME "TB_TSIF_TOP"


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
 * @brief Main function for the test of the TOP of TCP Shell Interface (TSIF).
 ******************************************************************************/
int main(int argc, char *argv[]) {

    //------------------------------------------------------
    //-- TESTBENCH GLOBAL VARIABLES
    //------------------------------------------------------
    gSimCycCnt = 0;  // Simulation cycle counter as a global variable

    //------------------------------------------------------
    //-- DUT SIGNAL INTERFACES
    //------------------------------------------------------
    //-- SHL / Mmio Interface
    CmdBit              sMMIO_TSIF_Enable;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------
    //-- TAF / Rx Data Interface
    stream<AxisRaw>       ssTAF_TSIF_Data     ("ssTAF_TSIF_Data");
    stream<TcpSessId>     ssTAF_TSIF_SessId   ("ssTAF_TSIF_SessId");
    stream<TcpDatLen>     ssTAF_TSIF_DatLen   ("ssTAF_TSIF_DatLen");
    //-- TSIF / Tx Data Interface
    stream<AxisRaw>       ssTSIF_TAF_Data     ("ssTSIF_TAF_Data");
    stream<TcpSessId>     ssTSIF_TAF_SessId   ("ssTSIF_TAF_SessId");
    stream<TcpDatLen>     ssTSIF_TAF_DatLen   ("ssTSIF_TAF_DatLen");
    //-- TOE  / Rx Data Interfaces
    stream<TcpAppNotif>   ssTOE_TSIF_Notif    ("ssTOE_TSIF_Notif");
    stream<AxisRaw>       ssTOE_TSIF_Data     ("ssTOE_TSIF_Data");
    stream<TcpAppMeta>    ssTOE_TSIF_Meta     ("ssTOE_TSIF_Meta");
    //-- TSIF / Rx Data Interface
    stream<TcpAppRdReq>   ssTSIF_TOE_DReq     ("ssTSIF_TOE_DReq");
    //-- TOE  / Listen Interface
    stream<TcpAppLsnRep>  ssTOE_TSIF_LsnRep   ("ssTOE_TSIF_LsnRep");
    //-- TSIF / Listen Interface
    stream<TcpAppLsnReq>  ssTSIF_TOE_LsnReq   ("ssTSIF_TOE_LsnReq");
    //-- TOE  / Tx Data Interfaces
    stream<TcpAppSndRep>  ssTOE_TSIF_SndRep   ("ssTOE_TSIF_SndRep");
    //-- TSIF  / Tx Data Interfaces
    stream<AxisRaw>       ssTSIF_TOE_Data     ("ssTSIF_TOE_Data");
    stream<TcpAppSndReq>  ssTSIF_TOE_SndReq   ("ssTSIF_TOE_SndReq");
    //-- TOE  / Connect Interfaces
    stream<TcpAppOpnRep>  ssTOE_TSIF_OpnRep   ("ssTOE_TSIF_OpnRep");
    //-- TSIF / Connect Interfaces
    stream<TcpAppOpnReq>  ssTSIF_TOE_OpnReq   ("ssTSIF_TOE_OpnReq");
    stream<TcpAppClsReq>  ssTSIF_TOE_ClsReq   ("ssTSIF_TOE_ClsReq");

    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    int nrErr  = 0;

    printInfo(THIS_NAME, "#################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_tcp_shell_top' STARTS HERE  ##\n");
    printInfo(THIS_NAME, "#################################################\n\n");
    printInfo(THIS_NAME, "  FYI - This testbench does nothing. \n");
    printInfo(THIS_NAME, "        It is just provided here for compilation purpose.\n");

    //-- Run DUT simulation
    int tbRun = 42;
    while (0) {
        tcp_shell_if_top(
            //-- SHELL / Mmio Interface
            &sMMIO_TSIF_Enable,
            //-- TAF / Rx & Tx Data Interfaces
            ssTAF_TSIF_Data,
            ssTAF_TSIF_SessId,
            ssTAF_TSIF_DatLen,
            ssTSIF_TAF_Data,
            ssTSIF_TAF_SessId,
            ssTSIF_TAF_DatLen,
            //-- TOE / Rx Data Interfaces
            ssTOE_TSIF_Notif,
            ssTSIF_TOE_DReq,
            ssTOE_TSIF_Data,
            ssTOE_TSIF_Meta,
            //-- TOE / Listen Interfaces
            ssTSIF_TOE_LsnReq,
            ssTOE_TSIF_LsnRep,
            //-- TOE / Tx Data Interfaces
            ssTSIF_TOE_Data,
            ssTSIF_TOE_SndReq,
            ssTOE_TSIF_SndRep,
            //-- TOE / Tx Open Interfaces
            ssTSIF_TOE_OpnReq,
            ssTOE_TSIF_OpnRep,
            //-- TOE / Close Interfaces
            ssTSIF_TOE_ClsReq);
        tbRun--;
        stepSim();
    }

    printInfo(THIS_NAME, "#########################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_tcp_shell_if_top' ENDS HERE (RC=0)  ##\n");
    printInfo(THIS_NAME, "#########################################################\n\n");

    return 0;  // Always

}

/*! \} */
