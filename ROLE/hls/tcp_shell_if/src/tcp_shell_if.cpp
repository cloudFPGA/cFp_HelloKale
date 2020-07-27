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
 * @file       : tcp_shell_if.cpp
 * @brief      : TCP Shell Interface (TSIF)
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp / ROLE
 * Language    : Vivado HLS
 *
 *------------------------------------------------------------------------------
 *
 * @details    : This entity handles the control flow interface between the
 *  the SHELL and the ROLE. The main purpose of the TSIF is to open a predefined
 *  set of ports in listening and/or to actively connect to remote host(s).
 *   The use of a dedicated layer is not a prerequisite but it is provided here
 *   for sake of simplicity.
 *
 *          +-------+  +--------------------------------+
 *          |       |  |  +------+     +-------------+  |
 *          |       <-----+      <-----+     TCP     |  |
 *          | SHELL |  |  | TSIF |     |             |  |
 *          |       +----->      +-----> APPLICATION |  |
 *          |       |  |  +------+     +-------------+  |
 *          +-------+  +--------------------------------+
 *
 * \ingroup ROLE
 * \addtogroup ROLE_TSIF
 * \{
 *******************************************************************************/

#include "tcp_shell_if.hpp"

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

#define THIS_NAME "TSIF"  // TcpShellInterface

#define TRACE_OFF  0x0000
#define TRACE_RDP 1 <<  1
#define TRACE_WRP 1 <<  2
#define TRACE_SAM 1 <<  3
#define TRACE_LSN 1 <<  4
#define TRACE_CON 1 <<  5
#define TRACE_ALL  0xFFFF
#define DEBUG_LEVEL (TRACE_LSN | TRACE_CON)


/*******************************************************************************
 * @brief Connect (COn).
 *
 * @param[in]  piSHL_Enable  Enable signal from [SHELL].
 * @param[out] soSHL_OpnReq  Open connection request to [SHELL].
 * @param[in]  siSHL_OpnRep  Open connection reply from [SHELL].
 * @param[out] soSHL_ClsReq  Close connection request to [SHELL].
 * @param[out] poTAF_SConId  Session connect Id to [ROLE/TAF].
 *
 * @details
 *  This process connects the FPGA in client mode to a remote server which
 *   socket address is specified by [TODO-TBD]. The session ID of the opened
 *   connection is then provided to the TcpApplicationFlash (TAF) process of
 *   the ROLE via the 'poTAF_SConId' port.
 *
 * [FIXME:FOR THE TIME BEING,  THIS PROCESS IS DISABLED UNTIL WE ADD A SOFTWARE
 *        CONFIG REGISTER TO SPECIFY THE REMOTE SOCKET]
 *******************************************************************************/
void pConnect(
        CmdBit                *piSHL_Enable,
        stream<TcpAppOpnReq>  &soSHL_OpnReq,
        stream<TcpAppOpnRep>  &siSHL_OpnRep,
        stream<TcpAppClsReq>  &soSHL_ClsReq,
        SessionId             *poTAF_SConId)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "COn");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates{ OPN_IDLE, OPN_REQ, OPN_REP, OPN_DONE }
                               con_fsmState=OPN_IDLE;
    #pragma HLS reset variable=con_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static SockAddr      con_hostSockAddr;
    static TcpAppOpnRep  con_newCon;
    static ap_uint< 12>  con_watchDogTimer;

    switch (con_fsmState) {
    case OPN_IDLE:
        if (*piSHL_Enable != 1) {
            if (!siSHL_OpnRep.empty()) {
                // Drain any potential status data
                siSHL_OpnRep.read(con_newCon);
                printWarn(myName, "Draining unexpected residue from the \'OpnRep\' stream. As a result, request to close sessionId=%d.\n", con_newCon.sessId.to_uint());
                soSHL_ClsReq.write(con_newCon.sessId);
            }
        }
        else {
            // [FIXME - THIS PROCESS IS DISABLED UNTIL WE ADD A SW CONFIG of THE REMOTE SOCKET]
            con_fsmState = OPN_IDLE;  // FIXME --> Should be OPN_REQ;
        }
        break;
    case OPN_REQ:
        if (!soSHL_OpnReq.full()) {
            // [FIXME - Remove the hard coding of this socket]
            SockAddr    hostSockAddr(FIXME_DEFAULT_HOST_IP4_ADDR, FIXME_DEFAULT_HOST_LSN_PORT);
            con_hostSockAddr.addr = hostSockAddr.addr;
            con_hostSockAddr.port = hostSockAddr.port;
            soSHL_OpnReq.write(con_hostSockAddr);
            if (DEBUG_LEVEL & TRACE_CON) {
                printInfo(myName, "Client is requesting to connect to remote socket:\n");
                printSockAddr(myName, con_hostSockAddr);
            }
            #ifndef __SYNTHESIS__
                con_watchDogTimer = 250;
            #else
                con_watchDogTimer = 10000;
            #endif
            con_fsmState = OPN_REP;
        }
        break;

    case OPN_REP:
        con_watchDogTimer--;
        if (!siSHL_OpnRep.empty()) {
            // Read the reply stream
            siSHL_OpnRep.read(con_newCon);
            if (con_newCon.tcpState == ESTABLISHED) {
                if (DEBUG_LEVEL & TRACE_CON) {
                    printInfo(myName, "Client successfully connected to remote socket:\n");
                    printSockAddr(myName, con_hostSockAddr);
                    printInfo(myName, "The Session ID of this connection is: %d\n", con_newCon.sessId.to_int());
                }
                *poTAF_SConId = con_newCon.sessId;
                con_fsmState = OPN_DONE;
            }
            else {
                 printError(myName, "Client failed to connect to remote socket (TCP state is '%s'):\n",
                            getTcpStateName(con_newCon.tcpState));
                 printSockAddr(myName, con_hostSockAddr);
                 con_fsmState = OPN_DONE;
            }
        }
        else {
            if (con_watchDogTimer == 0) {
                if (DEBUG_LEVEL & TRACE_CON) {
                    printError(myName, "Timeout: Failed to connect to the following remote socket:\n");
                    printSockAddr(myName, con_hostSockAddr);
                }
                #ifndef __SYNTHESIS__
                  con_watchDogTimer = 250;
                #else
                  con_watchDogTimer = 10000;
                #endif
            }

        }
        break;
    case OPN_DONE:
        break;
    }
}

/*******************************************************************************
 * @brief Listen(LSn)
 *
 * @param[in]  piSHL_Enable  Enable signal from [SHELL].
 * @param[out] soSHL_LsnReq  Listen port request to [SHELL].
 * @param[in]  siSHL_LsnRep  Listen port reply from [SHELL].
 *
 * @warning
 *  This process requests the SHELL/NTS/TOE to start listening for incoming
 *   connections on a specific port (.i.e, open connection in server mode).
 *  By default, the port numbers 5001, 5201, 8800 and 8803 will always be opened
 *   in listen mode at startup. Later on, we should be able to open more port if
 *   we provide some configuration register for the user to specify new ones.
 *  FYI - The Port Table (PRt) of SHELL/NTS/TOE supports two port ranges; one
 *   for static ports (0 to 32,767) which are used for listening ports, and one
 *   for dynamically assigned or ephemeral ports (32,768 to 65,535) which are
 *   used for active connections. Therefore, listening port numbers must always
 *   fall in the range 0 to 32,767.
 *******************************************************************************/
void pListen(
        ap_uint<1>            *piSHL_Enable,
        stream<TcpAppLsnReq>  &soSHL_LsnReq,
        stream<TcpAppLsnRep>  &siSHL_LsnRep)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName = concat3(THIS_NAME, "/", "LSn");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { LSN_IDLE, LSN_SEND_REQ,
                            LSN_WAIT_REP, LSN_DONE } \
                               lsn_fsmState=LSN_IDLE;
    #pragma HLS reset variable=lsn_fsmState
    static ap_uint<2>          lsn_i = 0;
    #pragma HLS reset variable=lsn_i

    //-- STATIC ARRAYS --------------------------------------------------------
    static const TcpPort LSN_PORT_TABLE[4] = { RECV_MODE_LSN_PORT, ECHO_MODE_LSN_PORT,
                                               IPERF_LSN_PORT, IPREF3_LSN_PORT };
    #pragma HLS RESOURCE variable=LSN_PORT_TABLE core=ROM_1P

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static ap_uint<8>  lsn_watchDogTimer;
    static TcpPort     lsn_tcpPort;

    switch (lsn_fsmState) {
    case LSN_IDLE:
        if (*piSHL_Enable != 1) {
            return;
        }
        else {
            if (lsn_i == 0) {
                lsn_fsmState = LSN_SEND_REQ;
            }
            else {
                //-- Port are already opened
                lsn_fsmState = LSN_DONE;
            }
        }
        break;
    case LSN_SEND_REQ:
        if (!soSHL_LsnReq.full()) {
            switch (lsn_i) {
            case 0:
                soSHL_LsnReq.write(RECV_MODE_LSN_PORT);
                break;
            case 1:
                soSHL_LsnReq.write(ECHO_MODE_LSN_PORT);
                break;
            case 2:
                soSHL_LsnReq.write(IPERF_LSN_PORT);
                break;
            case 3:
                soSHL_LsnReq.write(IPREF3_LSN_PORT);
                break;
            }
            if (DEBUG_LEVEL & TRACE_LSN) {
                printInfo(myName, "Server is requested to listen on port #%d (0x%4.4X).\n",
                          LSN_PORT_TABLE[lsn_i].to_uint(), LSN_PORT_TABLE[lsn_i].to_uint());
            #ifndef __SYNTHESIS__
                lsn_watchDogTimer = 10;
            #else
                lsn_watchDogTimer = 100;
            #endif
            lsn_fsmState = LSN_WAIT_REP;
            }
        }
        else {
            printWarn(myName, "Cannot send a listen port request to [TOE] because stream is full!\n");
        }
        break;
    case LSN_WAIT_REP:
        lsn_watchDogTimer--;
        if (!siSHL_LsnRep.empty()) {
            TcpAppLsnRep  listenDone;
            siSHL_LsnRep.read(listenDone);
            if (listenDone) {
                printInfo(myName, "Received listen acknowledgment from [TOE].\n");
                if (lsn_i == 3) {
                    lsn_fsmState = LSN_DONE;
                }
                else {
                    //-- Set next listen port number
                    lsn_i += 1;
                    lsn_fsmState = LSN_SEND_REQ;
                }
            }
            else {
                printWarn(myName, "TOE denied listening on port %d (0x%4.4X).\n",
                          LSN_PORT_TABLE[lsn_i].to_uint(), LSN_PORT_TABLE[lsn_i].to_uint());
                lsn_fsmState = LSN_SEND_REQ;
            }
        }
        else {
            if (lsn_watchDogTimer == 0) {
                printError(myName, "Timeout: Server failed to listen on port %d %d (0x%4.4X).\n",
                        LSN_PORT_TABLE[lsn_i].to_uint(), LSN_PORT_TABLE[lsn_i].to_uint());
                lsn_fsmState = LSN_SEND_REQ;
            }
        }
        break;
    case LSN_DONE:
        break;
    }  // End-of: switch()
}

/*******************************************************************************
 * @brief Read Request Handler (RRh)
 *
 * @param[in]  siSHL_Notif  A new Rx data notification from [SHELL].
 * @param[out] soSHL_DReq   An Rx data request to [SHELL].
 *
 * @details
 *  This process waits for a notification from [TOE] indicating the availability
 *   of new data for the TcpApplication Flash (TAF) process of the [ROLE].
 *  If the TCP segment length of the notification message is greater than 0, the
 *   data segment is valid and the notification is accepted.
 *******************************************************************************/
void pReadRequestHandler(
        stream<TcpAppNotif>    &siSHL_Notif,
        stream<TcpAppRdReq>    &soSHL_DReq)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "RRh");

    //-- LOCAL STATIC VARIABLES W/ RESET ---------------------------------------
    static enum FsmStates { RRH_WAIT_NOTIF, RRH_SEND_DREQ } \
                               rrh_fsmState=RRH_WAIT_NOTIF;
    #pragma HLS reset variable=rrh_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static   TcpAppNotif rrh_notif;

    switch(rrh_fsmState) {
    case RRH_WAIT_NOTIF:
        if (!siSHL_Notif.empty()) {
            siSHL_Notif.read(rrh_notif);
            if (rrh_notif.tcpSegLen != 0) {
                // Always request the data segment to be received
                rrh_fsmState = RRH_SEND_DREQ;
            }
            else {
                printFatal(myName, "Received a notification for a TCP segment of length 'zero'. Don't know what to do with it!\n.");
            }
        }
        break;
    case RRH_SEND_DREQ:
        if (!soSHL_DReq.full()) {
            soSHL_DReq.write(TcpAppRdReq(rrh_notif.sessionID, rrh_notif.tcpSegLen));
            rrh_fsmState = RRH_WAIT_NOTIF;
        }
        break;
    }
}

/*******************************************************************************
 * @brief Read Path (RDp)
 *
 * @param[in]  siSHL_Data  Data stream from [SHELL].
 * @param[in]  siSHL_Meta  Session Id from [SHELL].
 * @param[out] soTAF_Data  Data stream to [TAF].
 * @param[out] soTAF_Meta  Session Id to [TAF].
 *
 * @details
 *  This process waits for a new data segment to read and forwards it to the
 *   TcpApplicationFlash (TAF) process. As such, it implements a pipe for the
 *   TCP traffic from [SHELL] to [TAF]
 *******************************************************************************/
void pReadPath(
        stream<TcpAppData>  &siSHL_Data,
        stream<TcpAppMeta>  &siSHL_Meta,
        stream<TcpAppData>  &soTAF_Data,
        stream<TcpAppMeta>  &soTAF_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "RDp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { RDP_WAIT_META=0, RDP_STREAM } \
	                           rdp_fsmState = RDP_WAIT_META;
    #pragma HLS reset variable=rdp_fsmState

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TcpAppData  currChunk;
    TcpAppMeta  sessId;

    switch (rdp_fsmState ) {
    case RDP_WAIT_META:
        if (!siSHL_Meta.empty() and !soTAF_Meta.full()) {
            siSHL_Meta.read(sessId);
            soTAF_Meta.write(sessId);
            rdp_fsmState  = RDP_STREAM;
        }
        break;
    case RDP_STREAM:
        if (!siSHL_Data.empty() && !soTAF_Data.full()) {
            siSHL_Data.read(currChunk);
            soTAF_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_RDP) { printAxisRaw(myName, "soTAF_Data =", currChunk); }
            if (currChunk.getTLast())
                rdp_fsmState  = RDP_WAIT_META;
        }
        break;
    }
}

/*******************************************************************************
 * @brief Write Path (WRp)
 *
 * @param[in]  siTAF_Data  Tx data stream from [ROLE/TAF].
 * @param[in]  siTAF_Meta  The session Id from [ROLE/TAF].
 * @param[out] soSHL_Data  Tx data to [SHELL].
 * @param[out] soSHL_Meta  Tx session Id to to [SHELL].
 * @param[in]  siSHL_DSts  Tx data write status from [SHELL].
 *
 * @details
 *  This process waits for a new data segment to write from the TcpAppFlash (TAF)
 *   and forwards it to the [SHELL]. As such, it implements a pipe for the TCP
 *   traffic from [TAF] to [SHELL].
 *******************************************************************************/
void pWritePath(
        stream<TcpAppData>  &siTAF_Data,
        stream<TcpAppMeta>  &siTAF_Meta,
        stream<TcpAppData>  &soSHL_Data,
        stream<TcpAppMeta>  &soSHL_Meta,
        stream<TcpAppWrSts> &siSHL_DSts)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "WRp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { WRP_WAIT_META=0, WRP_STREAM } \
                               wrp_fsmState = WRP_WAIT_META;
    #pragma HLS reset variable=wrp_fsmState

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TcpAppMeta   tcpSessId;
    TcpAppData   currChunk;

    switch (wrp_fsmState) {
    case WRP_WAIT_META:
        //-- Always read the session Id provided by [ROLE/TAF]
        if (!siTAF_Meta.empty() and !soSHL_Meta.full()) {
            siTAF_Meta.read(tcpSessId);
            soSHL_Meta.write(tcpSessId);
            if (DEBUG_LEVEL & TRACE_WRP) {
                printInfo(myName, "Received new session ID #%d from [ROLE].\n",
                          tcpSessId.to_uint());
            }
            wrp_fsmState = WRP_STREAM;
        }
        break;
    case WRP_STREAM:
        if (!siTAF_Data.empty() && !soSHL_Data.full()) {
            siTAF_Data.read(currChunk);
            soSHL_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_WRP) { printAxisRaw(myName, "soSHL_Data =", currChunk); }
            if(currChunk.getTLast()) {
                wrp_fsmState = WRP_WAIT_META;
            }
        }
        break;
    }

    //-- ALWAYS -----------------------
    if (!siSHL_DSts.empty()) {
        siSHL_DSts.read();  // [TODO] Must check!
    }
}

/*******************************************************************************
 * @brief TCP Shell Interface (TSIF)
 *
 * @param[in]  piSHL_Mmio_En Enable signal from [SHELL/MMIO].
 * @param[in]  siTAF_Data    TCP data stream from TcpAppFlash (TAF).
 * @param[in]  siTAF_Meta    TCP session Id  from [TAF].
 * @param[out] soTAF_Data    TCP data stream to   [TAF].
 * @param[out] soTAF_Meta    TCP session Id  to   [TAF].
 * @param[in]  siSHL_Notif   TCP data notification from [SHELL].
 * @param[out] soSHL_DReq    TCP data request to [SHELL].
 * @param[in]  siSHL_Data    TCP data stream from [SHELL].
 * @param[in]  siSHL_Meta    TCP metadata from [SHELL].
 * @param[out] soSHL_LsnReq  TCP listen port request to [SHELL].
 * @param[in]  siSHL_LsnRep  TCP listen port acknowledge from [SHELL].
 * @param[out] soSHL_Data    TCP data stream to [SHELL].
 * @param[out] soSHL_Meta    TCP metadata to [SHELL].
 * @param[in]  siSHL_DSts    TCP write data status from [SHELL].
 * @param[out] soSHL_OpnReq  TCP open connection request to [SHELL].
 * @param[in]  siSHL_OpnRep  TCP open connection reply from [SHELL].
 * @param[out] soSHL_ClsReq  TCP close connection request to [SHELL].
 * @param[out] poTAF_SConId  TCP session connect id to [ROLE/TAF].
 *
 * @return Nothing.
 *
 *******************************************************************************/
void tcp_shell_if(

        //------------------------------------------------------
        //-- SHELL / Mmio Interface
        //------------------------------------------------------
        CmdBit                *piSHL_Mmio_En,

        //------------------------------------------------------
        //-- TAF / TxP Data Interface
        //------------------------------------------------------
        stream<TcpAppData>    &siTAF_Data,
        stream<TcpAppMeta>    &siTAF_Meta,

        //------------------------------------------------------
        //-- TAF / RxP Data Interface
        //------------------------------------------------------
        stream<TcpAppData>    &soTAF_Data,
        stream<TcpAppMeta>    &soTAF_Meta,

        //------------------------------------------------------
        //-- SHELL / Rx Data Interfaces
        //------------------------------------------------------
        stream<TcpAppNotif>   &siSHL_Notif,
        stream<TcpAppRdReq>   &soSHL_DReq,
        stream<TcpAppData>    &siSHL_Data,
        stream<TcpAppMeta>    &siSHL_Meta,

        //------------------------------------------------------
        //-- SHELL / Listen Interfaces
        //------------------------------------------------------
        stream<TcpAppLsnReq>  &soSHL_LsnReq,
        stream<TcpAppLsnRep>  &siSHL_LsnRep,

        //------------------------------------------------------
        //-- SHELL / Tx Data Interfaces
        //------------------------------------------------------
        stream<TcpAppData>    &soSHL_Data,
        stream<TcpAppMeta>    &soSHL_Meta,
        stream<TcpAppWrSts>   &siSHL_DSts,

        //------------------------------------------------------
        //-- SHELL / Tx Open Interfaces
        //------------------------------------------------------
        stream<TcpAppOpnReq>  &soSHL_OpnReq,
        stream<TcpAppOpnRep>  &siSHL_OpnRep,

        //------------------------------------------------------
        //-- SHELL / Close Interfaces
        //------------------------------------------------------
        stream<TcpAppClsReq>  &soSHL_ClsReq,

        //------------------------------------------------------
        //-- TAF / Session Connect Id Interface
        //------------------------------------------------------
        SessionId             *poTAF_SConId)
{
    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

  #if defined(USE_DEPRECATED_DIRECTIVES)

    #pragma HLS INTERFACE ap_stable          port=piSHL_Mmio_En    name=piSHL_Mmio_En

    #pragma HLS resource core=AXI4Stream variable=siTAF_Data   metadata="-bus_bundle siTAF_Data"
    #pragma HLS resource core=AXI4Stream variable=siTAF_Meta   metadata="-bus_bundle siTAF_Meta"

    #pragma HLS resource core=AXI4Stream variable=soTAF_Data   metadata="-bus_bundle soTAF_Data"
    #pragma HLS resource core=AXI4Stream variable=soTAF_Meta   metadata="-bus_bundle soTAF_Meta"

    #pragma HLS resource core=AXI4Stream variable=siSHL_Notif  metadata="-bus_bundle siSHL_Notif"
    #pragma HLS DATA_PACK                variable=siSHL_Notif
    #pragma HLS resource core=AXI4Stream variable=soSHL_DReq   metadata="-bus_bundle soSHL_DReq"
    #pragma HLS DATA_PACK                variable=soSHL_DReq
    #pragma HLS resource core=AXI4Stream variable=siSHL_Data   metadata="-bus_bundle siSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=siSHL_Meta   metadata="-bus_bundle siSHL_Meta"

    #pragma HLS resource core=AXI4Stream variable=soSHL_LsnReq metadata="-bus_bundle soSHL_LsnReq"
    #pragma HLS resource core=AXI4Stream variable=siSHL_LsnRep metadata="-bus_bundle siSHL_LsnRep"

    #pragma HLS resource core=AXI4Stream variable=soSHL_Data   metadata="-bus_bundle soSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=soSHL_Meta   metadata="-bus_bundle soSHL_Meta"
    #pragma HLS resource core=AXI4Stream variable=siSHL_DSts   metadata="-bus_bundle siSHL_DSts"
    #pragma HLS DATA_PACK                variable=siSHL_DSts

    #pragma HLS resource core=AXI4Stream variable=soSHL_OpnReq metadata="-bus_bundle soSHL_OpnReq"
    #pragma HLS DATA_PACK                variable=soSHL_OpnReq
    #pragma HLS resource core=AXI4Stream variable=siSHL_OpnRep metadata="-bus_bundle siSHL_OpnRep"
    #pragma HLS DATA_PACK                variable=siSHL_OpnRep

    #pragma HLS resource core=AXI4Stream variable=soSHL_ClsReq metadata="-bus_bundle soSHL_ClsReq"

    #pragma HLS INTERFACE ap_vld register    port=poTAF_SConId

  #else

    #pragma HLS INTERFACE ap_stable          port=piSHL_Mmio_En  name=piSHL_Mmio_En

    #pragma HLS INTERFACE axis register both port=siTAF_Data     name=siTAF_Data
    #pragma HLS INTERFACE axis register both port=siTAF_Meta     name=siTAF_Meta

    #pragma HLS INTERFACE axis register both port=soTAF_Data     name=soTAF_Data
    #pragma HLS INTERFACE axis register both port=soTAF_Meta     name=soTAF_Meta

    #pragma HLS INTERFACE axis register both port=siSHL_Notif    name=siSHL_Notif
    #pragma HLS DATA_PACK                variable=siSHL_Notif
    #pragma HLS INTERFACE axis register both port=soSHL_DReq     name=soSHL_DReq
    #pragma HLS DATA_PACK                variable=soSHL_DReq
    #pragma HLS INTERFACE axis register both port=siSHL_Data     name=siSHL_Data
    #pragma HLS INTERFACE axis register both port=siSHL_Meta     name=siSHL_Meta

    #pragma HLS INTERFACE axis register both port=soSHL_LsnReq   name=soSHL_LsnReq
    #pragma HLS INTERFACE axis register both port=siSHL_LsnRep   name=siSHL_LsnRep

    #pragma HLS INTERFACE axis register both port=soSHL_Data     name=soSHL_Data
    #pragma HLS INTERFACE axis register both port=soSHL_Meta     name=soSHL_Meta
    #pragma HLS INTERFACE axis register both port=siSHL_DSts     name=siSHL_DSts
    #pragma HLS DATA_PACK                variable=siSHL_DSts

    #pragma HLS INTERFACE axis register both port=soSHL_OpnReq   name=soSHL_OpnReq
    #pragma HLS DATA_PACK                variable=soSHL_OpnReq
    #pragma HLS INTERFACE axis register both port=siSHL_OpnRep   name=siSHL_OpnRep
    #pragma HLS DATA_PACK                variable=siSHL_OpnRep

    #pragma HLS INTERFACE axis register both port=soSHL_ClsReq   name=soSHL_ClsReq

    #pragma HLS INTERFACE ap_vld register    port=poTAF_SConId

  #endif

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //-- LOCAL STREAMS ---------------------------------------------------------

    //-- PROCESS FUNCTIONS -----------------------------------------------------
    pConnect(
            piSHL_Mmio_En,
            soSHL_OpnReq,
            siSHL_OpnRep,
            soSHL_ClsReq,
            poTAF_SConId);

    pListen(
            piSHL_Mmio_En,
            soSHL_LsnReq,
            siSHL_LsnRep);

    pReadRequestHandler(
            siSHL_Notif,
            soSHL_DReq);

    pReadPath(
            siSHL_Data,
            siSHL_Meta,
            soTAF_Data,
            soTAF_Meta);

    pWritePath(
            siTAF_Data,
            siTAF_Meta,
            soSHL_Data,
            soSHL_Meta,
            siSHL_DSts);

}

/*! \} */
