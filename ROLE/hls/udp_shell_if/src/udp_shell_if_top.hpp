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

/******************************************************************************
 * @file       : udp_shell_if_top.hpp
 * @brief      : Top of UDP Shell Interface (USIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp / ROLE
 * Language    : Vivado HLS
 *
 * \ingroup ROLE
 * \addtogroup ROLE_USIF
 * \{
 *******************************************************************************/

#ifndef _USIF_TOP_H_
#define _USIF_TOP_H_

#include <hls_stream.h>

#include "udp_shell_if.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts_utils.hpp"


/*******************************************************************************
 *
 * ENTITY - TOP of UDP SHELL INTERFACE (USIF)
 *
 *******************************************************************************/
void udp_shell_if_top(

        //------------------------------------------------------
        //-- SHELL / Mmio Interface
        //------------------------------------------------------
        CmdBit                  *piSHL_Mmio_En,

        //------------------------------------------------------
        //-- SHELL / Control Port Interfaces
        //------------------------------------------------------
        stream<UdpAppLsnReq>    &soSHL_LsnReq,
        stream<UdpAppLsnRep>    &siSHL_LsnRep,
        stream<UdpAppClsReq>    &soSHL_ClsReq,
        stream<UdpAppClsRep>    &siSHL_ClsRep,

        //------------------------------------------------------
        //-- SHELL / Rx Data Interfaces
        //------------------------------------------------------
        stream<AxisRaw>         &siSHL_Data,
        stream<UdpAppMeta>      &siSHL_Meta,

        //------------------------------------------------------
        //-- SHELL / Tx Data Interfaces
        //------------------------------------------------------
        stream<AxisRaw>         &soSHL_Data,
        stream<UdpAppMeta>      &soSHL_Meta,
        stream<UdpAppDLen>      &soSHL_DLen,

        //------------------------------------------------------
        //-- UAF / Tx Data Interfaces
        //------------------------------------------------------
        stream<AxisRaw>         &siUAF_Data,
        stream<UdpAppMeta>      &siUAF_Meta,
        stream<UdpAppDLen>      &siUAF_DLen,

        //------------------------------------------------------
        //-- UAF / Rx Data Interfaces
        //------------------------------------------------------
        stream<AxisRaw>         &soUAF_Data,
        stream<UdpAppMeta>      &soUAF_Meta

);

#endif

/*! \} */
