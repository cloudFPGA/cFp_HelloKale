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
 * @brief    : Top level with I/O ports for TCP Application Flash (TAF)
 *
 * System:   : cloudFPGA
 * Component : cFp_Monolithic / ROLE
 * Language  : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details This toplevel implements support for the interface synthesis process
 *   in which arguments and parameters of the core function are synthesized into
 *   RTL ports. The type of interfaces that are created by interface synthesis
 *   are directed by the pragma 'HLS INTERFACE'.
 * 
 * \ingroup ROLE
 * \addtogroup ROLE_TAF
 * \{
 *******************************************************************************/

#include "tcp_app_flash_top.hpp"

using namespace hls;
using namespace std;

/************************************************
 * INTERFACE SYNTHESIS DIRECTIVES
 *  For the time being, we may continue to design
 *  with the DEPRECATED directives because the
 *  new PRAGMAs do not always work for us.
 ************************************************/
#undef USE_DEPRECATED_DIRECTIVES

/*******************************************************************************
 * @brief   Top of TCP Application Flash (TAF)
 *
 * @param[in]  piSHL_MmioEchoCtrl  Configures the echo function.
 * @param[in]  siTSIF_Data         TCP data stream from TcpShellInterface (TSIF).
 * @param[in]  siTSIF_SessId       TCP session-id from [TSIF].
 * @param[in]  siTSIF_DatLen       TCP data-length from [TSIF].
 * @param[out] soTSIF_Data         TCP data stream to [TSIF].
 * @param[out] soTSIF_SessId       TCP session-id to [TSIF].
 * @param[out  soTSIF_DatLen       TCP data-length to [TSIF].
 *
 *******************************************************************************/
void tcp_app_flash_top (
        //------------------------------------------------------
        //-- SHELL / MMIO / Configuration Interfaces
        //------------------------------------------------------
        ap_uint<2>          *piSHL_MmioEchoCtrl,
        //------------------------------------------------------
        //-- TSIF / Rx Data Interface
        //------------------------------------------------------
        stream<AxisRaw>     &siTSIF_Data,
        stream<TcpSessId>   &siTSIF_SessId,
        stream<TcpDatLen>   &siTSIF_DatLen,
        //------------------------------------------------------
        //-- TSIF / Tx Data Interface
        //------------------------------------------------------
        stream<AxisRaw>     &soTSIF_Data,
        stream<TcpSessId>   &soTSIF_SessId,
        stream<TcpDatLen>   &soTSIF_DatLen)
{
    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

#if defined(USE_DEPRECATED_DIRECTIVES)
    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/
    #pragma HLS INTERFACE ap_stable    port=piSHL_MmioEchoCtrl

    #pragma HLS resource core=AXI4Stream variable=siTSIF_Data   metadata="-bus_bundle siTSIF_Data"
    #pragma HLS resource core=AXI4Stream variable=siTSIF_SessId metadata="-bus_bundle siTSIF_SessId"
    #pragma HLS resource core=AXI4Stream variable=siTSIF_DatLen metadata="-bus_bundle siTSIF_DatLen"

    #pragma HLS resource core=AXI4Stream variable=soTSIF_Data   metadata="-bus_bundle soTSIF_Data"
    #pragma HLS resource core=AXI4Stream variable=soTSIF_SessId metadata="-bus_bundle soTSIF_SessId"
    #pragma HLS resource core=AXI4Stream variable=soTSIF_DatLen metadata="-bus_bundle soTSIF_DatLen"
#else
    #pragma HLS INTERFACE ap_stable      port=piSHL_MmioEchoCtrl

    #pragma HLS INTERFACE axis register  port=siTSIF_Data   name=siTSIF_Data
    #pragma HLS INTERFACE axis register  port=siTSIF_SessId name=siTSIF_SessId
    #pragma HLS INTERFACE axis register  port=siTSIF_DatLen name=siTSIF_DatLen

    #pragma HLS INTERFACE axis register  port=soTSIF_Data   name=soTSIF_Data
    #pragma HLS INTERFACE axis register  port=soTSIF_SessId name=soTSIF_SessId
    #pragma HLS INTERFACE axis register  port=soTSIF_DatLen name=soTSIF_DatLen
#endif

    //-- LOCAL IN and OUT STREAMS ----------------------------------------------
    static stream<TcpAppData>   ssiTSIF_Data    ("ssiTSIF_Data");
    #pragma HLS STREAM variable=ssiTSIF_Data    depth=2
    static stream<TcpSessId>    ssiTSIF_SessId  ("ssiTSIF_SessId");
    #pragma HLS STREAM variable=ssiTSIF_SessId  depth=2
    static stream<TcpDatLen>    ssiTSIF_DatLen  ("ssiTSIF_DatLen");
    #pragma HLS STREAM variable=ssiTSIF_DatLen  depth=2

    static stream<TcpAppData>   ssoTSIF_Data    ("ssoTSIF_Data");
    #pragma HLS STREAM variable=ssoTSIF_Data    depth=2
    static stream<TcpSessId>    ssoTSIF_SessId  ("ssoTSIF_SessId");
    #pragma HLS STREAM variable=ssoTSIF_SessId  depth=2
    static stream<TcpDatLen>    ssoTSIF_DatLen  ("ssoTSIF_DatLen");
    #pragma HLS STREAM variable=ssoTSIF_DatLen  depth=2


    //-- INPUT INTERFACES ------------------------------------------------------
    if (!siTSIF_Data.empty() and !ssiTSIF_Data.full()) {
        ssiTSIF_Data.write(siTSIF_Data.read());
    }
    if (!siTSIF_SessId.empty() and !ssiTSIF_SessId.full()) {
        ssiTSIF_SessId.write(siTSIF_SessId.read());
    }
    if (!siTSIF_DatLen.empty() and !ssiTSIF_DatLen.full()) {
        ssiTSIF_DatLen.write(siTSIF_DatLen.read());
    }

    //-- INSTANTIATE TOPLEVEL --------------------------------------------------
    tcp_app_flash (
        //------------------------------------------------------
        //-- SHELL / MMIO / Configuration Interfaces
        //------------------------------------------------------
        piSHL_MmioEchoCtrl,
        //------------------------------------------------------
        //-- SHELL / TCP Rx Data Interface
        //------------------------------------------------------
        ssiTSIF_Data,
        ssiTSIF_SessId,
        ssiTSIF_DatLen,
        //------------------------------------------------------
        //-- SHELL / TCP Tx Data Interface
        //------------------------------------------------------
        ssoTSIF_Data,
        ssoTSIF_SessId,
        ssoTSIF_DatLen);

    //-- OUTPUT INTERFACES -----------------------------------------------------
    if (!ssoTSIF_Data.empty() and !soTSIF_Data.full()) {
        soTSIF_Data.write(ssoTSIF_Data.read());
    }
    if (!ssoTSIF_SessId.empty() and !soTSIF_SessId.full()) {
        soTSIF_SessId.write(ssoTSIF_SessId.read());
    }
    if (!ssoTSIF_DatLen.empty() and !soTSIF_DatLen.full()) {
        soTSIF_DatLen.write(ssoTSIF_DatLen.read());
    }

}

/*! \} */
