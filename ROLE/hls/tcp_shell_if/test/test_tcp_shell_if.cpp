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

/*****************************************************************************
 * @file       : test_tcp_shell_if.cpp
 * @brief      : Testbench for the TCP Shell Interface (TSIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_Monolithic / ROLE
 * Language    : Vivado HLS
 *
 * \ingroup ROLE
 * \addtogroup ROLE_TSIF
 * \{
 *****************************************************************************/

#include "test_tcp_shell_if.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF      0x0000
#define TRACE_TOE     1 <<  1
#define TRACE_TOE_LSN 1 <<  2
#define TRACE_TOE_OPN 1 <<  3
#define TRACE_TOE_RXP 1 <<  4
#define TRACE_TOE_TXP 1 <<  5
#define TRACE_TAF     1 <<  6
#define TRACE_MMIO    1 <<  7
#define TRACE_ALL     0xFFFF
#define DEBUG_LEVEL (TRACE_OFF)

/******************************************************************************
 * @brief Increment the simulation counter
 ******************************************************************************/
void stepSim() {
    gSimCycCnt++;
    if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
        printInfo(THIS_NAME, "-- [@%4.4d] -----------------------------\n",
                gSimCycCnt);
        gTraceEvent = false;
    } else if (0) {
        printInfo(THIS_NAME, "------------------- [@%d] ------------\n",
                gSimCycCnt);
    }
}

/*******************************************************************************
 * @brief Emulate the behavior of the TcpAppFlash (TAF).
 *
 * @param[in]  ofTAF_Data    A ref to the output TxApp file to write to.
 * @param[in]  siTSIF_Data   Data stream from TcpShellInterface (TSIF).
 * @param[in]  siTSIF_SessId Session-id from [TSIF].
 * @param[in]  siTSIF_DatLen Data-length from [TSIF].
 * @param[out] soTSIF_Data   Data stream to [TSIF].
 * @param[out] soTSIF_SessId Session-id to [TSIF].
 * @param[out] soTSIF_DatLen Data-length to [TSIF].
 * @details
 *  ALWAYS READ INCOMING DATA STREAM AND ECHO IT BACK.
 *******************************************************************************/
void pTAF(ofstream &ofTAF_Data, stream<TcpAppData> &siTSIF_Data,
        stream<TcpSessId> &siTSIF_SessId, stream<TcpDatLen> &siTSIF_DatLen,
        stream<TcpAppData> &soTSIF_Data, stream<TcpSessId> &soTSIF_SessId,
        stream<TcpSessId> &soTSIF_DatLen) {

    const char *myName = concat3(THIS_NAME, "/", "TAF");

    static enum FsmStates {
        RX_IDLE = 0, RX_STREAM
    } taf_fsmState = RX_IDLE;

    TcpAppData appData;
    TcpSessId sessId;
    TcpDatLen datLen;

    switch (taf_fsmState) {
    case RX_IDLE:
        if (!siTSIF_SessId.empty() and !soTSIF_SessId.full() and
            !siTSIF_DatLen.empty() and !soTSIF_DatLen.full()) {
            siTSIF_SessId.read(sessId);
            siTSIF_DatLen.read(datLen);
            soTSIF_SessId.write(sessId);
            soTSIF_DatLen.write(datLen);
            taf_fsmState = RX_STREAM;
        }
        break;
    case RX_STREAM:
        if (!siTSIF_Data.empty() and !soTSIF_Data.full()) {
            siTSIF_Data.read(appData);
            int bytes = writeAxisAppToFile(appData, ofTAF_Data);
            soTSIF_Data.write(appData);
            if (DEBUG_LEVEL & TRACE_TAF) {
                printAxisRaw(myName, "soTSIF_Data =", appData);
            }
            if (appData.getTLast()) {
                taf_fsmState = RX_IDLE;
            }
        }
        break;
    }
}

/*******************************************************************************
 * @brief Emulate the behavior of the SHELL & MMIO.
 *
 * @param[in]  piSHL_Ready    Ready signal from [SHELL].
 * @param[out] poTSIF_Enable  Enable signal to [TSIF] (.i.e, Enable Layer-7).
 *
 *******************************************************************************/
void pMMIO(
		StsBit *piSHL_Ready,
		CmdBit *poTSIF_Enable) {
    const char *myName = concat3(THIS_NAME, "/", "MMIO");

    static bool printOnce = true;

    if (*piSHL_Ready) {
        *poTSIF_Enable = 1;
        if (printOnce) {
            printInfo(myName,
                    "[SHELL/NTS/TOE] is ready -> Enabling operation of the TCP Shell Interface [TSIF].\n");
            printOnce = false;
        }
    } else {
        *poTSIF_Enable = 0;
    }
}

/*******************************************************************************
 * @brief Emulate behavior of the SHELL/NTS/TCP Offload Engine (TOE).
 *
 * @param[in]  nrErr         A ref to the error counter of main.
 * @param[in]  ofTAF_Gold    A ref to the TAF gold file to write.
 * @param[in]  ofTOE_Gold    A ref to the TOE gold file to write.
 * @param[in]  ofTOE_Data    A ref to the TOE data file to write.
 * @param[in]  echoDatLen    The length of the echo data to generate.
 * @param[in]  testSock      The destination socket to send traffic to.
 * @param[in]  tcpDataLen    The length of the TCP payload to generate.
 * @param[out] poMMIO_Ready  Ready signal to [MMIO].
 * @param[out] soTSIF_Notif  Notification to TcpShellInterface (TSIF).
 * @param[in]  siTSIF_DReq   Data read request from [TSIF].
 * @param[out] soTSIF_Data   Data to [TSIF].
 * @param[out] soTSIF_Meta   Session Id to [TSIF].
 * @param[in]  siTSIF_LsnReq Listen port request from [TSIF].
 * @param[out] soTSIF_LsnRep Listen port reply to [TSIF].
 *******************************************************************************/
void pTOE(
		int                  &nrErr,
		ofstream             &ofTAF_Gold,
		ofstream             &ofTOE_Gold,
        ofstream             &ofTOE_Data,
		TcpDatLen             echoDatLen,
		SockAddr              testSock,
        TcpDatLen             testDatLen,
        //-- MMIO / Ready Signal
        StsBit               *poMMIO_Ready,
        //-- TSIF / Tx Data Interfaces
        stream<TcpAppNotif>  &soTSIF_Notif,
		stream<TcpAppRdReq>  &siTSIF_DReq,
        stream<TcpAppData>   &soTSIF_Data,
		stream<TcpAppMeta>   &soTSIF_Meta,
        //-- TSIF / Listen Interfaces
        stream<TcpAppLsnReq> &siTSIF_LsnReq,
        stream<TcpAppLsnRep> &soTSIF_LsnRep,
        //-- TSIF / Rx Data Interfaces
        stream<TcpAppData>   &siTSIF_Data,
		stream<TcpAppSndReq> &siTSIF_SndReq,
        stream<TcpAppSndRep> &soTSIF_SndRep,
        //-- TSIF / Open Interfaces
        stream<TcpAppOpnReq> &siTSIF_OpnReq,
        stream<TcpAppOpnRep> &soTSIF_OpnRep) {

    static Ip4Addr toe_hostIp4Addr = DEFAULT_HOST_IP4_ADDR;
    static TcpPort toe_fpgaLsnPort = -1;
    static TcpPort toe_hostTcpSrcPort = DEFAULT_HOST_TCP_SRC_PORT;
    static TcpPort toe_hostTcpDstPort = ECHO_MODE_LSN_PORT;
    static int toe_loop = 1;

    static map<SessionId, InterruptEntry> toe_openedSess; // A map to keep track of the opened sessions
    static TcpAppSndReq toe_appSndReq;

    static enum LsnStates {
        LSN_WAIT_REQ, LSN_SEND_REP
    } toe_lsnState = LSN_WAIT_REQ;
    static enum OpnStates {
        OPN_WAIT_REQ, OPN_SEND_REP
    } toe_opnState = OPN_WAIT_REQ;
    static enum NotifStates {
        NTF_SEND, NTF_PAUSE, NTF_DONE, NTF_NEXT_SESS
    } toe_ntfState = NTF_SEND;
    static enum DrqStates {
        DRQ_WAIT_DREQ, DRQ_SEND_META, DRQ_SEND_DATA, DRQ_SEND_8801
    } toe_drqState = DRQ_WAIT_DREQ;
    static enum TxpStates {
        TXP_READ_REQ, TXP_REPLY_REQ, TXP_RECV_DATA
    } toe_txpState = TXP_READ_REQ;

    static int toe_startupDelay    = cSimToeStartupDelay;
    static int toe_rxpStartupDelay = cSimToeRxStartDelay;
    static int toe_txpStartupDelay = cSimToeTxStartDelay;
    static bool toe_isReady    = false;
    static bool toe_rxpIsReady = false;
    static bool toe_txpIsReady = false;

    const char *myLsnName = concat3(THIS_NAME, "/", "TOE/Listen");
    const char *myOpnName = concat3(THIS_NAME, "/", "TOE/OpnCon");
    const char *myNtfName = concat3(THIS_NAME, "/", "TOE/Notif");
    const char *myDrqName = concat3(THIS_NAME, "/", "TOE/DatReq");
    const char *myTxpName = concat3(THIS_NAME, "/", "TOE/TxPath");

    //------------------------------------------------------
    //-- FSM #0 - STARTUP DELAYS
    //------------------------------------------------------
    /**** OBSOLET ******
    if (toe_startupDelay) {
        *poMMIO_Ready = 0;
        toe_startupDelay--;
    } else {
        if (toe_rxpStartupDelay) {
            toe_rxpStartupDelay--;
        }
        else {
            toe_rxpIsReady = true;
        }
        if (toe_txpStartupDelay) {
            toe_txpStartupDelay--;
        }
        else
            toe_txpIsReady = true;
    }
    *poMMIO_Ready = (toe_rxpIsReady and toe_txpIsReady) ? 1 : 0;
	********************/
    if (toe_startupDelay) {
            toe_startupDelay--;
        } else {
            if (toe_rxpStartupDelay) {
                toe_rxpStartupDelay--;
            }
            else {
                toe_rxpIsReady = true;
            }
            if (toe_txpStartupDelay) {
                toe_txpStartupDelay--;
            }
            else
                toe_txpIsReady = true;
        }
        *poMMIO_Ready = (toe_startupDelay == 0) ? 1 : 0;

    //------------------------------------------------------
    //-- FSM #1 - LISTENING
    //------------------------------------------------------
    static TcpAppLsnReq toe_appLsnPortReq;
	if (*poMMIO_Ready) {
		switch (toe_lsnState) {
		case LSN_WAIT_REQ: // CHECK IF A LISTENING REQUEST IS PENDING
			if (!siTSIF_LsnReq.empty()) {
				siTSIF_LsnReq.read(toe_appLsnPortReq);
				if (DEBUG_LEVEL & TRACE_TOE_LSN) {
					printInfo(myLsnName,
							"Received a listen port request #%d from [TSIF].\n",
							toe_appLsnPortReq.to_int());
				}
				toe_lsnState = LSN_SEND_REP;
			}
			break;
		case LSN_SEND_REP: // SEND REPLY BACK TO [TSIF]
			if (!soTSIF_LsnRep.full()) {
				soTSIF_LsnRep.write(TcpAppLsnRep(NTS_OK));
				toe_fpgaLsnPort = toe_appLsnPortReq.to_int();
				toe_lsnState = LSN_WAIT_REQ;
			} else {
				printWarn(myLsnName,
						"Cannot send listen reply back to [TSIF] because stream is full.\n");
			}
			break;
		}  // End-of: switch (lsnState) {
    }

    //------------------------------------------------------
    //-- FSM #2 - OPEN CONNECTION
    //------------------------------------------------------
    TcpAppOpnReq toe_opnReq;
    TcpAppOpnRep opnReply(DEFAULT_SESSION_ID + 1, ESTABLISHED);
    if (*poMMIO_Ready) {
        switch (toe_opnState) {
        case OPN_WAIT_REQ:
            if (!siTSIF_OpnReq.empty()) {
                siTSIF_OpnReq.read(toe_opnReq);
                if (DEBUG_LEVEL & TRACE_TOE_OPN) {
                    printInfo(myOpnName,
                            "Received a request to open the following remote socket address:\n");
                    printSockAddr(myOpnName,
                            SockAddr(toe_opnReq.addr, toe_opnReq.port));
                }
                toe_opnState = OPN_SEND_REP;
            }
            break;
        case OPN_SEND_REP:
            if (!soTSIF_OpnRep.full()) {
                soTSIF_OpnRep.write(opnReply);
                // Create a new entry in the map
                toe_openedSess[opnReply.sessId] = InterruptEntry(0, 0);
                printInfo(myOpnName, "Session #%d is now established.\n",
                        opnReply.sessId.to_uint());
                toe_opnState = OPN_WAIT_REQ;
            } else {
                printWarn(myOpnName,
                        "Cannot send open connection reply back to [TSIF] because stream is full.\n");
            }
            break;
        }  // End-of: switch (toe_opnState) {
    }

    //------------------------------------------------------
    //-- FSM #3a - RX DATA PATH / NOTIFICATION
    //------------------------------------------------------
    TcpAppData appData;
    TcpSegLen notifByteCnt;
    static SessionId toe_sessId;
    static int toe_sessCnt = 0;
    static int toe_segCnt = 0;
    static int toe_sendPause = 0;
    if (toe_rxpIsReady) {
        switch (toe_ntfState) {
        case NTF_SEND:  // SEND A DATA NOTIFICATION TO [TSIF]
            switch (toe_segCnt) {
            case 1: //-- Set TSIF in receive mode and execute test
                toe_hostTcpDstPort = RECV_MODE_LSN_PORT;
                notifByteCnt = echoDatLen;
                toe_sessId = 1;
                gMaxSimCycles += (echoDatLen / 8);
                toe_sendPause = cMinWAIT + (notifByteCnt/2);
                break;
            case 2: //-- Request TSIF to open an active port
                toe_hostTcpDstPort = XMIT_MODE_LSN_PORT;
                notifByteCnt = 8; // {IP_DA,TCP_DP, 0x0000}
                toe_sessId = 2;
                gMaxSimCycles += (8 / 8);
                //-- Drain previous echo and send traffic
                toe_sendPause =
                        (cMinWAIT > (2 * echoDatLen.to_int())) ?
                                cMinWAIT : (2 * echoDatLen.to_int());
                break;
            case 3: //-- Set TSIF in transmit mode and execute test
                toe_hostTcpDstPort = XMIT_MODE_LSN_PORT;
                notifByteCnt = 8;  // {IP_DA,TCP_DP, testDatLen}
                toe_sessId = 3;
                gMaxSimCycles += (testDatLen / 8);
                toe_sendPause = 0;
                break;
            case 4:
                toe_hostTcpDstPort = ECHO_MODE_LSN_PORT;
                notifByteCnt = echoDatLen;
                toe_sessId = 4;
                gMaxSimCycles += (echoDatLen / 8);
                toe_sendPause = testDatLen; //-- Drain previous test traffic
                break;
            default:
                toe_hostTcpDstPort = ECHO_MODE_LSN_PORT;
                notifByteCnt = echoDatLen;
                toe_sessId = 0;
                gMaxSimCycles += (echoDatLen / 8);
                toe_sendPause = cMinWAIT + (notifByteCnt/2);
                break;
            }
            toe_segCnt += 1;
            toe_hostIp4Addr = DEFAULT_HOST_IP4_ADDR;
            toe_hostTcpSrcPort = DEFAULT_HOST_TCP_SRC_PORT;
            // Set the TCP destination port and byte count of the current session
            toe_openedSess[toe_sessId].byteCnt += notifByteCnt;
            if (DEBUG_LEVEL & TRACE_ALL) {
                printf("[+++] toe_openedSess[%d].byteCnt = %d\n",
                        toe_sessId.to_int(),
                        toe_openedSess[toe_sessId].byteCnt.to_int());
            }
            toe_openedSess[toe_sessId].dstPort = toe_hostTcpDstPort;
            if (!soTSIF_Notif.full()) {
                soTSIF_Notif.write(
                        TcpAppNotif(toe_sessId, notifByteCnt, toe_hostIp4Addr,
                                toe_hostTcpSrcPort, toe_hostTcpDstPort));
                if (DEBUG_LEVEL & TRACE_TOE_RXP) {
                    printInfo(myNtfName,
                            "Sending Notif to [TSIF] (sessId=%2d, datLen=%4d, dstPort=%4d).\n",
                            toe_sessId.to_int(), notifByteCnt.to_int(),
                            toe_hostTcpDstPort.to_uint());
                }
                toe_ntfState = NTF_PAUSE;
            }
            break;
        case NTF_PAUSE: // PAUSE BEFORE SENDING NEXT NOTIFICATION TO [TSIF]
            if (toe_sendPause) {
                toe_sendPause--;
            } else {
                if (toe_segCnt == cNrSegToSend) {
                    toe_ntfState = NTF_NEXT_SESS;
                } else {
                    toe_ntfState = NTF_SEND;
                }
            }
            break;
        case NTF_NEXT_SESS: // INCREMENT SESSION COUNTER
            toe_segCnt = 0;
            toe_sessCnt += 1;
            if (toe_sessCnt == cNrSessToSend) {
                toe_ntfState = NTF_DONE;
            } else {
                toe_ntfState = NTF_SEND;
            }
            break;
        case NTF_DONE: // END OF THE RX NOTIFICATION SEQUENCE
            // ALL SEGMENTS HAVE BEEN NOTIFIED
            break;
        } // End of: switch (toe_ntfState)
    }
    else if (*poMMIO_Ready) {
    	//appData.setTData(0);
    	//appData.setTKeep(0x00);
    	//appData.setTLast(0);
    	//soTSIF_Data.write(appData);
        //soTSIF_Notif.write(
        //        TcpAppNotif(toe_sessId, notifByteCnt, toe_hostIp4Addr,
        //                toe_hostTcpSrcPort, toe_hostTcpDstPort));
    }

    //------------------------------------------------------
    //-- FSM #3b - RX DATA PATH / DATA REQ HANDLING
    //------------------------------------------------------
    static TcpAppRdReq toe_appRdReq;
    if (toe_rxpIsReady) {
        switch (toe_drqState) {
        case DRQ_WAIT_DREQ: // WAIT FOR A DATA REQUEST FROM [TSIF]
            if (!siTSIF_DReq.empty()) {
                siTSIF_DReq.read(toe_appRdReq);
                if (DEBUG_LEVEL & TRACE_TOE_RXP) {
                    printInfo(myDrqName,
                            "Received a data read request from [TSIF] (sessId=%d, datLen=%d).\n",
                            toe_appRdReq.sessionID.to_int(),
                            toe_appRdReq.length.to_int());
                }
                //-- Decrement the byte counter of the session
                int sessByteCnt = toe_openedSess[toe_appRdReq.sessionID].byteCnt.to_int();
                if (toe_appRdReq.length.to_int() > sessByteCnt) {
                    printFatal(myDrqName,
                            "TOE is requesting more data (%d) than notified (%d) for session #%d !\n",
                            toe_appRdReq.length.to_int(), sessByteCnt,
                            toe_appRdReq.sessionID.to_uint());
                } else {
                    toe_openedSess[toe_appRdReq.sessionID].byteCnt -= toe_appRdReq.length;
                    if (DEBUG_LEVEL & TRACE_ALL) {
                        printf("[---] toe_openedSess[%d].byteCnt = %d\n",
                                toe_appRdReq.sessionID.to_int(),
                                toe_openedSess[toe_appRdReq.sessionID].byteCnt.to_int());
                    }
                }
                toe_drqState = DRQ_SEND_META;
            }
            break;
        case DRQ_SEND_META: // FORWARD METADATA TO [TSIF]
            if (!soTSIF_Meta.full()) {
                soTSIF_Meta.write(toe_appRdReq.sessionID);
                if (toe_openedSess[toe_appRdReq.sessionID].dstPort
                        == XMIT_MODE_LSN_PORT) {
                    toe_drqState = DRQ_SEND_8801;
                } else {
                    toe_drqState = DRQ_SEND_DATA;
                }
            }
            break;
        case DRQ_SEND_DATA: // FORWARD DATA TO [TSIF]
            if (!soTSIF_Data.full()) {
                appData.setTData((random() << 32) | random());
                if (toe_appRdReq.length > 8) {
                    appData.setTKeep(0xFF);
                    appData.setTLast(0);
                    toe_appRdReq.length -= 8;
                } else {
                    appData.setTKeep(lenTotKeep(toe_appRdReq.length));
                    appData.setTLast(TLAST);
                    toe_appRdReq.length = 0;
                    toe_drqState = DRQ_WAIT_DREQ;
                }
                soTSIF_Data.write(appData);
                if (DEBUG_LEVEL & TRACE_TOE) {
                    printAxisRaw(myDrqName, "Sending data chunk to [TSIF]: ",
                            appData);
                }
                if (toe_openedSess[toe_appRdReq.sessionID].dstPort
                        != RECV_MODE_LSN_PORT) {
                    int bytes = writeAxisAppToFile(appData, ofTAF_Gold);
                    writeAxisRawToFile(appData, ofTOE_Gold);
                }
                if (DEBUG_LEVEL & TRACE_TOE_RXP) {
                    printAxisRaw(myDrqName, "soTSIF_Data =", appData);
                }
            }
            break;
        case DRQ_SEND_8801: // FORWARD TX SOCKET ADDRESS AND TX DATA LENGTH REQUEST TO [TSIF]
            if (!soTSIF_Data.full()) {
                TcpDatLen testByteCnt = testDatLen;
                if (toe_appRdReq.sessionID == 2) {
                    printInfo(myDrqName,
                            "Requesting TSIF to connect to socket: \n");
                    printSockAddr(myDrqName, testSock);
                    appData.setTData(0);
                    appData.setLE_TData(byteSwap32(testSock.addr), 31, 0);
                    appData.setLE_TData(byteSwap16(testSock.port), 47, 32);
                    appData.setLE_TData(byteSwap16(0), 63, 48);
                    appData.setLE_TKeep(0xFF);   // Always
                    appData.setLE_TLast(TLAST);  // Always
                    soTSIF_Data.write(appData);
                } else {
                    printInfo(myDrqName,
                            "Requesting TSIF to generate a TCP payload of length=%d and to send it to socket: \n",
                            (testByteCnt).to_uint());
                    printSockAddr(myDrqName, testSock);
                    appData.setTData(0);
                    appData.setLE_TData(byteSwap32(testSock.addr), 31, 0);
                    appData.setLE_TData(byteSwap16(testSock.port), 47, 32);
                    appData.setLE_TData(byteSwap16(testByteCnt), 63, 48);
                    appData.setLE_TKeep(0xFF);   // Always
                    appData.setLE_TLast(TLAST);  // Always
                    soTSIF_Data.write(appData);

                    if (DEBUG_LEVEL & TRACE_TOE) {
                        printAxisRaw(myDrqName,
                                "Sending Tx data length request to [TSIF]: ",
                                appData);
                    }
                    //-- Generate content of the golden file
                    bool firstChunk = true;
                    while (testByteCnt) {
                        TcpAppData goldChunk(0, 0, 0);
                        if (firstChunk) {
                            for (int i = 0; i < 8; i++) {
                                if (testByteCnt) {
                                    unsigned char byte = (GEN_CHK0
                                            >> ((7 - i) * 8)) & 0xFF;
                                    goldChunk.setLE_TData(byte, (i * 8) + 7,
                                            (i * 8) + 0);
                                    goldChunk.setLE_TKeep(1, i, i);
                                    (testByteCnt)--;
                                }
                            }
                            firstChunk = !firstChunk;
                        } else {  // Second Chunk
                            for (int i = 0; i < 8; i++) {
                                if (testByteCnt) {
                                    unsigned char byte = (GEN_CHK1
                                            >> ((7 - i) * 8)) & 0xFF;
                                    goldChunk.setLE_TData(byte, (i * 8) + 7,
                                            (i * 8) + 0);
                                    goldChunk.setLE_TKeep(1, i, i);
                                    (testByteCnt)--;
                                }
                            }
                            firstChunk = !firstChunk;
                        }
                        if (testByteCnt == 0) {
                            goldChunk.setLE_TLast(TLAST);
                        }
                        int bytes = writeAxisRawToFile(goldChunk, ofTOE_Gold);
                    }
                }
                toe_drqState = DRQ_WAIT_DREQ;
            }
            break;
        }
    } // End of: switch (toe_drqState)

    //------------------------------------------------------
    //-- FSM #4 - TX DATA PATH
    //--    (Always drain the toe_appRxData coming from [TSIF])
    //------------------------------------------------------
    if (toe_txpIsReady) {
        switch (toe_txpState) {
        case TXP_READ_REQ:
            if (!siTSIF_SndReq.empty()) {
                // Read the request to send
                siTSIF_SndReq.read(toe_appSndReq);
                toe_txpState = TXP_REPLY_REQ;
            }
            break;
        case TXP_REPLY_REQ:
            if (!soTSIF_SndRep.full()) {
                // Check if session is established
                if (toe_openedSess.find(toe_appSndReq.sessId)
                        == toe_openedSess.end()) {
                    // Notify APP about the none-established connection
                    soTSIF_SndRep.write(
                            TcpAppSndRep(toe_appSndReq.sessId,
                                    toe_appSndReq.length, 0, NO_CONNECTION));
                    printError(myTxpName, "Session %d is not established.\n",
                            toe_appSndReq.sessId.to_uint());
                    nrErr++;
                    toe_txpState = TXP_REPLY_REQ;
                } else if (toe_appSndReq.length > 0x10000) { // [TODO-emulate 'maxWriteLength']
                    // Notify APP about the lack of space
                    soTSIF_SndRep.write(
                            TcpAppSndRep(toe_appSndReq.sessId,
                                    toe_appSndReq.length, 0x10000, NO_SPACE));
                    printError(myTxpName,
                            "There is not enough TxBuf memory space available for session %d.\n",
                            toe_appSndReq.sessId.to_uint());
                    nrErr++;
                    toe_txpState = TXP_REPLY_REQ;
                } else { //-- Session is ESTABLISHED and data-length <= maxWriteLength
                         // Notify APP about acceptance of the transmission
                    soTSIF_SndRep.write(
                            TcpAppSndRep(toe_appSndReq.sessId,
                                    toe_appSndReq.length, 0x10000, NO_ERROR));
                    toe_txpState = TXP_RECV_DATA;
                }
            }
            break;
        case TXP_RECV_DATA:
            if (!siTSIF_Data.empty()) {
                TcpAppData appData;
                siTSIF_Data.read(appData);
                writeAxisRawToFile(appData, ofTOE_Data);
                if (DEBUG_LEVEL & TRACE_TOE_TXP) {
                    printAxisRaw(myTxpName, "siTSIF_Data =", appData);
                }
                if (appData.getTLast())
                    toe_txpState = TXP_READ_REQ;
            }
            break;
        }
    }
}

/*******************************************************************************
 * @brief Main function for the test of the TCP Shell Interface (TSIF).
 *
 * This test take 0,1,2,3 or 4 parameters in the following order:
 * @param[in] The number of bytes to generate in 'Echo' or "Dump' mode [1:65535].
 * @param[in] The IPv4 address to open (must be in the range [0x00000000:0xFFFFFFFF].
 * @param[in] The TCP port number to open (must be in the range [0:65535].
 * @param[in] The number of bytes to generate in 'Tx' test mode [1:65535] or '0'
 *            to simply open a port w/o triggering the Tx test mode.
 *******************************************************************************/
int main(int argc, char *argv[]) {

    //------------------------------------------------------
    //-- TESTBENCH GLOBAL VARIABLES
    //------------------------------------------------------
    gSimCycCnt = 0;  // Simulation cycle counter as a global variable

    //------------------------------------------------------
    //-- DUT SIGNAL INTERFACES
    //------------------------------------------------------
    //-- SHL / Mmio Interface
    CmdBit sMMIO_TSIF_Enable;
    //-- TOE / Ready Signal
    StsBit sTOE_MMIO_Ready;
    //-- TSIF / Session Connect Id Interface
    SessionId sTSIF_TAF_SConId;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------
    //-- TAF / Rx Data Interface
    stream<TcpAppData>   ssTAF_TSIF_Data  ("ssTAF_TSIF_Data");
    stream<TcpSessId>    ssTAF_TSIF_SessId("ssTAF_TSIF_SessId");
    stream<TcpDatLen>    ssTAF_TSIF_DatLen("ssTAF_TSIF_DatLen");
    //-- TSIF / Tx Data Interface
    stream<TcpAppData>   ssTSIF_TAF_Data  ("ssTSIF_TAF_Data");
    stream<TcpSessId>    ssTSIF_TAF_SessId("ssTSIF_TAF_SessId");
    stream<TcpDatLen>    ssTSIF_TAF_DatLen("ssTSIF_TAF_DatLen");
    //-- TOE  / Rx Data Interfaces
    stream<TcpAppNotif>  ssTOE_TSIF_Notif ("ssTOE_TSIF_Notif");
    stream<TcpAppData>   ssTOE_TSIF_Data  ("ssTOE_TSIF_Data");
    stream<TcpAppMeta>   ssTOE_TSIF_Meta  ("ssTOE_TSIF_Meta");
    //-- TSIF / Rx Data Interface
    stream<TcpAppRdReq>  ssTSIF_TOE_DReq  ("ssTSIF_TOE_DReq");
    //-- TOE  / Listen Interface
    stream<TcpAppLsnRep> ssTOE_TSIF_LsnRep("ssTOE_TSIF_LsnRep");
    //-- TSIF / Listen Interface
    stream<TcpAppLsnReq> ssTSIF_TOE_LsnReq("ssTSIF_TOE_LsnReq");
    //-- TOE  / Tx Data Interfaces
    stream<TcpAppSndRep> ssTOE_TSIF_SndRep("ssTOE_TSIF_SndRep");
    //-- TSIF  / Tx Data Interfaces
    stream<TcpAppData>   ssTSIF_TOE_Data  ("ssTSIF_TOE_Data");
    stream<TcpAppSndReq> ssTSIF_TOE_SndReq("ssTSIF_TOE_SndReq");
    //-- TOE  / Connect Interfaces
    stream<TcpAppOpnRep> ssTOE_TSIF_OpnRep("ssTOE_TSIF_OpnRep");
    //-- TSIF / Connect Interfaces
    stream<TcpAppOpnReq> ssTSIF_TOE_OpnReq("ssTSIF_TOE_OpnReq");
    stream<TcpAppClsReq> ssTSIF_TOE_ClsReq("ssTSIF_TOE_ClsReq");

    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    int nrErr = 0;  // Total number of testbench errors
    ofstream ofTAF_Data;  // APP byte streams delivered to TAF
    const char *ofTAF_DataName = "../../../../test/simOutFiles/soTAF.dat";
    ofstream ofTAF_Gold;  // Gold reference file for 'ofTAF_Data'
    const char *ofTAF_GoldName = "../../../../test/simOutFiles/soTAF.gold";
    ofstream ofTOE_Data;  // Data streams delivered to TOE
    const char *ofTOE_DataName = "../../../../test/simOutFiles/soTOE_Data.dat";
    ofstream ofTOE_Gold;  // Gold streams to compare with
    const char *ofTOE_GoldName = "../../../../test/simOutFiles/soTOE_Gold.dat";

    const int defaultLenOfSegmentEcho = 42;
    const int defaultDestHostIpv4Test = 0xC0A80096; // 192.168.0.150
    const int defaultDestHostPortTest = 2718;
    const int defaultLenOfSegmentTest = 43;

    int echoLenOfSegment = defaultLenOfSegmentEcho;
    ap_uint<32> testDestHostIpv4 = defaultDestHostIpv4Test;
    ap_uint<16> testDestHostPort = defaultDestHostIpv4Test;
    int testLenOfSegment = defaultLenOfSegmentTest;

    //------------------------------------------------------
    //-- PARSING THE TESBENCH ARGUMENTS
    //------------------------------------------------------
    if (argc >= 2) {
        if ((atoi(argv[1]) < 1) or (atoi(argv[1]) >= 0x10000)) {
            printFatal(THIS_NAME,
                    "Argument 'len' is out of range [1:65535].\n");
            return NTS_KO;
        } else {
            echoLenOfSegment = atoi(argv[1]);
        }
    }
    if (argc >= 3) {
        if (isDottedDecimal(argv[2])) {
            testDestHostIpv4 = myDottedDecimalIpToUint32(argv[2]);
        } else {
            testDestHostIpv4 = atoi(argv[2]);
        }
        if ((testDestHostIpv4 < 0x00000000)
                or (testDestHostIpv4 > 0xFFFFFFFF)) {
            printFatal(THIS_NAME,
                    "Argument 'IPv4' is out of range [0x00000000:0xFFFFFFFF].\n");
            return NTS_KO;
        }
    }
    if (argc >= 4) {
        testDestHostPort = atoi(argv[3]);
        if ((testDestHostPort < 0x0000) or (testDestHostPort >= 0x10000)) {
            printFatal(THIS_NAME,
                    "Argument 'port' is out of range [0:65535].\n");
            return NTS_KO;
        }
    }
    if (argc >= 5) {
        testLenOfSegment = atoi(argv[4]);
        if ((testLenOfSegment <= 0) or (testLenOfSegment >= 0x10000)) {
            printFatal(THIS_NAME,
                    "Argument 'len' is out of range [0:65535].\n");
            return NTS_KO;
        }
    }

    //------------------------------------------------------
    //-- UPDATE THE LOCAL VARIABLES ACCORDINGLY
    //------------------------------------------------------
    SockAddr testSock(testDestHostIpv4, testDestHostPort);
    gMaxSimCycles += cNrSessToSend
            * (((echoLenOfSegment * (cNrSegToSend / 2 + 1))
                    + (testLenOfSegment * (cNrSegToSend / 2))));

    //------------------------------------------------------
    //-- REMOVE PREVIOUS OLD SIM FILES and OPEN NEW ONES
    //------------------------------------------------------
    remove(ofTAF_DataName);
    ofTAF_Data.open(ofTAF_DataName);
    if (!ofTAF_Data) {
        printError(THIS_NAME,
                "Cannot open the Application Tx file:  \n\t %s \n",
                ofTAF_DataName);
        return -1;
    }
    remove(ofTAF_GoldName);
    ofTAF_Gold.open(ofTAF_GoldName);
    if (!ofTAF_Gold) {
        printInfo(THIS_NAME,
                "Cannot open the Application Tx gold file:  \n\t %s \n",
                ofTAF_GoldName);
        return -1;
    }
    remove(ofTOE_DataName);
    if (!ofTOE_Data.is_open()) {
        ofTOE_Data.open(ofTOE_DataName, ofstream::out);
        if (!ofTOE_Data) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n",
                    ofTOE_DataName);
        }
    }
    remove(ofTOE_GoldName);
    if (!ofTOE_Gold.is_open()) {
        ofTOE_Gold.open(ofTOE_GoldName, ofstream::out);
        if (!ofTOE_Gold) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n",
                    ofTOE_GoldName);
            return (NTS_KO);
        }
    }

    printInfo(THIS_NAME,
            "############################################################################\n");
    printInfo(THIS_NAME,
            "## TESTBENCH 'test_tcp_shell' STARTS HERE                                 ##\n");
    printInfo(THIS_NAME,
            "############################################################################\n\n");
    if (argc > 1) {
        printInfo(THIS_NAME,
                "This testbench will be executed with the following parameters: \n");
        for (int i = 1; i < argc; i++) {
            printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i - 1), argv[i]);
        }
    }

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {
        //-------------------------------------------------
        //-- EMULATE TOE
        //-------------------------------------------------
        pTOE(nrErr, ofTAF_Gold, ofTOE_Gold, ofTOE_Data, echoLenOfSegment,
                testSock, testLenOfSegment,
                //-- TOE / Ready Signal
                &sTOE_MMIO_Ready,
                //-- TOE / Tx Data Interfaces
                ssTOE_TSIF_Notif, ssTSIF_TOE_DReq, ssTOE_TSIF_Data,
                ssTOE_TSIF_Meta,
                //-- TOE / Listen Interfaces
                ssTSIF_TOE_LsnReq, ssTOE_TSIF_LsnRep,
                //-- TOE / Tx Data Interfaces
                ssTSIF_TOE_Data, ssTSIF_TOE_SndReq, ssTOE_TSIF_SndRep,
                //-- TOE / Open Interfaces
                ssTSIF_TOE_OpnReq, ssTOE_TSIF_OpnRep);

        //-------------------------------------------------
        //-- EMULATE SHELL/MMIO
        //-------------------------------------------------
        pMMIO(
                //-- TOE / Ready Signal
                &sTOE_MMIO_Ready,
                //-- MMIO / Enable Layer-7 (.i.e APP alias ROLE)
                &sMMIO_TSIF_Enable);

        //-------------------------------------------------
        //-- RUN DUT
        //-------------------------------------------------
        tcp_shell_if(
                //-- SHELL / Mmio Interface
                &sMMIO_TSIF_Enable,
                //-- TAF / Rx & Tx Data Interfaces
                ssTAF_TSIF_Data, ssTAF_TSIF_SessId, ssTAF_TSIF_DatLen,
                ssTSIF_TAF_Data, ssTSIF_TAF_SessId, ssTSIF_TAF_DatLen,
                //-- TOE / Rx Data Interfaces
                ssTOE_TSIF_Notif, ssTSIF_TOE_DReq, ssTOE_TSIF_Data,
                ssTOE_TSIF_Meta,
                //-- TOE / Listen Interfaces
                ssTSIF_TOE_LsnReq, ssTOE_TSIF_LsnRep,
                //-- TOE / Tx Data Interfaces
                ssTSIF_TOE_Data, ssTSIF_TOE_SndReq, ssTOE_TSIF_SndRep,
                //-- TOE / Tx Open Interfaces
                ssTSIF_TOE_OpnReq, ssTOE_TSIF_OpnRep,
                //-- TOE / Close Interfaces
                ssTSIF_TOE_ClsReq);

        //-------------------------------------------------
        //-- EMULATE ROLE/TcpApplicationFlash
        //-------------------------------------------------
        pTAF(ofTAF_Data,
        //-- TSIF / Data Interface
                ssTSIF_TAF_Data, ssTSIF_TAF_SessId, ssTSIF_TAF_DatLen,
                //-- TAF / Data Interface
                ssTAF_TSIF_Data, ssTAF_TSIF_SessId, ssTAF_TSIF_DatLen);

        //------------------------------------------------------
        //-- INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        stepSim();

    } while ((gSimCycCnt < gMaxSimCycles) and (!gFatalError) and (nrErr < 10));

    printInfo(THIS_NAME,
            "############################################################################\n");
    printInfo(THIS_NAME,
            "## TESTBENCH 'test_tcp_shell_if' ENDS HERE                                ##\n");
    printInfo(THIS_NAME,
            "############################################################################\n");
    stepSim();

    //---------------------------------------------------------------
    //-- COMPARE RESULT DATA FILE WITH GOLDEN FILE
    //---------------------------------------------------------------
    if (ofTAF_Data.tellp() != 0) {
        int res = system(
                ("diff --brief -w " + std::string(ofTAF_DataName) + " "
                        + std::string(ofTAF_GoldName) + " ").c_str());
        if (res != 0) {
            printError(THIS_NAME, "File \"%s\" differs from file \"%s\" \n",
                    ofTAF_DataName, ofTAF_GoldName);
            nrErr++;
        }
    } else {
        printError(THIS_NAME, "File \"%s\" is empty.\n", ofTAF_DataName);
        nrErr++;
    }
    if (ofTOE_Data.tellp() != 0) {
        int res = system(
                ("diff --brief -w " + std::string(ofTOE_DataName) + " "
                        + std::string(ofTOE_GoldName) + " ").c_str());
        if (res != 0) {
            printError(THIS_NAME, "File \"%s\" differs from file \"%s\" \n",
                    ofTOE_DataName, ofTOE_GoldName);
            nrErr++;
        }
    } else {
        printError(THIS_NAME, "File \"%s\" is empty.\n", ofTOE_DataName);
        nrErr++;
    }

    //---------------------------------------------------------------
    //-- PRINT TESTBENCH STATUS
    //---------------------------------------------------------------
    printf("\n");
    printInfo(THIS_NAME,
            "This testbench was executed with the following parameters: \n");
    for (int i = 1; i < argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i - 1), argv[i]);
    }
    printf("\n");

    if (nrErr) {
        printError(THIS_NAME,
                "###########################################################\n");
        printError(THIS_NAME,
                "#### TEST BENCH FAILED : TOTAL NUMBER OF ERROR(S) = %2d ####\n",
                nrErr);
        printError(THIS_NAME,
                "###########################################################\n");
    } else {
        printInfo(THIS_NAME,
                "#############################################################\n");
        printInfo(THIS_NAME,
                "####               SUCCESSFUL END OF TEST                ####\n");
        printInfo(THIS_NAME,
                "#############################################################\n");
    }

    //---------------------------------
    //-- CLOSING OPEN FILES
    //---------------------------------
    ofTAF_Data.close();
    ofTAF_Gold.close();
    ofTOE_Data.close();
    ofTOE_Gold.close();

    return (nrErr);
}

/*! \} */
