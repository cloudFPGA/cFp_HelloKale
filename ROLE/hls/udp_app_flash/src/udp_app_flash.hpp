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

/*****************************************************************************
 * @file       : udp_app_flash.cpp
 * @brief      : UDP Application Flash (UAF)
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the UDP
 *                application embedded in the role of the cFp_BringUp.
 *
 * \ingroup ROL_UAF
 * \addtogroup ROL_UAF
 * \{
 *****************************************************************************/

#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts_utils.hpp"


/********************************************
 * SHELL/MMIO/EchoCtrl - Config Register
 ********************************************/
enum EchoCtrl {
    ECHO_PATH_THRU  = 0,
    ECHO_STORE_FWD  = 1,
    ECHO_OFF        = 2
};

#define MTU    1500    // Maximum Transmission Unit in bytes [TODO:Move to a common place]


/*******************************************************************************
 *
 * ENTITY - UDP APPLICATION FLASH (UAF)
 *
 *******************************************************************************/
void udp_app_flash (

        //------------------------------------------------------
        //-- SHELL / Mmio / Configuration Interfaces
        //------------------------------------------------------
        ap_uint<2>           piSHL_Mmio_EchoCtrl,
        //[TODO] ap_uint<1>  piSHL_Mmio_PostPktEn,
        //[TODO] ap_uint<1>  piSHL_Mmio_CaptPktEn,

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
        stream<UdpAppDLen>  &soUSIF_DLen

);

