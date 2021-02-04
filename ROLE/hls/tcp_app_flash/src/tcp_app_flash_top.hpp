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
 * @file     : tcp_app_flash_top.cpp
 * @brief    : Top of TCP Application Flash (TAF)
 *
 * System:   : cloudFPGA
 * Component : cFp_BringUp / ROLE
 * Language  : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details  :  Data structures, types and prototypes definitions for the TCP
 *                application embedded in the role of the cFp_BringUp.
 *
 * \ingroup ROLE
 * \addtogroup ROLE_TAF
 * \{
 *******************************************************************************/

#ifndef _TAF_TOP_H_
#define _TAF_TOP_H_

//OBSOLETE #include <stdio.h>
//OBSOLETE #include <fstream>
//OBSOLETE #include <string>
//OBSOLETE #include <math.h>
//OBSOLETE #include <hls_stream.h>
//OBSOLETE #include "ap_int.h"
//OBSOLETE #include <stdint.h>

#include "../src/tcp_app_flash_top.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts_utils.hpp"

//---------------------------------------------------------
/// -- SHELL/MMIO/EchoCtrl - Configuration Register
//---------------------------------------------------------
enum EchoCtrl {
    ECHO_PATH_THRU = 0,
    ECHO_STORE_FWD = 1,
    ECHO_OFF       = 2
};

//-------------------------------------------------------------------
//-- DEFAULT TESTING PORTS
//--  By default, the following port numbers will be used by the
//--  TcpApplicationFlash (unless user specifies new ones via TBD).
//--  Default testing ports:
//--  --> 8803  : Traffic received on this port is looped back and
//--              echoed to the sender in path-through mode.
//--  --> Others: Traffic received on any port != 8803 is looped back
//--              and echo to the sender in store-and-forward mode.
//-------------------------------------------------------------------
#define ECHO_PATH_THRU_PORT  8803   // 0x2263


/*******************************************************************************
 *
 * ENTITY - TCP APPLICATION FLASH TOP (TAF_TOP)
 *
 *******************************************************************************/
void tcp_app_flash_top (
        //------------------------------------------------------
        //-- SHELL / MMIO / Configuration Interfaces
        //------------------------------------------------------
        ap_uint<2>          *piSHL_MmioEchoCtrl,
        //------------------------------------------------------
        //-- TSIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<AxisRaw>     &siTSIF_Data,
        stream<TcpSessId>   &siTSIF_SessId,
        stream<TcpDatLen>   &siTSIF_DatLen,
        //------------------------------------------------------
        //-- TSIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<AxisRaw>     &soTSIF_Data,
        stream<TcpSessId>   &soTSIF_SessId,
        stream<TcpDatLen>   &soTSIF_DatLen
);

#endif

/*! \} */
