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

/*****************************************************************************
 * @file       : tcp_app_flash.cpp
 * @brief      : TCP Application Flash (TAF)
 *
 * System:     : cloudFPGA
 * Component   : RoleFlash
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details    : This application implements a set of TCP-oriented tests and
 *  functions which are embedded into the Flash of the cloudFPGA role.
 *
 * @note       : For the time being, we continue designing with the DEPRECATED
 *  directives because the new PRAGMAs do not work for us.
 *
 *****************************************************************************/

#include "../../role.hpp"
#include "../../role_utils.hpp"
#include "../../test_role_utils.hpp"

#include "tcp_app_flash.hpp"

/************************************************
 * INTERFACE SYNTHESIS DIRECTIVES
 *  For the time being, we may continue to design
 *  with the DEPRECATED directives because the
 *  new PRAGMAs do not always work for us.
 ************************************************/
#undef USE_DEPRECATED_DIRECTIVES

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (LSN_TRACE | ESF_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TAP"  // TcpAppFlash

#define TRACE_OFF  0x0000
#define TRACE_ESF 1 <<  1  // EchoStoreForward
#define TRACE_RXP 1 <<  2  // RxPath
#define TRACE_TXP 1 <<  3  // TxPath
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)


/*****************************************************************************
 * @brief Echo loopback with store and forward in DDR4.
 *
 * @param[in]  siRXp_Data,   data from pRXPath (RXp)
 * @param[in]  siRXp_SessId, session Id from [RXp]
 * @param[out] soTXp_Data,   data to pTXPath (TXp).
 * @param[out] soTXp_SessId, session Id to [TXp].
 *
 * @details
 *  Loopback between the Rx and Tx ports of the TCP connection. The echo is
 *  said to operate in "store-and-forward" mode because every received segment
 *  is stored into the DDR4 memory before being read again and and sent back.
 *
 * @return Nothing.
 ******************************************************************************/
void pEchoStoreAndForward( // [TODO - Implement this process as a real store-and-forward]
        stream<AppData>   &siRXp_Data,
        stream<AppMeta>   &siRXp_SessId,
        stream<AppData>   &soTXp_Data,
        stream<AppMeta>   &soTXp_SessId)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW  // Why not #pragma HLS PIPELINE II=1 ?

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxiWord     axiWord;
    TcpSessId   tcpSessId;

    //-- LOCAL STREAMS --------------------------------------------------------
    static stream<AxiWord>      ssDataFifo ("ssDataFifo");
    #pragma HLS stream     variable=ssDataFifo depth=2048

    static stream<TcpSessId>    ssMetaFifo ("ssMetaFifo");
    #pragma HLS stream variable=ssMetaFifo depth=64

    /*** [TODO - Do not use an internal FIFO here because it breaks the II=1]
    //-- DATA STREAM ----------------------------
    if ( !siRXp_Data.empty() && !ssDataFifo.full() &&
         !soTXp_Data.full()  && !ssDataFifo.empty() ) {
        //-- FiFo Push and Pop ------------
        ssDataFifo.write(siRXp_Data.read());
        soTXp_Data.write(ssDataFifo.read());
    }
    else if ( !siRXp_Data.empty() && !ssDataFifo.full() ) {
        //-- FiFo Push only ---------------
        ssDataFifo.write(siRXp_Data.read());
    }
    else if ( !soTXp_Data.full() && !ssDataFifo.empty() ) {
        //-- FiFo Pop only ----------------
        soTXp_Data.write(ssDataFifo.read());
       }

    //-- META STREAM ----------------------------
    if ( !siRXp_SessId.empty() && !ssMetaFifo.full() &&
         !soTXp_SessId.full()  && !ssMetaFifo.empty() ) {
        //-- FiFo Push and Pop ------------
        ssMetaFifo.write(siRXp_SessId.read());
        soTXp_SessId.write(ssMetaFifo.read());
    }
    else if ( !siRXp_SessId.empty() && !ssMetaFifo.full() ) {
        //-- FiFo Push only ---------------
        ssMetaFifo.write(siRXp_SessId.read());
    }
    else if ( !soTXp_SessId.full() && !ssMetaFifo.empty() ) {
        //-- FiFo Pop only ----------------
        soTXp_SessId.write(ssMetaFifo.read());
    }
    ************************************************************************/

    //-- DATA STREAM (w/o internal FIFO ---------
    if ( !siRXp_Data.empty() && !soTXp_Data.full() ) {
        soTXp_Data.write(siRXp_Data.read());
    }
    //-- META STREAM  (w/o internal FIFO ---------
    if ( !siRXp_SessId.empty() && !soTXp_SessId.full() ) {
        soTXp_SessId.write(siRXp_SessId.read());
    }

} // End of: pEchoStoreAndForward()


/*****************************************************************************
 * @brief Transmit Path (.i.e Data and metadata to SHELL)
 *
 * @param[in]  piSHL_MmioEchoCtrl, configuration of the echo function.
 * @param[in]  piSHL_MmioPostSegEn,enables the posting of TCP segments.
 * @param[in]  siEPt_Data,         data from EchoPassTrough (EPt).
 * @param[in]  siEPt_SessId,       metadata from [EPt].
 * @param[in]  siESf_Data,         data from EchoStoreAndForward (ESf).
 * @param[in]  siESf_SessId,       metadata from [ESf].
 * @param[out] soSHL_Data,         data to SHELL (SHL).
 * @param[out] soSHL_SessId,       metadata to [SHL].
 *
 * @return Nothing.
 *****************************************************************************/
void pTXPath(
        ap_uint<2>          *piSHL_MmioEchoCtrl,
        ap_uint<1>          *piSHL_MmioPostSegEn,
        stream<AppData>     &siEPt_Data,
        stream<AppMeta>     &siEPt_SessId,
        stream<AppData>     &siESf_Data,
        stream<AppMeta>     &siESf_SessId,
        stream<AppData>     &soSHL_Data,
        stream<AppMeta>     &soSHL_SessId)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW // #pragma HLS PIPELINE II=1

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum TxpFsmStates { START_OF_SEGMENT=0, CONTINUATION_OF_SEGMENT } \
                               txpFsmState = START_OF_SEGMENT;
    #pragma HLS reset variable=txpFsmState
    static enum MsgFsmStates { MSG0=0, MSG1, MSG2, MSG3 } \
                               msgFsmState = MSG0;
    #pragma HLS reset variable=msgFsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static AxiWord prevAxiWord;
    static bool    wasLastWord;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxiWord        tcpWord;
    AxiWord        currAxiWord;
    TcpSessId      tcpSessId;

    switch (txpFsmState ) {

    case START_OF_SEGMENT:
        switch(*piSHL_MmioEchoCtrl) {
        case ECHO_PATH_THRU:
            // Read session Id from pEchoPassThrough and forward to [SHL]
            if ( !siEPt_SessId.empty() and !soSHL_SessId.full() and
                 !siEPt_Data.empty() ) {
                AppMeta appMeta;
                siEPt_SessId.read(appMeta);
                soSHL_SessId.write(appMeta.tdata);
                siEPt_Data.read(currAxiWord);
                txpFsmState = CONTINUATION_OF_SEGMENT;
                wasLastWord = false;
            }
            break;
        case ECHO_STORE_FWD:
            //-- Read session Id from pEchoStoreAndForward and forward to [SHL]
            if ( !siESf_SessId.empty() and !soSHL_SessId.full()) {
                AppMeta appMeta;
                siESf_SessId.read(appMeta);
                soSHL_SessId.write(appMeta.tdata);
                txpFsmState = CONTINUATION_OF_SEGMENT;
            }
            break;
        case ECHO_OFF:
            // Always drain and drop the session Id (if any)
            if ( !siEPt_SessId.empty() )
                siEPt_SessId.read();
            else if ( !siESf_SessId.empty() )
                siESf_SessId.read();

            if (*piSHL_MmioPostSegEn) {
                // Prepare for sending a segment to remote host by forwarding
                // a session-id (here we use a temporary dirty hack ; we always
                // forward SessId=1 because we know it is '1' after the first
                // session is opened).
                if (!soSHL_SessId.full()) {
                    soSHL_SessId.write(AppMeta(1));  // [FIXME-Must retrieve the session ID from TRIF]
                    txpFsmState = CONTINUATION_OF_SEGMENT;
                }
            }
            else {
                txpFsmState = CONTINUATION_OF_SEGMENT;
            }
            break;
        default:
            // Always drain and drop the session Id (if any)
            if ( !siEPt_SessId.empty() )
                siEPt_SessId.read();
            else if ( !siESf_SessId.empty() )
                siESf_SessId.read();
            txpFsmState = CONTINUATION_OF_SEGMENT;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;

    case CONTINUATION_OF_SEGMENT:
        switch(*piSHL_MmioEchoCtrl) {
        case ECHO_PATH_THRU:
            //-- Read incoming data from pEchoPathThrough and forward to [SHL]
            if (wasLastWord & !soSHL_Data.full()) {
                soSHL_Data.write(prevAxiWord);
                wasLastWord = false;
                txpFsmState = START_OF_SEGMENT;
            }
            else {
                if (!siEPt_Data.empty() and !soSHL_Data.full()) {
                    siEPt_Data.read(currAxiWord);
                    soSHL_Data.write(prevAxiWord);
                    // Update FSM state
                   if (currAxiWord.tlast)
                       wasLastWord = true;
                }
            }
            break;
        case ECHO_STORE_FWD:
            //-- Read incoming data from pEchoStoreAndForward and forward to [SHL]
            if ( !siESf_Data.empty() and !soSHL_Data.full()) {
                siESf_Data.read(tcpWord);
                soSHL_Data.write(tcpWord);
                // Update FSM state
                if (tcpWord.tlast)
                    txpFsmState = START_OF_SEGMENT;
            }
            break;
        case ECHO_OFF:
            // Always drain and drop the data segments (if any)
            if ( !siEPt_Data.empty() )
                siEPt_Data.read(tcpWord);
            else if ( !siESf_Data.empty() )
                siESf_Data.read(tcpWord);

            if (*piSHL_MmioPostSegEn) {
                // Always send the same basic message to the remote host
                if ( !soSHL_Data.full()) {
                    switch(msgFsmState) {
                    case MSG0: // 'Hello Wo'
                        soSHL_Data.write(AxiWord(0x48656c6c6f20576f, 0xFF, 0));
                        msgFsmState = MSG1;
                        break;
                    case MSG1: // 'rld from'
                        soSHL_Data.write(AxiWord(0x726c642066726f6d, 0xFF, 0));
                        msgFsmState = MSG2;
                        break;
                    case MSG2: // ' FMKU60 '
                        soSHL_Data.write(AxiWord(0x20464d4b55363020, 0xFF, 0));
                        msgFsmState = MSG3;
                        break;
                    case MSG3: // '--------'
                        soSHL_Data.write(AxiWord(0x2d2d2d2d2d2d2d2d, 0xFF, 1));
                        msgFsmState = MSG0;
                        txpFsmState = START_OF_SEGMENT;
                        break;
                    }
                }
                else {
                    txpFsmState = START_OF_SEGMENT;
                }
            }
            break;
        default:
            // Always drain and drop the data segments (if any)
            if ( !siEPt_Data.empty() )
                siEPt_Data.read(tcpWord);
            else if ( !siESf_Data.empty() )
                siESf_Data.read(tcpWord);
            // Always alternate between START and CONTINUATION to drain all streams
            txpFsmState = START_OF_SEGMENT;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
    }  // End-of: switch (txpFsmState ) {

    //-- Update the one-word internal pipeline
    prevAxiWord = currAxiWord;

} // End of: pTXPath()


/*****************************************************************************
 * @brief Receive Path (.i.e Data and metadata from SHELL).
 *
 * @param[in]  piSHL_MmioEchoCtrl, configuration of the echo function.
 * @param[in]  siSHL_Data,         data from SHELL (SHL).
 * @param[in]  siSHL_SessId,       metadata from [SHL].
 * @param[out] soEPt_Data,         data to EchoPassTrough (EPt).
 * @param[out] soEPt_SessId,       metadata to [EPt].
 * @param[out] soESf_Data,         data to EchoStoreAndForward (ESf).
 * @param[out] soESF_SessId,       metadata to [ESf].
 *
 * @return Nothing.
 ******************************************************************************/
void pRXPath(
        ap_uint<2>           *piSHL_MmioEchoCtrl,
        stream<AppData>      &siSHL_Data,
        stream<AppMeta>      &siSHL_SessId,
        stream<AppData>      &soEPt_Data,
        stream<AppMeta>      &soEPt_SessId,
        stream<AppData>      &soESf_Data,
        stream<AppMeta>      &soESf_SessId)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1  // #pragma HLS DATAFLOW //

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum RxpFsmStates { START_OF_SEGMENT=0, CONTINUATION_OF_SEGMENT } \
                               rxpFsmState = START_OF_SEGMENT;
    #pragma HLS reset variable=rxpFsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static bool    wasLastWord;
    static AxiWord prevAxiWord;

    //-- LOCAL VARIABLES ------------------------------------------------------
    AxiWord        tcpWord;
    AxiWord        currAxiWord;
    //OBSOLETE TcpSessId      tcpSessId;

    switch (rxpFsmState ) {
      case START_OF_SEGMENT:
        switch(*piSHL_MmioEchoCtrl) {
          case ECHO_PATH_THRU:
            //-- Read incoming session Id and forward to pEchoPathThrough
            if ( !siSHL_SessId.empty() and !soEPt_SessId.full() and
                 !siSHL_Data.empty() ) {
                soEPt_SessId.write(siSHL_SessId.read());
                siSHL_Data.read(currAxiWord);
                rxpFsmState = CONTINUATION_OF_SEGMENT;
            }
            break;
          case ECHO_STORE_FWD:
            //-- Read incoming session Id and forward to pEchoStoreAndForward
            if ( !siSHL_SessId.empty() and !soESf_SessId.full()) {
                soESf_SessId.write(siSHL_SessId.read());
                rxpFsmState = CONTINUATION_OF_SEGMENT;
            }
            break;
          case ECHO_OFF:
          default:
            // Drain and drop the segment
            if ( !siSHL_SessId.empty() ) {
                siSHL_SessId.read();
                rxpFsmState = CONTINUATION_OF_SEGMENT;
            }
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        wasLastWord = false;
        break;

      case CONTINUATION_OF_SEGMENT:
        switch(*piSHL_MmioEchoCtrl) {
          case ECHO_PATH_THRU:
            //-- Read incoming data and forward to pEchoPathThrough
            if (wasLastWord & !soEPt_Data.full()) {
                soEPt_Data.write(prevAxiWord);
                wasLastWord = false;
                rxpFsmState = START_OF_SEGMENT;
            }
            else {
                if (!siSHL_Data.empty() and !soEPt_Data.full()) {
                    siSHL_Data.read(currAxiWord);
                    soEPt_Data.write(prevAxiWord);
                    // Update FSM state
                   if (currAxiWord.tlast)
                       wasLastWord = true;
                }
            }
            break;
          case ECHO_STORE_FWD:
            //-- Read incoming data and forward to pEchoStoreAndForward
            if ( !siSHL_Data.empty() and !soESf_Data.full()) {
                siSHL_Data.read(tcpWord);
                soESf_Data.write(tcpWord);
                // Update FSM state
                if (tcpWord.tlast)
                    rxpFsmState = START_OF_SEGMENT;
            }
            break;
          case ECHO_OFF:
          default:
            // Drain and drop the segment
            if ( !siSHL_Data.empty() ) {
                siSHL_Data.read(tcpWord);
            }
            // Always alternate between START and CONTINUATION to drain all streams
            rxpFsmState = START_OF_SEGMENT;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;

    }  // End-of: switch (rxpFsmState ) {

    //-- Update the one-word internal pipeline
    prevAxiWord = currAxiWord;

} // End of: pRXPath()


/*****************************************************************************
 * @brief   Main process of the TCP Application Flash
 *
 * @param[in]  piSHL_MmioEchoCtrl,  configures the echo function.
 * @param[in]  piSHL_MmioPostSegEn, enables posting of TCP segments.
 * @param[in]  piSHL_MmioCaptSegEn, enables capture of a TCP segment by [MMIO].
 * @param[in]  siSHL_Data,          TCP data stream from the SHELL [SHL].
 * @param[in]  siSHL_SessId,        TCP session Id from [SHL].
 * @param[out] soSHL_Data           TCP data stream to [SHL].
 *
 * @return Nothing.
 *****************************************************************************/
void tcp_app_flash (

        //------------------------------------------------------
        //-- SHELL / MMIO / Configuration Interfaces
        //------------------------------------------------------
        ap_uint<2>          *piSHL_MmioEchoCtrl,
        ap_uint<1>          *piSHL_MmioPostSegEn,
        //[TODO] ap_uint<1> *piSHL_MmioCaptSegEn,

        //------------------------------------------------------
        //-- SHELL / TCP Rx Data Interface
        //------------------------------------------------------
        stream<AppData>     &siSHL_Data,
        stream<AppMeta>     &siSHL_SessId,

        //------------------------------------------------------
        //-- SHELL / TCP Tx Data Interface
        //------------------------------------------------------
        stream<AppData>     &soSHL_Data,
        stream<AppMeta>     &soSHL_SessId)
{

    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

#if defined(USE_DEPRECATED_DIRECTIVES)

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

    #pragma HLS INTERFACE ap_stable    port=piSHL_MmioEchoCtrl
    #pragma HLS INTERFACE ap_stable    port=piSHL_MmioPostSegEn
    //[TODO] #pragma HLS INTERFACE ap_stable     port=piSHL_MmioCaptSegEn

    #pragma HLS resource core=AXI4Stream variable=siSHL_Data   metadata="-bus_bundle siSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=siSHL_SessId metadata="-bus_bundle siSHL_SessId"
    #pragma HLS resource core=AXI4Stream variable=soSHL_Data   metadata="-bus_bundle soSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=soSHL_SessId metadata="-bus_bundle soSHL_SessId"

#else

    #pragma HLS INTERFACE ap_stable          port=piSHL_MmioEchoCtrl
    #pragma HLS INTERFACE ap_stable          port=piSHL_MmioPostSegEn name=piSHL_MmioPostSegEn
    //[TODO] #pragma HLS INTERFACE ap_stable port=piSHL_MmioCapSegtEn name=piSHL_MmioCapSegtEn

    // [INFO] Always add "register off" because (default value is "both")
    #pragma HLS INTERFACE axis register both port=siSHL_Data
    #pragma HLS INTERFACE axis register both port=siSHL_SessId        name=siSHL_SessId
    #pragma HLS INTERFACE axis register both port=soSHL_Data          name=soSHL_Data
    #pragma HLS INTERFACE axis register both port=soSHL_SessId        name=soSHL_SessId

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)

    //-------------------------------------------------------------------------
    //-- Rx Path (RXp)
    //-------------------------------------------------------------------------
    static stream<AppData>      sRXpToTXp_Data    ("sRXpToTXp_Data");
    #pragma HLS STREAM variable=sRXpToTXp_Data    depth=2048
    static stream<AppMeta>      sRXpToTXp_SessId  ("sRXpToTXp_SessId");
    #pragma HLS STREAM variable=sRXpToTXp_SessId  depth=64

    static stream<AppData>      sRXpToESf_Data    ("sRXpToESf_Data");
    #pragma HLS STREAM variable=sRXpToESf_Data    depth=2
    static stream<AppMeta>      sRXpToESf_SessId  ("sRXpToESf_SessId");
    #pragma HLS STREAM variable=sRXpToESf_SessId  depth=2

    //-------------------------------------------------------------------------
    //-- Echo Store and Forward (ESf)
    //-------------------------------------------------------------------------
    static stream<AppData>      sESfToTXp_Data    ("sESfToTXp_Data");
    #pragma HLS STREAM variable=sESfToTXp_Data    depth=2
    static stream<AppMeta>      sESfToTXp_SessId  ("sESfToTXp_SessId");
    #pragma HLS STREAM variable=sESfToTXp_SessId  depth=2

    //-- PROCESS FUNCTIONS ----------------------------------------------------
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
    //#pragma HLS dependence variable=_____ inter false
    //-------------------------------------------------------------------------
    pRXPath(
            piSHL_MmioEchoCtrl,
            siSHL_Data,
            siSHL_SessId,
            sRXpToTXp_Data,
            sRXpToTXp_SessId,
            sRXpToESf_Data,
            sRXpToESf_SessId);

    pEchoStoreAndForward(
            sRXpToESf_Data,
            sRXpToESf_SessId,
            sESfToTXp_Data,
            sESfToTXp_SessId);

    pTXPath(
            piSHL_MmioEchoCtrl,
            piSHL_MmioPostSegEn,
            sRXpToTXp_Data,
            sRXpToTXp_SessId,
            sESfToTXp_Data,
            sESfToTXp_SessId,
            soSHL_Data,
            soSHL_SessId);

}

