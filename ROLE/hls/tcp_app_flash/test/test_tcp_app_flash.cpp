/************************************************
Copyright (c) 2016-2019, IBM Research.

All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*******************************************************************************
 * @file       : test_tcp_app_flash.cpp
 * @brief      : Testbench for TCP Application Flash.
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE/TcpApplicationFlash (TAF)
 * Language    : Vivado HLS
 *
 * \ingroup ROLE
 * \addtogroup ROLE_TAF
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
        //OBSOLETE_20200727 if (!writeAxiWordToFile(&currChunk, rawFileStream) or \
        //OBSOLETE_20200727     !writeTcpWordToFile(&currChunk, tcpFileStream)) {
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
 * @param[out] nrSegments    A ref to the counter of generated segments.
 *
 * @returns OK/KO.
 *******************************************************************************/
bool pTSIF_Send(
    int                &nrError,
    stream<TcpAppData> &soTAF_Data,
    stream<TcpAppMeta> &soTAF_Meta,
    ifstream           &inpFileStream,
    int                &nrSegments)
{
    const char *myName  = concat3(THIS_NAME, "/", "TSIF_Send");

    string      strLine;
    TcpAppData  currChunk;
    TcpAppMeta  tcpSessId = DEFAULT_SESS_ID;

    static bool startOfTcpSeg = true;

    //-- FEED DATA AND META STREAMS
    if (!inpFileStream.eof()) {
        getline(inpFileStream, strLine);
        if (!strLine.empty()) {
            //OBSOLETE_20200727 sscanf(strLine.c_str(), "%llx %d %x", &currChunk.tdata, &currChunk.tlast, &currChunk.tkeep);
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
        }
    }
    return OK;
}

/*******************************************************************************
 * @brief Emulate the behavior of TSIF.
 *
 * @param[in/out] nrErr           A ref to the error counter of the [TB].
 * @param[out]    poTAF_EchoCtrl  A ptr to set the ECHO mode.
 * @param[out]    poTAF_PostSegEn A ptr to set the POST segment mode.
 * @param[out]    poTAF_CaptSegEn A ptr to set the CAPTURE segment mode.
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
        CmdBit              *poTAF_PostSegEn,
        CmdBit              *poTAF_CaptSegEn,
        //-- TAF / TCP Data Interfaces
        stream<TcpAppData>  &soTAF_Data,
        stream<TcpAppMeta>  &soTAF_Meta,
        //-- TAF / TCP Data Interface
        stream<TcpAppData>  &siTAF_Data,
        stream<TcpAppMeta>  &siTAF_Meta,
        //-- TSIF / Session Connection Id
        SessionId           *poTAF_SessConId)
{
    const char *myName  = concat3(THIS_NAME, "/", "TSIF");

    //-- STATIC VARIABLES ------------------------------------------------------
    static bool     tsif_doneWithPassThroughTest1 = false;
    static bool     tsif_doneWithPostSegmentTest2 = false;
    static int      tsif_txSegCnt = 0;
    static int      tsif_rxSegCnt = 0;
    static int      tsif_graceTime;

    static ifstream ifSHL_Data;
    static ofstream ofRawFile1;
    static ofstream ofTcpFile1;
    static ofstream ofRawFile2;
    static ofstream ofTcpFile2;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    bool   rcSend;
    bool   rcRecv;
    string ifSHL_DataName = "../../../../test/testVectors/siSHL_Data.dat";
    string ofRawFile1Name = "../../../../test/simOutFiles/soTAF_Shl_Echo_Path_Thru_Data.dat";
    string ofTcpFileName1 = "../../../../test/simOutFiles/soTAF_Shl_Echo_Path_Thru_Data.tcp";
    string ofRawFileName2 = "../../../../test/simOutFiles/soTAF_Shl_Echo_Off_Data.dat";
    string ofTcpFileName2 = "../../../../test/simOutFiles/soTAF_Shl_Echo_Off_Data.tcp";

    //------------------------------------------------------
    //-- STEP-1 : GIVE THE TEST SOME STARTUP TIME
    //------------------------------------------------------
    if (gSimCycCnt < STARTUP_DELAY)
        return;

    //------------------------------------------------------
    //-- STEP-2 : TEST THE PASS-THROUGH MODE
    //------------------------------------------------------
    if (gSimCycCnt == STARTUP_DELAY) {
        printf("\n## PART-1 : TEST OF THE PASS-THROUGH MODE ####################\n");
        tsif_rxSegCnt = 0;
        *poTAF_EchoCtrl  = ECHO_PATH_THRU;
        *poTAF_PostSegEn = DISABLED;
        *poTAF_SessConId = (SessionId(0));

        //-- STEP-2.0 : REMOVE PREVIOUS OUTPUT FILES
        std::remove(ofRawFile1Name.c_str());
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
            ofRawFile1.open (ofRawFile1Name.c_str(), ofstream::out);
            if (!ofRawFile1) {
                printFatal(myName, "Could not open the output Raw data file \'%s\'. \n", ofRawFile1Name.c_str());
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
    else if (!tsif_doneWithPassThroughTest1) {
        //-- STEP-2.3 : FEED THE TAF
        rcSend = pTSIF_Send(
                nrErr,
                soTAF_Data,
                soTAF_Meta,
                ifSHL_Data,
                tsif_txSegCnt);
        //-- STEP-2.4 : READ FROM THE TAF
        rcRecv = pTSIF_Recv(
                nrErr,
                siTAF_Data,
                siTAF_Meta,
                ofRawFile1,
                ofTcpFile1,
                tsif_rxSegCnt);
        //-- STEP-2.5 : CHECK IF TEST IS FINISHED
        if (!rcSend or !rcRecv or (tsif_txSegCnt == tsif_rxSegCnt)) {
            ifSHL_Data.close();
            ofRawFile1.close();
            ofTcpFile1.close();
            int rc1 = system(("diff --brief -w " + std::string(ofRawFile1Name) + " " + std::string(ifSHL_DataName) + " ").c_str());
            if (rc1) {
                printError(myName, "File '%s' does not match '%s'.\n", ofRawFile1Name.c_str(), ifSHL_DataName.c_str());
                nrErr += 1;
            }
            tsif_doneWithPassThroughTest1 = true;
            tsif_graceTime = gSimCycCnt + 100;
        }
    }

    //------------------------------------------------------
    //-- STEP-3 : GIVE TEST #1 SOME GRACE TIME TO FINISH
    //------------------------------------------------------
    if (!tsif_doneWithPassThroughTest1 or (gSimCycCnt < tsif_graceTime))
        return;

    //------------------------------------------------------
    //-- STEP-3 : TEST THE POST-SEGMENT MODE
    //------------------------------------------------------
    if (gSimCycCnt == tsif_graceTime) {
        printf("## PART-2 : TEST OF THE POST-SEGMENT MODE ######################\n");
        tsif_rxSegCnt = 0;
        //-- STEP-3.1 : SET THE ECHO_OFF & POST_SEG_ENABLE MODES
        *poTAF_EchoCtrl  = ECHO_OFF;
        *poTAF_PostSegEn = ENABLED;
        printInfo(myName, "Enabling the segment posting mode.\n");
        //-- STEP-3.2 : SET FORWARD A NEW SESSION CONNECT ID TO TCP_APP_FLASH
        *poTAF_SessConId = (SessionId(1));
        //-- STEP-3.3 : OPEN TWO OUTPUT DATA FILES #2
        if (!ofRawFile2.is_open()) {
            ofRawFile2.open (ofRawFileName2.c_str(), ofstream::out);
            if (!ofRawFile2) {
                printFatal(myName, "Could not open the output Raw data file \'%s\'. \n", ofRawFileName2.c_str());
                nrErr++;
                tsif_doneWithPostSegmentTest2 = true;
                return;
            }
        }
        if (!ofTcpFile2.is_open()) {
             ofTcpFile2.open (ofTcpFileName2.c_str(), ofstream::out);
             if (!ofTcpFile2) {
                 printFatal(myName, "Could not open the Tcp output data file \'%s\'. \n", ofTcpFileName2.c_str());
                 nrErr++;
                 tsif_doneWithPostSegmentTest2 = true;
                 return;
             }
         }
    }
    else if (!tsif_doneWithPostSegmentTest2) {
        //-- STEP-3.4 : READ FROM THE TAF
        rcRecv = pTSIF_Recv(
                nrErr,
                siTAF_Data,
                siTAF_Meta,
                ofRawFile2,
                ofTcpFile2,
                tsif_rxSegCnt);
        //-- STEP-3.5 : IF TEST IS FINISHED, DISABLE THE POSTING MODE
        if (!rcRecv or (tsif_rxSegCnt == 6)) {
            *poTAF_PostSegEn = DISABLED;
            printInfo(myName, "Disabling the segment posting mode.\n");
            //-- CLOSE FILES AND VERIFY CONTENT OF OUTPUT FILES
            ofRawFile2.close();
            ofTcpFile2.close();
            int rc1 = system("[ $(grep -o -c '6F57206F6C6C65486D6F726620646C72203036554B4D46202D2D2D2D2D2D2D2D' ../../../../test/ofsTAF_Shl_Echo_Off_Data.tcp) -eq 6 ] ");
            if (rc1) {
                printError(myName, "File \'%s\' does not content 6 instances of the expected \'Hello World\' string.\n", std::string(ofTcpFileName2).c_str());
                nrErr += 1;
            }
            tsif_doneWithPostSegmentTest2 = true;
        }
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
    CmdBit              sMMIO_TAF_PostSegEn;
    CmdBit              sMMIO_TAF_CaptSegEn;
    //-- TSIF / Session Connection Id
    SessionId           sTSIF_TAF_SessConId;

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
    printInfo(THIS_NAME, "## TESTBENCH 'test_toe' STARTS HERE                                       ##\n");
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
            &sMMIO_TAF_PostSegEn,
            &sMMIO_TAF_CaptSegEn,
            //-- TAF / TCP Data Interfaces
            ssTSIF_TAF_Data,
            ssTSIF_TAF_Meta,
            //-- TAF / TCP Data Interface
            ssTAF_TSIF_Data,
            ssTAF_TSIF_Meta,
            //-- TSIF / Session Connection Id
            &sTSIF_TAF_SessConId);

        //-------------------------------------------------
        //-- RUN DUT
        //-------------------------------------------------
        tcp_app_flash(
            //-- MMIO / Configuration Interfaces
            &sMMIO_TAF_EchoCtrl,
            &sMMIO_TAF_PostSegEn,
            //[TODO] *piSHL_MmioCaptSegEn,
            //-- TSIF / Session Connect Id Interface
            &sTSIF_TAF_SessConId,
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
