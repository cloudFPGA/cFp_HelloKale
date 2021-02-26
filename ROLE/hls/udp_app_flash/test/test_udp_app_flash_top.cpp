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
 * @file       : test_udp_app_flash_top.cpp
 * @brief      : Testbench for toplevel of the UDP Application Flash (UAF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_Monolithic / ROLE
 * Language    : Vivado HLS
 *
 * \ingroup ROLE_UAF
 * \addtogroup ROLE_UAF_TEST
 * \{
 *******************************************************************************/

#include "test_udp_app_flash_top.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB_UAF_TOP"

#define TRACE_OFF     0x0000
#define TRACE_USIF   1 <<  1
#define TRACE_UAF    1 <<  2
#define TRACE_CGTF   1 <<  3
#define TRACE_DUMTF  1 <<  4
#define TRACE_ALL     0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)

/*******************************************************************************
 * @brief Increment the simulation counter
 *******************************************************************************/
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
 * @brief Read a datagram from a DAT file.
 *
 * @param[in]  myName       The name of the caller process.
 * @param[in]  appDatagram  A reference to the datagram to read.
 * @param[in]  ifsData      The input file stream to read from.
 * @param[out] udpAppMeta   A ref to the current active socket pair.
 * @param[out] udpMetaQueue A ref to a container queue which holds the sequence of UDP socket-pairs.
 * @param[out] inpChunks    A ref to the number of processed chunks.
 * @param[out] inptDgrms    A ref to the number of processed datagrams.
 * @param[out] inpBytes     A ref to the number of processed bytes.
 * @return true if successful, otherwise false.
 *******************************************************************************/
bool readDatagramFromFile(const char *myName,     SimUdpDatagram &appDatagram,
                          ifstream   &ifsData,    UdpAppMeta     &udpAppMeta,
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
        rc = readHostSocketFromLine(udpAppMeta.src, stringBuffer);
        if (rc) {
            if (DEBUG_LEVEL & TRACE_CGTF) {
                printInfo(myName, "Read a new HOST socket address from DAT file:\n");
                printSockAddr(myName, udpAppMeta.src);
            }
        }
        //-- Read the Fpga Socket Address from line (if present)
        rc = readFpgaSocketFromLine(udpAppMeta.dst, stringBuffer);
        if (rc) {
            if (DEBUG_LEVEL & TRACE_CGTF) {
                printInfo(myName, "Read a new FPGA socket address from DAT file:\n");
                printSockAddr(myName, udpAppMeta.dst);
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
                udpMetaQueue.push(udpAppMeta);
                endOfDgm = true;
            }
        }
    } while ((ifsData.peek() != EOF) && (!endOfDgm));

    return endOfDgm;
}

/*****************************************************************************
 * @brief Create the golden UDP Tx files from an input test file.
 *
 * @param[in]  tbMode            The testbench mode of operation.
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
        EchoCtrl          tbMode,
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
    do {
        SimUdpDatagram appDatagram(8);

        UdpAppMeta     udpAppMeta = SocketPair(hostSock, fpgaSock);
        UdpAppDLen     udpAppDLen = 0;
        bool           endOfDgm=false;
        //-- Retrieve one APP datagram from input DAT file (can be up to 2^16-1 bytes)
        endOfDgm = readDatagramFromFile(myName, appDatagram, ifsData,
                                        udpAppMeta, udpMetaQueue,
                                        inpChunks, inpDgrms, inpBytes);
        if (endOfDgm) {
            //-- Swap the IP_SA/IP_DA but keep UDP_SP/UDP/DP as is
            UdpAppMeta udpGldMeta(SockAddr(udpAppMeta.dst.addr, udpAppMeta.src.port),
                                  SockAddr(udpAppMeta.src.addr, udpAppMeta.dst.port));
            // Dump the socket pair to file
            if (tbMode != ECHO_OFF) {
                writeSocketPairToFile(udpGldMeta, ofsMetaGold);
                if (DEBUG_LEVEL & TRACE_CGTF) {
                    printInfo(myName, "Writing new socket-pair to gold file:\n");
                    printSockPair(myName, udpGldMeta);
                }
            }

            // Dump the data len to file
            if (tbMode != ECHO_OFF) {
                UdpAppDLen appDLen;
                if (udpAppMeta.dst.port == ECHO_PATH_THRU_PORT) {
                    //-- The traffic will be forwarded in 'ECHO-CUT-THRU' mode
                    //--  and in 'STREAMING-MODE' by setting 'DLen' to zero.
                    appDLen = 0;
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
            if (tbMode != ECHO_OFF) {
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
bool drainUdpMetaStreamToFile(stream<SocketPair> &ss, string ssName,
        string datFile, int &nrChunks, int &nrFrames, int &nrBytes) {
    ofstream    outFileStream;
    char        currPath[FILENAME_MAX];
    SocketPair  udpMeta;

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
        SocketPair socketPair(SockAddr(udpMeta.src.addr,
                                       udpMeta.src.port),
                              SockAddr(udpMeta.dst.addr,
                                       udpMeta.dst.port));
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


/*******************************************************************************
 * @brief Main function for the test of the UDP Application Flash (UAF).
 *******************************************************************************/
int main(int argc, char *argv[]) {

    gSimCycCnt = 0;

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int      nrErr  = 0;
    EchoCtrl tbMode = ECHO_PATH_THRU; // Indicates TB testing mode.

    //------------------------------------------------------
    //-- NONE-STREAM-BASED INTERFACES
    //------------------------------------------------------
    // INFO - The UAF is specified to use the block-level IO protocol
    // 'ap_ctrl_none'. This core uses also a few configurations signals
    // which are not stream-based and which prevent the design from
    // being verified in C/RTL co-simulation mode. In oder to comply
    // with the 'Interface Synthesis Requirements' of UG902, the design
    // is compiled with a preprocessor macro that statically defines the
    // testbench mode of operation. This avoid the following error issued
    // when trying to C/RTL co-simulate this component:
    //   @E [SIM-345] Cosim only supports the following 'ap_ctrl_none'
    //      designs: (1) combinational designs; (2) pipelined design with
    //      task interval of 1; (3) designs with array streaming or
    //      hls_stream ports.
    //   @E [SIM-4] *** C/RTL co-simulation finished: FAIL **
   //------------------------------------------------------
#if TB_MODE == 1
    ap_uint<2>          sSHL_UAF_Mmio_EchoCtrl  = ECHO_STORE_FWD;
    ap_uint<1>          sSHL_UAF_Mmio_PostPktEn = 0;
    ap_uint<1>          sSHL_UAF_Mmio_CaptPktEn = 0;
#elif TB_MODE == 2
    ap_uint<2>          sSHL_UAF_Mmio_EchoCtrl  = ECHO_OFF;
    ap_uint<1>          sSHL_UAF_Mmio_PostPktEn = 0;
    ap_uint<1>          sSHL_UAF_Mmio_CaptPktEn = 0;
#else
    ap_uint<2>          sSHL_UAF_Mmio_EchoCtrl  = ECHO_PATH_THRU;
    ap_uint<1>          sSHL_UAF_Mmio_PostPktEn = 0;
    ap_uint<1>          sSHL_UAF_Mmio_CaptPktEn = 0;
#endif

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    stream<UdpAppData>  ssUSIF_UAF_Data  ("ssUSIF_UAF_Data");
    stream<UdpAppMeta>  ssUSIF_UAF_Meta  ("ssUSIF_UAF_Meta");
    stream<UdpAppData>  ssUAF_USIF_Data  ("ssUAF_USIF_Data");
    stream<UdpAppMeta>  ssUAF_USIF_Meta  ("ssUAF_USIF_Meta");
    stream<UdpAppDLen>  ssUAF_USIF_DLen  ("ssUAF_USIF_DLen");

    //------------------------------------------------------
    //-- PARSING THE TESBENCH ARGUMENTS
    //------------------------------------------------------
    if (argc < 3) {
        printFatal(THIS_NAME, "Expected a minimum of 2 parameters with the following synopsis:\n \t\t mode(0|1|2)   siUAF_<Filename>.dat\n");
    }
    tbMode = EchoCtrl(atoi(argv[1]));
    if (tbMode != sSHL_UAF_Mmio_EchoCtrl) {
        printFatal(THIS_NAME, "tbMode (%d) does not match TB_MODE (%d). Modify the CFLAG and re-compile.\n", tbMode, TB_MODE);
    }

    switch (tbMode) {
    case ECHO_PATH_THRU:
    case ECHO_STORE_FWD:
        break;
    case ECHO_OFF:
        printFatal(THIS_NAME, "The 'ECHO_OFF' mode is no longer supported since the removal of the MMIO EchoCtrl bits. \n");
        break;
    default:
        printFatal(THIS_NAME, "Unknown testing mode '%d' (or not yet implemented). \n");
    }

    printf("#####################################################\n");
    printf("## TESTBENCH STARTS HERE                           ##\n");
    printf("#####################################################\n");
    printInfo(THIS_NAME, "This testbench will be executed with the following parameters: \n");
    printInfo(THIS_NAME, "\t==> TB Mode  = %c\n", *argv[1]);
    for (int i=2; i<argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
    }
    printf("\n");

    if ((tbMode == ECHO_PATH_THRU) or (tbMode == ECHO_STORE_FWD)) {

        //-- DUT OUTPUT TRAFFIC AS STREAMS ------------------------------
        string  ofsUSIF_Data_FileName      = "../../../../test/simOutFiles/soUSIF_Data.dat";
        string  ofsUSIF_Meta_FileName      = "../../../../test/simOutFiles/soUSIF_Meta.dat";
        string  ofsUSIF_DLen_FileName      = "../../../../test/simOutFiles/soUSIF_DLen.dat";
        string  ofsUSIF_Data_Gold_FileName = "../../../../test/simOutFiles/soUSIF_Data_Gold.dat";
        string  ofsUSIF_Meta_Gold_FileName = "../../../../test/simOutFiles/soUSIF_Meta_Gold.dat";
        string  ofsUSIF_DLen_Gold_FileName = "../../../../test/simOutFiles/soUSIF_DLen_Gold.dat";

        vector<string>   ofNames;
        ofNames.push_back(ofsUSIF_Data_FileName);
        ofNames.push_back(ofsUSIF_Meta_FileName);
        ofNames.push_back(ofsUSIF_DLen_FileName);
        ofstream         ofStreams[ofNames.size()]; // Stored in the same order

        //-- STEP-1: The setting of the ECHO mode is already done via CFLAGS
        if (tbMode == ECHO_PATH_THRU) {
            printInfo(THIS_NAME, "### TEST_MODE = ECHO_PATH_THRU #########\n");
        }
        else if (tbMode == ECHO_STORE_FWD) {
            printInfo(THIS_NAME, "### TEST_MODE = ECHO_STORE_FWD #########\n");
        }

        //-- STEP-2: Remove previous old files and open new files
        for (int i = 0; i < ofNames.size(); i++) {
            remove(ofNames[i].c_str());
            if (not isDatFile(ofNames[i])) {
                printError(THIS_NAME, "File \'%s\' is not of type \'DAT\'.\n", ofNames[i].c_str());
                nrErr++;
                continue;
            }
            if (!ofStreams[i].is_open()) {
                ofStreams[i].open(ofNames[i].c_str(), ofstream::out);
                if (!ofStreams[i]) {
                    printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofNames[i].c_str());
                    nrErr++;
                    continue;
                }
            }
        }

        //-- STEP-3: Create golden Tx files
        queue<UdpAppMeta>   udpSocketPairs;
        if (NTS_OK == createGoldenTxFiles(tbMode, string(argv[2]), udpSocketPairs,
                ofsUSIF_Data_Gold_FileName, ofsUSIF_Meta_Gold_FileName, ofsUSIF_DLen_Gold_FileName) != NTS_OK) {
            printError(THIS_NAME, "Failed to create golden Tx files. \n");
            nrErr++;
        }

        //-- STEP-4: Create the USIF->UAF INPUT {Data,Meta} as streams
        int nrUSIF_UAF_Chunks=0;
        if (not createUdpRxTraffic(ssUSIF_UAF_Data, "ssUSIF_UAF_Data",
                                   ssUSIF_UAF_Meta, "ssUSIF_UAF_Meta",
                                   string(argv[2]),
                                   udpSocketPairs,
                                   nrUSIF_UAF_Chunks)) {
            printFatal(THIS_NAME, "Failed to create the USIF->UAF traffic as streams.\n");
        }

        //-- STEP-5: Run simulation
        int tbRun = (nrErr == 0) ? (nrUSIF_UAF_Chunks + TB_GRACE_TIME) : 0;
        while (tbRun) {
            udp_app_flash_top(
                    //-- SHELL / Mmio / Configuration Interfaces
                    //[NOT_USED] sSHL_UAF_Mmio_EchoCtrl,
                    //[NOT_USED] sSHL_UAF_Mmio_PostPktEn,
                    //[NOT_USED] sSHL_UAF_Mmio_CaptPktEn,
                    //-- USIF / Rx Data Interfaces
                    ssUSIF_UAF_Data,
                    ssUSIF_UAF_Meta,
                    //-- USIF / Tx Data Interfaces
                    ssUAF_USIF_Data,
                    ssUAF_USIF_Meta,
                    ssUAF_USIF_DLen);
            tbRun--;
            stepSim();
        }

        printInfo(THIS_NAME, "############################################################################\n");
        printInfo(THIS_NAME, "## TESTBENCH 'test_udp_app_flash' ENDS HERE                               ##\n");
        printInfo(THIS_NAME, "############################################################################\n");
        stepSim();

        //-- STEP-6a: Drain UAF-->USIF DATA OUTPUT STREAM
        int nrUAF_USIF_DataChunks=0, nrUAF_USIF_DataGrams=0, nrUAF_USIF_DataBytes=0;
        if (not drainAxisToFile(ssUAF_USIF_Data, "ssUAF_USIF_Data",
            ofNames[0], nrUAF_USIF_DataChunks, nrUAF_USIF_DataGrams, nrUAF_USIF_DataBytes)) {
            printError(THIS_NAME, "Failed to drain UAF-to-USIF data traffic from DUT. \n");
            nrErr++;
        }
        else {
            printInfo(THIS_NAME, "Done with the draining of the UAF-to-USIF data traffic:\n");
            printInfo(THIS_NAME, "\tReceived %d chunks in %d datagrams, for a total of %d bytes.\n\n",
                      nrUAF_USIF_DataChunks, nrUAF_USIF_DataGrams, nrUAF_USIF_DataBytes);
        }
        //-- STEP-6b: Drain UAF-->USIF META OUTPUT STREAM
        int nrUAF_USIF_MetaChunks=0, nrUAF_USIF_MetaGrams=0, nrUAF_USIF_MetaBytes=0;
        if (not drainUdpMetaStreamToFile(ssUAF_USIF_Meta, "ssUAF_USIF_Meta",
                ofNames[1], nrUAF_USIF_MetaChunks, nrUAF_USIF_MetaGrams, nrUAF_USIF_MetaBytes)) {
            printError(THIS_NAME, "Failed to drain UAF-to-USIF meta traffic from DUT. \n");
            nrErr++;
        }
        //-- STEP-6c: Drain UAF-->USIF DLEN OUTPUT STREAM
        int nrUAF_USIF_DLenChunks=0, nrUAF_USIF_DLenGrams=0, nrUAF_USIF_DLenBytes=0;
        if (not drainUdpDLenStreamToFile(ssUAF_USIF_DLen, "ssUAF_USIF_DLen",
                ofNames[2], nrUAF_USIF_DLenChunks, nrUAF_USIF_DLenGrams, nrUAF_USIF_DLenBytes)) {
            printError(THIS_NAME, "Failed to drain UAF-to-USIF dlen traffic from DUT. \n");
            nrErr++;
        }

        //-- STEP-7: Compare output DAT vs gold DAT
        int res = myDiffTwoFiles(std::string(ofsUSIF_Data_FileName),
                                 std::string(ofsUSIF_Data_Gold_FileName));
        if (res) {
            printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
                       ofsUSIF_Data_FileName.c_str(), ofsUSIF_Data_Gold_FileName.c_str());
            nrErr += 1;
        }
        res = myDiffTwoFiles(std::string(ofsUSIF_DLen_FileName),
                             std::string(ofsUSIF_DLen_Gold_FileName));
        if (res) {
            printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
                       ofsUSIF_DLen_FileName.c_str(), ofsUSIF_DLen_Gold_FileName.c_str());
            nrErr += 1;
        }

    }  // End-of: if (tbMode == ECHO_PATH_THRU or ECHO_STORE_FWD) {


    //---------------------------------------------------------------
    //-- PRINT TESTBENCH STATUS
    //---------------------------------------------------------------
    printf("\n\n");
    printInfo(THIS_NAME, "This testbench was executed with the following parameters: \n");
    printInfo(THIS_NAME, "\t==> TB Mode  = %c\n", *argv[1]);
    for (int i=2; i<argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
    }

    if (nrErr) {
        printError(THIS_NAME, "###########################################################\n");
        printError(THIS_NAME, "#### TEST BENCH FAILED : TOTAL NUMBER OF ERROR(S) = %2d ####\n", nrErr);
        printError(THIS_NAME, "###########################################################\n\n");

        printInfo(THIS_NAME, "FYI - You may want to check for \'ERROR\' and/or \'WARNING\' alarms in the LOG file...\n\n");
    }
    else {
        printInfo(THIS_NAME, "#############################################################\n");
        printInfo(THIS_NAME, "####               SUCCESSFUL END OF TEST                ####\n");
        printInfo(THIS_NAME, "#############################################################\n");
    }

    return nrErr;

}

/*! \} */

