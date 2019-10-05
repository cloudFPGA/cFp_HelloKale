/*****************************************************************************
 * @file       : test_tcp_app_flash.cpp
 * @brief      : Testbench for TCP Application Flash (TAF).
 *
 * System:     : cloudFPGA
 * Component   : RoleFlash
 * Language    : Vivado HLS
 *
 * Copyright 2015-2019 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <hls_stream.h>

#include "../src/tcp_app_flash.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF     0x0000
#define TRACE_TRIF   1 <<  1
#define TRACE_TAF    1 <<  2
#define TRACE_MMIO   1 <<  3
#define TRACE_ALL     0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

//------------------------------------------------------
//-- TESTBENCH DEFINES
//------------------------------------------------------
#define MAX_SIM_CYCLES  500
#define VALID           true
#define UNVALID         false
#define DONE            true
#define NOT_YET_DONE    false

#define ENABLED         (ap_uint<1>)1
#define DISABLED        (ap_uint<1>)0

#define DEFAULT_SESS_ID 42

#define STARTUP_DELAY   25

//------------------------------------------------------
// UTILITY PROTOTYPE DEFINITIONS
//------------------------------------------------------
bool writeAxiWordToFile(AxiWord  *tcpWord, ofstream &outFileStream);
int  writeTcpWordToFile(AxiWord  *tcpWord, ofstream &outFile);

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
unsigned int    gSimCycCnt    = 0;
bool            gTraceEvent   = false;
bool            gFatalError   = false;
unsigned int    gMaxSimCycles = MAX_SIM_CYCLES;


/*****************************************************************************
 * @brief Receiving part of the TRIF process.
 *
 * @param[in/out] nrError,    A reference to the error counter of the [TB].
 * @param[out] siTAF_Data,    the data stream to read.
 * @param[out] siTAF_Meta,    the meta stream to read.
 * @param[in]  outFileStream, a ref to the a raw output file stream.
 * @param[in]  outFileStream, a ref to the a tcp output file stream.
 * @param[out] nrSegments,    a ref to the counter of received segments.
 *
 * @returns OK/KO.
 ******************************************************************************/
bool pTRIF_Recv(
    int                &nrErr,
    stream<AppData>    &siTAF_Data,
    stream<AppMeta>    &siTAF_SessId,
    ofstream           &rawFileStream,
    ofstream           &tcpFileStream,
    int                &nrSegments)
{
    const char *myName  = concat3(THIS_NAME, "/", "TRIF_Recv");

    AppData     tcpWord;
    AppMeta     appMeta;
    TcpSessId   tcpSessId;
    bool        startOfTcpSeg;

    //-- EMPTY STREAMS AND DUMP DATA TO FILE
    startOfTcpSeg = true;
    // Read and drain the siMetaStream
    if (startOfTcpSeg) {
        if (!siTAF_SessId.empty()) {
            siTAF_SessId.read(appMeta);
            tcpSessId = appMeta;
            printInfo(myName, "Reading META (SessId) = %d \n", tcpSessId.to_uint());
            startOfTcpSeg = false;
        }
        // Read and drain sDataStream
        if (!siTAF_Data.empty()) {
            siTAF_Data.read(tcpWord);
            printInfo(myName, "Reading [TAF] - DATA = {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                      tcpWord.tdata.to_long(), tcpWord.tkeep.to_int(), tcpWord.tlast.to_int());
            if (tcpWord.tlast) {
                startOfTcpSeg = true;
                nrSegments++;
            }
            //-- Write data to file
            if (!writeAxiWordToFile(&tcpWord, rawFileStream) or \
                !writeTcpWordToFile(&tcpWord, tcpFileStream)) {
                return KO;
            }
        }
    }
    return OK;
}

/*****************************************************************************
 * @brief Sending part of the TRIF process.
 *
 * @param[in/out] nrError,    A reference to the error counter of the [TB].
 * @param[out] soTAF_Data,    the data stream to write.
 * @param[out] soTAF_Meta,    the meta stream to write.
 * @param[in]  inpFileStream, a ref to the input file stream.
 * @param[out] nrSegments,    a ref to the counter of generated segments.
 *
 * @returns OK/KO.
 ******************************************************************************/
bool pTRIF_Send(
    int                &nrError,
    stream<AppData>    &soTAF_Data,
    stream<AppMeta>    &soTAF_Meta,
    ifstream           &inpFileStream,
    int                &nrSegments)
{
    const char *myName  = concat3(THIS_NAME, "/", "TRIF_Send");

    string      strLine;
    AxiWord     tcpWord;
    TcpSessId   tcpSessId = DEFAULT_SESS_ID;

    static bool startOfTcpSeg = true;

    //-- FEED DATA AND META STREAMS
    if (!inpFileStream.eof()) {
        getline(inpFileStream, strLine);
        if (!strLine.empty()) {
            sscanf(strLine.c_str(), "%llx %x %d", &tcpWord.tdata, &tcpWord.tkeep, &tcpWord.tlast);
            // Write to soMetaStream
            if (startOfTcpSeg) {
                if (!soTAF_Meta.full()) {
                    soTAF_Meta.write(AppMeta(tcpSessId));
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
                soTAF_Data.write(tcpWord);
                // Print Data to console
                printInfo(myName, "Feeding [TAF] - DATA = {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                          tcpWord.tdata.to_long(), tcpWord.tkeep.to_int(), tcpWord.tlast.to_int());
                if (tcpWord.tlast) {
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

/******************************************************************************
 * @brief Emulate the behavior of the TRIF.
 *
 * @param[in/out] A reference to   the error counter of the [TB].
 * @param[out]    poTAF_EchoCtrl,  a ptr to set the ECHO mode.
 * @param[out]    poTAF_PostSegEn, a ptr to set the POST segment mode.
 * @param[out]    poTAF_CaptSegEn, a ptr to set the CAPTURE segment mode.
 * @param[out]    soTAF_Data       Data from TcpAppFlash (TAF).
 * @param[out]    soTAF_SessId     Session ID from [TAF].
 * @param[in]     siTAF_Data       Data stream to [TAF].
 * @param[in]     siTAF_SessId     SessionID to [TAF].
 *
 ******************************************************************************/
void pTRIF(
        int                 &nrErr,
        //-- MMIO/ Configuration Interfaces
        ap_uint<2>          *poTAF_EchoCtrl,
        CmdBit              *poTAF_PostSegEn,
        CmdBit              *poTAF_CaptSegEn,
        //-- TAF / TCP Data Interfaces
        stream<AppData>     &soTAF_Data,
        stream<AppMeta>     &soTAF_SessId,
        //-- TAF / TCP Data Interface
        stream<AppData>     &siTAF_Data,
        stream<AppMeta>     &siTAF_SessId,
        //-- TRIF / Session Connection Id
        SessionId           *poTAF_SessConId)
{
    const char *myName  = concat3(THIS_NAME, "/", "TRIF");

    static bool doneWithPassThroughTest1 = false;
    static bool doneWithPostSegmentTest2 = false;

    static int      txSegCnt = 0;
    static int      rxSegCnt = 0;
    static ifstream ifs1;
    static ofstream ofsRaw1;
    static ofstream ofsTcp1;
    string inpDatFile1 = "../../../../test/ifsSHL_Taf_Data.dat";
    string outRawFile1 = "../../../../test/ofsTAF_Shl_Echo_Path_Thru_Data.dat";
    string outTcpFile1 = "../../../../test/ofsTAF_Shl_Echo_Path_Thru_Data.tcp";
    string outDatFile2 = "../../../../test/ofsTAF_Shl_Echo_Off_Data.dat";
    static int      graceTime;

    bool rcSend;
    bool rcRecv;

    //------------------------------------------------------
    //-- STEP-1 : GIVE THE TEST SOME STARTUP TIME
    //------------------------------------------------------
    if (gSimCycCnt < STARTUP_DELAY)
        return;

    //------------------------------------------------------
    //-- STEP-2 : TEST THE PASS-THROUGH MODE
    //------------------------------------------------------
    if (gSimCycCnt == STARTUP_DELAY) {
        *poTAF_EchoCtrl  = ECHO_PATH_THRU;
        *poTAF_PostSegEn = DISABLED;
        //[TODO] *poTAF_CaptSegEn = DISABLED;
        *poTAF_SessConId = (SessionId(0));
    }
    else if (!doneWithPassThroughTest1) {
        //-- STEP-1.1 : OPEN INPUT TEST FILE #1
        if (!ifs1.is_open()) {
            ifs1.open (inpDatFile1.c_str(), ifstream::in);
            if (!ifs1) {
                printFatal(myName, "Could not open the input data file \'%s\'. \n", inpDatFile1.c_str());
                nrErr++;
                doneWithPassThroughTest1 = true;
                return;
            }
        }
        //-- STEP-1.2 : OPEN TWO OUTPUT DATA FILES #1
        if (!ofsRaw1.is_open()) {
            ofsRaw1.open (outRawFile1.c_str(), ofstream::out);
            if (!ofsRaw1) {
                printFatal(myName, "Could not open the output Raw data file \'%s\'. \n", outRawFile1.c_str());
                nrErr++;
                doneWithPassThroughTest1 = true;
                return;
            }
        }
        if (!ofsTcp1.is_open()) {
             ofsTcp1.open (outTcpFile1.c_str(), ofstream::out);
             if (!ofsTcp1) {
                 printFatal(myName, "Could not open the Tcp output data file \'%s\'. \n", outTcpFile1.c_str());
                 nrErr++;
                 doneWithPassThroughTest1 = true;
                 return;
             }
         }
        //-- STEP-1.3 : FEED THE TAF
        rcSend = pTRIF_Send(
                nrErr,
                soTAF_Data,
                soTAF_SessId,
                ifs1,
                txSegCnt);
        //-- STEP-1.4 : READ FROM THE TAF
        rcRecv = pTRIF_Recv(
                nrErr,
                siTAF_Data,
                siTAF_SessId,
                ofsRaw1,
                ofsTcp1,
                rxSegCnt);
        //-- STEP-1.5 : CHECK IF TEST IS FINISHED
        if (!rcSend or !rcRecv or (txSegCnt == rxSegCnt)) {
            ifs1.close();
            ofsRaw1.close();
            ofsTcp1.close();
            int rc1 = system(("diff --brief -w " + std::string(outRawFile1) + " " + std::string(inpDatFile1) + " ").c_str());
            if (rc1) {
                printError(myName, "File \'ofsTAF_Shl_Echo_Path_Thru_Data.dat\' does not match \'ifsSHL_Taf_Data.dat\'.\n");
                nrErr += 1;
            }
            doneWithPassThroughTest1 = true;
            graceTime = gSimCycCnt + 100;
        }
    }

    //------------------------------------------------------
    //-- STEP-2 : GIVE TEST #1 SOME GRACE TIME TO FINISH
    //------------------------------------------------------
    if (!doneWithPassThroughTest1 or (gSimCycCnt < graceTime))
        return;

    //------------------------------------------------------
    //-- STEP-3 : TEST THE POST-SEGMENT MODE
    //------------------------------------------------------
    if (gSimCycCnt == graceTime) {
        *poTAF_EchoCtrl  = ECHO_OFF;
        *poTAF_PostSegEn = ENABLED;
        //[TODO] *poTAF_CaptSegEn = DISABLED;
        //-- Set the Session Connect Id Interface
        *poTAF_SessConId = (SessionId(1));
    }
    else if (!doneWithPostSegmentTest2) {

    }

/***
while (gSimCycCnt < 300) {
    stepDut(ssDataFromTrif, ssMetaFromTrif);
    if (gSimCycCnt == 100) {
        //------------------------------------------------------
        //-- STEP-2.1 : SET THE ECHO_OFF & POST_SEG_ENABLE MODES
        //------------------------------------------------------
        piSHL_MmioEchoCtrl.write(ECHO_OFF);
        piSHL_MmioPostSegEn.write(ENABLED);
        printf("[%5.6d] [TB] is enabling the segment posting mode.\n", gSimCycCnt);
    }
    else if (gSimCycCnt == 130) {
        //------------------------------------------------------
        //-- STEP-2.3 : DISABLE THE POSTING MODE
        //------------------------------------------------------
        piSHL_MmioPostSegEn.write(DISABLED);
        printf("[%4.4d] [TB] is disabling the segment posting mode.\n", gSimCycCnt);
    }
    else if (gSimCycCnt > 200) {
        printf("## TESTBENCH PART-2 ENDS HERE ####################\n");
        break;
    }
}  // End: while()
***/


}


/******************************************************************************
 * @brief Main function.
 *
 ******************************************************************************/
int main() {

    //------------------------------------------------------
    //-- DUT SIGNAL INTERFACES
    //------------------------------------------------------
    //-- MMIO/ Configuration Interfaces
    ap_uint<2>          sMMIO_TAF_EchoCtrl;
    CmdBit              sMMIO_TAF_PostSegEn;
    CmdBit              sMMIO_TAF_CaptSegEn;
    //-- TRIF / Session Connection Id
    SessionId           sTRIF_TAF_SessConId;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------
    //-- TRIF / TCP Data Interfaces
    stream<AppData>     ssTRIF_TAF_Data   ("ssTRIF_TAF_Data");
    stream<AppMeta>     ssTRIF_TAF_SessId ("ssTRIF_TAF_SessId");
    stream<AppData>     ssTAF_TRIF_Data   ("ssTAF_TRIF_Data");
    stream<AppMeta>     ssTAF_TRIF_SessId ("ssTAF_TRIF_SessId");

    //-- TRIF / Listen Interfaces
    stream<AppLsnReq>   ssTAF_TRIF_LsnReq ("ssTRIF_TAF_LsnReq");
    stream<AppLsnAck>   ssTRIF_TAF_LsnAck ("ssTRIF_TAF_LsnAck");

    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    int nrErr  = 0;
    int segCnt = 0;

    printf("#####################################################\n");
    printf("## TESTBENCH PART-1 STARTS HERE                    ##\n");
    printf("#####################################################\n");

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {
        //-------------------------------------------------
        //-- EMULATE TRIF
        //-------------------------------------------------
        pTRIF(
            nrErr,
            //-- MMIO / Configuration Interfaces
            &sMMIO_TAF_EchoCtrl,
            &sMMIO_TAF_PostSegEn,
            &sMMIO_TAF_CaptSegEn,
            //-- TAF / TCP Data Interfaces
            ssTRIF_TAF_Data,
            ssTRIF_TAF_SessId,
            //-- TAF / TCP Data Interface
            ssTAF_TRIF_Data,
            ssTAF_TRIF_SessId,
            //-- TRIF / Session Connection Id
            &sTRIF_TAF_SessConId);

        //-------------------------------------------------
        //-- RUN DUT
        //-------------------------------------------------
        tcp_app_flash(
            //-- MMIO / Configuration Interfaces
            &sMMIO_TAF_EchoCtrl,
            &sMMIO_TAF_PostSegEn,
            //[TODO] *piSHL_MmioCaptSegEn,
            //-- TRIF / Session Connect Id Interface
            &sTRIF_TAF_SessConId,
            //-- TRIF / TCP Rx Data Interface
            ssTRIF_TAF_Data,
            ssTRIF_TAF_SessId,
            //-- TRIF / TCP Tx Data Interface
            ssTAF_TRIF_Data,
            ssTAF_TRIF_SessId);

        //------------------------------------------------------
        //-- INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        gSimCycCnt++;
        if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
            printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
            gTraceEvent = false;
        }

    } while ( (gSimCycCnt < gMaxSimCycles) and
            !gFatalError and
            (nrErr < 10) );

    printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
    printf("############################################################################\n");
    printf("## TESTBENCH ENDS HERE                                                    ##\n");
    printf("############################################################################\n\n");

    if (nrErr) {
         printError(THIS_NAME, "###########################################################\n");
         printError(THIS_NAME, "#### TEST BENCH FAILED : TOTAL NUMBER OF ERROR(S) = %2d ####\n", nrErr);
         printError(THIS_NAME, "###########################################################\n");
     }
         else {
         printInfo(THIS_NAME, "#############################################################\n");
         printInfo(THIS_NAME, "####               SUCCESSFUL END OF TEST                ####\n");
         printInfo(THIS_NAME, "#############################################################\n");
     }

    return(nrErr);

}
