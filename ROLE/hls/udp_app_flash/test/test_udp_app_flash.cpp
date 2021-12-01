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
 * Component   : cFp_HelloKale/ROLE/UdpApplicationFlash (UAF)
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
 * \ingroup udp_app_flash
 * \addtogroup udp_app_flash
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
    EchoCtrl tbCtrlMode = ECHO_CTRL_DISABLED;

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
    CmdBit              sSHL_UAF_Mmio_Enable    = CMD_ENABLE;
#if TB_MODE == 0
    ap_uint<2>          sSHL_UAF_Mmio_EchoCtrl  = ECHO_CTRL_DISABLED;
    ap_uint<1>          sSHL_UAF_Mmio_PostPktEn = 0;
    ap_uint<1>          sSHL_UAF_Mmio_CaptPktEn = 0;
#elif TB_MODE == 1
    ap_uint<2>          sSHL_UAF_Mmio_EchoCtrl  = ECHO_PATH_THRU;
    ap_uint<1>          sSHL_UAF_Mmio_PostPktEn = 0;
    ap_uint<1>          sSHL_UAF_Mmio_CaptPktEn = 0;
#elif TB_MODE == 2
    ap_uint<2>          sSHL_UAF_Mmio_EchoCtrl  = ECHO_STORE_FWD;
    ap_uint<1>          sSHL_UAF_Mmio_PostPktEn = 0;
    ap_uint<1>          sSHL_UAF_Mmio_CaptPktEn = 0;
#else
    ap_uint<2>          sSHL_UAF_Mmio_EchoCtrl  = ECHO_OFF;
    ap_uint<1>          sSHL_UAF_Mmio_PostPktEn = 0;
    ap_uint<1>          sSHL_UAF_Mmio_CaptPktEn = 0;
#endif

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    stream<UdpAppData>  ssUSIF_UAF_Data  ("ssUSIF_UAF_Data");
    stream<UdpAppMeta>  ssUSIF_UAF_Meta  ("ssUSIF_UAF_Meta");
    stream<UdpAppDLen>  ssUSIF_UAF_DLen  ("ssUSIF_UAF_DLen");
    stream<UdpAppData>  ssUAF_USIF_Data  ("ssUAF_USIF_Data");
    stream<UdpAppMeta>  ssUAF_USIF_Meta  ("ssUAF_USIF_Meta");
    stream<UdpAppDLen>  ssUAF_USIF_DLen  ("ssUAF_USIF_DLen");

    //------------------------------------------------------
    //-- PARSING THE TESBENCH ARGUMENTS
    //------------------------------------------------------
    if (argc < 3) {
        printFatal(THIS_NAME, "Expected a minimum of 2 parameters with the following synopsis:\n \t\t mode(0|1|2)   siUAF_<Filename>.dat\n");
    }
    tbCtrlMode = EchoCtrl(atoi(argv[1]));
    if (tbCtrlMode != sSHL_UAF_Mmio_EchoCtrl) {
        printFatal(THIS_NAME, "tbCtrlMode (%d) does not match TB_MODE (%d). Modify the CFLAG and re-compile.\n", tbCtrlMode, TB_MODE);
    }

    switch (tbCtrlMode) {
    case ECHO_CTRL_DISABLED:
        break;
    case ECHO_PATH_THRU:
    case ECHO_STORE_FWD:
    case ECHO_OFF:
        printFatal(THIS_NAME, "The 'ECHO' mode %d is no longer supported since the removal of the MMIO EchoCtrl bits. \n", tbCtrlMode);
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

    if (tbCtrlMode == ECHO_CTRL_DISABLED) {

        //-- DUT OUTPUT TRAFFIC AS STREAMS ------------------------------
        ofstream ofsUSIF_Data;  // APP data streams delivered to USIF
        const char *ofsUSIF_Data_FileName      = "../../../../test/simOutFiles/soUSIF_Data.dat";
        ofstream ofsUSIF_Meta;  // APP meta streams delivered to USIF
        const char *ofsUSIF_Meta_FileName      = "../../../../test/simOutFiles/soUSIF_Meta.dat";
        ofstream ofsUSIF_DLen;  // APP data len streams delivered to USIF
        const char *ofsUSIF_DLen_FileName      = "../../../../test/simOutFiles/soUSIF_DLen.dat";

        string   ofsUSIF_Data_Gold_FileName = "../../../../test/simOutFiles/soUSIF_Data_Gold.dat";
        string   ofsUSIF_Meta_Gold_FileName = "../../../../test/simOutFiles/soUSIF_Meta_Gold.dat";
        string   ofsUSIF_DLen_Gold_FileName = "../../../../test/simOutFiles/soUSIF_DLen_Gold.dat";

        //-- STEP-1: The setting of the ECHO mode is already done via CFLAGS
        printInfo(THIS_NAME, "### TEST_MODE = ECHO_CTRL_DISABLED #########\n");

        //-- STEP-2: Remove previous old files and open new files
        if (not isDatFile(ofsUSIF_Data_FileName)) {
            printError(THIS_NAME, "File \'%s\' is not of type \'DAT\'.\n", ofsUSIF_Data_FileName);
            nrErr++;
        }
        else {
            remove(ofsUSIF_Data_FileName);
        }
        if (not isDatFile(ofsUSIF_Meta_FileName)) {
            printError(THIS_NAME, "File \'%s\' is not of type \'DAT\'.\n", ofsUSIF_Meta_FileName);
            nrErr++;
        }
        else {
            remove(ofsUSIF_Meta_FileName);
            if (!ofsUSIF_Meta.is_open()) {
                ofsUSIF_Meta.open(ofsUSIF_Meta_FileName, ofstream::out);
                if (!ofsUSIF_Meta) {
                    printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofsUSIF_Meta_FileName);
                    nrErr++;
                }
            }
        }
        if (not isDatFile(ofsUSIF_DLen_FileName)) {
            printError(THIS_NAME, "File \'%s\' is not of type \'DAT\'.\n", ofsUSIF_DLen_FileName);
            nrErr++;
        }
        else {
            remove(ofsUSIF_DLen_FileName);
            if (!ofsUSIF_DLen.is_open()) {
                ofsUSIF_DLen.open(ofsUSIF_DLen_FileName, ofstream::out);
                if (!ofsUSIF_DLen) {
                    printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofsUSIF_DLen_FileName);
                    nrErr++;
                }
            }
        }

        //-- STEP-3: Create golden Tx files
        queue<UdpAppMeta>   udpMetaQueue;
        queue<UdpAppDLen>   udpDLenQueue;
        if (NTS_OK == createGoldenTxFiles(tbCtrlMode, string(argv[2]), udpMetaQueue, udpDLenQueue,
                ofsUSIF_Data_Gold_FileName, ofsUSIF_Meta_Gold_FileName, ofsUSIF_DLen_Gold_FileName) != NTS_OK) {
            printError(THIS_NAME, "Failed to create golden Tx files. \n");
            nrErr++;
        }

        //-- STEP-4: Create the USIF->UAF INPUT {Data,Meta} as streams
        int nrUSIF_UAF_Chunks=0;
        if (not createUdpRxTraffic(ssUSIF_UAF_Data, "ssUSIF_UAF_Data",
                                   ssUSIF_UAF_Meta, "ssUSIF_UAF_Meta",
                                   ssUSIF_UAF_DLen, "ssUSIF_UAF_DLen",
                                   string(argv[2]),
                                   udpMetaQueue, udpDLenQueue,
                                   nrUSIF_UAF_Chunks)) {
            printFatal(THIS_NAME, "Failed to create the USIF->UAF traffic as streams.\n");
        }

        //-- STEP-5: Run simulation
        int tbRun = (nrErr == 0) ? (nrUSIF_UAF_Chunks + TB_GRACE_TIME) : 0;
        while (tbRun) {
            udp_app_flash(
                    //-- SHELL / Mmio Interfaces
                    &sSHL_UAF_Mmio_Enable,
                    //[NOT_USED] sSHL_UAF_Mmio_EchoCtrl,
                    //[NOT_USED] sSHL_UAF_Mmio_PostPktEn,
                    //[NOT_USED] sSHL_UAF_Mmio_CaptPktEn,
                    //-- USIF / Rx Data Interfaces
                    ssUSIF_UAF_Data,
                    ssUSIF_UAF_Meta,
                    ssUSIF_UAF_DLen,
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
            ofsUSIF_Data_FileName, nrUAF_USIF_DataChunks, nrUAF_USIF_DataGrams, nrUAF_USIF_DataBytes)) {
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
            ofsUSIF_Meta_FileName, nrUAF_USIF_MetaChunks, nrUAF_USIF_MetaGrams, nrUAF_USIF_MetaBytes)) {
            printError(THIS_NAME, "Failed to drain UAF-to-USIF meta traffic from DUT. \n");
            nrErr++;
        }
        //-- STEP-6c: Drain UAF-->USIF DLEN OUTPUT STREAM
        int nrUAF_USIF_DLenChunks=0, nrUAF_USIF_DLenGrams=0, nrUAF_USIF_DLenBytes=0;
        if (not drainUdpDLenStreamToFile(ssUAF_USIF_DLen, "ssUAF_USIF_DLen",
            ofsUSIF_DLen_FileName, nrUAF_USIF_DLenChunks, nrUAF_USIF_DLenGrams, nrUAF_USIF_DLenBytes)) {
            printError(THIS_NAME, "Failed to drain UAF-to-USIF dlen traffic from DUT. \n");
            nrErr++;
        }

        //-- STEP-7: Compare output DAT vs gold DAT
        int res;
        ifstream ifsFile;

        ifsFile.open(ofsUSIF_Data_FileName, ofstream::in);
        if (!ifsFile) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofsUSIF_Data_FileName);
            nrErr++;
        }
        else if (not (ifsFile.peek() == ifstream::traits_type::eof())) {
            res = myDiffTwoFiles(std::string(ofsUSIF_Data_FileName),
                                 std::string(ofsUSIF_Data_Gold_FileName));
            if (res) {
                printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
                           ofsUSIF_Data_FileName, ofsUSIF_Data_Gold_FileName.c_str());
               nrErr += 1;
            }
        }
        else {
            printError(THIS_NAME, "File \"%s\" is empty.\n", ofsUSIF_Data_FileName);
            nrErr++;
        }
        ifsFile.close();

        ifsFile.open(ofsUSIF_DLen_FileName, ofstream::in);
        if (!ifsFile) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofsUSIF_DLen_FileName);
            nrErr++;
        }
        else if (not (ifsFile.peek() == ifstream::traits_type::eof())) {
            res = myDiffTwoFiles(std::string(ofsUSIF_DLen_FileName),
                                 std::string(ofsUSIF_DLen_Gold_FileName));
            if (res) {
                printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
                           ofsUSIF_DLen_FileName, ofsUSIF_DLen_Gold_FileName.c_str());
               nrErr += 1;
            }
        }
        else {
            printError(THIS_NAME, "File \"%s\" is empty.\n", ofsUSIF_DLen_FileName);
            nrErr++;
        }
        ifsFile.close();

    }  // End-of: if (tbCtrlMode == ECHO_CTRL_DISABLED) {


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

