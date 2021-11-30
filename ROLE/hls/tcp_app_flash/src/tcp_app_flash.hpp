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
 * @file     : tcp_app_flash.cpp
 * @brief    : TCP Application Flash (TAF)
 *
 * System:   : cloudFPGA
 * Component : cFp_BringUp / ROLE
 * Language  : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details  : This application implements a set of TCP-oriented tests and
 *  functions for the bring-up of a cloudFPGA module.
 *
 * \ingroup tcp_app_flash 
 * \addtogroup tcp_app_flash 
 * \{
 *******************************************************************************/

#ifndef _TAF_H_
#define _TAF_H_

#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts_utils.hpp"

/********************************************************************
 * IMPLEMENTATION DIRECTIVES
 *
 * [TAF_USE_NON_FIFO_IO] This conditional compilation directive is
 *  used to enable/disable the implementation of non-FIFO-IOs
 *  interfaces on the current toplevel function. FYI, the use of non-
 *  FIFO-IOs generates the following warning message during the C
 *  synthesis:
 *    WARNING: [HLS 200-631] Ignoring ap_ctrl_none interface for
 *           tcp_shell_if_top due to tcp_shell_if with non-FIFO I/O.
 ********************************************************************/
#undef TAF_USE_NON_FIFO_IO

//---------------------------------------------------------
//-- SHELL/MMIO/EchoCtrl - Configuration Register
//---------------------------------------------------------
enum EchoCtrl {
    ECHO_STORE_FWD = 0,
    ECHO_PATH_THRU = 1
    //[NOT-USED] ECHO_OFF = 2
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
 * ENTITY - TCP APPLICATION FLASH (TAF)
 *
 *******************************************************************************/
void tcp_app_flash (
        //------------------------------------------------------
        //-- SHELL / MMIO / Configuration Interfaces
        //------------------------------------------------------
    #if defined TAF_USE_NON_FIFO_IO
        ap_uint<2>           piSHL_MmioEchoCtrl,
    #endif
        //------------------------------------------------------
        //-- SHELL / TCP Rx Data Interface
        //------------------------------------------------------
        stream<TcpAppData>  &siTSIF_Data,
        stream<TcpSessId>   &siTSIF_SessId,
        stream<TcpDatLen>   &siTSIF_DatLen,
        //------------------------------------------------------
        //-- SHELL / TCP Tx Data Interface
        //------------------------------------------------------
        stream<TcpAppData>  &soTSIF_Data,
        stream<TcpSessId>   &soTSIF_SessId,
        stream<TcpDatLen>   &soTSIF_DatLen
);

#endif

/*! \} */
