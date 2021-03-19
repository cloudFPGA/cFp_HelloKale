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

/************************************************
 * INTERFACE SYNTHESIS DIRECTIVES
 *  For the time being, we continue designing
 *  with the DEPRECATED directives because the
 *  new PRAGMAs do not work for us.
 ************************************************/
#undef  USE_DEPRECATED_DIRECTIVES
#define USE_AP_FIFO

/*** [NOT-USED] ***********************
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

#if defined(USE_DEPRECATED_DIRECTIVES)
    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------

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
#else
    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
  #if defined (USE_AP_FIFO)
    #pragma HLS INTERFACE ap_stable register    port=piSHL_Mmio_En  name=piSHL_Mmio_En

    //-- [SHL] INTERFACES ------------------------------------------------------
    #pragma HLS INTERFACE axis register off     port=soSHL_LsnReq   name=soSHL_LsnReq
    #pragma HLS INTERFACE axis register off     port=siSHL_LsnRep   name=siSHL_LsnRep
    #pragma HLS INTERFACE axis register off     port=soSHL_ClsReq   name=soSHL_ClsReq
    #pragma HLS INTERFACE axis register off     port=siSHL_ClsRep   name=siSHL_ClsRep

    #pragma HLS INTERFACE axis register off     port=siSHL_Data     name=siSHL_Data
    #pragma HLS INTERFACE axis register off     port=siSHL_Meta     name=siSHL_Meta
    #pragma HLS DATA_PACK                   variable=siSHL_Meta

    #pragma HLS INTERFACE axis register off     port=soSHL_Data     name=soSHL_Data
    #pragma HLS INTERFACE axis register off     port=soSHL_Meta     name=soSHL_Meta
    #pragma HLS DATA_PACK                   variable=soSHL_Meta
    #pragma HLS INTERFACE axis register off     port=soSHL_DLen     name=soSHL_DLen

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
    #pragma HLS INTERFACE axis register off     port=soSHL_LsnReq   name=soSHL_LsnReq
    #pragma HLS INTERFACE axis register off     port=siSHL_LsnRep   name=siSHL_LsnRep
    #pragma HLS INTERFACE axis register off     port=soSHL_ClsReq   name=soSHL_ClsReq
    #pragma HLS INTERFACE axis register off     port=siSHL_ClsRep   name=siSHL_ClsRep

    #pragma HLS INTERFACE axis register off     port=siSHL_Data     name=siSHL_Data
    #pragma HLS INTERFACE axis register off     port=siSHL_Meta     name=siSHL_Meta
    #pragma HLS DATA_PACK                   variable=siSHL_Meta

    #pragma HLS INTERFACE axis register off     port=soSHL_Data     name=soSHL_Data
    #pragma HLS INTERFACE axis register off     port=soSHL_Meta     name=soSHL_Meta
    #pragma HLS DATA_PACK                   variable=soSHL_Meta
    #pragma HLS INTERFACE axis register off     port=soSHL_DLen     name=soSHL_DLen

    //-- [UAF] INTERFACES ------------------------------------------------------
    #pragma HLS INTERFACE axis register off     port=siUAF_Data     name=siUAF_Data
    #pragma HLS INTERFACE axis register off     port=siUAF_Meta     name=siUAF_Meta
    #pragma HLS DATA_PACK                   variable=siUAF_Meta
    #pragma HLS INTERFACE axis register off     port=siUAF_DLen     name=siUAF_DLen

    #pragma HLS INTERFACE axis register off     port=siUAF_Data     name=soUAF_Data
    #pragma HLS INTERFACE axis register off     port=soUAF_Meta     name=soUAF_Meta
    #pragma HLS DATA_PACK                   variable=soUAF_Meta
  #endif
#endif

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
  #if HLS_VERSION == 2017
    #pragma HLS DATAFLOW
  #else
    #pragma HLS DATAFLOW disable_start_propagation
  #endif
  #pragma HLS INTERFACE ap_ctrl_none port=return

    /*** [NOT-USED] ************************

    //-- LOCAL IN and OUT STREAMS ----------------------------------------------
    //--  FYI, an internal hls::stream<> is implemented as a FIFO with a default
    //--  depth of 2. You may change it with the optimization directive 'STREAM'.
    //--    E.g., #pragma HLS STREAM variable=ssiUAF_Data depth=4
    static stream<UdpPort>       ssoSHL_LsnReq  ("ssoSHL_LsnReq");
    static stream<StsBool>       ssiSHL_LsnRep  ("ssiSHL_LsnRep");
    static stream<UdpPort>       ssoSHL_ClsReq  ("ssoSHL_ClsReq");
    static stream<StsBool>       ssiSHL_ClsRep  ("ssiSHL_ClsRep");
    static stream<UdpAppData>    ssiSHL_Data    ("ssiSHL_Data");
    static stream<UdpAppMeta>    ssiSHL_Meta    ("ssiSHL_Meta");
    static stream<UdpAppData>    ssoSHL_Data    ("ssoSHL_Data");
    static stream<UdpAppMeta>    ssoSHL_Meta    ("ssoSHL_Meta");
    static stream<UdpAppDLen>    ssoSHL_DLen    ("ssoSHL_DLen");
    static stream<UdpAppData>    ssiUAF_Data    ("ssiUAF_Data");
    static stream<UdpAppMeta>    ssiUAF_Meta    ("ssiUAF_Meta");
    static stream<UdpAppDLen>    ssiUAF_DLen    ("ssiUAF_DLen");
    static stream<UdpAppData>    ssoUAF_Data    ("ssoUAF_Data");
    static stream<UdpAppMeta>    ssoUAF_Meta    ("ssoUAF_Meta");

    //-- INPUT AXIS INTERFACES -------------------------------------------------
    pAxisToFifo(siSHL_LsnRep, ssiSHL_LsnRep);
    pAxisToFifo(siSHL_ClsRep, ssiSHL_ClsRep);
    pAxisToFifo(siSHL_Data,   ssiSHL_Data);
    pAxisToFifo(siSHL_Meta,   ssiSHL_Meta);
    ****************************************/

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

    /*** [NOT-USED] ************************
    //-- OUTPUT INTERFACES -----------------------------------------------------
    pFifoToAxis(ssoSHL_LsnReq, soSHL_LsnReq);
    pFifoToAxis(ssoSHL_ClsReq, soSHL_ClsReq);
    pFifoToAxis(ssoSHL_Data,   soSHL_Data);
    pFifoToAxis(ssoSHL_Meta,   soSHL_Meta);
    pFifoToAxis(ssoSHL_DLen,   soSHL_DLen);
    pFifoToAxis(ssoUAF_Data,   soUAF_Data);
    pFifoToAxis(ssoUAF_Meta,   soUAF_Meta);
    ****************************************/

}

/*! \} */
