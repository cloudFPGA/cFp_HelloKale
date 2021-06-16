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
 * @file       : simu_udp_shell_if_env.cpp
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
 *               |        (UOE)          +
 *               +-----------------------+
 *
 * \ingroup ROLE_USIF
 * \addtogroup ROLE_USIF_TEST
 * \{
 *****************************************************************************/

#include "simu_udp_shell_if_env.hpp"

using namespace hls;
using namespace std;

/*******************************************************************************
 * GLOBAL VARIABLES USED BY THE SIMULATION ENVIRONMENT
 *******************************************************************************/
extern unsigned int gSimCycCnt;
extern bool         gTraceEvent;
extern bool         gFatalError;
extern unsigned int gMaxSimCycles;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "SIM"

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
 * @brief Increase the simulation time of the testbench
 *
 * @param[in]  The number of cycles to increase.
 *******************************************************************************/
void increaseSimTime(unsigned int cycles) {
    gMaxSimCycles += cycles;
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
        stream<UdpAppMeta>  &siUSIF_Meta,
        //-- USIF / Tx Data Interface
        stream<UdpAppData>  &soUSIF_Data,
        stream<UdpAppMeta>  &soUSIF_Meta,
        stream<UdpAppDLen>  &soUSIF_DLen)
{

    const char *myRxName  = concat3(THIS_NAME, "/", "UAF-Rx");
    const char *myTxName  = concat3(THIS_NAME, "/", "UAF-Tx");

    static enum RxFsmStates { RX_IDLE=0, RX_STREAM } uaf_rxFsmState=RX_IDLE;

    UdpAppData      appData;
    UdpAppMeta      appMeta;

    switch (uaf_rxFsmState ) {
    case RX_IDLE:
        if (!siUSIF_Meta.empty() and !soUSIF_Meta.full()) {
            siUSIF_Meta.read(appMeta);
            // Swap IP_SA/IP_DA and update UPD_SP/UDP_DP
            soUSIF_Meta.write(UdpAppMeta(appMeta.ip4DstAddr, DEFAULT_FPGA_SND_PORT,
                                         appMeta.ip4SrcAddr, DEFAULT_HOST_LSN_PORT));
            soUSIF_DLen.write(0);
            uaf_rxFsmState  = RX_STREAM;
        }
        break;
    case RX_STREAM:
        if (!siUSIF_Data.empty() and !soUSIF_Data.full()) {
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
 * @brief Emulate the behavior of the SHELL & MMIO.
 *
 * @param[in]  piSHL_Ready    Ready Signal from [SHELL].
 * @param[out] poUSIF_Enable  Enable signal to [USIF] (.i.e, Enable Layer-7).
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
 * @brief Emulate behavior of the SHELL/NTS/UDP Offload Engine (UOE).
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

    static int  uoe_startupDelay = cUoeInitCycles;
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

/*! \} */
