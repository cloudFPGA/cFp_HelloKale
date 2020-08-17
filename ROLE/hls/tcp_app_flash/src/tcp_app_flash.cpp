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
 * @file     : tcp_app_flash.cpp
 * @brief    : TCP Application Flash (TAF)
 *
 * System:   : cloudFPGA
 * Component : cFp_BringUp / ROLE
 * Language  : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details  : This application implements a set of TCP-oriented tests and
 *  functions for the bring-up of a cloudFPGA module.
 *  The TAF connects to the SHELL via a TCP Shell Interface (TSIF) block. The
 *  main purpose of the TSIF is to provide a placeholder for the opening of one
 *  or multiple listening and active port(s). Its use is not a prerequisite, but
 *  it is provided here for sake of simplicity.
 *
 *          +-------+  +--------------------------------+
 *          |       |  |  +------+     +-------------+  |
 *          |       <-----+      <-----+             |  |
 *          | SHELL |  |  | TSIF |     |     TAF     |  |
 *          |       +----->      +----->             |  |
 *          |       |  |  +------+     +-------------+  |
 *          +-------+  +--------------------------------+
 *
 * \ingroup ROLE
 * \addtogroup ROLE_TAF
 * \{
 *******************************************************************************/

#include "tcp_app_flash.hpp"

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
 *  .e.g: DEBUG_LEVEL = (LSN_TRACE | ESF_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TAF"  // TcpAppFlash

#define TRACE_OFF  0x0000
#define TRACE_ESF 1 <<  1  // EchoStoreForward
#define TRACE_RXP 1 <<  2  // RxPath
#define TRACE_TXP 1 <<  3  // TxPath
#define TRACE_ALL  0xFFFF
#define DEBUG_LEVEL (TRACE_ALL)


/*******************************************************************************
 * @brief Echo Store and Forward (ESf)
 *
 * @param[in]  siRXp_Data  Data stream from pTcpRxPath (RXp)
 * @param[in]  siRXp_Meta  Metadata from [RXp]
 * @param[out] soTXp_Data  Data stream to pTcpRxPath (TXp).
 * @param[out] soTXp_Meta  Metadata to [TXp].
 *
 * @details
 *  Echo incoming traffic with store and forward in DDR4. Performs a loopback
 *   between the receive and transmit ports of the same TCP connection. The
 *   echo is said to operate in "store-and-forward" mode because every received
 *   segment is stored into the DDR4 memory before being read again and sent
 *   back to the remote source.
 *
 * [TODO - Implement this process as a real store-and-forward process]
 *******************************************************************************/
void pTcpEchoStoreAndForward(
        stream<TcpAppData>   &siRXp_Data,
        stream<TcpAppMeta>   &siRXp_Meta,
        stream<TcpAppData>   &soTXp_Data,
        stream<TcpAppMeta>   &soTXp_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "ESf");

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisApp     currChunk;
    TcpAppMeta  tcpSessId;

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { ESF_META=0, ESF_STREAM } \
                               esf_fsmState = ESF_META;
    #pragma HLS reset variable=esf_fsmState
    static  Ly4Len             esf_byteCnt;
    #pragma HLS reset variable=esf_byteCnt

    if ( !siRXp_Data.empty() and !soTXp_Data.full() ) {
        TcpAppData appData = siRXp_Data.read();
        soTXp_Data.write(appData);
        esf_byteCnt += appData.getLen();
        if (appData.getTLast()) {
            esf_byteCnt = 0;
        }
    }

    if ( !siRXp_Meta.empty() and !soTXp_Meta.full() ) {
        TcpAppMeta appMeta = siRXp_Meta.read();
        soTXp_Meta.write(appMeta);
    }

} // End of: pTcpEchoStoreAndForward()

/*******************************************************************************
 * @brief Transmit Path - From THIS to TSIF
 *
 * @param[in]  piSHL_MmioEchoCtrl  Configuration of the echo function.
 * @param[in]  siEPt_Data          Data from EchoPassTrough (EPt).
 * @param[in]  siEPt_Meta          Metadata from [EPt].
 * @param[in]  siESf_Data          Data from EchoStoreAndForward (ESf).
 * @param[in]  siESf_Meta          Metadata from [ESf].
 * @param[out] soSHL_Data          Data to SHELL (SHL).
 * @param[out] soSHL_Meta          Metadata to [SHL].
 *******************************************************************************/
void pTcpTxPath(
        ap_uint<2>          *piSHL_MmioEchoCtrl,
        stream<TcpAppData>  &siEPt_Data,
        stream<TcpAppMeta>  &siEPt_Meta,
        stream<TcpAppData>  &siESf_Data,
        stream<TcpAppMeta>  &siESf_Meta,
        stream<TcpAppData>  &soSHL_Data,
        stream<TcpAppMeta>  &soSHL_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "TXp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { TXP_START_OF_SEGMENT=0, TXP_CONTINUATION_OF_SEGMENT } \
                               txp_fsmState=TXP_START_OF_SEGMENT;
    #pragma HLS reset variable=txp_fsmState
    static enum MsgStates { MSG0=0, MSG1, MSG2, MSG3, MSG_GAP} \
                               txp_msgState = MSG0;
    #pragma HLS reset variable=txp_msgState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static AxisApp     txp_prevChunk;
    static bool        txp_wasLastChunk;
    static ap_uint<16> txp_msgGapSize;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TcpAppData     appData;
    TcpAppMeta     tcpSessId;

    switch (txp_fsmState ) {
    case TXP_START_OF_SEGMENT:
        switch(*piSHL_MmioEchoCtrl) {
        case ECHO_PATH_THRU:
            // Read session Id from pEchoPassThrough and forward to [SHL]
            if ( !siEPt_Meta.empty() and !soSHL_Meta.full() and
                 !siEPt_Data.empty() ) {
                TcpAppMeta appMeta;
                siEPt_Meta.read(appMeta);
                soSHL_Meta.write(appMeta);
                siEPt_Data.read(appData);
                txp_fsmState = TXP_CONTINUATION_OF_SEGMENT;
                txp_wasLastChunk = false;
            }
            break;
        case ECHO_STORE_FWD:
            //-- Read session Id from pTcpEchoStoreAndForward and forward to [SHL]
            if ( !siESf_Meta.empty() and !soSHL_Meta.full()) {
                TcpAppMeta appMeta;
                siESf_Meta.read(appMeta);
                soSHL_Meta.write(appMeta);
                txp_fsmState = TXP_CONTINUATION_OF_SEGMENT;
            }
            break;
        case ECHO_OFF:
            // Always drain and drop the session Id (if any)
            if ( !siEPt_Meta.empty() ) {
                siEPt_Meta.read();
            }
            else if ( !siESf_Meta.empty() ) {
                siESf_Meta.read();
            }
            break;
        default:
            // Always drain and drop the session Id (if any)
            if ( !siEPt_Meta.empty() ) {
                siEPt_Meta.read();
            }
            else if ( !siESf_Meta.empty() ) {
                siESf_Meta.read();
            }
            txp_fsmState = TXP_CONTINUATION_OF_SEGMENT;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
    case TXP_CONTINUATION_OF_SEGMENT:
        switch(*piSHL_MmioEchoCtrl) {
        case ECHO_PATH_THRU:
            //-- Read incoming data from pEchoPathThrough and forward to [SHL]
            if (txp_wasLastChunk & !soSHL_Data.full()) {
                soSHL_Data.write(txp_prevChunk);
                txp_wasLastChunk = false;
                txp_fsmState = TXP_START_OF_SEGMENT;
            }
            else {
                if (!siEPt_Data.empty() and !soSHL_Data.full()) {
                    siEPt_Data.read(appData);
                    soSHL_Data.write(txp_prevChunk);
                    // Update FSM state
                   if (appData.getTLast()) {
                       txp_wasLastChunk = true;
                   }
                }
            }
            break;
        case ECHO_STORE_FWD:
            //-- Read incoming data from pTcpEchoStoreAndForward and forward to [SHL]
            if ( !siESf_Data.empty() and !soSHL_Data.full()) {
                siESf_Data.read(appData);
                soSHL_Data.write(appData);
                // Update FSM state
                if (appData.getTLast()) {
                    txp_fsmState = TXP_START_OF_SEGMENT;
                }
            }
            break;
        case ECHO_OFF:
            // Always drain and drop the data segments (if any)
            if ( !siEPt_Data.empty() ) {
                siEPt_Data.read(appData);
            }
            else if ( !siESf_Data.empty() ) {
                siESf_Data.read(appData);
            }
            break;
        default:
            // Always drain and drop the data segments (if any)
            if ( !siEPt_Data.empty() ) {
                siEPt_Data.read(appData);
            }
            else if ( !siESf_Data.empty() ) {
                siESf_Data.read(appData);
            }
            // Always alternate between START and CONTINUATION to drain all streams
            txp_fsmState = TXP_START_OF_SEGMENT;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
    }  // End-of: switch (txp_fsmState ) {

    //-- Update the one-word internal pipeline
    txp_prevChunk = appData;

} // End of: pTcpTxPath()

/*******************************************************************************
 * @brief TCP Receive Path (RXp) - From SHELL->ROLE/TSIF to THIS.
 *
 * @param[in]  piSHL_MmioEchoCtrl Configuration of the echo function.
 * @param[in]  siSHL_Data         Data segment from TcpShellInterface (TSIF).
 * @param[in]  siSHL_Meta         Metadata from [TSIF].
 * @param[out] soEPt_Data         Data segment to EchoPassTrough (EPt).
 * @param[out] soEPt_Meta         Metadata to [EPt].
 * @param[out] soESf_Data         Data segment to EchoStoreAndForward (ESf).
 * @param[out] soESF_Meta         Metadata to [ESf].
 *
 * @details This Process waits for a new segment to read and forwards it to the
 *   EchoPathThrough (EPt) or EchoStoreAndForward (ESf) process upon the setting
 *   of the TCP destination port.
 *   (FYI-This function used to be performed by the 'piSHL_Mmio_EchoCtrl' bits).
 *******************************************************************************/
void pTcpRxPath(
        ap_uint<2>           *piSHL_MmioEchoCtrl,
        stream<TcpAppData>   &siTSIF_Data,
        stream<TcpAppMeta>   &siTSIF_Meta,
        stream<TcpAppData>   &soEPt_Data,
        stream<TcpAppMeta>   &soEPt_Meta,
        stream<TcpAppData>   &soESf_Data,
        stream<TcpAppMeta>   &soESf_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "RXp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { RXP_START_OF_SEGMENT=0, RXP_CONTINUATION_OF_SEGMENT } \
                               rxp_fsmState=RXP_START_OF_SEGMENT;
    #pragma HLS reset variable=rxp_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static bool       rxp_wasLastChunk;
    static TcpAppData rxp_prevChunk;

    //-- LOCAL VARIABLES -------------------------------------------------------
    TcpAppData        appData;

    switch (rxp_fsmState ) {
    case RXP_START_OF_SEGMENT:
        switch(*piSHL_MmioEchoCtrl) {
        case ECHO_PATH_THRU:
            //-- Read incoming session Id and forward to pEchoPathThrough
            if ( !siTSIF_Meta.empty() and !soEPt_Meta.full() and
                 !siTSIF_Data.empty() ) {
                soEPt_Meta.write(siTSIF_Meta.read());
                siTSIF_Data.read(appData);
                rxp_fsmState = RXP_CONTINUATION_OF_SEGMENT;
            }
            break;
          case ECHO_STORE_FWD:
            //-- Read incoming session Id and forward to pTcpEchoStoreAndForward
            if ( !siTSIF_Meta.empty() and !soESf_Meta.full()) {
                soESf_Meta.write(siTSIF_Meta.read());
                rxp_fsmState = RXP_CONTINUATION_OF_SEGMENT;
            }
            break;
          case ECHO_OFF:
          default:
            // Drain and drop the segment
            if ( !siTSIF_Meta.empty() ) {
                siTSIF_Meta.read();
                rxp_fsmState = RXP_CONTINUATION_OF_SEGMENT;
            }
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        rxp_wasLastChunk = false;
        break;
      case RXP_CONTINUATION_OF_SEGMENT:
        switch(*piSHL_MmioEchoCtrl) {
          case ECHO_PATH_THRU:
            //-- Read incoming data and forward to pEchoPathThrough
            if (rxp_wasLastChunk & !soEPt_Data.full()) {
                soEPt_Data.write(rxp_prevChunk);
                rxp_wasLastChunk = false;
                rxp_fsmState = RXP_START_OF_SEGMENT;
            }
            else {
                if (!siTSIF_Data.empty() and !soEPt_Data.full()) {
                    siTSIF_Data.read(appData);
                    soEPt_Data.write(rxp_prevChunk);
                    // Update FSM state
                   if (appData.getTLast()) {
                       rxp_wasLastChunk = true;
                   }
                }
            }
            break;
          case ECHO_STORE_FWD:
            //-- Read incoming data and forward to pTcpEchoStoreAndForward
            if ( !siTSIF_Data.empty() and !soESf_Data.full()) {
                siTSIF_Data.read(appData);
                soESf_Data.write(appData);
                // Update FSM state
                if (appData.getTLast()) {
                    rxp_fsmState = RXP_START_OF_SEGMENT;
                }
            }
            break;
          case ECHO_OFF:
          default:
            // Drain and drop the segment
            if ( !siTSIF_Data.empty() ) {
                siTSIF_Data.read(appData);
            }
            // Always alternate between START and CONTINUATION to drain all streams
            rxp_fsmState = RXP_START_OF_SEGMENT;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;

    }  // End-of: switch (rxpFsmState ) {

    //-- Update the one-word internal pipeline
    rxp_prevChunk = appData;

} // End of: pTcpRxPath()

/*******************************************************************************
 * @brief   Main process of the TCP Application Flash (TAF)
 *
 * @param[in]  piSHL_MmioEchoCtrl  Configures the echo function.
 * @param[in]  siSHL_Data          TCP data stream from the SHELL [SHL].
 * @param[in]  siSHL_Meta          TCP session Id from [SHL].
 * @param[out] soSHL_Data          TCP data stream to [SHL].
 *******************************************************************************/
void tcp_app_flash (

        //------------------------------------------------------
        //-- SHELL / MMIO / Configuration Interfaces
        //------------------------------------------------------
        ap_uint<2>          *piSHL_MmioEchoCtrl,
        //------------------------------------------------------
        //-- SHELL / TCP Rx Data Interface
        //------------------------------------------------------
        stream<TcpAppData>  &siSHL_Data,
        stream<TcpAppMeta>  &siSHL_Meta,
        //------------------------------------------------------
        //-- SHELL / TCP Tx Data Interface
        //------------------------------------------------------
        stream<TcpAppData>  &soSHL_Data,
        stream<TcpAppMeta>  &soSHL_Meta)
{

    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

#if defined(USE_DEPRECATED_DIRECTIVES)

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

    #pragma HLS INTERFACE ap_stable    port=piSHL_MmioEchoCtrl

    #pragma HLS resource core=AXI4Stream variable=siSHL_Data metadata="-bus_bundle siSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=siSHL_Meta metadata="-bus_bundle siSHL_Meta"

	#pragma HLS resource core=AXI4Stream variable=soSHL_Data metadata="-bus_bundle soSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=soSHL_Meta metadata="-bus_bundle soSHL_Meta"

#else

    #pragma HLS INTERFACE ap_stable          port=piSHL_MmioEchoCtrl

    // [INFO] Always add "register off" because (default value is "both")
    #pragma HLS INTERFACE axis register both port=siSHL_Data
    #pragma HLS INTERFACE axis register both port=siSHL_Meta        name=siSHL_Meta
    #pragma HLS INTERFACE axis register both port=soSHL_Data        name=soSHL_Data
    #pragma HLS INTERFACE axis register both port=soSHL_Meta        name=soSHL_Meta

#endif

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW


    //--------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    //-- Rx Path (RXp) ---------------------------------------------------------
    static stream<TcpAppData>   ssRXpToTXp_Data   ("ssRXpToTXp_Data");
    #pragma HLS STREAM variable=ssRXpToTXp_Data   depth=2048
    static stream<TcpAppMeta>   ssRXpToTXp_Meta   ("ssRXpToTXp_Meta");
    #pragma HLS STREAM variable=ssRXpToTXp_Meta   depth=64

    static stream<TcpAppData>   ssRXpToESf_Data   ("ssRXpToESf_Data");
    #pragma HLS STREAM variable=ssRXpToESf_Data   depth=1024
    static stream<TcpAppMeta>   ssRXpToESf_Meta   ("ssRXpToESf_Meta");
    #pragma HLS STREAM variable=ssRXpToESf_Meta   depth=32

    //-- Echo Store and Forward (ESf) ------------------------------------------
    static stream<TcpAppData>   ssESfToTXp_Data   ("ssESfToTXp_Data");
    #pragma HLS STREAM variable=ssESfToTXp_Data   depth=1024
    static stream<TcpAppMeta>   ssESfToTXp_Meta   ("ssESfToTXp_Meta");
    #pragma HLS STREAM variable=ssESfToTXp_Meta   depth=32

    //-- PROCESS FUNCTIONS -----------------------------------------------------
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
    //--------------------------------------------------------------------------
    pTcpRxPath(
            piSHL_MmioEchoCtrl,
            siSHL_Data,
            siSHL_Meta,
            ssRXpToTXp_Data,
            ssRXpToTXp_Meta,
            ssRXpToESf_Data,
            ssRXpToESf_Meta);

    pTcpEchoStoreAndForward(
            ssRXpToESf_Data,
            ssRXpToESf_Meta,
            ssESfToTXp_Data,
            ssESfToTXp_Meta);

    pTcpTxPath(
            piSHL_MmioEchoCtrl,
            ssRXpToTXp_Data,
            ssRXpToTXp_Meta,
            ssESfToTXp_Data,
            ssESfToTXp_Meta,
            soSHL_Data,
            soSHL_Meta);

}

/*! \} */
