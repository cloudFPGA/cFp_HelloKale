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
 * @file       : tcp_role_if.cpp
 * @brief      : TCP Role Interface (TRIF)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details    : This process handles the control flow interface between the
 *  the SHELL and the ROLE. The main purpose of the TRIF is to open port(s) for
 *   listening and for connecting to remote host(s).
 *
 * @todo       : [TODO - The logic for listening and connecting is most likely going to end up in the FMC].
 *
 *****************************************************************************/

#include "tcp_role_if.hpp"

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


#define THIS_NAME "TRIF"

#define TRACE_OFF  0x0000
#define TRACE_RDP 1 <<  1
#define TRACE_WRP 1 <<  2
#define TRACE_SAM 1 <<  3
#define TRACE_LSN 1 <<  4
#define TRACE_CON 1 <<  5
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

enum DropCmd {KEEP_CMD=false, DROP_CMD};

//---------------------------------------------------------
//-- DEFAULT LOCAL FPGA AND FOREIGN HOST SOCKETS
//--  By default, the following sockets will be used by the
//--  TCP Role Interface, unless the user specifies new ones
//--  via TBD.
//--  FYI --> 8803 is the ZIP code of Ruschlikon ;-)
//---------------------------------------------------------
#define DEFAULT_FPGA_LSN_PORT   0x2263      // TOE    listens on port = 8803 (static  ports must be     0..32767)
#define DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // HOST's IP Address      = 10.12.200.50
#define DEFAULT_HOST_LSN_PORT   8803+0x8000 // HOST   listens on port = 41571



/******************************************************************************
 * @brief Client connection to remote HOST or FPGA socket (COn).
 *
 * @param[in]  piSHL_Enable,  enable signal from [SHELL].
 * @param[out] soTOE_OpnReq,  open connection request to [TOE].
 * @param[in]  siTOE_OpnRep,  open connection reply from [TOE].
 * @param[out] soTOE_ClsReq,  close connection request to [TOE].
 * @param[out] poROL_SConId,  session connect Id to [ROL].
 *
 * @details
 *  For testing purposes, this connect process opens a single connection to a
 *   'hard-code' remote HOST IP address (10.12.200.50) and port 8803.
 *  The session ID of this single connection is provided to the TCP Application
 *  Flash process (TAF) of the ROLE over the 'poROL_SConId' port.
 ******************************************************************************/
void pConnect(
        CmdBit                *piSHL_Enable,
        stream<AppOpnReq>     &soTOE_OpnReq,
        stream<AppOpnRep>     &siTOE_OpnRep,
        stream<AppClsReqAxis> &soTOE_ClsReq,
        SessionId             *poROL_SConId)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "COn");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum OpnFsmStates{ OPN_IDLE, OPN_REQ, OPN_REP, OPN_DONE } opnFsmState=OPN_IDLE;
    #pragma HLS reset                                       variable=opnFsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static LE_SockAddr   leHostSockAddr;  // Socket Address stored in LITTLE-ENDIAN ORDER
                                          // [FIXME - Change to default Big-Endian]
    static AppOpnRep     newConn;
    static ap_uint< 12>  watchDogTimer;

    switch (opnFsmState) {

    case OPN_IDLE:
        if (*piSHL_Enable != 1) {
            if (!siTOE_OpnRep.empty()) {
                // Drain any potential status data
                siTOE_OpnRep.read(newConn);
                printWarn(myName, "Draining unexpected residue from the \'OpnRep\' stream. As a result, request to close sessionId=%d.\n", newConn.sessionID.to_uint());
                soTOE_ClsReq.write(newConn.sessionID);
            }
        }
        else
            opnFsmState = OPN_REQ;
        break;

    case OPN_REQ:
        if (!soTOE_OpnReq.full()) {
            // [TODO - Remove the hard coding of this socket]
            SockAddr    hostSockAddr(DEFAULT_HOST_IP4_ADDR, DEFAULT_HOST_LSN_PORT);
            leHostSockAddr.addr = byteSwap32(hostSockAddr.addr);
            leHostSockAddr.port = byteSwap16(hostSockAddr.port);
            soTOE_OpnReq.write(leHostSockAddr);
            if (DEBUG_LEVEL & TRACE_CON) {
                printInfo(myName, "Client is requesting to connect to remote socket:\n");
                printSockAddr(myName, leHostSockAddr);
            }
            #ifndef __SYNTHESIS__
                watchDogTimer = 250;
            #else
                watchDogTimer = 10000;
            #endif
            opnFsmState = OPN_REP;
        }
        break;

    case OPN_REP:
        watchDogTimer--;
        if (!siTOE_OpnRep.empty()) {
            // Read the reply stream
            siTOE_OpnRep.read(newConn);
            if (newConn.success) {
                if (DEBUG_LEVEL & TRACE_CON) {
                    printInfo(myName, "Client successfully connected to remote socket:\n");
                    printSockAddr(myName, leHostSockAddr);
                    printInfo(myName, "The Session ID of this connection is: %d\n", newConn.sessionID.to_int());
                }
                *poROL_SConId = newConn.sessionID;
                opnFsmState = OPN_DONE;
            }
            else {
                 printError(myName, "Client failed to connect to remote socket:\n");
                 printSockAddr(myName, leHostSockAddr);
                 opnFsmState = OPN_DONE;
            }
        }
        else {
            if (watchDogTimer == 0) {
                if (DEBUG_LEVEL & TRACE_CON) {
                    printError(myName, "Timeout: Failed to connect to the following remote socket:\n");
                    printSockAddr(myName, leHostSockAddr);
                }
                #ifndef __SYNTHESIS__
                  watchDogTimer = 250;
                #else
                  watchDogTimer = 10000;
                #endif
            }

        }
        break;
    case OPN_DONE:
        break;
    }
}

/*****************************************************************************
 * @brief Request the TOE to start listening (LSn) for incoming connections
 *  on a specific port (.i.e open connection for reception mode).
 *
 * @param[in]  piSHL_Enable,   enable signal from [SHELL].
 * @param[out] soTOE_LsnReq,   listen port request to [TOE].
 * @param[in]  siTOE_LsnAck,   listen port acknowledge from [TOE].
 *
 * @warning
 *  The Port Table (PRt) supports two port ranges; one for static ports (0 to
 *   32,767) which are used for listening ports, and one for dynamically
 *   assigned or ephemeral ports (32,768 to 65,535) which are used for active
 *   connections. Therefore, listening port numbers must be in the range 0
 *   to 32,767.
 * 
 ******************************************************************************/
void pListen(
        ap_uint<1>            *piSHL_Enable,
        stream<AppLsnReqAxis> &soTOE_LsnReq,
        stream<AppLsnAckAxis> &siTOE_LsnAck)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char    *myName      = concat3(THIS_NAME, "/", "LSn");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum LsnFsmStates { LSN_IDLE, LSN_SEND_REQ, LSN_WAIT_ACK, LSN_DONE } lsnFsmState=LSN_IDLE;
    #pragma HLS reset                                                  variable=lsnFsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<8>  watchDogTimer;

    switch (lsnFsmState) {

    case LSN_IDLE:
        if (*piSHL_Enable != 1)
            return;
        else
            lsnFsmState = LSN_SEND_REQ;
        break;

    case LSN_SEND_REQ:
        if (!soTOE_LsnReq.full()) {
            TcpPort  tcpListenPort = DEFAULT_FPGA_LSN_PORT;
            soTOE_LsnReq.write(tcpListenPort);
            if (DEBUG_LEVEL & TRACE_LSN) {
                printInfo(myName, "Server is requested to listen on port #%d (0x%4.4X).\n",
                          DEFAULT_FPGA_LSN_PORT, DEFAULT_FPGA_LSN_PORT);
            #ifndef __SYNTHESIS__
                watchDogTimer = 10;
            #else
                watchDogTimer = 100;
            #endif
            lsnFsmState = LSN_WAIT_ACK;
            }
        }
        else {
            printWarn(myName, "Cannot send a listen port request to [TOE] because stream is full!\n");
        }
        break;

    case LSN_WAIT_ACK:
        watchDogTimer--;
        if (!siTOE_LsnAck.empty()) {
            AppLsnAckAxis  listenDone;
            siTOE_LsnAck.read(listenDone);
            if (listenDone.tdata) {
                printInfo(myName, "Received listen acknowledgment from [TOE].\n");
                lsnFsmState = LSN_DONE;
            }
            else {
                printWarn(myName, "TOE denied listening on port %d (0x%4.4X).\n",
                          DEFAULT_FPGA_LSN_PORT, DEFAULT_FPGA_LSN_PORT);
                lsnFsmState = LSN_SEND_REQ;
            }
        }
        else {
            if (watchDogTimer == 0) {
                printError(myName, "Timeout: Server failed to listen on port %d %d (0x%4.4X).\n",
                           DEFAULT_FPGA_LSN_PORT, DEFAULT_FPGA_LSN_PORT);
                lsnFsmState = LSN_SEND_REQ;
            }
        }
        break;

    case LSN_DONE:
        break;
    }  // End-of: switch()
}

/*****************************************************************************
 * @brief ReadRequestHandler (RRh).
 *  Waits for a notification indicating the availability of new data for
 *  the ROLE. If the TCP segment length is greater than 0, the notification
 *  is accepted.
 *
 * @param[in]  siTOE_Notif, a new Rx data notification from TOE.
 * @param[out] soTOE_DReq,  a Rx data request to TOE.
 *
 * @return Nothing.
 ******************************************************************************/
void pReadRequestHandler(
        stream<AppNotif>    &siTOE_Notif,
        stream<AppRdReq>    &soTOE_DReq)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "RRh");

    //-- LOCAL STATIC VARIABLES W/ RESET --------------------------------------
    static enum                RrhFsmStates { RRH_WAIT_NOTIF, RRH_SEND_DREQ } rrhFsmState=RRH_WAIT_NOTIF;
    #pragma HLS reset variable=rrhFsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static   AppNotif notif;

    switch(rrhFsmState) {
    case RRH_WAIT_NOTIF:
        if (!siTOE_Notif.empty()) {
            siTOE_Notif.read(notif);
            if (notif.tcpSegLen != 0) {
                // Always request the data segment to be received
                rrhFsmState = RRH_SEND_DREQ;
            }
        }
        break;
    case RRH_SEND_DREQ:
        if (!soTOE_DReq.full()) {
            soTOE_DReq.write(AppRdReq(notif.sessionID, notif.tcpSegLen));
            rrhFsmState = RRH_WAIT_NOTIF;
        }
        break;
    }
}

/*****************************************************************************
 * @brief Read Path (RDp) - From TOE to ROLE.
 *  Process waits for a new data segment to read and forwards it to the ROLE.
 *
 * @param[in]  siTOE_Data,   Data from [TOE].
 * @param[in]  siTOE_SessId, Session Id from [TOE].
 * @param[out] soROL_Data,   Data to [ROLE].
 * @param[out] soROL_SessId, Session Id to [ROLE].
 *
 * @return Nothing.
 *****************************************************************************/
void pReadPath(
        stream<AppData>     &siTOE_Data,
        stream<AppMetaAxis> &siTOE_SessId,
        stream<AppData>     &soROL_Data,
        stream<AppMetaAxis> &soROL_SessId)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "RDp");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum RdpFsmStates { RDP_WAIT_META=0, RDP_STREAM } rdpFsmState = RDP_WAIT_META;
    #pragma HLS reset variable=rdpFsmState

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AppData     currWord;
    AppMetaAxis sessId;

    switch (rdpFsmState ) {
    case RDP_WAIT_META:
        if (!siTOE_SessId.empty() and !soROL_SessId.full()) {
            siTOE_SessId.read(sessId);
            soROL_SessId.write(sessId);
            rdpFsmState  = RDP_STREAM;
        }
        break;
    case RDP_STREAM:
        if (!siTOE_Data.empty() && !soROL_Data.full()) {
            siTOE_Data.read(currWord);
            if (DEBUG_LEVEL & TRACE_RDP) {
                 printAxiWord(myName, currWord);
            }
            soROL_Data.write(currWord);
            if (currWord.tlast)
                rdpFsmState  = RDP_WAIT_META;
        }
        break;
    }
}

/*****************************************************************************
 * @brief Write Path (WRp) - From ROLE to TOE.
 *  Process waits for a new data segment to write and forwards it to TOE.
 *
 * @param[in]  siROL_Data,   Tx data from [ROLE].
 * @param[in]  siROL_SessId, the session Id from [ROLE].
 * @param[out] soTOE_Data,   Tx data to [TOE].
 * @param[out] soTOE_SessId, Tx session Id to to [TOE].
 * @param[in]  siTOE_DSts,   Tx data write status from [TOE].
 *
 * @return Nothing.
 ******************************************************************************/
void pWritePath(
        stream<AppData>      &siROL_Data,
        stream<AppMetaAxis>  &siROL_SessId,
        stream<AppData>      &soTOE_Data,
        stream<AppMetaAxis>  &soTOE_SessId,
        stream<AppWrSts>     &siTOE_DSts)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "WRp");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum WrpFsmStates { WRP_WAIT_META=0, WRP_STREAM } wrpFsmState = WRP_WAIT_META;
    #pragma HLS reset variable=wrpFsmState

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AppMetaAxis   tcpSessId;
    AxiWord       currWordIn;
    AxiWord       currWordOut;

    switch (wrpFsmState) {
    case WRP_WAIT_META:
        //-- Always read the session Id provided by [ROLE]
        if (!siROL_SessId.empty() and !soTOE_SessId.full()) {
            siROL_SessId.read(tcpSessId);
            soTOE_SessId.write(tcpSessId);
            if (DEBUG_LEVEL & TRACE_WRP) {
                printInfo(myName, "Received new session ID #%d from [ROLE].\n",
                          tcpSessId.tdata.to_uint());
            }
            wrpFsmState = WRP_STREAM;
        }
        break;
    case WRP_STREAM:
        if (!siROL_Data.empty() && !soTOE_Data.full()) {
            siROL_Data.read(currWordIn);
            if (DEBUG_LEVEL & TRACE_WRP) {
                 printAxiWord(myName, currWordIn);
            }
            soTOE_Data.write(currWordIn);
            if(currWordIn.tlast)
                wrpFsmState = WRP_WAIT_META;
        }
        break;
    }

    //-- ALWAYS -----------------------
    if (!siTOE_DSts.empty()) {
        siTOE_DSts.read();  // [TODO] Checking.
    }
}


/*****************************************************************************
 * @brief   Main process of the TCP Role Interface (TRIF)
 * @ingroup tcp_role_if
 *
 * @param[in]  piSHL_Mmio_En Enable signal from [MMIO].
 * @param[in]  siROL_Data    TCP data stream from [ROLE].
 * @param[in]  siROL_SessId  TCP session Id  from [ROLE].
 * @param[out] soRL_Data     TCP data stream to   [ROLE].
 * @param[in]  siTOE_Notif   TCP data notification from [TOE].
 * @param[out] soTOE_RdReq   TCP data request to [TOE].
 * @param[in]  siTOE_Data    TCP data stream from [TOE].
 * @param[in]  siTOE_SessId  TCP metadata from [TOE].
 * @param[out] soTOE_LsnReq  TCP listen port request to [TOE].
 * @param[in]  siTOE_LsnAck  TCP listen port acknowledge from [TOE].
 * @param[out] soTOE_Data    TCP data stream to [TOE].
 * @param[out] soTOE_SessId  TCP metadata to [TOE].
 * @param[in]  siTOE_DSts    TCP write data status from [TOE].
 * @param[out] soTOE_OpnReq  TCP open connection request to [TOE].
 * @param[in]  siTOE_OpnRep  TCP open connection reply from [TOE].
 * @param[out] soTOE_ClsReq  TCP close connection request to [TOE].
 *
 * @return Nothing.
 *
 *****************************************************************************/
void tcp_role_if(

        //------------------------------------------------------
        //-- SHELL / Mmio Interface
        //------------------------------------------------------
        CmdBit                *piSHL_Mmio_En,

        //------------------------------------------------------
        //-- ROLE / TxP Data Interface
        //------------------------------------------------------
        stream<AppData>       &siROL_Data,
        stream<AppMetaAxis>   &siROL_SessId,

        //------------------------------------------------------
        //-- ROLE / RxP Data Interface
        //------------------------------------------------------
        stream<AppData>       &soROL_Data,
        stream<AppMetaAxis>   &soROL_SessId,

        //------------------------------------------------------
        //-- TOE / Rx Data Interfaces
        //------------------------------------------------------
        stream<AppNotif>      &siTOE_Notif,
        stream<AppRdReq>      &soTOE_DReq,
        stream<AppData>       &siTOE_Data,
        stream<AppMetaAxis>   &siTOE_SessId,

        //------------------------------------------------------
        //-- TOE / Listen Interfaces
        //------------------------------------------------------
        stream<AppLsnReqAxis> &soTOE_LsnReq,
        stream<AppLsnAckAxis> &siTOE_LsnAck,

        //------------------------------------------------------
        //-- TOE / Tx Data Interfaces
        //------------------------------------------------------
        stream<AppData>       &soTOE_Data,
        stream<AppMetaAxis>   &soTOE_SessId,
        stream<AppWrSts>      &siTOE_DSts,

        //------------------------------------------------------
        //-- TOE / Tx Open Interfaces
        //------------------------------------------------------
        stream<AppOpnReq>     &soTOE_OpnReq,
        stream<AppOpnRep>     &siTOE_OpnRep,

        //------------------------------------------------------
        //-- TOE / Close Interfaces
        //------------------------------------------------------
        stream<AppClsReqAxis> &soTOE_ClsReq,

        //------------------------------------------------------
        //-- ROLE / Session Connect Id Interface
        //------------------------------------------------------
        SessionId             *poROL_SConId)
{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

  #if defined(USE_DEPRECATED_DIRECTIVES)

    #pragma HLS INTERFACE ap_stable          port=piSHL_Mmio_En    name=piSHL_Mmio_En

    #pragma HLS resource core=AXI4Stream variable=siROL_Data   metadata="-bus_bundle siROL_Data"
    #pragma HLS resource core=AXI4Stream variable=siROL_SessId metadata="-bus_bundle siROL_SessId"

    #pragma HLS resource core=AXI4Stream variable=soROL_Data   metadata="-bus_bundle soROL_Data"
    #pragma HLS resource core=AXI4Stream variable=soROL_SessId metadata="-bus_bundle soROL_SessId"

    #pragma HLS resource core=AXI4Stream variable=siTOE_Notif  metadata="-bus_bundle siTOE_Notif"
    #pragma HLS DATA_PACK                variable=siTOE_Notif
    #pragma HLS resource core=AXI4Stream variable=soTOE_DReq   metadata="-bus_bundle soTOE_DReq"
    #pragma HLS DATA_PACK                variable=soTOE_DReq
    #pragma HLS resource core=AXI4Stream variable=siTOE_Data   metadata="-bus_bundle siTOE_Data"
    #pragma HLS resource core=AXI4Stream variable=siTOE_SessId metadata="-bus_bundle siTOE_SessId"

    #pragma HLS resource core=AXI4Stream variable=soTOE_LsnReq metadata="-bus_bundle soTOE_LsnReq"
    #pragma HLS resource core=AXI4Stream variable=siTOE_LsnAck metadata="-bus_bundle siTOE_LsnAck"

    #pragma HLS resource core=AXI4Stream variable=soTOE_Data   metadata="-bus_bundle soTOE_Data"
    #pragma HLS resource core=AXI4Stream variable=soTOE_SessId metadata="-bus_bundle soTOE_SessId"
    #pragma HLS resource core=AXI4Stream variable=siTOE_DSts   metadata="-bus_bundle siTOE_DSts"
    #pragma HLS DATA_PACK                variable=siTOE_DSts

    #pragma HLS resource core=AXI4Stream variable=soTOE_OpnReq metadata="-bus_bundle soTOE_OpnReq"
    #pragma HLS DATA_PACK                variable=soTOE_OpnReq
    #pragma HLS resource core=AXI4Stream variable=siTOE_OpnRep metadata="-bus_bundle siTOE_OpnRep"
    #pragma HLS DATA_PACK                variable=siTOE_OpnRep

    #pragma HLS resource core=AXI4Stream variable=soTOE_ClsReq metadata="-bus_bundle soTOE_ClsReq"

    #pragma HLS INTERFACE ap_vld register    port=poROL_SConId

  #else

    #pragma HLS INTERFACE ap_stable          port=piSHL_Mmio_En  name=piSHL_Mmio_En

    #pragma HLS INTERFACE axis register both port=siROL_Data     name=siROL_Data
    #pragma HLS INTERFACE axis register both port=siROL_SessId   name=siROL_SessId

    #pragma HLS INTERFACE axis register both port=soROL_Data     name=soROL_Data
    #pragma HLS INTERFACE axis register both port=soROL_SessId   name=soROL_SessId

    #pragma HLS INTERFACE axis register both port=siTOE_Notif    name=siTOE_Notif
    #pragma HLS DATA_PACK                variable=siTOE_Notif
    #pragma HLS INTERFACE axis register both port=soTOE_DReq     name=soTOE_DReq
    #pragma HLS DATA_PACK                variable=soTOE_DReq
    #pragma HLS INTERFACE axis register both port=siTOE_Data     name=siTOE_Data
    #pragma HLS INTERFACE axis register both port=siTOE_SessId   name=siTOE_SessId

    #pragma HLS INTERFACE axis register both port=soTOE_LsnReq   name=soTOE_LsnReq
    #pragma HLS INTERFACE axis register both port=siTOE_LsnAck   name=siTOE_LsnAck

    #pragma HLS INTERFACE axis register both port=soTOE_Data     name=soTOE_Data
    #pragma HLS INTERFACE axis register both port=soTOE_SessId   name=soTOE_SessId
    #pragma HLS INTERFACE axis register both port=siTOE_DSts     name=siTOE_DSts
    #pragma HLS DATA_PACK                variable=siTOE_DSts

    #pragma HLS INTERFACE axis register both port=soTOE_OpnReq   name=soTOE_OpnReq
    #pragma HLS DATA_PACK                variable=soTOE_OpnReq
    #pragma HLS INTERFACE axis register both port=siTOE_OpnRep   name=siTOE_OpnRep
    #pragma HLS DATA_PACK                variable=siTOE_OpnRep

    #pragma HLS INTERFACE axis register both port=soTOE_ClsReq   name=soTOE_ClsReq

    #pragma HLS INTERFACE ap_vld register    port=poROL_SConId

  #endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-- LOCAL STREAMS --------------------------------------------------------

    //-- PROCESS FUNCTIONS ----------------------------------------------------
    pConnect(
            piSHL_Mmio_En,
            soTOE_OpnReq,
            siTOE_OpnRep,
            soTOE_ClsReq,
            poROL_SConId);

    pListen(
            piSHL_Mmio_En,
            soTOE_LsnReq,
            siTOE_LsnAck);

    pReadRequestHandler(
            siTOE_Notif,
            soTOE_DReq);

    pReadPath(
            siTOE_Data,
            siTOE_SessId,
            soROL_Data,
            soROL_SessId);

    pWritePath(
            siROL_Data,
            siROL_SessId,
            soTOE_Data,
            soTOE_SessId,
            siTOE_DSts);

}

