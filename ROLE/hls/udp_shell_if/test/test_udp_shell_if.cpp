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

/*****************************************************************************
 * @file       : test_udp_shell_if.cpp
 * @brief      : Testbench for the UDP Shell Interface (USIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_HelloKale/ROLE/UdpShellInterface (USIF)
 * Language    : Vivado HLS
 *
 *               +-----------------------+
 *               |  UdpApplicationFlash  |
 *               |        (UAF)          |
 *               +-----/|\------+--------+
 *                      |       |
 *                      |       |
 *               +------+------\|/-------+
 *               |   UdpShellInterface   |     +--------+
 *               |       (USIF)          <-----+  MMIO  |
 *               +-----/|\------+--------+     +--/|\---+
 *                      |       |                  |
 *                      |       |                  |
 *               +------+------\|/-------+         |
 *               |    UdpOffloadEngine   |         |
 *               |        (UOE)          +---------+
 *               +-----------------------+
 *
 * \ingroup udp_shell_if
 * \addtogroup udp_shell_if
 * \{
 *****************************************************************************/

#include "test_udp_shell_if.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB_USIF"

#ifndef __SYNTHESIS__
  extern unsigned int gSimCycCnt;
  extern unsigned int gMaxSimCycles;
#endif


/*******************************************************************************
 * @brief Main function for the test of the UDP Shell Interface (USIF).
 *
 * @param[in] The number of bytes to generate in 'Echo' mode [1:65535].
 * @param[in] The IP4 destination address of the remote host.
 * @param[in] The UDP destination port of the remote host.
 * @param[in] The number of bytes to generate in  or "Test' mode [1:65535].
 *
 * @info Usage example --> "512 10.11.12.13 2718 1024"
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
    stream<UdpAppData>    ssUAF_USIF_Data     ("ssUAF_USIF_Data");
    stream<UdpAppMeta>    ssUAF_USIF_Meta     ("ssUAF_USIF_Meta");
    stream<UdpAppDLen>    ssUAF_USIF_DLen     ("ssUAF_USIF_DLen");
    //-- USIF->UOE / UDP Tx Data Interface
    stream<UdpAppData>    ssUSIF_UOE_Data     ("ssUSIF_UOE_Data");
    stream<UdpAppMeta>    ssUSIF_UOE_Meta     ("ssUSIF_UOE_Meta");
    stream<UdpAppDLen>    ssUSIF_UOE_DLen     ("ssUSIF_UOE_DLen");
    //-- UOE->USIF / UDP Rx Data Interface
    stream<UdpAppData>    ssUOE_USIF_Data     ("ssUOE_USIF_Data");
    stream<UdpAppMeta>    ssUOE_USIF_Meta     ("ssUOE_USIF_Meta");
    stream<UdpAppDLen>    ssUOE_USIF_DLen     ("ssUOE_USIF_DLen");
    //-- USIF->UAF / UDP Rx Data Interface
    stream<UdpAppData>    ssUSIF_UAF_Data     ("ssUSIF_UAF_Data");
    stream<UdpAppMeta>    ssUSIF_UAF_Meta     ("ssUSIF_UAF_Meta");
    stream<UdpAppDLen>    ssUSIF_UAF_DLen     ("ssUSIF_UAF_DLen");
    //-- UOE / Control Port Interfaces
    stream<UdpPort>       ssUSIF_UOE_LsnReq   ("ssUSIF_UOE_LsnReq");
    stream<StsBool>       ssUOE_USIF_LsnRep   ("ssUOE_USIF_LsnRep");
    stream<UdpPort>       ssUSIF_UOE_ClsReq   ("ssUSIF_UOE_ClsReq");
    stream<StsBool>       ssUOE_USIF_ClsRep   ("ssUOE_USIF_ClsRep");

    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    int         nrErr = 0;   // Total number of testbench errors
    ofstream    ofUOE_Data;  // Data streams delivered to UOE
    ofstream    ofUOE_Meta;  // Meta streams delivered to UOE
    string      ofUOE_DataName = "../../../../test/simOutFiles/soUOE_Data.dat";
    string      ofUOE_MetaName = "../../../../test/simOutFiles/soUOE_Meta.dat";
    ofstream    ofUOE_DataGold;  // Data gold streams to compare with
    ofstream    ofUOE_MetaGold;  // Meta gold streams to compare with
    string      ofUOE_DataGoldName = "../../../../test/simOutFiles/soUOE_DataGold.dat";
    string      ofUOE_MetaGoldName = "../../../../test/simOutFiles/soUOE_MetaGold.dat";

    const int   defaultLenOfDatagramEcho = 42;
    const int   defaultDestHostIpv4Test  = 0xC0A80096; // 192.168.0.150
    const int   defaultDestHostPortTest  = 2718;
    const int   defaultLenOfDatagramTest = 43;

    int         echoLenOfDatagram = defaultLenOfDatagramEcho;
    ap_uint<32> testDestHostIpv4  = defaultDestHostIpv4Test;
    ap_uint<16> testDestHostPort  = defaultDestHostIpv4Test;
    int         testLenOfDatagram = defaultLenOfDatagramTest;

    //------------------------------------------------------
    //-- PARSING THE TESBENCH ARGUMENTS
    //------------------------------------------------------
    if (argc >= 2) {
        echoLenOfDatagram = atoi(argv[1]);
        if ((echoLenOfDatagram < 1) or (echoLenOfDatagram >= 0x10000)) {
            printFatal(THIS_NAME, "Argument 'len' is out of range [1:65535].\n");
            return NTS_KO;
        }
    }
    if (argc >= 3) {
        if (isDottedDecimal(argv[2])) {
            testDestHostIpv4 = myDottedDecimalIpToUint32(argv[2]);
        }
        else {
            testDestHostIpv4 = atoi(argv[2]);
        }
        if ((testDestHostIpv4 < 0x00000000) or (testDestHostIpv4 > 0xFFFFFFFF)) {
            printFatal(THIS_NAME, "Argument 'IPv4' is out of range [0x00000000:0xFFFFFFFF].\n");
            return NTS_KO;
        }
    }
    if (argc >= 4) {
        testDestHostPort = atoi(argv[3]);
        if ((testDestHostPort < 0x0000) or (testDestHostPort >= 0x10000)) {
            printFatal(THIS_NAME, "Argument 'port' is out of range [0:65535].\n");
            return NTS_KO;
        }
    }
    if (argc >= 5) {
            testLenOfDatagram = atoi(argv[4]);
            if ((testLenOfDatagram <= 0) or (testLenOfDatagram >= 0x10000)) {
                printFatal(THIS_NAME, "Argument 'len' is out of range [0:65535].\n");
                return NTS_KO;
            }
    }

    SockAddr testSock(testDestHostIpv4, testDestHostPort);

    //------------------------------------------------------
    //-- REMOVE PREVIOUS OLD SIM FILES and OPEN NEW ONES
    //------------------------------------------------------
    remove(ofUOE_DataName.c_str());
    if (!ofUOE_Data.is_open()) {
        ofUOE_Data.open(ofUOE_DataName.c_str(), ofstream::out);
        if (!ofUOE_Data) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofUOE_DataName.c_str());
            return(NTS_KO);
        }
    }
    remove(ofUOE_MetaName.c_str());
    if (!ofUOE_Meta.is_open()) {
        ofUOE_Meta.open(ofUOE_MetaName.c_str(), ofstream::out);
        if (!ofUOE_Meta) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofUOE_MetaName.c_str());
            return(NTS_KO);
        }
    }
    remove(ofUOE_DataGoldName.c_str());
    if (!ofUOE_DataGold.is_open()) {
        ofUOE_DataGold.open(ofUOE_DataGoldName.c_str(), ofstream::out);
        if (!ofUOE_DataGold) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofUOE_DataGoldName.c_str());
            return(NTS_KO);
        }
    }
    remove(ofUOE_MetaGoldName.c_str());
    if (!ofUOE_MetaGold.is_open()) {
        ofUOE_MetaGold.open(ofUOE_MetaGoldName.c_str(), ofstream::out);
        if (!ofUOE_MetaGold) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofUOE_MetaGoldName.c_str());
            return(NTS_KO);
        }
    }

    printInfo(THIS_NAME,
            "############################################################################\n");
    printInfo(THIS_NAME,
            "## TESTBENCH 'test_udp_shell' STARTS HERE                                 ##\n");
    printInfo(THIS_NAME,
            "############################################################################\n\n");
    if (argc > 1) {
        printInfo(THIS_NAME,
                "This testbench will be executed with the following parameters: \n");
        for (int i = 1; i < argc; i++) {
            printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i - 1), argv[i]);
        }
    }

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {
        //-------------------------------------------------
        //-- EMULATE SHELL/NTS/UOE
        //-------------------------------------------------
        pUOE(
            nrErr,
            ofUOE_DataGold,
            ofUOE_Data,
            ofUOE_MetaGold,
            ofUOE_Meta,
            echoLenOfDatagram,
            testSock,
            testLenOfDatagram,
            //-- UOE / Ready Signal
            &sUOE_MMIO_Ready,
            //-- UOE->USIF / UDP Rx Data Interfaces
            ssUOE_USIF_Data,
            ssUOE_USIF_Meta,
            ssUOE_USIF_DLen,
            //-- USIF->UOE / UDP Tx Data Interfaces
            ssUSIF_UOE_Data,
            ssUSIF_UOE_Meta,
            ssUSIF_UOE_DLen,
            //-- USIF<->UOE / Listen Interfaces
            ssUSIF_UOE_LsnReq,
            ssUOE_USIF_LsnRep,
            //-- USIF<->UOE / Close Interfaces
            ssUSIF_UOE_ClsReq);

        //-------------------------------------------------
        //-- EMULATE SHELL/MMIO
        //-------------------------------------------------
        pMMIO(
            //-- UOE / Ready Signal
            &sUOE_MMIO_Ready,
            //-- MMIO / Enable Layer-7 (.i.e APP alias ROLE)
            &sMMIO_USIF_Enable);

        //-------------------------------------------------
        //-- RUN DUT
        //-------------------------------------------------
        udp_shell_if(
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
            ssUOE_USIF_DLen,
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
            ssUSIF_UAF_Meta,
            ssUSIF_UAF_DLen);

        //-------------------------------------------------
        //-- EMULATE ROLE/UdpApplicationFlash (UAF)
        //-------------------------------------------------
        pUAF(
            //-- USIF / Data Interface
            ssUSIF_UAF_Data,
            ssUSIF_UAF_Meta,
            //-- UAF / Data Interface
            ssUAF_USIF_Data,
            ssUAF_USIF_Meta,
            ssUAF_USIF_DLen);

        //------------------------------------------------------
        //-- INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        stepSim();

    } while ( (gSimCycCnt < gMaxSimCycles) or gFatalError or (nrErr > 10) );

    //---------------------------------
    //-- CLOSING OPEN FILES
    //---------------------------------
    ofUOE_DataGold.close();
    ofUOE_Data.close();
    ofUOE_MetaGold.close();
    ofUOE_Meta.close();

    printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
    printf("############################################################################\n");
    printf("## TESTBENCH 'test_udp_shell_if' ENDS HERE                                ##\n");
    printf("############################################################################\n");

    //---------------------------------------------------------------
    //-- PRINT TESTBENCH STATUS
    //---------------------------------------------------------------
    printf("\n");
    printInfo(THIS_NAME, "This testbench was executed with the following parameters: \n");
    for (int i=1; i<argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
    }
    printf("\n");

    //---------------------------------------------------------------
    //-- COMPARE THE RESULTS FILES WITH GOLDEN FILES
    //---------------------------------------------------------------
    int res = myDiffTwoFiles(std::string(ofUOE_DataName),
                             std::string(ofUOE_DataGoldName));
    if (res) {
        printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
                   ofUOE_DataName.c_str(), ofUOE_DataGoldName.c_str());
        nrErr += 1;
    }
    res = myDiffTwoFiles(std::string(ofUOE_MetaName),
                         std::string(ofUOE_MetaGoldName));
    if (res) {
        printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
                   ofUOE_MetaName.c_str(), ofUOE_MetaGoldName.c_str());
        nrErr += 1;
    }

    if (nrErr) {
         printError(THIS_NAME, "###############################################################################\n");
         printError(THIS_NAME, "#### TESTBENCH 'test_udp_shell_if' FAILED : TOTAL NUMBER OF ERROR(S) = %2d ####\n", nrErr);
         printError(THIS_NAME, "###############################################################################\n");

         printInfo(THIS_NAME, "FYI - You may want to check for \'ERROR\' and/or \'WARNING\' alarms in the LOG file...\n\n");
    }
         else {
         printInfo(THIS_NAME, "#############################################################\n");
         printInfo(THIS_NAME, "####        SUCCESSFUL END OF 'test_udp_shell_if'        ####\n");
         printInfo(THIS_NAME, "#############################################################\n");
     }

    return(nrErr);

}

/*! \} */
