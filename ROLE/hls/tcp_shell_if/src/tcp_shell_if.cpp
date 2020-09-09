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
 * @details This entity handles the control flow interface between the SHELL
 *   and the ROLE. The main purpose of the TSIF is to open a predefined set of
 *   ports in listening mode and/or to actively connect to remote host(s).
 *   The use of a dedicated layer is not a prerequisite but it is provided here
 *   for sake of simplicity by separating the data flow processing (see TAF)
 *   from the current control flow processing.
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
#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * @brief Connect (COn).
 *
 * @param[in]  piSHL_Enable     Enable signal from [SHELL].
 * @param[in]  siRDp_OpnSockReq The remote socket to connect from ReadPath(RDp).
 * @param[in]  siRDp_TxCountReq The #bytes to be transmitted after connection is opened.
 * @param[out] soWRp_TxBytesReq The #bytes to be transmitted to WritePath (WRp).
 * @param[out] soWRp_TxSessId   The session id of the active opened connection to [WRp].
 * @param[out] soSHL_OpnReq     Open connection request to [SHELL].
 * @param[in]  siSHL_OpnRep     Open connection reply from [SHELL].
 * @param[out] soSHL_ClsReq     Close connection request to [SHELL].
 *
 * @details
 *  This process connects the FPGA in client mode to a remote server which
 *   socket address is specified by 'siRDp_OpnSockReq'.
 *  Alternatively, the process is also used to trigger the TxPath (TXp) to xmit
 *   a segment to the newly opened connection. The number of bytes to transmit
 *   is then specified by the [RDp] via the 'siRDp_TxCountReq' input.
 *   This mode is used to test the TOE in transmit mode.
 *******************************************************************************/
void pConnect(
        CmdBit                *piSHL_Enable,
        stream<SockAddr>      &siRDp_OpnSockReq,
        stream<Ly4Len>        &siRDp_TxCountReq,
        stream<Ly4Len>        &soWRp_TxBytesReq,
        stream<SessionId>     &soWRp_TxSessId,
        stream<TcpAppOpnReq>  &soSHL_OpnReq,
        stream<TcpAppOpnRep>  &siSHL_OpnRep,
        stream<TcpAppClsReq>  &soSHL_ClsReq)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "COn");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates{ OPN_IDLE, OPN_REQ, OPN_REP, OPN_8801 }
                               con_fsmState=OPN_IDLE;
    #pragma HLS reset variable=con_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static TcpAppOpnRep  con_opnRep;
    static Ly4Len        con_txBytesReq;
    static ap_uint< 12>  con_watchDogTimer;

    switch (con_fsmState) {
    case OPN_IDLE:
        if (*piSHL_Enable != 1) {
            if (!siSHL_OpnRep.empty()) {
                // Drain any potential status data
                siSHL_OpnRep.read(con_opnRep);
                printWarn(myName, "Draining unexpected residue from the \'OpnRep\' stream. As a result, request to close sessionId=%d.\n", con_opnRep.sessId.to_uint());
                soSHL_ClsReq.write(con_opnRep.sessId);
            }
        }
        else {
            con_fsmState = OPN_REQ;
        }
        break;
    case OPN_REQ:
        if (!siRDp_OpnSockReq.empty() and !siRDp_TxCountReq.empty() and
            !soSHL_OpnReq.full()) {
            siRDp_TxCountReq.read(con_txBytesReq);
            SockAddr remoteSockAddr = siRDp_OpnSockReq.read();
            soSHL_OpnReq.write(remoteSockAddr);
            if (DEBUG_LEVEL & TRACE_CON) {
                printInfo(myName, "Client is requesting to connect to remote socket:\n");
                printSockAddr(myName, remoteSockAddr);
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
            siSHL_OpnRep.read(con_opnRep);
            if (con_opnRep.tcpState == ESTABLISHED) {
                if (DEBUG_LEVEL & TRACE_CON) {
                    printInfo(myName, "Client successfully established connection.\n");
                }
                con_fsmState = OPN_8801;
            }
            else {
                 printError(myName, "Client failed to establish connection with remote socket (TCP state is '%s'):\n",
                            getTcpStateName(con_opnRep.tcpState));
                 con_fsmState = OPN_IDLE;
            }
        }
        else {
            if (con_watchDogTimer == 0) {
                if (DEBUG_LEVEL & TRACE_CON) {
                    printError(myName, "Timeout: Failed to establish connection.\n");
                }
                #ifndef __SYNTHESIS__
                  con_watchDogTimer = 250;
                #else
                  con_watchDogTimer = 10000;
                #endif
            }
        }
        break;
    case OPN_8801:
        if (con_txBytesReq != 0) {
            if(!soWRp_TxBytesReq.full() and !soWRp_TxSessId.full()) {
                //-- Request [WRp] to start the xmit test
                soWRp_TxBytesReq.write(con_txBytesReq);
                soWRp_TxSessId.write(con_opnRep.sessId);
                con_fsmState = OPN_IDLE;
            }
        }
        else {
            con_fsmState = OPN_IDLE;
        }
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
 *  By default, the port numbers 5001, 5201, 8800 to 8803 will always be opened
 *   in listen mode at startup. Later on, we should be able to open more ports
 *   if we provide some configuration register for the user to specify new ones.
 *  FYI - The PortTable (PRt) of the SHELL/NTS/TOE supports two port ranges; one
 *   for static ports (0 to 32,767) which are used for listening ports, and one
 *   for dynamically assigned or ephemeral ports (32,768 to 65,535) which are
 *   used for active connections. Therefore, listening port numbers must always
 *   fall in the range 0 to 32,767.
 *******************************************************************************/
void pListen(
        CmdBit                *piSHL_Enable,
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
    static ap_uint<3>          lsn_i = 0;
    #pragma HLS reset variable=lsn_i

    //-- STATIC ARRAYS --------------------------------------------------------
    static const TcpPort LSN_PORT_TABLE[6] = { RECV_MODE_LSN_PORT, XMIT_MODE_LSN_PORT,
                                               ECHO_MOD2_LSN_PORT, ECHO_MODE_LSN_PORT,
                                               IPERF_LSN_PORT,     IPREF3_LSN_PORT };
    #pragma HLS RESOURCE variable=LSN_PORT_TABLE core=ROM_1P

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static ap_uint<8>  lsn_watchDogTimer;

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
                soSHL_LsnReq.write(XMIT_MODE_LSN_PORT);
                break;
            case 2:
                soSHL_LsnReq.write(ECHO_MOD2_LSN_PORT);
                break;
            case 3:
                soSHL_LsnReq.write(ECHO_MODE_LSN_PORT);
                break;
            case 4:
                soSHL_LsnReq.write(IPERF_LSN_PORT);
                break;
            case 5:
                soSHL_LsnReq.write(IPREF3_LSN_PORT);
                break;
            }
            if (DEBUG_LEVEL & TRACE_LSN) {
                printInfo(myName, "Server is requested to listen on port #%d (0x%4.4X).\n",
                          LSN_PORT_TABLE[lsn_i].to_uint(), LSN_PORT_TABLE[lsn_i].to_uint());
            }
            #ifndef __SYNTHESIS__
                lsn_watchDogTimer = 10;
            #else
                lsn_watchDogTimer = 100;
            #endif
            lsn_fsmState = LSN_WAIT_REP;
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
                printInfo(myName, "Received OK listen reply from [TOE] for port %d.\n", LSN_PORT_TABLE[lsn_i].to_uint());
                if (lsn_i == sizeof(LSN_PORT_TABLE)/sizeof(LSN_PORT_TABLE[0])-1) {
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
}  // End-of: pListen()

/*******************************************************************************
 * @brief Read Request Handler (RRh)
 *
 * @param[in]  siSHL_Notif  A new Rx data notification from [SHELL].
 * @param[out] soSHL_DReq   An Rx data request to [SHELL].
 * @param[out] soRDp_FwdCmd A command telling the ReadPath (RDp) to keep/drop a stream.
 *
 * @details
 *  This process waits for a notification from [TOE] indicating the availability
 *   of new data for the TcpApplication Flash (TAF) process of the [ROLE]. If
 *   the TCP segment length of the notification message is greater than 0, the
 *   data segment is valid and the notification is accepted.
 *  For testing purposes, the TCP destination port is evaluated here and one of
 *   the following actions is taken upon its value:
 *     - 8800 : The RxPath (RXp) process is requested to dump/sink this segment.
 *              This is an indirect way for a remote client to run iPerf on that
 *              FPGA port used here as a server.
 *     - 8801 : The TxPath (TXp) is expected to open an active connection and
 *              to send an certain amount of bytes to the producer of this
 *              request. It is the responsibility of the RxPath (RXp) to extract
 *              the TCP destination socket {IpAddr, PortNum} and the #bytes to
 *              transmit, out of the 64 first bits of the segment and to forward
 *              these data to the Connect (COn) process. The [COn] will then
 *              open an active connection before triggering the WritePath (WRp)
 *              to send the requested amount of bytes on the new connection.
 *     - 5001 : [TBD]
 *     - 5201 : [TBD]
 *     - Others: The RXp process is requested to forward these data and metadata
 *              streams to the TcpApplicationFlash (TAF).
 *******************************************************************************/
void pReadRequestHandler(
        stream<TcpAppNotif>    &siSHL_Notif,
        stream<TcpAppRdReq>    &soSHL_DReq,
        stream<ForwardCmd>     &soRDp_FwdCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "RRh");

    //-- LOCAL STATIC VARIABLES W/ RESET ---------------------------------------
    static enum FsmStates { RRH_IDLE, RRH_SEND_DREQ } \
                               rrh_fsmState=RRH_IDLE;
    #pragma HLS reset variable=rrh_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static   TcpAppNotif rrh_notif;

    switch(rrh_fsmState) {
    case RRH_IDLE:
        if (!siSHL_Notif.empty() and !soRDp_FwdCmd.full()) {
            siSHL_Notif.read(rrh_notif);
            if (rrh_notif.tcpSegLen != 0) {
                // Always request the data segment to be received
                switch (rrh_notif.tcpDstPort) {
                case RECV_MODE_LSN_PORT: // 8800
                    soRDp_FwdCmd.write(ForwardCmd(rrh_notif.sessionID, CMD_DROP, NOP));

                    break;
                case XMIT_MODE_LSN_PORT: // 8801
                    soRDp_FwdCmd.write(ForwardCmd(rrh_notif.sessionID, CMD_DROP, GEN));
                    break;
                default:
                    soRDp_FwdCmd.write(ForwardCmd(rrh_notif.sessionID, CMD_KEEP, NOP));
                    break;
                }
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
            rrh_fsmState = RRH_IDLE;
        }
        break;
    }
}

/*******************************************************************************
 * @brief Read Path (RDp)
 *
 * @param[in]  siSHL_Data   Data stream from [SHELL].
 * @param[in]  siSHL_Meta   Session Id from [SHELL].
 * @param[in]  siRRh_FwdCmd A command to keep/drop a stream from ReadRequestHandler (RRh).
 * @param[out] soCOn_OpnSockReq The remote socket to open to Connect (COn).
 * @param[out] soCOn_TxCountReq The #bytes to be transmitted after active connection is opened by [COn].
 * @param[out] soTAF_Data   Data stream to [TAF].
 * @param[out] soTAF_Meta   Session Id to [TAF].
 *
 * @details
 *  This process waits for a new data segment to read and forwards it to the
 *   TcpApplicationFlash (TAF) process or drops it based upon the 'action' field
 *   of the 'siRRh_FwdCmd' command.
 *   - If the action is 'CMD_KEEP', the stream is forwarded to the next layer.
 *     As such, [RDp] implements a pipe for the TCP traffic from [SHELL] to [TAF].
 *   - If the action is 'CMD_DROP' the field 'opCOde' becomes meaningful and may
 *     specify additional sub-options:
 *     - If the op-code is 'NOP', simply drop the stream and do nothing more.
 *     - If the op-code is 'GEN', extract the remote socket to connect to as
 *       well as the number of bytes to transmit out of the 64 first incoming
 *       bits of the data stream. Next, drop the rest of that stream and forward
 *       the extracted fields to the Connect (COn) process.
 *******************************************************************************/
void pReadPath(
        stream<TcpAppData>  &siSHL_Data,
        stream<TcpAppMeta>  &siSHL_Meta,
        stream<ForwardCmd>  &siRRh_FwdCmd,
        stream<SockAddr>    &soCOn_OpnSockReq,
        stream<Ly4Len>      &soCOn_TxCountReq,
        stream<TcpAppData>  &soTAF_Data,
        stream<TcpAppMeta>  &soTAF_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "RDp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { RDP_IDLE=0, RDP_STREAM, RDP_8801 } \
                               rdp_fsmState=RDP_IDLE;
    #pragma HLS reset variable=rdp_fsmState
    static FlagBool            rdp_keepFlag=true;
    #pragma HLS reset variable=rdp_keepFlag

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TcpAppData  appData;
    TcpAppMeta  appMeta;
    ForwardCmd  fwdCmd;

    switch (rdp_fsmState ) {
    case RDP_IDLE:
        if (!siSHL_Meta.empty() and !siRRh_FwdCmd.empty() and !soTAF_Meta.full()) {
            siSHL_Meta.read(appMeta);
            siRRh_FwdCmd.read(fwdCmd);
            if (fwdCmd.action == CMD_KEEP) {
                rdp_keepFlag = true;
                soTAF_Meta.write(appMeta);
                rdp_fsmState  = RDP_STREAM;
            }
            else {
                rdp_keepFlag = false;
                if (fwdCmd.dropCode == GEN) {
                    rdp_fsmState  = RDP_8801;
                }
                else {
                    rdp_fsmState  = RDP_STREAM;
                }
            }
        }
        break;
    case RDP_STREAM:
        if (!siSHL_Data.empty() and !soTAF_Data.full()) {
            siSHL_Data.read(appData);
            if (rdp_keepFlag) {
                soTAF_Data.write(appData);
                if (DEBUG_LEVEL & TRACE_RDP) { printAxisRaw(myName, "soTAF_Data =", appData); }
            }
            if (appData.getTLast()) {
                rdp_fsmState  = RDP_IDLE;
            }
        }
        break;
    case RDP_8801:
        if (!siSHL_Data.empty() and !soCOn_OpnSockReq.full() and !soCOn_TxCountReq.full()) {
            // Extract the remote socket address and the requested #bytes to transmit
            siSHL_Data.read(appData);
            //OBSOLETE_20200908 TcpPort portToOpen = byteSwap16(appData.getLE_TData(15,  0));
            //OBSOLETE_20200908 Ly4Len  bytesToSend = byteSwap16(appData.getLE_TData(31, 16));
            SockAddr sockToOpen(byteSwap32(appData.getLE_TData(31,  0)),   // IP4 address
                                byteSwap16(appData.getLE_TData(47, 32)));  // TCP port
            Ly4Len bytesToSend = byteSwap16(appData.getLE_TData(63, 48));
            soCOn_OpnSockReq.write(sockToOpen);
            soCOn_TxCountReq.write(bytesToSend);
            if (DEBUG_LEVEL & TRACE_RDP) {
                printInfo(myName, "Received request for Tx test mode to generate a segment of length=%d and to send it to socket:\n",
                          bytesToSend.to_int());
                printSockAddr(myName, sockToOpen);
            }
            if (appData.getTLast()) {
                rdp_fsmState  = RDP_IDLE;
            }
            else {
                rdp_fsmState = RDP_STREAM;
            }
        }
    }
}

/*******************************************************************************
 * @brief Write Path (WRp)
 *
 * @param[in]  siTAF_Data   Tx data stream from [ROLE/TAF].
 * @param[in]  siTAF_Meta   The session Id from [ROLE/TAF].
 * @param[in]  siCOn_TxBytesReq The #bytes to be transmitted on the active opened connection from Connect(COn).
 * @param[in]  siCOn_SessId The session id of the active opened connection from [COn].
 * @param[out] soSHL_Data   Tx data to [SHELL].
 * @param[out] soSHL_Meta   Tx session Id to to [SHELL].
 * @param[in]  siSHL_DSts   Tx data write status from [SHELL].
 *
 * @details
 *  This process waits for a new data segment to write from the TcpAppFlash (TAF)
 *   and forwards it to the [SHELL]. As such, it implements a pipe for the TCP
 *   traffic from [TAF] to [SHELL].
 *******************************************************************************/
void pWritePath(
        stream<TcpAppData>  &siTAF_Data,
        stream<TcpAppMeta>  &siTAF_Meta,
        stream<Ly4Len>      &siCOn_TxBytesReq,
        stream<SessionId>   &siCOn_TxSessId,
        stream<TcpAppData>  &soSHL_Data,
        stream<TcpAppMeta>  &soSHL_Meta,
        stream<TcpAppWrSts> &siSHL_DSts)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "WRp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { WRP_IDLE=0, WRP_STREAM, WRP_8801 } \
                               wrp_fsmState=WRP_IDLE;
    #pragma HLS reset variable=wrp_fsmState
    static enum GenChunks { CHK0=0, CHK1, } \
                               wrp_genChunk=CHK0;
    #pragma HLS reset variable=wrp_genChunk

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static Ly4Len       wrp_txBytesReq;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TcpAppMeta   appMeta;
    TcpAppData   appData;

    switch (wrp_fsmState) {
    case WRP_IDLE:
        //-- Always read the metadata provided by [ROLE/TAF]
        if (!siTAF_Meta.empty() and !soSHL_Meta.full()) {
            siTAF_Meta.read(appMeta);
            soSHL_Meta.write(appMeta);
            if (DEBUG_LEVEL & TRACE_WRP) {
                printInfo(myName, "Received new session ID #%d from [ROLE].\n",
                          appMeta.to_uint());
            }
            wrp_fsmState = WRP_STREAM;
        }
        else if (!siCOn_TxBytesReq.empty() and !siCOn_TxSessId.empty()) {
            siCOn_TxBytesReq.read(wrp_txBytesReq);
            siCOn_TxSessId.read(appMeta);
            if (DEBUG_LEVEL & TRACE_WRP) {
                printInfo(myName, "Received a Tx test request for %d bytes (appMeta=%d).\n",
                          wrp_txBytesReq.to_uint(), appMeta.to_uint());
            }
            if (wrp_txBytesReq != 0) {
                soSHL_Meta.write(appMeta);
                wrp_fsmState = WRP_8801;
                wrp_genChunk = CHK0;
            }
            else {
                wrp_fsmState = WRP_IDLE;
            }
        }
        break;
    case WRP_STREAM:
        if (!siTAF_Data.empty() and !soSHL_Data.full()) {
            siTAF_Data.read(appData);
            soSHL_Data.write(appData);
            if (DEBUG_LEVEL & TRACE_WRP) { printAxisRaw(myName, "soSHL_Data =", appData); }
            if(appData.getTLast()) {
                wrp_fsmState = WRP_IDLE;
            }
        }
        break;
    case WRP_8801:
        if (!soSHL_Data.full()) {
            TcpAppData currChunk(0,0,0);
            if (wrp_txBytesReq > 8) {
                currChunk.setLE_TKeep(0xFF);
                wrp_txBytesReq -= 8;
            }
            else {
                currChunk.setLE_TKeep(lenToLE_tKeep(wrp_txBytesReq));
                currChunk.setLE_TLast(TLAST);
                wrp_fsmState = WRP_IDLE;
            }
            switch (wrp_genChunk) {
            case CHK0: // Send 'Hi from '
                currChunk.setTData(GEN_CHK0);
                wrp_genChunk = CHK1;
                break;
            case CHK1: // Send 'FMKU60!\n'
                currChunk.setTData(GEN_CHK1);
                wrp_genChunk = CHK0;
                break;
            }
            currChunk.clearUnusedBytes();
            soSHL_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_WRP) { printAxisRaw(myName, "soSHL_Data =", currChunk); }
        }
        break;
    } // End-of: switch

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
        stream<TcpAppClsReq>  &soSHL_ClsReq)
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

  #endif

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //--------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    //-- Read Path (RDp)
    static stream<SockAddr>        ssRDpToCOn_OpnSockReq ("ssRDpToCOn_OpnSockReq");
    #pragma HLS stream    variable=ssRDpToCOn_OpnSockReq depth=2
    static stream<Ly4Len>          ssRDpToCOn_TxCountReq ("ssRDpToCOn_TxCountReq");
    #pragma HLS stream    variable=ssRDpToCOn_TxCountReq depth=2

    //-- ReadrequestHandler (RRh)
    static stream<ForwardCmd>      ssRRhToRDp_FwdCmd     ("ssRRhToRDp_FwdCmd");
    #pragma HLS stream    variable=ssRRhToRDp_FwdCmd     depth=8
    #pragma HLS DATA_PACK variable=ssRRhToRDp_FwdCmd

    //-- Connect (COn)
    static stream<Ly4Len>          ssCOnToWRp_TxBytesReq ("ssCOnToWRp_TxBytesReq");
    #pragma HLS stream    variable=ssCOnToWRp_TxBytesReq depth=2
    static stream<SessionId>       ssCOnToWRp_TxSessId   ("ssCOnToWRp_TxSessId");
    #pragma HLS stream    variable=ssCOnToWRp_TxSessId   depth=2

    //-- PROCESS FUNCTIONS -----------------------------------------------------
    pConnect(
            piSHL_Mmio_En,
            ssRDpToCOn_OpnSockReq,
            ssRDpToCOn_TxCountReq,
            ssCOnToWRp_TxBytesReq,
            ssCOnToWRp_TxSessId,
            soSHL_OpnReq,
            siSHL_OpnRep,
            soSHL_ClsReq);

    pListen(
            piSHL_Mmio_En,
            soSHL_LsnReq,
            siSHL_LsnRep);

    pReadRequestHandler(
            siSHL_Notif,
            soSHL_DReq,
            ssRRhToRDp_FwdCmd);

    pReadPath(
            siSHL_Data,
            siSHL_Meta,
            ssRRhToRDp_FwdCmd,
            ssRDpToCOn_OpnSockReq,
            ssRDpToCOn_TxCountReq,
            soTAF_Data,
            soTAF_Meta);

    pWritePath(
            siTAF_Data,
            siTAF_Meta,
            ssCOnToWRp_TxBytesReq,
            ssCOnToWRp_TxSessId,
            soSHL_Data,
            soSHL_Meta,
            siSHL_DSts);

}

/*! \} */
