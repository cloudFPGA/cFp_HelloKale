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
 * Component   : cFp_Monolithic / ROLE
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
#undef  USE_DEPRECATED_DIRECTIVES
#define USE_AP_FIFO

/*** OBSOLETE_20210222 ****************
template <class Type>
void pAxisToFifo(
        stream<Type>& siAxisStream,
        stream<Type>& soFifoStream)
{
    #pragma HLS INLINE off
    Type currChunk;
    siAxisStream.read(currChunk);  // Blocking read
    if (!soFifoStream.full()) {
        soFifoStream.write(currChunk);  // else drop
    }
}

template <class Type>
void pFifoToAxis(
        stream<Type>& siFifoStream,
        stream<Type>& soAxisStream)
{
    #pragma HLS INLINE off
    Type currChunk;
    if (!siFifoStream.empty()) {
        siFifoStream.read(currChunk);
        soAxisStream.write(currChunk);  // Blocking write
    }
}
***************************************/

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
 * @info This toplevel exemplifies the instantiation of a core that was designed
 *   for non-blocking read and write streams, but which needs to be exported as
 *   an IP with AXI-Stream interfaces (axis).
 *   Because the 'axis' interface does not support non-blocking accesses, this
 *   toplevel implements a shim layer between the 'axis' and the 'ap_fifo'
 *   interfaces and vice versa.
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
        stream<UdpAppData>  &siUSIF_Data,
        stream<UdpAppMeta>  &siUSIF_Meta,

        //------------------------------------------------------
        //-- USIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &soUSIF_Data,
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
  #if defined (USE_AP_FIFO)
    //[NOT_USED] #pragma HLS INTERFACE ap_stable register port=piSHL_Mmio_EchoCtrl  name=piSHL_Mmio_EchoCtrl
    //[NOT_USED] #pragma HLS INTERFACE ap_stable port=piSHL_Mmio_PostPktEn
    //[NOT_USED] #pragma HLS INTERFACE ap_stable port=piSHL_Mmio_CaptPktEn

    //-- [USIF] INTERFACES ------------------------------------------------------
    #pragma HLS INTERFACE ap_fifo   port=siUSIF_Data    name=siUSIF_Data
    //OBSOLETE_20210224 #pragma HLS DATA_PACK       variable=siUSIF_Data
    #pragma HLS INTERFACE ap_fifo   port=siUSIF_Meta    name=siUSIF_Meta
    // [WARNING] Do not pack 'siUSIF_Meta' because the DATA_PACK optimization
    //    does not support packing structs which contain other structs!!!

    #pragma HLS INTERFACE ap_fifo   port=soUSIF_Data    name=soUSIF_Data
    #pragma HLS DATA_PACK       variable=soUSIF_Data
    #pragma HLS INTERFACE ap_fifo   port=soUSIF_Meta    name=soUSIF_Meta
    // [WARNING] Do not pack 'siUSIF_Meta' because the DATA_PACK optimization
    //    does not support packing structs which contain other structs!!!
    #pragma HLS INTERFACE ap_fifo   port=soUSIF_DLen    name=soUSIF_DLen
  #else
    //[NOT_USED] #pragma HLS INTERFACE ap_stable register port=piSHL_Mmio_EchoCtrl  name=piSHL_Mmio_EchoCtrl
    //[NOT_USED] #pragma HLS INTERFACE ap_stable port=piSHL_Mmio_PostPktEn
    //[NOT_USED] #pragma HLS INTERFACE ap_stable port=piSHL_Mmio_CaptPktEn

    //-- [USIF] INTERFACES ------------------------------------------------------
    #pragma HLS INTERFACE axis  register off   port=siUSIF_Data    name=siUSIF_Data
    #pragma HLS INTERFACE axis  register off   port=siUSIF_Meta    name=siUSIF_Meta
    // [WARNING] Do not pack 'siUSIF_Meta' because the DATA_PACK optimization
    //    does not support packing structs which contain other structs!!!
    #pragma HLS INTERFACE axis  register off   port=soUSIF_Data    name=soUSIF_Data
    #pragma HLS INTERFACE axis  register off   port=soUSIF_Meta    name=soUSIF_Meta
    // [WARNING] Do not pack 'siUSIF_Meta' because the DATA_PACK optimization
    //    does not support packing structs which contain other structs!!!
    #pragma HLS INTERFACE axis  register off   port=soUSIF_DLen    name=soUSIF_DLen
  #endif
#endif

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW

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
        siUSIF_Data,
        siUSIF_Meta,
        //------------------------------------------------------
        //-- USIF / Tx Data Interfaces
        //------------------------------------------------------
        soUSIF_Data,
        soUSIF_Meta,
        soUSIF_DLen);

}

/*! \} */
