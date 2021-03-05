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
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TSIF"  // TcpShellInterface

#define TRACE_OFF  0x0000
#define TRACE_IRB 1 <<  1
#define TRACE_RDP 1 <<  2
#define TRACE_WRP 1 <<  3
#define TRACE_RRH 1 <<  4
#define TRACE_LSN 1 <<  5
#define TRACE_CON 1 <<  6
#define TRACE_ALL  0xFFFF
#define DEBUG_LEVEL (TRACE_ALL)


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
    #pragma HLS PIPELINE II=1


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
                printFatal(myName, "Client is requesting the FPGA to send traffic to a none opened connection:\n");
                printSockAddr(myName, currSockAddr);
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
    #pragma HLS PIPELINE II=1


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

/*******************************************************************************
 * @brief Updates the counter which tracks the occupancy of the input read buffer.
 *
 * @param[in] siEnqueueSig  Signals the enqueue of a chunk in the buffer.
 * @param[in] siEnqueueSig  Signals the dequeue of a chunk from the buffer.
 * @param[in,out]  counter  A ref. to the counter to increment/decrement.
 *
 * @warning The
 *******************************************************************************/
void pInputBufferOccupancy(
        stream<SigBit>                     &siEnqueueSig,
        stream<SigBit>                     &siDequeueSig,
        ap_uint<log2Ceil<cIBuffSize>::val> &counter)
{
    const char *myName  = concat3(THIS_NAME, "/", "RRh/Ibo");

    bool inc = false;
    bool dec = false;

    if (!siEnqueueSig.empty() and siDequeueSig.empty()) {
        siEnqueueSig.read();
        counter += 1;
        inc = true;
    } else if (siEnqueueSig.empty() and !siDequeueSig.empty()) {
        siDequeueSig.read();
        counter -= 1;
        dec = true;
    } else if (!siEnqueueSig.empty() and !siDequeueSig.empty()) {
        siEnqueueSig.read();
        siDequeueSig.read();
        inc = true;
        dec = true;
    }

    if (DEBUG_LEVEL & TRACE_RRH) {
        if (inc or dec) {
            printInfo(myName, "Input buffer occupancy = %d chunks (%c|%c)\n",
                     (cIBuffSize - 1 - counter.to_uint()),
                     (inc ? '+' : ' '), (dec ? '-' : ' '));
        }
    }
}

/*******************************************************************************
 * @brief Receive Interrupt Table (Rit).
 *
 * @param[in] siSHL_Notif A new Rx data notification from [SHELL].
 * @param[in]  buffSpace    The available space in the input buffer (in chunks).
 * @param[out] soSHL_DReq   A data pull request to [SHELL].
 * @param[out] soRDp_FwdCmd A command telling the ReadPath (RDp) to keep/drop the next stream that will come in.
 *
 * @details
 *  A 1st process 'POST' reads the incoming notification and updates a local
 *  interrupt table accordingly. The interrupt table is used here to keep
 *   track of the total amount of pending bytes for a given session. The 'POST'
 *   process also updates a bitmap encoded vector that represents the
 *   sessions that have pending bytes to be read. For example, if bitmap[0] is
 *   set, it indicates that session #0 has pending bytes.
 *  A 2nd process 'MUTEX' grants exclusive access to the dual-port shared
 *   memory.
 *  A 3rd process 'SCHED' implements a round-robin scheduler that generates data
 *   requests based on the bitmap encoded vector of pending interrupts. The
 *   number of bytes requested to TOE is the maximum of the pending bytes in
 *   the interrupt table and the space available in the receive buffer. After
 *   sending the request to TOE, the pending byte count of the interrupt table
 *   is decreased accordingly. Warning, the buffer space is given in number of
 *   chunks (i.e., 1 chunk = 8B for 10GbE).
 ********************************************************************************/
void pReceiveInterruptTable(
        stream<TcpAppNotif>                      &siSHL_Notif,
        const ap_uint<log2Ceil<cIBuffSize>::val> &buffSpace,
        stream<TcpAppRdReq>                      &soSHL_DReq,
        stream<ForwardCmd>                       &soRDp_FwdCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "RRh/Rit");

    //-- STATIC ARRAYS ---------------------------------------------------------
    static InterruptEntry                     INTERRUPT_TABLE[cMaxSessions];
    #pragma HLS RESOURCE             variable=INTERRUPT_TABLE core=RAM_2P_BRAM
    #pragma HLS DEPENDENCE           variable=INTERRUPT_TABLE inter false

    //-- STATIC VARIABLES W/ RESET ---------------------------------------------
    static bool                                 rit_isInit=false;
    #pragma HLS reset                  variable=rit_isInit
    static ap_uint<log2Ceil<cMaxSessions>::val> rit_initEntry=0;
    #pragma HLS reset                  variable=rit_initEntry
    static ap_uint<cMaxSessions>                rit_pendingInterrupts=0;
    #pragma HLS reset                  variable=rrh_pendingInterrupts
    static SessionId                            rit_currSess=0;
    #pragma HLS reset                  variable=rit_currSess
    static bool                                 rit_mutexPostReq = false;
    #pragma HLS reset                  variable=rit_mutexPostReq
    static bool                                 rit_mutexPostGrt = false;
    #pragma HLS reset                  variable=rit_mutexPostGrt
    static bool                                 rit_mutexSchedReq = false;
    #pragma HLS reset                  variable=rit_mutexSchedReq
    static bool                                 rit_mutexSchedGrt = false;
    #pragma HLS reset                  variable=rit_mutexSchedGrt
    static enum PostFsmStates { RIT_POST_IDLE, RIT_POST_GET, RIT_POST_INC } \
                                                rit_postFsmState=RIT_POST_IDLE;
    #pragma HLS reset                  variable=rit_postFsmState
    static enum SchedFsmStates { RIT_SCHED_IDLE, RIT_SCHED_GET, RIT_SCHED_PUT, RIT_SCHED_FWD } \
                                                rit_schedFsmState=RIT_SCHED_IDLE;
    #pragma HLS reset                  variable=rit_schedFsmState
    static enum MutexFsmStates { RIT_MUTEX_IDLE, RIT_MUTEX_POST, RIT_MUTEX_SCHED } \
                                                rit_mutexFsmState=RIT_MUTEX_IDLE;

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static TcpAppNotif     rit_notif;
    static InterruptEntry  rit_postEntry;
    static InterruptEntry  rit_schedEntry;
    static TcpDatLen       rit_datLenReq;

    //=====================================================
    //== PROCESS POST (and INIT)
    //==  Warning: This process must always be available
    //==   for handling an incoming notification w/o delay.
    //=====================================================
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
        TcpDatLen newPostByteCnt;
        switch (rit_postFsmState) {
            case RIT_POST_IDLE:
                if (!siSHL_Notif.empty()) {
                    rit_notif = siSHL_Notif.read();
                    rit_mutexPostReq = true;
                    rit_postFsmState = RIT_POST_GET;
                }
                break;
            case RIT_POST_GET:
               if (rit_mutexPostGrt) {
                    rit_postEntry = INTERRUPT_TABLE[rit_notif.sessionID];
                    rit_postFsmState = RIT_POST_INC;
                }
                break;
            case RIT_POST_INC:
                newPostByteCnt = rit_postEntry.byteCnt + rit_notif.tcpDatLen;
                INTERRUPT_TABLE[rit_notif.sessionID] = InterruptEntry(newPostByteCnt, rit_notif.tcpDstPort);
                rit_pendingInterrupts[rit_notif.sessionID] = 1;
                rit_mutexPostReq = false;
                rit_postFsmState = RIT_POST_IDLE;
                break;
        }
    }

    //=====================================================
    //== PROCESS MUTEX
    //==  This process grants access to the dual-port shared
    //==   memory. Priority is always given to the POST
    //==   process.
    //=====================================================
    switch (rit_mutexFsmState) {
        case RIT_MUTEX_IDLE:
            rit_mutexPostGrt = false;
            rit_mutexSchedGrt = false;
            if (rit_mutexPostReq) {
               rit_mutexFsmState = RIT_MUTEX_POST;
            } else if (rit_mutexSchedReq) {
                rit_mutexFsmState = RIT_MUTEX_SCHED;
            }
            break;
        case RIT_MUTEX_POST:
            rit_mutexPostGrt = true;
            if (not rit_mutexPostReq) {
                rit_mutexFsmState = RIT_MUTEX_IDLE;
            }
            break;
        case RIT_MUTEX_SCHED:
            rit_mutexSchedGrt = true;
            if (not rit_mutexSchedReq) {
                rit_mutexFsmState = RIT_MUTEX_IDLE;
            }
            break;
    }

    //=====================================================
    //== PROCESS SHEDULE
    //==  This process implements a round-robin scheduler
    //==  that generates data requests to TOE based on a
    //==  bitmap encoded vector that represents the sessions
    //==  that have pending bytes to be read. For example,
    //==  if 'rit_pendingInterrupts[0]' is set, it indicates
    //==  that session #0 has pending bytes.
    //=====================================================
    TcpDatLen newSchedByteCnt;
    switch(rit_schedFsmState) {
        case RIT_SCHED_IDLE:
            if (rit_pendingInterrupts) {
                //-- Search for next active request in round-robin mode
                SessionId    nextSess=0;
                ValBool      nextSessValid=false;
                //-- Search highest pending request in between 'currSess' and 'cMaxSessions'.
                for (uint8_t i=0; i<cMaxSessions; ++i) {
                    if (rit_pendingInterrupts[i] == 1) {
                        nextSess = i;
                        nextSessValid = true;
                    }
                }
                //-- Search highest pending request in between 0 and 'currSess'
                for (uint8_t i=0; i<cMaxSessions; ++i) {
                    if (i < rit_currSess) {
                        if (rit_pendingInterrupts[i] == 1) {
                            nextSess = i;
                            nextSessValid = true;
                        }
                    }
                }
                if (nextSessValid) {
                    rit_currSess = nextSess;
                    rit_mutexSchedReq = true;
                    rit_schedFsmState = RIT_SCHED_GET;
                }
            }
            break;
        case RIT_SCHED_GET:
            if (rit_mutexSchedGrt) {
                rit_schedEntry = INTERRUPT_TABLE[rit_currSess];
                // Request to TOE = max(rit_schedEntry.byteCnt, buffSpace)
                TcpDatLen buffSpaceInBytes = buffSpace << 3;
                rit_datLenReq = (buffSpaceInBytes < rit_schedEntry.byteCnt) ? (buffSpaceInBytes) : (rit_schedEntry.byteCnt);
                rit_schedFsmState = RIT_SCHED_PUT;
            }
            break;
        case RIT_SCHED_PUT:
            // Decrement counter and put it back in the table
            newSchedByteCnt = rit_schedEntry.byteCnt - rit_datLenReq;
            INTERRUPT_TABLE[rit_currSess] = InterruptEntry(newSchedByteCnt, rit_schedEntry.dstPort);
            // Release the lock
            rit_mutexSchedReq = false;
            if (newSchedByteCnt == 0) {
                rit_pendingInterrupts[rit_currSess] = 0;
            }
            rit_schedFsmState = RIT_SCHED_FWD;
            break;
        case RIT_SCHED_FWD:
            if (!soSHL_DReq.full() and !soRDp_FwdCmd.full()) {
                soSHL_DReq.write(TcpAppRdReq(rit_currSess, rit_datLenReq));
                switch (rit_schedEntry.dstPort) {
                    case RECV_MODE_LSN_PORT: // 8800
                        soRDp_FwdCmd.write(ForwardCmd(rit_currSess, rit_datLenReq, CMD_DROP, NOP));
                        break;
                    case XMIT_MODE_LSN_PORT: // 8801
                        soRDp_FwdCmd.write(ForwardCmd(rit_currSess, rit_datLenReq, CMD_DROP, GEN));
                        break;
                    default:
                        soRDp_FwdCmd.write(ForwardCmd(rit_currSess, rit_datLenReq, CMD_KEEP, NOP));
                        break;
                }
            }
            rit_schedFsmState = RIT_SCHED_IDLE;
            break;
    }
}

/*******************************************************************************
 * @brief Read Request Handler (RRh)
 *
 * @param[in]  siSHL_Notif   A new Rx data notification from [SHELL].
 * @param[out] soSHL_DReq    An Rx data request to [SHELL].
 * @param[in]  siIRb_EnquSig Signals the enqueue of a chunk from InputReadBuffer (IRb).
 * @param[in]  siRDp_EnquSig Signals the dequeue of a chunk from ReadPath (RDp).
 * @param[out] soRDp_FwdCmd  A command telling the ReadPath (RDp) to keep/drop a stream.
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
        stream<SigBit>         &siIRb_EnquSig,
        stream<SigBit>         &siRDp_DequSig,
        stream<ForwardCmd>     &soRDp_FwdCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "RRh");

    //-- LOCAL STATIC VARIABLES W/ RESET ---------------------------------------
    static enum FsmStates { RRH_IDLE, RRH_SEND_DREQ } \
                                              rrh_fsmState=RRH_IDLE;
    #pragma HLS reset                variable=rrh_fsmState
    static ap_uint<log2Ceil<cIBuffSize>::val> rrh_bufSpace=(cIBuffSize-1);
    #pragma HLS reset                variable=rrh_bufSpace

    //-- SUB-PROCESS: Input buffer space management
    pInputBufferOccupancy(
            siIRb_EnquSig,
            siRDp_DequSig,
            rrh_bufSpace);

    //-- SUB-PROCESS: Notification management and data request scheduling
    pReceiveInterruptTable(
            siSHL_Notif,
            rrh_bufSpace,
            soSHL_DReq,
            soRDp_FwdCmd);

    //OBSOLETE -- SUB-PROCESS: Handle notifications from TOE
    /*** OBSOLETE_20210304 ************
    switch(rrh_fsmState) {
    case RRH_IDLE:
        if (!siSHL_Notif.empty() and !soRDp_FwdCmd.full()) {
            siSHL_Notif.read(rrh_notif);
            if (rrh_notif.tcpDatLen != 0) {
                // Always request the data to be received
                switch (rrh_notif.tcpDstPort) {
                case RECV_MODE_LSN_PORT: // 8800
                    soRDp_FwdCmd.write(ForwardCmd(rrh_notif.sessionID, rrh_notif.tcpDatLen, CMD_DROP, NOP));
                    break;
                case XMIT_MODE_LSN_PORT: // 8801
                    soRDp_FwdCmd.write(ForwardCmd(rrh_notif.sessionID, rrh_notif.tcpDatLen, CMD_DROP, GEN));
                    break;
                default:
                    soRDp_FwdCmd.write(ForwardCmd(rrh_notif.sessionID, rrh_notif.tcpDatLen, CMD_KEEP, NOP));
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
            soSHL_DReq.write(TcpAppRdReq(rrh_notif.sessionID, rrh_notif.tcpDatLen));
            rrh_fsmState = RRH_IDLE;
        }
        break;
    }
    ***********************************/
}

/*******************************************************************************
 * @brief Input Read Buffer (IRb)
 *
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
        stream<TcpAppData>  &siSHL_Data,
        stream<TcpAppMeta>  &siSHL_Meta,
        stream<SigBit>      &soRRh_EnquSig,
        stream<TcpAppData>  &soRDp_Data,
        stream<TcpAppMeta>  &soRDp_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "IRb");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { IRB_IDLE=0,    IRB_STREAM } \
                               irb_fsmState=IRB_IDLE;
    #pragma HLS reset variable=irb_fsmState

    switch (irb_fsmState ) {
     case IRB_IDLE:
         if (!siSHL_Meta.empty() and !soRDp_Meta.full()) {
             soRDp_Meta.write(siSHL_Meta.read());
             irb_fsmState = IRB_STREAM;
         }
         break;
     case IRB_STREAM:
         if (!siSHL_Data.empty() and !soRDp_Data.full()) {
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
 * @brief Read Path (RDp)
 *
 * @param[in]  siSHL_Data    Data stream from [SHELL].
 * @param[in]  siSHL_Meta    Session Id from [SHELL].
 * @param[in]  siRRh_FwdCmd  A command to keep/drop a stream from ReadRequestHandler (RRh).
 * @param[out] soCOn_OpnSockReq The remote socket to open to Connect (COn).
 * @param[out] soCOn_TxCountReq The #bytes to be transmitted once connection is opened by [COn].
 * @param[out] soRRh_DequSig Signals the dequeue of a chunk to ReadRequestHandler (RRh).
 * @param[out] soTAF_Data    Data stream to [TAF].
 * @param[out] soTAF_SessId  The session-id to [TAF].
 * @param[out] soTAF_DatLen  The data-length to [TAF].
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
        stream<TcpAppData>  &siSHL_Data,
        stream<TcpAppMeta>  &siSHL_Meta,
        stream<ForwardCmd>  &siRRh_FwdCmd,
        stream<SockAddr>    &soCOn_OpnSockReq,
        stream<TcpDatLen>   &soCOn_TxCountReq,
        stream<SigBit>      &soRRh_DequSig,
        stream<TcpAppData>  &soTAF_Data,
        stream<TcpSessId>   &soTAF_SessId,
        stream<TcpDatLen>   &soTAF_DatLen)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1


    const char *myName  = concat3(THIS_NAME, "/", "RDp");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { RDP_IDLE=0,    RDP_8801,
                            RDP_FWD_META,  RDP_FWD_STREAM,
                            RDP_SINK_META, RDP_SINK_STREAM } \
                               rdp_fsmState=RDP_IDLE;
    #pragma HLS reset variable=rdp_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static ForwardCmd  rdp_fwdCmd;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TcpAppData  appData;
    TcpSessId   sessId;

    switch (rdp_fsmState ) {
    case RDP_IDLE:
        if (!siRRh_FwdCmd.empty()) {
            siRRh_FwdCmd.read(rdp_fwdCmd);
            if (rdp_fwdCmd.action == CMD_KEEP) {
                rdp_fsmState  = RDP_FWD_META;
            }
            else {
                rdp_fsmState  = RDP_SINK_META;
            }
        }
        break;
    case RDP_FWD_META:
        if (!siSHL_Meta.empty() and !soTAF_SessId.full() and !soTAF_DatLen.full()) {
            siSHL_Meta.read(sessId);
            soTAF_SessId.write(sessId);
            soTAF_DatLen.write(rdp_fwdCmd.datLen);
            if (DEBUG_LEVEL & TRACE_RDP) {
                printInfo(myName, "soTAF_SessId = %d \n", sessId.to_uint());
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
        if (!siSHL_Meta.empty()) {
            siSHL_Meta.read();
            if (rdp_fwdCmd.dropCode == GEN) {
                rdp_fsmState  = RDP_8801;
            }
            else {
                rdp_fsmState  = RDP_SINK_STREAM;
            }
        }
        break;
    case RDP_SINK_STREAM:
        if (!siSHL_Data.empty()) {
            siSHL_Data.read(appData);
            soRRh_DequSig.write(1);
            if (DEBUG_LEVEL & TRACE_RDP) { printAxisRaw(myName, "Sink Data =", appData); }
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
    #pragma HLS PIPELINE II=1


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

    switch (wrp_fsmState) {
    case WRP_IDLE:
        //-- Always read the metadata provided by [ROLE/TAF]
        if (!siTAF_SessId.empty() and !siTAF_DatLen.empty()) {
            siTAF_SessId.read(wrp_sendReq.sessId);
            siTAF_DatLen.read(wrp_sendReq.length);
            if (DEBUG_LEVEL & TRACE_WRP) {
                printInfo(myName, "Received a data forward request from [ROLE/TAF] for sessId=%d and nrBytes=%d.\n",
                          wrp_sendReq.sessId.to_uint(), wrp_sendReq.length.to_uint());
            }
            wrp_testMode = false;
            wrp_retryCnt = 0x200;
            wrp_fsmState = WRP_RTS;
        }
        else if (!siCOn_TxSessId.empty() and !siCOn_TxBytesReq.empty()) {
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
                printWarn(myName, "Received unknown TCP request to send reply from [TOE].\n");
                wrp_fsmState = WRP_IDLE;
                break;
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
        stream<TcpAppClsReq>  &soSHL_ClsReq)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW interval=1
    #pragma HLS INLINE

    //--------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    //-- Input Read Buffer (IRb)
    static stream<SigBit>          ssIRbToRRh_Enqueue    ("ssIRbToRRh_Enqueue");
    #pragma HLS stream    variable=ssIRbToRRh_Enqueue    depth=2
    static stream<TcpAppData>      ssIRbToRDp_Data       ("ssIRbToRDp_Data");
    #pragma HLS stream    variable=ssIRbToRDp_Data       depth=cIBuffSize
    static stream<TcpAppMeta>      ssIRbToRDp_Meta       ("ssIRbToRDp_Meta");
    #pragma HLS stream    variable=ssIRbToRDp_Meta       depth=cIBuffSize

    //-- Read Path (RDp)
    static stream<SigBit>          ssRDpToRRh_Dequeue    ("ssRDpbToRRh_Dequeue");
    #pragma HLS stream    variable=ssRDpToRRh_Dequeue    depth=2
    static stream<SockAddr>        ssRDpToCOn_OpnSockReq ("ssRDpToCOn_OpnSockReq");
    #pragma HLS stream    variable=ssRDpToCOn_OpnSockReq depth=2
    static stream<TcpDatLen>       ssRDpToCOn_TxCountReq ("ssRDpToCOn_TxCountReq");
    #pragma HLS stream    variable=ssRDpToCOn_TxCountReq depth=2

    //-- ReadrequestHandler (RRh)
    static stream<ForwardCmd>      ssRRhToRDp_FwdCmd     ("ssRRhToRDp_FwdCmd");
    #pragma HLS stream    variable=ssRRhToRDp_FwdCmd     depth=8
    #pragma HLS DATA_PACK variable=ssRRhToRDp_FwdCmd

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

    pReadRequestHandler(
            siSHL_Notif,
            soSHL_DReq,
            ssIRbToRRh_Enqueue,
            ssRDpToRRh_Dequeue,
            ssRRhToRDp_FwdCmd);

    pInputReadBuffer(
            siSHL_Data,
            siSHL_Meta,
            ssIRbToRRh_Enqueue,
            ssIRbToRDp_Data,
            ssIRbToRDp_Meta
    );

    pReadPath(
            ssIRbToRDp_Data,
            ssIRbToRDp_Meta,
            ssRRhToRDp_FwdCmd,
            ssRDpToCOn_OpnSockReq,
            ssRDpToCOn_TxCountReq,
            ssRDpToRRh_Dequeue,
            soTAF_Data,
            soTAF_SessId,
            soTAF_DatLen);

    pWritePath(
            siTAF_Data,
            siTAF_SessId,
            siTAF_DatLen,
            ssCOnToWRp_TxBytesReq,
            ssCOnToWRp_TxSessId,
            soSHL_Data,
            soSHL_SndReq,
            siSHL_SndRep);

}

/*! \} */
