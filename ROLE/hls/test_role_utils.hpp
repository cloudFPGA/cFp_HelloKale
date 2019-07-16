/*****************************************************************************
 * @file       : test_role_utils.hpp
 * @brief      : Utilities for the test of the ROLE
 *
 * System:     : cloudFPGA
 * Component   : RoleFlash
 * Language    : Vivado HLS
 *
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#ifndef TEST_ROLE_UTILS_H_
#define TEST_ROLE_UTILS_H_

//OBSOLETE #include "../src/toe.hpp"
#include "./test_role_utils.hpp"

using namespace std;


/******************************************************************************
 * FORWARD DECLARATIONS
 ******************************************************************************/
class SockAddr;
class SocketPair;
//OBSOLETE class AxiSockAddr;
//OBSOLETE class AxiSocketPair;
//OBSOLETE class AxiWord;
//OBSOLETE class DmCmd;
//OBSOLETE struct fourTupleInternal;


/******************************************************************************
 * PRINT PROTOTYPE DEFINITIONS
 *******************************************************************************/
void printAxiWord      (const char *callerName, AxiWord       chunk);
//void printDmCmd        (const char *callerName, DmCmd         dmCmd);
void printSockAddr     (const char *callerName, SockAddr      sockAddr);
//void printSockAddr     (const char *callerName, AxiSockAddr   axiSockAddr);
void printSockAddr     (                        SockAddr      sockAddr);
void printSockPair     (const char *callerName, SocketPair    sockPair);
//void printSockPair     (const char *callerName, AxiSocketPair axiSockPair);
//void printSockPair     (const char *callerName, int  src,     fourTupleInternal fourTuple);
//void printAxiSockAddr  (const char *callerName, AxiSockAddr   axiSockAddr);
//void printAxiSockPair  (const char *callerName, AxiSocketPair axiSockPair);
void printIp4Addr      (const char *callerName, Ip4Addr       ip4Addr);
void printIp4Addr      (                        Ip4Addr       ip4Addr);
void printTcpPort      (const char *callerName, TcpPort       tcpPort);
void printTcpPort      (                        TcpPort       tcpPort);

/******************************************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  FYI: The global variable 'gTraceEvent' is set
 *        whenever a trace call is done.
 ******************************************************************************/
#ifndef __SYNTHESIS__
  extern bool         gTraceEvent;
  extern bool         gFatalError;
  extern unsigned int gSimCycCnt;
#endif

/******************************************************************************
 * MACRO DEFINITIONS
 ******************************************************************************/
// Concatenate two char constants
#define concat2(firstCharConst, secondCharConst) firstCharConst secondCharConst
// Concatenate three char constants
#define concat3(firstCharConst, secondCharConst, thirdCharConst) firstCharConst secondCharConst thirdCharConst

/**********************************************************
 * @brief A macro to print an information message.
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] message,      the message to print.
 **********************************************************/
#ifndef __SYNTHESIS__
  #define printInfo(callerName , format, ...) \
    do { gTraceEvent = true; printf("(@%5.5d) [%s] INFO - " format, gSimCycCnt, callerName, ##__VA_ARGS__); } while (0)
#else
  #define printInfo(callerName , format, ...) \
    do {} while (0);
#endif

/**********************************************************
 * @brief A macro to print a warning message.
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] message,      the message to print.
 **********************************************************/
#ifndef __SYNTHESIS__
  #define printWarn(callerName , format, ...) \
    do { gTraceEvent = true; printf("(@%5.5d) [%s] WARNING - " format, gSimCycCnt, callerName, ##__VA_ARGS__); } while (0)
#else
  #define printWarn(callerName , format, ...) \
    do {} while (0);
#endif

/**********************************************************
 * @brief A macro to print an error message.
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] message,      the message to print.
 **********************************************************/
#ifndef __SYNTHESIS__
  #define printError(callerName , format, ...) \
    do { gTraceEvent = true; printf("(@%5.5d) [%s] ERROR - " format, gSimCycCnt, callerName, ##__VA_ARGS__); } while (0)
#else
  #define printError(callerName , format, ...) \
    do {} while (0);
#endif

/**********************************************************
 * @brief A macro to print a fatal error message and exit.
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] message,      the message to print.
 **********************************************************/
#ifndef __SYNTHESIS__
  #define printFatal(callerName , format, ...) \
    do { gTraceEvent = true; gFatalError = true; printf("(@%5.5d) [%s] FATAL - " format, gSimCycCnt, callerName, ##__VA_ARGS__); exit(99); } while (0)
#else
  #define printFatal(callerName , format, ...) \
    do {} while (0);
#endif


/******************************************************************************
 * HELPER PROTOTYPE DEFINITIONS
 *******************************************************************************/
bool isDottedDecimal   (string ipStr);
bool isHexString       (string str);

//ap_uint<32>    myDottedDecimalIpToUint32(string ipStr);
//vector<string> myTokenizer     (string      strBuff, char delimiter);
//string         myUint64ToStrHex(ap_uint<64> inputNumber);
//string         myUint8ToStrHex (ap_uint<8>  inputNumber);
//ap_uint<64>    myStrHexToUint64(string      dataString);
//ap_uint<8>     myStrHexToUint8 (string      keepString);

#endif
