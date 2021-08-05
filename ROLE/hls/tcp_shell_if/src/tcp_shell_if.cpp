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
 * Component   : cFp_Monolithic / ROLE
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
 * ARCHITECTURE DIRECTIVE
 *  The ReadRequestHandler (RRh) of TSIF can be
 *  implemented with an interrupt handler and
 *  scheduler approach by setting the following
 *  pre-processor directive.
 ************************************************/
#undef USE_INTERRUPTS

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TSIF"  // TcpShellInterface

#define TRACE_OFF      0x0000
#define TRACE_IRB     1 <<  1
#define TRACE_RDP     1 <<  2
#define TRACE_WRP     1 <<  3
#define TRACE_LSN     1 <<  4
#define TRACE_CON     1 <<  5
#define TRACE_RNH     1 <<  6
#define TRACE_RRH     1 <<  7
#define TRACE_RRM     1 <<  8
#define TRACE_ALL      0xFFFF
#define DEBUG_LEVEL (TRACE_OFF)

/*******************************************************************************
 * @brief Stream Data Mover - Moves data chunks from incoming stream to outgoing
 *  stream with blocking read and write access methods.
 *
 * @param[in]  si  Stream in.
 * @param[out] so  Stream out.
 *******************************************************************************/
template <class Type>
void pStreamDataMover(
        stream<Type>& si,
        stream<Type>& so)
{
    #pragma HLS INLINE
    Type currChunk;
    si.read(currChunk);  // Blocking read
    so.write(currChunk); // Blocking write
    // [TODO] so.write(si.read());
}

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
 *  Alternatively, the process is also used to trigger the TxPath (TXp) to
 *   transmit a segment to the newly opened connection.
 *  The switch between opening a connection and sending traffic a remote host is
 *   defined by the value of the 'siRDp_TxCountReq' input:
 *     1) If 'siRDp_TxCountReq' == 0, the process opens a new connection with
 *        the remote host specified by 'siRDp_OpnSockReq'.
 *     2) If 'siRDp_TxCountReq' != 0, the process triggers the TxPath (TXp) to
 *        transmit a segment to the *LAST* opened connection.
 *        The number of bytes to transmit is specified by 'siRDp_TxCountReq'.
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
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "COn");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates{ CON_IDLE, CON_RD_RDp,  CON_WR_WRp,    \
                                     CON_OPN_REQ, CON_OPN_REP } \
                               con_fsmState=CON_IDLE;
    #pragma HLS reset variable=con_fsmState
    static SockAddr            con_testSockAddr;
    #pragma HLS reset variable=con_testSockAddr

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static TcpAppOpnRep  con_opnRep;
    static Ly4Len        con_txBytesReq;
    static ap_uint< 12>  con_watchDogTimer;

    switch (con_fsmState) {
    case CON_IDLE:
        if (*piSHL_Enable != 1) {
            if (!siSHL_OpnRep.empty()) {
                // Drain any potential status data
                siSHL_OpnRep.read(con_opnRep);
                printWarn(myName, "Draining unexpected residue from the \'OpnRep\' stream. As a result, request to close sessionId=%d.\n", con_opnRep.sessId.to_uint());
                soSHL_ClsReq.write(con_opnRep.sessId);
            }
        }
        else {
            con_fsmState = CON_RD_RDp;
        }
        break;
    case CON_RD_RDp:
        if (!siRDp_OpnSockReq.empty() and !siRDp_TxCountReq.empty()) {
            siRDp_TxCountReq.read(con_txBytesReq);
            SockAddr currSockAddr = siRDp_OpnSockReq.read();
            if (con_txBytesReq == 0) {
                con_fsmState = CON_OPN_REQ;
                con_testSockAddr = currSockAddr;
                if (DEBUG_LEVEL & TRACE_CON) {
                    printInfo(myName, "Client is requesting to connect to new remote socket:\n");
                              printSockAddr(myName, con_testSockAddr);
                }
            }
            else if (currSockAddr == con_testSockAddr) {
                con_fsmState = CON_WR_WRp;
                if (DEBUG_LEVEL & TRACE_CON) {
                    printInfo(myName, "Client is requesting the FPGA to send %d bytes to the last opened socket:\n", con_txBytesReq.to_uint());
                              printSockAddr(myName, currSockAddr);
                }
            }
            else {
                con_fsmState = CON_RD_RDp;
                printInfo(myName, "Client is requesting the FPGA to send traffic to a none opened connection:\n");
                printSockAddr(myName, currSockAddr);
                printFatal(myName, "Error.\n");
            }
        }
        break;
    case CON_OPN_REQ:
        if (!soSHL_OpnReq.full()) {
            soSHL_OpnReq.write(con_testSockAddr);
            #ifndef __SYNTHESIS__
                con_watchDogTimer = 250;
            #else
                con_watchDogTimer = 10000;
            #endif
            con_fsmState = CON_OPN_REP;
        }
        break;
    case CON_OPN_REP:
        con_watchDogTimer--;
        if (!siSHL_OpnRep.empty()) {
            // Read the reply stream
            siSHL_OpnRep.read(con_opnRep);
            if (con_opnRep.tcpState == ESTABLISHED) {
                if (DEBUG_LEVEL & TRACE_CON) {
                    printInfo(myName, "Client successfully established connection.\n");
                }
            }
            else {
                 printError(myName, "Client failed to establish connection with remote socket (TCP state is '%s'):\n",
                            getTcpStateName(con_opnRep.tcpState));
            }
            con_fsmState = CON_IDLE;
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
    case CON_WR_WRp:
        if(!soWRp_TxBytesReq.full() and !soWRp_TxSessId.full()) {
            //-- Request [WRp] to start the xmit test
            soWRp_TxBytesReq.write(con_txBytesReq);
            soWRp_TxSessId.write(con_opnRep.sessId);
            con_fsmState = CON_IDLE;
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
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName = concat3(THIS_NAME, "/", "LSn");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { LSN_IDLE, LSN_SEND_REQ, LSN_WAIT_REP, LSN_DONE } \
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
                if (DEBUG_LEVEL & TRACE_LSN) {
                    printInfo(myName, "Received OK listen reply from [TOE] for port %d.\n", LSN_PORT_TABLE[lsn_i].to_uint());
                }
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
                printError(myName, "Timeout: Server failed to listen on port %d (0x%4.4X).\n",
                        LSN_PORT_TABLE[lsn_i].to_uint(), LSN_PORT_TABLE[lsn_i].to_uint());
                lsn_fsmState = LSN_SEND_REQ;
            }
        }
        break;
    case LSN_DONE:
        break;
    }  // End-of: switch()
}  // End-of: pListen()

#if defined USE_INTERRUPTS
/*******************************************************************************
 * @brief Input Read Buffer (IRb)
 *
 * @param[in]  piSHL_Enable  Enable signal from [SHELL].
 * @param[in]  siSHL_Data    Data stream from [SHELL].
 * @param[in]  siSHL_Meta    Session Id from [SHELL].
 * @param[out] soRRh_EnquSig Signals the enqueue of a new chunk to ReadRequestHandler (RRh).
 * @param[out] soRDp_Data    Data stream to ReadPath (RDp).
 * @param[out] soRDp_Meta    Metadata stream [RDp].
 *
 * @details
 *  This process counts and enqueues the incoming data segments and their meta-
 *  data into two FIFOs. The goal is to provision a buffer to store all the
 *  bytes that were requested from the TCP Rx buffer by the ReadRequestHandler
 *  (RRh) process. If this process does not absorb the incoming data stream fast
 *  enough, the [TOE] will start dropping segments on his side to avoid any
 *  blocking situation.
 *******************************************************************************/
void pInputReadBuffer(
        CmdBit              *piSHL_Enable,
        stream<TcpAppData>  &siSHL_Data,
        stream<TcpAppMeta>  &siSHL_Meta,
        stream<SigBit>      &soRRh_EnquSig,
        stream<TcpAppData>  &soRDp_Data,
        stream<TcpAppMeta>  &soRDp_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "IRb");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { IRB_IDLE=0,    IRB_STREAM } \
                               irb_fsmState=IRB_IDLE;
    #pragma HLS reset variable=irb_fsmState

    if (*piSHL_Enable != 1) {
        return;
    }

    switch (irb_fsmState ) {
    case IRB_IDLE:
        if (!siSHL_Meta.empty() and !soRDp_Meta.full()) {
            soRDp_Meta.write(siSHL_Meta.read());
            irb_fsmState = IRB_STREAM;
        }
        break;
    case IRB_STREAM:
        if (!siSHL_Data.empty() and !soRDp_Data.full() and !soRRh_EnquSig.full()) {
            TcpAppData currChunk = siSHL_Data.read();
            soRDp_Data.write(currChunk);
            soRRh_EnquSig.write(1);
            if (currChunk.getTLast()) {
                irb_fsmState = IRB_IDLE;
            }
        }
        break;
    }
}

/*******************************************************************************
 * @brief Rx Buffer Occupancy (Rxb) -
 *   Keeps track of the occupancy of the input read buffer.
 *
 * @param[in]  siEnqueueSig Signals the enqueue of a chunk in the buffer.
 * @param[in]  siEnqueueSig Signals the dequeue of a chunk from the buffer.
 * @param[out] soFreeSpace  The available space in the input buffer (in bytes).
 *******************************************************************************/
void pRxBufferOccupancy(
        stream<SigBit>                                 &siEnqueueSig,
        stream<SigBit>                                 &siDequeueSig,
        stream<ap_uint<log2Ceil<cIBuffBytes>::val+1> > &soFreeSpace)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "RRh/Rxb");

    static ap_uint<log2Ceil<cIBuffBytes>::val+1>  rrh_freeSpace=cIBuffBytes;
    #pragma HLS reset                    variable=rrh_freeSpace

    bool traceInc = false;
    bool traceDec = false;

    if (!siEnqueueSig.empty() and siDequeueSig.empty()) {
        siEnqueueSig.read();
        rrh_freeSpace -= (ARW/8);
        traceInc = true;
    } else if (siEnqueueSig.empty() and !siDequeueSig.empty()) {
        siDequeueSig.read();
        rrh_freeSpace += (ARW/8);
        traceDec = true;
    } else if (!siEnqueueSig.empty() and !siDequeueSig.empty()) {
        siEnqueueSig.read();
        siDequeueSig.read();
        traceInc = true;
        traceDec = true;
    }
    //-- Always
    soFreeSpace.write(rrh_freeSpace);

    if (DEBUG_LEVEL & TRACE_RRH_IBO) {
        if (traceInc or traceDec) {
            printInfo(myName, "Input buffer occupancy = %d bytes (%c|%c)\n",
                     (cIBuffBytes - rrh_freeSpace.to_uint()),
                     (traceInc ? '+' : ' '), (traceDec ? '-' : ' '));
        }
    }
}

/*******************************************************************************
 * @brief Post a new notification into the Rx interrupt table.
 *
 * @param[in]  siSHL_Notif    A new Rx data notification from [SHELL].
 * @param[out] soRit_InterruptQry  Interrupt query to RxInterruptTable(Rit).
 * @param[in]  siRit_InterruptRep  Interrupt reply from [Rit].
 * @param[out] soRst_SessId   Session Id to RxSchedulerRequest(Rsr).
 *
 * @details
 *  This 'POST' process reads the incoming notification from the shell and
 *   updates the Rx interrupt table accordingly. The interrupt table is used
 *   to keep track of the TCP destination port and the total amount of pending
 *   bytes for a given session.
 *  After that, the session Id of the incoming notification is forwarded to the
 *   RxSchedulerRequest (Rsr).
 *
 * @warning
 *  The incoming notification is only added to the interrupt table when the TCP
 *   segment length is greater than 0.
 *******************************************************************************/
void pRxPostNotification(
        stream<TcpAppNotif>     &siSHL_Notif,
        stream<InterruptQuery>  &soRit_InterruptQry,
        stream<InterruptEntry>  &siRit_InterruptRep,
        stream<SessionId>       &soRsr_SessId)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "RRh/Rxp");

    //-- STATIC VARIABLES W/ RESET ---------------------------------------------
    static enum PostFsmStates { RPN_IDLE, RPN_INC, RPN_POST } \
                                  rpn_fsmState=RPN_IDLE;
    #pragma HLS reset    variable=rpn_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static TcpAppNotif     rpn_notif;
    static TcpDatLen       rpn_newPendingByteCnt;

    //-- PROCESS FUNCTION ------------------------------------------------------

    switch (rpn_fsmState) {
    case RPN_IDLE:
        if (!siSHL_Notif.empty() and !soRit_InterruptQry.full()) {
            rpn_notif = siSHL_Notif.read();
            if (rpn_notif.tcpDatLen != 0) {
                InterruptQuery getQuery(rpn_notif.sessionID, GET);
                soRit_InterruptQry.write(getQuery);
                rpn_fsmState = RPN_INC;
            }
        }
        break;
    case RPN_INC:
        if (!siRit_InterruptRep.empty()) {
            InterruptEntry currEntry = siRit_InterruptRep.read();
            rpn_newPendingByteCnt = currEntry.byteCnt + rpn_notif.tcpDatLen;
            rpn_fsmState = RPN_POST;
        }
        break;
    case RPN_POST:
        if (!soRit_InterruptQry.full() and !soRsr_SessId.full()) {
            InterruptEntry currEntry(rpn_newPendingByteCnt, rpn_notif.tcpDstPort);
            InterruptQuery postQuery(rpn_notif.sessionID, currEntry);
            soRit_InterruptQry.write(postQuery);
            soRsr_SessId.write(rpn_notif.sessionID);
            rpn_fsmState = RPN_IDLE;
        }
        break;
    }
}

/*******************************************************************************
 * @brief Receive Interrupt Table (Rit).
 *
 * @param[in]  siRpn_InterruptQry  Interrupt query from RxPostNotification (Rpn).
 * @param[out] soRpn_InterruptRep  Interrupt reply to [Rpn].
 * @param[in]  siRsr_InterruptQry  Interrupt query from RxSchedulerRequest (Rsr).
 * @param[out] soRsr_InterruptRep  Interrupt reply to [Rsr].
 *
 * @details
 *  This process implements the interrupt table which keeps track on the number
 *   of pending bytes per session as well as the TCP destination port.
 *  The table is a dual-port RAM with 'POST' writing on one port while 'SCHED'
 *   reads from the other one.
 *  Upon reception of a 'POST' or 'PUT' query, the process also maintains a
 *   bitmap encoded vector that represents the sessions that have pending bytes.
 *   For example, if bitmap[0] is set, it indicates that session #0 has pending
 *   bytes.
 *******************************************************************************/
void pRxInterruptTable(
        stream<InterruptQuery>  &siRpn_InterruptQry,
        stream<InterruptEntry>  &soRpn_InterruptRep,
        stream<InterruptQuery>  &siRsr_InterruptQry,
        stream<InterruptEntry>  &soRsr_InterruptRep)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName = concat3(THIS_NAME, "/", "RRh/Rit");

    //-- STATIC ARRAYS ---------------------------------------------------------
    static InterruptEntry                     INTERRUPT_TABLE[cMaxSessions];
    #pragma HLS RESOURCE             variable=INTERRUPT_TABLE core=RAM_2P
    #pragma HLS DEPENDENCE           variable=INTERRUPT_TABLE inter false

    //-- STATIC VARIABLES W/ RESET ---------------------------------------------
    static bool                                 rit_isInit=false;
    #pragma HLS reset                  variable=rit_isInit
    static ap_uint<log2Ceil<cMaxSessions>::val> rit_initEntry=0;
    #pragma HLS reset                  variable=rit_initEntry

    static enum FsmStates { RIT_IDLE, RIT_NOTIF_REQ, RIT_NOTIF_REP, RIT_NOTIF_UPDATE, \
                                      RIT_SCHED_REQ, RIT_SCHED_REP, RIT_SCHED_UPDATE } rit_fsmState=RIT_IDLE;

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static InterruptEntry  rit_notifEntry;
    static InterruptEntry  rit_schedEntry;
    static InterruptQuery  rit_notifQuery;
    static InterruptQuery  rit_schedQuery;

    //-- PROCESS FUNCTION ------------------------------------------------------
    if (!rit_isInit) {
        //-- This the table must be cleared upon reset
        INTERRUPT_TABLE[rit_initEntry] = InterruptEntry(0, 0);
        if (rit_initEntry == (cMaxSessions-1)) {
            rit_isInit = true;
            if (DEBUG_LEVEL & TRACE_RRH) {
                printInfo(myName, "Done with initialization of INTERRUPT_TABLE.\n");
            }
        } else {
            rit_initEntry += 1;
        }
    }
    else {
        switch (rit_fsmState) {
            case RIT_IDLE:
                if (!siRpn_InterruptQry.empty()) {
                    rit_notifQuery = siRpn_InterruptQry.read();
                    if (rit_notifQuery.action != GET) {
                        printFatal(myName, "Expecting a query 'GET' request!\n");
                    }
                    rit_fsmState = RIT_NOTIF_REQ;
                }
                else if (!siRsr_InterruptQry.empty()) {
                    rit_schedQuery = siRsr_InterruptQry.read();
                    if (rit_schedQuery.action != GET) {
                        printFatal(myName, "Expecting a query 'GET' request!\n");
                    }
                    rit_fsmState = RIT_SCHED_REQ;
                }
                break;
            case RIT_NOTIF_REQ:
                rit_notifEntry = INTERRUPT_TABLE[rit_notifQuery.sessId];
                rit_fsmState = RIT_NOTIF_REP;
                break;
            case RIT_NOTIF_REP:
                if (!soRpn_InterruptRep.full()) {
                    soRpn_InterruptRep.write(rit_notifEntry);
                    rit_fsmState = RIT_NOTIF_UPDATE;
                }
                break;
            case RIT_SCHED_REQ:
                rit_schedEntry = INTERRUPT_TABLE[rit_schedQuery.sessId];
                rit_fsmState = RIT_SCHED_REP;
                break;
            case RIT_SCHED_REP:
                if (!soRsr_InterruptRep.full()) {
                    soRsr_InterruptRep.write(rit_schedEntry);
                    rit_fsmState = RIT_SCHED_UPDATE;
                }
                break;
            case RIT_NOTIF_UPDATE:
                if (!siRpn_InterruptQry.empty()) {
                    InterruptQuery notifQry = siRpn_InterruptQry.read();
                    if (notifQry.action == POST) {
                        INTERRUPT_TABLE[notifQry.sessId] = InterruptEntry(notifQry.entry);
                    }
                    else if (notifQry.action == PUT) {
                        INTERRUPT_TABLE[notifQry.sessId].byteCnt = notifQry.entry.byteCnt;
                    }
                    else {
                        printFatal(myName, "Expecting a query 'PUT' or 'PUSH' request!\n");
                    }
                    rit_fsmState = RIT_IDLE;
                }
                break;
            case RIT_SCHED_UPDATE:
                if (!siRsr_InterruptQry.empty()) {
                    InterruptQuery schedQry = siRsr_InterruptQry.read();
                    if (schedQry.action == POST) {
                        INTERRUPT_TABLE[schedQry.sessId] = InterruptEntry(schedQry.entry);
                    }
                    else if (schedQry.action == PUT) {
                        INTERRUPT_TABLE[schedQry.sessId].byteCnt = schedQry.entry.byteCnt;
                    }
                    else {
                        printFatal(myName, "Expecting a query 'PUT' or 'PUSH' request!\n");
                    }
                    rit_fsmState = RIT_IDLE;
                }
                break;
        }
    }
}

/*******************************************************************************
 * @brief Rx Handler (Rxs)
 *  Reads the incoming session Id (w/ II=1) and updates the vector of pending
 *  interrupts.
 *******************************************************************************/
void pRxHandler(
        stream<SessionId>              &siRpn_SessId,
        stream<ReqBit>                 &siRxs_SessIdReq,
        stream<SessionId>              &soRxs_SessIdRep,
        stream<SessionId>              &siRxs_ClearInt)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName = concat3(THIS_NAME, "/", "RRh/Rxh");

    //-- STATIC VARIABLES W/ RESET ---------------------------------------------
    static ap_uint<cMaxSessions>    rsr_pendingInterrupts=0;
    #pragma HLS reset      variable=rsr_pendingInterrupts
    static SessionId                rsr_currSess=0;
    #pragma HLS reset      variable=rsr_currSess

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    ap_uint<log2Ceil<cMaxSessions>::val> nextSess=0;
    ap_uint<cMaxSessions>   setVec=0;
    ap_uint<cMaxSessions>   clrVec=0;

    //-- Set interrupt ----------------
    if (!siRpn_SessId.empty()) {
        SessionId sessId = siRpn_SessId.read();
        setVec[sessId] = 1;
        if (DEBUG_LEVEL & TRACE_RRH) {
            printInfo(myName, "Set   interrupt for session #%d.\n", sessId.to_uint());
        }
    }
    //-- Clear interrupt --------------
    if (!siRxs_ClearInt.empty()) {
        SessionId sessId = siRxs_ClearInt.read();
        clrVec[sessId] = 1;
        if (DEBUG_LEVEL & TRACE_RRH) {
            printInfo(myName, "Clear interrupt for session #%d.\n", sessId.to_uint());
        }
    }
    //-- Update interrupt vector ------
    rsr_pendingInterrupts = (rsr_pendingInterrupts xor clrVec) | setVec;

    //-- Forward interrupt
    bool currSessValid = false;
    if (!siRxs_SessIdReq.empty() and !soRxs_SessIdRep.full()) {
        //-- Round-robin scheduler
        for (uint8_t i=0; i<cMaxSessions; ++i) {
            #pragma HLS UNROLL
            nextSess = rsr_currSess + i + 1;
            if (rsr_pendingInterrupts[nextSess] == 1) {
                rsr_currSess = nextSess;
                currSessValid = true;
            }
        }
        if (currSessValid == true) {
            siRxs_SessIdReq.read();
            soRxs_SessIdRep.write(nextSess);
            if (DEBUG_LEVEL & TRACE_RRH) {
                printInfo(myName, "RR-Arbiter has scheduled session #%d.\n", nextSess.to_uint());
            }
        }
    }
}

/*******************************************************************************
 * @brief Rx Scheduler (Rxs)
 *
 * @param[out] soRxh_SessIdReq     Request a session id from the RxHandler (Rxh).
 * @param[in]  siRxh_SessIdRep     A session id reply from [Rxh].
 * @param[in]  siRxb_FreeSpace     The available space (in bytes) from the RxBuffer (Rxb).
 * @param[out] soRit_InterruptQry  Interrupt query to RxInterruptTable(Rit).
 * @param[in]  siRit_InterruptRep  Interrupt reply from [Rit].
 * @param[out] soRxh_ClearInt      A request to clear an interrupt to [Rxh].
 * @param[out] soSHL_DReq          An Rx data request to [SHELL].
 * @param[out] soRDp_FwdCmd        A command telling the ReadPath (RDp) to keep/drop a stream.
 *
 * @detail
 *  This process implements a round-robin scheduler that generates data requests
 *   to TOE based on a bitmap encoded vector that represents the sessions that
 *   have pending bytes to be read. For example, if 'rit_pendingInterrupts[0]'
 *   is set, it indicates that session #0 has pending bytes.
 *  After sending the request to TOE, the pending byte count of the interrupt
 *   table is decreased accordingly.
 *  Next, the scheduler sends a data request to [SHELL] indicating the number of
 *   bytes that it is willing to receive for a given session.
 *******************************************************************************/
void pRxScheduler(
        stream<ReqBit>                 &soRxh_SessIdReq,
        stream<SessionId>              &siRxh_SessIdRep,
        stream<ap_uint<log2Ceil<cIBuffBytes>::val + 1> > &siRxb_FreeSpace,
        stream<InterruptQuery>         &soRit_InterruptQry,
        stream<InterruptEntry>         &siRit_InterruptRep,
        stream<SessionId>              &soRxh_ClearReq,
        stream<TcpAppRdReq>            &soSHL_DReq,
        stream<ForwardCmd>             &soRDp_FwdCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName = concat3(THIS_NAME, "/", "RRh/Rxs");

    //-- STATIC VARIABLES W/ RESET ---------------------------------------------
    static enum FsmStates { RSR_SREQ, RSR_SREP, RSR_DEC, RSR_PUT, RSR_FWD } \
                                                 rsr_fsmState=RSR_SREQ;
    #pragma HLS reset                   variable=rsr_fsmState
    static TcpDatLen                             rsr_freeSpace;
    #pragma HLS reset                   variable=rsr_freeSpace

    //-- STATIC VARIABLES ------------------------------------------------------
    static SessionId              rsr_currSess;
    static InterruptEntry         rsr_intEntry;
    static TcpDatLen              rsr_datLenReq;
    static ap_uint<cMaxSessions>  rsr_pendingInterrupts;

    TcpDatLen                     newByteCnt;

    if (!siRxb_FreeSpace.empty()) {
        rsr_freeSpace = siRxb_FreeSpace.read();
    }

    switch(rsr_fsmState) {
        case RSR_SREQ:
            if (!soRxh_SessIdReq.full()) {
                soRxh_SessIdReq.write(1);
                rsr_fsmState = RSR_SREP;
            }
            break;
        case RSR_SREP:
            if (!siRxh_SessIdRep.empty() and !soRit_InterruptQry.full()) {
                rsr_currSess = siRxh_SessIdRep.read();
                soRit_InterruptQry.write(rsr_currSess);
                if (DEBUG_LEVEL & TRACE_RRH) {
                    printInfo(myName, "Querying [Rit] for session #%d.\n", rsr_currSess.to_uint());
                }
                rsr_fsmState = RSR_DEC;
            }
            break;
        case RSR_DEC:
            if (!siRit_InterruptRep.empty()) {
                // Decrement counter and put it back in the table
                rsr_intEntry  = siRit_InterruptRep.read();
                // Request to TOE = max(rit_schedEntry.byteCnt, rit_freeSSpace)
                rsr_datLenReq = (rsr_freeSpace < rsr_intEntry.byteCnt) ? (rsr_freeSpace) : (rsr_intEntry.byteCnt);
                if (DEBUG_LEVEL & TRACE_RRH) {
                    printInfo(myName, "NotifBytes=%d - FreeSpace=%d \n",
                            rsr_intEntry.byteCnt.to_uint(), rsr_freeSpace.to_uint());
                }
                rsr_fsmState = RSR_PUT;
            }
            break;
        case RSR_PUT:
            if (!soRit_InterruptQry.full() and !soRxh_ClearReq.full()) {
                newByteCnt = rsr_intEntry.byteCnt - rsr_datLenReq;
                soRit_InterruptQry.write(InterruptQuery(rsr_currSess, newByteCnt));
                if (newByteCnt == 0) {
                    soRxh_ClearReq.write(rsr_currSess);
                    if (DEBUG_LEVEL & TRACE_RRH) {
                        printInfo(myName, "Request to clear interrupt #%d.\n", rsr_currSess.to_uint());
                    }
                }
                rsr_fsmState = RSR_FWD;
            }
            break;
        case RSR_FWD:
            if (!soSHL_DReq.full() and !soRDp_FwdCmd.full()) {
                soSHL_DReq.write(TcpAppRdReq(rsr_currSess, rsr_datLenReq));
                switch (rsr_intEntry.dstPort) {
                    case RECV_MODE_LSN_PORT: // 8800
                        soRDp_FwdCmd.write(ForwardCmd(rsr_currSess, rsr_datLenReq, CMD_DROP, NOP));
                        break;
                    case XMIT_MODE_LSN_PORT: // 8801
                        soRDp_FwdCmd.write(ForwardCmd(rsr_currSess, rsr_datLenReq, CMD_DROP, GEN));
                        break;
                    default:
                        soRDp_FwdCmd.write(ForwardCmd(rsr_currSess, rsr_datLenReq, CMD_KEEP, NOP));
                        break;
                }
                if (DEBUG_LEVEL & TRACE_RRH) {
                    printInfo(myName, "Sending DReq(SessId=%2d, DatLen=%4d) to RDp (expected TcpDstPort=%4d).\n",
                              rsr_currSess.to_uint(), rsr_datLenReq.to_uint(), rsr_intEntry.dstPort.to_uint());
                }
            }
            rsr_fsmState = RSR_SREQ;
            break;
    }
}

/*******************************************************************************
 * @brief Read Request Handler (RRh)
 *
 * @param[in]  siSHL_Notif   A new Rx data notification from [SHELL].
 * @param[in]  siIRb_EnquSig Signals the enqueue of a chunk from InputReadBuffer (IRb).
 * @param[in]  siRDp_EnquSig Signals the dequeue of a chunk from ReadPath (RDp).
 * @param[out] soSHL_DReq    An Rx data request to [SHELL].
 * @param[out] soRDp_FwdCmd  A command telling the ReadPath (RDp) to keep/drop a stream.
 *
 * @details
 *  The [RRh] consists of 4 sub-processes:
 *   1) pRxBufferOccupancy (Rxb) keeps tracks of the input read buffer occupancy.
 *   2) pRxInterruptTable (Rit) keeps track of the received notifications.
 *   3) pRxPostNotification (Rxp) handles the incoming notifications and post them into the interrupt table.
 *   4) pRxScheduler (Rxs) schedules new data requests among the pending interrupts.
 *
 *  The [RRh] waits for a notification from [TOE] indicating the availability
 *   of new data for the TcpApplication Flash (TAF) process of the [ROLE]. If
 *   the TCP segment length of the notification message is greater than 0, the
 *   data segment is valid and the notification is accepted. The #bytes and TCP
 *   destination port specified by the notification are added to the interrupt
 *   table by the [Rxp] process.
 *  The [Rxh] implement a round-robin that schedules among the pending requests
 *   and generates data requests to [TOE] accordingly. Upon, request, the number
 *   of pending bytes in [Rit] is decreased.
 *   For testing purposes, the TCP destination port is evaluated here and one of
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
        stream<SigBit>         &siIRb_EnquSig,
        stream<SigBit>         &siRDp_DequSig,
        stream<TcpAppRdReq>    &soSHL_DReq,
        stream<ForwardCmd>     &soRDp_FwdCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "RRh");

    //-- LOCAL STREAM ----------------------------------------------------------
    static stream<InterruptQuery>   ssRpnToRit_InterruptQry ("ssRpnToRit_InterruptQry");
    #pragma HLS stream     variable=ssRpnToRit_InterruptQry depth=2
    static stream<SessionId>        ssRpnToRxh_SessId       ("ssRpnToRxh_SessId");
    #pragma HLS stream     variable=ssRpnToRxh_SessId       depth=4

    static stream<InterruptEntry>   ssRitToRsr_InterruptRep ("ssRitToRsr_InterruptRep");
    #pragma HLS stream     variable=ssRitToRsr_InterruptRep depth=2
    static stream<InterruptEntry>   ssRitToRpn_InterruptRep ("ssRitToRpn_InterruptRep");
    #pragma HLS stream     variable=ssRitToRpn_InterruptRep depth=2

    static stream<InterruptQuery>   ssRxsToRit_InterruptQry ("ssRxsToRit_InterruptQry");
    #pragma HLS stream     variable=ssRxsToRit_InterruptQry depth=2

    static stream<ReqBit>           ssRxsToRxh_SessIdReq    ("ssRxsToRxh_SessIdReq");
    #pragma HLS stream     variable=ssRxsToRxh_SessIdReq    depth=2
    static stream<SessionId>        ssRxsToRxh_ClearInt     ("ssRxsToRxh_ClearInt");
    #pragma HLS stream     variable=ssRxsToRxh_ClearInt     depth=2

    static stream<SessionId>        ssRxhToRxs_SessIdRep    ("ssRxhToRxs_SessIdRep");
    #pragma HLS stream     variable=ssRxhToRxs_SessIdRep    depth=2

    static stream<ap_uint<log2Ceil<cIBuffBytes>::val+1> >  ssRxbToRxs_FreeSpace  ("ssRxbToRxs_FreeSpace");
    #pragma HLS stream                            variable=ssRxbToRxs_FreeSpace  depth=4

    //-- PROCESS FUNCTIONS -----------------------------------------------------
    pRxBufferOccupancy(
            siIRb_EnquSig,
            siRDp_DequSig,
            ssRxbToRxs_FreeSpace);

    pRxPostNotification(
            siSHL_Notif,
            ssRpnToRit_InterruptQry,
            ssRitToRpn_InterruptRep,
            ssRpnToRxh_SessId);

    pRxHandler(
            ssRpnToRxh_SessId,
            ssRxsToRxh_SessIdReq,
            ssRxhToRxs_SessIdRep,
            ssRxsToRxh_ClearInt);

    pRxScheduler(
            ssRxsToRxh_SessIdReq,
            ssRxhToRxs_SessIdRep,
            ssRxbToRxs_FreeSpace,
            ssRxsToRit_InterruptQry,
            ssRitToRsr_InterruptRep,
            ssRxsToRxh_ClearInt,
            soSHL_DReq,
            soRDp_FwdCmd);

    pRxInterruptTable(
            ssRpnToRit_InterruptQry,
            ssRitToRpn_InterruptRep,
            ssRxsToRit_InterruptQry,
            ssRitToRsr_InterruptRep);
}

#else

/*******************************************************************************
 * @brief Input Read Buffer (IRb)
 *
 * @param[in]  piSHL_Enable  Enable signal from [SHELL].
 * @param[in]  siSHL_Data    Data stream from [SHELL].
 * @param[in]  siSHL_Meta    Session Id from [SHELL].
 * @param[out] soRDp_Data    Data stream to ReadPath (RDp).
 * @param[out] soRDp_Meta    Metadata stream [RDp].
 *
 * @details
 *  This process implements an input FIFO buffer as a stream. The goal is to
 *   provision a buffer to store all the bytes that were requested by the
 *   ReadRequestHandler (RRh) process.
 *  FYI, if this user process does not absorb the incoming data stream fast
 *   enough, the [TOE] will start dropping segments on his side to avoid any
 *   blocking situation.
 *******************************************************************************/
void pInputReadBuffer(
        CmdBit              *piSHL_Enable,
        stream<TcpAppData>  &siSHL_Data,
        stream<TcpAppMeta>  &siSHL_Meta,
        stream<TcpAppData>  &soRDp_Data,
        stream<TcpAppMeta>  &soRDp_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "IRb");

    if (*piSHL_Enable != 1) {
        return;
    }

    if (!siSHL_Meta.empty() and !soRDp_Meta.full()) {
        soRDp_Meta.write(siSHL_Meta.read());
    }
    if (!siSHL_Data.empty() and !soRDp_Data.full()) {
        soRDp_Data.write(siSHL_Data.read());
    }

    /*** OBSOLETE_20210325 ************
    switch (irb_fsmState ) {
    case IRB_IDLE:
        if (!siSHL_Meta.empty() and !soRDp_Meta.full()) {
            soRDp_Meta.write(siSHL_Meta.read());
            irb_fsmState = IRB_STREAM;
        }
        break;
    case IRB_STREAM:
        if (!siSHL_Data.empty() and !soRDp_Data.full() and !soRRh_EnquSig.full()) {
            TcpAppData currChunk = siSHL_Data.read();
            soRDp_Data.write(currChunk);
            soRRh_EnquSig.write(1);
            if (currChunk.getTLast()) {
                irb_fsmState = IRB_IDLE;
            }
        }
        break;
    }
    ***********************************/
}

/*******************************************************************************
 * @brief Read Notification Handler (RNh)
 *
 * @param[in]  piSHL_Enable  Enable signal from [SHELL].
 * @param[in]  siSHL_Notif   A new Rx data notification from [SHELL].
 * @param[out] soRRh_Notif   The notification forwarded to ReadRequestHandler (RRh).
 *
 * @details
 *  This process waits for a notification from [TOE] indicating the availability
 *   of new data for the TcpApplication Flash (TAF) process of the [ROLE]. If
 *   the TCP segment length of the notification message is greater than 0, the
 *   data segment is valid and the notification is accepted. The notification
 *   is then pushed into a FIFO for later processing by the [RRh].
 *  This process runs with II=1 in order to never miss an incoming notification.
 *******************************************************************************/
void pReadNotificationHandler(
        CmdBit                *piSHL_Enable,
        stream<TcpAppNotif>    &siSHL_Notif,
        stream<TcpAppNotif>    &soRRh_Notif)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "RNh");

    if (*piSHL_Enable != 1) {
        return;
    }

    if (!siSHL_Notif.empty()) {
        TcpAppNotif notif;
        siSHL_Notif.read(notif);
        if (notif.tcpDatLen == 0) {
            printFatal(myName, "Received a notification for a TCP segment of length 'zero'. Don't know what to do with it!\n");
        }
        if (!soRRh_Notif.full()) {
            soRRh_Notif.write(notif);
        }
        else {
            printFatal(myName, "The Rx Notif FiFo is full. Consider increasing the depth of this FiFo.\n");
        }
    }
}

/*******************************************************************************
 * @brief Read Request Handler (RRh)
 *
 * @param[in]  piSHL_Enable  Enable signal from [SHELL].
 * @param[in]  siRNh_Notif   A new Rx data notification from ReadNotifHandler (RNh).
 * @param[in]  siRDp_DequSig Signals the dequeue of a chunk from ReadPath (RDp).
 * @param[out] soRRm_DReq    A data read request to ReadRequestMover (RRm).
 * @param[out] soRDp_FwdCmd  A command telling the ReadPath (RDp) to keep/drop a stream.
 *
 * @details
 *  The [RRh] consists of 2 sub-processes:
 *   1) pRxBufferManager (Rbm) keeps tracks of the free space in the Rx buffer.
 *   2) pRxDataRequester (Rdr) dequeues the notifications from a FiFo, assesses
 *       the available space in the Rx buffer and requests one or multiple data
 *       read requests from the [SHELL] accordingly.
 *      The rule is as follows:
 *       #RequestedBytes = min(NotifDatLen, max(AvailableSpace, cMinDataReqLen).
 *
 *   For testing purposes, the TCP destination port is also evaluated here and
 *    one of the following actions is taken upon its value:
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
        CmdBit                *piSHL_Enable,
        stream<TcpAppNotif>    &siRNh_Notif,
        stream<SigBit>         &siRDp_DequSig,
        stream<TcpAppRdReq>    &soRRm_DReq,
        stream<ForwardCmd>     &soRDp_FwdCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "RRh");

    static ap_uint<log2Ceil<cIBuffBytes>::val+1>  rrh_freeSpace=cIBuffBytes;
    #pragma HLS reset                    variable=rrh_freeSpace

    if (*piSHL_Enable != 1) {
        return;
    }

    //==========================================================================
    //== pRxBufferManager (Rbm)
    //==========================================================================
    if (!siRDp_DequSig.empty()) {
        siRDp_DequSig.read();
        rrh_freeSpace += (ARW/8);
        if (DEBUG_LEVEL & TRACE_RRH) {
            printInfo(myName, "FreeSpace=%4d bytes\n", rrh_freeSpace.to_uint());
        }
    }

    //==========================================================================
    //== pRxDataRequester (Rdr)
    //==========================================================================
    //-- LOCAL STATIC VARIABLES W/ RESET ---------------------------------------
    static enum FsmStates { RDR_IDLE, RDR_GEN_DLEN, RDR_SEND_DREQ} \
                               rdr_fsmState=RDR_IDLE;
    #pragma HLS reset variable=rdr_fsmState

    //-- STATIC VARIABLES ------------------------------------------------------
    static  TcpAppNotif rdr_notif;
    static  TcpDatLen   rdr_datLenReq;

    switch(rdr_fsmState) {
        case RDR_IDLE:
            if (!siRNh_Notif.empty()) {
                siRNh_Notif.read(rdr_notif);
                rdr_fsmState = RDR_GEN_DLEN;
                if (DEBUG_LEVEL & TRACE_RRH) {
                    printInfo(myName, "Received a new notification (SessId=%2d | DatLen=%4d | TcpDstPort=%4d).\n",
                              rdr_notif.sessionID.to_uint(), rdr_notif.tcpDatLen.to_uint(), rdr_notif.tcpDstPort.to_uint());
                }
            }
            break;
        case RDR_GEN_DLEN:
            if (rrh_freeSpace >= cMinDataReqLen) {
                if (DEBUG_LEVEL & TRACE_RRH) {
                    printInfo(myName, "FreeSpace=%4d | NotifBytes=%4d \n",
                              rrh_freeSpace.to_uint(), rdr_notif.tcpDatLen.to_uint());
                }
                // Requested bytes = max(rdr_notif.byteCnt, rrh_freeSpace)
                if (rrh_freeSpace < rdr_notif.tcpDatLen) {
                    rdr_datLenReq        = rrh_freeSpace;
                    rdr_notif.tcpDatLen -= rrh_freeSpace;
                    rrh_freeSpace        = 0;
                }
                else {
                    rdr_datLenReq        = rdr_notif.tcpDatLen;
                    rrh_freeSpace       -= (rdr_notif.tcpDatLen & ~(ARW/8-1));
                    rdr_notif.tcpDatLen  = 0;
                }
                if (DEBUG_LEVEL & TRACE_RRH) {
                     printInfo(myName, "DataLenReq=%d\n", rdr_datLenReq.to_uint());
                 }
                rdr_fsmState = RDR_SEND_DREQ;
            }
            else {
                if (DEBUG_LEVEL & TRACE_RRH) {
                    printInfo(myName, "FreeSpace=%4d is too low. Waiting for buffer to drain. \n",
                                      rrh_freeSpace.to_uint());
               }
            }
            break;
        case RDR_SEND_DREQ:
            if (!soRRm_DReq.full() and !soRDp_FwdCmd.full()) {
                soRRm_DReq.write(TcpAppRdReq(rdr_notif.sessionID, rdr_datLenReq));
                switch (rdr_notif.tcpDstPort) {
                    case RECV_MODE_LSN_PORT: // 8800
                        soRDp_FwdCmd.write(ForwardCmd(rdr_notif.sessionID, rdr_datLenReq, CMD_DROP, NOP));
                        break;
                    case XMIT_MODE_LSN_PORT: // 8801
                        soRDp_FwdCmd.write(ForwardCmd(rdr_notif.sessionID, rdr_datLenReq, CMD_DROP, GEN));
                        break;
                    default:
                        soRDp_FwdCmd.write(ForwardCmd(rdr_notif.sessionID, rdr_datLenReq, CMD_KEEP, NOP));
                        break;
                }
                if (rdr_notif.tcpDatLen == 0) {
                    rdr_fsmState = RDR_IDLE;
                    if (DEBUG_LEVEL & TRACE_RRH) {
                         printInfo(myName, "Done with notification (SessId=%2d | DatLen=%4d | TcpDstPort=%4d).\n",
                                   rdr_notif.sessionID.to_uint(), rdr_notif.tcpDatLen.to_uint(), rdr_notif.tcpDstPort.to_uint());
                     }
                }
                else {
                    rdr_fsmState = RDR_GEN_DLEN;
                }
                if (DEBUG_LEVEL & TRACE_RRH) {
                    printInfo(myName, "Sending DReq(SessId=%2d, DatLen=%4d) to SHELL (requested TcpDstPort was %4d).\n",
                              rdr_notif.sessionID.to_uint(), rdr_datLenReq.to_uint(), rdr_notif.tcpDstPort.to_uint());
                }
            }
            break;
    }
}

/*******************************************************************************
 * @brief Read Request Mover (RRm)
 *
 * @param[out] siRRh_DReq  A data read request from ReadRequestHandler (RRh).
 * @param[out] soSHL_DReq  The TCP data request forwarded to [SHELL].
 *
 * @details
 *  Dequeues the read requests form a FiFo and forwards them to the [SHELL]
 *   in blocking read and write access modes.
 *******************************************************************************/
void pReadRequestMover(
        CmdBit               *piSHL_Enable,
        stream<TcpAppRdReq>  &siRRh_DReq,
        stream<TcpAppRdReq>  &soSHL_DReq)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    if (*piSHL_Enable != 1) {
        return;
    }
    if (!siRRh_DReq.empty()) {
        pStreamDataMover(siRRh_DReq, soSHL_DReq);
    }
}

#endif

/*******************************************************************************
 * @brief Read Path (RDp)
 *
 * @param[in]  piSHL_Enable     Enable signal from [SHELL].
 * @param[in]  siSHL_Data       Data stream from [SHELL].
 * @param[in]  siSHL_Meta       Session Id from [SHELL].
 * @param[in]  siRRh_FwdCmd     A command to keep/drop a stream from ReadRequestHandler (RRh).
 * @param[out] soCOn_OpnSockReq The remote socket to open to Connect (COn).
 * @param[out] soCOn_TxCountReq The #bytes to be transmitted once connection is opened by [COn].
 * @param[out] soRRh_DequSig    Signals the dequeue of a chunk to ReadRequestHandler (RRh).
 * @param[out] soTAF_Data       Data stream to [TAF].
 * @param[out] soTAF_SessId     The session-id to [TAF].
 * @param[out] soTAF_DatLen     The data-length to [TAF].
 * @param[out] soDBG_SinkCount  Counts the number of sinked bytes (for debug).
 *
 * @details
 *  This process waits for new data to read and either forwards them to the
 *   TcpApplicationFlash (TAF) process or drops them based on the 'action' field
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
        CmdBit               *piSHL_Enable,
        stream<TcpAppData>   &siSHL_Data,
        stream<TcpAppMeta>   &siSHL_Meta,
        stream<ForwardCmd>   &siRRh_FwdCmd,
        stream<SockAddr>     &soCOn_OpnSockReq,
        stream<TcpDatLen>    &soCOn_TxCountReq,
        stream<SigBit>       &soRRh_DequSig,
        stream<TcpAppData>   &soTAF_Data,
        stream<TcpSessId>    &soTAF_SessId,
        stream<TcpDatLen>    &soTAF_DatLen,
        stream<ap_uint<32> > &soDBG_SinkCount)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "RDp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { RDP_IDLE=0,    RDP_8801,
                            RDP_FWD_META,  RDP_FWD_STREAM,
                            RDP_SINK_META, RDP_SINK_STREAM } \
                               rdp_fsmState=RDP_IDLE;
    #pragma HLS reset variable=rdp_fsmState
    static ap_uint<32>         rdp_sinkCnt=0;
    #pragma HLS reset variable=rdp_sinkCnt

    //-- STATIC VARIABLES ------------------------------------------------------
    static ForwardCmd  rdp_fwdCmd;
    static TcpSessId   rdp_sessId;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TcpAppData  appData;

    if (*piSHL_Enable != 1) {
        return;
    }

    switch (rdp_fsmState ) {
    case RDP_IDLE:
        if (!siRRh_FwdCmd.empty() and !siSHL_Meta.empty()) {
            siRRh_FwdCmd.read(rdp_fwdCmd);
            siSHL_Meta.read(rdp_sessId);
            if ((rdp_fwdCmd.action == CMD_KEEP) and (rdp_fwdCmd.sessId == rdp_sessId)) {
                rdp_fsmState  = RDP_FWD_META;
            }
            else {
                rdp_fsmState  = RDP_SINK_META;
            }
        }
        break;
    case RDP_FWD_META:
        if (!soTAF_SessId.full() and !soTAF_DatLen.full()) {
            soTAF_SessId.write(rdp_sessId);
            soTAF_DatLen.write(rdp_fwdCmd.datLen);
            if (DEBUG_LEVEL & TRACE_RDP) {
                printInfo(myName, "soTAF_SessId = %d \n", rdp_sessId.to_uint());
                printInfo(myName, "soTAF_DatLen = %d \n", rdp_fwdCmd.datLen.to_uint());
            }
            rdp_fsmState  = RDP_FWD_STREAM;
        }
        break;
    case RDP_FWD_STREAM:
        if (!siSHL_Data.empty() and !soTAF_Data.full()) {
            siSHL_Data.read(appData);
            soRRh_DequSig.write(1);
            soTAF_Data.write(appData);
            if (DEBUG_LEVEL & TRACE_RDP) { printAxisRaw(myName, "soTAF_Data =", appData); }
            if (appData.getTLast()) {
                rdp_fsmState  = RDP_IDLE;
            }
        }
        break;
    case RDP_SINK_META:
        if (rdp_fwdCmd.dropCode == GEN) {
            rdp_fsmState  = RDP_8801;
        }
        else {
            rdp_fsmState  = RDP_SINK_STREAM;
        }
        break;
    case RDP_SINK_STREAM:
        if (!siSHL_Data.empty()) {
            siSHL_Data.read(appData);
            soRRh_DequSig.write(1);
            if (DEBUG_LEVEL & TRACE_RDP) { printAxisRaw(myName, "Sink Data =", appData); }
            rdp_sinkCnt += appData.getLen();
            soDBG_SinkCount.write(rdp_sinkCnt);
            if (appData.getTLast()) {
                rdp_fsmState  = RDP_IDLE;
            }
        }
        break;
    case RDP_8801:
        if (!siSHL_Data.empty() and !soCOn_OpnSockReq.full() and !soCOn_TxCountReq.full()) {
            // Extract the remote socket address and the requested #bytes to transmit
            siSHL_Data.read(appData);
            soRRh_DequSig.write(1);
            SockAddr sockToOpen(byteSwap32(appData.getLE_TData(31,  0)),   // IP4 address
                                byteSwap16(appData.getLE_TData(47, 32)));  // TCP port
            TcpDatLen bytesToSend = byteSwap16(appData.getLE_TData(63, 48));
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
                rdp_fsmState = RDP_SINK_STREAM;
            }
        }
    }
}

/*******************************************************************************
 * @brief Write Path (WRp)
 *
 * @param[in]  piSHL_Enable  Enable signal from [SHELL].
 * @param[in]  siTAF_Data   Tx data stream from [ROLE/TAF].
 * @param[in]  siTAF_SessId The session Id from [ROLE/TAF].
 * @param[in]  siTAF_DatLen The data length from [ROLE/TAF].
 * @param[in]  siCOn_TxBytesReq The #bytes to be transmitted on the active opened connection from Connect(COn).
 * @param[in]  siCOn_SessId The session id of the active opened connection from [COn].
 * @param[out] soSHL_Data   Tx data to [SHELL].
 * @param[out] soSHL_SndReq Request to send to [SHELL].
 * @param[in]  siSHL_SndRep Send reply from [SHELL].
 *
 * @details
 *  This process waits for new data to be forwarded from the TcpAppFlash (TAF)
 *   or for a transmit test command from Connect(COn).
 *  Upon reception of one of these two requests, the process issues a request to
 *   send message and waits for its reply. A request to send consists of a
 *   session-id and a data-length information.
 *  A send reply consists of a session-id, a data-length, the amount of space
 *   left in TCP Tx buffer and an error code.
 *   1) If the return code is 'NO_ERROR', the process is allowed to send the
 *      amount of requested bytes.
 *   2) If the return code is 'NO_SPACE', there is not enough space available in
 *      the TCP Tx buffer of the session. The process will retry its request
 *      after a short delay but may give up after a maximum number f trials.
 *   3) If the return code is 'NO_CONNECTION', the process will abandon the
 *      transmission because there is no established connection for session-id.
 *
 *******************************************************************************/
void pWritePath(
        CmdBit               *piSHL_Enable,
        stream<TcpAppData>   &siTAF_Data,
        stream<TcpSessId>    &siTAF_SessId,
        stream<TcpDatLen>    &siTAF_DatLen,
        stream<TcpDatLen>    &siCOn_TxBytesReq,
        stream<SessionId>    &siCOn_TxSessId,
        stream<TcpAppData>   &soSHL_Data,
        stream<TcpAppSndReq> &soSHL_SndReq,
        stream<TcpAppSndRep> &siSHL_SndRep)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName = concat3(THIS_NAME, "/", "WRp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { WRP_IDLE=0, WRP_RTS, WRP_RTS_REP,
                            WRP_STREAM, WRP_TXGEN, WRP_DRAIN } \
                               wrp_fsmState=WRP_IDLE;
    #pragma HLS reset variable=wrp_fsmState
    static enum GenChunks { CHK0=0, CHK1 } \
                               wrp_genChunk=CHK0;
    #pragma HLS reset variable=wrp_genChunk

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static TcpAppSndReq wrp_sendReq;
    static FlagBool     wrp_testMode;
    static uint16_t     wrp_retryCnt;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TcpAppData  appData;

    if (*piSHL_Enable != 1) {
        return;
    }

    switch (wrp_fsmState) {
    case WRP_IDLE:
        //-- Always give priority to the Tx test generator (for testing reasons)
        if (!siCOn_TxSessId.empty() and !siCOn_TxBytesReq.empty()) {
            siCOn_TxSessId.read(wrp_sendReq.sessId);
            siCOn_TxBytesReq.read(wrp_sendReq.length);
            if (DEBUG_LEVEL & TRACE_WRP) {
                printInfo(myName, "Received a Tx test request from [TSIF/COn] for sessId=%d and nrBytes=%d.\n",
                        wrp_sendReq.sessId.to_uint(), wrp_sendReq.length.to_uint());
            }
            if (wrp_sendReq.length != 0) {
                wrp_testMode = true;
                wrp_retryCnt = 0x200;
                wrp_fsmState = WRP_RTS;
            }
            else {
                wrp_fsmState = WRP_IDLE;
            }
        }
        else if (!siTAF_SessId.empty() and !siTAF_DatLen.empty()) {
            siTAF_SessId.read(wrp_sendReq.sessId);
            siTAF_DatLen.read(wrp_sendReq.length);
            if (DEBUG_LEVEL & TRACE_WRP) {
                printInfo(myName, "Received a data forward request from [ROLE/TAF] for sessId=%d and nrBytes=%d.\n",
                          wrp_sendReq.sessId.to_uint(), wrp_sendReq.length.to_uint());
            }
            if (wrp_sendReq.length != 0) {
                wrp_testMode = false;
                wrp_retryCnt = 0x200;
                wrp_fsmState = WRP_RTS;
            }
        }
        break;
    case WRP_RTS:
        if (!soSHL_SndReq.full()) {
            soSHL_SndReq.write(wrp_sendReq);
            wrp_fsmState = WRP_RTS_REP;
        }
        break;
    case WRP_RTS_REP:
        if (!siSHL_SndRep.empty()) {
            //-- Read the request-to-send reply and continue accordingly
            TcpAppSndRep appSndRep = siSHL_SndRep.read();
            switch (appSndRep.error) {
            case NO_ERROR:
                if (wrp_testMode) {
                    wrp_genChunk = CHK0;
                    wrp_fsmState = WRP_TXGEN;
                }
                else {
                    wrp_fsmState = WRP_STREAM;
                }
                break;
            case NO_SPACE:
                printWarn(myName, "Not enough space for writing %d bytes in the Tx buffer of session #%d. Available space is %d bytes.\n",
                          appSndRep.length.to_uint(), appSndRep.sessId.to_uint(), appSndRep.spaceLeft.to_uint());
                if (wrp_retryCnt) {
                    wrp_retryCnt -= 1;
                    wrp_fsmState = WRP_RTS;
                }
                else {
                    if (wrp_testMode) {
                        wrp_fsmState = WRP_IDLE;
                    }
                    else {
                        wrp_fsmState = WRP_DRAIN;
                    }
                }
                break;
            case NO_CONNECTION:
                printWarn(myName, "Attempt to write data for a session that is not established.\n");
                if (wrp_testMode) {
                    wrp_fsmState = WRP_IDLE;
                }
                else {
                    wrp_fsmState = WRP_DRAIN;
                }
                break;
            default:
                printFatal(myName, "Received unknown TCP request to send reply from [TOE].\n");
                wrp_fsmState = WRP_IDLE;
                break;
            }
        }
        break;
    case WRP_STREAM:
        if (!siTAF_Data.empty() and !soSHL_Data.full()) {
            siTAF_Data.read(appData);
            soSHL_Data.write(appData);
            if (DEBUG_LEVEL & TRACE_WRP) { printAxisRaw(myName, "soSHL_Data = ", appData); }
            if(appData.getTLast()) {
                wrp_fsmState = WRP_IDLE;
            }
        }
        break;
    case WRP_TXGEN:
        if (!soSHL_Data.full()) {
            TcpAppData currChunk(0,0,0);
            if (wrp_sendReq.length > 8) {
                currChunk.setLE_TKeep(0xFF);
                wrp_sendReq.length -= 8;
            }
            else {
                currChunk.setLE_TKeep(lenToLE_tKeep(wrp_sendReq.length));
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
    case WRP_DRAIN:
        if (!siTAF_Data.empty()) {
            siTAF_Data.read(appData);
            if (DEBUG_LEVEL & TRACE_WRP) { printAxisRaw(myName, "Draining siTAF_Data =", appData); }
            if(appData.getTLast()) {
                wrp_fsmState = WRP_IDLE;
            }
        }
        break;
    } // End-of: switch

}

/*******************************************************************************
 * @brief TCP Shell Interface (TSIF)
 *
 * @param[in]  piSHL_Mmio_En Enable signal from [SHELL/MMIO].
 * @param[in]  siTAF_Data    TCP data stream from TcpAppFlash (TAF).
 * @param[in]  siTAF_SessId  TCP session Id  from [TAF].
 * @param[out] soTAF_Data    TCP data stream to   [TAF].
 * @param[out] soTAF_SessId  TCP session Id  to   [TAF].
 * @param[in]  siSHL_Notif   TCP data notification from [SHELL].
 * @param[out] soSHL_DReq    TCP data request to [SHELL].
 * @param[in]  siSHL_Data    TCP data stream from [SHELL].
 * @param[in]  siSHL_Meta    TCP metadata from [SHELL].
 * @param[out] soSHL_LsnReq  TCP listen port request to [SHELL].
 * @param[in]  siSHL_LsnRep  TCP listen port acknowledge from [SHELL].
 * @param[out] soSHL_Data    TCP data stream to [SHELL].
 * @param[out] soSHL_SndReq  TCP send request to [SHELL].
 * @param[in]  siSHL_SndRep  TCP send reply from [SHELL].
 * @param[out] soSHL_OpnReq  TCP open connection request to [SHELL].
 * @param[in]  siSHL_OpnRep  TCP open connection reply from [SHELL].
 * @param[out] soSHL_ClsReq  TCP close connection request to [SHELL].
 * @param[out] soDBG_SinkCnt Counts the number of sinked bytes (for debug).
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
        stream<TcpSessId>     &siTAF_SessId,
        stream<TcpDatLen>     &siTAF_DatLen,

        //------------------------------------------------------
        //-- TAF / RxP Data Interface
        //------------------------------------------------------
        stream<TcpAppData>    &soTAF_Data,
        stream<TcpSessId>     &soTAF_SessId,
        stream<TcpDatLen>     &soTAF_DatLen,

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
        stream<TcpAppSndReq>  &soSHL_SndReq,
        stream<TcpAppSndRep>  &siSHL_SndRep,

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
        //-- DEBUG Probes
        //------------------------------------------------------
        stream<ap_uint<32> >  &soDBG_SinkCnt)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INLINE
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //--------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    //-- Input Read Buffer (IRb)
    static stream<TcpAppData>      ssIRbToRDp_Data       ("ssIRbToRDp_Data");
    #pragma HLS stream    variable=ssIRbToRDp_Data       depth=cIBuffChunks
    static stream<TcpAppMeta>      ssIRbToRDp_Meta       ("ssIRbToRDp_Meta");
    #pragma HLS stream    variable=ssIRbToRDp_Meta       depth=cIBuffChunks

    //-- Read Notification Handler (RNh)
    static stream <TcpAppNotif>    ssRNhToRRh_Notif      ("ssRNhToRRh_Notif");
    #pragma HLS stream    variable=ssRNhToRRh_Notif      depth=cIBuffNotifs

    //-- Read Request Handler (RRh)
    static stream<ForwardCmd>      ssRRhToRDp_FwdCmd     ("ssRRhToRDp_FwdCmd");
    #pragma HLS stream    variable=ssRRhToRDp_FwdCmd     depth=cOBuffDReqs
    #pragma HLS DATA_PACK variable=ssRRhToRDp_FwdCmd
    static stream<TcpAppRdReq>     ssRRhToRRm_DReq       ("ssRRhToRRm_DReq");
    #pragma HLS stream    variable=ssRRhToRRm_DReq       depth=cOBuffDReqs

    //-- Read Path (RDp)
    static stream<SigBit>          ssRDpToRRh_Dequeue    ("ssRDpToRRh_Dequeue");
    #pragma HLS stream    variable=ssRDpToRRh_Dequeue    depth=4
    static stream<SockAddr>        ssRDpToCOn_OpnSockReq ("ssRDpToCOn_OpnSockReq");
    #pragma HLS stream    variable=ssRDpToCOn_OpnSockReq depth=2
    static stream<TcpDatLen>       ssRDpToCOn_TxCountReq ("ssRDpToCOn_TxCountReq");
    #pragma HLS stream    variable=ssRDpToCOn_TxCountReq depth=2

    //-- Connect (COn)
    static stream<TcpDatLen>       ssCOnToWRp_TxBytesReq ("ssCOnToWRp_TxBytesReq");
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

    pInputReadBuffer(
            piSHL_Mmio_En,
            siSHL_Data,
            siSHL_Meta,
            ssIRbToRDp_Data,
            ssIRbToRDp_Meta);

    pReadPath(
            piSHL_Mmio_En,
            ssIRbToRDp_Data,
            ssIRbToRDp_Meta,
            ssRRhToRDp_FwdCmd,
            ssRDpToCOn_OpnSockReq,
            ssRDpToCOn_TxCountReq,
            ssRDpToRRh_Dequeue,
            soTAF_Data,
            soTAF_SessId,
            soTAF_DatLen,
            soDBG_SinkCnt);

    pReadNotificationHandler(
            piSHL_Mmio_En,
            siSHL_Notif,
            ssRNhToRRh_Notif);

  #if defined USE_INTERRUPTS
    pReadRequestHandler(
            siSHL_Notif,
            ssIRbToRRh_Enqueue,
            ssRDpToRRh_Dequeue,
            soSHL_DReq,
            ssRRhToRDp_FwdCmd);

  #else
    pReadRequestHandler(
            piSHL_Mmio_En,
            ssRNhToRRh_Notif,
            ssRDpToRRh_Dequeue,
            ssRRhToRRm_DReq,
            ssRRhToRDp_FwdCmd);

    pReadRequestMover(
            piSHL_Mmio_En,
            ssRRhToRRm_DReq,
            soSHL_DReq);

    pWritePath(
            piSHL_Mmio_En,
            siTAF_Data,
            siTAF_SessId,
            siTAF_DatLen,
            ssCOnToWRp_TxBytesReq,
            ssCOnToWRp_TxSessId,
            soSHL_Data,
            soSHL_SndReq,
            siSHL_SndRep);

  #endif
}

/*! \} */
