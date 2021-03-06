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
 * @file       : udp_app_flash_top.hpp
 * @brief      : Top of UDP Application Flash (UAF)
 *
 * System:     : cloudFPGA
 * Component   : cFp_HelloKale / ROLE
 * Language    : Vivado HLS
 *
 *------------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the UDP
 *                application embedded in the role of the cFp_HelloKale.
 *
 * \ingroup udp_app_flash
 * \addtogroup udp_app_flash
 * \{
 *******************************************************************************/

#ifndef _UAF_TOP_H_
#define _UAF_TOP_H_

#include "./udp_app_flash.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts_utils.hpp"


/*******************************************************************************
 *
 * ENTITY - UDP APPLICATION FLASH TOP (UAF_TOP)
 *
 *******************************************************************************/
void udp_app_flash_top (
        //------------------------------------------------------
        //-- SHELL / Mmio Interfaces
        //------------------------------------------------------
        CmdBit              *piSHL_Mmio_Enabe,
        //[NOT_USED] ap_uint<2>  piSHL_Mmio_EchoCtrl,
        //[NOT_USED] CmdBit      piSHL_Mmio_PostPktEn,
        //[NOT_USED] CmdBit      piSHL_Mmio_CaptPktEn,
        //------------------------------------------------------
        //-- USIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &siUSIF_Data,
        stream<UdpAppMeta>  &siUSIF_Meta,
        stream<UdpAppDLen>  &siUSIF_DLen,
        //------------------------------------------------------
        //-- USIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>  &soUSIF_Data,
        stream<UdpAppMeta>  &soUSIF_Meta,
        stream<UdpAppDLen>  &soUSIF_DLen

);

#endif

/*! \} */
