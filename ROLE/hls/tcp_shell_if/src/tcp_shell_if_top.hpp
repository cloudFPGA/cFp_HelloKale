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
 * @file       : tcp_shell_if_top.hpp
 * @brief      : Top of TCP Shell Interface (TSIF)
 *
 * System:     : cloudFPGA
 * Component   : cFp_HelloKale / ROLE
 * Language    : Vivado HLS
 *
 * \ingroup tcp_shell_if
 * \addtogroup tcp_shell_if
 * \{
 *******************************************************************************/

#ifndef _TSIF_TOP_H_
#define _TSIF_TOP_H_

#include <hls_stream.h>

#include "./tcp_shell_if.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts_utils.hpp"

/*******************************************************************************
 *
 * ENTITY - TOP of TCP SHELL INTERFACE (TSIF)
 *
 *******************************************************************************/
void tcp_shell_if_top(

        //------------------------------------------------------
        //-- SHELL / Mmio Interface
        //------------------------------------------------------
        CmdBit                *piSHL_Mmio_En,

        //------------------------------------------------------
        //-- TAF / Rx Data Interface
        //------------------------------------------------------
        stream<TcpAppData>    &siTAF_Data,
        stream<TcpSessId>     &siTAF_SessId,
        stream<TcpDatLen>     &siTAF_DatLen,

        //------------------------------------------------------
        //-- TAF / Tx Data Interface
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
        //-- SHELL / Open Interfaces
        //------------------------------------------------------
        stream<TcpAppOpnReq>  &soSHL_OpnReq,
        stream<TcpAppOpnRep>  &siSHL_OpnRep,

        //------------------------------------------------------
        //-- SHELL / Close Interfaces
        //------------------------------------------------------
        stream<TcpAppClsReq>  &soSHL_ClsReq,
        //-- Not Used         &siSHL_ClsSts,

        //------------------------------------------------------
        //-- DEBUG Probes
        //------------------------------------------------------
        stream<ap_uint<32> >  &soDBG_SinkCnt,
        stream<ap_uint<16> >  &soDBG_InpBufSpace
);

#endif

/*! \} */
