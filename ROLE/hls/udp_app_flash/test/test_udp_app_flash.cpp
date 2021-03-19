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
 * @brief      : Testbench for the UDP Application Flash (UAF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_Monolithic/ROLE/UdpApplicationFlash (UAF)
 * Language    : Vivado HLS
 *
 *               +-----------------------+
 *               |  UdpApplicationFlash  |     +--------+
 *               |        (UAF)          <-----+  MMIO  |
 *               +-----/|\------+--------+     +--------+
 *                      |       |
 *                      |       ||
 *               +------+------\|/-------+
 *               |   UdpShellInterface   |
 *               |       (USIF)          |
 *               +-----/|\------+--------+
 *                      |       |
 *                      |      \|/
 *
 * \ingroup ROLE_UAF
 * \addtogroup ROLE_UAF_TEST
 * \{
 *******************************************************************************/

#include "test_udp_app_flash.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB_UAF"

#ifndef __SYNTHESIS__
  extern unsigned int gSimCycCnt;
  extern unsigned int gMaxSimCycles;
#endif

/*******************************************************************************
 * @brief Main function for the test of the UDP Application Flash (UAF).
 *******************************************************************************/
int main(int argc, char *argv[]) {

    gSimCycCnt = 0;

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int      nrErr  = 0;
    EchoCtrl tbMode = ECHO_PATH_THRU; // Indicates TB testing mode.

    //------------------------------------------------------
    //-- NONE-STREAM-BASED INTERFACES
    //------------------------------------------------------
    // INFO - The UAF is specified to use the block-level IO protocol
    // 'ap_ctrl_none'. This core uses also a few configurations signals
    // which are not stream-based and which prevent the design from
    // being verified in C/RTL co-simulation mode. In oder to comply
    // with the 'Interface Synthesis Requirements' of UG902, the design
    // is compiled with a preprocessor macro that statically defines the
    // testbench mode of operation. This avoid the following error issued
    // when trying to C/RTL co-simulate this component:
    //   @E [SIM-345] Cosim only supports the following 'ap_ctrl_none'
    //      designs: (1) combinational designs; (2) pipelined design with
    //      task interval of 1; (3) designs with array streaming or
    //      hls_stream ports.
    //   @E [SIM-4] *** C/RTL co-simulation finished: FAIL **
   //------------------------------------------------------
#if TB_MODE == 1
    ap_uint<2>          sSHL_UAF_Mmio_EchoCtrl  = ECHO_STORE_FWD;
    ap_uint<1>          sSHL_UAF_Mmio_PostPktEn = 0;
    ap_uint<1>          sSHL_UAF_Mmio_CaptPktEn = 0;
#elif TB_MODE == 2
    ap_uint<2>          sSHL_UAF_Mmio_EchoCtrl  = ECHO_OFF;
    ap_uint<1>          sSHL_UAF_Mmio_PostPktEn = 0;
    ap_uint<1>          sSHL_UAF_Mmio_CaptPktEn = 0;
#else
    ap_uint<2>          sSHL_UAF_Mmio_EchoCtrl  = ECHO_PATH_THRU;
    ap_uint<1>          sSHL_UAF_Mmio_PostPktEn = 0;
    ap_uint<1>          sSHL_UAF_Mmio_CaptPktEn = 0;
#endif

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    stream<UdpAppData>  ssUSIF_UAF_Data  ("ssUSIF_UAF_Data");
    stream<UdpAppMetb>  ssUSIF_UAF_Meta  ("ssUSIF_UAF_Meta");
    stream<UdpAppData>  ssUAF_USIF_Data  ("ssUAF_USIF_Data");
    stream<UdpAppMetb>  ssUAF_USIF_Meta  ("ssUAF_USIF_Meta");
    stream<UdpAppDLen>  ssUAF_USIF_DLen  ("ssUAF_USIF_DLen");

    //------------------------------------------------------
    //-- PARSING THE TESBENCH ARGUMENTS
    //------------------------------------------------------
    if (argc < 3) {
        printFatal(THIS_NAME, "Expected a minimum of 2 parameters with the following synopsis:\n \t\t mode(0|1|2)   siUAF_<Filename>.dat\n");
    }
    tbMode = EchoCtrl(atoi(argv[1]));
    if (tbMode != sSHL_UAF_Mmio_EchoCtrl) {
        printFatal(THIS_NAME, "tbMode (%d) does not match TB_MODE (%d). Modify the CFLAG and re-compile.\n", tbMode, TB_MODE);
    }

    switch (tbMode) {
    case ECHO_PATH_THRU:
    case ECHO_STORE_FWD:
        break;
    case ECHO_OFF:
        printFatal(THIS_NAME, "The 'ECHO_OFF' mode is no longer supported since the removal of the MMIO EchoCtrl bits. \n");
        break;
    default:
        printFatal(THIS_NAME, "Unknown testing mode '%d' (or not yet implemented). \n");
    }

    printf("#####################################################\n");
    printf("## TESTBENCH STARTS HERE                           ##\n");
    printf("#####################################################\n");
    printInfo(THIS_NAME, "This testbench will be executed with the following parameters: \n");
    printInfo(THIS_NAME, "\t==> TB Mode  = %c\n", *argv[1]);
    for (int i=2; i<argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
    }
    printf("\n");

    if ((tbMode == ECHO_PATH_THRU) or (tbMode == ECHO_STORE_FWD)) {

        //-- DUT OUTPUT TRAFFIC AS STREAMS ------------------------------
        string  ofsUSIF_Data_FileName      = "../../../../test/simOutFiles/soUSIF_Data.dat";
        string  ofsUSIF_Meta_FileName      = "../../../../test/simOutFiles/soUSIF_Meta.dat";
        string  ofsUSIF_DLen_FileName      = "../../../../test/simOutFiles/soUSIF_DLen.dat";
        string  ofsUSIF_Data_Gold_FileName = "../../../../test/simOutFiles/soUSIF_Data_Gold.dat";
        string  ofsUSIF_Meta_Gold_FileName = "../../../../test/simOutFiles/soUSIF_Meta_Gold.dat";
        string  ofsUSIF_DLen_Gold_FileName = "../../../../test/simOutFiles/soUSIF_DLen_Gold.dat";

        vector<string>   ofNames;
        ofNames.push_back(ofsUSIF_Data_FileName);
        ofNames.push_back(ofsUSIF_Meta_FileName);
        ofNames.push_back(ofsUSIF_DLen_FileName);
        ofstream         ofStreams[ofNames.size()]; // Stored in the same order

        //-- STEP-1: The setting of the ECHO mode is already done via CFLAGS
        if (tbMode == ECHO_PATH_THRU) {
            printInfo(THIS_NAME, "### TEST_MODE = ECHO_PATH_THRU #########\n");
        }
        else if (tbMode == ECHO_STORE_FWD) {
            printInfo(THIS_NAME, "### TEST_MODE = ECHO_STORE_FWD #########\n");
        }

        //-- STEP-2: Remove previous old files and open new files
        for (int i = 0; i < ofNames.size(); i++) {
            remove(ofNames[i].c_str());
            if (not isDatFile(ofNames[i])) {
                printError(THIS_NAME, "File \'%s\' is not of type \'DAT\'.\n", ofNames[i].c_str());
                nrErr++;
                continue;
            }
            if (!ofStreams[i].is_open()) {
                ofStreams[i].open(ofNames[i].c_str(), ofstream::out);
                if (!ofStreams[i]) {
                    printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofNames[i].c_str());
                    nrErr++;
                    continue;
                }
            }
        }

        //-- STEP-3: Create golden Tx files
        queue<UdpAppMetb>   udpMetaQueue;
        if (NTS_OK == createGoldenTxFiles(tbMode, string(argv[2]), udpMetaQueue,
                ofsUSIF_Data_Gold_FileName, ofsUSIF_Meta_Gold_FileName, ofsUSIF_DLen_Gold_FileName) != NTS_OK) {
            printError(THIS_NAME, "Failed to create golden Tx files. \n");
            nrErr++;
        }

        //-- STEP-4: Create the USIF->UAF INPUT {Data,Meta} as streams
        int nrUSIF_UAF_Chunks=0;
        if (not createUdpRxTraffic(ssUSIF_UAF_Data, "ssUSIF_UAF_Data",
                                   ssUSIF_UAF_Meta, "ssUSIF_UAF_Meta",
                                   string(argv[2]),
                                   udpMetaQueue,
                                   nrUSIF_UAF_Chunks)) {
            printFatal(THIS_NAME, "Failed to create the USIF->UAF traffic as streams.\n");
        }

        //-- STEP-5: Run simulation
        int tbRun = (nrErr == 0) ? (nrUSIF_UAF_Chunks + TB_GRACE_TIME) : 0;
        while (tbRun) {
            udp_app_flash(
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
        printInfo(THIS_NAME, "## TESTBENCH 'test_udp_app_flash' ENDS HERE                               ##\n");
        printInfo(THIS_NAME, "############################################################################\n");
        stepSim();

        //-- STEP-6a: Drain UAF-->USIF DATA OUTPUT STREAM
        int nrUAF_USIF_DataChunks=0, nrUAF_USIF_DataGrams=0, nrUAF_USIF_DataBytes=0;
        if (not drainAxisToFile(ssUAF_USIF_Data, "ssUAF_USIF_Data",
            ofNames[0], nrUAF_USIF_DataChunks, nrUAF_USIF_DataGrams, nrUAF_USIF_DataBytes)) {
            printError(THIS_NAME, "Failed to drain UAF-to-USIF data traffic from DUT. \n");
            nrErr++;
        }
        else {
            printInfo(THIS_NAME, "Done with the draining of the UAF-to-USIF data traffic:\n");
            printInfo(THIS_NAME, "\tReceived %d chunks in %d datagrams, for a total of %d bytes.\n\n",
                      nrUAF_USIF_DataChunks, nrUAF_USIF_DataGrams, nrUAF_USIF_DataBytes);
        }
        //-- STEP-6b: Drain UAF-->USIF META OUTPUT STREAM
        int nrUAF_USIF_MetaChunks=0, nrUAF_USIF_MetaGrams=0, nrUAF_USIF_MetaBytes=0;
        if (not drainUdpMetaStreamToFile(ssUAF_USIF_Meta, "ssUAF_USIF_Meta",
                ofNames[1], nrUAF_USIF_MetaChunks, nrUAF_USIF_MetaGrams, nrUAF_USIF_MetaBytes)) {
            printError(THIS_NAME, "Failed to drain UAF-to-USIF meta traffic from DUT. \n");
            nrErr++;
        }
        //-- STEP-6c: Drain UAF-->USIF DLEN OUTPUT STREAM
        int nrUAF_USIF_DLenChunks=0, nrUAF_USIF_DLenGrams=0, nrUAF_USIF_DLenBytes=0;
        if (not drainUdpDLenStreamToFile(ssUAF_USIF_DLen, "ssUAF_USIF_DLen",
                ofNames[2], nrUAF_USIF_DLenChunks, nrUAF_USIF_DLenGrams, nrUAF_USIF_DLenBytes)) {
            printError(THIS_NAME, "Failed to drain UAF-to-USIF dlen traffic from DUT. \n");
            nrErr++;
        }

        //-- STEP-7: Compare output DAT vs gold DAT
        int res = myDiffTwoFiles(std::string(ofsUSIF_Data_FileName),
                                 std::string(ofsUSIF_Data_Gold_FileName));
        if (res) {
            printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
                       ofsUSIF_Data_FileName.c_str(), ofsUSIF_Data_Gold_FileName.c_str());
            nrErr += 1;
        }
        res = myDiffTwoFiles(std::string(ofsUSIF_DLen_FileName),
                             std::string(ofsUSIF_DLen_Gold_FileName));
        if (res) {
            printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
                       ofsUSIF_DLen_FileName.c_str(), ofsUSIF_DLen_Gold_FileName.c_str());
            nrErr += 1;
        }

    }  // End-of: if (tbMode == ECHO_PATH_THRU or ECHO_STORE_FWD) {


    //---------------------------------------------------------------
    //-- PRINT TESTBENCH STATUS
    //---------------------------------------------------------------
    printf("\n\n");
    printInfo(THIS_NAME, "This testbench was executed with the following parameters: \n");
    printInfo(THIS_NAME, "\t==> TB Mode  = %c\n", *argv[1]);
    for (int i=2; i<argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
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

    return nrErr;

}

/*! \} */

