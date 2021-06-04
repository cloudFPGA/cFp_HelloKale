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
 * @file       : udp_app_flash.cpp
 * @brief      : UDP Application Flash (UAF)
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp / ROLE
 * Language    : Vivado HLS
 *
 *------------------------------------------------------------------------------
 *
 * @details This application implements a set of UDP-oriented tests which are
 *  embedded into the role of the cFp_BringUp.
 *  The UAF connects to the SHELL via a UDP Shell Interface (USIF) block. The
 *  main purpose of the USIF is to provide a placeholder for the opening of one
 *  or multiple listening port(s). Its use is not a prerequisite, but it is
 *  provided here for sake of clarity and simplicity.
 *
 *          +-------+  +--------------------------------+
 *          |       |  |  +------+     +-------------+  |
 *          |       <-----+      <-----+             |  |
 *          | SHELL |  |  | USIF |     |     UAF     |  |
 *          |       +----->      +----->             |  |
 *          |       |  |  +------+     +-------------+  |
 *          +-------+  +--------------------------------+
 *
 * \ingroup ROLE
 * \addtogroup ROLE_UAF
 * \{
 *******************************************************************************/

#include "udp_app_flash.hpp"

using namespace hls;
using namespace std;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (RXP_TRACE | ESF_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "UAF"  // UdpApplicationFlash

#define TRACE_OFF  0x0000
#define TRACE_ESF 1 <<  1
#define TRACE_RXP 1 <<  2
#define TRACE_TXP 1 <<  3
#define TRACE_ALL  0xFFFF
#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * @brief Echo loopback between the Rx and Tx ports of the UDP connection.
 *
 * @param[in]  siRXp_Data   UDP datagram from RxPath (RXp).
 * @param[in]  siRXp_Meta   UDP metadata from [RXp].
 * @param[in]  siRXp_DLen   UDP data len from [RXp].
 * @param[out] soTXp_Data   UDP datagram to TxPath (TXp).
 * @param[out] soTXp_Meta   UDP metadata to [TXp].
 * @param[out] soTXp_DLen   UDP data len to [TXp].
 *
 * @details The echo is said to operate in "store-and-forward" mode because
 *   every received packet is stored into the DDR4 memory before being read
 *   again from the DDR4 and and sent back.
 *
 * [TODO - Implement this process as a real store-and-forward]
 *******************************************************************************/
void pUdpEchoStoreAndForward(
        stream<UdpAppData>  &siRXp_Data,
        stream<UdpAppMetb>  &siRXp_Meta,
        stream<UdpAppDLen>  &siRXp_DLen,
        stream<UdpAppData>  &soTXp_Data,
        stream<UdpAppMetb>  &soTXp_Meta,
        stream<UdpAppDLen>  &soTXp_DLen)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1  enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "ESf");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { ESF_META=0, ESF_STREAM } \
                               esf_fsmState = ESF_META;
    #pragma HLS reset variable=esf_fsmState
    static UdpAppDLen          esf_byteCnt;
    #pragma HLS reset variable=esf_byteCnt

    //=====================================================
    //== PROCESS DATA FORWARDING
    //=====================================================
    if ( !siRXp_Data.empty() and !soTXp_Data.full() ) {
        UdpAppData appData = siRXp_Data.read();
        soTXp_Data.write(appData);
        esf_byteCnt += appData.getLen();
        if (appData.getTLast()) {
            esf_byteCnt = 0;
        }
    }

    //=====================================================
    //== PROCESS META FORWARDING
    //=====================================================
    if ( !siRXp_Meta.empty() and !soTXp_Meta.full() and
         !siRXp_DLen.empty() and !soTXp_DLen.full() ) {
        UdpAppMetb appMeta = siRXp_Meta.read();
        UdpAppDLen appDLen = siRXp_DLen.read();
        // Swap IP_SA/IP_DA as well as UPD_SP/UDP_DP
        soTXp_Meta.write(UdpAppMetb(appMeta.ip4DstAddr, appMeta.udpDstPort,
                                    appMeta.ip4SrcAddr, appMeta.udpSrcPort));
        soTXp_DLen.write(appDLen);
    }
}    // End-of: pEchoStoreAndForward()

/*******************************************************************************
 * @brief Transmit Path - From THIS to USIF.
 *
 * @param[in]  piSHL_Mmio_EchoCtrl Configuration of the echo function.
 * @param[in]  siEPt_Data          Datagram from pEchoPassTrough (EPt).
 * @param[in]  siEPt_Meta          Metadata from [EPt].
 * @param[in]  siEPt_DLen          Data len from [EPt].
 * @param[in]  siESf_Data          Datagram from pEchoStoreAndForward (ESf).
 * @param[in]  siESf_Meta          Metadata from [ESf].
 * @param[in]  siESf_DLen          Data len from [ESf].
 * @param[out] soUSIF_Data         Datagram to UdpShellInterface (USIF).
 * @param[out] soUSIF_Meta         Metadata to [USIF].
 * @param[out] soUSIF_DLen         Data len to [USIF].
 *
 * @details The 'EchoPathThrough' forwards the datagrams in either DATAGRAM_MODE
 *   or STREAMING_MODE. The mode is defined by the content of the 'DLen' field:
 *   1) DATAGRAM_MODE: If the 'DLen' field is loaded with a length != 0, this
 *     length is used as reference for handling the incoming datagram. If the
 *     length is larger than 1472 bytes (.i.e, MTU-IP_HEADER_LEN-UDP_HEADER_LEN),
 *     the UDP Offload Engine (UOE) of the NTS is expected to split the incoming
 *     datagram and generate as many sub-datagrams as required to transport all
 *     'DLen' bytes over Ethernet.
 *     frames.
 *  2) STREAMING_MODE: If the 'DLen' field is configured with a length==0, the
 *     corresponding stream is expected to be forwarded by the UOE of the NTS
 *     based on the same metadata information until the 'TLAST' bit of the data
 *     stream is set. In this mode, the UOE will wait for the reception of 1472
 *     bytes before generating a new UDP-over-IPv4 packet, unless the 'TLAST'
 *     bit of the data stream is set.
 *******************************************************************************/
void pUdpTxPath(
        //[NOT_USED} ap_uint<2> piSHL_Mmio_EchoCtrl,
        stream<UdpAppData>  &siEPt_Data,
        stream<UdpAppMetb>  &siEPt_Meta,
        stream<UdpAppDLen>  &siEPt_DLen,
        stream<UdpAppData>  &siESf_Data,
        stream<UdpAppMetb>  &siESf_Meta,
        stream<UdpAppDLen>  &siESf_DLen,
        stream<UdpAppData>  &soUSIF_Data,
        stream<UdpAppMetb>  &soUSIF_Meta,
        stream<UdpAppDLen>  &soUSIF_DLen)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "TXp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { TXP_IDLE=0, TXP_DATA_EPT, TXP_DATA_ESF} \
                               txp_fsmState = TXP_IDLE;
    #pragma HLS reset variable=txp_fsmState
    static enum DgmMode   { STRM_MODE=0, DGRM_MODE } \
                               txp_fwdMode = DGRM_MODE;
    #pragma HLS reset variable=txp_fwdMode

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static ap_int<17>  txp_lenCnt;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    UdpAppMetb    appMeta;
    UdpAppDLen    appDLen;
    UdpAppData    appData;

    switch (txp_fsmState) {
    case TXP_IDLE:
        if (!siEPt_Meta.empty() and !soUSIF_Meta.full() and
            !siEPt_DLen.empty() and !soUSIF_DLen.full()) {
            appMeta = siEPt_Meta.read();
            appDLen = siEPt_DLen.read();
            // Swap IP_SA/IP_DA as well as UPD_SP/UDP_DP
            soUSIF_Meta.write(UdpAppMetb(appMeta.ip4DstAddr, appMeta.udpDstPort,
                                         appMeta.ip4SrcAddr, appMeta.udpSrcPort));
            soUSIF_DLen.write(appDLen);
            txp_fsmState = TXP_DATA_EPT;
        }
        else if (!siESf_Meta.empty() and !soUSIF_Meta.full() and
                 !siESf_DLen.empty() and !soUSIF_DLen.full()) {
            appMeta = siESf_Meta.read();
            appDLen = siESf_DLen.read();
            soUSIF_Meta.write(appMeta);
            soUSIF_DLen.write(appDLen);
            txp_fsmState = TXP_DATA_ESF;
        }
        if (appDLen == 0) {
            txp_fwdMode = STRM_MODE;
            txp_lenCnt = 0;
        }
        else {
            txp_fwdMode = DGRM_MODE;
            txp_lenCnt = appDLen;
        }
        break;
    case TXP_DATA_EPT:
        if (!siEPt_Data.empty() and !soUSIF_Data.full()) {
            appData = siEPt_Data.read();
            if (txp_fwdMode == STRM_MODE) {
                txp_lenCnt = txp_lenCnt + appData.getLen();  // Just for tracing
                if (appData.getTLast()) {
                    txp_fsmState = TXP_IDLE;
                    if (DEBUG_LEVEL & TRACE_TXP) {
                        printInfo(myName, "ECHO_PATH_THRU + STREAM   MODE - Finished forwarding %d bytes.\n", txp_lenCnt.to_uint());
                    }
                }
            }
            else {
                txp_lenCnt -= appData.getLen();
                if ((txp_lenCnt <= 0) or (appData.getTLast())) {
                    appData.setTLast(TLAST);
                    txp_fsmState = TXP_IDLE;
                    if (DEBUG_LEVEL & TRACE_TXP) {
                        printInfo(myName, "ECHO_PATH_THRU + DATAGRAM MODE - Finished datagram forwarding.\n");
                    }
                }
                else {
                    appData.setTLast(0);
                }
            }
            soUSIF_Data.write(appData);
        }
        break;
    case TXP_DATA_ESF:
        if (!siESf_Data.empty() and !soUSIF_Data.full()) {
            appData = siESf_Data.read();
            if (txp_fwdMode == STRM_MODE) {
                txp_lenCnt = txp_lenCnt + appData.getLen();  // Just for tracing
                if (appData.getTLast()) {
                    txp_fsmState = TXP_IDLE;
                    if (DEBUG_LEVEL & TRACE_TXP) {
                        printInfo(myName, "ECHO_STORE_FWD + STREAM   MODE - Finished forwarding %d bytes.\n", txp_lenCnt.to_uint());
                    }
                }
            }
            else {
                txp_lenCnt -= appData.getLen();
                if ((txp_lenCnt <= 0) or (appData.getTLast())) {
                    appData.setTLast(TLAST);
                    txp_fsmState = TXP_IDLE;
                    if (DEBUG_LEVEL & TRACE_TXP) {
                        printInfo(myName, "ECHO_STORE_FWD + DATAGRAM MODE - Finished datagram forwarding.\n");
                    }
                }
                else {
                    appData.setTLast(0);
                }
            }
            soUSIF_Data.write(appData);
        }
        break;
    }  // End-of: switch (txp_fsmState) {

    /*** UNUSED_VERSION_BASED_ON_MMIO_ECHO_CTRL_BITS *****************
    switch (txp_fsmState) {
    case TXP_META:
        switch(piSHL_Mmio_EchoCtrl) {
        case ECHO_PATH_THRU:
            if (!siEPt_Meta.empty() and !soUSIF_Meta.full() and
                !siEPt_DLen.empty() and !soUSIF_DLen.full()) {
                UdpAppMetb appMeta = siEPt_Meta.read();
                UdpAppDLen appDLen = siEPt_DLen.read();
                // Swap IP_SA/IP_DA as well as UPD_SP/UDP_DP
                soUSIF_Meta.write(SocketPair(
                            SockAddr(appMeta.dst.addr, appMeta.dst.port),
                            SockAddr(appMeta.src.addr, appMeta.src.port)));
                soUSIF_DLen.write(appDLen);
                if (appDLen == 0) {
                    txp_dgmMode = STRM_MODE;
                }
                else {
                    txp_dgmMode = DGRM_MODE;
                }
                txp_lenCnt = 0;
                txp_lenCnt = appDLen.to_int();
                txp_fsmState = TXP_STREAM;
            }
            break;
        case ECHO_STORE_FWD:
            if (!siESf_Meta.empty() and !soUSIF_Meta.full() and
                !siESf_DLen.empty() and !soUSIF_DLen.full()) {
                UdpAppMetb appMeta = siESf_Meta.read();
                UdpAppDLen appDLen = siESf_DLen.read();
                soUSIF_Meta.write(appMeta);
                soUSIF_DLen.write(appDLen);
                if (appDLen == 0) {
                    txp_dgmMode = STRM_MODE;
                }
                else {
                    txp_dgmMode = DGRM_MODE;
                    txp_lenCnt  = appDLen;
                }
                txp_fsmState = TXP_STREAM;
            }
            break;
        case ECHO_OFF:
            // Always drain and drop the metadata and datagram length (if any)
            if ( !siEPt_Meta.empty() ) { siEPt_Meta.read(); }
            if ( !siEPt_DLen.empty() ) { siEPt_DLen.read(); }
            if ( !siESf_Meta.empty() ) { siESf_Meta.read(); }
            if ( !siESf_DLen.empty() ) { siESf_DLen.read(); }
            txp_fsmState = TXP_STREAM;
            break;
        default:
            // Reserved configuration ==> Do nothing
            break;
        }  // End-of: switch(piSHL_Mmio_EchoCtrl) {
        break;
    case TXP_STREAM:
        switch(piSHL_Mmio_EchoCtrl) {
        case ECHO_PATH_THRU:
            if (!siEPt_Data.empty() and !soUSIF_Data.full()) {
                UdpAppData appData = siEPt_Data.read();
                if (txp_dgmMode == STRM_MODE) {
                    soUSIF_Data.write(appData);
                    txp_lenCnt = txp_lenCnt + appData.getLen();
                    if (appData.getTLast()) {
                        txp_fsmState = TXP_META;
                        if (DEBUG_LEVEL & TRACE_TXP) {
                            printInfo(myName, "ECHO_PATH_THRU - Finished forwarding a datagram of len=%d.\n", txp_lenCnt.to_uint());
                        }
                    }
                }
                else {
                    txp_lenCnt -= appData.getLen();
                    if (txp_lenCnt <= 0) {
                        appData.setTLast(TLAST);
                        txp_fsmState = TXP_META;
                    }
                    else {
                        appData.setTLast(0);
                    }
                    soUSIF_Data.write(appData);
                }
            }
            break;
        case ECHO_STORE_FWD:
            if (!siESf_Data.empty() and !soUSIF_Data.full()) {
                UdpAppData appData = siESf_Data.read();
                if (txp_dgmMode == STRM_MODE) {
                    soUSIF_Data.write(appData);
                    if (appData.getTLast()) {
                        txp_fsmState = TXP_META;
                    }
                }
                else {
                    txp_lenCnt -= appData.getLen();
                    if (txp_lenCnt <= 0) {
                        appData.setTLast(TLAST);
                        txp_fsmState = TXP_META;
                    }
                    else {
                        appData.setTLast(0);
                    }
                    soUSIF_Data.write(appData);
                }
            }
            break;
        case ECHO_OFF:
            // Always drain and drop the datagrams (if any)
            if ( !siEPt_Data.empty() ) { siEPt_Data.read(); }
            if ( !siESf_Data.empty() ) { siESf_Data.read(); }
            txp_fsmState = TXP_META;
            break;
        default:
            // Reserved configuration ==> Do nothing
            break;
        }  // End-of: switch(piSHL_Mmio_EchoCtrl) {
        break;
    }  // End-of: switch (txp_fsmState) {
    ******************************************************************/

}  // End-of: pTxPath()

/*******************************************************************************
 * @brief UDP Receive Path (RXp) - From SHELL->ROLE/USIF to THIS.
 *
 * @param[in]  piSHL_Mmio_EchoCtrl Configuration of the echo function.
 * @param[in]  siUSIF_Data         Datagram from UdpShellInterface (USIF).
 * @param[in]  siUSIF_Meta         Metadata from [USIF].
 * @param[out] soEPt_Data          Datagram to EchoPassTrough (EPt).
 * @param[out] soEPt_Meta          Metadata to [EPt].
 * @param[out] soEPt_DLen          Data len to [EPt].
 * @param[out] soESf_Data          Datagram to EchoStoreAndForward (ESf).
 * @param[out] soESf_Meta          Metadata to [ESf].
 * @param[out] soESf_DLen          Metadata to [ESf].
 *
 * @details This Process waits for a new datagram to read and forwards it to the
 *   EchoPathThrough (EPt) or EchoStoreAndForward (ESf) process upon the setting
 *   of the UDP destination port.
 *   (FYI-This function used to be performed by the 'piSHL_Mmio_EchoCtrl' bits).
 *******************************************************************************/
void pUdpRxPath(
        //[NOT_USED] ap_uint<2>  piSHL_Mmio_EchoCtrl,
        stream<UdpAppData>   &siUSIF_Data,
        stream<UdpAppMetb>   &siUSIF_Meta,
        stream<UdpAppData>   &soEPt_Data,
        stream<UdpAppMetb>   &soEPt_Meta,
        stream<UdpAppDLen>   &soEPt_DLen,
        stream<UdpAppData>   &soESf_Data,
        stream<UdpAppMetb>   &soESf_Meta,
        stream<UdpAppDLen>   &soESf_DLen)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "RXp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { RXP_IDLE=0, RXP_META_EPT, RXP_META_ESF,
                                        RXP_DATA_EPT, RXP_DATA_ESF,
                                        RXP_DLEN_EPT, RXP_DLEN_ESF} \
                               rxp_fsmState = RXP_IDLE;
    #pragma HLS reset variable=rxp_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static UdpAppMetb rxp_appMeta;
    static UdpAppDLen rxp_byteCnt;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    UdpAppData  appData;

    switch (rxp_fsmState) {
    case RXP_IDLE:
        if (!siUSIF_Meta.empty()) {
            siUSIF_Meta.read(rxp_appMeta);
            rxp_byteCnt = 0;
            switch (rxp_appMeta.udpDstPort) {
            case ECHO_PATH_THRU_PORT:
                // (DstPort == 8803) Echo this traffic in path-through mode
                if (DEBUG_LEVEL & TRACE_RXP) { printInfo(myName, "Entering Rx path-through mode (DstPort=%4.4d)\n", rxp_appMeta.udpDstPort.to_uint()); }
                rxp_fsmState  = RXP_META_EPT;
                break;
            default:
                // (DstPort != 8803) Echo this traffic in store-and-forward mode
                if (DEBUG_LEVEL & TRACE_RXP) { printInfo(myName, "Entering Rx store-and-forward mode (DstPort=%4.4d)\n", rxp_appMeta.udpDstPort.to_uint()); }
                rxp_fsmState  = RXP_META_ESF;
                break;
            }
        }
        break;
    case RXP_META_EPT:
        if (!soEPt_Meta.full()) {
            //-- Forward incoming metadata to pEchoPathThrough
            soEPt_Meta.write(rxp_appMeta);
            rxp_fsmState = RXP_DATA_EPT;
        }
        break;
    case RXP_DATA_EPT:
        if (!siUSIF_Data.empty() and !soEPt_Data.full()) {
            //-- Read incoming data and forward to pEchoPathThrough
            siUSIF_Data.read(appData);
            soEPt_Data.write(appData);
            rxp_byteCnt += appData.getLen();
            if (appData.getTLast()) {
                rxp_fsmState = RXP_DLEN_EPT;
            }
        }
        break;
    case RXP_DLEN_EPT:
        if (!soEPt_DLen.full()) {
            //-- Forward the computed data length to pEchoPathThrough
            soEPt_DLen.write(rxp_byteCnt);
            rxp_fsmState = RXP_IDLE;
        }
        break;
    case RXP_META_ESF:
        if (!soESf_Meta.full()) {
            //-- Forward incoming metadata to pEchoStoreAndForward
            soESf_Meta.write(rxp_appMeta);
            rxp_fsmState = RXP_DATA_ESF;
        }
        break;
    case RXP_DATA_ESF:
        if (!siUSIF_Data.empty() and !soESf_Data.full()) {
            //-- Read incoming data and forward to pEchoStoreAndForward
            UdpAppData appData;
            siUSIF_Data.read(appData);
            soESf_Data.write(appData);
            rxp_byteCnt += appData.getLen();
            if (appData.getTLast()) {
                rxp_fsmState = RXP_DLEN_ESF;
            }
        }
        break;
    case RXP_DLEN_ESF:
        if (!soESf_DLen.full()) {
            //-- Forward the computed data length to pEchoPathThrough
            soESf_DLen.write(rxp_byteCnt);
            rxp_fsmState = RXP_IDLE;
        }
        break;
    }  // End-of: switch (rxp_fsmState) {

    /*** UNUSED_VERSION_BASED_ON_MMIO_ECHO_CTRL_BITS *****************
    switch (rxp_fsmState) {
    case RXP_META:
        rxp_byteCnt = 0;
        switch(piSHL_Mmio_EchoCtrl) {
        case ECHO_PATH_THRU:
            //-- Read incoming metadata and forward to pEchoPathThrough
            if (!siUSIF_Meta.empty() and !soEPt_Meta.full() and !soEPt_DLen.full()) {
                soEPt_Meta.write(siUSIF_Meta.read());
                soEPt_DLen.write(0);  // Tells [TXp] to operate in STREAMING-MODE
                rxp_fsmState = RXP_STREAM;
            }
            break;
        case ECHO_STORE_FWD:
            //-- Read incoming metadata and forward to pEchoStoreAndForward
            if ( !siUSIF_Meta.empty() and !soESf_Meta.full()) {
               soESf_Meta.write(siUSIF_Meta.read());
               rxp_fsmState = RXP_STREAM;
           }
           break;
        case ECHO_OFF:
        default:
            // Drain and drop the metadata
            if ( !siUSIF_Meta.empty() ) {
                siUSIF_Meta.read();
            }
            rxp_fsmState = RXP_STREAM;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
    case RXP_STREAM:
        switch(piSHL_Mmio_EchoCtrl) {
        case ECHO_PATH_THRU:
            //-- Read incoming data and forward to pEchoPathThrough
            if (!siUSIF_Data.empty() and !soEPt_Data.full()) {
                UdpAppData appData = siUSIF_Data.read();
                soEPt_Data.write(appData);
                rxp_byteCnt += appData.getLen();
                if (appData.getTLast()) {
                    rxp_fsmState = RXP_META;
                }
            }
            break;
        case ECHO_STORE_FWD:
            //-- Read incoming data and forward to pEchoStoreAndForward
            if ( !siUSIF_Data.empty() and !soESf_Data.full()) {
                UdpAppData appData = siUSIF_Data.read();
                soESf_Data.write(appData);
                if (appData.getTLast()) {
                    rxp_fsmState = RXP_META;
                }
            }
            break;
        case ECHO_OFF:
        default:
            // Drain and drop the datagram
            if ( !siUSIF_Data.empty() ) {
                UdpAppData appData = siUSIF_Data.read();
            }
            rxp_fsmState = RXP_META;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
    }  // End-of: switch (rxp_fsmState) {
    ******************************************************************/

}  // End-of: pRxPath()

/*******************************************************************************
 * @brief   Main process of the UDP Application Flash (UAF)
 *
 * @param[in]  piSHL_Mmio_EchoCtrl  Configures the echo function.
 * @param[in]  piSHL_Mmio_PostPktEn Enables posting of UDP packets.
 * @param[in]  piSHL_Mmio_CaptPktEn Enables capture of UDP packets.
 * @param[in]  siUSIF_Data          UDP datagram from UdpShellInterface (USIF).
 * @param[in]  siUSIF_Meta          UDP metadata from [USIF].
 * @param[out] soUSIF_Data          UDP datagram to [USIF].
 * @param[out] soUSIF_Meta          UDP metadata to [USIF].
 * @param[out] soUSIF_DLen          UDP data len to [USIF].
 *
 * @info This core is designed with non-blocking read and write streams in mind.
 *   FYI, this is the normal way of operation for an internal stream and for an
 *   interface using the 'ap_fifo' protocol.
 *
 * @warning This core will not work properly if operated with a handshake
 *   interface(ap_hs) or an AXI-Stream interface (axis) because these two
 *   interfaces do not support non-blocking accesses.
 *******************************************************************************/
void udp_app_flash (

        //------------------------------------------------------
        //-- SHELL / Mmio / Configuration Interfaces
        //------------------------------------------------------
        //[NOT_USED] ap_uint<2>  piSHL_Mmio_EchoCtrl,
        //[NOT_USED] ap_uint<1>  piSHL_Mmio_PostPktEn,
        //[NOT_USED] ap_uint<1>  piSHL_Mmio_CaptPktEn,

        //------------------------------------------------------
        //-- USIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &siUSIF_Data,
        stream<UdpAppMetb>  &siUSIF_Meta,

        //------------------------------------------------------
        //-- USIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &soUSIF_Data,
        stream<UdpAppMetb>  &soUSIF_Meta,
        stream<UdpAppDLen>  &soUSIF_DLen)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INLINE

    //--------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    //-- Rx Path (RXp) ---------------------------------------------------------
    static stream<UdpAppData>     ssRXpToTXp_Data    ("ssRXpToTXp_Data");
    #pragma HLS STREAM   variable=ssRXpToTXp_Data    depth=2048
    static stream<UdpAppMetb>     ssRXpToTXp_Meta    ("ssRXpToTXp_Meta");
    #pragma HLS STREAM   variable=ssRXpToTXp_Meta    depth=64
    static stream<UdpAppDLen>     ssRXpToTXp_DLen    ("ssRXpToTXp_DLen");
    #pragma HLS STREAM   variable=ssRXpToTXp_DLen    depth=64

    static stream<UdpAppData>     ssRXpToESf_Data    ("ssRXpToESf_Data");
    #pragma HLS STREAM   variable=ssRXpToESf_Data    depth=1024
    static stream<UdpAppMetb>     ssRXpToESf_Meta    ("ssRXpToESf_Meta");
    #pragma HLS STREAM   variable=ssRXpToESf_Meta    depth=32
    static stream<UdpAppDLen>     ssRXpToESf_DLen    ("ssRXpToESf_DLen");
    #pragma HLS STREAM   variable=ssRXpToESf_DLen    depth=32

    //-- Echo Store and Forward (ESf) ------------------------------------------
    static stream<UdpAppData>     ssESfToTXp_Data    ("ssESfToTXp_Data");
    #pragma HLS STREAM   variable=ssESfToTXp_Data    depth=1024
    static stream<UdpAppMetb>     ssESfToTXp_Meta    ("ssESfToTXp_Meta");
    #pragma HLS STREAM   variable=ssESfToTXp_Meta    depth=32
    static stream<UdpAppDLen>     ssESfToTXp_DLen    ("ssESfToTXp_DLen");
    #pragma HLS STREAM   variable=ssESfToTXp_DLen    depth=32

    //-- PROCESS FUNCTIONS ----------------------------------------------------
    //
    //
    //                     +----------+
    //           +-------->|   pESf   |----------+
    //           |         +----------+          |
    //           |          --------+            |
    //           |  +--------> sEPt |---------+  |
    //           |  |       --------+         |  |
    //     +--+--+--+--+                   +--+--+--+--+
    //     |   pRXp    |                   |   pTXp    |
    //     +------+----+                   +-----+-----+
    //          /|\                              |
    //           |                               |
    //           |                               |
    //           |                              \|/
    //
    //-------------------------------------------------------------------------
    pUdpRxPath(
            //[NOT_USED] piSHL_Mmio_EchoCtrl,
            siUSIF_Data,
            siUSIF_Meta,
            ssRXpToTXp_Data,
            ssRXpToTXp_Meta,
            ssRXpToTXp_DLen,
            ssRXpToESf_Data,
            ssRXpToESf_Meta,
            ssRXpToESf_DLen);

    pUdpEchoStoreAndForward(
            ssRXpToESf_Data,
            ssRXpToESf_Meta,
            ssRXpToESf_DLen,
            ssESfToTXp_Data,
            ssESfToTXp_Meta,
            ssESfToTXp_DLen);

    pUdpTxPath(
            //[NOT_USED] piSHL_Mmio_EchoCtrl,
            ssRXpToTXp_Data,
            ssRXpToTXp_Meta,
            ssRXpToTXp_DLen,
            ssESfToTXp_Data,
            ssESfToTXp_Meta,
            ssESfToTXp_DLen,
            soUSIF_Data,
            soUSIF_Meta,
            soUSIF_DLen);

}

/*! \} */
