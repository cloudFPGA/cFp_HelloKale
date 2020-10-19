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
 * @file       : test_tcp_app_flash.cpp
 * @brief      : Testbench for TCP Application Flash.
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE/TcpApplicationFlash (TAF)
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
 * \ingroup ROLE_TAF
 * \addtogroup ROLE_TAF_TEST
 * \{
 *******************************************************************************/

#include "test_tcp_app_flash.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF     0x0000
#define TRACE_TSIF   1 <<  1
#define TRACE_TAF    1 <<  2
#define TRACE_MMIO   1 <<  3
#define TRACE_ALL     0xFFFF
#define DEBUG_LEVEL (TRACE_ALL)

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

/*******************************************************************************
 * @brief Emulate the receiving part of the TSIF process.
 *
 * @param[in/out] nrError    A reference to the error counter of the [TB].
 * @param[out] siTAF_Data    The data stream to read.
 * @param[out] siTAF_Meta    The meta stream to read.
 * @param[in]  outFileStream A ref to the a raw output file stream.
 * @param[in]  outFileStream A ref to the a tcp output file stream.
 * @param[out] nrSegments    A ref to the counter of received segments.
 *
 * @returns OK/KO.
 *******************************************************************************/
bool pTSIF_Recv(
    int                &nrErr,
    stream<TcpAppData> &siTAF_Data,
    stream<TcpAppMeta> &siTAF_Meta,
    ofstream           &rawFileStream,
    ofstream           &tcpFileStream,
    int                &nrSegments)
{
    const char *myName  = concat3(THIS_NAME, "/", "TSIF_Recv");

    static int  tsifr_startOfSegCount = 0;

    TcpAppData  currChunk;
    TcpAppMeta  appMeta;
    TcpSessId   tcpSessId;

    // Read and drain the siMetaStream
    if (!siTAF_Meta.empty()) {
        siTAF_Meta.read(appMeta);
        tsifr_startOfSegCount++;
        if (tsifr_startOfSegCount > 1) {
            printWarn(myName, "Meta and Data stream did not arrive in expected order!\n");
        }
        tcpSessId = appMeta;
        printInfo(myName, "Reading META (SessId) = %d \n", tcpSessId.to_uint());
    }
    // Read and drain sDataStream
    if (!siTAF_Data.empty()) {
        siTAF_Data.read(currChunk);
        if (DEBUG_LEVEL & TRACE_TAF) { printAxisRaw(myName, "siTAF_Data=", currChunk); }
        if (currChunk.getTLast()) {
            tsifr_startOfSegCount--;
            nrSegments++;
        }
        //-- Write data to file
        if (!writeAxisRawToFile(currChunk, rawFileStream) or \
            !writeAxisAppToFile(currChunk, tcpFileStream)) {
            return KO;
        }
    }
    return OK;
}

/*******************************************************************************
 * @brief Emulate the sending part of the TSIF process.
 *
 * @param[in/out] nrError    A reference to the error counter of the [TB].
 * @param[out] soTAF_Data    The data stream to write.
 * @param[out] soTAF_Meta    The meta stream to write.
 * @param[in]  inpFileStream A ref to the input file stream.
 * @param[in]  outGoldStream A ref to the a golden raw output file stream.
 * @param[out] nrSegments    A ref to the counter of generated segments.
 *
 * @returns OK or KO upon error or end-of-file.
 *******************************************************************************/
bool pTSIF_Send(
    int                &nrError,
    stream<TcpAppData> &soTAF_Data,
    stream<TcpAppMeta> &soTAF_Meta,
    ifstream           &inpFileStream,
    ofstream           &outGoldStream,
    int                &nrSegments)
{
    const char *myName  = concat3(THIS_NAME, "/", "TSIF_Send");

    string      strLine;
    vector<string>      stringVector;
    TcpAppData  currChunk;
    TcpAppMeta  tcpSessId = DEFAULT_SESS_ID;

    static bool startOfTcpSeg = true;

    //-- FEED DATA AND META STREAMS
    while (!inpFileStream.eof()) {
        getline(inpFileStream, strLine);
        stringVector = myTokenizer(strLine, ' ');

        if (!strLine.empty()) {
            if (stringVector[0].length() == 1) {
                //------------------------------------------------------
                //-- Process the command and comment lines
                //--  FYI: A command or a comment start with a single
                //--       character followed by a space character.
                //------------------------------------------------------
                if (stringVector[0] == "#") {
                    // This is a comment line.
                    if (DEBUG_LEVEL & TRACE_TSIF) {
                        for (int t=0; t<stringVector.size(); t++) {
                            printf("%s ", stringVector[t].c_str());
                        }
                    printf("\n");
                    }
                }
                else if (stringVector[0] == ">") {
                    // The test vector is issuing a command
                    //  FYI, don't forget to return at the end of command execution.
                    if (stringVector[1] == "IDLE") {
                        // COMMAND = Request to idle for <NUM> cycles.
                        //TODO iprx_idleCycReq = atoi(stringVector[2].c_str());
                        //TODO iprx_idlingReq = true;
                        if (DEBUG_LEVEL & TRACE_TSIF) {
                            //TODO printInfo(myName, "Request to idle for %d cycles. \n", iprx_idleCycReq);
                            printInfo(myName, "[TODO] Request to idle for ... \n");
                        }
                        //TODO increaseSimTime(iprx_idleCycReq);
                        //TODO return;
                    }
                }
            }
            else {
                bool rc = readAxisRawFromLine(currChunk, strLine);
                // Write to soMetaStream
                if (startOfTcpSeg) {
                    if (!soTAF_Meta.full()) {
                        soTAF_Meta.write(TcpAppMeta(tcpSessId));
                        startOfTcpSeg = false;
                        nrSegments += 1;
                    }
                    else {
                        printError(myName, "Cannot write META to [TAF] because stream is full.\n");
                        nrError++;
                        inpFileStream.close();
                        return KO;
                    }
                }
                // Write to soDataStream
                if (!soTAF_Data.full()) {
                    soTAF_Data.write(currChunk);
                    bool rc = writeAxisRawToFile(currChunk, outGoldStream);
                    if (DEBUG_LEVEL & TRACE_TAF) { printAxisRaw(myName, "soTAF_Data =", currChunk); }
                    if (currChunk.getTLast()) {
                        startOfTcpSeg = true;
                    }
                }
                else {
                    printError(myName, "Cannot write DATA to [TAF] because stream is full.\n");
                    nrError++;
                    inpFileStream.close();
                    return KO;
                }
                return OK;
            }
        }
    }
    return KO;
}

/*******************************************************************************
 * @brief Emulate the behavior of TSIF.
 *
 * @param[in/out] nrErr           A ref to the error counter of the [TB].
 * @param[out]    poTAF_EchoCtrl  A ptr to set the ECHO mode.
 * @param[out]    soTAF_Data      Data stream from TcpAppFlash (TAF).
 * @param[out]    soTAF_Meta      Metadata from [TAF].
 * @param[in]     siTAF_Data      Data stream to [TAF].
 * @param[in]     siTAF_Meta      Metadata to [TAF].
 *
 *******************************************************************************/
void pTSIF(
        int                 &nrErr,
        //-- MMIO/ Configuration Interfaces
        ap_uint<2>          *poTAF_EchoCtrl,
        //-- TAF / TCP Data Interfaces
        stream<TcpAppData>  &soTAF_Data,
        stream<TcpAppMeta>  &soTAF_Meta,
        //-- TAF / TCP Data Interface
        stream<TcpAppData>  &siTAF_Data,
        stream<TcpAppMeta>  &siTAF_Meta)
{
    const char *myName  = concat3(THIS_NAME, "/", "TSIF");

    //-- STATIC VARIABLES ------------------------------------------------------
    static bool     tsif_doneWithPassThroughTest1 = false;
    //OBSOLETE_20201019 static bool     tsif_doneWithPostSegmentTest2 = false;
    static int      tsif_txSegCnt = 0;
    static int      tsif_rxSegCnt = 0;
    static int      tsif_graceTime1 = 25; // Give TEST #1 some grace time to finish

    static ifstream ifSHL_Data;
    static ofstream ofRawFile1;
    static ofstream ofRawGold1;
    static ofstream ofTcpFile1;
    static ofstream ofRawFile2;
    static ofstream ofTcpFile2;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    bool   rcSend;
    bool   rcRecv;
    string ifSHL_DataName = "../../../../test/testVectors/siTSIF_Data.dat";
    string ofRawFileName1 = "../../../../test/simOutFiles/soTAF_Shl_Echo_Path_Thru_Data.dat";
    string ofRawGoldName1 = "../../../../test/simOutFiles/soTAF_Shl_Echo_Path_Thru_Data_Gold.dat";
    string ofTcpFileName1 = "../../../../test/simOutFiles/soTAF_Shl_Echo_Path_Thru_Data.tcp";
    string ofRawFileName2 = "../../../../test/simOutFiles/soTAF_Shl_Echo_Off_Data.dat";
    string ofTcpFileName2 = "../../../../test/simOutFiles/soTAF_Shl_Echo_Off_Data.tcp";

    //------------------------------------------------------
    //-- STEP-1 : RETURN IF DONE or STARTUP TIME
    //------------------------------------------------------
    if (tsif_doneWithPassThroughTest1) {
        return;
    }
    if (gSimCycCnt < STARTUP_DELAY) {
         return;
    }

    //------------------------------------------------------
    //-- STEP-2 : TEST THE PASS-THROUGH MODE
    //------------------------------------------------------
    if (gSimCycCnt == STARTUP_DELAY) {
        printf("\n## PART-1 : TEST OF THE PASS-THROUGH MODE ####################\n");
        tsif_rxSegCnt = 0;
        *poTAF_EchoCtrl  = ECHO_PATH_THRU;

        //-- STEP-2.0 : REMOVE PREVIOUS OUTPUT FILES
        std::remove(ofRawFileName1.c_str());
        std::remove(ofRawGoldName1.c_str());
        std::remove(ofRawFileName2.c_str());
        std::remove(ofTcpFileName1.c_str());
        std::remove(ofTcpFileName2.c_str());

        //-- STEP-2.1 : OPEN INPUT TEST FILE #1
        if (!ifSHL_Data.is_open()) {
            ifSHL_Data.open (ifSHL_DataName.c_str(), ifstream::in);
            if (!ifSHL_Data) {
                printFatal(myName, "Could not open the input data file \'%s\'. \n", ifSHL_DataName.c_str());
                nrErr++;
                tsif_doneWithPassThroughTest1 = true;
                return;
            }
        }
        //-- STEP-2.2 : OPEN TWO OUTPUT DATA FILES #1
        if (!ofRawFile1.is_open()) {
            ofRawFile1.open (ofRawFileName1.c_str(), ofstream::out);
            if (!ofRawFile1) {
                printFatal(myName, "Could not open the output Raw data file \'%s\'. \n", ofRawFileName1.c_str());
                nrErr++;
                tsif_doneWithPassThroughTest1 = true;
                return;
            }
        }
        if (!ofRawGold1.is_open()) {
            ofRawGold1.open (ofRawGoldName1.c_str(), ofstream::out);
            if (!ofRawGold1) {
                printFatal(myName, "Could not open the output Raw gold file \'%s\'. \n", ofRawGoldName1.c_str());
                nrErr++;
                tsif_doneWithPassThroughTest1 = true;
                return;
            }
        }
        if (!ofTcpFile1.is_open()) {
             ofTcpFile1.open (ofTcpFileName1.c_str(), ofstream::out);
             if (!ofTcpFile1) {
                 printFatal(myName, "Could not open the Tcp output data file \'%s\'. \n", ofTcpFileName1.c_str());
                 nrErr++;
                 tsif_doneWithPassThroughTest1 = true;
                 return;
             }
         }
    }
    else if (tsif_graceTime1) {
        //-- STEP-2.3 : FEED THE TAF
        rcSend = pTSIF_Send(
                nrErr,
                soTAF_Data,
                soTAF_Meta,
                ifSHL_Data,
                ofRawGold1,
                tsif_txSegCnt);
        //-- STEP-2.4 : READ FROM THE TAF
        rcRecv = pTSIF_Recv(
                nrErr,
                siTAF_Data,
                siTAF_Meta,
                ofRawFile1,
                ofTcpFile1,
                tsif_rxSegCnt);
        //-- STEP-2.5 : CHECK IF TEST FAILED or IS FINISHED
        if ((rcSend != OK) or (rcRecv != OK)) {
            tsif_graceTime1--; // Give the test some grace time to finish
        }
    }

    //------------------------------------------------------
    //-- STEP-3 : VERIFY THE PASS-THROUGH MODE
    //------------------------------------------------------
    if (tsif_graceTime1 == 0) {
        ifSHL_Data.close();
        ofRawFile1.close();
        ofRawGold1.close();
        ofTcpFile1.close();
        int rc1 = system(("diff --brief -w " + std::string(ofRawFileName1) + " " + std::string(ofRawGoldName1) + " ").c_str());
        if (rc1) {
            printError(myName, "File '%s' does not match '%s'.\n", ofRawFileName1.c_str(), ofRawGoldName1.c_str());
            nrErr += 1;
        }
        tsif_doneWithPassThroughTest1 = true;
    }
}


/******************************************************************************
 * @brief Main function.
 *
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
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------
    //-- TSIF / TCP Data Interfaces
    stream<TcpAppData>  ssTSIF_TAF_Data ("ssTSIF_TAF_Data");
    stream<TcpAppMeta>  ssTSIF_TAF_Meta ("ssTSIF_TAF_Meta");
    stream<TcpAppData>  ssTAF_TSIF_Data ("ssTAF_TSIF_Data");
    stream<TcpAppMeta>  ssTAF_TSIF_Meta ("ssTAF_TSIF_Meta");

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
            &sMMIO_TAF_EchoCtrl,
            //-- TAF / TCP Data Interfaces
            ssTSIF_TAF_Data,
            ssTSIF_TAF_Meta,
            //-- TAF / TCP Data Interface
            ssTAF_TSIF_Data,
            ssTAF_TSIF_Meta);

        //-------------------------------------------------
        //-- RUN DUT
        //-------------------------------------------------
        tcp_app_flash(
            //-- MMIO / Configuration Interfaces
            &sMMIO_TAF_EchoCtrl,
            //-- TSIF / TCP Rx Data Interface
            ssTSIF_TAF_Data,
            ssTSIF_TAF_Meta,
            //-- TSIF / TCP Tx Data Interface
            ssTAF_TSIF_Data,
            ssTAF_TSIF_Meta);

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
