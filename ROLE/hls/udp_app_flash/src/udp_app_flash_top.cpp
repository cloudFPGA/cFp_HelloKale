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
 * @file       : udp_app_flash_top.cpp
 * @brief      : Top level with I/O ports for UDP Application Flash (UAF)
 *
 * System:     : cloudFPGA
 * Component   : cFp_Monolithic/ROLE
 * Language    : Vivado HLS
 *
 *------------------------------------------------------------------------------
 *
 * @details This toplevel implements support for the interface synthesis process
 *   in which arguments and parameters of the core function are synthesized into
 *   RTL ports. The type of interfaces that are created by interface synthesis
 *   are directed by the pragma 'HLS INTERFACE'.
 *
 * \ingroup ROLE
 * \addtogroup ROLE_UAF
 * \{
 *******************************************************************************/

#include "udp_app_flash.hpp"

using namespace hls;
using namespace std;

/************************************************
 * INTERFACE SYNTHESIS DIRECTIVES
 *  For the time being, we continue to design
 *  with the DEPRECATED directives because the
 *  new PRAGMAs do not always work for us.
 ************************************************/
#undef USE_DEPRECATED_DIRECTIVES


/*******************************************************************************
 * @brief   Top of UDP Application Flash (UAF)
 *
 * @param[in]  piSHL_Mmio_EchoCtrl  Configures the echo function.
 * @param[in]  piSHL_Mmio_PostPktEn Enables posting of UDP packets.
 * @param[in]  piSHL_Mmio_CaptPktEn Enables capture of UDP packets.
 * @param[in]  siUSIF_Data          UDP datagram from UdpShellInterface (USIF).
 * @param[in]  siUSIF_Meta          UDP metadata from [USIF].
 * @param[out] soUSIF_Data          UDP datagram to [USIF].
 * @param[out] soUSIF_Meta          UDP metadata to [USIF].
 * @param[out] soUSIF_DLen          UDP data len to [USIF].
 *
 *******************************************************************************/
void udp_app_flash_top (

        //------------------------------------------------------
        //-- SHELL / Mmio / Configuration Interfaces
        //------------------------------------------------------
        //[NOT_USED] ap_uint<2>  piSHL_Mmio_EchoCtrl,
        //[NOT_USED] ap_uint<1>  piSHL_Mmio_PostPktEn,
        //[NOT_USED] ap_uint<1>  piSHL_Mmio_CaptPktEn,

        //------------------------------------------------------
        //-- USIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<AxisRaw>     &siUSIF_Data,
        stream<UdpAppMeta>  &siUSIF_Meta,

        //------------------------------------------------------
        //-- USIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<AxisRaw>     &soUSIF_Data,
        stream<UdpAppMeta>  &soUSIF_Meta,
        stream<UdpAppDLen>  &soUSIF_DLen)
{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

#if defined(USE_DEPRECATED_DIRECTIVES)

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

    //[NOT_USED] #pragma HLS INTERFACE ap_stable port=piSHL_Mmio_EchoCtrl
    //[NOT_USED] #pragma HLS INTERFACE ap_stable port=piSHL_Mmio_PostPktEn
    //[NOT_USED] #pragma HLS INTERFACE ap_stable port=piSHL_Mmio_CaptPktEn

    #pragma HLS resource core=AXI4Stream variable=siUSIF_Data    metadata="-bus_bundle siUSIF_Data"
    #pragma HLS resource core=AXI4Stream variable=siUSIF_Meta    metadata="-bus_bundle siUSIF_Meta"
    #pragma HLS DATA_PACK                variable=siUSIF_Meta

    #pragma HLS resource core=AXI4Stream variable=soUSIF_Data    metadata="-bus_bundle soUSIF_Data"
    #pragma HLS resource core=AXI4Stream variable=soUSIF_Meta    metadata="-bus_bundle soUSIF_Meta"
    #pragma HLS DATA_PACK                variable=soUSIF_Meta
    #pragma HLS resource core=AXI4Stream variable=soUSIF_DLen    metadata="-bus_bundle soUSIF_DLen"

#else

    //[NOT_USED] #pragma HLS INTERFACE ap_stable register port=piSHL_Mmio_EchoCtrl  name=piSHL_Mmio_EchoCtrl
    //[NOT_USED] #pragma HLS INTERFACE ap_stable port=piSHL_Mmio_PostPktEn
    //[NOT_USED] #pragma HLS INTERFACE ap_stable port=piSHL_Mmio_CaptPktEn

    #pragma HLS INTERFACE axis register      port=siUSIF_Data     name=siUSIF_Data
    #pragma HLS INTERFACE axis register      port=siUSIF_Meta     name=siUSIF_Meta
    #pragma HLS DATA_PACK                variable=siUSIF_Meta instance=siUSIF_Meta

    #pragma HLS INTERFACE axis register      port=soUSIF_Data     name=soUSIF_Data
    #pragma HLS INTERFACE axis register      port=soUSIF_Meta     name=soUSIF_Meta
	#pragma HLS DATA_PACK                variable=soUSIF_Meta instance=soUSIF_Meta
    #pragma HLS INTERFACE axis register      port=soUSIF_DLen     name=soUSIF_DLen
    #pragma HLS DATA_PACK                variable=soUSIF_DLen instance=soUSIF_DLen

#endif

	//-- LOCAL IN and OUT STREAMS ----------------------------------------------
    static stream<UdpAppData>       ssiUSIF_Data  ("ssiUSIF_Data");
    #pragma HLS STREAM     variable=ssiUSIF_Data  depth=2
    static stream<UdpAppMeta>       ssiUSIF_Meta  ("ssiUSIF_Meta");
    #pragma HLS STREAM     variable=ssiUSIF_Meta  depth=2

    static stream<UdpAppData>       ssoUSIF_Data  ("ssoUSIF_Data");
    #pragma HLS STREAM     variable=ssoUSIF_Data  depth=2
    static stream<UdpAppMeta>       ssoUSIF_Meta  ("ssoUSIF_Meta");
    #pragma HLS STREAM     variable=ssoUSIF_Meta  depth=2
    static stream<UdpAppDLen>       ssoUSIF_DLen  ("ssoUSIF_DLen");
    #pragma HLS STREAM     variable=ssoUSIF_DLen  depth=2

    //-- INPUT INTERFACES ------------------------------------------------------
    if (!siUSIF_Data.empty() and !ssiUSIF_Data.full()) {
        ssiUSIF_Data.write(siUSIF_Data.read());
    }
    if (!siUSIF_Meta.empty() and !ssiUSIF_Meta.full()) {
        ssiUSIF_Meta.write(siUSIF_Meta.read());
    }

	//-- INSTANTIATE TOPLEVEL --------------------------------------------------
    udp_app_flash (

        //------------------------------------------------------
        //-- SHELL / Mmio / Configuration Interfaces
        //------------------------------------------------------
        //[NOT_USED] piSHL_Mmio_EchoCtrl,
        //[NOT_USED] piSHL_Mmio_PostPktEn,
        //[NOT_USED] piSHL_Mmio_CaptPktEn,
        //------------------------------------------------------
        //-- USIF / Rx Data Interfaces
        //------------------------------------------------------
        ssiUSIF_Data,
        ssiUSIF_Meta,
        //------------------------------------------------------
        //-- USIF / Tx Data Interfaces
        //------------------------------------------------------
        ssoUSIF_Data,
        ssoUSIF_Meta,
        ssoUSIF_DLen);

    //-- OUTPUT INTERFACES -----------------------------------------------------
    if (!ssoUSIF_Data.empty() and !soUSIF_Data.full()) {
        soUSIF_Data.write(ssoUSIF_Data.read());
    }
    if (!ssoUSIF_Meta.empty() and !soUSIF_Meta.full()) {
        soUSIF_Meta.write(ssoUSIF_Meta.read());
    }
    if (!ssoUSIF_DLen.empty() and !soUSIF_DLen.full()) {
        soUSIF_DLen.write(ssoUSIF_DLen.read());
    }

}

/*! \} */
