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
 * Component   : cFp_Monolithic / ROLE
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
 * \ingroup ROLE
 * \addtogroup ROLE_USIF
 * \{
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

#define DEBUG_LEVEL (TRACE_UOE)

/*******************************************************************************
 * @brief Increment the simulation counter
 *******************************************************************************/
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
 * @brief Emulate the behavior of the ROLE/UdpAppFlash (UAF).
 *
 * @param[in]  siUSIF_Data  Data from UdpShellInterface (USIF).
 * @param[in]  siUSIF_Meta  Metadata from [USIF].
 * @param[out] soUSIF_Data  Data to [USIF].
 * @param[out] soUSIF_Meta  Metadata to [USIF].
 * @param[out] soUSIF_DLen  Data len to [USIF].
 *
 * @details
 *  ALWAYS READ INCOMING DATA STREAM AND ECHO IT BACK.
 *  Warning: For sake of simplicity, this testbench is operated in streaming
 *   mode by setting the 'DLen' interface to zero.
 *******************************************************************************/
void pUAF(
        //-- USIF / Rx Data Interface
        stream<UdpAppData>  &siUSIF_Data,
        stream<UdpAppMetb>  &siUSIF_Meta,
        //-- USIF / Tx Data Interface
        stream<UdpAppData>  &soUSIF_Data,
        stream<UdpAppMetb>  &soUSIF_Meta,
        stream<UdpAppDLen>  &soUSIF_DLen)
{

    const char *myRxName  = concat3(THIS_NAME, "/", "UAF-Rx");
    const char *myTxName  = concat3(THIS_NAME, "/", "UAF-Tx");

    static enum RxFsmStates { RX_IDLE=0, RX_STREAM } uaf_rxFsmState=RX_IDLE;

    UdpAppData      appData;
    UdpAppMetb      appMeta;

    switch (uaf_rxFsmState ) {
    case RX_IDLE:
        if (!siUSIF_Meta.empty() and !soUSIF_Meta.full()) {
            siUSIF_Meta.read(appMeta);
            // Swap IP_SA/IP_DA and update UPD_SP/UDP_DP
            //OBSOLETE_20210226 soUSIF_Meta.write(SocketPair(
            //OBSOLETE_20210226               SockAddr(appMeta.dst.addr, DEFAULT_FPGA_SND_PORT),
            //OBSOLETE_20210226               SockAddr(appMeta.src.addr, DEFAULT_HOST_LSN_PORT)));
            soUSIF_Meta.write(UdpAppMetb(appMeta.ip4DstAddr, DEFAULT_FPGA_SND_PORT,
                                         appMeta.ip4SrcAddr, DEFAULT_HOST_LSN_PORT));
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
                uaf_rxFsmState  = RX_IDLE;
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
 * @param[in]  dataGoldFile   A ref to the file storing the DUT gold data.
 * @param[in]  dataFile       A ref to the file storing the DUT output data.
 * @param[in]  metaGoldFile   A ref to the file storing the DUT gold meta.
 * @param[in]  metaFile       A ref to the file storing the DUT output meta.
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
        ofstream              &dataGoldFile,
        ofstream              &dataFile,
        ofstream              &metaGoldFile,
        ofstream              &metaFile,
        int                    echoDgrmLen,
        SockAddr               testSock,
        int                    testDgrmLen,
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

    static enum LsnStates { LSN_WAIT_REQ,  LSN_SEND_REP }  uoe_lsnState = LSN_WAIT_REQ;
    static enum RxpStates { RXP_SEND_META, RXP_SEND_DATA, \
                            RXP_SEND_8801, RXP_DONE }      uoe_rxpState = RXP_SEND_META;
    static enum TxpStates { TXP_WAIT_META, TXP_RECV_DATA } uoe_txpState = TXP_WAIT_META;

    static int  uoe_startupDelay = UOE_INIT_CYCLES;
    static int  uoe_rxpStartupDelay = 50;  // Wait until all the listen port are opened
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
            soUSIF_LsnRep.write(OK);
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
    static Ly4Len      uoe_txByteCnt;
    static int         uoe_dgmCnt=0;
    static int         uoe_waitEndOfTxTest=0;
    const  int         nrDgmToSend=7;
    if (uoe_rxpIsReady) {
        switch (uoe_rxpState) {
        case RXP_SEND_META: // SEND METADATA TO [USIF]
            if (!soUSIF_Meta.full()) {
                if (uoe_waitEndOfTxTest) {
                    // Give USIF the time to finish sending all the requested bytes
                    uoe_waitEndOfTxTest--;
                }
                else if (uoe_dgmCnt == nrDgmToSend) {
                    uoe_rxpState = RXP_DONE;
                }
                else {
                    uoe_rxMeta.src.addr = DEFAULT_HOST_IP4_ADDR;
                    uoe_rxMeta.src.port = DEFAULT_HOST_SND_PORT;
                    uoe_rxMeta.dst.addr = DEFAULT_FPGA_IP4_ADDR;
                    switch (uoe_dgmCnt) {
                    case 1:
                    case 3:
                        uoe_rxMeta.dst.port = RECV_MODE_LSN_PORT;
                        uoe_rxByteCnt = echoDgrmLen;
                        gMaxSimCycles += (echoDgrmLen / 8);
                        uoe_waitEndOfTxTest = 0;
                        uoe_rxpState = RXP_SEND_DATA;
                        break;
                    case 2:
                    case 4:
                        uoe_rxMeta.dst.port = XMIT_MODE_LSN_PORT;
                        uoe_txByteCnt = testDgrmLen;
                        gMaxSimCycles += (testDgrmLen / 8);
                        uoe_waitEndOfTxTest = (testDgrmLen / 8) + 1;
                        uoe_rxpState = RXP_SEND_8801;
                        break;
                    default:
                        uoe_rxMeta.dst.port = ECHO_MODE_LSN_PORT;
                        uoe_rxByteCnt = echoDgrmLen;
                        gMaxSimCycles += (echoDgrmLen / 8);
                        uoe_waitEndOfTxTest = 0;
                        uoe_rxpState = RXP_SEND_DATA;
                        // Swap IP_SA/IP_DA and update UPD_SP/UDP_DP
                        UdpAppMeta udpGldMeta(SockAddr(uoe_rxMeta.dst.addr, DEFAULT_FPGA_SND_PORT),
                                              SockAddr(uoe_rxMeta.src.addr, DEFAULT_HOST_LSN_PORT));
                        // Dump this socket pair to a gold file
                        writeSocketPairToFile(udpGldMeta, metaGoldFile);
                        break;
                    }
                    soUSIF_Meta.write(uoe_rxMeta);
                    if (DEBUG_LEVEL & TRACE_UOE) {
                        printInfo(myRxpName, "Sending metadata to [USIF].\n");
                        printSockPair(myRxpName, uoe_rxMeta);
                    }
                    uoe_dgmCnt += 1;
                }
            }
            break;
        case RXP_SEND_DATA: // FORWARD DATA TO [USIF]
            if (!soUSIF_Data.full()) {
                appData.setTData((random() << 32) | random()) ;
                if (uoe_rxByteCnt > 8) {
                    appData.setTKeep(0xFF);
                    appData.setTLast(0);
                    uoe_rxByteCnt -= 8;
                }
                else {
                    appData.setTKeep(lenTotKeep(uoe_rxByteCnt));
                    appData.setTLast(TLAST);
                    uoe_rxpState = RXP_SEND_META;
                }
                soUSIF_Data.write(appData);
                if (DEBUG_LEVEL & TRACE_UOE) {
                    printAxisRaw(myRxpName, "Sending data chunk to [USIF]: ", appData);
                }
                if (uoe_rxMeta.dst.port != RECV_MODE_LSN_PORT) {
                    writeAxisRawToFile(appData, dataGoldFile);
                }
            }
            break;
        case RXP_SEND_8801: // FORWARD TX DATA LENGTH REQUEST TO [USIF]
            if (!soUSIF_Data.full()) {
                printInfo(myRxpName, "Requesting Tx test mode to generate a datagram of length=%d and to send it to socket: \n",
                          uoe_txByteCnt.to_uint());
                printSockAddr(myRxpName, testSock);
                appData.setLE_TData(byteSwap32(testSock.addr), 31,  0);
                appData.setLE_TData(byteSwap16(testSock.port), 47, 32);
                appData.setLE_TData(byteSwap16(uoe_txByteCnt), 63, 48);
                appData.setLE_TKeep(0xFF);
                appData.setLE_TLast(TLAST);
                soUSIF_Data.write(appData);
                if (DEBUG_LEVEL & TRACE_UOE) {
                    printAxisRaw(myRxpName, "Sending Tx data length request to [USIF]: ", appData);
                }
                //-- Update the Meta-Gold File
                UdpAppMeta udpGldMeta(SockAddr(DEFAULT_FPGA_IP4_ADDR, uoe_rxMeta.dst.port),
                                      SockAddr(testSock.addr,         testSock.port));
                // Dump this socket pair to a gold file
                writeSocketPairToFile(udpGldMeta, metaGoldFile);
                //-- Update the Data-Gold File
                bool firstChunk=true;
                int txByteReq = uoe_txByteCnt;
                while (txByteReq) {
                    UdpAppData goldChunk(0,0,0);
                    if (firstChunk) {
                        for (int i=0; i<8; i++) {
                            if (txByteReq) {
                                unsigned char byte = (GEN_CHK0 >> ((7-i)*8)) & 0xFF;
                                goldChunk.setLE_TData(byte, (i*8)+7, (i*8)+0);
                                goldChunk.setLE_TKeep(1, i, i);
                                txByteReq--;
                            }
                        }
                        firstChunk = !firstChunk;
                    }
                    else {  // Second Chunk
                        for (int i=0; i<8; i++) {
                            if (txByteReq) {
                                unsigned char byte = (GEN_CHK1 >> ((7-i)*8)) & 0xFF;
                                goldChunk.setLE_TData(byte, (i*8)+7, (i*8)+0);
                                goldChunk.setLE_TKeep(1, i, i);
                                txByteReq--;
                            }
                        }
                        firstChunk = !firstChunk;
                    }
                    if (txByteReq == 0) {
                        goldChunk.setLE_TLast(TLAST);
                    }
                    writeAxisRawToFile(goldChunk, dataGoldFile);
                }
                uoe_rxpState = RXP_SEND_META;
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
    static UdpAppDLen uoe_appDLen;
    if (uoe_txpIsReady) {
        switch (uoe_txpState) {
        case TXP_WAIT_META:
            if (!siUSIF_Meta.empty() and !siUSIF_DLen.empty()) {
                UdpAppMeta  appMeta;
                UdpAppDLen  appDLen;
                siUSIF_Meta.read(appMeta);
                siUSIF_DLen.read(uoe_appDLen);
                if (DEBUG_LEVEL & TRACE_UOE) {
                    if (uoe_appDLen == 0) {
                        printInfo(myTxpName, "This UDP Tx datagram is forwarded in streaming mode (DLen=0).\n");
                    }
                    else {
                        printInfo(myTxpName, "Receiving %d bytes of data from [USIF].\n", uoe_appDLen.to_int());
                    }
                    printSockPair(myTxpName, appMeta);
                }
                writeSocketPairToFile(appMeta, metaFile);
                uoe_txpState = TXP_RECV_DATA;
            }
            break;
        case TXP_RECV_DATA:
            if (!siUSIF_Data.empty()) {
                UdpAppData     appData;
                siUSIF_Data.read(appData);
                writeAxisRawToFile(appData, dataFile);
                if (uoe_appDLen != 0) {
                    uoe_appDLen -= appData.getLen();
                }
                if (DEBUG_LEVEL & TRACE_UOE) {
                    printAxisRaw(myTxpName, "Received data chunk from [USIF] ", appData);
                }
                if (appData.getTLast()) {
                    uoe_txpState = TXP_WAIT_META;
                    if (uoe_appDLen != 0) {
                        printWarn(myTxpName, "TLAST is set but DLen != 0.\n");
                    }
                }
            }
            break;
        }
    }
}

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
    stream<UdpAppMetb>    ssUAF_USIF_Meta     ("ssUAF_USIF_Meta");
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
    stream<UdpAppMetb>    ssUSIF_UAF_Meta     ("ssUSIF_UAF_Meta");
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

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_udp_shell' STARTS HERE                                 ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    if (argc > 1) {
        printInfo(THIS_NAME, "This testbench will be executed with the following parameters: \n");
        for (int i=1; i<argc; i++) {
            printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
        }
    }

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
