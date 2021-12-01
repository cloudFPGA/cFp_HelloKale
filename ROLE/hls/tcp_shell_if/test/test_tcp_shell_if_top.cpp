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

/******************************************************************************
 * @file     : test_tcp_shell_if_top.cpp
 * @brief    : Testbench for the toplevel of the TCP Shell Interface (TSIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_HelloKale/ROLE/TcpShellInterface (TSIF)
 * Language    : Vivado HLS
 *
 *           +-----------------------+
 *           |  TcpApplicationFlash  |     +--------+
 *           |        (TAF)          <-----+  MMIO  |
 *           +-----/|\------+--------+     +--------+
 *                  |       |
 *                  |       |
 *           +------+------\|/-------+
 *           |   TcpShellInterface   |
 *           |        (TSIF)         |
 *           +-----/|\------+--------+
 *                  |       |
 *                  |      \|/
 *
 * \ingroup tcp_shell_if
 * \addtogroup tcp_shell_if
 * \{
 ******************************************************************************/

#include "test_tcp_shell_if_top.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//---------------------------------------------------------
#define THIS_NAME "TB_TSIF_TOP"

#ifndef __SYNTHESIS__
  extern unsigned int gSimCycCnt;
  extern unsigned int gMaxSimCycles;
#endif

#define TRACE_OFF      0x0000
#define TRACE_DDC      1 <<  1
#define TRACE_ALL      0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)

/*****************************************************************************
 * @brief Empty a debug stream and throw it away.
 *
 * @param[in/out] ss        A ref to the stream to drain.
 * @param[in]     ssName    The name of the stream to drain.
 *
 * @return NTS_OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
template<typename T> bool drainDebugCounter(stream<T> &ss, string ssName) {
    int          nr=0;
    const char  *myName  = concat3(THIS_NAME, "/", "DDC");
    T  currCount;
    T  prevCount=0;

    //-- READ FROM STREAM
    while (!(ss.empty())) {
        ss.read(currCount);
        if (currCount != prevCount) {
            if (DEBUG_LEVEL & TRACE_DDC) {
                printInfo(myName, "Detected a change on stream '%s' (currCounter=%d). \n",
                          ssName.c_str(), currCount.to_uint());
            }
        }
        prevCount = currCount;
    }
    return(NTS_OK);
}


/*******************************************************************************
 * @brief Main function for the test of the TCP Shell Interface (TSIF) TOP.
 *
 * This test take 0,1,2,3 or 4 parameters in the following order:
 * @param[in] The number of bytes to generate in 'Echo' or "Dump' mode [1:65535].
 * @param[in] The IPv4 address to open (must be in the range [0x00000000:0xFFFFFFFF].
 * @param[in] The TCP port number to open (must be in the range [0:65535].
 * @param[in] The number of bytes to generate in 'Tx' test mode [1:65535] or '0'
 *        to simply open a port w/o triggering the Tx test mode.
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
    //-- DEBUG Interface
    stream<ap_uint<32> > ssTSIF_DBG_SinkCnt("ssTSIF_DBG_SinkCnt");
    stream<ap_uint<16> > ssTSIF_DBG_InpBufSpace("ssTSIF_DBG_InpBufSpace");

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
      if ((atoi(argv[1]) < 1) or (atoi(argv[1]) > 0x4000)) {
        printFatal(THIS_NAME,
            "Argument 'len' is out of range [1:16384].\n");
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
      if ((testLenOfSegment < 1) or (testLenOfSegment > 0x4000)) {
        printFatal(THIS_NAME,
            "Argument 'len' is out of range [1:16384].\n");
        return NTS_KO;
      }
    }

    //------------------------------------------------------
    //-- ASSESS THE TESTBENCH ARGUMENTS
    //------------------------------------------------------
    int totalRxBytes = (cNrSessToSend * (testLenOfSegment + 8 + 8 + 2*echoLenOfSegment));
    if (totalRxBytes > cIBuffBytes) {
      printFatal(THIS_NAME, "The total amount of Rx bytes (%d) exceeds the size of the input TSIF read buffer (%d).\n",
        totalRxBytes, cIBuffBytes);
    }
    if (testLenOfSegment > cIBuffBytes) {
      printFatal(THIS_NAME, "The length of the test segment (%d) exceeds the size of the input TSIF read buffer (%d).\n",
           testLenOfSegment, cIBuffBytes);
    }

    //------------------------------------------------------
    //-- UPDATE THE LOCAL VARIABLES ACCORDINGLY
    //------------------------------------------------------
    SockAddr testSock(testDestHostIpv4, testDestHostPort);
    gMaxSimCycles += cNrSessToSend
        * (((echoLenOfSegment * (cNrSegToSend / 2))
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
          "## TESTBENCH 'test_tcp_shell' STARTS HERE                       ##\n");
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
            ssTOE_TSIF_Notif, ssTSIF_TOE_DReq,
            ssTOE_TSIF_Data,  ssTOE_TSIF_Meta,
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
        tcp_shell_if_top(
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
            ssTSIF_TOE_ClsReq,
            //-- DEBUG Interfaces
            ssTSIF_DBG_SinkCnt,
            ssTSIF_DBG_InpBufSpace);

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
    //-- DRAIN THE TSIF SINK and FREESPACE COUNTER STREAMS
    //---------------------------------------------------------------
    if (not drainDebugSinkCounter(ssTSIF_DBG_SinkCnt, "ssTSIF_DBG_SinkCnt")) {
            printError(THIS_NAME, "Failed to drain debug sink counter from DUT. \n");
        nrErr++;
    }
    if (not drainDebugSpaceCounter(ssTSIF_DBG_InpBufSpace, "ssTSIF_DBG_InpBufSpace")) {
            printError(THIS_NAME, "Failed to drain debug space counter from DUT. \n");
        nrErr++;
    }

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
            "####           SUCCESSFUL END OF TEST                    ####\n");
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
