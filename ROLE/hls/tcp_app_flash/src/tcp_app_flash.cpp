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
 *  or multiple listening port(s). Its use is not a prerequisite, but it is
 *  provided here for sake of simplicity.
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

    //-- LOCAL STREAMS ---------------------------------------------------------
    static stream<AxisApp>      ssDataFifo ("ssDataFifo");
    #pragma HLS stream variable=ssDataFifo depth=2048

    static stream<TcpAppMeta>   ssMetaFifo ("ssMetaFifo");
    #pragma HLS stream variable=ssMetaFifo depth=64

    //-- DATA STREAM (w/o internal FIFO ---------
    if ( !siRXp_Data.empty() && !soTXp_Data.full() ) {
        soTXp_Data.write(siRXp_Data.read());
    }
    //-- META STREAM  (w/o internal FIFO --------
    if ( !siRXp_Meta.empty() && !soTXp_Meta.full() ) {
        soTXp_Meta.write(siRXp_Meta.read());
    }

} // End of: pTcpEchoStoreAndForward()

/*******************************************************************************
 * @brief Transmit Path - From THIS to SHELL
 *
 * @param[in]  piSHL_MmioEchoCtrl  Configuration of the echo function.
 * @param[in]  piSHL_MmioPostSegEn Enables the posting of TCP segments.
 * @param[in]  piTSIF_SConnectId   Session connect Id from [TSIF].
 * @param[in]  siEPt_Data          Data from EchoPassTrough (EPt).
 * @param[in]  siEPt_Meta          Metadata from [EPt].
 * @param[in]  siESf_Data          Data from EchoStoreAndForward (ESf).
 * @param[in]  siESf_Meta          Metadata from [ESf].
 * @param[out] soSHL_Data          Data to SHELL (SHL).
 * @param[out] soSHL_Meta          Metadata to [SHL].
 *******************************************************************************/
void pTcpTxPath(
        ap_uint<2>          *piSHL_MmioEchoCtrl,
        CmdBit              *piSHL_MmioPostSegEn,
        SessionId           *piTSIF_SConnectId,
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
    static enum FsmStates { START_OF_SEGMENT=0, CONTINUATION_OF_SEGMENT } \
                               txp_fsmState = START_OF_SEGMENT;
    #pragma HLS reset variable=txp_fsmState
    static enum MsgStates { MSG0=0, MSG1, MSG2, MSG3, MSG_GAP} \
                               txp_msgState = MSG0;
    #pragma HLS reset variable=txp_msgState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static AxisApp     txp_prevChunk;
    static bool        txp_wasLastChunk;
    static ap_uint<16> txp_msgGapSize;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TcpAppData     currChunk;
    TcpAppMeta     tcpSessId;

    switch (txp_fsmState ) {
    case START_OF_SEGMENT:
        switch(*piSHL_MmioEchoCtrl) {
        case ECHO_PATH_THRU:
            // Read session Id from pEchoPassThrough and forward to [SHL]
            if ( !siEPt_Meta.empty() and !soSHL_Meta.full() and
                 !siEPt_Data.empty() ) {
                TcpAppMeta appMeta;
                siEPt_Meta.read(appMeta);
                soSHL_Meta.write(appMeta);
                siEPt_Data.read(currChunk);
                txp_fsmState = CONTINUATION_OF_SEGMENT;
                txp_wasLastChunk = false;
            }
            break;
        case ECHO_STORE_FWD:
            //-- Read session Id from pTcpEchoStoreAndForward and forward to [SHL]
            if ( !siESf_Meta.empty() and !soSHL_Meta.full()) {
                TcpAppMeta appMeta;
                siESf_Meta.read(appMeta);
                soSHL_Meta.write(appMeta);
                txp_fsmState = CONTINUATION_OF_SEGMENT;
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
            if (*piSHL_MmioPostSegEn) {
                // Prepare for sending a segment to remote host by forwarding
                // a session-id
                if (!soSHL_Meta.full()) {
                    SessionId sessConId = *piTSIF_SConnectId;
                    soSHL_Meta.write(TcpAppMeta(sessConId));
                    txp_fsmState = CONTINUATION_OF_SEGMENT;
                }
            }
            else {
                txp_fsmState = CONTINUATION_OF_SEGMENT;
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
            txp_fsmState = CONTINUATION_OF_SEGMENT;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
    case CONTINUATION_OF_SEGMENT:
        switch(*piSHL_MmioEchoCtrl) {
        case ECHO_PATH_THRU:
            //-- Read incoming data from pEchoPathThrough and forward to [SHL]
            if (txp_wasLastChunk & !soSHL_Data.full()) {
                soSHL_Data.write(txp_prevChunk);
                txp_wasLastChunk = false;
                txp_fsmState = START_OF_SEGMENT;
            }
            else {
                if (!siEPt_Data.empty() and !soSHL_Data.full()) {
                    siEPt_Data.read(currChunk);
                    soSHL_Data.write(txp_prevChunk);
                    // Update FSM state
                   if (currChunk.getTLast()) {
                       txp_wasLastChunk = true;
                   }
                }
            }
            break;
        case ECHO_STORE_FWD:
            //-- Read incoming data from pTcpEchoStoreAndForward and forward to [SHL]
            if ( !siESf_Data.empty() and !soSHL_Data.full()) {
                siESf_Data.read(currChunk);
                soSHL_Data.write(currChunk);
                // Update FSM state
                if (currChunk.getTLast()) {
                    txp_fsmState = START_OF_SEGMENT;
                }
            }
            break;
        case ECHO_OFF:
            // Always drain and drop the data segments (if any)
            if ( !siEPt_Data.empty() ) {
                siEPt_Data.read(currChunk);
            }
            else if ( !siESf_Data.empty() ) {
                siESf_Data.read(currChunk);
            }
            if (*piSHL_MmioPostSegEn) {
                // Always send the same basic message to the remote host
                if ( !soSHL_Data.full()) {
                    switch(txp_msgState) {
                    case MSG0: // 'Hello Wo'
                        soSHL_Data.write(TcpAppData(0x48656c6c6f20576f, 0xFF, 0));
                        txp_msgState = MSG1;
                        break;
                    case MSG1: // 'rld from'
                        soSHL_Data.write(TcpAppData(0x726c642066726f6d, 0xFF, 0));
                        txp_msgState = MSG2;
                        break;
                    case MSG2: // ' FMKU60 '
                        soSHL_Data.write(TcpAppData(0x20464d4b55363020, 0xFF, 0));
                        txp_msgState = MSG3;
                        break;
                    case MSG3: // '--------'
                        soSHL_Data.write(TcpAppData(0x2d2d2d2d2d2d2d2d, 0xFF, 1));
                        txp_msgGapSize  = 0xFFFF; // [TODO - Make this programmable]
                        txp_msgState = MSG_GAP;
                        break;
                    case MSG_GAP: // Short pause before sending next segment
                        if (txp_msgGapSize) {
                            txp_msgGapSize--;
                        }
                        else {
                            txp_msgState = MSG0;
                            txp_fsmState = START_OF_SEGMENT;
                        }
                    }
                }
                else {
                    txp_fsmState = START_OF_SEGMENT;
                }
            }
            break;
        default:
            // Always drain and drop the data segments (if any)
            if ( !siEPt_Data.empty() ) {
                siEPt_Data.read(currChunk);
            }
            else if ( !siESf_Data.empty() ) {
                siESf_Data.read(currChunk);
            }
            // Always alternate between START and CONTINUATION to drain all streams
            txp_fsmState = START_OF_SEGMENT;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
    }  // End-of: switch (txp_fsmState ) {

    //-- Update the one-word internal pipeline
    txp_prevChunk = currChunk;

} // End of: pTcpTxPath()

/*******************************************************************************
 * @brief Receive Path - FROM SHELL to THIS
 *
 * @param[in]  piSHL_MmioEchoCtrl Configuration of the echo function.
 * @param[in]  siSHL_Data         Data from SHELL (SHL).
 * @param[in]  siSHL_Meta         Metadata from [SHL].
 * @param[out] soEPt_Data         Data to EchoPassTrough (EPt).
 * @param[out] soEPt_Meta         Metadata to [EPt].
 * @param[out] soESf_Data         Data to EchoStoreAndForward (ESf).
 * @param[out] soESF_Meta         Metadata to [ESf].
 *******************************************************************************/
void pTcpRxPath(
        ap_uint<2>           *piSHL_MmioEchoCtrl,
        stream<TcpAppData>   &siSHL_Data,
        stream<TcpAppMeta>   &siSHL_Meta,
        stream<TcpAppData>   &soEPt_Data,
        stream<TcpAppMeta>   &soEPt_Meta,
        stream<TcpAppData>   &soESf_Data,
        stream<TcpAppMeta>   &soESf_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "RXp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { START_OF_SEGMENT=0, CONTINUATION_OF_SEGMENT } \
                               rxp_fsmState = START_OF_SEGMENT;
    #pragma HLS reset variable=rxp_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static bool       rxp_wasLastChunk;
    static TcpAppData rxp_prevChunk;

    //-- LOCAL VARIABLES -------------------------------------------------------
    //OBSOLETE TcpAppData        tcpWord;
    TcpAppData        currChunk;

    switch (rxp_fsmState ) {
      case START_OF_SEGMENT:
        switch(*piSHL_MmioEchoCtrl) {
          case ECHO_PATH_THRU:
            //-- Read incoming session Id and forward to pEchoPathThrough
            if ( !siSHL_Meta.empty() and !soEPt_Meta.full() and
                 !siSHL_Data.empty() ) {
                soEPt_Meta.write(siSHL_Meta.read());
                siSHL_Data.read(currChunk);
                rxp_fsmState = CONTINUATION_OF_SEGMENT;
            }
            break;
          case ECHO_STORE_FWD:
            //-- Read incoming session Id and forward to pTcpEchoStoreAndForward
            if ( !siSHL_Meta.empty() and !soESf_Meta.full()) {
                soESf_Meta.write(siSHL_Meta.read());
                rxp_fsmState = CONTINUATION_OF_SEGMENT;
            }
            break;
          case ECHO_OFF:
          default:
            // Drain and drop the segment
            if ( !siSHL_Meta.empty() ) {
                siSHL_Meta.read();
                rxp_fsmState = CONTINUATION_OF_SEGMENT;
            }
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        rxp_wasLastChunk = false;
        break;
      case CONTINUATION_OF_SEGMENT:
        switch(*piSHL_MmioEchoCtrl) {
          case ECHO_PATH_THRU:
            //-- Read incoming data and forward to pEchoPathThrough
            if (rxp_wasLastChunk & !soEPt_Data.full()) {
                soEPt_Data.write(rxp_prevChunk);
                rxp_wasLastChunk = false;
                rxp_fsmState = START_OF_SEGMENT;
            }
            else {
                if (!siSHL_Data.empty() and !soEPt_Data.full()) {
                    siSHL_Data.read(currChunk);
                    soEPt_Data.write(rxp_prevChunk);
                    // Update FSM state
                   if (currChunk.getTLast()) {
                       rxp_wasLastChunk = true;
                   }
                }
            }
            break;
          case ECHO_STORE_FWD:
            //-- Read incoming data and forward to pTcpEchoStoreAndForward
            if ( !siSHL_Data.empty() and !soESf_Data.full()) {
                siSHL_Data.read(currChunk);
                soESf_Data.write(currChunk);
                // Update FSM state
                if (currChunk.getTLast()) {
                    rxp_fsmState = START_OF_SEGMENT;
                }
            }
            break;
          case ECHO_OFF:
          default:
            // Drain and drop the segment
            if ( !siSHL_Data.empty() ) {
                siSHL_Data.read(currChunk);
            }
            // Always alternate between START and CONTINUATION to drain all streams
            rxp_fsmState = START_OF_SEGMENT;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;

    }  // End-of: switch (rxpFsmState ) {

    //-- Update the one-word internal pipeline
    rxp_prevChunk = currChunk;

} // End of: pTcpRxPath()

/*******************************************************************************
 * @brief   Main process of the TCP Application Flash
 *
 * @param[in]  piSHL_MmioEchoCtrl  Configures the echo function.
 * @param[in]  piSHL_MmioPostSegEn Enables posting of TCP segments.
 * @param[in]  piTSIF_SConnectId   Session connect Id from [TSIF].
 * @param[in]  piSHL_MmioCaptSegEn Enables capture of a TCP segment by [MMIO].
 * @param[in]  siSHL_Data          TCP data stream from the SHELL [SHL].
 * @param[in]  siSHL_Meta          TCP session Id from [SHL].
 * @param[out] soSHL_Data          TCP data stream to [SHL].
 *******************************************************************************/
void tcp_app_flash (

        //------------------------------------------------------
        //-- SHELL / MMIO / Configuration Interfaces
        //------------------------------------------------------
        ap_uint<2>          *piSHL_MmioEchoCtrl,
        CmdBit              *piSHL_MmioPostSegEn,
        //[TODO] ap_uint<1> *piSHL_MmioCaptSegEn,

        //------------------------------------------------------
        //-- TSIF / Session Connect Id Interface
        //------------------------------------------------------
        SessionId           *piTSIF_SConnectId,

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
    #pragma HLS INTERFACE ap_stable    port=piSHL_MmioPostSegEn
    //[TODO] #pragma HLS INTERFACE ap_stable     port=piSHL_MmioCaptSegEn

    #pragma HLS INTERFACE ap_stable    port=piTSIF_SConnectId

    #pragma HLS resource core=AXI4Stream variable=siSHL_Data metadata="-bus_bundle siSHL_Data"

    #pragma HLS resource core=AXI4Stream variable=siSHL_Meta metadata="-bus_bundle siSHL_Meta"
    #pragma HLS resource core=AXI4Stream variable=soSHL_Data metadata="-bus_bundle soSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=soSHL_Meta metadata="-bus_bundle soSHL_Meta"

#else

    #pragma HLS INTERFACE ap_stable          port=piSHL_MmioEchoCtrl
    #pragma HLS INTERFACE ap_stable          port=piSHL_MmioPostSegEn name=piSHL_MmioPostSegEn
    //[TODO] #pragma HLS INTERFACE ap_stable port=piSHL_MmioCapSegtEn name=piSHL_MmioCapSegtEn

    #pragma HLS INTERFACE ap_vld       port=piTSIF_SConnectId

    // [INFO] Always add "register off" because (default value is "both")
    #pragma HLS INTERFACE axis register both port=siSHL_Data
    #pragma HLS INTERFACE axis register both port=siSHL_Meta        name=siSHL_Meta
    #pragma HLS INTERFACE axis register both port=soSHL_Data        name=soSHL_Data
    #pragma HLS INTERFACE axis register both port=soSHL_Meta        name=soSHL_Meta

#endif

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW

    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)

    //--------------------------------------------------------------------------
    //-- Rx Path (RXp)
    //--------------------------------------------------------------------------
    static stream<TcpAppData>   ssRXpToTXp_Data   ("ssRXpToTXp_Data");
    #pragma HLS STREAM variable=ssRXpToTXp_Data   depth=2048
    static stream<TcpAppMeta>   ssRXpToTXp_Meta   ("ssRXpToTXp_Meta");
    #pragma HLS STREAM variable=ssRXpToTXp_Meta   depth=64

    static stream<TcpAppData>   ssRXpToESf_Data   ("ssRXpToESf_Data");
    #pragma HLS STREAM variable=ssRXpToESf_Data   depth=2
    static stream<TcpAppMeta>   ssRXpToESf_Meta   ("ssRXpToESf_Meta");
    #pragma HLS STREAM variable=ssRXpToESf_Meta   depth=2

    //--------------------------------------------------------------------------
    //-- Echo Store and Forward (ESf)
    //--------------------------------------------------------------------------
    static stream<TcpAppData>   ssESfToTXp_Data   ("ssESfToTXp_Data");
    #pragma HLS STREAM variable=ssESfToTXp_Data   depth=2
    static stream<TcpAppMeta>   ssESfToTXp_Meta   ("ssESfToTXp_Meta");
    #pragma HLS STREAM variable=ssESfToTXp_Meta   depth=2

    //-- PROCESS FUNCTIONS -----------------------------------------------------
    //
    //       +-+                                   | |
    //       |S|    1      +----------+            |S|
    //       |i| +-------->|   pESf   |----------+ |r|
    //       |n| |         +----------+          | |c|
    //       |k| |    2     --------+            | +++
    //       /|\ |  +--------> sEPt |---------+  |  |
    //       0|  |  |       --------+         |  | \|/
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
            piSHL_MmioPostSegEn,
            piTSIF_SConnectId,
            ssRXpToTXp_Data,
            ssRXpToTXp_Meta,
            ssESfToTXp_Data,
            ssESfToTXp_Meta,
            soSHL_Data,
            soSHL_Meta);

}

/*! \} */
