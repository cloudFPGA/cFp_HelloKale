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
 * @file       : test_udp_shell_if.cpp
 * @brief      : Testbench for the UDP Shell Interface (USIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE
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
 *****************************************************************************/

#include "test_udp_shell_if.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF     0x0000
#define TRACE_UOE    1 <<  1
#define TRACE_UAF    1 <<  2
#define TRACE_MMIO   1 <<  3
#define TRACE_ALL     0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)


/*****************************************************************************
 * @brief Emulate the behavior of the ROLE/UdpAppFlash (UAF).
 *
 * @param[in]  siUSIF_Data  Data from UdpShellInterface (USIF).
 * @param[in]  siUSIF_Meta  Metadata from [USIF].
 * @param[out] soUSIF_Data  Data to [USIF].
 * @param[out] soUSIF_Meta  Metadata to [USIF].
 * @param[out] soUSIF_DLen  Data len to [USIF].
 *
 ******************************************************************************/
void pUAF(
        //-- USIF / Rx Data Interface
        stream<UdpAppData>  &siUSIF_Data,
        stream<UdpAppMeta>  &siUSIF_Meta,
        //-- USIF / Tx Data Interface
        stream<UdpAppData>  &soUSIF_Data,
        stream<UdpAppMeta>  &soUSIF_Meta,
        stream<UdpAppDLen>  &soUSIF_DLen)
{

    const char *myRxName  = concat3(THIS_NAME, "/", "UAF-Rx");
    const char *myTxName  = concat3(THIS_NAME, "/", "UAF-Tx");

    static enum RxFsmStates { RX_WAIT_META=0, RX_STREAM } uaf_rxFsmState = RX_WAIT_META;

    //------------------------------------------------------
    //-- ALWAYS READ INCOMING DATA STREAM AND ECHO IT BACK
    //--  Warning: For sake of simplicity, this testbench is
    //--   operated in streaming mode by setting the 'DLen'
    //--   interface to zero.
    //------------------------------------------------------
    UdpAppData      appData;
    UdpAppMeta      appMeta;

    switch (uaf_rxFsmState ) {
    case RX_WAIT_META:
        if (!siUSIF_Meta.empty() and !soUSIF_Meta.full()) {
            siUSIF_Meta.read(appMeta);
            // Swap IP_SA/IP_DA and update UPD_SP/UDP_DP
            soUSIF_Meta.write(SocketPair(
                          SockAddr(appMeta.src.addr, DEFAULT_FPGA_SND_PORT),
                          SockAddr(appMeta.dst.addr, DEFAULT_HOST_LSN_PORT)));
            soUSIF_DLen.write(0);
            uaf_rxFsmState  = RX_STREAM;
        }
        break;
    case RX_STREAM:
        if (!siUSIF_Data.empty() && !soUSIF_Data.full()) {
            siUSIF_Data.read(appData);
            if (DEBUG_LEVEL & TRACE_UAF) {
                 printAxisRaw(myRxName, "Received data: ", appData);
            }
            soUSIF_Data.write(appData);
            if (appData.getTLast()) {
                uaf_rxFsmState  = RX_WAIT_META;
            }
        }
        break;
    }
}

/*******************************************************************************
 * @brief Emulate the behavior of the SHELL/MMIO.
 *
 * @param[in]  pSHL_Ready     Ready Signal from [SHELL].
 * @param[out] poUSIF_Enable  Enable signal to [USIF].
 *
 *******************************************************************************/
void pMMIO(
        //-- SHELL / Ready Signal
        StsBit      *piSHL_Ready,
        //-- MMIO / Enable Layer-7 (.i.e APP alias ROLE)
        CmdBit      *poUSIF_Enable)
{
    const char *myName  = concat3(THIS_NAME, "/", "MMIO");

    static bool mmio_printOnce = true;

    if (*piSHL_Ready) {
        *poUSIF_Enable = 1;
        if (mmio_printOnce) {
            printInfo(myName, "[SHELL/NTS/UOE] is ready -> Enabling operation of the UDP Shell Interface (USIF).\n");
            mmio_printOnce = false;
        }
    }
    else {
        *poUSIF_Enable = 0;
    }
}

/*****************************************************************************
 * @brief Emulate behavior of the SHELL/NTS/TCP Offload Engine (TOE).
 *
 * @param[in]  nrErr          A ref to the error counter of main.
 * @param[in]  goldFile       A ref to the file storing the DUT gold data.
 * @param[in]  dataFile       A ref to the file storing the DUT output data.
 * @param[in]  dgrmLen        The length of the test datatagram to generate.
 * @param[out] poMMIO_Ready   Ready signal to [MMIO].
 * @param[out] soUSIF_Data    The UDP datagram to [USIF].
 * @param[out] soUSIF_Meta    The UDP metadata to [USIF].
 * @param[in]  siUSIF_Data    The UDP datagram from [USIF].
 * @param[in]  siUSIF_Meta    The UDP metadata from [USIF].
 * @param[in]  siUSIF_DLen    The UDP data len from [USIF].
 * @param[in]  siUSIF_LsnReq  The listen port request from [USIF].
 * @param[out] soUSIF_LsnRep  The listen port reply to [USIF].
 * @param[in]  siUSIF_ClsReq  The close port request from [USIF].
 *
 ******************************************************************************/
void pUOE(
        int                   &nrErr,
        ofstream              &goldFile,
        ofstream              &dataFile,
        int                    dgrmLen,
        //-- MMIO / Ready Signal
        StsBit                *poMMIO_Ready,
        //-- UOE->USIF / UDP Rx Data Interfaces
        stream<UdpAppData>    &soUSIF_Data,
        stream<UdpAppMeta>    &soUSIF_Meta,
        //-- USIF->UOE / UDP Tx Data Interfaces
        stream<UdpAppData>    &siUSIF_Data,
        stream<UdpAppMeta>    &siUSIF_Meta,
        stream<UdpAppDLen>    &siUSIF_DLen,
        //-- USIF<->UOE / Listen Interfaces
        stream<UdpPort>       &siUSIF_LsnReq,
        stream<StsBool>       &soUSIF_LsnRep,
        //-- USIF<->UOE / Close Interfaces
        stream<UdpPort>       &siUSIF_ClsReq)
{

    static enum LsnStates { LSN_WAIT_REQ,  LSN_SEND_REP}  uoe_lsnState = LSN_WAIT_REQ;
    static enum RxpStates { RXP_SEND_META, RXP_SEND_DATA, RXP_DONE} uoe_rxpState = RXP_SEND_META;
    static enum TxpStates { TXP_WAIT_META, TXP_RECV_DATA} uoe_txpState = TXP_WAIT_META;

    static int  uoe_startupDelay = UOE_INIT_CYCLES;
    static int  uoe_rxpStartupDelay = 10;  // Wait a few more cycles until the listen port is opened
    static int  uoe_txpStartupDelay = 0;
    static bool uoe_isReady = false;
    static bool uoe_rxpIsReady = false;
    static bool uoe_txpIsReady = false;

    const char  *myLsnName = concat3(THIS_NAME, "/", "UOE/Listen");
    const char  *myRxpName = concat3(THIS_NAME, "/", "UOE/RxPath");
    const char  *myTxpName = concat3(THIS_NAME, "/", "UOE/TxPath");

    //------------------------------------------------------
    //-- FSM #0 - STARTUP DELAYS
    //------------------------------------------------------
    if (uoe_startupDelay) {
        *poMMIO_Ready = 0;
        uoe_startupDelay--;
    }
    else {
        *poMMIO_Ready = 1;
        if (uoe_rxpStartupDelay)
            uoe_rxpStartupDelay--;
        else
            uoe_rxpIsReady = true;
        if (uoe_txpStartupDelay)
            uoe_txpStartupDelay--;
        else
            uoe_txpIsReady = true;
    }

    //------------------------------------------------------
    //-- FSM #1 - LISTENING
    //------------------------------------------------------
    static UdpPort  uoe_lsnPortReq;
    switch (uoe_lsnState) {
    case LSN_WAIT_REQ: // CHECK IF A LISTENING REQUEST IS PENDING
        if (!siUSIF_LsnReq.empty()) {
            siUSIF_LsnReq.read(uoe_lsnPortReq);
            printInfo(myLsnName, "Received a listen port request #%d from [USIF].\n",
                      uoe_lsnPortReq.to_int());
            uoe_lsnState = LSN_SEND_REP;
        }
        break;
    case LSN_SEND_REP: // SEND REPLY BACK TO [USIF]
        if (!soUSIF_LsnRep.full()) {
            soUSIF_LsnRep.write(LSN_ACK);
            uoe_lsnState = LSN_WAIT_REQ;
        }
        else {
            printWarn(myLsnName, "Cannot send listen reply back to [USIF] because stream is full.\n");
        }
        break;
    }  // End-of: switch (uoe_lsnState) {

    //------------------------------------------------------
    //-- FSM #2 - RX DATA PATH
    //------------------------------------------------------
    UdpAppData         appData;
    static UdpAppMeta  uoe_rxMeta;
    static int         uoe_rxByteCnt;
    if (uoe_rxpIsReady) {
        switch (uoe_rxpState) {
        case RXP_SEND_META: // SEND METADATA TO [USIF]
            uoe_rxByteCnt = dgrmLen;
            uoe_rxMeta.src.addr = DEFAULT_HOST_IP4_ADDR;
            uoe_rxMeta.src.port = DEFAULT_HOST_SND_PORT;
            uoe_rxMeta.dst.addr = DEFAULT_FPGA_IP4_ADDR;
            uoe_rxMeta.dst.port = DEFAULT_FPGA_LSN_PORT;
            if (!soUSIF_Meta.full()) {
                soUSIF_Meta.write(uoe_rxMeta);
                printInfo(myRxpName, "Sending metadata to [USIF].\n");
                printSockPair(myRxpName, uoe_rxMeta);
                uoe_rxpState = RXP_SEND_DATA;
            }
            break;
        case RXP_SEND_DATA: // FORWARD DATA TO [USIF]
            appData.setTData((random() << 32) | random()) ;
            if (uoe_rxByteCnt > 8) {
                appData.setTKeep(0xFF);
                appData.setTLast(0);
                uoe_rxByteCnt -= 8;
            }
            else {
                appData.setTKeep(lenTotKeep(uoe_rxByteCnt));
                appData.setTLast(TLAST);
                uoe_rxpState = RXP_DONE;
            }
            if (!soUSIF_Data.full()) {
                soUSIF_Data.write(appData);
                writeAxisRawToFile(appData, goldFile);
                if (DEBUG_LEVEL & TRACE_UOE) {
                    printAxisRaw(myRxpName, "Sending data chunk to [USIF]: ", appData);
                }
            }
            break;
        case RXP_DONE: // END OF THE RX PATH SEQUENCE
            // ALL DATAGRAMS HAVE BEEN SENT
            uoe_rxpState = RXP_DONE;
            break;
        }  // End-of: switch())
    }

    //------------------------------------------------------
    //-- FSM #3 - TX DATA PATH
    //--    (Always drain the data coming from [USIF])
    //------------------------------------------------------
    static int  uoe_txByteCnt;
    if (uoe_txpIsReady) {
        switch (uoe_txpState) {
        case TXP_WAIT_META:
            if (!siUSIF_Meta.empty() and !siUSIF_DLen.empty()) {
                UdpAppMeta  appMeta;
                UdpAppDLen  appDLen;
                siUSIF_Meta.read(appMeta);
                siUSIF_DLen.read(appDLen);
                uoe_txByteCnt = appDLen;
                if (DEBUG_LEVEL & TRACE_UOE) {
                    if (uoe_txByteCnt == 0) {
                        printInfo(myTxpName, "This UDP Tx datagram is forwarded in streaming mode (DLen=0).\n");
                    }
                    else {
                        printInfo(myTxpName, "Receiving %d bytes of data from [USIF].\n", appDLen.to_int());
                    }
                    printSockPair(myTxpName, appMeta);
                }
                uoe_txpState = TXP_RECV_DATA;
            }
            break;
        case TXP_RECV_DATA:
            if (!siUSIF_Data.empty()) {
                UdpAppData     appData;
                siUSIF_Data.read(appData);
                writeAxisRawToFile(appData, dataFile);
                if (uoe_txByteCnt != 0) {
                    uoe_txByteCnt -= keepToLen(appData.getTKeep());
                }
                if (DEBUG_LEVEL & TRACE_UOE) {
                    printAxisRaw(myTxpName, "Received data chunk from [USIF] ", appData);
                }
                if (appData.getTLast()) {
                    uoe_txpState = TXP_WAIT_META;
                    if (uoe_txByteCnt != 0) {
                        printWarn(myTxpName, "TLAST is set but DLen != 0.\n");
                    }
                }
            }
            break;
        }
    }
}

/*****************************************************************************
 * @brief Main function for the test of the UDP Shell Interface (USIF).
 ******************************************************************************/
int main(int argc, char *argv[])
{
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
    //-- USIF->UAF / UDP Rx Data Interface
    stream<UdpAppData>    ssUSIF_UAF_Data     ("ssUSIF_UAF_Data");
    stream<UdpAppMeta>    ssUSIF_UAF_Meta     ("ssUSIF_UAF_Meta");
    //-- UOE / Control Port Interfaces
    stream<UdpPort>       ssUSIF_UOE_LsnReq   ("ssUSIF_UOE_LsnReq");
    stream<StsBool>       ssUOE_USIF_LsnRep   ("ssUOE_USIF_LsnRep");
    stream<UdpPort>       ssUSIF_UOE_ClsReq   ("ssUSIF_UOE_ClsReq");

    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    int         nrErr;
    ofstream    ofsData;
    ofstream    ofsGold;
    string      ofsGold_FileName = "../../../../test/ssUOE_USIF_Gold.dat";
    string      ofsData_FileName = "../../../../test/ssUSIF_UOE_Data.dat";
    int         lenOfDatagramTest;

    //------------------------------------------------------
    //-- PARSING THE TESBENCH ARGUMENTS
    //------------------------------------------------------
    if (argc == 1) {
        lenOfDatagramTest = 42; // In bytes
    }
    else if (argc == 2) {
        lenOfDatagramTest = atoi(argv[1]);
        if ((lenOfDatagramTest <= 0) or (lenOfDatagramTest >= 0x10000)) {
            printFatal(THIS_NAME, "Argument is out of range [0:65535].\n");
            return NTS_KO;
        }
    }
    else {
        printFatal(THIS_NAME, "Expected an argument specifying the length of the data stream to generate.\n");
        return NTS_KO;
    }
    gMaxSimCycles += lenOfDatagramTest;

    printf("#####################################################\n");
    printf("## TESTBENCH STARTS HERE                           ##\n");
    printf("#####################################################\n");
    gSimCycCnt = 0; // Simulation cycle counter as a global variable
    nrErr      = 0; // Total number of testbench errors

    //-- REMOVE PREVIOUS FILES AND OPEN NEW ONES
    remove(ofsData_FileName.c_str());
    if (!ofsData.is_open()) {
        ofsData.open(ofsData_FileName.c_str(), ofstream::out);
        if (!ofsData) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofsData_FileName.c_str());
            return(NTS_KO);
        }
    }
    remove(ofsGold_FileName.c_str());
    if (!ofsGold.is_open()) {
        ofsGold.open(ofsGold_FileName.c_str(), ofstream::out);
        if (!ofsGold) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofsGold_FileName.c_str());
            return(NTS_KO);
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
            ofsGold,
            ofsData,
            lenOfDatagramTest,
            //-- UOE / Ready Signal
            &sUOE_MMIO_Ready,
            //-- UOE->USIF / UDP Rx Data Interfaces
            ssUOE_USIF_Data,
            ssUOE_USIF_Meta,
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
        gSimCycCnt++;
        if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
            printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
            gTraceEvent = false;
        }

    } while ( (gSimCycCnt < gMaxSimCycles) or gFatalError or (nrErr > 10) );

    //-- CLOSE FILES
    ofsGold.close();
    ofsData.close();

    printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
    printf("############################################################################\n");
    printf("## TESTBENCH ENDS HERE                                                    ##\n");
    printf("############################################################################\n\n");

    //-- COMPARE DATA and GOLD STREAMS
    int res = system(("diff --brief -w " + \
                       std::string(ofsData_FileName) + " " + \
                       std::string(ofsGold_FileName) + " ").c_str());
    if (res) {
        printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
                   ofsData_FileName.c_str(), ofsGold_FileName.c_str());
        nrErr += 1;
    }

    if (nrErr) {
         printError(THIS_NAME, "###########################################################\n");
         printError(THIS_NAME, "#### TEST BENCH FAILED : TOTAL NUMBER OF ERROR(S) = %2d ####\n", nrErr);
         printError(THIS_NAME, "###########################################################\n");

         printInfo(THIS_NAME, "FYI - You may want to check for \'ERROR\' and/or \'WARNING\' alarms in the LOG file...\n\n");
    }
         else {
         printInfo(THIS_NAME, "#############################################################\n");
         printInfo(THIS_NAME, "####               SUCCESSFUL END OF TEST                ####\n");
         printInfo(THIS_NAME, "#############################################################\n");
     }

    return(nrErr);

}
