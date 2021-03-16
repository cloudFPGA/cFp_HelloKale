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
 * \ingroup ROLE_TAF
 * \addtogroup ROLE_TAF_TEST
 * \{
 ******************************************************************************/

#include "test_tcp_app_flash_top.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB_TAF_TOP"

#define TRACE_OFF     0x0000
#define TRACE_TSs    1 <<  1
#define TRACE_TSr    1 <<  2
#define TRACE_TAF    1 <<  3
#define TRACE_MMIO   1 <<  4
#define TRACE_ALL     0xFFFF
#define DEBUG_LEVEL (TRACE_OFF)

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
 * @brief Increase the simulation time of the testbench
 *
 * @param[in]  The number of cycles to increase.
 *******************************************************************************/
void increaseSimTime(unsigned int cycles) {
    gMaxSimCycles += cycles;
}

/*******************************************************************************
 * @brief Emulate the receiving part of the TSIF process.
 *
 * @param[in/out] nrError    A reference to the error counter of the [TB].
 * @param[in]  siTAF_Data    The data stream to read from [TAF].
 * @param[in]  siTAF_SessId  TCP session-id to read from [TAF].
 * @param[in]  siTAF_DatLen  TCP data-length to read from [TAF].
 * @param[out] outFileStream A ref to the a raw output file stream.
 * @param[out] outFileStream A ref to the a tcp output file stream.
 * @param[out] nrSegments    A ref to the counter of received segments.
 *
 * @returns OK/KO.
 *******************************************************************************/
bool pTSIF_Recv(
    int                &nrErr,
    stream<TcpAppData> &siTAF_Data,
    stream<TcpSessId>  &siTAF_SessId,
    stream<TcpDatLen>  &siTAF_DatLen,
    ofstream           &rawFileStream,
    ofstream           &tcpFileStream,
    int                &nrSegments)
{
    const char *myName  = concat3(THIS_NAME, "/", "TSr");

    static int  tsr_startOfSegCount = 0;

    TcpAppData  currChunk;
    TcpSessId   tcpSessId;
    TcpDatLen   tcpDatLen;

    // Read and drain the metadata
    if (!siTAF_SessId.empty() and !siTAF_DatLen.empty()) {
        siTAF_SessId.read(tcpSessId);
        siTAF_DatLen.read(tcpDatLen);
        tsr_startOfSegCount++;
        if (tsr_startOfSegCount > 1) {
            printWarn(myName, "Metadata and data streams did not arrive in expected order!\n");
        }
        if (DEBUG_LEVEL & TRACE_TSr) {
            printInfo(myName, "Received SessId=%d and DatLen=%d\n",
                      tcpSessId.to_uint(), tcpDatLen.to_uint());
        }
    }
    // Read and drain data stream
    if (!siTAF_Data.empty()) {
        siTAF_Data.read(currChunk);
        if (DEBUG_LEVEL & TRACE_TSr) { printAxisRaw(myName, "siTAF_Data=", currChunk); }
        if (currChunk.getTLast()) {
            tsr_startOfSegCount--;
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
 * @param[out] soTAF_SessId  TCP session-id to write.
 * @param[out] soTAF_DatLen  TCP data-length to write.
 * @param[in]  inpFileStream A ref to the input file stream.
 * @param[in]  outGoldStream A ref to the a golden raw output file stream.
 * @param[out] nrSegments    A ref to the counter of generated segments.
 *
 * @returns OK or KO upon error or end-of-file.
 *******************************************************************************/
bool pTSIF_Send(
    int                &nrError,
    stream<TcpAppData> &soTAF_Data,
    stream<TcpSessId>  &soTAF_SessId,
    stream<TcpDatLen>  &soTAF_DatLen,
    ifstream           &inpFileStream,
    ofstream           &outGoldStream,
    int                &nrSegments)
{
    const char *myName  = concat3(THIS_NAME, "/", "TSs");

    //-- STATIC VARIABLES ------------------------------------------------------
    static bool         tss_startOfTcpSeg = true;
    static bool         tss_idlingReq  = false; // Request to idle (i.e., do not feed TAF's input stream)
    static unsigned int tss_idleCycReq = 0;  // The requested number of idle cycles
    static unsigned int tss_idleCycCnt = 0;  // The count of idle cycles
    static SimAppData   tss_simAppData;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    string          strLine;
    vector<string>  stringVector;
    TcpAppData      currChunk;
    TcpSessId       tcpSessId = DEFAULT_SESS_ID;
    char           *pEnd;

    //-----------------------------------------------------
    //-- RETURN IF IDLING IS REQUESTED
    //-----------------------------------------------------
    if (tss_idlingReq == true) {
        if (tss_idleCycCnt >= tss_idleCycReq) {
            tss_idleCycCnt = 0;
            tss_idlingReq = false;
            if (DEBUG_LEVEL & TRACE_TSs) {
                printInfo(myName, "End of Tx idling phase. \n");
            }
        }
        else {
            tss_idleCycCnt++;
        }
        return true;
    }

    //------------------------------------------------------
    //-- FEED DATA STREAM
    //------------------------------------------------------
    if (tss_simAppData.size() != 0) {
        if (!soTAF_Data.full()) {
            //-- Feed TAF with a new data chunk
            AxisApp appChunk = tss_simAppData.pullChunk();
            soTAF_Data.write(appChunk);
            if (DEBUG_LEVEL & TRACE_TSs) { printAxisRaw(myName, "soTAF_Data=", appChunk); }
            increaseSimTime(1);
        }
    }
    else {
        //------------------------------------------------------
        //-- BUILD a new DATA STREAM from FILE
        //------------------------------------------------------
        while (!inpFileStream.eof()) {
            //-- Read a line from input vector file -----------
            getline(inpFileStream, strLine);
            if (DEBUG_LEVEL & TRACE_TSs) { printf("%s \n", strLine.c_str()); fflush(stdout); }
            stringVector = myTokenizer(strLine, ' ');
            if (!strLine.empty()) {
                //-- Build a new tss_simAppData from file
                if (stringVector[0].length() == 1) {
                    //------------------------------------------------------
                    //-- Process the command and comment lines
                    //--  FYI: A command or a comment start with a single
                    //--       character followed by a space character.
                    //------------------------------------------------------
                    if (stringVector[0] == "#") {
                        // This is a comment line.
                        continue;
                    }
                    else if (stringVector[0] == ">") {
                        // The test vector is issuing a command
                        //  FYI, don't forget to return at the end of command execution.
                        if (stringVector[1] == "IDLE") {
                            // COMMAND = Request to idle for <NUM> cycles.
                            tss_idleCycReq = strtol(stringVector[2].c_str(), &pEnd, 10);
                            tss_idlingReq = true;
                            if (DEBUG_LEVEL & TRACE_TSs) {
                                printInfo(myName, "Request to idle for %d cycles. \n", tss_idleCycReq);
                            }
                            increaseSimTime(tss_idleCycReq);
                            return true;
                        }
                        if (stringVector[1] == "SET") {
                           printWarn(myName, "The 'SET' command is not yet implemented...");
                        }
                   }
                    else {
                        printFatal(myName, "Read unknown command \"%s\" from TSIF.\n", stringVector[0].c_str());
                   }
               }
                else {
                    AxisApp currChunk;
                    bool    firstChunkFlag = true; // Axis chunk is first data chunk
                    int     writtenBytes = 0;
                    do {
                        if (firstChunkFlag == false) {
                            getline(inpFileStream, strLine);
                            if (DEBUG_LEVEL & TRACE_TSs) { printf("%s \n", strLine.c_str()); fflush(stdout); }
                            stringVector = myTokenizer(strLine, ' ');
                            // Skip lines that might be commented out
                            if (stringVector[0] == "#") {
                                // This is a comment line.
                                if (DEBUG_LEVEL & TRACE_TSs) { printf("%s ", strLine.c_str()); fflush(stdout); }
                                continue;
                            }
                        }
                        firstChunkFlag = false;
                        bool rc = readAxisRawFromLine(currChunk, strLine);
                        if (rc) {
                            tss_simAppData.pushChunk(currChunk);
                        }
                        // Write current chunk to the gold file
                        rc = writeAxisRawToFile(currChunk, outGoldStream);
                        if (currChunk.getTLast()) {
                            // Send metadata to [TAF]
                            soTAF_SessId.write(TcpSessId(tcpSessId));
                            soTAF_DatLen.write(TcpDatLen(tss_simAppData.length()));
                            return OK;
                        }
                    } while (not currChunk.getTLast());
                }
            }
        }
    }
    return OK;
}

/*******************************************************************************
 * @brief Emulate the behavior of TSIF.
 *
 * @param[in/out] nrErr           A ref to the error counter of the [TB].
 * @param[out]    poTAF_EchoCtrl  A scalar to set the ECHO mode.
 * @param[out]    soTAF_Data      Data stream from TcpAppFlash (TAF).
 * @param[out]    soTAF_SessId    TCP session-id from [TAF].
 * @param[out]    soTAF_DatLen    TCP data-length from [TAF].
 * @param[in]     siTAF_Data      Data stream to [TAF].
 * @param[in]     siTAF_SessId    TCP session-id to [TAF].
 * @param[in]     siTAF_DatLen    TCP data-length to [TAF].
 *
 *******************************************************************************/
void pTSIF(
        int                 &nrErr,
        //-- MMIO/ Configuration Interfaces
        //OBSOLETE_20210316 ap_uint<2>           poTAF_EchoCtrl,
        //-- TAF / TCP Data Interfaces
        stream<TcpAppData>  &soTAF_Data,
        stream<TcpSessId>   &soTAF_SessId,
        stream<TcpDatLen>   &soTAF_DatLen,
        //-- TAF / TCP Data Interface
        stream<TcpAppData>  &siTAF_Data,
        stream<TcpSessId>   &siTAF_SessId,
        stream<TcpDatLen>   &siTAF_DatLen)
{
    const char *myName  = concat3(THIS_NAME, "/", "TSIF");

    //-- STATIC VARIABLES ------------------------------------------------------
    static bool     tsif_doneWithPassThroughTest1 = false;
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
        //OBSOLETE_20210316 poTAF_EchoCtrl  = ECHO_PATH_THRU;

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
                soTAF_SessId,
                soTAF_DatLen,
                ifSHL_Data,
                ofRawGold1,
                tsif_txSegCnt);
        //-- STEP-2.4 : READ FROM THE TAF
        rcRecv = pTSIF_Recv(
                nrErr,
                siTAF_Data,
                siTAF_SessId,
                siTAF_DatLen,
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
        if (ofRawFile1.tellp() != 0) {
            int rc1 = system(("diff --brief -w " + std::string(ofRawFileName1) + " " + std::string(ofRawGoldName1) + " ").c_str());
            if (rc1) {
                printError(myName, "File '%s' does not match '%s'.\n", ofRawFileName1.c_str(), ofRawGoldName1.c_str());
                nrErr += 1;
            }
            tsif_doneWithPassThroughTest1 = true;
        }
        else {
            printError(THIS_NAME, "File \"%s\" is empty.\n", ofRawFileName1);
            nrErr += 1;
        }
        //-- Closing open files
        ifSHL_Data.close();
        ofRawFile1.close();
        ofRawGold1.close();
        ofTcpFile1.close();
    }
}


/******************************************************************************
 * @brief Main function for the test of the TCP Application Flash (TAF) TOP.
 ******************************************************************************/
int old_stuff(int argc, char *argv[]) {

    //------------------------------------------------------
    //-- TESTBENCH GLOBAL VARIABLES
    //------------------------------------------------------
    gSimCycCnt = 0;  // Simulation cycle counter as a global variable

    //------------------------------------------------------
    //-- DUT SIGNAL INTERFACES
    //------------------------------------------------------
    //-- MMIO/ Configuration Interfaces
    ap_uint<2>          sMMIO_TAF_EchoCtrl = 0;

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
    while (0) {
        tcp_app_flash_top(
            //-- SHELL / MMIO / Configuration Interfaces
            //OBSOLETE_20210316 sMMIO_TAF_EchoCtrl,
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

    printInfo(THIS_NAME, "#########################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_tcp_app_flash_top' ENDS HERE (RC=0) ##\n");
    printInfo(THIS_NAME, "#########################################################\n\n");

    return 0;  // Always

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
    //[NOT_USED] ap_uint<2> sMMIO_TAF_EchoCtrl;
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
    printInfo(THIS_NAME, "## TESTBENCH 'test_tcp_app_flash_top' STARTS HERE                         ##\n");
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
            //OBSOLETE_20210316 sMMIO_TAF_EchoCtrl,
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
        tcp_app_flash_top(
            //-- MMIO / Configuration Interfaces
            //OBSOLETE_20210316 sMMIO_TAF_EchoCtrl,
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
