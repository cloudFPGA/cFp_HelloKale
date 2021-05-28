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
 * @file       : udp_shell_if_top.cpp
 * @brief      : Top level with I/O ports for UDP Shell Interface (USIF)
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
 * \addtogroup ROLE_USIF
 * \{
 *******************************************************************************/

#include "udp_shell_if_top.hpp"

using namespace hls;
using namespace std;

#define USE_AP_FIFO

/*****************************************************************************
 * @brief   Top of UDP Shell Interface (USIF)
 *
 * @param[in]  piSHL_Mmio_En Enable signal from [SHELL/MMIO].
 * @param[out] soSHL_LsnReq  Listen port request to [SHELL].
 * @param[in]  siSHL_LsnRep  Listen port reply from [SHELL].
 * @param[out] soSHL_ClsReq  Close port request to [SHELL].
 * @param[in]  siSHL_ClsRep  Close port reply from [SHELL].
 * @param[in]  siSHL_Data    UDP datagram from [SHELL].
 * @param[in]  siSHL_Meta    UDP metadata from [SHELL].
 * @param[out] soSHL_Data    UDP datagram to [SHELL].
 * @param[out] soSHL_Meta    UDP metadata to [SHELL].
 * @param[out] soSHL_DLen    UDP data len to [SHELL].
 * @param[in]  siUAF_Data    UDP datagram from UdpAppFlash (UAF).
 * @param[in]  siUAF_Meta    UDP metadata from [UAF].
 * @param[out] soUAF_Data    UDP datagram to [UAF].
 * @param[out] soUAF_Meta    UDP metadata to [UAF].
 * @param[out] soUAF_DLen    UDP data len to [UAF].
 *
 * @info This toplevel exemplifies the instantiation of a core that was designed
 *   for non-blocking read and write streams, but which needs to be exported as
 *   an IP with AXI-Stream interfaces (axis).
 *   Because the 'axis' interface does not support non-blocking accesses, this
 *   toplevel implements a shim layer between the 'axis' and the 'ap_fifo'
 *   interfaces and vice versa.
 *****************************************************************************/
#if HLS_VERSION == 2016
    void udp_shell_if_top(
        //------------------------------------------------------
        //-- SHELL / Mmio Interface
        //------------------------------------------------------
        CmdBit              *piSHL_Mmio_En,
       //------------------------------------------------------
        //-- SHELL / Control Port Interfaces
        //------------------------------------------------------
        stream<UdpPort>     &soSHL_LsnReq,
        stream<StsBool>     &siSHL_LsnRep,
        stream<UdpPort>     &soSHL_ClsReq,
        stream<StsBool>     &siSHL_ClsRep,
        //------------------------------------------------------
        //-- SHELL / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &siSHL_Data,
        stream<UdpAppMeta>  &siSHL_Meta,
        //------------------------------------------------------
        //-- SHELL / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &soSHL_Data,
        stream<UdpAppMeta>  &soSHL_Meta,
        stream<UdpAppDLen>  &soSHL_DLen,
        //------------------------------------------------------
        //-- UAF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &siUAF_Data,
        stream<UdpAppMetb>  &siUAF_Meta,
        stream<UdpAppDLen>  &siUAF_DLen,
        //------------------------------------------------------
        //-- UAF / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &soUAF_Data,
        stream<UdpAppMetb>  &soUAF_Meta)
{

    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/
    #pragma HLS INTERFACE ap_stable register port=piSHL_Mmio_En name=piSHL_Mmio_En

    //-- [SHL] INTERFACES ------------------------------------------------------
    #pragma HLS resource core=AXI4Stream variable=soSHL_LsnReq  metadata="-bus_bundle soSHL_LsnReq"
    #pragma HLS resource core=AXI4Stream variable=siSHL_LsnRep  metadata="-bus_bundle siSHL_LsnRep"
    #pragma HLS resource core=AXI4Stream variable=soSHL_ClsReq  metadata="-bus_bundle soSHL_ClsReq"
    #pragma HLS resource core=AXI4Stream variable=siSHL_ClsRep  metadata="-bus_bundle siSHL_ClsRep"

    #pragma HLS resource core=AXI4Stream variable=siSHL_Data    metadata="-bus_bundle siSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=siSHL_Meta    metadata="-bus_bundle siSHL_Meta"
    #pragma HLS DATA_PACK                variable=siSHL_Meta

    #pragma HLS resource core=AXI4Stream variable=soSHL_Data    metadata="-bus_bundle soSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=soSHL_Meta    metadata="-bus_bundle soSHL_Meta"
    #pragma HLS DATA_PACK                variable=soSHL_Meta
    #pragma HLS resource core=AXI4Stream variable=soSHL_DLen    metadata="-bus_bundle soSHL_DLen"

    //-- [UAF] INTERFACES ------------------------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siUAF_Data    metadata="-bus_bundle siUAF_Data"
    #pragma HLS resource core=AXI4Stream variable=siUAF_Meta    metadata="-bus_bundle siUAF_Meta"
    #pragma HLS DATA_PACK                variable=siUAF_Meta
    #pragma HLS resource core=AXI4Stream variable=siUAF_DLen    metadata="-bus_bundle siUAF_DLen"

    #pragma HLS resource core=AXI4Stream variable=soUAF_Data    metadata="-bus_bundle soUAF_Data"
    #pragma HLS resource core=AXI4Stream variable=soUAF_Meta    metadata="-bus_bundle soUAF_Meta"
    #pragma HLS DATA_PACK                variable=soUAF_Meta

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW

    //-- INSTANTIATE TOPLEVEL --------------------------------------------------
    udp_shell_if(
        //-- SHELL / Mmio Interface
        piSHL_Mmio_En,
        //-- SHELL / Control Port Interfaces
        soSHL_LsnReq,
        siSHL_LsnRep,
        soSHL_ClsReq,
        siSHL_ClsRep,
        //-- SHELL / Rx Data Interfaces
        siSHL_Data,
        siSHL_Meta,
        //-- SHELL / Tx Data Interfaces
        soSHL_Data,
        soSHL_Meta,
        soSHL_DLen,
        //-- UAF / Tx Data Interfaces
        siUAF_Data,
        siUAF_Meta,
        siUAF_DLen,
        //-- UAF / Rx Data Interfaces
        soUAF_Data,
        soUAF_Meta);
}
#else
    void udp_shell_if_top(
        //------------------------------------------------------
        //-- SHELL / Mmio Interface
        //------------------------------------------------------
        CmdBit              *piSHL_Mmio_En,
       //------------------------------------------------------
        //-- SHELL / Control Port Interfaces
        //------------------------------------------------------
        stream<UdpPort>     &soSHL_LsnReq,
        stream<StsBool>     &siSHL_LsnRep,
        stream<UdpPort>     &soSHL_ClsReq,
        stream<StsBool>     &siSHL_ClsRep,
        //------------------------------------------------------
        //-- SHELL / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &siSHL_Data,
        stream<UdpAppMeta>  &siSHL_Meta,
        //------------------------------------------------------
        //-- SHELL / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &soSHL_Data,
        stream<UdpAppMeta>  &soSHL_Meta,
        stream<UdpAppDLen>  &soSHL_DLen,
        //------------------------------------------------------
        //-- UAF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &siUAF_Data,
        stream<UdpAppMetb>  &siUAF_Meta,
        stream<UdpAppDLen>  &siUAF_DLen,
        //------------------------------------------------------
        //-- UAF / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &soUAF_Data,
        stream<UdpAppMetb>  &soUAF_Meta)
{
    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

  #if defined (USE_AP_FIFO)
    #pragma HLS INTERFACE ap_stable register    port=piSHL_Mmio_En  name=piSHL_Mmio_En

    //-- [SHL] INTERFACES ------------------------------------------------------
    #pragma HLS INTERFACE axis off              port=soSHL_LsnReq   name=soSHL_LsnReq
    #pragma HLS INTERFACE axis off              port=siSHL_LsnRep   name=siSHL_LsnRep
    #pragma HLS INTERFACE axis off              port=soSHL_ClsReq   name=soSHL_ClsReq
    #pragma HLS INTERFACE axis off              port=siSHL_ClsRep   name=siSHL_ClsRep

    #pragma HLS INTERFACE axis off              port=siSHL_Data     name=siSHL_Data
    #pragma HLS INTERFACE axis off              port=siSHL_Meta     name=siSHL_Meta
    #pragma HLS DATA_PACK                   variable=siSHL_Meta

    #pragma HLS INTERFACE axis off              port=soSHL_Data     name=soSHL_Data
    #pragma HLS INTERFACE axis off              port=soSHL_Meta     name=soSHL_Meta
    #pragma HLS DATA_PACK                   variable=soSHL_Meta
    #pragma HLS INTERFACE axis off              port=soSHL_DLen     name=soSHL_DLen

    //-- [UAF] INTERFACES ------------------------------------------------------
    #pragma HLS INTERFACE ap_fifo               port=siUAF_Data     name=siUAF_Data
    #pragma HLS DATA_PACK                   variable=siUAF_Data
    #pragma HLS INTERFACE ap_fifo               port=siUAF_Meta     name=siUAF_Meta
    // [WARNING] Do not pack 'siUSIF_Meta' because the DATA_PACK optimization
    //    does not support packing structs which contain other structs!!!
    #pragma HLS INTERFACE ap_fifo               port=siUAF_DLen     name=siUAF_DLen

    #pragma HLS INTERFACE ap_fifo               port=soUAF_Data     name=soUAF_Data
    #pragma HLS DATA_PACK                   variable=soUAF_Data
    #pragma HLS INTERFACE ap_fifo               port=soUAF_Meta     name=soUAF_Meta
    // [WARNING] Do not pack 'siUSIF_Meta' because the DATA_PACK optimization
    //    does not support packing structs which contain other structs!!!
  #else
    #pragma HLS INTERFACE ap_stable register    port=piSHL_Mmio_En  name=piSHL_Mmio_En

    //-- [SHL] INTERFACES ------------------------------------------------------
    #pragma HLS INTERFACE axis off              port=soSHL_LsnReq   name=soSHL_LsnReq
    #pragma HLS INTERFACE axis off              port=siSHL_LsnRep   name=siSHL_LsnRep
    #pragma HLS INTERFACE axis off              port=soSHL_ClsReq   name=soSHL_ClsReq
    #pragma HLS INTERFACE axis off              port=siSHL_ClsRep   name=siSHL_ClsRep

    #pragma HLS INTERFACE axis off              port=siSHL_Data     name=siSHL_Data
    #pragma HLS INTERFACE axis off              port=siSHL_Meta     name=siSHL_Meta
    #pragma HLS DATA_PACK                   variable=siSHL_Meta

    #pragma HLS INTERFACE axis off              port=soSHL_Data     name=soSHL_Data
    #pragma HLS INTERFACE axis off              port=soSHL_Meta     name=soSHL_Meta
    #pragma HLS DATA_PACK                   variable=soSHL_Meta
    #pragma HLS INTERFACE axis off              port=soSHL_DLen     name=soSHL_DLen

    //-- [UAF] INTERFACES ------------------------------------------------------
    #pragma HLS INTERFACE axis off              port=siUAF_Data     name=siUAF_Data
    #pragma HLS INTERFACE axis off              port=siUAF_Meta     name=siUAF_Meta
    #pragma HLS DATA_PACK                   variable=siUAF_Meta
    #pragma HLS INTERFACE axis off              port=siUAF_DLen     name=siUAF_DLen

    #pragma HLS INTERFACE axis off              port=siUAF_Data     name=soUAF_Data
    #pragma HLS INTERFACE axis off              port=soUAF_Meta     name=soUAF_Meta
    #pragma HLS DATA_PACK                   variable=soUAF_Meta
  #endif

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
  #if HLS_VERSION == 2017
    #pragma HLS DATAFLOW
  #else
    #pragma HLS DATAFLOW disable_start_propagation
  #endif

    //-- INSTANTIATE TOPLEVEL --------------------------------------------------
    udp_shell_if(
        //-- SHELL / Mmio Interface
        piSHL_Mmio_En,
        //-- SHELL / Control Port Interfaces
        soSHL_LsnReq,
        siSHL_LsnRep,
        soSHL_ClsReq,
        siSHL_ClsRep,
        //-- SHELL / Rx Data Interfaces
        siSHL_Data,
        siSHL_Meta,
        //-- SHELL / Tx Data Interfaces
        soSHL_Data,
        soSHL_Meta,
        soSHL_DLen,
        //-- UAF / Tx Data Interfaces
        siUAF_Data,
        siUAF_Meta,
        siUAF_DLen,
        //-- UAF / Rx Data Interfaces
        soUAF_Data,
        soUAF_Meta);

}

#endif  //  HLS_VERSION

/*! \} */
