/*****************************************************************************
 * @file       : tcp_app_flash.hpp
 * @brief      : TCP Application Flash (TAF).
 *
 * System:     : cloudFPGA
 * Component   : Role
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the TCP
 *                application embedded in the ROLE of the cloudFPGA flash.
 *
 *****************************************************************************/

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>

using namespace hls;


/********************************************
 * SHELL/MMIO/EchoCtrl - Config Register
 ********************************************/
enum EchoCtrl {
    ECHO_PATH_THRU = 0,
    ECHO_STORE_FWD = 1,
    ECHO_OFF       = 2
};

/********************************************
 * SINGLE BIT DEFINITIONS
 ********************************************/
typedef bool AckBool;  // Acknowledge

/*********************************************************
 * IPv4 - HEADER FIELDS
 *  Default Type Definitions (as used by HLS)
 *********************************************************/
typedef ap_uint<32> Ip4Addr;        // IP4 Source or Destination Address

/*********************************************************
 * TCP - HEADER FIELDS
 *  Default Type Definitions (as used by HLS)
 *********************************************************/
//typedef ap_uint<16> TcpSrcPort;     // TCP Source Port
//typedef ap_uint<16> TcpDstPort;     // TCP Destination Port
typedef ap_uint<16> TcpPort;        // TCP Source or Destination Port Number

//OBSOLETE-20190906 /********************************************
//OBSOLETE-20190906  * TCP Session Id
//OBSOLETE-20190906  ********************************************/
//OBSOLETE-20190906 typedef ap_uint<16> TcpSessId;


/***********************************************
 * Application Data
 *  Data transfered between TOE and APP.
 ***********************************************/
typedef AxiWord     AppData;

//OBSOLETE-20190906 /***********************************************
//OBSOLETE-20190906  * Application Metadata
//OBSOLETE-20190906  *  Meta-data transfered between TOE and APP.
//OBSOLETE-20190906  ***********************************************/
//OBSOLETE-20190906 typedef TcpSessId   AppMeta;

/***********************************************
 * Application Listen Request
 *  The TCP port that the application is willing
 *  to open for listening.
 ***********************************************/
typedef TcpPort     AppLsnReq;

/***********************************************
 * Application Listen Acknowledgment
 *  Acknowledge bit returned by TOE after a
 *  TCP listening port request.
 ***********************************************/
typedef AckBool     AppLsnAck;


/*************************************************************************
 *
 * ENTITY - TCP APPLICATION FLASH (TAF)
 *
 *************************************************************************/
void tcp_app_flash (

        //------------------------------------------------------
        //-- SHELL / MMIO / Configuration Interfaces
        //------------------------------------------------------
        ap_uint<2>          *piSHL_MmioEchoCtrl,
        ap_uint<1>          *piSHL_MmioPostSegEn,
        //[TODO] ap_uint<1> *piSHL_MmioCaptSegEn,

        //------------------------------------------------------
        //-- SHELL / TCP Rx Data Interface
        //------------------------------------------------------
        stream<AppData>     &siSHL_Data,
        stream<AppMeta>     &siSHL_SessId,

        //------------------------------------------------------
        //-- SHELL / TCP Tx Data Interface
        //------------------------------------------------------
        stream<AppData>     &soSHL_Data,
        stream<AppMeta>     &soSHL_SessId
);

