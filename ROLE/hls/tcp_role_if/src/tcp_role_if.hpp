/*****************************************************************************
 * @file       : tcp_role_if.hpp
 * @brief      : TCP Role Interface (TRIF).
 *
 * System:     : cloudFPGA
 * Component   : Interface with Network Transport Stack (NTS) of the SHELL.
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
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


//OBSOLETE #define NR_SESSION_ENTRIES  4
//OBSOLETE #define TRIF_MAX_SESSIONS   NR_SESSION_ENTRIES


/********************************************
 * Specific Definitions for the TCP Role I/F
 ********************************************/


//OBSOLETE /********************************************
//OBSOLETE  * Session Id Table Entry
//OBSOLETE  ********************************************/
//OBSOLETE class SessionIdCamEntry {
//OBSOLETE   public:
//OBSOLETE    TcpSessId   sessId;
//OBSOLETE     ValBit      valid;
//OBSOLETE
//OBSOLETE     SessionIdCamEntry() {}
//OBSOLETE     SessionIdCamEntry(TcpSessId tcpSessId, ValBit val) :
//OBSOLETE         sessId(tcpSessId), valid(val){}
//OBSOLETE };


//OBSOLETE /********************************************
//OBSOLETE  * Session Id CAM
//OBSOLETE  ********************************************/
//OBSOLETE class SessionIdCam {
//OBSOLETE   public:
//OBSOLETE     SessionIdCamEntry CAM[NR_SESSION_ENTRIES];
//OBSOLETE     SessionIdCam();
//OBSOLETE     bool   write(SessionIdCamEntry wrEntry);
//OBSOLETE     bool   search(TcpSessId        sessId);
//OBSOLETE };


/*************************************************************************
 *
 * ENTITY - TCP ROLE INTERFACE (TRIF)
 *
 *************************************************************************/
void tcp_role_if(

        //------------------------------------------------------
        //-- ROLE / Rx Data Interface
        //------------------------------------------------------
        stream<AppData>     &siROL_Data,
        stream<AppMeta>     &siROL_SessId,

        //------------------------------------------------------
        //-- ROLE / Tx Data Interface
        //------------------------------------------------------
        stream<AppData>     &soROL_Data,
        stream<AppMeta>     &soROL_SessId,

        //------------------------------------------------------
        //-- TOE / Rx Data Interfaces
        //------------------------------------------------------
        stream<AppNotif>    &siTOE_Notif,
        stream<AppRdReq>    &soTOE_DReq,
        stream<AppData>     &siTOE_Data,
        stream<AppMeta>     &siTOE_SessId,

        //------------------------------------------------------
        //-- TOE / Listen Interfaces
        //------------------------------------------------------
        stream<AppLsnReq>   &soTOE_LsnReq,
        stream<AppLsnAck>   &siTOE_LsnAck,

        //------------------------------------------------------
        //-- TOE / Tx Data Interfaces
        //------------------------------------------------------
        stream<AppData>     &soTOE_Data,
        stream<AppMeta>     &soTOE_SessId,
        stream<AppWrSts>    &siTOE_DSts,

        //------------------------------------------------------
        //-- TOE / Open Interfaces
        //------------------------------------------------------
        stream<AppOpnReq>   &soTOE_OpnReq,
        stream<AppOpnSts>   &siTOE_OpnRep,

        //------------------------------------------------------
        //-- TOE / Close Interfaces
        //------------------------------------------------------
        stream<AppClsReq>   &soTOE_ClsReq

);

