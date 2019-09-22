/*****************************************************************************
 * @file       : tcp_app_flash.hpp
 * @brief      : TCP Application Flash (TAF).
 *
 * System:     : cloudFPGA
 * Component   : Role
 * Language    : Vivado HLS
 *
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

