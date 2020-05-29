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
 * @file       : udp_shell_if.cpp
 * @brief      : UDP Shell Interface (USIF)
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE
 * Language    : Vivado HLS
 *
 *------------------------------------------------------------------------------
 *
 * @details This process handles the control flow interface between the SHELL
 *   and the ROLE. The main purpose of the USIF is to provide a placeholder for
 *   the opening of one or multiple listening port(s).
 *   The use of such a dedicated layer is not a prerequisite but it is provided
 *   here for sake of clarity and simplicity.
 *
 *          +-------+  +--------------------------------+
 *          |       |  |  +------+     +-------------+  |
 *          |       <-----+      <-----+     UDP     |  |
 *          | SHELL |  |  | USIF |     |             |  |
 *          |       +----->      +-----> APPLICATION |  |
 *          |       |  |  +------+     +-------------+  |
 *          +-------+  +--------------------------------+
 *
 * [TODO] - The DEFAULT_FPGA_LSN_PORT (0x2263=8803) and the DEFAULT_HOST_IP4_ADDR must be made programmable.
 *
 * \ingroup ROL
 * \addtogroup ROL_USIF
 * \{
 *******************************************************************************/

#include "udp_shell_if.hpp"

using namespace hls;
using namespace std;

/************************************************
 * INTERFACE SYNTHESIS DIRECTIVES
 *  For the time being, we continue designing
 *  with the DEPRECATED directives because the
 *  new PRAGMAs do not work for us.
 ************************************************/
#define USE_DEPRECATED_DIRECTIVES

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "USIF"  // UdpShellInterface

#define TRACE_OFF  0x0000
#define TRACE_RDP 1 <<  1
#define TRACE_WRP 1 <<  2
#define TRACE_SAM 1 <<  3
#define TRACE_LSN 1 <<  4
#define TRACE_CLS 1 <<  5
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

enum DropCmd {KEEP_CMD=false, DROP_CMD};

//---------------------------------------------------------
//-- DEFAULT LOCAL FPGA AND FOREIGN HOST SOCKETS
//--  By default, the following sockets will be used by the
//--  UDP Role Interface, unless the user specifies new ones
//--  via TBD.
//--  FYI --> 8803 is the ZIP code of Ruschlikon ;-)
//---------------------------------------------------------
#define DEFAULT_FPGA_LSN_PORT   0x2263      // ROLE   listens on port = 8803
#define DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // HOST's IP Address      = 10.12.200.50


/*******************************************************************************
 * @brief Request the SHELL/NTS/UOE to start listening on a specific port.
 *
 * @param[in]  piSHL_Enable  Enable signal from [SHELL].
 * @param[out] soSHL_LsnReq  Request to listen on a port to [SHELL].
 * @param[in]  siSHL_LsnRep  Listen reply from [SHELL].
 *******************************************************************************/
void pListen(
        CmdBit              *piSHL_Enable,
        stream<UdpPort>      &soSHL_LsnReq,
        stream<StsBool>      &siSHL_LsnRep)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName = concat3(THIS_NAME, "/", "LSn");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { LSN_IDLE, LSN_SEND_REQ, LSN_WAIT_ACK, LSN_DONE } \
                               lsn_fsmState=LSN_IDLE;
    #pragma HLS reset variable=lsn_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<8>  lsn_watchDogTimer;

    switch (lsn_fsmState) {
    case LSN_IDLE:
        if (*piSHL_Enable != 1) {
            return;
        }
        else
            lsn_fsmState = LSN_SEND_REQ;
        break;

    case LSN_SEND_REQ:
        if (!soSHL_LsnReq.full()) {
            UdpPort  udpListenPort = DEFAULT_FPGA_LSN_PORT;
            soSHL_LsnReq.write(udpListenPort);
            if (DEBUG_LEVEL & TRACE_LSN) {
                printInfo(myName, "SHELL/NTS/USIF is requested to listen on port #%d (0x%4.4X).\n",
                          DEFAULT_FPGA_LSN_PORT, DEFAULT_FPGA_LSN_PORT);
            #ifndef __SYNTHESIS__
                lsn_watchDogTimer = 10;
            #else
                lsn_watchDogTimer = 100;
            #endif
            lsn_fsmState = LSN_WAIT_ACK;
            }
        }
        else {
            printWarn(myName, "Cannot send a listen port request to [UOE] because stream is full!\n");
        }
        break;

    case LSN_WAIT_ACK:
        lsn_watchDogTimer--;
        if (!siSHL_LsnRep.empty()) {
            StsBool listenDone;
            siSHL_LsnRep.read(listenDone);
            if (listenDone) {
                printInfo(myName, "Received listen acknowledgment from [UOE].\n");
                lsn_fsmState = LSN_DONE;
            }
            else {
                printWarn(myName, "UOE denied listening on port %d (0x%4.4X).\n",
                          DEFAULT_FPGA_LSN_PORT, DEFAULT_FPGA_LSN_PORT);
                lsn_fsmState = LSN_SEND_REQ;
            }
        }
        else {
            if (lsn_watchDogTimer == 0) {
                printError(myName, "Timeout: Server failed to listen on port %d %d (0x%4.4X).\n",
                           DEFAULT_FPGA_LSN_PORT, DEFAULT_FPGA_LSN_PORT);
                lsn_fsmState = LSN_SEND_REQ;
            }
        }
        break;

    case LSN_DONE:
        break;
    }
}  // End-of: pListen()

/*******************************************************************************
 * @brief Request the SHELL/NTS/UOE to close a previously opened port.
 *
 * @param[in]  piSHL_Enable  Enable signal from [SHELL].
 * @param[out] soSHL_ClsReq  Close port request to [SHELL].
 * @param[in]  siSHL_ClsRep  Close port reply from [SHELL].
 *******************************************************************************/
void pClose(
        CmdBit              *piSHL_Enable,
        stream<UdpPort>      &soSHL_ClsReq,
        stream<StsBool>      &siSHL_ClsRep)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName = concat3(THIS_NAME, "/", "CLs");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { CLS_IDLE, CLS_SEND_REQ, CLS_WAIT_REP, CLS_DONE } \
                               cls_fsmState=CLS_IDLE;
    #pragma HLS reset variable=cls_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------

    switch (cls_fsmState) {
    case CLS_IDLE:
        if (*piSHL_Enable != 1) {
            if (!siSHL_ClsRep.empty()) {
                // Drain any potential status data
                siSHL_ClsRep.read();
                printWarn(myName, "Draining unexpected residue from the \'ClsRep\' stream.\n");
            }
            return;
        }
        else {
            cls_fsmState = CLS_SEND_REQ;
        }
        break;
    case CLS_SEND_REQ:
        if (!soSHL_ClsReq.full()) {
            // [FIXME-Must take the port # from a CfgReg] - DEFAULT_FPGA_LSN_PORT;
            // In the mean time, close a fake port to avoid the logic to be synthesized away.
            UdpPort  udpClosePort = 0xDEAD;
            soSHL_ClsReq.write(udpClosePort);
            if (DEBUG_LEVEL & TRACE_CLS) {
                printInfo(myName, "SHELL/NTS/USIF is requesting to close port #%d (0x%4.4X).\n",
                          udpClosePort.to_int(), udpClosePort.to_int());
            }
            cls_fsmState = CLS_WAIT_REP;
        }
        else {
            printWarn(myName, "Cannot send a listen port request to [UOE] because stream is full!\n");
        }
        break;
    case CLS_WAIT_REP:
        if (!siSHL_ClsRep.empty()) {
            StsBool isOpened;
            siSHL_ClsRep.read(isOpened);
            if (not isOpened) {
                printInfo(myName, "Received close acknowledgment from [UOE].\n");
                cls_fsmState = CLS_DONE;
            }
            else {
                printWarn(myName, "UOE denied closing the port %d (0x%4.4X) which is still opened.\n",
                          DEFAULT_FPGA_LSN_PORT, DEFAULT_FPGA_LSN_PORT);
                cls_fsmState = CLS_SEND_REQ;
            }
        }
        break;
    case CLS_DONE:
        break;
    }
}  // End-of: pClose()


/*******************************************************************************
 * @brief Read Path (RDp) - From SHELL/UOE to ROLE/UAF.
 *  Process waits for a new datagram to read and forwards it to the UAF.
 *
 * @param[in]  siSHL_Data   Datagram from [SHELL].
 * @param[in]  siSHL_Meta   Metadata from [SHELL].
 * @param[out] soUAF_Data   Datagram to [UAF].
 * @param[out] soUAF_Meta   Metadata to [UAF].
 *
 *******************************************************************************/
void pReadPath(
        stream<UdpAppData>  &siSHL_Data,
        stream<UdpAppMeta>  &siSHL_Meta,
        stream<UdpAppData>  &soUAF_Data,
        stream<UdpAppMeta>  &soUAF_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "RDp");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { RDP_WAIT_META=0, RDP_STREAM } \
	                           rdp_fsmState = RDP_WAIT_META;
    #pragma HLS reset variable=rdp_fsmState

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    UdpAppData  appData;
    UdpAppMeta  appMeta;

    switch (rdp_fsmState ) {
    case RDP_WAIT_META:
        if (!siSHL_Meta.empty() and !soUAF_Meta.full()) {
            siSHL_Meta.read(appMeta);
            soUAF_Meta.write(appMeta);
            rdp_fsmState  = RDP_STREAM;
        }
        break;
    case RDP_STREAM:
        if (!siSHL_Data.empty() && !soUAF_Data.full()) {
            siSHL_Data.read(appData);
            if (DEBUG_LEVEL & TRACE_RDP) {
                 printAxisRaw(myName, "Received data chunk from SHELL: ", appData);
            }
            soUAF_Data.write(appData);
            if (appData.getLE_TLast())
                rdp_fsmState  = RDP_WAIT_META;
        }
        break;
    }
}  // End-of: pReadPath()

/*******************************************************************************
 * @brief Write Path (WRp) - From ROLE/UAF to SHELL/NTS/UOE.
 *  Process waits for a new datagram to write and forwards it to SHELL.
 *
 * @param[in]  siUAF_Data   UDP datagram from [ROLE/UAF].
 * @param[in]  siUAF_Meta   UDP metadata from [ROLE/UAF].
 * @Param[in]  siUAF_DLen   UDP data len from [ROLE/UAF].
 * @param[out] soSHL_Data   UDP datagram to [SHELL].
 * @param[out] soSHL_Meta   UDP metadata to [SHELL].
 * @param[in]  soSHL_DLen   UDP data len to [SHELL].
 *
 *******************************************************************************/
void pWritePath(
        stream<UdpAppData>   &siUAF_Data,
        stream<UdpAppMeta>   &siUAF_Meta,
        stream<UdpAppDLen>   &siUAF_DLen,
        stream<UdpAppData>   &soSHL_Data,
        stream<UdpAppMeta>   &soSHL_Meta,
        stream<UdpAppDLen>   &soSHL_DLen)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "WRp");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { WRP_WAIT_META=0, WRP_STREAM } \
                               wrp_fsmState = WRP_WAIT_META;
    #pragma HLS reset variable=wrp_fsmState

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    UdpAppMeta    appMeta;
    UdpAppDLen    appDLen;
    UdpAppData    appData;

    switch (wrp_fsmState) {
    case WRP_WAIT_META:
        //-- Always read the metadata and the length provided by [ROLE/UAF]
        if (!siUAF_Meta.empty() and !siUAF_DLen.empty() and
            !soSHL_Meta.full()  and !soSHL_DLen.full()) {
            siUAF_Meta.read(appMeta);
            siUAF_DLen.read(appDLen);
            soSHL_Meta.write(appMeta);
            soSHL_DLen.write(appDLen);
            if (DEBUG_LEVEL & TRACE_WRP) {
                printInfo(myName, "Received a new datagram of length %d from ROLE.\n", appDLen.to_uint());
                printSockPair(myName, appMeta);
            }
            wrp_fsmState = WRP_STREAM;
        }
        break;
    case WRP_STREAM:
        if (!siUAF_Data.empty() && !soSHL_Data.full()) {
            siUAF_Data.read(appData);
            if (DEBUG_LEVEL & TRACE_WRP) {
                 printAxisRaw(myName, "Received data chunk from ROLE: ", appData);
            }
            soSHL_Data.write(appData);
            if(appData.getTLast())
                wrp_fsmState = WRP_WAIT_META;
        }
        break;
    }
}  // End-of: pWritePath()


/*****************************************************************************
 * @brief   Main process of the UDP Shell Interface (USIF)
 *
 * @param[in]  piSHL_Mmio_En Enable signal from [SHELL/MMIO].
 * @param[out] soSHL_LsnReq  Listen port request to [SHELL].
 * @param[in]  siSHL_LsnRep  Listen port reply from [SHELL].
 * @param[out] soSHL_ClsReq  Close port request to [SHELL].
 * @param[in]  siSHL_ClsRep  Close port reply from [SHELL].
 * @param[in]  siSHL_Data    UDP datagram from [SHELL].
 * @param[in]  siSHL_Meta    UDP metadata from [SHELL].
 * @param[out] soSHL_Data    UDP datagram to [SHELL].
 * @param[out] soSHL_Meta    UDP metadata to [SHELL].
 * @param[out] soSHL_DLen    UDP data len to [SHELL].
 * @param[in]  siUAF_Data    UDP datagram from UdpAppFlash (UAF).
 * @param[in]  siUAF_Meta    UDP metadata from [UAF].
 * @param[out] soUAF_Data    UDP datagram to [UAF].
 * @param[out] soUAF_Meta    UDP metadata to [UAF].
 * @param[out] soUAF_DLen    UDP data len to [UAF].
 *
 *****************************************************************************/
void udp_shell_if(

        //------------------------------------------------------
        //-- SHELL / Mmio Interface
        //------------------------------------------------------
        CmdBit              *piSHL_Mmio_En,

       //------------------------------------------------------
        //-- SHELL / Control Port Interfaces
        //------------------------------------------------------
        stream<UdpPort>     &soSHL_LsnReq,
        stream<StsBool>     &siSHL_LsnRep,
        stream<UdpPort>     &soSHL_ClsReq,
        stream<StsBool>     &siSHL_ClsRep,

        //------------------------------------------------------
        //-- SHELL / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &siSHL_Data,
        stream<UdpAppMeta>  &siSHL_Meta,

        //------------------------------------------------------
        //-- SHELL / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &soSHL_Data,
        stream<UdpAppMeta>  &soSHL_Meta,
        stream<UdpAppDLen>  &soSHL_DLen,

        //------------------------------------------------------
        //-- UAF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &siUAF_Data,
        stream<UdpAppMeta>  &siUAF_Meta,
        stream<UdpAppDLen>  &siUAF_DLen,

        //------------------------------------------------------
        //-- UAF / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &soUAF_Data,
        stream<UdpAppMeta>  &soUAF_Meta)
{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

#if defined(USE_DEPRECATED_DIRECTIVES)

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

    #pragma HLS INTERFACE ap_stable          port=piSHL_Mmio_En name=piSHL_Mmio_En

    #pragma HLS resource core=AXI4Stream variable=soSHL_LsnReq  metadata="-bus_bundle soSHL_LsnReq"
    #pragma HLS resource core=AXI4Stream variable=siSHL_LsnRep  metadata="-bus_bundle siSHL_LsnRep"
    #pragma HLS resource core=AXI4Stream variable=soSHL_ClsReq  metadata="-bus_bundle soSHL_ClsReq"
    #pragma HLS resource core=AXI4Stream variable=siSHL_ClsRep  metadata="-bus_bundle siSHL_ClsRep"

    #pragma HLS resource core=AXI4Stream variable=siSHL_Data    metadata="-bus_bundle siSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=siSHL_Meta    metadata="-bus_bundle siSHL_Meta"
    #pragma HLS DATA_PACK                variable=siSHL_Meta

    #pragma HLS resource core=AXI4Stream variable=soSHL_Data    metadata="-bus_bundle soSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=soSHL_Meta    metadata="-bus_bundle soSHL_Meta"
    #pragma HLS DATA_PACK                variable=soSHL_Meta
    #pragma HLS resource core=AXI4Stream variable=soSHL_DLen    metadata="-bus_bundle soSHL_DLen"

    #pragma HLS resource core=AXI4Stream variable=siUAF_Data    metadata="-bus_bundle siUAF_Data"
    #pragma HLS resource core=AXI4Stream variable=siUAF_Meta    metadata="-bus_bundle siUAF_Meta"
    #pragma HLS DATA_PACK                variable=siUAF_Meta
    #pragma HLS resource core=AXI4Stream variable=siUAF_DLen    metadata="-bus_bundle siUAF_DLen"

    #pragma HLS resource core=AXI4Stream variable=soUAF_Data    metadata="-bus_bundle soUAF_Data"
    #pragma HLS resource core=AXI4Stream variable=soUAF_Meta    metadata="-bus_bundle soUAF_Meta"
    #pragma HLS DATA_PACK                variable=soUAF_Meta

#else

    #pragma HLS INTERFACE ap_stable          port=piSHL_Mmio_En  name=piSHL_Mmio_En

    #pragma HLS INTERFACE axis register both port=soSHL_LsnReq   name=soSHL_LsnReq
    #pragma HLS INTERFACE axis register both port=siSHL_LsnRep   name=siSHL_LsnRep
    #pragma HLS INTERFACE axis register both port=soSHL_ClsReq   name=soSHL_ClsReq
    #pragma HLS INTERFACE axis register both port=siSHL_ClsRep   name=siSHL_ClsRep

    #pragma HLS INTERFACE axis register both port=siSHL_Data     name=siSHL_Data
    #pragma HLS INTERFACE axis register both port=siSHL_Meta     name=siSHL_Meta
    #pragma HLS DATA_PACK                variable=siSHL_Meta

    #pragma HLS INTERFACE axis register both port=soSHL_Data     name=soSHL_Data
    #pragma HLS INTERFACE axis register both port=soSHL_Meta     name=soSHL_Meta
    #pragma HLS DATA_PACK                variable=soSHL_Meta
    #pragma HLS INTERFACE axis register both port=soSHL_DLen     name=soSHL_DLen

    #pragma HLS INTERFACE axis register both port=siUAF_Data     name=siUAF_Data
    #pragma HLS INTERFACE axis register both port=siUAF_Meta     name=siUAF_Meta
    #pragma HLS DATA_PACK                variable=siUAF_Meta
    #pragma HLS INTERFACE axis register both port=siUAF_DLen     name=siUAF_DLen

    #pragma HLS INTERFACE axis register both port=soUAF_Data     name=soUAF_Data
    #pragma HLS INTERFACE axis register both port=soUAF_Meta     name=soUAF_Meta
    #pragma HLS DATA_PACK                variable=soUAF_Meta

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW  interval=1

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- PROCESS FUNCTIONS ----------------------------------------------------
    pListen(
            piSHL_Mmio_En,
            soSHL_LsnReq,
            siSHL_LsnRep);

    pClose(
            piSHL_Mmio_En,
            soSHL_ClsReq,
            siSHL_ClsRep);

    pReadPath(
            siSHL_Data,
            siSHL_Meta,
            soUAF_Data,
            soUAF_Meta);

    pWritePath(
            siUAF_Data,
            siUAF_Meta,
            siUAF_DLen,
            soSHL_Data,
            soSHL_Meta,
            soSHL_DLen);

}

/*! \} */
