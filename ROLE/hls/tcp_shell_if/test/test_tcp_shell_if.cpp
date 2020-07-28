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
 * @file       : test_tcp_shell_if.cpp
 * @brief      : Testbench for the TCP Shell Interface (TSIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE
 * Language    : Vivado HLS
 *
 * \ingroup ROLE
 * \addtogroup ROLE_TSIF
 * \{
 *****************************************************************************/

#include "test_tcp_shell_if.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF      0x0000
#define TRACE_TOE     1 <<  1
#define TRACE_TOE_LSN 1 <<  2
#define TRACE_TOE_OPN 1 <<  3
#define TRACE_TOE_RXP 1 <<  4
#define TRACE_TOE_TXP 1 <<  5
#define TRACE_TAF     1 <<  6
#define TRACE_MMIO    1 <<  7
#define TRACE_ALL     0xFFFF
#define DEBUG_LEVEL (TRACE_TOE_LSN | TRACE_TOE_RXP | TRACE_MMIO)

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
 * @brief Emulate the behavior of the TcpAppFlash (TAF).
 *
 * @param[in]  ofTAF_Data   A ref to the output TxApp file to write to.
 * @param[in]  siTSIF_Data  Data stream from TcpShellInterface (TSIF).
 * @param[in]  siTSIF_Meta  Session ID to [TSIF].
 * @param[out] soTSIF_Data  Data stream to [TSIF].
 * @param[out] soTSIF_Meta  SessionID to [TSIF].
 *******************************************************************************/
void pTAF(
        ofstream            &ofTAF_Data,
        stream<TcpAppData>  &siTSIF_Data,
        stream<TcpAppMeta>  &siTSIF_Meta,
        stream<TcpAppData>  &soTSIF_Data,
        stream<TcpAppMeta>  &soTSIF_Meta)
{
    TcpAppData      currChunk;
    TcpAppMeta      tcpSessId;

    const char *myName = concat3(THIS_NAME, "/", "TAF");

    //------------------------------------------------------
    //-- ALWAYS READ INCOMING DATA STREAM AND ECHO IT BACK
    //------------------------------------------------------
    static enum FsmStates { RX_WAIT_META=0, RX_STREAM } taf_fsmState = RX_WAIT_META;

    switch (taf_fsmState ) {
    case RX_WAIT_META:
        if (!siTSIF_Meta.empty() and !soTSIF_Meta.full()) {
            siTSIF_Meta.read(tcpSessId);
            soTSIF_Meta.write(tcpSessId);
            taf_fsmState  = RX_STREAM;
        }
        break;
    case RX_STREAM:
        if (!siTSIF_Data.empty() && !soTSIF_Data.full()) {
            siTSIF_Data.read(currChunk);
            int bytes = writeAxisAppToFile(currChunk, ofTAF_Data);
            soTSIF_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_TAF) { printAxisRaw(myName, "soTSIF_Data =", currChunk); }
            if (currChunk.getTLast()) {
                taf_fsmState  = RX_WAIT_META;
            }
        }
        break;
    }
}

/*******************************************************************************
 * @brief Emulate the behavior of the SHELL & MMIO.
 *
 * @param[in]  piSHL_Ready    Ready signal from [SHELL].
 * @param[out] poTSIF_Enable  Enable signal to [TSIF] (.i.e, Enable Layer-7).
 *
 *******************************************************************************/
void pMMIO(
        StsBit      *piSHL_Ready,
        CmdBit      *poTSIF_Enable)
{
    const char *myName  = concat3(THIS_NAME, "/", "MMIO");

    static bool printOnce = true;

    if (*piSHL_Ready) {
        *poTSIF_Enable = 1;
        if (printOnce) {
            printInfo(myName, "[SHELL/NTS/TOE] is ready -> Enabling operation of the TCP Shell Interface [TSIF].\n");
            printOnce = false;
        }
    }
    else {
        *poTSIF_Enable = 0;
    }
}

/*******************************************************************************
 * @brief Emulate behavior of the SHELL/NTS/TCP Offload Engine (TOE).
 *
 * @param[in]  nrErr         A ref to the error counter of main.
 * @param[in]  ofTAF_Gold    A ref to the TAF gold file to write.
 * @param[out] poMMIO_Ready  Ready signal to [MMIO].
 * @param[out] soTSIF_Notif  Notification to TcpShellInterface (TSIF).
 * @param[in]  siTSIF_DReq   Data read request from [TSIF].
 * @param[out] soTSIF_Data   Data to [TSIF].
 * @param[out] soTSIF_Meta   Session Id to [TSIF].
 * @param[in]  siTSIF_LsnReq Listen port request from [TSIF].
 * @param[out] soTSIF_LsnRep Listen port reply to [TSIF].
 *******************************************************************************/
void pTOE(
        int                   &nrErr,
       ofstream               &ofTAF_Gold,
        //-- MMIO / Ready Signal
        StsBit                *poMMIO_Ready,
        //-- TSIF / Tx Data Interfaces
        stream<TcpAppNotif>   &soTSIF_Notif,
        stream<TcpAppRdReq>   &siTSIF_DReq,
        stream<TcpAppData>    &soTSIF_Data,
        stream<TcpAppMeta>    &soTSIF_Meta,
        //-- TSIF / Listen Interfaces
        stream<TcpAppLsnReq>  &siTSIF_LsnReq,
        stream<TcpAppLsnRep>  &soTSIF_LsnRep,
        //-- TSIF / Rx Data Interfaces
        stream<TcpAppData>    &siTSIF_Data,
        stream<TcpAppMeta>    &siTSIF_Meta,
        stream<TcpAppWrSts>   &soTSIF_DSts,
        //-- TSIF / Open Interfaces
        stream<TcpAppOpnReq>  &siTSIF_OpnReq,
        stream<TcpAppOpnRep>  &soTSIF_OpnRep)
{

    const char  *myLsnName = concat3(THIS_NAME, "/", "TOE/Listen");
    const char  *myOpnName = concat3(THIS_NAME, "/", "TOE/OpnCon");
    const char  *myRxpName = concat3(THIS_NAME, "/", "TOE/RxPath");
    const char  *myTxpName = concat3(THIS_NAME, "/", "TOE/TxPath");

    TcpSegLen  tcpSegLen  = 32;

    static Ip4Addr toe_hostIp4Addr = DEFAULT_HOST_IP4_ADDR;
    static TcpPort toe_fpgaLsnPort = -1;
    static TcpPort toe_hostTcpSrcPort = DEFAULT_HOST_TCP_SRC_PORT;
    static TcpPort toe_hostTcpDstPort = ECHO_MODE_LSN_PORT;
    static int     toe_loop        = 1;

    static enum LsnStates { LSN_WAIT_REQ,   LSN_SEND_REP}  lsnState = LSN_WAIT_REQ;
    static enum OpnStates { OPN_WAIT_REQ,   OPN_SEND_REP}  opnState = OPN_WAIT_REQ;
    static enum RxpStates { RXP_SEND_NOTIF, RXP_WAIT_DREQ,
                            RXP_SEND_DATA,  RXP_NEXT_SESS,
                                            RXP_DONE}      rxpState = RXP_SEND_NOTIF;
    static enum TxpStates { TXP_WAIT_META,  TXP_RECV_DATA} txpState = TXP_WAIT_META;

    static int  toe_startupDelay = 0x8000;
    static int  toe_rxpStartupDelay = 100;
    static int  toe_txpStartupDelay = 100;
    static bool toe_isReady = false;
    static bool toe_rxpIsReady = false;
    static bool toe_txpIsReady = false;

    //------------------------------------------------------
    //-- FSM #0 - STARTUP DELAYS
    //------------------------------------------------------
    if (toe_startupDelay) {
        *poMMIO_Ready = 0;
        toe_startupDelay--;
    }
    else {
        *poMMIO_Ready = 1;
        if (toe_rxpStartupDelay)
            toe_rxpStartupDelay--;
        else
            toe_rxpIsReady = true;
        if (toe_txpStartupDelay)
            toe_txpStartupDelay--;
        else
            toe_txpIsReady = true;
    }

    //------------------------------------------------------
    //-- FSM #1 - LISTENING
    //------------------------------------------------------
    static TcpAppLsnReq toe_appLsnPortReq;
    switch (lsnState) {
    case LSN_WAIT_REQ: // CHECK IF A LISTENING REQUEST IS PENDING
        if (!siTSIF_LsnReq.empty()) {
            siTSIF_LsnReq.read(toe_appLsnPortReq);
            if (DEBUG_LEVEL & TRACE_TOE_LSN) {
                printInfo(myLsnName, "Received a listen port request #%d from [TSIF].\n",
                          toe_appLsnPortReq.to_int());
            }
            lsnState = LSN_SEND_REP;
        }
        break;
    case LSN_SEND_REP: // SEND REPLY BACK TO [TSIF]
        if (!soTSIF_LsnRep.full()) {
            soTSIF_LsnRep.write(TcpAppLsnRep(NTS_OK));
            toe_fpgaLsnPort = toe_appLsnPortReq.to_int();
            lsnState = LSN_WAIT_REQ;
        }
        else {
            printWarn(myLsnName, "Cannot send listen reply back to [TSIF] because stream is full.\n");
        }
        break;
    }  // End-of: switch (lsnState) {

    //------------------------------------------------------
    //-- FSM #2 - OPEN CONNECTION
    //------------------------------------------------------
    TcpAppOpnReq  toe_opnReq;
    TcpAppOpnRep  opnReply(DEFAULT_SESSION_ID, ESTABLISHED);
    switch(opnState) {
    case OPN_WAIT_REQ:
        if (!siTSIF_OpnReq.empty()) {
            siTSIF_OpnReq.read(toe_opnReq);
            if (DEBUG_LEVEL & TRACE_TOE_OPN) {
                printInfo(myOpnName, "Received a request to open the following remote socket address:\n");
                printSockAddr(myOpnName, LE_SockAddr(toe_opnReq.addr, toe_opnReq.port));
            }
            opnState = OPN_SEND_REP;
        }
        break;
    case OPN_SEND_REP:
        if (!soTSIF_OpnRep.full()) {
            soTSIF_OpnRep.write(opnReply);
            opnState = OPN_WAIT_REQ;
        }
        else {
            printWarn(myOpnName, "Cannot send open connection reply back to [TSIF] because stream is full.\n");
        }
        break;
    }  // End-of: switch (opnState) {

    //------------------------------------------------------
    //-- FSM #3 - RX DATA PATH
    //------------------------------------------------------
    static TcpAppRdReq toe_appRdReq;
    static SessionId   toe_sessId;
    static int         toe_appRxBytCnt=0;
    static int         toe_appRxSegCnt=0;
    static ap_uint<64> toe_appRxData=0;
    static int         toe_sessCnt=0;
    const  int         nrSegToSend=3;
    const  int         nrSessToSend=3;
    ap_uint< 8>        keep;
    ap_uint< 1>        last;

    if (toe_rxpIsReady) {
        switch (rxpState) {
        case RXP_SEND_NOTIF: // SEND A DATA NOTIFICATION TO [TSIF]
            switch (toe_sessCnt) {
            case 1:
                toe_hostTcpDstPort = RECV_MODE_LSN_PORT;
                break;
            default:
                toe_hostTcpDstPort = ECHO_MODE_LSN_PORT;
                break;
            }
            toe_sessId  = DEFAULT_SESSION_ID + toe_sessCnt;
            tcpSegLen   = DEFAULT_SESSION_LEN;
            toe_hostIp4Addr = DEFAULT_HOST_IP4_ADDR;
            toe_hostTcpSrcPort = DEFAULT_HOST_TCP_SRC_PORT;
            if (!soTSIF_Notif.full()) {
                soTSIF_Notif.write(TcpAppNotif(toe_sessId,  tcpSegLen, toe_hostIp4Addr,
                                            toe_hostTcpSrcPort, toe_hostTcpDstPort));
                if (DEBUG_LEVEL & TRACE_TOE_RXP) {
                    printInfo(myRxpName, "Sending notification #%d to [TSIF] (sessId=%d, segLen=%d, dstPort=%d).\n",
                              toe_appRxSegCnt, toe_sessId.to_int(), tcpSegLen.to_int(), toe_hostTcpDstPort.to_uint());
                }
                rxpState = RXP_WAIT_DREQ;
            }
            break;
        case RXP_WAIT_DREQ: // WAIT FOR A DATA REQUEST FROM [TSIF]
            if (!siTSIF_DReq.empty()) {
                siTSIF_DReq.read(toe_appRdReq);
                if (DEBUG_LEVEL & TRACE_TOE_RXP) {
                    printInfo(myRxpName, "Received a data read request from [TSIF] (sessId=%d, segLen=%d).\n",
                              toe_appRdReq.sessionID.to_int(), toe_appRdReq.length.to_int());
                }
                toe_appRxBytCnt = 0;
                rxpState = RXP_SEND_DATA;
           }
           break;
        case RXP_SEND_DATA: // FORWARD DATA AND METADATA TO [TSIF]
            // Note: We always assume 'tcpSegLen' is multiple of 8B.
            keep = 0xFF;
            last = (toe_appRxBytCnt==tcpSegLen) ? 1 : 0;
            if (toe_appRxBytCnt == 0) {
                if (!soTSIF_Meta.full() && !soTSIF_Data.full()) {
                    soTSIF_Meta.write(toe_sessId);
                    TcpAppData currChunk(toe_appRxData, keep, last);
                    soTSIF_Data.write(currChunk);
                    if (toe_hostTcpDstPort != RECV_MODE_LSN_PORT) {
                        int bytes = writeAxisAppToFile(currChunk, ofTAF_Gold);
                    }
                    if (DEBUG_LEVEL & TRACE_TOE_RXP) {
                        printAxisRaw(myRxpName, "soTSIF_Data =", TcpAppData(toe_appRxData, keep, last));
                    }
                    toe_appRxBytCnt += 8;
                    toe_appRxData += 8;
                }
                else
                    break;
            }
            else if (toe_appRxBytCnt <= (tcpSegLen)) {
                if (!soTSIF_Data.full()) {
                    TcpAppData currChunk(toe_appRxData, keep, last);
                    soTSIF_Data.write(currChunk);
                    if (toe_hostTcpDstPort != RECV_MODE_LSN_PORT) {
                        int bytes = writeAxisAppToFile(currChunk, ofTAF_Gold);
                    }
                    if (DEBUG_LEVEL & TRACE_TOE_RXP) {
                        printAxisRaw(myRxpName, "soTSIF_Data =", TcpAppData(toe_appRxData, keep, last));
                    }
                    toe_appRxBytCnt += 8;
                    toe_appRxData += 8;
                }
            }
            else {
                toe_appRxSegCnt++;
                if (toe_appRxSegCnt == nrSegToSend) {
                    rxpState = RXP_NEXT_SESS;
                }
                else {
                    rxpState = RXP_SEND_NOTIF;
                }
            }
            break;
        case RXP_NEXT_SESS:
            toe_appRxSegCnt = 0;
            toe_sessCnt += 1;
            if (toe_sessCnt == nrSessToSend)
                rxpState = RXP_DONE;
            else
                rxpState = RXP_SEND_NOTIF;
            break;
        case RXP_DONE: // END OF THE RX PATH SEQUENCE
            // ALL SEGMENTS HAVE BEEN SENT
            break;
        }  // End-of: switch())
    }

    //------------------------------------------------------
    //-- FSM #4 - TX DATA PATH
    //--    (Always drain the toe_appRxData coming from [TSIF])
    //------------------------------------------------------
    if (toe_txpIsReady) {
        switch (txpState) {
        case TXP_WAIT_META:
            if (!siTSIF_Meta.empty() && !siTSIF_Data.empty()) {
                TcpAppData      appData;
                TcpAppMeta      sessId;
                siTSIF_Meta.read(sessId);
                siTSIF_Data.read(appData);
                if (DEBUG_LEVEL & TRACE_TOE_TXP) {
                    printInfo(myTxpName, "Receiving data for session #%d\n", sessId.to_uint());
                    printAxisRaw(myTxpName, "siTSIF_Data =", appData);
                }
                if (!appData.getTLast())
                    txpState = TXP_RECV_DATA;
            }
            break;
        case TXP_RECV_DATA:
            if (!siTSIF_Data.empty()) {
                TcpAppData     appData;
                siTSIF_Data.read(appData);
                if (DEBUG_LEVEL & TRACE_TOE_TXP) {
                    printAxisRaw(myTxpName, "siTSIF_Data =", appData);
                }
                if (appData.getTLast())
                    txpState = TXP_WAIT_META;
            }
            break;
        }
    }
}

/*******************************************************************************
 * @brief Main function.
 *
 *******************************************************************************/
int main(int argc, char *argv[]) {

    //------------------------------------------------------
    //-- DUT SIGNAL INTERFACES
    //------------------------------------------------------
    //-- SHL / Mmio Interface
    CmdBit              sMMIO_TSIF_Enable;
    //-- TOE / Ready Signal
    StsBit              sTOE_MMIO_Ready;
    //-- TSIF / Session Connect Id Interface
    SessionId           sTSIF_TAF_SConId;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------
    //-- TAF / Rx Data Interface
    stream<TcpAppData>    ssTAF_TSIF_Data     ("ssTAF_TSIF_Data");
    stream<TcpAppMeta>    ssTAF_TSIF_Meta     ("ssTAF_TSIF_Meta");
    //-- TSIF / Tx Data Interface
    stream<TcpAppData>    ssTSIF_TAF_Data     ("ssTSIF_TAF_Data");
    stream<TcpAppMeta>    ssTSIF_TAF_Meta     ("ssTSIF_TAF_Meta");
    //-- TOE  / Rx Data Interfaces
    stream<TcpAppNotif>   ssTOE_TSIF_Notif    ("ssTOE_TSIF_Notif");
    stream<TcpAppData>    ssTOE_TSIF_Data     ("ssTOE_TSIF_Data");
    stream<TcpAppMeta>    ssTOE_TSIF_Meta     ("ssTOE_TSIF_Meta");
    //-- TSIF / Rx Data Interface
    stream<TcpAppRdReq>   ssTSIF_TOE_DReq     ("ssTSIF_TOE_DReq");
    //-- TOE  / Listen Interface
    stream<TcpAppLsnRep>  ssTOE_TSIF_LsnRep   ("ssTOE_TSIF_LsnRep");
    //-- TSIF / Listen Interface
    stream<TcpAppLsnReq>  ssTSIF_TOE_LsnReq   ("ssTSIF_TOE_LsnReq");
    //-- TOE  / Tx Data Interfaces
    stream<TcpAppWrSts>   ssTOE_TSIF_DSts     ("ssTOE_TSIF_DSts");
    //-- TSIF  / Tx Data Interfaces
    stream<TcpAppData>    ssTSIF_TOE_Data     ("ssTSIF_TOE_Data");
    stream<TcpAppMeta>    ssTSIF_TOE_Meta     ("ssTSIF_TOE_Meta");
    //-- TOE  / Connect Interfaces
    stream<TcpAppOpnRep>  ssTOE_TSIF_OpnRep   ("ssTOE_TSIF_OpnRep");
    //-- TSIF / Connect Interfaces
    stream<TcpAppOpnReq>  ssTSIF_TOE_OpnReq   ("ssTSIF_TOE_OpnReq");
    stream<TcpAppClsReq>  ssTSIF_TOE_ClsReq   ("ssTSIF_TOE_ClsReq");

    //-----------------------------------------------------
    //-- TESTBENCH INPUT & OUTPUT FILE STREAMS
    //-----------------------------------------------------
    ofstream ofTAF_Data;  // APP byte streams delivered to TAF
    ofstream ofTAF_Gold;  // Gold reference file for 'ofTAF_Data'
    const char  *ofTAF_DataName  = "../../../../test/simOutFiles/soTAF.dat";
    const char  *ofTAF_GoldName  = "../../../../test/simOutFiles/soTAF.gold";

    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    int  nrErr = 0;  // Total number of testbench errors
    gSimCycCnt = 0; // Simulation cycle counter as a global variable

    //------------------------------------------------------
    //-- REMOVE PREVIOUS OLD SIM FILES and OPEN NEW ONES
    //------------------------------------------------------
    remove(ofTAF_DataName);
    ofTAF_Data.open(ofTAF_DataName);
    if (!ofTAF_Data) {
        printError(THIS_NAME, "Cannot open the Application Tx file:  \n\t %s \n", ofTAF_DataName);
        return -1;
    }
    remove(ofTAF_GoldName);
    ofTAF_Gold.open(ofTAF_GoldName);
    if (!ofTAF_Gold) {
        printInfo(THIS_NAME, "Cannot open the Application Tx gold file:  \n\t %s \n", ofTAF_GoldName);
        return -1;
    }

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_uoe' STARTS HERE                                       ##\n");
    printInfo(THIS_NAME, "############################################################################\n\n");
    printInfo(THIS_NAME, "This testbench will be executed without parameters: \n");
    printf("\n\n");

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {
        //-------------------------------------------------
        //-- EMULATE TOE
        //-------------------------------------------------
        pTOE(
            nrErr,
            ofTAF_Gold,
            //-- TOE / Ready Signal
            &sTOE_MMIO_Ready,
            //-- TOE / Tx Data Interfaces
            ssTOE_TSIF_Notif,
            ssTSIF_TOE_DReq,
            ssTOE_TSIF_Data,
            ssTOE_TSIF_Meta,
            //-- TOE / Listen Interfaces
            ssTSIF_TOE_LsnReq,
            ssTOE_TSIF_LsnRep,
            //-- TOE / Tx Data Interfaces
            ssTSIF_TOE_Data,
            ssTSIF_TOE_Meta,
            ssTOE_TSIF_DSts,
            //-- TOE / Open Interfaces
            ssTSIF_TOE_OpnReq,
            ssTOE_TSIF_OpnRep);

        //-------------------------------------------------
        //-- EMULATE SHELL/MMIO
        //-------------------------------------------------
        pMMIO(
            //-- TOE / Ready Signal
            &sTOE_MMIO_Ready,
            //-- MMIO / Enable Layer-7 (.i.e APP alias ROLE)
            &sMMIO_TSIF_Enable);

        //-------------------------------------------------
        //-- RUN DUT
        //-------------------------------------------------
        tcp_shell_if(
            //-- SHELL / Mmio Interface
            &sMMIO_TSIF_Enable,
            //-- TAF / Rx & Tx Data Interfaces
            ssTAF_TSIF_Data,
            ssTAF_TSIF_Meta,
            ssTSIF_TAF_Data,
            ssTSIF_TAF_Meta,
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
            ssTSIF_TOE_Meta,
            ssTOE_TSIF_DSts,
            //-- TOE / Tx Open Interfaces
            ssTSIF_TOE_OpnReq,
            ssTOE_TSIF_OpnRep,
            //-- TOE / Close Interfaces
            ssTSIF_TOE_ClsReq,
            //-- ROLE / Session Connect Id Interface
            &sTSIF_TAF_SConId);

        //-------------------------------------------------
        //-- EMULATE ROLE/TcpApplicationFlash
        //-------------------------------------------------
        pTAF(
            ofTAF_Data,
            //-- TSIF / Data Interface
            ssTSIF_TAF_Data,
            ssTSIF_TAF_Meta,
            //-- TAF / Data Interface
            ssTAF_TSIF_Data,
            ssTAF_TSIF_Meta);

        //------------------------------------------------------
        //-- INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        stepSim();

    } while ( (gSimCycCnt < gMaxSimCycles) or
              gFatalError or
              (nrErr > 10) );

    //---------------------------------
    //-- CLOSING OPEN FILES
    //---------------------------------
    ofTAF_Data.close();
    ofTAF_Gold.close();

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_tcpshell_if' ENDS HERE                                 ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    stepSim();

    //---------------------------------------------------------------
    //-- COMPARE RESULT DATA FILE WITH GOLDEN FILE
    //---------------------------------------------------------------
    int res = system(("diff --brief -w " + std::string(ofTAF_DataName) + " " + std::string(ofTAF_GoldName) + " ").c_str());
    if (res != 0) {
    printError(THIS_NAME, "File \"%s\" differs from file \"%s\" \n", ofTAF_DataName, ofTAF_GoldName);
        nrErr++;
    }

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

/*! \} */
