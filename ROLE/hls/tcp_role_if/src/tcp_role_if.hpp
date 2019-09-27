/************************************************
Copyright (c) 2016-2019, IBM Research.

All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*****************************************************************************
 * @file       : tcp_role_if.hpp
 * @brief      : TCP Role Interface (TRIF).
 *
 * System:     : cloudFPGA
 * Component   : Interface with Network Transport Stack (NTS) of the SHELL.
 * Language    : Vivado HLS
 *
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the
 *                   TCP-Role interface.
 *
 *****************************************************************************/

#include "../../role.hpp"

#include <hls_stream.h>
#include "ap_int.h"

using namespace hls;


/*************************************************************************
 *
 * ENTITY - TCP ROLE INTERFACE (TRIF)
 *
 *************************************************************************/
void tcp_role_if(

        //------------------------------------------------------
        //-- SHELL / Mmio Interface
        //------------------------------------------------------
        CmdBit                  *piSHL_Mmio_En,

        //------------------------------------------------------
        //-- ROLE / Rx Data Interface
        //------------------------------------------------------
        stream<AppData>         &siROL_Data,
        stream<AppMetaAxis>     &siROL_SessId,

        //------------------------------------------------------
        //-- ROLE / Tx Data Interface
        //------------------------------------------------------
        stream<AppData>         &soROL_Data,
        stream<AppMetaAxis>     &soROL_SessId,

        //------------------------------------------------------
        //-- TOE / Rx Data Interfaces
        //------------------------------------------------------
        stream<AppNotif>        &siTOE_Notif,
        stream<AppRdReq>        &soTOE_DReq,
        stream<AppData>         &siTOE_Data,
        stream<AppMetaAxis>     &siTOE_SessId,

        //------------------------------------------------------
        //-- TOE / Listen Interfaces
        //------------------------------------------------------
        stream<AppLsnReqAxis>   &soTOE_LsnReq,
        stream<AppLsnAckAxis>   &siTOE_LsnAck,

        //------------------------------------------------------
        //-- TOE / Tx Data Interfaces
        //------------------------------------------------------
        stream<AppData>         &soTOE_Data,
        stream<AppMetaAxis>     &soTOE_SessId,
        stream<AppWrSts>        &siTOE_DSts,

        //------------------------------------------------------
        //-- TOE / Open Interfaces
        //------------------------------------------------------
        stream<AppOpnReq>       &soTOE_OpnReq,
        stream<AppOpnSts>       &siTOE_OpnRep,

        //------------------------------------------------------
        //-- TOE / Close Interfaces
        //------------------------------------------------------
        stream<AppClsReqAxis>   &soTOE_ClsReq,

        //------------------------------------------------------
        //-- ROLE / Session Connect Id Interface
        //------------------------------------------------------
        SessionId               *poROL_SConId
);

