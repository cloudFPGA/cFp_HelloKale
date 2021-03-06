/*
 * Copyright 2016 -- 2021 IBM Corporation
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
 * Component : cFp_HelloKale/ROLE/TCP Application Flash (TAF)
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
 * \ingroup tcp_app_flash 
 * \addtogroup tcp_app_flash 
 * \{
 *******************************************************************************/

#include "tcp_app_flash.hpp"

using namespace hls;
using namespace std;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_RXP | TRACE_TXP)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TAF"  // TcpApplicationFlash

#define TRACE_OFF  0x0000
#define TRACE_ESF 1 <<  1  // EchoStoreForward
#define TRACE_RXP 1 <<  2  // RxPath
#define TRACE_TXP 1 <<  3  // TxPath
#define TRACE_ALL  0xFFFF
#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * @brief Echo Store and Forward (ESf)
 *
 * @param[in]  siRXp_Data   Data stream from pTcpRxPath (RXp).
 * @param[in]  siRXp_SessId TCP session-id from [RXp].
 * @param[in]  siRXp_DatLen TCP data-length from [RXp].
 * @param[out] soTXp_Data   Data stream to pTcpRxPath (TXp).
 * @param[out] soTXp_SessId TCP session-id to [TXp].
 * @param[out] soTXp_DatLen TCP data-length to [TXp].
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
        stream<TcpSessId>    &siRXp_SessId,
        stream<TcpDatLen>    &siRXp_DatLen,
        stream<TcpAppData>   &soTXp_Data,
        stream<TcpSessId>    &soTXp_SessId,
        stream<TcpDatLen>    &soTXp_DatLen)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "ESf");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { ESF_META=0, ESF_STREAM } \
                               esf_fsmState = ESF_META;
    #pragma HLS reset variable=esf_fsmState
    static  Ly4Len             esf_byteCnt;
    #pragma HLS reset variable=esf_byteCnt

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisApp     currChunk;
    TcpAppMeta  tcpSessId;

    //=====================================================
    //== PROCESS DATA FORWARDING
    //=====================================================
    if ( !siRXp_Data.empty() and !soTXp_Data.full() ) {
        TcpAppData appData = siRXp_Data.read();
        soTXp_Data.write(appData);
        esf_byteCnt += appData.getLen();
        if (appData.getTLast()) {
            esf_byteCnt = 0;
        }
    }

    //=====================================================
    //== PROCESS META FORWARDING
    //=====================================================
    if ( !siRXp_SessId.empty() and !soTXp_SessId.full() and
         !siRXp_DatLen.empty() and !soTXp_DatLen.full() ) {
        TcpSessId sessId = siRXp_SessId.read();
        TcpDatLen datLen = siRXp_DatLen.read();
        soTXp_SessId.write(sessId);
        soTXp_DatLen.write(datLen);
    }

} // End of: pTcpEchoStoreAndForward()

/*******************************************************************************
 * @brief Transmit Path - From THIS to TSIF
 *
 * @param[in]  piSHL_MmioEchoCtrl  Configuration of the echo function.
 * @param[in]  siEPt_Data          Data from EchoPassTrough (EPt).
 * @param[in]  siEPt_SessId        TCP session-id from [EPt].
 * @param[in]  siEPt_DatLen        TCP data-length from [EPt].
 * @param[in]  siESf_Data          Data from EchoStoreAndForward (ESf).
 * @param[in]  siESf_SessId        TCP session-id from [ESf].
 * @param[in]  siESf_DatLen        TCP data-length from [ESf].
 * @param[out] soTSIF_Data         Data to SHELL (SHL).
 * @param[out] soTSIF_SessId       TCP session-id to [SHL].
 * @param[out] soTSIF_DatLen       TCP data-length to [SHL].
 *******************************************************************************/
void pTcpTxPath(
    #if defined TAF_USE_NON_FIFO_IO
        ap_uint<2>  piSHL_MmioEchoCtrl,
    #endif
        stream<TcpAppData>  &siEPt_Data,
        stream<TcpSessId>   &siEPt_SessId,
        stream<TcpDatLen>   &siEPt_DatLen,
        stream<TcpAppData>  &siESf_Data,
        stream<TcpSessId>   &siESf_SessId,
        stream<TcpDatLen>   &siESf_DatLen,
        stream<TcpAppData>  &soTSIF_Data,
        stream<TcpSessId>   &soTSIF_SessId,
        stream<TcpDatLen>   &soTSIF_DatLen)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "TXp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { TXP_START_OF_STREAM=0, TXP_CONTINUATION_OF_STREAM } \
                               txp_fsmState=TXP_START_OF_STREAM;
    #pragma HLS RESET variable=txp_fsmState
    static EchoCtrl            txp_EchoCtrl=ECHO_PATH_THRU;
    #pragma HLS RESET variable=txp_EchoCtrl

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static ap_uint<16> txp_msgGapSize;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TcpAppData     appData;
    TcpAppMeta     tcpSessId;

  #if defined TAF_USE_NON_FIFO_IO
    switch (txp_fsmState ) {
    case TXP_START_OF_STREAM:
        switch(piSHL_MmioEchoCtrl) {
        case ECHO_PATH_THRU:
            // Read session Id from pEchoPassThrough and forward to [SHL]
            if ( !siEPt_SessId.empty() and !soTSIF_SessId.full() and
                 !siEPt_DatLen.empty() and !soTSIF_DatLen.full() ) {
                TcpSessId sessId;
                TcpDatLen datLen;
                siEPt_SessId.read(sessId);
                siEPt_DatLen.read(datLen);
                soTSIF_SessId.write(sessId);
                soTSIF_DatLen.write(datLen);
                txp_fsmState = TXP_CONTINUATION_OF_STREAM;
            }
            break;
        case ECHO_STORE_FWD:
            //-- Read session Id from pTcpEchoStoreAndForward and forward to [SHL]
            if ( !siESf_SessId.empty() and !soTSIF_SessId.full() and
                 !siESf_DatLen.empty() and !soTSIF_DatLen.full() ) {
                TcpSessId sessId;
                TcpDatLen datLen;
                siESf_SessId.read(sessId);
                siESf_DatLen.read(datLen);
                soTSIF_SessId.write(sessId);
                soTSIF_DatLen.write(datLen);
                txp_fsmState = TXP_CONTINUATION_OF_STREAM;
            }
            break;
        case ECHO_OFF:
        default:
            // Always drain and drop the session Id (if any)
            if ( !siEPt_SessId.empty() ) {
                siEPt_SessId.read();
            }
            else if ( !siESf_SessId.empty() ) {
                siESf_SessId.read();
            }
            // Always drain and drop the data-length (if any)
            if ( !siEPt_DatLen.empty() ) {
                siEPt_DatLen.read();
            }
            else if ( !siESf_DatLen.empty() ) {
                siESf_DatLen.read();
            }
            txp_fsmState = TXP_CONTINUATION_OF_STREAM;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
    case TXP_CONTINUATION_OF_STREAM:
        switch(piSHL_MmioEchoCtrl) {
        case ECHO_PATH_THRU:
            //-- Read incoming data from pEchoPathThrough and forward to [SHL]
            if (!siEPt_Data.empty() and !soTSIF_Data.full()) {
                siEPt_Data.read(appData);
                soTSIF_Data.write(appData);
                // Update FSM state
               if (appData.getTLast()) {
                   txp_fsmState = TXP_START_OF_STREAM;
               }
            }
            break;
        case ECHO_STORE_FWD:
            //-- Read incoming data from pTcpEchoStoreAndForward and forward to [SHL]
            if ( !siESf_Data.empty() and !soTSIF_Data.full()) {
                siESf_Data.read(appData);
                soTSIF_Data.write(appData);
                // Update FSM state
                if (appData.getTLast()) {
                    txp_fsmState = TXP_START_OF_STREAM;
                }
            }
            break;
        case ECHO_OFF:
        default:
            // Always drain and drop the data streams (if any)
            if ( !siEPt_Data.empty() ) {
                siEPt_Data.read(appData);
            }
            else if ( !siESf_Data.empty() ) {
                siESf_Data.read(appData);
            }
            // Always alternate between START and CONTINUATION to drain all streams
            txp_fsmState = TXP_START_OF_STREAM;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
    }  // End-of: switch (txp_fsmState ) {
  #else
    switch (txp_fsmState ) {
    case TXP_START_OF_STREAM:
        if (!siEPt_SessId.empty() and !siEPt_DatLen.empty() and
            !soTSIF_SessId.full() and !soTSIF_DatLen.full()) {
            soTSIF_SessId.write(siEPt_SessId.read());
            soTSIF_DatLen.write(siEPt_DatLen.read());
            txp_EchoCtrl = ECHO_PATH_THRU;
            txp_fsmState = TXP_CONTINUATION_OF_STREAM;
        }
        else if (!siESf_SessId.empty() and !siESf_DatLen.empty() and
                 !soTSIF_SessId.full() and !soTSIF_DatLen.full()) {
            soTSIF_SessId.write(siESf_SessId.read());
            soTSIF_DatLen.write(siESf_DatLen.read());
            txp_EchoCtrl = ECHO_STORE_FWD;
            txp_fsmState = TXP_CONTINUATION_OF_STREAM;
        }
        break;
    case TXP_CONTINUATION_OF_STREAM:
        if (txp_EchoCtrl == ECHO_PATH_THRU) {
           if(!siEPt_Data.empty() and !soTSIF_Data.full()) {
               siEPt_Data.read(appData);
               soTSIF_Data.write(appData);
               if (appData.getTLast()) {
                   txp_fsmState = TXP_START_OF_STREAM;
                   txp_EchoCtrl = ECHO_STORE_FWD; // Toggle
               }
            }
        }
        else {
            if(!siESf_Data.empty() and !soTSIF_Data.full()) {
                siESf_Data.read(appData);
                soTSIF_Data.write(appData);
                if (appData.getTLast()) {
                    txp_fsmState = TXP_START_OF_STREAM;
                    txp_EchoCtrl = ECHO_PATH_THRU; // Toggle
                }
             }
        }
        break;
    }  // End-of: switch (txp_fsmState ) {
  #endif

} // End of: pTcpTxPath()

/*******************************************************************************
 * @brief TCP Receive Path (RXp) - From SHELL->ROLE/TSIF to THIS.
 *
 * @param[in]  piSHL_MmioEchoCtrl Configuration of the echo function.
 * @param[in]  siTSIF_Data        Data segment from TcpShellInterface (TSIF).
 * @param[in]  siTSIF_SessId      TCP session-id from [TSIF].
 * @param[in]  siTSIF_DatLen      TCP data-length from [TSIF].
 * @param[out] soEPt_Data         Data segment to EchoPassTrough (EPt).
 * @param[out] soEPt_SessId       TCP session-id to [EPt].
 * @param[out] soEPt_DatLen       TCP data-length to [EPt].
 * @param[out] soESf_Data         Data segment to EchoStoreAndForward (ESf).
 * @param[out] soESF_SessId       TCP session-id to [ESf].
 * @param[out] soESF_DatLen       TCP data-length to [ESf].
 *
 * @details This Process waits for a new TCP data segment to read and forwards
 *   it to the EchoPathThrough (EPt) or the EchoStoreAndForward (ESf) process
 *   upon the setting of LSBit of the session-id (if sessId[0] --> EPt).
 *   (FYI-This function used to be performed by the 'piSHL_Mmio_EchoCtrl' bits).
 *******************************************************************************/
void pTcpRxPath(
    #if defined TAF_USE_NON_FIFO_IO
        ap_uint<2>            piSHL_MmioEchoCtrl,
    #endif
        stream<TcpAppData>   &siTSIF_Data,
        stream<TcpSessId>    &siTSIF_SessId,
        stream<TcpDatLen>    &siTSIF_DatLen,
        stream<TcpAppData>   &soEPt_Data,
        stream<TcpSessId>    &soEPt_SessId,
        stream<TcpDatLen>    &soEPt_DatLen,
        stream<TcpAppData>   &soESf_Data,
        stream<TcpSessId>    &soESf_SessId,
        stream<TcpDatLen>    &soESf_DatLen)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "RXp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { RXP_START_OF_STREAM=0, RXP_CONTINUATION_OF_STREAM } \
                               rxp_fsmState=RXP_START_OF_STREAM;
    #pragma HLS reset variable=rxp_fsmState
    static EchoCtrl            rxp_EchoCtrl=ECHO_PATH_THRU;
    #pragma HLS RESET variable=rxp_EchoCtrl

    //-- LOCAL VARIABLES -------------------------------------------------------
    TcpAppData        appData;
    TcpSessId         sessId;
    TcpDatLen         datLen;

  #if defined TAF_USE_NON_FIFO_IO
    switch(piSHL_MmioEchoCtrl) {
    case ECHO_PATH_THRU:
        switch (rxp_fsmState ) {
        case RXP_START_OF_STREAM:
            //-- Read incoming metadata and forward to pEchoPathThrough
            if ( !siTSIF_SessId.empty() and !soEPt_SessId.full() and
                 !siTSIF_DatLen.empty() and !soEPt_DatLen.full() ) {
                siTSIF_SessId.read(sessId);
                siTSIF_DatLen.read(datLen);
                soEPt_SessId.write(sessId);
                soEPt_DatLen.write(datLen);
                rxp_fsmState = RXP_CONTINUATION_OF_STREAM;
            }
            break;
        case RXP_CONTINUATION_OF_STREAM:
            //-- Read incoming data and forward to pEchoPathThrough
            if (!siTSIF_Data.empty() and !soEPt_Data.full()) {
                siTSIF_Data.read(appData);
                soEPt_Data.write(appData);
                // Update FSM state
                if (appData.getTLast()) {
                    rxp_fsmState = RXP_START_OF_STREAM;
                }
            }
            break;
        }
        break;
    case ECHO_STORE_FWD:
        switch (rxp_fsmState ) {
        case RXP_START_OF_STREAM:
            //-- Read incoming metadata and forward to pTcpEchoStoreAndForward
            if ( !siTSIF_SessId.empty() and !soESf_SessId.full() and
                 !siTSIF_DatLen.empty() and !soEPt_DatLen.full()) {
                soESf_SessId.write(siTSIF_SessId.read());
                soESf_DatLen.write(siTSIF_DatLen.read());
                rxp_fsmState = RXP_CONTINUATION_OF_STREAM;
            }
            break;
        case RXP_CONTINUATION_OF_STREAM:
            //-- Read incoming data and forward to pTcpEchoStoreAndForward
            if ( !siTSIF_Data.empty() and !soESf_Data.full()) {
                siTSIF_Data.read(appData);
                soESf_Data.write(appData);
                // Update FSM state
                if (appData.getTLast()) {
                    rxp_fsmState = RXP_START_OF_STREAM;
                }
            }
            break;
        }
        break;
    case ECHO_OFF:
    default:
        switch (rxp_fsmState ) {
        case RXP_START_OF_STREAM:
            // Drain and drop the TCP metadata
            if ( !siTSIF_SessId.empty() and !siTSIF_DatLen.empty()) {
                siTSIF_SessId.read();
                siTSIF_DatLen.read();
                rxp_fsmState = RXP_CONTINUATION_OF_STREAM;
            }
            break;
        case RXP_CONTINUATION_OF_STREAM:
            // Drain and drop the TCP data
            if ( !siTSIF_Data.empty() ) {
                siTSIF_Data.read();
            }
            // Always alternate between START and CONTINUATION to drain all streams
            rxp_fsmState = RXP_START_OF_STREAM;
            break;
        }
        break;
    }
    /*** [ALTERNATE-SWITCH-CASE-STRUCTURE] ****************
    switch (rxp_fsmState ) {
      case RXP_START_OF_STREAM:
        switch(piSHL_MmioEchoCtrl) {
          case ECHO_PATH_THRU:
            //-- Read incoming metadata and forward to pEchoPathThrough
            if ( !siTSIF_SessId.empty() and !soEPt_SessId.full() and
                 !siTSIF_DatLen.empty() and !soEPt_DatLen.full() ) {
                siTSIF_SessId.read(sessId);
                siTSIF_DatLen.read(datLenEpt);
                soEPt_SessId.write(sessId);
                soEPt_DatLen.write(datLenEpt);
                rxp_fsmState = RXP_CONTINUATION_OF_STREAM;
            }
            break;
          case ECHO_STORE_FWD:
            //-- Read incoming metadata and forward to pTcpEchoStoreAndForward
            if ( !siTSIF_SessId.empty() and !soESf_SessId.full() and
                 !siTSIF_DatLen.empty() and !soEPt_DatLen.full()) {
                soESf_SessId.write(siTSIF_SessId.read());
                soESf_DatLen.write(siTSIF_DatLen.read());
                rxp_fsmState = RXP_CONTINUATION_OF_STREAM;
            }
            break;
          case ECHO_OFF:
          default:
            // Drain and drop the TCP metadata
            if ( !siTSIF_SessId.empty() and !siTSIF_DatLen.empty()) {
                siTSIF_SessId.read();
                siTSIF_DatLen.read();
                rxp_fsmState = RXP_CONTINUATION_OF_STREAM;
            }
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
      case RXP_CONTINUATION_OF_STREAM:
        switch(piSHL_MmioEchoCtrl) {
          case ECHO_PATH_THRU:
            //-- Read incoming data and forward to pEchoPathThrough
            if (!siTSIF_Data.empty() and !soEPt_Data.full()) {
                siTSIF_Data.read(appData);
                soEPt_Data.write(appData);
                // Update FSM state
                if (appData.getTLast()) {
                    rxp_fsmState = RXP_START_OF_STREAM;
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
                    rxp_fsmState = RXP_START_OF_STREAM;
                }
            }
            break;
          case ECHO_OFF:
          default:
            // Drain and drop the incoming data stream
            if ( !siTSIF_Data.empty() ) {
                siTSIF_Data.read();
            }
            // Always alternate between START and CONTINUATION to drain all streams
            rxp_fsmState = RXP_START_OF_STREAM;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
    }  // End-of: switch (rxpFsmState ) {
    *** [ALTERNATE-SWITCH-CASE-STRUCTURE] *****************/
  #else
    switch (rxp_fsmState ) {
    case RXP_START_OF_STREAM:
        if (!siTSIF_SessId.empty() and !siTSIF_DatLen.empty() and
            !soEPt_SessId.full()   and !soEPt_DatLen.full()   and
            !soESf_SessId.full()   and !soESf_DatLen.full()) {
            siTSIF_SessId.read(sessId);
            siTSIF_DatLen.read(datLen);
            if (sessId.get_bit(0)) {
                soEPt_SessId.write(sessId);
                soEPt_DatLen.write(datLen);
                rxp_EchoCtrl = ECHO_PATH_THRU;
                if (DEBUG_LEVEL & TRACE_RXP) {
                    printInfo(myName, "SessId=%d --> Forwarding segment in ECHO_PATH_THRU mode.\n", sessId.to_ushort());
                }
            }
            else {
                // Forward to pEchoStoreAndForward
                soESf_SessId.write(sessId);
                soESf_DatLen.write(datLen);
                rxp_EchoCtrl = ECHO_STORE_FWD;
                if (DEBUG_LEVEL & TRACE_RXP) {
                    printInfo(myName, "SessId=%d --> Forwarding segment in ECHO_STORE_FWD mode.\n", sessId.to_ushort());
                }
            }
            rxp_fsmState = RXP_CONTINUATION_OF_STREAM;
        }
        break;
    case RXP_CONTINUATION_OF_STREAM:
        if (!siTSIF_Data.empty()) {
            if ((rxp_EchoCtrl == ECHO_PATH_THRU) and !soEPt_Data.full()) {
                siTSIF_Data.read(appData);
                soEPt_Data.write(appData);
                if (appData.getTLast()) {
                   rxp_fsmState = RXP_START_OF_STREAM;
                }
            }
            else if ((rxp_EchoCtrl == ECHO_STORE_FWD) and !soESf_Data.full()) {
                siTSIF_Data.read(appData);
                soESf_Data.write(appData);
                if (appData.getTLast()) {
                   rxp_fsmState = RXP_START_OF_STREAM;
                }
            }
        }
        break;
    }
  #endif

} // End of: pTcpRxPath()

/*******************************************************************************
 * @brief   Main process of the TCP Application Flash (TAF)
 *
 * @param[in]  piSHL_MmioEchoCtrl  Configures the echo function.
 * @param[in]  siTSIF_Data         TCP data stream from the SHELL [SHL].
 * @param[in]  siTSIF_SessId       TCP session-id from [SHL].
 * @param[in]  siTSIF_DataLen      TCP data-length from [SHL].
 * @param[out] soTSIF_Data         TCP data stream to [SHL].
 * @param[out] soTSIF_SessId       TCP session-id to [SHL].
 * @param[out  soTSIF_DatLen       TCP data-length to [SHL].
 *
 *******************************************************************************/
void tcp_app_flash (

        //------------------------------------------------------
        //-- SHELL / MMIO / Configuration Interfaces
        //------------------------------------------------------
    #if defined TAF_USE_NON_FIFO_IO
        ap_uint<2>           piSHL_MmioEchoCtrl,
    #endif
        //------------------------------------------------------
        //-- SHELL / TCP Rx Data Interface
        //------------------------------------------------------
        stream<TcpAppData>  &siTSIF_Data,
        stream<TcpSessId>   &siTSIF_SessId,
        stream<TcpDatLen>   &siTSIF_DataLen,
        //------------------------------------------------------
        //-- SHELL / TCP Tx Data Interface
        //------------------------------------------------------
        stream<TcpAppData>  &soTSIF_Data,
        stream<TcpSessId>   &soTSIF_SessId,
        stream<TcpDatLen>   &soTSIF_DatLen)
{

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INLINE
    #pragma HLS INTERFACE ap_ctrl_none port=return

  #if defined TAF_USE_NON_FIFO_IO
    #pragma HLS STABLE variable=piSHL_MmioEchoCtrl
  #endif

    //--------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    //-- Rx Path (RXp) ---------------------------------------------------------
    static stream<TcpAppData>   ssRXpToTXp_Data   ("ssRXpToTXp_Data");
    #pragma HLS STREAM variable=ssRXpToTXp_Data   depth=1024
    static stream<TcpSessId>    ssRXpToTXp_SessId ("ssRXpToTXp_SessId");
    #pragma HLS STREAM variable=ssRXpToTXp_SessId depth=64
    static stream<TcpDatLen>    ssRXpToTXp_DatLen ("ssRXpToTXp_DatLen");
    #pragma HLS STREAM variable=ssRXpToTXp_DatLen depth=64

    static stream<TcpAppData>   ssRXpToESf_Data   ("ssRXpToESf_Data");
    #pragma HLS STREAM variable=ssRXpToESf_Data   depth=2048
    static stream<TcpSessId>    ssRXpToESf_SessId ("ssRXpToESf_SessId");
    #pragma HLS STREAM variable=ssRXpToESf_SessId depth=32
    static stream<TcpDatLen>    ssRXpToESf_DatLen ("ssRXpToESf_DatLen");
    #pragma HLS STREAM variable=ssRXpToESf_DatLen depth=32

    //-- Echo Store and Forward (ESf) ------------------------------------------
    static stream<TcpAppData>   ssESfToTXp_Data   ("ssESfToTXp_Data");
    #pragma HLS STREAM variable=ssESfToTXp_Data   depth=1024
    static stream<TcpSessId>    ssESfToTXp_SessId ("ssESfToTXp_SessId");
    #pragma HLS STREAM variable=ssESfToTXp_SessId depth=32
    static stream<TcpDatLen>    ssESfToTXp_DatLen ("ssESfToTXp_DatLen");
    #pragma HLS STREAM variable=ssESfToTXp_DatLen depth=32

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
        #if defined TAF_USE_NON_FIFO_IO
            piSHL_MmioEchoCtrl,
        #endif
            siTSIF_Data,
            siTSIF_SessId,
            siTSIF_DataLen,
            ssRXpToTXp_Data,
            ssRXpToTXp_SessId,
            ssRXpToTXp_DatLen,
            ssRXpToESf_Data,
            ssRXpToESf_SessId,
            ssRXpToESf_DatLen);

    pTcpEchoStoreAndForward(
            ssRXpToESf_Data,
            ssRXpToESf_SessId,
            ssRXpToESf_DatLen,
            ssESfToTXp_Data,
            ssESfToTXp_SessId,
            ssESfToTXp_DatLen);

    pTcpTxPath(
        #if defined TAF_USE_NON_FIFO_IO
            piSHL_MmioEchoCtrl,
        #endif
            ssRXpToTXp_Data,
            ssRXpToTXp_SessId,
            ssRXpToTXp_DatLen,
            ssESfToTXp_Data,
            ssESfToTXp_SessId,
            ssESfToTXp_DatLen,
            soTSIF_Data,
            soTSIF_SessId,
            soTSIF_DatLen);

}

/*! \} */
