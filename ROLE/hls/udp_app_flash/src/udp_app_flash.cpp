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
 * Component   : cFp_BringUp/ROLE
 * Language    : Vivado HLS
 *
 *------------------------------------------------------------------------------
 *
 * @details This application implements a set of UDP-oriented tests which are
 *  embedded into the role of the cFp_BringUp.
 *  The UAF connects to the SHELL via a UDP Shell Interface (USIF) block. The
 *  main purpose of the USIF is to provide a placeholder for the opening of one
 *  or multiple listening port(s). Its use a prerequisite, but it is provided
 *  here for sake of clarity and simplicity.
 *
 *          +-------+  +--------------------------------+
 *          |       |  |  +------+     +-------------+  |
 *          |       <-----+      <-----+             |  |
 *          | SHELL |  |  | USIF |     |     UAF     |  |
 *          |       +----->      +----->             |  |
 *          |       |  |  +------+     +-------------+  |
 *          +-------+  +--------------------------------+
 *
 * \ingroup ROL_UAF
 * \addtogroup ROL_UAF
 * \{
 *******************************************************************************/

#include "udp_app_flash.hpp"

using namespace hls;
using namespace std;

/************************************************
 * INTERFACE SYNTHESIS DIRECTIVES
 *  For the time being, we may continue to design
 *  with the DEPRECATED directives because the
 *  new PRAGMAs do not always work for us.
 ************************************************/
#define USE_DEPRECATED_DIRECTIVES

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

#define DEBUG_LEVEL (TRACE_ALL)


/*******************************************************************************
 * @brief Echo loopback between the Rx and Tx ports of the UDP connection.
 *
 * @param[in]  siRXp_Data   UDP datagram from RxPath (RXp).
 * @param[in]  siRXp_Meta   UDP metadata from [RXp].
 * @param[out] soTXp_Data   UDP datagram to TxPath (TXp).
 * @param[out] soTXp_Meta   UDP metadata to [TXp].
 * @param[out] soTXp_DLen   UDP data len to [TXp].
 *
 * @details The echo is said to operate in "store-and-forward" mode because
 *   every received packet is stored into the DDR4 memory before being read
 *   again from the DDR4 and and sent back.
 *******************************************************************************/
void pEchoStoreAndForward( // [TODO - Implement this process as a real store-and-forward]
        stream<UdpAppData>  &siRXp_Data,
        stream<UdpAppMeta>  &siRXp_Meta,
        stream<UdpAppData>  &soTXp_Data,
        stream<UdpAppMeta>  &soTXp_Meta,
        stream<UdpAppDLen>  &soTXp_DLen)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    //-- LOCAL STREAMS ---------------------------------------------------------
    static stream<UdpAppData>   ssDataFifo ("ssDataFifo");
    #pragma HLS stream variable=ssDataFifo depth=2048
    static stream<UdpAppMeta>   ssMetaFifo ("ssMetaFifo");
    #pragma HLS stream variable=ssMetaFifo depth=64
    static stream<UdpAppDLen>   ssDLenFifo ("ssDLenFifo");
    #pragma HLS stream variable=ssDLenFifo depth=64

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { ESF_META=0, ESF_STREAM } \
                               esf_fsmState = ESF_META;
    #pragma HLS reset variable=esf_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static UdpAppDLen          esf_byteCnt;
    #pragma HLS reset variable=esf_byteCnt

    //-- Always: DataFiFo Push
    if ( !siRXp_Data.empty() && !ssDataFifo.full() ) {
        UdpAppData appData = siRXp_Data.read();
        ssDataFifo.write(appData);
        esf_byteCnt += appData.getLen();
        if (appData.getTLast()) {
            ssDLenFifo.write(esf_byteCnt);
            esf_byteCnt = 0;
        }
    }
    //-- Always: MetaFiFo Push
    if ( !siRXp_Meta.empty() && !ssMetaFifo.full() ) {
        UdpAppMeta appMeta = siRXp_Meta.read();
        // Swap IP_SA/IP_DA and re-use the same UPD_SP/UDP_DP
        ssMetaFifo.write(SocketPair(
                    SockAddr(appMeta.dst.addr, appMeta.src.port),
                    SockAddr(appMeta.src.addr, appMeta.dst.port)));
    }

    //-- Always: DataFiFo Pop
    if ( !ssDataFifo.empty() && !soTXp_Data.full() ) {
        soTXp_Data.write(ssDataFifo.read());
    }
    //-- Always: MetaFiFo Pop
    if ( !ssMetaFifo.empty() && !soTXp_Meta.full() ) {
        soTXp_Meta.write(ssMetaFifo.read());
    }
    //-- Always: DLenFiFo Pop
    if ( !ssDLenFifo.empty() && !soTXp_DLen.full() ) {
        soTXp_DLen.write(ssDLenFifo.read());
    }
}    // End-of: pEchoStoreAndForward()

/*****************************************************************************
 * @brief Transmit Path - From THIS to SHELL.
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
 *****************************************************************************/
void pTxPath(
        ap_uint<2>           piSHL_Mmio_EchoCtrl,
        stream<UdpAppData>  &siEPt_Data,
        stream<UdpAppMeta>  &siEPt_Meta,
        stream<UdpAppDLen>  &siEPt_DLen,
        stream<UdpAppData>  &siESf_Data,
        stream<UdpAppMeta>  &siESf_Meta,
        stream<UdpAppDLen>  &siESf_DLen,
        stream<UdpAppData>  &soUSIF_Data,
        stream<UdpAppMeta>  &soUSIF_Meta,
        stream<UdpAppDLen>  &soUSIF_DLen)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "TXp");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { TXP_META=0, TXP_STREAM } \
                               txp_fsmState = TXP_META;
    #pragma HLS reset variable=txp_fsmState
    static enum DgmMode   { DGRM_MODE=0, STRM_MODE } \
                               txp_dgmMode = DGRM_MODE;
    #pragma HLS reset variable=txp_dgmMode

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_int<17>  txp_lenCnt;

    switch (txp_fsmState) {
    case TXP_META:
        switch(piSHL_Mmio_EchoCtrl) {
        case ECHO_PATH_THRU:
            if (!siEPt_Meta.empty() and !soUSIF_Meta.full() and
                !siEPt_DLen.empty() and !soUSIF_DLen.full()) {
                UdpAppMeta appMeta = siEPt_Meta.read();
                UdpAppDLen appDLen = siEPt_DLen.read();
                // Swap IP_SA/IP_DA and re-use the same UPD_SP/UDP_DP
                soUSIF_Meta.write(SocketPair(
                            SockAddr(appMeta.dst.addr, appMeta.src.port),
                            SockAddr(appMeta.src.addr, appMeta.dst.port)));
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
                UdpAppMeta appMeta = siESf_Meta.read();
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
 *
 * @details This Process waits for a new datagram to read and forwards it to the
 *   EchoPathThrough (EPt) or EchoStoreAndForward (ESf) process upon the setting
 *   of the 'piSHL_Mmio_EchoCtrl' bits.
 *
 * @return Nothing.
 *******************************************************************************/
void pRxPath(
        ap_uint<2>            piSHL_Mmio_EchoCtrl,
        stream<UdpAppData>   &siUSIF_Data,
        stream<UdpAppMeta>   &siUSIF_Meta,
        stream<UdpAppData>   &soEPt_Data,
        stream<UdpAppMeta>   &soEPt_Meta,
        stream<UdpAppDLen>   &soEPt_DLen,
        stream<UdpAppData>   &soESf_Data,
        stream<UdpAppMeta>   &soESf_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "RXp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { RXP_META=0, RXP_STREAM } \
                               rxp_fsmState = RXP_META;
    #pragma HLS reset variable=rxp_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static UdpAppDLen rxp_byteCnt;  // Just for debug

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
 *******************************************************************************/
void udp_app_flash (

        //------------------------------------------------------
        //-- SHELL / Mmio / Configuration Interfaces
        //------------------------------------------------------
        ap_uint<2>           piSHL_Mmio_EchoCtrl,
        //[TODO] ap_uint<1>  piSHL_Mmio_PostPktEn,
        //[TODO] ap_uint<1>  piSHL_Mmio_CaptPktEn,

        //------------------------------------------------------
        //-- USIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &siUSIF_Data,
        stream<UdpAppMeta>  &siUSIF_Meta,

        //------------------------------------------------------
        //-- USIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &soUSIF_Data,
        stream<UdpAppMeta>  &soUSIF_Meta,
        stream<UdpAppDLen>  &soUSIF_DLen)
{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

#if defined(USE_DEPRECATED_DIRECTIVES)

    #pragma HLS INTERFACE ap_stable          port=piSHL_Mmio_EchoCtrl
    //[TODO] #pragma HLS INTERFACE ap_stable port=piSHL_Mmio_PostPktEn
    //[TODO] #pragma HLS INTERFACE ap_stable port=piSHL_Mmio_CaptPktEn

    #pragma HLS resource core=AXI4Stream variable=siUSIF_Data    metadata="-bus_bundle siUSIF_Data"
    #pragma HLS resource core=AXI4Stream variable=siUSIF_Meta    metadata="-bus_bundle siUSIF_Meta"
    #pragma HLS DATA_PACK                variable=siUSIF_Meta

    #pragma HLS resource core=AXI4Stream variable=soUSIF_Data    metadata="-bus_bundle soUSIF_Data"
    #pragma HLS resource core=AXI4Stream variable=soUSIF_Meta    metadata="-bus_bundle soUSIF_Meta"
    #pragma HLS DATA_PACK                variable=soUSIF_Meta
    #pragma HLS resource core=AXI4Stream variable=soUSIF_DLen    metadata="-bus_bundle soUSIF_DLen"

#else

    #pragma HLS INTERFACE ap_stable register port=piSHL_Mmio_EchoCtrl  name=piSHL_Mmio_EchoCtrl
    //[TODO] #pragma HLS INTERFACE ap_stable port=piSHL_Mmio_PostPktEn
    //[TODO] #pragma HLS INTERFACE ap_stable port=piSHL_Mmio_CaptPktEn

    #pragma HLS INTERFACE axis register both port=siUSIF_Data  name=siUSIF_Data
    #pragma HLS INTERFACE axis register both port=siUSIF_Meta  name=siUSIF_Meta
    #pragma HLS DATA_PACK                variable=siUSIF_Meta

    #pragma HLS INTERFACE axis register both port=soUSIF_Data  name=soUSIF_Data
    #pragma HLS INTERFACE axis register both port=soUSIF_Meta  name=soUSIF_Meta
    #pragma HLS DATA_PACK                variable=soUSIF_Meta
    #pragma HLS INTERFACE axis register both port=soUSIF_DLen  name=soUSIF_DLen

#endif

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW

    //--------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    //-- Rx Path (RXp) ---------------------------------------------------------
    static stream<UdpAppData>     ssRXpToTXp_Data    ("ssRXpToTXp_Data");
    #pragma HLS STREAM   variable=ssRXpToTXp_Data    depth=2048
    static stream<UdpAppMeta>     ssRXpToTXp_Meta    ("ssRXpToTXp_Meta");
    #pragma HLS STREAM   variable=ssRXpToTXp_Meta    depth=64
    static stream<UdpAppDLen>     ssRXpToTXp_DLen    ("ssRXpToTXp_DLen");
    #pragma HLS STREAM   variable=ssRXpToTXp_DLen    depth=64

    static stream<UdpAppData>     ssRXpToESf_Data    ("ssRXpToESf_Data");
    #pragma HLS STREAM   variable=ssRXpToESf_Data    depth=16
    static stream<UdpAppMeta>     ssRXpToESf_Meta    ("ssRXpToESf_Meta");
    #pragma HLS STREAM   variable=ssRXpToESf_Meta    depth=2

    //-- Echo Store and Forward (ESf) ------------------------------------------
    static stream<UdpAppData>     ssESfToTXp_Data    ("ssESfToTXp_Data");
    #pragma HLS STREAM   variable=ssESfToTXp_Data    depth=16
    static stream<UdpAppMeta>     ssESfToTXp_Meta    ("ssESfToTXp_Meta");
    #pragma HLS STREAM   variable=ssESfToTXp_Meta    depth=2
    static stream<UdpAppDLen>     ssESfToTXp_DLen    ("ssESfToTXp_DLen");
    #pragma HLS STREAM   variable=ssESfToTXp_DLen    depth=2

    //-- PROCESS FUNCTIONS ----------------------------------------------------
    //
    //       +-+                                   | |
    //       |S|    1      +----------+            |S|
    //       |i| +-------->|   pESf   |----------+ |r|
    //       |n| |         +----------+          | |c|
    //       |k| |    0     --------+            | +++
    //       /|\ |  +--------> sEPt |---------+  |  |
    //       2|  |  |       --------+         |  | \|/
    //     +--+--+--+--+                   +--+--+--+--+
    //     |   pRXp    |                   |   pTXp    |
    //     +------+----+                   +-----+-----+
    //          /|\                              |
    //           |                               |
    //           |                               |
    //           |                              \|/
    //
    //-------------------------------------------------------------------------
    pRxPath(
            piSHL_Mmio_EchoCtrl,
            siUSIF_Data,
            siUSIF_Meta,
            ssRXpToTXp_Data,
            ssRXpToTXp_Meta,
            ssRXpToTXp_DLen,
            ssRXpToESf_Data,
            ssRXpToESf_Meta);

    pEchoStoreAndForward(
            ssRXpToESf_Data,
            ssRXpToESf_Meta,
            ssESfToTXp_Data,
            ssESfToTXp_Meta,
            ssESfToTXp_DLen);

    pTxPath(
            piSHL_Mmio_EchoCtrl,
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
