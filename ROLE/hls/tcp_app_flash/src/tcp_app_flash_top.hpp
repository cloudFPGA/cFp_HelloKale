/*
 * Copyright 2016 -- 2021 IBM Corporation
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
 * @file     : tcp_app_flash_top.hpp
 * @brief    : Top of TCP Application Flash (TAF)
 *
 * System:   : cloudFPGA
 * Component : cFp_HelloKale / ROLE
 * Language  : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details  :  Data structures, types and prototypes definitions for the TCP
 *                application embedded in the role of the cFp_HelloKale.
 *
 * \ingroup tcp_app_flash
 * \addtogroup tcp_app_flash
 * \{
 *******************************************************************************/

#ifndef _TAF_TOP_H_
#define _TAF_TOP_H_

#include "./tcp_app_flash.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts_utils.hpp"


/*******************************************************************************
 *
 * ENTITY - TCP APPLICATION FLASH TOP (TAF_TOP)
 *
 *******************************************************************************/
void tcp_app_flash_top (
        //------------------------------------------------------
        //-- SHELL / MMIO / Configuration Interfaces
        //------------------------------------------------------
      #if defined TAF_USE_NON_FIFO_IO
        ap_uint<2>           piSHL_MmioEchoCtrl,
      #endif
        //------------------------------------------------------
        //-- TSIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<TcpAppData>  &siTSIF_Data,
        stream<TcpSessId>   &siTSIF_SessId,
        stream<TcpDatLen>   &siTSIF_DatLen,
        //------------------------------------------------------
        //-- TSIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<TcpAppData>  &soTSIF_Data,
        stream<TcpSessId>   &soTSIF_SessId,
        stream<TcpDatLen>   &soTSIF_DatLen
);

#endif

/*! \} */
