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
 * @file       : tcp_shell_if_top.cpp
 * @brief      : Top level with I/O ports for TCP Shell Interface (TSIF)
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
 * \addtogroup ROLE_TSIF
 * \{
 *******************************************************************************/

#include "tcp_shell_if_top.hpp"

using namespace hls;
using namespace std;


/*******************************************************************************
 * @brief Top of TCP Shell Interface (TSIF)
 *
 * @param[in]  piSHL_Mmio_En Enable signal from [SHELL/MMIO].
 * @param[in]  siTAF_Data    TCP data stream from TcpAppFlash (TAF).
 * @param[in]  siTAF_SessId  TCP session Id  from [TAF].
 * @param[out] soTAF_Data    TCP data stream to   [TAF].
 * @param[out] soTAF_SessId  TCP session Id  to   [TAF].
 * @param[in]  siSHL_Notif   TCP data notification from [SHELL].
 * @param[out] soSHL_DReq    TCP data request to [SHELL].
 * @param[in]  siSHL_Data    TCP data stream from [SHELL].
 * @param[in]  siSHL_Meta    TCP metadata from [SHELL].
 * @param[out] soSHL_LsnReq  TCP listen port request to [SHELL].
 * @param[in]  siSHL_LsnRep  TCP listen port acknowledge from [SHELL].
 * @param[out] soSHL_Data    TCP data stream to [SHELL].
 * @param[out] soSHL_SndReq  TCP send request to [SHELL].
 * @param[in]  siSHL_SndRep  TCP send reply from [SHELL].
 * @param[out] soSHL_OpnReq  TCP open connection request to [SHELL].
 * @param[in]  siSHL_OpnRep  TCP open connection reply from [SHELL].
 * @param[out] soSHL_ClsReq  TCP close connection request to [SHELL].
 *******************************************************************************/
#if HLS_VERSION == 2016
    void tcp_shell_if_top(
        //------------------------------------------------------
        //-- SHELL / Mmio Interface
        //------------------------------------------------------
        CmdBit                *piSHL_Mmio_En,
        //------------------------------------------------------
        //-- TAF / TxP Data Interface
        //------------------------------------------------------
        stream<TcpAppData>    &siTAF_Data,
        stream<TcpSessId>     &siTAF_SessId,
        stream<TcpDatLen>     &siTAF_DatLen,
        //------------------------------------------------------
        //-- TAF / RxP Data Interface
        //------------------------------------------------------
        stream<TcpAppData>    &soTAF_Data,
        stream<TcpSessId>     &soTAF_SessId,
        stream<TcpDatLen>     &soTAF_DatLen,
        //------------------------------------------------------
        //-- SHELL / Rx Data Interfaces
        //------------------------------------------------------
        stream<TcpAppNotif>   &siSHL_Notif,
        stream<TcpAppRdReq>   &soSHL_DReq,
        stream<TcpAppData>    &siSHL_Data,
        stream<TcpAppMeta>    &siSHL_Meta,
        //------------------------------------------------------
        //-- SHELL / Listen Interfaces
        //------------------------------------------------------
        stream<TcpAppLsnReq>  &soSHL_LsnReq,
        stream<TcpAppLsnRep>  &siSHL_LsnRep,
        //------------------------------------------------------
        //-- SHELL / Tx Data Interfaces
        //------------------------------------------------------
        stream<TcpAppData>    &soSHL_Data,
        stream<TcpAppSndReq>  &soSHL_SndReq,
        stream<TcpAppSndRep>  &siSHL_SndRep,
        //------------------------------------------------------
        //-- SHELL / Tx Open Interfaces
        //------------------------------------------------------
        stream<TcpAppOpnReq>  &soSHL_OpnReq,
        stream<TcpAppOpnRep>  &siSHL_OpnRep,
        //------------------------------------------------------
        //-- SHELL / Close Interfaces
        //------------------------------------------------------
        stream<TcpAppClsReq>  &soSHL_ClsReq)
{

    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/
    #pragma HLS INTERFACE ap_stable register port=piSHL_Mmio_En    name=piSHL_Mmio_En

    #pragma HLS resource core=AXI4Stream variable=siTAF_Data   metadata="-bus_bundle siTAF_Data"
    #pragma HLS resource core=AXI4Stream variable=siTAF_SessId metadata="-bus_bundle siTAF_SessId"
    #pragma HLS resource core=AXI4Stream variable=siTAF_DatLen metadata="-bus_bundle siTAF_DatLen"

    #pragma HLS resource core=AXI4Stream variable=soTAF_Data   metadata="-bus_bundle soTAF_Data"
    #pragma HLS resource core=AXI4Stream variable=soTAF_SessId metadata="-bus_bundle soTAF_SessId"
    #pragma HLS resource core=AXI4Stream variable=soTAF_DatLen metadata="-bus_bundle soTAF_DatLen"

    #pragma HLS resource core=AXI4Stream variable=siSHL_Notif  metadata="-bus_bundle siSHL_Notif"
    #pragma HLS DATA_PACK                variable=siSHL_Notif
    #pragma HLS resource core=AXI4Stream variable=soSHL_DReq   metadata="-bus_bundle soSHL_DReq"
    #pragma HLS DATA_PACK                variable=soSHL_DReq
    #pragma HLS resource core=AXI4Stream variable=siSHL_Data   metadata="-bus_bundle siSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=siSHL_Meta   metadata="-bus_bundle siSHL_Meta"

    #pragma HLS resource core=AXI4Stream variable=soSHL_LsnReq metadata="-bus_bundle soSHL_LsnReq"
    #pragma HLS resource core=AXI4Stream variable=siSHL_LsnRep metadata="-bus_bundle siSHL_LsnRep"

    #pragma HLS resource core=AXI4Stream variable=soSHL_Data   metadata="-bus_bundle soSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=soSHL_SndReq metadata="-bus_bundle soSHL_SndReq"
    #pragma HLS DATA_PACK                variable=soSHL_SndReq
    #pragma HLS resource core=AXI4Stream variable=siSHL_SndRep metadata="-bus_bundle siSHL_SndRep"
    #pragma HLS DATA_PACK                variable=siSHL_SndRep

    #pragma HLS resource core=AXI4Stream variable=soSHL_OpnReq metadata="-bus_bundle soSHL_OpnReq"
    #pragma HLS DATA_PACK                variable=soSHL_OpnReq
    #pragma HLS resource core=AXI4Stream variable=siSHL_OpnRep metadata="-bus_bundle siSHL_OpnRep"
    #pragma HLS DATA_PACK                variable=siSHL_OpnRep

    #pragma HLS resource core=AXI4Stream variable=soSHL_ClsReq metadata="-bus_bundle soSHL_ClsReq"

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW

    //-- INSTANTIATE TOPLEVEL --------------------------------------------------
    tcp_shell_if(
        //-- SHELL / Mmio Interface
        piSHL_Mmio_En,
        //-- TAF / Rx & Tx Data Interfaces
        siTAF_Data,
        siTAF_SessId,
        siTAF_DatLen,
        soTAF_Data,
        soTAF_SessId,
        soTAF_DatLen,
        //-- TOE / Rx Data Interfaces
        siSHL_Notif,
        soSHL_DReq,
        siSHL_Data,
        siSHL_Meta,
        //-- TOE / Listen Interfaces
        soSHL_LsnReq,
        siSHL_LsnRep,
        //-- TOE / Tx Data Interfaces
        soSHL_Data,
        soSHL_SndReq,
        siSHL_SndRep,
        //-- TOE / Tx Open Interfaces
        soSHL_OpnReq,
        siSHL_OpnRep,
        //-- TOE / Close Interfaces
        soSHL_ClsReq);

}
#else
    void tcp_shell_if_top(
        //------------------------------------------------------
        //-- SHELL / Mmio Interface
        //------------------------------------------------------
        CmdBit                *piSHL_Mmio_En,
        //------------------------------------------------------
        //-- TAF / TxP Data Interface
        //------------------------------------------------------
        stream<TcpAppData>    &siTAF_Data,
        stream<TcpSessId>     &siTAF_SessId,
        stream<TcpDatLen>     &siTAF_DatLen,
        //------------------------------------------------------
        //-- TAF / RxP Data Interface
        //------------------------------------------------------
        stream<TcpAppData>    &soTAF_Data,
        stream<TcpSessId>     &soTAF_SessId,
        stream<TcpDatLen>     &soTAF_DatLen,
        //------------------------------------------------------
        //-- SHELL / Rx Data Interfaces
        //------------------------------------------------------
        stream<TcpAppNotif>   &siSHL_Notif,
        stream<TcpAppRdReq>   &soSHL_DReq,
        stream<TcpAppData>    &siSHL_Data,
        stream<TcpAppMeta>    &siSHL_Meta,
        //------------------------------------------------------
        //-- SHELL / Listen Interfaces
        //------------------------------------------------------
        stream<TcpAppLsnReq>  &soSHL_LsnReq,
        stream<TcpAppLsnRep>  &siSHL_LsnRep,
        //------------------------------------------------------
        //-- SHELL / Tx Data Interfaces
        //------------------------------------------------------
        stream<TcpAppData>    &soSHL_Data,
        stream<TcpAppSndReq>  &soSHL_SndReq,
        stream<TcpAppSndRep>  &siSHL_SndRep,
        //------------------------------------------------------
        //-- SHELL / Tx Open Interfaces
        //------------------------------------------------------
        stream<TcpAppOpnReq>  &soSHL_OpnReq,
        stream<TcpAppOpnRep>  &siSHL_OpnRep,
        //------------------------------------------------------
        //-- SHELL / Close Interfaces
        //------------------------------------------------------
        stream<TcpAppClsReq>  &soSHL_ClsReq)
{
    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE ap_stable register    port=piSHL_Mmio_En  name=piSHL_Mmio_En

    #pragma HLS INTERFACE axis off              port=siTAF_Data     name=siTAF_Data
    #pragma HLS INTERFACE axis off              port=siTAF_SessId   name=siTAF_SessId
    #pragma HLS INTERFACE axis off              port=siTAF_DatLen   name=siTAF_DatLen

    #pragma HLS INTERFACE axis off              port=soTAF_Data     name=soTAF_Data
    #pragma HLS INTERFACE axis off              port=soTAF_SessId   name=soTAF_SessId
    #pragma HLS INTERFACE axis off              port=soTAF_DatLen   name=soTAF_DatLen

    #pragma HLS INTERFACE axis off              port=siSHL_Notif    name=siSHL_Notif
    #pragma HLS DATA_PACK                   variable=siSHL_Notif
    #pragma HLS INTERFACE axis off              port=soSHL_DReq     name=soSHL_DReq
    #pragma HLS DATA_PACK                   variable=soSHL_DReq
    #pragma HLS INTERFACE axis off              port=siSHL_Data     name=siSHL_Data
    #pragma HLS INTERFACE axis off              port=siSHL_Meta     name=siSHL_Meta

    #pragma HLS INTERFACE axis off              port=soSHL_LsnReq   name=soSHL_LsnReq
    #pragma HLS INTERFACE axis off              port=siSHL_LsnRep   name=siSHL_LsnRep

    #pragma HLS INTERFACE axis off              port=soSHL_Data     name=soSHL_Data
    #pragma HLS INTERFACE axis off              port=soSHL_SndReq   name=soSHL_SndReq
    #pragma HLS DATA_PACK                   variable=soSHL_SndReq
    #pragma HLS INTERFACE axis off              port=siSHL_SndRep   name=siSHL_SndRep
    #pragma HLS DATA_PACK                   variable=siSHL_SndRep

    #pragma HLS INTERFACE axis off               port=soSHL_OpnReq   name=soSHL_OpnReq
    #pragma HLS DATA_PACK                    variable=soSHL_OpnReq
    #pragma HLS INTERFACE axis off              port=siSHL_OpnRep   name=siSHL_OpnRep
    #pragma HLS DATA_PACK                   variable=siSHL_OpnRep

    #pragma HLS INTERFACE axis off              port=soSHL_ClsReq   name=soSHL_ClsReq

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
  #if HLS_VERSION == 2017
    #pragma HLS DATAFLOW
  #else
    #pragma HLS DATAFLOW disable_start_propagation
  #endif

    //-- INSTANTIATE TOPLEVEL --------------------------------------------------
    tcp_shell_if(
        //-- SHELL / Mmio Interface
        piSHL_Mmio_En,
        //-- TAF / Rx & Tx Data Interfaces
        siTAF_Data,
        siTAF_SessId,
        siTAF_DatLen,
        soTAF_Data,
        soTAF_SessId,
        soTAF_DatLen,
        //-- TOE / Rx Data Interfaces
        siSHL_Notif,
        soSHL_DReq,
        siSHL_Data,
        siSHL_Meta,
        //-- TOE / Listen Interfaces
        soSHL_LsnReq,
        siSHL_LsnRep,
        //-- TOE / Tx Data Interfaces
        soSHL_Data,
        soSHL_SndReq,
        siSHL_SndRep,
        //-- TOE / Tx Open Interfaces
        soSHL_OpnReq,
        siSHL_OpnRep,
        //-- TOE / Close Interfaces
        soSHL_ClsReq);

}

#endif  //  HLS_VERSION

/*! \} */
