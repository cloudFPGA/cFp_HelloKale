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
 * @file       : simu_udp_app_flash_env.cpp
 * @brief      : Simulation environment for the UDP Application Flash (UAF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_Monolithic/ROLE/TcpApplicationFlash (UAF)
 * Language    : Vivado HLS
 *
 *               +-----------------------+
 *               |  UdpApplicationFlash  |     +--------+
 *               |        (UAF)          <-----+  MMIO  |
 *               +-----/|\------+--------+     +--------+
 *                      |       |
 *                      |       ||
 *               +------+------\|/-------+
 *               |   UdpShellInterface   |
 *               |       (USIF)          |
 *               +-----/|\------+--------+
 *                      |       |
 *                      |      \|/
 *
 * \ingroup ROLE_UAF
 * \addtogroup ROLE_UAF_TEST
 * \{
 *******************************************************************************/

#include "simu_udp_app_flash_env.hpp"

using namespace hls;
using namespace std;

/*******************************************************************************
 * GLOBAL VARIABLES USED BY THE SIMULATION ENVIRONMENT
 *******************************************************************************/
extern unsigned int gSimCycCnt;
extern bool         gTraceEvent;
extern bool         gFatalError;
extern unsigned int gMaxSimCycles; //  = MAX_SIM_CYCLES + TB_GRACE_TIME;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_USIF | TRACE_UAF)
 ************************************************/
#define THIS_NAME "SIM"

#define TRACE_OFF     0x0000
#define TRACE_USIF   1 <<  1
#define TRACE_UAF    1 <<  2
#define TRACE_CGTF   1 <<  3
#define TRACE_DUMTF  1 <<  4
#define TRACE_ALL     0xFFFF
#define DEBUG_LEVEL (TRACE_OFF)

/******************************************************************************
 * @brief Increment the simulation counter
 ******************************************************************************/
void stepSim() {
    gSimCycCnt++;
    if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
        printInfo(THIS_NAME, "-- [@%4.4d] -----------------------------\n", gSimCycCnt);
        gTraceEvent = false;
    }
    else if (0) {
        printInfo(THIS_NAME, "------------------- [@%d] ------------\n", gSimCycCnt);
    }
}

/*******************************************************************************
 * @brief Increase the simulation time of the testbench
 *
 * @param[in]  The number of cycles to increase.
 *******************************************************************************/
void increaseSimTime(unsigned int cycles) {
    gMaxSimCycles += cycles;
}

/*******************************************************************************
 * @brief Read a datagram from a DAT file.
 *
 * @param[in]  myName       The name of the caller process.
 * @param[in]  appDatagram  A reference to the datagram to read.
 * @param[in]  ifsData      The input file stream to read from.
 * @param[out] sockPair     A ref to the current active socket pair.
 * @param[out] udpMetaQueue A ref to a container queue which holds the sequence of UDP socket-pairs.
 * @param[out] inpChunks    A ref to the number of processed chunks.
 * @param[out] inptDgrms    A ref to the number of processed datagrams.
 * @param[out] inpBytes     A ref to the number of processed bytes.
 * @return true if successful, otherwise false.
 *******************************************************************************/
bool readDatagramFromFile(const char *myName,     SimUdpDatagram &appDatagram,
                          ifstream   &ifsData,    SocketPair     &sockPair,
                          queue<UdpAppMeta> &udpMetaQueue,  int  &inpChunks,
                          int        &inpDgrms,   int            &inpBytes) {

    string          stringBuffer;
    vector<string>  stringVector;
    UdpAppData      udpAppData;
    bool            endOfDgm=false;
    bool            rc;

    do {
        //-- Read one line at a time from the input test DAT file
        getline(ifsData, stringBuffer);
        stringVector = myTokenizer(stringBuffer, ' ');
        //-- Read the Host Socket Address from line (if present)
        rc = readHostSocketFromLine(sockPair.src, stringBuffer);
        if (rc) {
            if (DEBUG_LEVEL & TRACE_CGTF) {
                printInfo(myName, "Read a new HOST socket address from DAT file:\n");
                printSockAddr(myName, sockPair.src);
            }
        }
        //-- Read the Fpga Socket Address from line (if present)
        rc = readFpgaSocketFromLine(sockPair.dst, stringBuffer);
        if (rc) {
            if (DEBUG_LEVEL & TRACE_CGTF) {
                printInfo(myName, "Read a new FPGA socket address from DAT file:\n");
                printSockAddr(myName, sockPair.dst);
            }
        }
        //-- Read an AxiWord from line
        rc = readAxisRawFromLine(udpAppData, stringBuffer);
        if (rc) {
            appDatagram.pushChunk(AxisUdp(udpAppData.getLE_TData(),
                                          udpAppData.getLE_TKeep(),
                                          udpAppData.getLE_TLast()));
            inpChunks++;
            inpBytes += udpAppData.getLen();
            if (udpAppData.getLE_TLast() == 1) {
                inpDgrms++;
                udpMetaQueue.push(UdpAppMeta(sockPair.src.addr, sockPair.src.port,
                                             sockPair.dst.addr, sockPair.dst.port));
                endOfDgm = true;
            }
        }
    } while ((ifsData.peek() != EOF) && (!endOfDgm));

    return endOfDgm;
}

/*****************************************************************************
 * @brief Create the golden UDP Tx files from an input test file.
 *
 * @param[in]  tbCtrlMode        The testbench mode of operation.
 * @param[in]  inpData_FileName  The input data file to generate from.
 * @param[out] udpMetaQueue      A ref to a container queue which holds the
 *                                sequence of UDP socket-pairs.
 * @param[in]  outData_GoldName  The output datagram gold file to create.
 * @param[in]  outMeta_GoldName  The output metadata gold file to create.
 * @param[in]  outDLen_GoldName  The output data len gold file to create.
 *
 * @return NTS_ OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
int createGoldenTxFiles(
        EchoCtrl          tbCtrlMode,
        string            inpData_FileName,
        queue<UdpAppMeta> &udpMetaQueue,
        string            outData_GoldName,
        string            outMeta_GoldName,
        string            outDLen_GoldName)
{
    const char *myName  = concat3(THIS_NAME, "/", "CGTF");

    const Ip4Addr hostDefaultIp4Address = 0x0A0CC832;   //  10.012.200.50
    const Ip4Addr fpgaDefaultIp4Address = 0x0A0CC807;   //  10.012.200.7
    const UdpPort fpgaDefaultUdpLsnPort = ECHO_PATH_THRU_PORT;
    const UdpPort hostDefaultUdpLsnPort = fpgaDefaultUdpLsnPort;
    const UdpPort hostDefaultUdpSndPort = 32768+ECHO_PATH_THRU_PORT; // 41571
    const UdpPort fpgaDefaultUdpSndPort = hostDefaultUdpSndPort;

    ifstream    ifsData;
    ofstream    ofsDataGold;
    ofstream    ofsMetaGold;
    ofstream    ofsDLenGold;

    char        currPath[FILENAME_MAX];
    int         ret=NTS_OK;
    int         inpChunks=0,  outChunks=0;
    int         inpDgrms=0,   outDgrms=0;
    int         inpBytes=0,   outBytes=0;

    //-- STEP-1 : OPEN INPUT TEST FILE ----------------------------------------
    if (not isDatFile(inpData_FileName)) {
        printError(myName, "Cannot create golden files from input file \'%s\' because file is not of type \'.dat\'.\n",
                   inpData_FileName.c_str());
        return(NTS_KO);
    }
    else {
        ifsData.open(inpData_FileName.c_str());
        if (!ifsData) {
            getcwd(currPath, sizeof(currPath));
            printError(myName, "Cannot open the file: %s \n\t (FYI - The current working directory is: %s) \n",
                       inpData_FileName.c_str(), currPath);
            return(NTS_KO);
        }
    }

    //-- STEP-2 : OPEN THE OUTPUT GOLD FILES ----------------------------------
    remove(outData_GoldName.c_str());
    remove(outMeta_GoldName.c_str());
    remove(outDLen_GoldName.c_str());
    if (!ofsDataGold.is_open()) {
        ofsDataGold.open (outData_GoldName.c_str(), ofstream::out);
        if (!ofsDataGold) {
            printFatal(THIS_NAME, "Could not open the output gold file \'%s\'. \n",
                       outData_GoldName.c_str());
        }
    }
    if (!ofsMetaGold.is_open()) {
        ofsMetaGold.open (outMeta_GoldName.c_str(), ofstream::out);
        if (!ofsMetaGold) {
            printFatal(THIS_NAME, "Could not open the output gold file \'%s\'. \n",
                       outMeta_GoldName.c_str());
        }
    }
    if (!ofsDLenGold.is_open()) {
        ofsDLenGold.open (outDLen_GoldName.c_str(), ofstream::out);
        if (!ofsDLenGold) {
            printFatal(THIS_NAME, "Could not open the output gold file \'%s\'. \n",
                       outDLen_GoldName.c_str());
        }
    }

    //-- STEP-3 : READ AND PARSE THE INPUT TEST FILE --------------------------
    SockAddr  hostSock = SockAddr(hostDefaultIp4Address, hostDefaultUdpSndPort);
    SockAddr  fpgaSock = SockAddr(fpgaDefaultIp4Address, fpgaDefaultUdpLsnPort);
    int       dgmCnt = 0;
    do {
        SimUdpDatagram appDatagram(8);
        SocketPair     currSockPair(hostSock, fpgaSock);
        UdpAppDLen     udpAppDLen = 0;
        bool           endOfDgm=false;
        //-- Retrieve one APP datagram from input DAT file (can be up to 2^16-1 bytes)
        endOfDgm = readDatagramFromFile(myName, appDatagram, ifsData,
                                        currSockPair, udpMetaQueue,
                                        inpChunks, inpDgrms, inpBytes);
        if (endOfDgm) {
            //-- Swap the IP_SA/IP_DA but keep UDP_SP/UDP/DP as is
            SocketPair goldSockPair(SockAddr(currSockPair.dst.addr, currSockPair.src.port),
                                    SockAddr(currSockPair.src.addr, currSockPair.dst.port));
            // Dump the socket pair to file
            if (tbCtrlMode == ECHO_CTRL_DISABLED) {
                writeSocketPairToFile(goldSockPair, ofsMetaGold);
                if (DEBUG_LEVEL & TRACE_CGTF) {
                    printInfo(myName, "Writing new socket-pair to gold file:\n");
                    printSockPair(myName, goldSockPair);
                }
            }

            // Dump the data len to file
            if (tbCtrlMode == ECHO_CTRL_DISABLED) {
                UdpAppDLen appDLen;
                if (currSockPair.dst.port == ECHO_PATH_THRU_PORT) {
                    //-- The traffic will be forwarded in 'ECHO-CUT-THRU' mode
                    // [TODO] if (dgmCnt % 2) {
                    // [TODO]     //-- Datagram counter is an ODD value
                    // [TODO]     //-- Enable the 'STREAMING-MODE' by setting 'DLen' to zero.
                    // [TODO]     appDLen = 0;
                    // [TODO] }
                    // [TODO] else {
                        appDLen = appDatagram.length() - UDP_HEADER_LEN;
                    // [TODO] }
                }
                else {
                    appDLen = appDatagram.length() - UDP_HEADER_LEN;
                }
                /*** UNUSED_VERSION_BASED_ON_MMIO_ECHO_CTRL_BITS *****
                if (tbMode == ECHO_PATH_THRU) {
                    appDLen = 0;
                }
                else {
                    appDLen = appDatagram.length() - UDP_HEADER_LEN;
                }
                ******************************************************/
                writeApUintToFile(appDLen, ofsDLenGold);
                if (DEBUG_LEVEL & TRACE_CGTF) {
                    printInfo(myName, "Writing new datagram len (%d) to gold file:\n", appDLen.to_int());
                }
            }

            // Dump UDP datagram payload to gold file
            if (tbCtrlMode == ECHO_CTRL_DISABLED) {
                if (appDatagram.writePayloadToDatFile(ofsDataGold) == false) {
                    printError(myName, "Failed to write UDP payload to GOLD file.\n");
                    ret = NTS_KO;
                }
                else {
                    outDgrms  += 1;
                    outChunks += appDatagram.size();
                    outBytes  += appDatagram.length();
                }
            }
            dgmCnt++;
        } // End-of:if (endOfDgm) {

    } while(ifsData.peek() != EOF);

    //-- STEP-4: CLOSE FILES
    ifsData.close();
    ofsDataGold.close();
    ofsMetaGold.close();
    ofsDLenGold.close();

    //-- STEP-5: PRINT RESULTS
    printInfo(myName, "Done with the creation of the golden file.\n");
    printInfo(myName, "\tProcessed %5d chunks in %4d datagrams, for a total of %6d bytes.\n",
              inpChunks, inpDgrms, inpBytes);
    printInfo(myName, "\tGenerated %5d chunks in %4d datagrams, for a total of %6d bytes.\n",
              outChunks, outDgrms, outBytes);
    return NTS_OK;
}

/*****************************************************************************
 * @brief Create the UDP Rx traffic as streams from an input test file.
 *
 * @param[in/out] ssData      A ref to the data stream to set.
 * @param[in]     ssDataName  The name of the data stream to set.
 * @param[in/out] ssMeta      A ref to the metadata stream to set.
 * @param[in]     ssMetaName  The name of the metadata stream to set.
 * @param[in]     datFileName The path to the DAT file to read from.
 * @param[in]     metaQueue   A ref to a queue of metadata.
 * @param[out]    nrChunks    a ref to the number of feeded chunks.
 *
 * @return NTS_ OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
int createUdpRxTraffic(
        stream<AxisApp>    &ssData, const string      ssDataName,
        stream<UdpAppMeta> &ssMeta, const string      ssMetaName,
        string             datFile,
        queue<UdpAppMeta>  &metaQueue,
        int                &nrFeededChunks)
{

    int  nrUSIF_UAF_MetaChunks = 0;
    int  nrUSIF_UAF_MetaGrams  = 0;
    int  nrUSIF_UAF_MetaBytes  = 0;

    //-- STEP-1: FEED AXIS DATA STREAM FROM DAT FILE --------------------------
    int  nrDataChunks=0, nrDataGrams=0, nrDataBytes=0;
    if (feedAxisFromFile(ssData, ssDataName, datFile,
                         nrDataChunks, nrDataGrams, nrDataBytes)) {
        printInfo(THIS_NAME, "Done with the creation of UDP-Data traffic as a stream:\n");
        printInfo(THIS_NAME, "\tGenerated %d chunks in %d datagrams, for a total of %d bytes.\n\n",
                  nrDataChunks, nrDataGrams, nrDataBytes);
        nrFeededChunks = nrDataChunks;
    }
    else {
        printError(THIS_NAME, "Failed to create UDP-Data traffic as input stream. \n");
        return NTS_KO;
    }

    //-- STEP-2: FEED METADATA STREAM FROM QUEUE ------------------------------
    while (!metaQueue.empty()) {
        ssMeta.write(metaQueue.front());
        metaQueue.pop();
    }

    return NTS_OK;

} // End-of: createUdpRxTraffic()

/*****************************************************************************
 * @brief Empty an UdpMeta stream to a DAT file.
 *
 * @param[in/out] ss        A ref to the UDP metadata stream to drain.
 * @param[in]     ssName    The name of the UDP metadata stream to drain.
 * @param[in]     fileName  The DAT file to write to.
 * @param[out     nrChunks  A ref to the number of written chunks.
 * @param[out]    nrFrames  A ref to the number of written AXI4 streams.
 * @param[out]    nrBytes   A ref to the number of written bytes.
  *
 * @return NTS_OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
bool drainUdpMetaStreamToFile(stream<UdpAppMeta> &ss, string ssName,
        string datFile, int &nrChunks, int &nrFrames, int &nrBytes) {
    ofstream    outFileStream;
    char        currPath[FILENAME_MAX];
    UdpAppMeta  udpMeta;

    const char *myName  = concat3(THIS_NAME, "/", "DUMTF");

    //-- REMOVE PREVIOUS FILE
    remove(ssName.c_str());

    //-- OPEN FILE
    if (!outFileStream.is_open()) {
        outFileStream.open(datFile.c_str(), ofstream::out);
        if (!outFileStream) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", datFile.c_str());
            return(NTS_KO);
        }
    }

    // Assess that file has ".dat" extension
    if ( datFile.find_last_of ( '.' ) != string::npos ) {
        string extension ( datFile.substr( datFile.find_last_of ( '.' ) + 1 ) );
        if (extension != "dat") {
            printError(THIS_NAME, "Cannot dump SocketPair stream to file \'%s\' because file is not of type \'DAT\'.\n", datFile.c_str());
            outFileStream.close();
            return(NTS_KO);
        }
    }

    //-- READ FROM STREAM AND WRITE TO FILE
    while (!(ss.empty())) {
        ss.read(udpMeta);
        SocketPair socketPair(SockAddr(udpMeta.ip4SrcAddr, udpMeta.udpSrcPort),
                              SockAddr(udpMeta.ip4DstAddr, udpMeta.udpDstPort));
        writeSocketPairToFile(socketPair, outFileStream);
        nrChunks++;
        nrBytes += 12;
        nrFrames++;
        if (DEBUG_LEVEL & TRACE_DUMTF) {
            printInfo(myName, "Writing new socket-pair to file:\n");
            printSockPair(myName, socketPair);
        }
    }

    //-- CLOSE FILE
    outFileStream.close();

    return(NTS_OK);
}

/*******************************************************************************
 * @brief Empty a UdpDLen stream to a DAT file.
 *
 * @param[in/out] ss        A ref to the UDP data len to drain.
 * @param[in]     ssName    The name of the data len stream to drain.
 * @param[in]     fileName  The DAT file to write to.
 * @param[out     nrChunks  A ref to the number of written chunks.
 * @param[out]    nrFrames  A ref to the number of written AXI4 streams.
 * @param[out]    nrBytes   A ref to the number of written bytes.
  *
 * @return NTS_OK if successful,  otherwise NTS_KO.
 *******************************************************************************/
bool drainUdpDLenStreamToFile(stream<UdpAppDLen> &ss, string ssName,
        string datFile, int &nrChunks, int &nrFrames, int &nrBytes) {
    ofstream    outFileStream;
    char        currPath[FILENAME_MAX];
    UdpAppDLen  udpDLen;

    const char *myName  = concat3(THIS_NAME, "/", "DUDTF");

    //-- REMOVE PREVIOUS FILE
    remove(ssName.c_str());

    //-- OPEN FILE
    if (!outFileStream.is_open()) {
        outFileStream.open(datFile.c_str(), ofstream::out);
        if (!outFileStream) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", datFile.c_str());
            return(NTS_KO);
        }
    }

    // Assess that file has ".dat" extension
    if ( datFile.find_last_of ( '.' ) != string::npos ) {
        string extension ( datFile.substr( datFile.find_last_of ( '.' ) + 1 ) );
        if (extension != "dat") {
            printError(THIS_NAME, "Cannot dump SocketPair stream to file \'%s\' because file is not of type \'DAT\'.\n", datFile.c_str());
            outFileStream.close();
            return(NTS_KO);
        }
    }

    //-- READ FROM STREAM AND WRITE TO FILE
    while (!(ss.empty())) {
        ss.read(udpDLen);
        writeApUintToFile(udpDLen, outFileStream);
        nrChunks++;
        nrBytes += 2;
        nrFrames++;
        if (DEBUG_LEVEL & TRACE_DUMTF) {
            printInfo(myName, "Writing new datagram length file. Len=%d.\n", udpDLen.to_int());
        }
    }

    //-- CLOSE FILE
    outFileStream.close();

    return(NTS_OK);
}

/*! \} */

