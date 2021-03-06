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

-- *****************************************************************************
-- *
-- * Title : Role for the bring-up of the FMKU2595 when equipped with a XCKU060.
-- *
-- * File    : Role.vhdl
-- *
-- * Tools   : Vivado v2016.4, v2017.4, v2019.2 (64-bit) 
-- *
-- * Description : In cloudFPGA, the user application is referred to as a 'role'    
-- *    and is integrated along with a 'shell' that abstracts the HW components
-- *    of the FPGA module. 
-- *    The current role implements a set of TCP, UDP and DDR4 tests for the  
-- *    bring-up of the FPGA module FMKU60. This role is typically paired with
-- *    the shell 'Kale' by the cloudFPGA project 'cFp_HelloKale'.
-- *
-- *****************************************************************************

--******************************************************************************
--**  CONTEXT CLAUSE  **  FMKU60 ROLE(BringUp)
--******************************************************************************
library IEEE;
use     IEEE.std_logic_1164.all;
use     IEEE.numeric_std.all;

library UNISIM; 
use     UNISIM.vcomponents.all;

library XIL_DEFAULTLIB;
use     XIL_DEFAULTLIB.all;  


--******************************************************************************
--**  ENTITY  **  ROLE_KALE
--******************************************************************************
entity Role_Kale is
  generic (
    gVivadoVersion : integer := 2019
  );
  port (
    --------------------------------------------------------
    -- SHELL / Clock, Reset and Enable Interface
    --------------------------------------------------------
    piSHL_156_25Clk                     : in    std_ulogic;
    piSHL_156_25Rst                     : in    std_ulogic;
    ------------------------------------------------------
    -- SHELL / Nts / Udp / Tx Data Interfaces (.i.e SHELL-->ROLE)
    ------------------------------------------------------
    ---- Axi4-Stream UDP Data ----------------
    siSHL_Nts_Udp_Data_tdata            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Nts_Udp_Data_tkeep            : in    std_ulogic_vector(  7 downto 0);
    siSHL_Nts_Udp_Data_tlast            : in    std_ulogic;
    siSHL_Nts_Udp_Data_tvalid           : in    std_ulogic;  
    siSHL_Nts_Udp_Data_tready           : out   std_ulogic;
    ---- Axi4-Stream UDP Metadata ------------
    siSHL_Nts_Udp_Meta_tdata            : in    std_ulogic_vector( 95 downto 0);
    siSHL_Nts_Udp_Meta_tvalid           : in    std_ulogic;
    siSHL_Nts_Udp_Meta_tready           : out   std_ulogic;
    ---- Axi4-Stream UDP Data Len ------------
    siSHL_Nts_Udp_DLen_tdata            : in    std_ulogic_vector( 15 downto 0);
    siSHL_Nts_Udp_DLen_tvalid           : in    std_ulogic;
    siSHL_Nts_Udp_DLen_tready           : out   std_ulogic;
    ------------------------------------------------------
    -- SHELL / Nts / Udp / Rx Data Interfaces (.i.e ROLE-->SHELL)
    ------------------------------------------------------
    ---- Axi4-Stream UDP Data ---------------
    soSHL_Nts_Udp_Data_tdata            : out   std_ulogic_vector( 63 downto 0);
    soSHL_Nts_Udp_Data_tkeep            : out   std_ulogic_vector(  7 downto 0);
    soSHL_Nts_Udp_Data_tlast            : out   std_ulogic;
    soSHL_Nts_Udp_Data_tvalid           : out   std_ulogic;
    soSHL_Nts_Udp_Data_tready           : in    std_ulogic;
    ---- Axi4-Stream UDP Meta ---------------
    soSHL_Nts_Udp_Meta_tdata            : out   std_ulogic_vector( 95 downto 0);
    soSHL_Nts_Udp_Meta_tvalid           : out   std_ulogic;
    soSHL_Nts_Udp_Meta_tready           : in    std_ulogic;
    ---- Axi4-Stream UDP Data Length ---------
    soSHL_Nts_Udp_DLen_tdata            : out   std_ulogic_vector( 15 downto 0);
    soSHL_Nts_Udp_DLen_tvalid           : out   std_ulogic;
    soSHL_Nts_Udp_DLen_tready           : in    std_ulogic;
    ------------------------------------------------------
    -- SHELL / Nts/ Udp / Rx Ctrl Interfaces (.i.e ROLE<-->SHELL)
    ------------------------------------------------------
    ---- Axi4-Stream UDP Listen Request -----
    soSHL_Nts_Udp_LsnReq_tdata          : out   std_ulogic_vector( 15 downto 0);
    soSHL_Nts_Udp_LsnReq_tvalid         : out   std_ulogic;           
    soSHL_Nts_Udp_LsnReq_tready         : in    std_ulogic;           
    ---- Axi4-Stream UDP Listen Reply --------
    siSHL_Nts_Udp_LsnRep_tdata          : in    std_ulogic_vector(  7 downto 0);
    siSHL_Nts_Udp_LsnRep_tvalid         : in    std_ulogic;   
    siSHL_Nts_Udp_LsnRep_tready         : out   std_ulogic;
    ---- Axi4-Stream UDP Close Request ------
    soSHL_Nts_Udp_ClsReq_tdata          : out   std_ulogic_vector( 15 downto 0); 
    soSHL_Nts_Udp_ClsReq_tvalid         : out   std_ulogic;   
    soSHL_Nts_Udp_ClsReq_tready         : in    std_ulogic;
    ---- Axi4-Stream UDP Close Reply --------
    siSHL_Nts_Udp_ClsRep_tdata          : in    std_ulogic_vector(  7 downto 0);
    siSHL_Nts_Udp_ClsRep_tvalid         : in    std_ulogic;   
    siSHL_Nts_Udp_ClsRep_tready         : out   std_ulogic;
    ------------------------------------------------------
    -- SHELL / Nts / Tcp / Tx Data Interfaces (.i.e ROLE-->SHELL)
    ------------------------------------------------------
    ---- Axi4-Stream TCP Data ---------------
    soSHL_Nts_Tcp_Data_tdata            : out   std_ulogic_vector( 63 downto 0);
    soSHL_Nts_Tcp_Data_tkeep            : out   std_ulogic_vector(  7 downto 0);
    soSHL_Nts_Tcp_Data_tlast            : out   std_ulogic;
    soSHL_Nts_Tcp_Data_tvalid           : out   std_ulogic;
    soSHL_Nts_Tcp_Data_tready           : in    std_ulogic;
    ---- Axi4-Stream TCP Send Request -------
    soSHL_Nts_Tcp_SndReq_tdata          : out   std_ulogic_vector( 31 downto 0);
    soSHL_Nts_Tcp_SndReq_tvalid         : out   std_ulogic;
    soSHL_Nts_Tcp_SndReq_tready         : in    std_ulogic;
    ---- Axi4-Stream TCP Send Reply ---------
    siSHL_Nts_Tcp_SndRep_tdata          : in    std_ulogic_vector( 55 downto 0);
    siSHL_Nts_Tcp_SndRep_tvalid         : in    std_ulogic;
    siSHL_Nts_Tcp_SndRep_tready         : out   std_ulogic;
    --------------------------------------------------------
    -- SHELL / Nts / Tcp / Rx Data Interfaces  (.i.e SHELL-->ROLE)
    --------------------------------------------------------
    ---- Axi4-Stream TCP Data -----------------
    siSHL_Nts_Tcp_Data_tdata            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Nts_Tcp_Data_tkeep            : in    std_ulogic_vector(  7 downto 0);
    siSHL_Nts_Tcp_Data_tlast            : in    std_ulogic;
    siSHL_Nts_Tcp_Data_tvalid           : in    std_ulogic;
    siSHL_Nts_Tcp_Data_tready           : out   std_ulogic;
    ----  Axi4-Stream TCP Metadata ------------
    siSHL_Nts_Tcp_Meta_tdata            : in    std_ulogic_vector( 15 downto 0);
    siSHL_Nts_Tcp_Meta_tvalid           : in    std_ulogic;
    siSHL_Nts_Tcp_Meta_tready           : out   std_ulogic;
    ----  Axi4-Stream TCP Data Notification ---
    siSHL_Nts_Tcp_Notif_tdata           : in    std_ulogic_vector(7+96 downto 0);
    siSHL_Nts_Tcp_Notif_tvalid          : in    std_ulogic;
    siSHL_Nts_Tcp_Notif_tready          : out   std_ulogic;
    ----  Axi4-Stream TCP Data Request --------
    soSHL_Nts_Tcp_DReq_tdata            : out   std_ulogic_vector( 31 downto 0); 
    soSHL_Nts_Tcp_DReq_tvalid           : out   std_ulogic;       
    soSHL_Nts_Tcp_DReq_tready           : in    std_ulogic;
    ------------------------------------------------------
    -- SHELL / Nts / Tcp / TxP Ctlr Interfaces (.i.e ROLE<-->SHELL)
    ------------------------------------------------------
    ---- Axi4-Stream TCP Open Session Request
    soSHL_Nts_Tcp_OpnReq_tdata          : out   std_ulogic_vector( 47 downto 0);  
    soSHL_Nts_Tcp_OpnReq_tvalid         : out   std_ulogic;
    soSHL_Nts_Tcp_OpnReq_tready         : in    std_ulogic;
    ---- Axi4-Stream TCP Open Session Reply
    siSHL_Nts_Tcp_OpnRep_tdata          : in    std_ulogic_vector( 23 downto 0); 
    siSHL_Nts_Tcp_OpnRep_tvalid         : in    std_ulogic;
    siSHL_Nts_Tcp_OpnRep_tready         : out   std_ulogic;
    ---- Axi4-Stream TCP Close Request ------
    soSHL_Nts_Tcp_ClsReq_tdata          : out   std_ulogic_vector( 15 downto 0);  
    soSHL_Nts_Tcp_ClsReq_tvalid         : out   std_ulogic;
    soSHL_Nts_Tcp_ClsReq_tready         : in    std_ulogic;
    ------------------------------------------------------
    -- SHELL / Nts / Tcp / Rx Ctlr Interfaces (.i.e SHELL-->ROLE)
    ------------------------------------------------------
    ---- Axi4-Stream TCP Listen Request ----
    soSHL_Nts_Tcp_LsnReq_tdata          : out   std_ulogic_vector( 15 downto 0);  
    soSHL_Nts_Tcp_LsnReq_tvalid         : out   std_ulogic;
    soSHL_Nts_Tcp_LsnReq_tready         : in    std_ulogic;
    ----  Axi4-Stream TCP Listen Rep --------
    siSHL_Nts_Tcp_LsnRep_tdata          : in    std_ulogic_vector(  7 downto 0); 
    siSHL_Nts_Tcp_LsnRep_tvalid         : in    std_ulogic;
    siSHL_Nts_Tcp_LsnRep_tready         : out   std_ulogic;
    --------------------------------------------------------
    -- SHELL / Mem / Mp0 Interface
    --------------------------------------------------------
    ---- Memory Port #0 / S2MM-AXIS ----------------   
    ------ Stream Read Command ---------
    soSHL_Mem_Mp0_RdCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp0_RdCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp0_RdCmd_tready          : in    std_ulogic;
    ------ Stream Read Status ----------
    siSHL_Mem_Mp0_RdSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp0_RdSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp0_RdSts_tready          : out   std_ulogic;
    ------ Stream Read Data ------------
    siSHL_Mem_Mp0_Read_tdata            : in    std_ulogic_vector(511 downto 0);
    siSHL_Mem_Mp0_Read_tkeep            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Mem_Mp0_Read_tlast            : in    std_ulogic;
    siSHL_Mem_Mp0_Read_tvalid           : in    std_ulogic;
    siSHL_Mem_Mp0_Read_tready           : out   std_ulogic;
    ------ Stream Write Command --------
    soSHL_Mem_Mp0_WrCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp0_WrCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp0_WrCmd_tready          : in    std_ulogic;
    ------ Stream Write Status ---------
    siSHL_Mem_Mp0_WrSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp0_WrSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp0_WrSts_tready          : out   std_ulogic;
    ------ Stream Write Data -----------
    soSHL_Mem_Mp0_Write_tdata           : out   std_ulogic_vector(511 downto 0);
    soSHL_Mem_Mp0_Write_tkeep           : out   std_ulogic_vector( 63 downto 0);
    soSHL_Mem_Mp0_Write_tlast           : out   std_ulogic;
    soSHL_Mem_Mp0_Write_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp0_Write_tready          : in    std_ulogic; 
    --------------------------------------------------------
    -- SHELL / Mem / Mp1 Interface
    --------------------------------------------------------
    ---- Write Address Channel ---------
    moSHL_Mem_Mp1_AWID                  : out   std_ulogic_vector(  7 downto 0);
    moSHL_Mem_Mp1_AWADDR                : out   std_ulogic_vector( 32 downto 0);
    moSHL_Mem_Mp1_AWLEN                 : out   std_ulogic_vector(  7 downto 0);
    moSHL_Mem_Mp1_AWSIZE                : out   std_ulogic_vector(  2 downto 0);
    moSHL_Mem_Mp1_AWBURST               : out   std_ulogic_vector(  1 downto 0);
    moSHL_Mem_Mp1_AWVALID               : out   std_ulogic;
    moSHL_Mem_Mp1_AWREADY               : in    std_ulogic;
    ---- Write Data Channel ------------
    moSHL_Mem_Mp1_WDATA                 : out   std_ulogic_vector(511 downto 0);
    moSHL_Mem_Mp1_WSTRB                 : out   std_ulogic_vector( 63 downto 0);
    moSHL_Mem_Mp1_WLAST                 : out   std_ulogic;
    moSHL_Mem_Mp1_WVALID                : out   std_ulogic;
    moSHL_Mem_Mp1_WREADY                : in    std_ulogic;
    ---- Write Response Channel --------
    moSHL_Mem_Mp1_BID                   : in    std_ulogic_vector(  7 downto 0);
    moSHL_Mem_Mp1_BRESP                 : in    std_ulogic_vector(  1 downto 0);
    moSHL_Mem_Mp1_BVALID                : in    std_ulogic;
    moSHL_Mem_Mp1_BREADY                : out   std_ulogic;
    ---- Read Address Channel ----------
    moSHL_Mem_Mp1_ARID                  : out   std_ulogic_vector(  7 downto 0);
    moSHL_Mem_Mp1_ARADDR                : out   std_ulogic_vector( 32 downto 0);
    moSHL_Mem_Mp1_ARLEN                 : out   std_ulogic_vector(  7 downto 0);
    moSHL_Mem_Mp1_ARSIZE                : out   std_ulogic_vector(  2 downto 0);
    moSHL_Mem_Mp1_ARBURST               : out   std_ulogic_vector(  1 downto 0);
    moSHL_Mem_Mp1_ARVALID               : out   std_ulogic;
    moSHL_Mem_Mp1_ARREADY               : in    std_ulogic;
    ---- Read Data Channel -------------
    moSHL_Mem_Mp1_RID                   : in    std_ulogic_vector(  7 downto 0);
    moSHL_Mem_Mp1_RDATA                 : in    std_ulogic_vector(511 downto 0);
    moSHL_Mem_Mp1_RRESP                 : in    std_ulogic_vector(  1 downto 0);
    moSHL_Mem_Mp1_RLAST                 : in    std_ulogic;
    moSHL_Mem_Mp1_RVALID                : in    std_ulogic;
    moSHL_Mem_Mp1_RREADY                : out   std_ulogic;
    
    --------------------------------------------------------
    -- SHELL / Mmio / AppFlash Interface
    --------------------------------------------------------
    ---- [PHY_RESET] -------------------
    piSHL_Mmio_Ly7Rst                   : in    std_ulogic;
    ---- [PHY_ENABLE] ------------------
    piSHL_Mmio_Ly7En                    : in    std_ulogic;
    ---- [DIAG_CTRL_1] -----------------
    piSHL_Mmio_Mc1_MemTestCtrl          : in    std_ulogic_vector(  1 downto 0);
    ---- [DIAG_STAT_1] -----------------
    poSHL_Mmio_Mc1_MemTestStat          : out   std_ulogic_vector(  1 downto 0);
    ---- [DIAG_CTRL_2] -----------------
    --[NOT_USED] piSHL_Mmio_UdpEchoCtrl   : in    std_ulogic_vector(  1 downto 0);
    --[NOT_USED] piSHL_Mmio_UdpPostDgmEn  : in    std_ulogic;
    --[NOT_USED] piSHL_Mmio_UdpCaptDgmEn  : in    std_ulogic;
    --[NOT_USED] piSHL_Mmio_TcpEchoCtrl   : in    std_ulogic_vector(  1 downto 0);
    --[NOT_USED] piSHL_Mmio_TcpPostSegEn  : in    std_ulogic;
    --[NOT_USED] piSHL_Mmio_TcpCaptSegEn  : in    std_ulogic;
    ---- [APP_RDROL] -------------------
    poSHL_Mmio_RdReg                    : out   std_ulogic_vector( 15 downto 0);
    --- [APP_WRROL] --------------------
    piSHL_Mmio_WrReg                    : in    std_ulogic_vector( 15 downto 0);
    --------------------------------------------------------
    -- TOP : Secondary Clock (Asynchronous)
    --------------------------------------------------------
    piTOP_250_00Clk                     : in    std_ulogic   -- Freerunning
  );
end Role_Kale;


-- *****************************************************************************
-- **  ARCHITECTURE  **  BRING_UP of ROLE_KALE
-- *****************************************************************************

architecture BringUp of Role_Kale is
 
  --============================================================================
  --  SIGNAL DECLARATIONS
  --============================================================================  
  -- Delayed reset signal and counter 
  signal s156_25Rst_delayed          : std_ulogic;
  signal sRstDelayCounter            : std_ulogic_vector(5 downto 0);

  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : TSIF --> TARS --> TAF
  --------------------------------------------------------
  signal ssTSIF_TARS_Data_tdata     : std_ulogic_vector( 63 downto 0);
  signal ssTSIF_TARS_Data_tkeep     : std_ulogic_vector(  7 downto 0);
  signal ssTSIF_TARS_Data_tlast     : std_ulogic;
  signal ssTSIF_TARS_Data_tvalid    : std_ulogic;
  signal ssTSIF_TARS_Data_tready    : std_ulogic;
  --
  signal ssTSIF_TARS_SessId_tdata   : std_ulogic_vector( 15 downto 0);
  signal ssTSIF_TARS_SessId_tvalid  : std_ulogic;
  signal ssTSIF_TARS_SessId_tready  : std_ulogic;
  --
  signal ssTSIF_TARS_DatLen_tdata   : std_ulogic_vector( 15 downto 0);
  signal ssTSIF_TARS_DatLen_tvalid  : std_ulogic;
  signal ssTSIF_TARS_DatLen_tready  : std_ulogic;  
  -- -------------------------------------------
  signal ssTARS_TAF_Data_tdata      : std_ulogic_vector( 63 downto 0);
  signal ssTARS_TAF_Data_tkeep      : std_ulogic_vector(  7 downto 0);
  signal ssTARS_TAF_Data_tlast      : std_ulogic;
  signal ssTARS_TAF_Data_tvalid     : std_ulogic;
  signal ssTARS_TAF_Data_tready     : std_ulogic;
  --
  signal ssTARS_TAF_SessId_tdata    : std_ulogic_vector( 15 downto 0);
  signal ssTARS_TAF_SessId_tvalid   : std_ulogic;
  signal ssTARS_TAF_SessId_tready   : std_ulogic;
  --
  signal ssTARS_TAF_DatLen_tdata    : std_ulogic_vector( 15 downto 0);
  signal ssTARS_TAF_DatLen_tvalid   : std_ulogic;
  signal ssTARS_TAF_DatLen_tready   : std_ulogic;  
  
  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : TSIF --> ARS --> DEBUG
  --------------------------------------------------------
  signal ssTSIF_ARS_SinkCnt_tdata   : std_ulogic_vector( 31 downto 0);
  signal ssTSIF_ARS_SinkCnt_tvalid  : std_ulogic;
  signal ssTSIF_ARS_SinkCnt_tready  : std_ulogic; 

  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : TAF --> TARS --> TSIF
  --------------------------------------------------------
  signal ssTAF_TARS_Data_tdata      : std_ulogic_vector( 63 downto 0);
  signal ssTAF_TARS_Data_tkeep      : std_ulogic_vector(  7 downto 0);
  signal ssTAF_TARS_Data_tlast      : std_ulogic;
  signal ssTAF_TARS_Data_tvalid     : std_ulogic;
  signal ssTAF_TARS_Data_tready     : std_ulogic;
  --
  signal ssTAF_TARS_SessId_tdata    : std_ulogic_vector( 15 downto 0);
  signal ssTAF_TARS_SessId_tvalid   : std_ulogic;
  signal ssTAF_TARS_SessId_tready   : std_ulogic;
  --
  signal ssTAF_TARS_DatLen_tdata    : std_ulogic_vector( 15 downto 0);
  signal ssTAF_TARS_DatLen_tvalid   : std_ulogic;
  signal ssTAF_TARS_DatLen_tready   : std_ulogic;
  -- -------------------------------------------
  signal ssTARS_TSIF_Data_tdata     : std_ulogic_vector( 63 downto 0);
  signal ssTARS_TSIF_Data_tkeep     : std_ulogic_vector(  7 downto 0);
  signal ssTARS_TSIF_Data_tlast     : std_ulogic;
  signal ssTARS_TSIF_Data_tvalid    : std_ulogic;
  signal ssTARS_TSIF_Data_tready    : std_ulogic;
  --
  signal ssTARS_TSIF_SessId_tdata   : std_ulogic_vector( 15 downto 0);
  signal ssTARS_TSIF_SessId_tvalid  : std_ulogic;
  signal ssTARS_TSIF_SessId_tready  : std_ulogic;
  --
  signal ssTARS_TSIF_DatLen_tdata   : std_ulogic_vector( 15 downto 0);
  signal ssTARS_TSIF_DatLen_tvalid  : std_ulogic;
  signal ssTARS_TSIF_DatLen_tready  : std_ulogic;
  
  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : USIF --> UARS --> UAF
  --------------------------------------------------------
  signal ssUSIF_UARS_Data_tdata     : std_ulogic_vector( 63 downto 0);
  signal ssUSIF_UARS_Data_tkeep     : std_ulogic_vector(  7 downto 0);
  signal ssUSIF_UARS_Data_tlast     : std_ulogic;
  signal ssUSIF_UARS_Data_tvalid    : std_ulogic;
  signal ssUSIF_UARS_Data_tready    : std_ulogic;
  --
  signal ssUSIF_UARS_Meta_tdata     : std_ulogic_vector( 95 downto 0);
  signal ssUSIF_UARS_Meta_tvalid    : std_ulogic;
  signal ssUSIF_UARS_Meta_tready    : std_ulogic;
  --
  signal ssUSIF_UARS_DLen_tdata     : std_ulogic_vector( 15 downto 0);
  signal ssUSIF_UARS_DLen_tvalid    : std_ulogic;
  signal ssUSIF_UARS_DLen_tready    : std_ulogic;
  -- -------------------------------------------
  signal ssUARS_UAF_Data_tdata      : std_ulogic_vector( 63 downto 0);
  signal ssUARS_UAF_Data_tkeep      : std_ulogic_vector(  7 downto 0);
  signal ssUARS_UAF_Data_tlast      : std_ulogic;
  signal ssUARS_UAF_Data_tvalid     : std_ulogic;
  signal ssUARS_UAF_Data_tready     : std_ulogic;
  --
  signal ssUARS_UAF_Meta_tdata      : std_ulogic_vector( 95 downto 0);
  signal ssUARS_UAF_Meta_tvalid     : std_ulogic;
  signal ssUARS_UAF_Meta_tready     : std_ulogic;
  --
  signal ssUARS_UAF_DLen_tdata      : std_ulogic_vector( 15 downto 0);
  signal ssUARS_UAF_DLen_tvalid     : std_ulogic;
  signal ssUARS_UAF_DLen_tready     : std_ulogic;
  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : UAF --> UARS --> USIF
  --------------------------------------------------------
  signal ssUAF_UARS_Data_tdata      : std_ulogic_vector( 63 downto 0);
  signal ssUAF_UARS_Data_tkeep      : std_ulogic_vector(  7 downto 0);
  signal ssUAF_UARS_Data_tlast      : std_ulogic;
  signal ssUAF_UARS_Data_tvalid     : std_ulogic;
  signal ssUAF_UARS_Data_tready     : std_ulogic;
  --
  signal ssUAF_UARS_Meta_tdata      : std_ulogic_vector( 95 downto 0);
  signal ssUAF_UARS_Meta_tvalid     : std_ulogic;
  signal ssUAF_UARS_Meta_tready     : std_ulogic;
  --
  signal ssUAF_UARS_DLen_tdata      : std_ulogic_vector( 15 downto 0);
  signal ssUAF_UARS_DLen_tvalid     : std_ulogic;
  signal ssUAF_UARS_DLen_tready     : std_ulogic;
  -- -------------------------------------------
  signal ssUARS_USIF_Data_tdata     : std_ulogic_vector( 63 downto 0);
  signal ssUARS_USIF_Data_tkeep     : std_ulogic_vector(  7 downto 0);
  signal ssUARS_USIF_Data_tlast     : std_ulogic;
  signal ssUARS_USIF_Data_tvalid    : std_ulogic;
  signal ssUARS_USIF_Data_tready    : std_ulogic;
  --
  signal ssUARS_USIF_Meta_tdata     : std_ulogic_vector( 95 downto 0);
  signal ssUARS_USIF_Meta_tvalid    : std_ulogic;
  signal ssUARS_USIF_Meta_tready    : std_ulogic;
  --
  signal ssUARS_USIF_DLen_tdata     : std_ulogic_vector( 15 downto 0);
  signal ssUARS_USIF_DLen_tvalid    : std_ulogic;
  signal ssUARS_USIF_DLen_tready    : std_ulogic;

  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : USIF <--> UAF (Axis-based)
  --------------------------------------------------------
  -- USIF-> UAF / UDP Rx Data Interfaces
  signal ssUSIF_UAF_Data_tdata      : std_logic_vector(63 downto 0);
  signal ssUSIF_UAF_Data_tkeep      : std_logic_vector( 7 downto 0);
  signal ssUSIF_UAF_Data_tlast      : std_logic;
  signal ssUSIF_UAF_Data_tvalid     : std_logic;
  signal ssUSIF_UAF_Data_tready     : std_logic;
  --
  signal ssUSIF_UAF_Meta_tdata      : std_logic_vector(95 downto 0);
  signal ssUSIF_UAF_Meta_tvalid     : std_logic;
  signal ssUSIF_UAF_Meta_tready     : std_logic;
    --
  signal ssUSIF_UAF_DLen_tdata      : std_logic_vector(15 downto 0);
  signal ssUSIF_UAF_DLen_tvalid     : std_logic;
  signal ssUSIF_UAF_DLen_tready     : std_logic;
  
  -- UAF->USIF / UDP Tx Data Interfaces
  signal ssUAF_USIF_Data_tdata      : std_logic_vector(63 downto 0);
  signal ssUAF_USIF_Data_tkeep      : std_logic_vector( 7 downto 0);
  signal ssUAF_USIF_Data_tlast      : std_logic;
  signal ssUAF_USIF_Data_tvalid     : std_logic;
  signal ssUAF_USIF_Data_tready     : std_logic;
  --
  signal ssUAF_USIF_Meta_tdata      : std_logic_vector(95 downto 0);
  signal ssUAF_USIF_Meta_tvalid     : std_logic;
  signal ssUAF_USIF_Meta_tready     : std_logic;
  --     
  signal ssUAF_USIF_DLen_tdata      : std_logic_vector(15 downto 0);
  signal ssUAF_USIF_DLen_tvalid     : std_logic;
  signal ssUAF_USIF_DLen_tready     : std_logic;
  
  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : USIF <--> FIFO <--> UAF
  --------------------------------------------------------
  -- USIF -> FIFO_Write / UDP Rx Data Interfaces
  signal ssUSIF_FIFO_Udp_Data_data         : std_logic_vector(72 downto 0);
  signal ssUSIF_FIFO_Udp_Data_write        : std_logic;
  signal ssUSIF_FIFO_Udp_Data_full         : std_logic;
  --
  signal ssUSIF_FIFO_Udp_Meta_data         : std_logic_vector(95 downto 0);
  signal ssUSIF_FIFO_Udp_Meta_write        : std_logic;
  signal ssUSIF_FIFO_Udp_Meta_full         : std_logic;
  -- FIFO_Read -> UAF / UDP Rx Data Interfaces
  signal ssFIFO_UAF_Udp_Data_data          : std_logic_vector(72 downto 0);
  signal ssFIFO_UAF_Udp_Data_read          : std_logic;
  signal ssFIFO_UAF_Udp_Data_empty         : std_logic; 
  --
  signal ssFIFO_UAF_Udp_Meta_data          : std_logic_vector(95 downto 0);
  signal ssFIFO_UAF_Udp_Meta_read          : std_logic;
  signal ssFIFO_UAF_Udp_Meta_empty         : std_logic;
  
  -- UAF -> FIFO_write / UDP Tx Data Interfaces
  signal ssUAF_FIFO_Udp_Data_data          : std_logic_vector(72 downto 0);
  signal ssUAF_FIFO_Udp_Data_write         : std_logic;
  signal ssUAF_FIFO_Udp_Data_full          : std_logic;
  --
  signal ssUAF_FIFO_Udp_Meta_data          : std_logic_vector(95 downto 0);
  signal ssUAF_FIFO_Udp_Meta_write         : std_logic;
  signal ssUAF_FIFO_Udp_Meta_full          : std_logic;
  --
  signal ssUAF_FIFO_Udp_DLen_data          : std_logic_vector(15 downto 0);
  signal ssUAF_FIFO_Udp_DLen_write         : std_logic;
  signal ssUAF_FIFO_Udp_DLen_full          : std_logic; 
  -- FIFO_Read -> USIF / UDP Tx Data Interfaces
  signal ssFIFO_USIF_Udp_Data_data         : std_logic_vector(72 downto 0);
  signal ssFIFO_USIF_Udp_Data_read         : std_logic;
  signal ssFIFO_USIF_Udp_Data_empty        : std_logic;
  --
  signal ssFIFO_USIF_Udp_Meta_data         : std_logic_vector(95 downto 0);
  signal ssFIFO_USIF_Udp_Meta_read         : std_logic;
  signal ssFIFO_USIF_Udp_Meta_empty        : std_logic;
  --     
  signal ssFIFO_USIF_Udp_DLen_data         : std_logic_vector(15 downto 0);
  signal ssFIFO_USIF_Udp_DLen_read         : std_logic;
  signal ssFIFO_USIF_Udp_DLen_empty        : std_logic;

  signal sSHL_Mem_Mp0_Write_tlast          : std_ulogic_vector(0 downto 0);
  
  --------------------------------------------------------
  -- DEBUG SIGNALS
  --------------------------------------------------------
  attribute mark_debug                     : string;
  --
  signal sTSIF_DBG_SinkCnt                 : std_logic_vector(31 downto 0);
  attribute mark_debug of sTSIF_DBG_SinkCnt: signal is "true"; -- Set to "true' if you need/want to trace these signals
  --
  signal sTSIF_DBG_InpBufSpace             : std_logic_vector(15 downto 0);
  attribute mark_debug of sTSIF_DBG_InpBufSpace : signal is "true"; -- Set to "true' if you need/want to trace these signals
  
  --============================================================================
  --  VARIABLE DECLARATIONS
  --============================================================================  
   
  --===========================================================================
  --== COMPONENT DECLARATIONS
  --===========================================================================
  component UdpApplicationFlash_Deprecated is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock, Reset
      ------------------------------------------------------
      aclk                    : in  std_logic;
      aresetn                 : in  std_logic; 
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------
      piSHL_Mmio_En_V        : in  std_logic_vector( 0 downto 0);
      --[NOT_USED] piSHL_Mmio_EchoCtrl_V  : in  std_logic_vector(  1 downto 0);
      --[NOT_USED] piSHL_MmioPostDgmEn_V  : in  std_logic;
      --[NOT_USED] piSHL_MmioCaptDgmEn_V  : in  std_logic;
      --------------------------------------------------------
      -- From USIF / UDP Rx Data Interfaces
      --------------------------------------------------------
      siUSIF_Data_tdata      : in  std_logic_vector( 63 downto 0);
      siUSIF_Data_tkeep      : in  std_logic_vector(  7 downto 0);
      siUSIF_Data_tlast      : in  std_logic;
      siUSIF_Data_tvalid     : in  std_logic;
      siUSIF_Data_tready     : out std_logic;
      --
      siUSIF_Meta_tdata      : in  std_logic_vector(95 downto 0);
      siUSIF_Meta_tvalid     : in  std_logic;
      siUSIF_Meta_tready     : out std_logic;
      --
      siUSIF_DLen_tdata      : in  std_logic_vector(15 downto 0);
      siUSIF_DLen_tvalid     : in  std_logic;
      siUSIF_DLen_tready     : out std_logic;
      --------------------------------------------------------
      -- To USIF / UDP Tx Data Interfaces
      --------------------------------------------------------
      soUSIF_Data_tdata      : out std_logic_vector( 63 downto 0);
      soUSIF_Data_tkeep      : out std_logic_vector(  7 downto 0);
      soUSIF_Data_tlast      : out std_logic;
      soUSIF_Data_tvalid     : out std_logic;
      soUSIF_Data_tready     : in  std_logic;
      --               
      soUSIF_Meta_tdata      : out std_logic_vector(95 downto 0);
      soUSIF_Meta_tvalid     : out std_logic;
      soUSIF_Meta_tready     : in  std_logic; 
      --
      soUSIF_DLen_tdata      : out std_logic_vector(15 downto 0);
      soUSIF_DLen_tvalid     : out std_logic;
      soUSIF_DLen_tready     : in  std_logic  
    );
  end component UdpApplicationFlash_Deprecated;  

  component UdpApplicationFlash_ApFifo is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                             : in  std_logic;
      ap_rst                             : in  std_logic;
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------
      piSHL_Mmio_En_V                    : in  std_logic_vector( 0 downto 0);
      --[NOT_USED] piSHL_Mmio_EchoCtrl_V  : in  std_logic_vector(  1 downto 0);
      --[NOT_USED] piSHL_Mmio_PostDgmEn_V : in  std_logic;
      --[NOT_USED] piSHL_Mmio_CaptDgmEn_V : in  std_logic;
      --------------------------------------------------------
      -- From USIF / UDP Rx Data Interfaces
      --------------------------------------------------------
      siUSIF_Data_V_dout                 : in  std_logic_vector( 72 downto 0);  -- 64+8+1
      siUSIF_Data_V_empty_n              : in  std_logic;
      siUSIF_Data_V_read                 : out std_logic;
      --
      siUSIF_Meta_V_dout                 : in  std_logic_vector(95 DOWNTO 0);
      siUSIF_Meta_V_empty_n              : in  std_logic;
      siUSIF_Meta_V_read                 : out std_logic;
      --
      siUSIF_DLen_V_V_dout               : in  std_logic_vector(15 DOWNTO 0);
      siUSIF_DLen_V_V_empty_n            : in  std_logic;
      siUSIF_DLen_V_V_read               : out std_logic;
      --------------------------------------------------------
      -- To USIF / UDP Tx Data Interfaces
      --------------------------------------------------------
      soUSIF_Data_V_din                  : out std_logic_vector( 72 downto 0);
      soUSIF_Data_V_write                : out std_logic;
      soUSIF_Data_V_full_n               : in  std_logic;
      --               
      soUSIF_Meta_V_din                  : out std_logic_vector(95 DOWNTO 0);
      soUSIF_Meta_V_write                : out std_logic;
      soUSIF_Meta_V_full_n               : in  std_logic;
      --
      soUSIF_DLen_V_V_din                : out std_logic_vector( 15 downto 0);
      soUSIF_DLen_V_V_write              : out std_logic;
      soUSIF_DLen_V_V_full_n             : in  std_logic
    );
  end component UdpApplicationFlash_ApFifo;
  
  component UdpApplicationFlash is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                 : in  std_logic;
      ap_rst_n               : in  std_logic;
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------
      piSHL_Mmio_En_V        : in  std_logic_vector( 0 downto 0);
      --[NOT_USED] piSHL_Mmio_EchoCtrl_V  : in  std_logic_vector(  1 downto 0);
      --[NOT_USED] piSHL_Mmio_PostDgmEn_V : in  std_logic;
      --[NOT_USED] piSHL_Mmio_CaptDgmEn_V : in  std_logic;
      --------------------------------------------------------
      -- From USIF / UDP Rx Data Interfaces
      --------------------------------------------------------
      siUSIF_Data_tdata      : in  std_logic_vector( 63 downto 0);
      siUSIF_Data_tkeep      : in  std_logic_vector(  7 downto 0);
      siUSIF_Data_tlast      : in  std_logic;
      siUSIF_Data_tvalid     : in  std_logic;
      siUSIF_Data_tready     : out std_logic;
      --
      siUSIF_Meta_V_tdata    : in  std_logic_vector(95 downto 0);
      siUSIF_Meta_V_tvalid   : in  std_logic;
      siUSIF_Meta_V_tready   : out std_logic;
            --
      siUSIF_DLen_V_V_tdata  : in  std_logic_vector(15 downto 0);
      siUSIF_DLen_V_V_tvalid : in  std_logic;
      siUSIF_DLen_V_V_tready : out std_logic;
      --------------------------------------------------------
      -- To USIF / UDP Tx Data Interfaces
      --------------------------------------------------------
      soUSIF_Data_tdata      : out std_logic_vector( 63 downto 0);
      soUSIF_Data_tkeep      : out std_logic_vector(  7 downto 0);
      soUSIF_Data_tlast      : out std_logic;
      soUSIF_Data_tvalid     : out std_logic;
      soUSIF_Data_tready     : in  std_logic;
      --               
      soUSIF_Meta_V_tdata    : out std_logic_vector(95 downto 0);
      soUSIF_Meta_V_tvalid   : out std_logic;
      soUSIF_Meta_V_tready   : in  std_logic; 
      --
      soUSIF_DLen_V_V_tdata  : out std_logic_vector(15 downto 0);
      soUSIF_DLen_V_V_tvalid : out std_logic;
      soUSIF_DLen_V_V_tready : in  std_logic 
    );
  end component UdpApplicationFlash;

  component UdpShellInterface_Deprecated is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      aclk                  : in  std_logic;
      aresetn               : in  std_logic;
      --------------------------------------------------------
      -- SHELL / Mmio Interface
      --------------------------------------------------------
      piSHL_Mmio_En_V       : in  std_logic_vector( 0 downto 0);
      --------------------------------------------------------
      -- SHELL / UDP Control Port Interfaces
      --------------------------------------------------------
      soSHL_LsnReq_tdata    : out std_logic_vector(15 downto 0);
      soSHL_LsnReq_tvalid   : out std_logic;
      soSHL_LsnReq_TREADY   : in  std_logic;
      --
      siSHL_LsnRep_tdata    : in  std_logic_vector( 7 downto 0);
      siSHL_LsnRep_tvalid   : in  std_logic;
      siSHL_LsnRep_tready   : out std_logic;
      --      
      soSHL_ClsReq_tdata    : out std_logic_vector(15 downto 0);
      soSHL_ClsReq_tvalid   : out std_logic;
      soSHL_ClsReq_TREADY   : in  std_logic;
      --
      siSHL_ClsRep_tdata    : in  std_logic_vector( 7 downto 0);
      siSHL_ClsRep_tvalid   : in  std_logic;
      siSHL_ClsRep_tready   : out std_logic;
      --------------------------------------------------------
      -- SHELL / Rx Data Interfaces
      --------------------------------------------------------
      siSHL_Data_tdata      : in  std_logic_vector(63 downto 0);
      siSHL_Data_tkeep      : in  std_logic_vector( 7 downto 0);
      siSHL_Data_tlast      : in  std_logic;
      siSHL_Data_tvalid     : in  std_logic;
      siSHL_Data_tready     : out std_logic;
      --
      siSHL_Meta_tdata      : in  std_logic_vector(95 downto 0);
      siSHL_Meta_tvalid     : in  std_logic;
      siSHL_Meta_tready     : out std_logic;
      --
      siSHL_DLen_tdata      : in  std_logic_vector(15 downto 0);
      siSHL_DLen_tvalid     : in  std_logic;
      siSHL_DLen_tready     : out std_logic;
      --------------------------------------------------------
      -- SHELL / UDP Tx Data Interfaces
      --------------------------------------------------------
      soSHL_Data_tdata      : out std_logic_vector(63 downto 0);
      soSHL_Data_tkeep      : out std_logic_vector( 7 downto 0);
      soSHL_Data_tlast      : out std_logic;
      soSHL_Data_tvalid     : out std_logic;
      soSHL_Data_tready     : in std_logic;
      --
      soSHL_Meta_tdata      : out std_logic_vector(95 downto 0);
      soSHL_Meta_tvalid     : out std_logic;
      soSHL_Meta_tready     : in  std_logic;
      --
      soSHL_DLen_tdata      : out std_logic_vector(15 downto 0);
      soSHL_DLen_tvalid     : out std_logic;
      soSHL_DLen_tready     : in  std_logic;
      --------------------------------------------------------
      -- UAF / UDP Tx Data Interfaces
      --------------------------------------------------------
      siUAF_Data_tdata      : in  std_logic_vector(63 downto 0);
      siUAF_Data_tkeep      : in  std_logic_vector( 7 downto 0);
      siUAF_Data_tlast      : in  std_logic;
      siUAF_Data_tvalid     : in  std_logic;
      siUAF_Data_tready     : out std_logic;
      --
      siUAF_Meta_tdata      : in  std_logic_vector(95 downto 0);
      siUAF_Meta_tvalid     : in  std_logic;
      siUAF_Meta_tready     : out std_logic;
      --     
      siUAF_DLen_tdata      : in  std_logic_vector(15 downto 0);
      siUAF_DLen_tvalid     : in  std_logic;
      siUAF_DLen_tready     : out std_logic;
      --------------------------------------------------------
      -- UAF / Rx Data Interfaces
      --------------------------------------------------------
      soUAF_Data_tdata      : out std_logic_vector(63 downto 0);
      soUAF_Data_tkeep      : out std_logic_vector( 7 downto 0);
      soUAF_Data_tlast      : out std_logic;
      soUAF_Data_tvalid     : out std_logic;
      soUAF_Data_tready     : in  std_logic;
      --
      soUAF_Meta_tdata      : out std_logic_vector(95 downto 0);
      soUAF_Meta_tvalid     : out std_logic;
      soUAF_Meta_tready     : in  std_logic;
      --
      soUAF_DLen_tdata      : out std_logic_vector(15 downto 0);
      soUAF_DLen_tvalid     : out std_logic;
      soUAF_DLen_tready     : in  std_logic
  );
  end component UdpShellInterface_Deprecated;  
  
  component UdpShellInterface_ApFifo is
    port (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        ap_clk                  : in  std_logic;
        ap_rst_n                : in  std_logic;
        --------------------------------------------------------
        -- SHELL / Mmio Interface
        --------------------------------------------------------
        piSHL_Mmio_En_V         : in  std_logic;
        --------------------------------------------------------
        -- SHELL / UDP Control Port Interfaces
        --------------------------------------------------------
        soSHL_LsnReq_V_V_tdata  : out std_logic_vector(15 downto 0);
        soSHL_LsnReq_V_V_tvalid : out std_logic;
        soSHL_LsnReq_V_V_tready : in  std_logic;
        --
        siSHL_LsnRep_V_tdata    : in  std_logic_vector( 7 downto 0);
        siSHL_LsnRep_V_tvalid   : in  std_logic;
        siSHL_LsnRep_V_tready   : out std_logic;
        --      
        soSHL_ClsReq_V_V_tdata  : out std_logic_vector(15 downto 0);
        soSHL_ClsReq_V_V_tvalid : out std_logic;
        soSHL_ClsReq_V_V_tready : in  std_logic;
        --
        siSHL_ClsRep_V_tdata    : in  std_logic_vector( 7 downto 0);
        siSHL_ClsRep_V_tvalid   : in  std_logic;
        siSHL_ClsRep_V_tready   : out std_logic;
        --------------------------------------------------------
        -- SHELL / Rx Data Interfaces
        --------------------------------------------------------
        siSHL_Data_tdata        : in  std_logic_vector(63 downto 0);
        siSHL_Data_tkeep        : in  std_logic_vector( 7 downto 0);
        siSHL_Data_tlast        : in  std_logic;
        siSHL_Data_tvalid       : in  std_logic;
        siSHL_Data_tready       : out std_logic;
        --
        siSHL_Meta_V_tdata      : in  std_logic_vector(95 downto 0);
        siSHL_Meta_V_tvalid     : in  std_logic;
        siSHL_Meta_V_tready     : out std_logic;
        --
        siSHL_DLen_V_V_tdata    : in  std_logic_vector(15 downto 0);
        siSHL_DLen_V_V_tvalid   : in  std_logic;
        siSHL_DLen_V_V_tready   : out std_logic;
        --------------------------------------------------------
        -- SHELL / UDP Tx Data Interfaces
        --------------------------------------------------------
        soSHL_Data_tdata        : out std_logic_vector(63 downto 0);
        soSHL_Data_tkeep        : out std_logic_vector( 7 downto 0);
        soSHL_Data_tlast        : out std_logic;
        soSHL_Data_tvalid       : out std_logic;
        soSHL_Data_tready       : in std_logic;
        --
        soSHL_Meta_V_tdata      : out std_logic_vector(95 downto 0);
        soSHL_Meta_V_tvalid     : out std_logic;
        soSHL_Meta_V_tready     : in  std_logic;
        --
        soSHL_DLen_V_V_tdata    : out std_logic_vector(15 downto 0);
        soSHL_DLen_V_V_tvalid   : out std_logic;
        soSHL_DLen_V_V_tready   : in  std_logic;
        --------------------------------------------------------
        -- UAF / UDP Tx Data Interfaces
        --------------------------------------------------------
        siUAF_Data_V_dout       : in  std_logic_vector(72 downto 0);
        siUAF_Data_V_empty_n    : in  std_logic;
        siUAF_Data_V_read       : out std_logic;
        --
        siUAF_Meta_V_dout       : in  std_logic_vector(95 downto 0);
        siUAF_Meta_V_empty_n    : in  std_logic;
        siUAF_Meta_V_read       : out std_logic;
        --    
        siUAF_DLen_V_V_dout     : in  std_logic_vector(15 downto 0);
        siUAF_DLen_V_V_empty_n  : in  std_logic;
        siUAF_DLen_V_V_read     : out std_logic;
        --------------------------------------------------------
        -- UAF / Rx Data Interfaces
        --------------------------------------------------------
        soUAF_Data_V_din        : out std_logic_vector(72 downto 0);
        soUAF_Data_V_write      : out std_logic;
        soUAF_Data_V_full_n     : in  std_logic;
        --
        soUAF_Meta_V_din        : out std_logic_vector(95 downto 0);
        soUAF_Meta_V_write      : out std_logic;
        soUAF_Meta_V_full_n     : in  std_logic;
        --
        soUAF_DLen_V_din        : out std_logic_vector(15 downto 0);
        soUAF_DLen_V_write      : out std_logic;
        soUAF_DLen_V_full_n     : in  std_logic 
    );
 
  end component UdpShellInterface_ApFifo;
 
  component UdpShellInterface is
    port (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        ap_clk                  : in  std_logic;
        ap_rst_n                : in  std_logic;
        --------------------------------------------------------
        -- SHELL / Mmio Interface
        --------------------------------------------------------
        piSHL_Mmio_En_V         : in  std_logic;
        --------------------------------------------------------
        -- SHELL / UDP Control Port Interfaces
        --------------------------------------------------------
        soSHL_LsnReq_V_V_tdata  : out std_logic_vector(15 downto 0);
        soSHL_LsnReq_V_V_tvalid : out std_logic;
        soSHL_LsnReq_V_V_tready : in  std_logic;
        --
        siSHL_LsnRep_V_tdata    : in  std_logic_vector( 7 downto 0);
        siSHL_LsnRep_V_tvalid   : in  std_logic;
        siSHL_LsnRep_V_tready   : out std_logic;
        --      
        soSHL_ClsReq_V_V_tdata  : out std_logic_vector(15 downto 0);
        soSHL_ClsReq_V_V_tvalid : out std_logic;
        soSHL_ClsReq_V_V_tready : in  std_logic;
        --
        siSHL_ClsRep_V_tdata    : in  std_logic_vector( 7 downto 0);
        siSHL_ClsRep_V_tvalid   : in  std_logic;
        siSHL_ClsRep_V_tready   : out std_logic;
        --------------------------------------------------------
        -- SHELL / Rx Data Interfaces
        --------------------------------------------------------
        siSHL_Data_tdata        : in  std_logic_vector(63 downto 0);
        siSHL_Data_tkeep        : in  std_logic_vector( 7 downto 0);
        siSHL_Data_tlast        : in  std_logic;
        siSHL_Data_tvalid       : in  std_logic;
        siSHL_Data_tready       : out std_logic;
        --
        siSHL_Meta_V_tdata      : in  std_logic_vector(95 downto 0);
        siSHL_Meta_V_tvalid     : in  std_logic;
        siSHL_Meta_V_tready     : out std_logic;
        --
        siSHL_DLen_V_V_tdata    : in  std_logic_vector(15 downto 0);
        siSHL_DLen_V_V_tvalid   : in  std_logic;
        siSHL_DLen_V_V_tready   : out std_logic;
        --------------------------------------------------------
        -- SHELL / UDP Tx Data Interfaces
        --------------------------------------------------------
        soSHL_Data_tdata        : out std_logic_vector(63 downto 0);
        soSHL_Data_tkeep        : out std_logic_vector( 7 downto 0);
        soSHL_Data_tlast        : out std_logic;
        soSHL_Data_tvalid       : out std_logic;
        soSHL_Data_tready       : in std_logic;
        --
        soSHL_Meta_V_tdata      : out std_logic_vector(95 downto 0);
        soSHL_Meta_V_tvalid     : out std_logic;
        soSHL_Meta_V_tready     : in  std_logic;
        --
        soSHL_DLen_V_V_tdata    : out std_logic_vector(15 downto 0);
        soSHL_DLen_V_V_tvalid   : out std_logic;
        soSHL_DLen_V_V_tready   : in  std_logic;
        --------------------------------------------------------
        -- UAF / UDP Tx Data Interfaces
        --------------------------------------------------------
        siUAF_Data_tdata      : in  std_logic_vector(63 downto 0);
        siUAF_Data_tkeep      : in  std_logic_vector( 7 downto 0);
        siUAF_Data_tlast      : in  std_logic;
        siUAF_Data_tvalid     : in  std_logic;
        siUAF_Data_tready     : out std_logic;
        --
        siUAF_Meta_V_tdata    : in  std_logic_vector(95 downto 0);
        siUAF_Meta_V_tvalid   : in  std_logic;
        siUAF_Meta_V_tready   : out std_logic;
        --     
        siUAF_DLen_V_V_tdata  : in  std_logic_vector(15 downto 0);
        siUAF_DLen_V_V_tvalid : in  std_logic;
        siUAF_DLen_V_V_tready : out std_logic;
        --------------------------------------------------------
        -- UAF / Rx Data Interfaces
        --------------------------------------------------------
        soUAF_Data_tdata      : out std_logic_vector(63 downto 0);
        soUAF_Data_tkeep      : out std_logic_vector( 7 downto 0);
        soUAF_Data_tlast      : out std_logic;
        soUAF_Data_tvalid     : out std_logic;
        soUAF_Data_tready     : in  std_logic;
        --
        soUAF_Meta_V_tdata    : out std_logic_vector(95 downto 0);
        soUAF_Meta_V_tvalid   : out std_logic;
        soUAF_Meta_V_tready   : in  std_logic;
        --
        soUAF_DLen_V_V_tdata  : out std_logic_vector(15 downto 0);
        soUAF_DLen_V_V_tvalid : out std_logic;
        soUAF_DLen_V_V_tready : in  std_logic    
    );
  end component UdpShellInterface;  
 
  component TcpApplicationFlash_Deprecated is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      aclk                   : in  std_logic;
      aresetn                : in  std_logic;    
      ------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      ------------------------------------------------------       
      --[NOT_USED] piSHL_MmioEchoCtrl_V  : in  std_logic_vector(  1 downto 0);
      --[NOT_USED] piSHL_MmioPostSegEn_V : in  std_logic;
      --[NOT_USED] piSHL_MmioCaptSegEn_V : in  std_logic;      
      --------------------------------------------------------
      -- From SHELL / Tcp Data Interfaces
      --------------------------------------------------------
      siTSIF_Data_tdata      : in  std_logic_vector( 63 downto 0);
      siTSIF_Data_tkeep      : in  std_logic_vector(  7 downto 0);
      siTSIF_Data_tlast      : in  std_logic;
      siTSIF_Data_tvalid     : in  std_logic;
      siTSIF_Data_tready     : out std_logic;
      --
      siTSIF_SessId_tdata    : in  std_logic_vector( 15 downto 0);
      siTSIF_SessId_tvalid   : in  std_logic;
      siTSIF_SessId_tready   : out std_logic;
      --
      siTSIF_DatLen_tdata    : in  std_logic_vector( 15 downto 0);
      siTSIF_DatLen_tvalid   : in  std_logic;
      siTSIF_DatLen_tready   : out std_logic;
      --------------------------------------------------------
      -- To SHELL / Tcp Data Interfaces
      --------------------------------------------------------
      soTSIF_Data_tdata      : out std_logic_vector( 63 downto 0);
      soTSIF_Data_tkeep      : out std_logic_vector(  7 downto 0);
      soTSIF_Data_tlast      : out std_logic;
      soTSIF_Data_tvalid     : out std_logic;
      soTSIF_Data_tready     : in  std_logic;
      --
      soTSIF_SessId_tdata    : out std_logic_vector( 15 downto 0);
      soTSIF_SessId_tvalid   : out std_logic;
      soTSIF_SessId_tready   : in  std_logic;
      --
      soTSIF_DatLen_tdata    : out std_logic_vector( 15 downto 0);
      soTSIF_DatLen_tvalid   : out std_logic;
      soTSIF_DatLen_tready   : in  std_logic
    );
  end component TcpApplicationFlash_Deprecated;
 
  component TcpApplicationFlash is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                   : in  std_logic;
      ap_rst_n                 : in  std_logic;
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------       
      --[NOT_USED] piSHL_MmioEchoCtrl_V  : in  std_logic_vector(  1 downto 0);
      --[NOT_USED] piSHL_MmioPostSegEn_V : in  std_logic;
      --[NOT_USED] piSHL_MmioCaptSegEn   : in  std_logic;
      --------------------------------------------------------
      -- From SHELL / Tcp Data Interfaces
      --------------------------------------------------------
      siTSIF_Data_tdata        : in  std_logic_vector( 63 downto 0);
      siTSIF_Data_tkeep        : in  std_logic_vector(  7 downto 0);
      siTSIF_Data_tlast        : in  std_logic;
      siTSIF_Data_tvalid       : in  std_logic;
      siTSIF_Data_tready       : out std_logic;
      --
      siTSIF_SessId_V_V_tdata  : in  std_logic_vector( 15 downto 0);
      siTSIF_SessId_V_V_tvalid : in  std_logic;
      siTSIF_SessId_V_V_tready : out std_logic;
      --
      siTSIF_DatLen_V_V_tdata  : in  std_logic_vector( 15 downto 0);
      siTSIF_DatLen_V_V_tvalid : in  std_logic;
      siTSIF_DatLen_V_V_tready : out std_logic;
      --------------------------------------------------------
      -- To SHELL / Tcp Data Interfaces
      --------------------------------------------------------
      soTSIF_Data_tdata        : out std_logic_vector( 63 downto 0);
      soTSIF_Data_tkeep        : out std_logic_vector(  7 downto 0);
      soTSIF_Data_tlast        : out std_logic;
      soTSIF_Data_tvalid       : out std_logic;
      soTSIF_Data_tready       : in  std_logic;
      --
      soTSIF_SessId_V_V_tdata  : out std_logic_vector( 15 downto 0);
      soTSIF_SessId_V_V_tvalid : out std_logic;
      soTSIF_SessId_V_V_tready : in  std_logic;
      --
      soTSIF_DatLen_V_V_tdata  : out std_logic_vector( 15 downto 0);
      soTSIF_DatLen_V_V_tvalid : out std_logic;
      soTSIF_DatLen_V_V_tready : in  std_logic
    );
  end component TcpApplicationFlash;

  component TcpShellInterface_Deprecated is
    port (
      ------------------------------------------------------
      -- SHELL / Clock and Reset
      ------------------------------------------------------
      aclk                  : in  std_ulogic;
      aresetn               : in  std_ulogic;
      --------------------------------------------------------
      -- SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piSHL_Mmio_En_V       : in  std_ulogic;
      ------------------------------------------------------
      -- TAF / TxP Data Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (APP-->SHELL) ----------
      ---- TCP Data Stream   
      siTAF_Data_tdata      : in  std_ulogic_vector( 63 downto 0);
      siTAF_Data_tkeep      : in  std_ulogic_vector(  7 downto 0);
      siTAF_Data_tlast      : in  std_ulogic;
      siTAF_Data_tvalid     : in  std_ulogic;
      siTAF_Data_tready     : out std_ulogic;
      ---- TCP Session-Id 
      siTAF_SessId_tdata    : in  std_ulogic_vector( 15 downto 0);
      siTAF_SessId_tvalid   : in  std_ulogic;
      siTAF_SessId_tready   : out std_ulogic;
      ---- TCP Data-Length 
      siTAF_DatLen_tdata    : in  std_ulogic_vector( 15 downto 0);
      siTAF_DatLen_tvalid   : in  std_ulogic;
      siTAF_DatLen_tready   : out std_ulogic; 
      ------------------------------------------------------               
      -- TAF /RxP Data Flow Interfaces                      
      ------------------------------------------------------               
      -- FPGA Transmit Path (SHELL-->APP) --------                      
      ---- TCP Data Stream 
      soTAF_Data_tdata      : out std_ulogic_vector( 63 downto 0);
      soTAF_Data_tkeep      : out std_ulogic_vector(  7 downto 0);
      soTAF_Data_tlast      : out std_ulogic;
      soTAF_Data_tvalid     : out std_ulogic;
      soTAF_Data_tready     : in  std_ulogic;
      ---- TCP Session-Id
      soTAF_SessId_tdata    : out std_ulogic_vector( 15 downto 0);
      soTAF_SessId_tvalid   : out std_ulogic;
      soTAF_SessId_tready   : in  std_ulogic;
      ---- TCP Data-Length
      soTAF_DatLen_tdata    : out std_ulogic_vector( 15 downto 0);
      soTAF_DatLen_tvalid   : out std_ulogic;
      soTAF_DatLen_tready   : in  std_ulogic;
      ------------------------------------------------------
      -- SHELL / RxP Data Flow Interfaces
      ------------------------------------------------------
      ---- TCP Data Notification Stream  
      siSHL_Notif_tdata     : in  std_ulogic_vector(7+96 downto 0); -- 8-bits boundary
      siSHL_Notif_tvalid    : in  std_ulogic;
      siSHL_Notif_tready    : out std_ulogic;
      ---- TCP Data Request Stream 
      soSHL_DReq_tdata      : out std_ulogic_vector( 31 downto 0);
      soSHL_DReq_tvalid     : out std_ulogic;
      soSHL_DReq_tready     : in  std_ulogic;
      ---- TCP Data Stream 
      siSHL_Data_tdata      : in  std_ulogic_vector( 63 downto 0);
      siSHL_Data_tkeep      : in  std_ulogic_vector(  7 downto 0);
      siSHL_Data_tlast      : in  std_ulogic;
      siSHL_Data_tvalid     : in  std_ulogic;
      siSHL_Data_tready     : out std_ulogic;
      ---- TCP Metadata Stream 
      siSHL_Meta_tdata      : in  std_ulogic_vector( 15 downto 0);
      siSHL_Meta_tvalid     : in  std_ulogic;
      siSHL_Meta_tready     : out std_ulogic;
      ------------------------------------------------------
      -- SHELL / RxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Receive Path (SHELL-->APP) -------
      ---- TCP Listen Request Stream 
      soSHL_LsnReq_tdata    : out std_ulogic_vector( 15 downto 0);
      soSHL_LsnReq_tvalid   : out std_ulogic;
      soSHL_LsnReq_tready   : in  std_ulogic;
      ---- TCP Listen Status Stream 
      siSHL_LsnRep_tdata    : in  std_ulogic_vector(  7 downto 0);
      siSHL_LsnRep_tvalid   : in  std_ulogic;
      siSHL_LsnRep_tready   : out std_ulogic;
      ------------------------------------------------------
      -- SHELL / TxP Data Flow Interfaces
      ------------------------------------------------------
      ---- TCP Data Stream 
      soSHL_Data_tdata      : out std_ulogic_vector( 63 downto 0);
      soSHL_Data_tkeep      : out std_ulogic_vector(  7 downto 0);
      soSHL_Data_tlast      : out std_ulogic;
      soSHL_Data_tvalid     : out std_ulogic;
      soSHL_Data_tready     : in  std_ulogic;
      ---- TCP Send Request Stream 
      soSHL_SndReq_tdata    : out std_ulogic_vector( 31 downto 0);
      soSHL_SndReq_tvalid   : out std_ulogic;
      soSHL_SndReq_tready   : in  std_ulogic;
      ---- TCP Send Reply Stream 
      siSHL_SndRep_tdata    : in  std_ulogic_vector( 55 downto 0);
      siSHL_SndRep_tvalid   : in  std_ulogic;
      siSHL_SndRep_tready   : out std_ulogic;
      ------------------------------------------------------
      -- SHELL / TxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (APP-->SHELL) ------
      ---- TCP Open Session Request Stream 
      soSHL_OpnReq_tdata    : out std_ulogic_vector( 47 downto 0);
      soSHL_OpnReq_tvalid   : out std_ulogic;
      soSHL_OpnReq_tready   : in  std_ulogic;
      ---- TCP Open Session Status Stream  
      siSHL_OpnRep_tdata    : in  std_ulogic_vector( 23 downto 0);
      siSHL_OpnRep_tvalid   : in  std_ulogic;
      siSHL_OpnRep_tready   : out std_ulogic;
      ---- TCP Close Request Stream
      soSHL_ClsReq_tdata    : out std_ulogic_vector( 15 downto 0);
      soSHL_ClsReq_tvalid   : out std_ulogic;
      soSHL_ClsReq_tready   : in  std_ulogic
    );
  end component TcpShellInterface_Deprecated;
 
  component TcpShellInterface is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                  : in  std_ulogic;
      ap_rst_n                : in  std_ulogic;
       --------------------------------------------------------
       -- From SHELL / Mmio Interfaces
       --------------------------------------------------------       
       piSHL_Mmio_En_V        : in  std_ulogic;
      ------------------------------------------------------
      -- TAF / TxP Data Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (APP-->SHELL) ---------
      ---- TCP Data Stream 
      siTAF_Data_tdata        : in  std_ulogic_vector( 63 downto 0);
      siTAF_Data_tkeep        : in  std_ulogic_vector(  7 downto 0);
      siTAF_Data_tlast        : in  std_ulogic;
      siTAF_Data_tvalid       : in  std_ulogic;
      siTAF_Data_tready       : out std_ulogic;
      ---- TCP Session-Id 
      siTAF_SessId_V_V_tdata  : in  std_ulogic_vector( 15 downto 0);
      siTAF_SessId_V_V_tvalid : in  std_ulogic;
      siTAF_SessId_V_V_tready : out std_ulogic;
      ---- TCP Data-Length 
      siTAF_DatLen_V_V_tdata  : in  std_ulogic_vector( 15 downto 0);
      siTAF_DatLen_V_V_tvalid : in  std_ulogic;
      siTAF_DatLen_V_V_tready : out std_ulogic; 
      ------------------------------------------------------               
      -- TAF / RxP Data Flow Interfaces                      
      ------------------------------------------------------               
      -- FPGA Transmit Path (SHELL-->APP) --------                      
      ---- TCP Data Stream 
      soTAF_Data_tdata        : out std_ulogic_vector( 63 downto 0);
      soTAF_Data_tkeep        : out std_ulogic_vector(  7 downto 0);
      soTAF_Data_tlast        : out std_ulogic;
      soTAF_Data_tvalid       : out std_ulogic;
      soTAF_Data_tready       : in  std_ulogic;
      ---- TCP Session-Id
      soTAF_SessId_V_V_tdata  : out std_ulogic_vector( 15 downto 0);
      soTAF_SessId_V_V_tvalid : out std_ulogic;
      soTAF_SessId_V_V_tready : in  std_ulogic;
      ---- TCP Data-Length
      soTAF_DatLen_V_V_tdata  : out std_ulogic_vector( 15 downto 0);
      soTAF_DatLen_V_V_tvalid : out std_ulogic;
      soTAF_DatLen_V_V_tready : in  std_ulogic;
      ------------------------------------------------------
      -- SHELL / RxP Data Flow Interfaces
      ------------------------------------------------------
      ---- TCP Data Notification Stream  
      siSHL_Notif_V_tdata     : in  std_ulogic_vector(103 downto 0);
      siSHL_Notif_V_tvalid    : in  std_ulogic;
      siSHL_Notif_V_tready    : out std_ulogic;
      ---- TCP Data Request Stream 
      soSHL_DReq_V_tdata      : out std_ulogic_vector( 31 downto 0);
      soSHL_DReq_V_tvalid     : out std_ulogic;
      soSHL_DReq_V_tready     : in  std_ulogic;
      ---- TCP Data Stream 
      siSHL_Data_tdata        : in  std_ulogic_vector( 63 downto 0);
      siSHL_Data_tkeep        : in  std_ulogic_vector(  7 downto 0);
      siSHL_Data_tlast        : in  std_ulogic;
      siSHL_Data_tvalid       : in  std_ulogic;
      siSHL_Data_tready       : out std_ulogic;
      ---- TCP Metadata Stream 
      siSHL_Meta_V_V_tdata    : in  std_ulogic_vector( 15 downto 0);
      siSHL_Meta_V_V_tvalid   : in  std_ulogic;
      siSHL_Meta_V_V_tready   : out std_ulogic;
      ------------------------------------------------------
      -- SHELL / RxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Receive Path (SHELL-->APP) -------
      ---- TCP Listen Request Stream 
      soSHL_LsnReq_V_V_tdata  : out std_ulogic_vector( 15 downto 0);
      soSHL_LsnReq_V_V_tvalid : out std_ulogic;
      soSHL_LsnReq_V_V_tready : in  std_ulogic;
      ---- TCP Listen Status Stream 
      siSHL_LsnRep_V_tdata    : in  std_ulogic_vector(  7 downto 0);
      siSHL_LsnRep_V_tvalid   : in  std_ulogic;
      siSHL_LsnRep_V_tready   : out std_ulogic;
      ------------------------------------------------------
      -- SHELL / TxP Data Flow Interfaces
      ------------------------------------------------------
      ---- TCP Data Stream 
      soSHL_Data_tdata        : out std_ulogic_vector( 63 downto 0);
      soSHL_Data_tkeep        : out std_ulogic_vector(  7 downto 0);
      soSHL_Data_tlast        : out std_ulogic;
      soSHL_Data_tvalid       : out std_ulogic;
      soSHL_Data_tready       : in  std_ulogic;
      ---- TCP Send Request Stream 
      soSHL_SndReq_V_tdata    : out std_ulogic_vector( 31 downto 0);
      soSHL_SndReq_V_tvalid   : out std_ulogic;
      soSHL_SndReq_V_tready   : in  std_ulogic;
      ---- TCP Send Reply Stream 
      siSHL_SndRep_V_tdata    : in  std_ulogic_vector( 55 downto 0);
      siSHL_SndRep_V_tvalid   : in  std_ulogic;
      siSHL_SndRep_V_tready   : out std_ulogic;
      ------------------------------------------------------
      -- SHELL / TxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (APP-->SHELL) ------
      ---- TCP Open Session Request Stream 
      soSHL_OpnReq_V_tdata    : out std_ulogic_vector( 47 downto 0);
      soSHL_OpnReq_V_tvalid   : out std_ulogic;
      soSHL_OpnReq_V_tready   : in  std_ulogic;
      ---- TCP Open Session Status Stream  
      siSHL_OpnRep_V_tdata    : in  std_ulogic_vector( 23 downto 0);
      siSHL_OpnRep_V_tvalid   : in  std_ulogic;
      siSHL_OpnRep_V_tready   : out std_ulogic;
      ---- TCP Close Request Stream 
      soSHL_ClsReq_V_V_tdata  : out std_ulogic_vector( 15 downto 0);
      soSHL_ClsReq_V_V_tvalid : out std_ulogic;
      soSHL_ClsReq_V_V_tready : in  std_ulogic;
      ------------------------------------------------------
      -- DEBUG Interfaces
      ------------------------------------------------------
      ---- Sink Counter Stream
      soDBG_SinkCnt_V_V_tdata : out std_ulogic_vector( 31 downto 0);
      soDBG_SinkCnt_V_V_tvalid: out std_ulogic;
      soDBG_SinkCnt_V_V_tready: in  std_ulogic;
      ---- Input Buffer Space
      soDBG_InpBufSpace_V_V_tdata : out std_ulogic_vector( 15 downto 0);
      soDBG_InpBufSpace_V_V_tvalid: out std_ulogic;
      soDBG_InpBufSpace_V_V_tready: in  std_ulogic
    );
  end component TcpShellInterface;
  
  component MemTestFlash is
    port (
     ------------------------------------------------------
     -- From SHELL / Clock and Reset
     ------------------------------------------------------
      ap_clk                     : in  std_logic;
      ap_rst_n                   : in  std_logic;
      ------------------------------------------------------
      -- BLock-Level I/O Protocol
      ------------------------------------------------------
      ap_start                   : in  std_logic;
      ap_done                    : out std_logic;
      ap_idle                    : out std_logic;
      ap_ready                   : out std_logic;
      ------------------------------------------------------
      -- From ROLE / Delayed Reset
      ------------------------------------------------------
      piSysReset_V               : in  std_logic_vector( 0 downto 0);
      piSysReset_V_ap_vld        : in  std_logic;
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piMMIO_diag_ctrl_V         : in  std_logic_vector(  1 downto 0);
      piMMIO_diag_ctrl_V_ap_vld  : in  std_logic;
      poMMIO_diag_stat_V         : out std_logic_vector(  1 downto 0);
      poMMIO_diag_stat_V_ap_vld  : out std_logic;
      poDebug_V                  : out std_logic_vector( 15 downto 0);
      poDebug_V_ap_vld           : out std_logic;
      ------------------------------------------------------  
      -- ROLE / Mem / Mp0 Interface
      ------------------------------------------------------
      ---- Axi4-Stream Read Command -----
      soMemRdCmdP0_TDATA         : out std_logic_vector( 79 downto 0);
      soMemRdCmdP0_TVALID        : out std_logic;
      soMemRdCmdP0_TREADY        : in  std_logic;
      ---- Axi4-Stream Read Status ------
      siMemRdStsP0_TDATA         : in  std_logic_vector(  7 downto 0);
      siMemRdStsP0_TVALID        : in  std_logic;
      siMemRdStsP0_TREADY        : out std_logic;
      ---- Axi4-Stream Data Output Channel
      siMemReadP0_TDATA          : in  std_logic_vector(511 downto 0);
      siMemReadP0_TKEEP          : in  std_logic_vector( 63 downto 0);
      siMemReadP0_TLAST          : in  std_logic_vector(  0 downto 0);
      siMemReadP0_TVALID         : in  std_logic;
      siMemReadP0_TREADY         : out std_logic;
      ---- Axi4-Stream Write Command ----  
      soMemWrCmdP0_TDATA         : out std_logic_vector( 79 downto 0);
      soMemWrCmdP0_TVALID        : out std_logic;
      soMemWrCmdP0_TREADY        : in  std_logic;
      ---- Axi4-Stream Write Status -----
      siMemWrStsP0_TDATA         : in  std_logic_vector(  7 downto 0);
      siMemWrStsP0_TVALID        : in  std_logic;
      siMemWrStsP0_TREADY        : out std_logic;
       ---- Axi4-Stream Write Command ----
      soMemWriteP0_TDATA         : out std_logic_vector(511 downto 0);
      soMemWriteP0_TKEEP         : out std_logic_vector( 63 downto 0);
      soMemWriteP0_TLAST         : out std_logic_vector(  0 downto 0);
      soMemWriteP0_TVALID        : out std_logic;
      soMemWriteP0_TREADY        : in  std_logic
    );
  end component MemTestFlash;

  component AxisRegisterSlice_64_8_1
    port (
      aclk          : in  std_logic;
      aresetn       : in  std_logic;
      s_axis_tdata  : in  std_logic_vector(63 downto 0);
      s_axis_tkeep  : in  std_logic_vector( 7 downto 0);
      s_axis_tlast  : in  std_logic;
      s_axis_tvalid : in  std_logic;
      s_axis_tready : out std_logic;
      m_axis_tdata  : out std_logic_vector(63 downto 0);
      m_axis_tkeep  : out std_logic_vector( 7 downto 0);
      m_axis_tlast  : out std_logic;
      m_axis_tvalid : out std_logic;
      m_axis_tready : in  std_logic
    );
  end component AxisRegisterSlice_64_8_1;
  
  component AxisRegisterSlice_32
    port (
      aclk          : in  std_logic;
      aresetn       : in  std_logic;
      s_axis_tdata  : in  std_logic_vector(31 downto 0);
      s_axis_tvalid : in  std_logic;
      s_axis_tready : out std_logic;
      m_axis_tdata  : out std_logic_vector(31 downto 0);
      m_axis_tvalid : out std_logic;
      m_axis_tready : in  std_logic
    );
  end component AxisRegisterSlice_32;
  
  component AxisRegisterSlice_16
    port (
      aclk          : in  std_logic;
      aresetn       : in  std_logic;
      s_axis_tvalid : in  std_logic;
      s_axis_tready : out std_logic;
      s_axis_tdata  : in  std_logic_vector(15 downto 0);
      m_axis_tvalid : out std_logic;
      m_axis_tready : in  std_logic;
      m_axis_tdata  : out std_logic_vector(15 downto 0)
    );
  end component AxisRegisterSlice_16;
  
  component AxisRegisterSlice_96
    port (
      aclk          : in  std_logic;
      aresetn       : in  std_logic;
      s_axis_tvalid : in  std_logic;
      s_axis_tready : out std_logic;
      s_axis_tdata  : in  std_logic_vector(95 downto 0);
      m_axis_tvalid : out std_logic;
      m_axis_tready : in  std_logic;
      m_axis_tdata  : out std_logic_vector(95 downto 0)
    );
  end component AxisRegisterSlice_96;

  component Fifo_16x16 is
    port (
      clk         : in  std_logic;
      srst        : in  std_logic;
      din         : in  std_logic_vector(15 downto 0);
      wr_en       : in  std_logic;
      rd_en       : in  std_logic;
      dout        : out std_logic_vector(15 downto 0);
      full        : out std_logic;
      empty       : out std_logic;
      wr_rst_busy : out std_logic;
      rd_rst_busy : out std_logic
    );
  end component Fifo_16x16;
  
  component Fifo_16x73 is
    port (
      clk         : in  std_logic;
      srst        : in  std_logic;
      din         : in  std_logic_vector(72 downto 0);
      wr_en       : in  std_logic;
      rd_en       : in  std_logic;
      dout        : out std_logic_vector(72 downto 0);
      full        : out std_logic;
      empty       : out std_logic;
      wr_rst_busy : out std_logic;
      rd_rst_busy : out std_logic
    );
  end component Fifo_16x73;
  
  component Fifo_16x96 is
    port (
      clk         : in  std_logic;
      srst        : in  std_logic;
      din         : in  std_logic_vector(95 downto 0);
      wr_en       : in  std_logic;
      rd_en       : in  std_logic;
      dout        : out std_logic_vector(95 downto 0);
      full        : out std_logic;
      empty       : out std_logic;
      wr_rst_busy : out std_logic;
      rd_rst_busy : out std_logic
    );
  end component Fifo_16x96;

  --===========================================================================
  --== FUNCTION DECLARATIONS  [TODO-Move to a package]
  --===========================================================================
  function fVectorize(s: std_ulogic) return std_ulogic_vector is
    variable v: std_ulogic_vector(0 downto 0);
  begin
    v(0) := s;
    return v;
  end fVectorize;
  
  function fScalarize(v: in std_ulogic_vector) return std_ulogic is
  begin
    assert v'length = 1
    report "scalarize: output port must be single bit!"
    severity FAILURE;
    return v(v'LEFT);
  end;

   
--################################################################################
--#                                                                              #
--#                          #####   ####  ####  #     #                         #
--#                          #    # #    # #   #  #   #                          #
--#                          #    # #    # #    #  ###                           #
--#                          #####  #    # #    #   #                            #
--#                          #    # #    # #    #   #                            #
--#                          #    # #    # #   #    #                            #
--#                          #####   ####  ####     #                            #
--#                                                                              #
--################################################################################
 
begin

  --################################################################################
  --#                                                                              #
  --#    #     #  ######  ###  #######                                             #
  --#    #     #  #        #   #                                                   #
  --#    #     #  #        #   #                                                   #
  --#    #     #  ######   #   ####                                                #
  --#    #     #       #   #   #                                                   #
  --#    #######  ######  ###  #                                                   #
  --#                                                                              #
  --################################################################################
  gUdpShellInterface : if gVivadoVersion = 2016 generate
      USIF : UdpShellInterface_Deprecated
        port map (
          ------------------------------------------------------
          -- From SHELL / Clock and Reset
          ------------------------------------------------------
          aclk                   => piSHL_156_25Clk,
          aresetn                => not piSHL_Mmio_Ly7Rst,
          --------------------------------------------------------
          -- SHELL / Mmio Interface
          --------------------------------------------------------
          piSHL_Mmio_En_V(0)     => piSHL_Mmio_Ly7En,
          --------------------------------------------------------
          -- SHELL / UDP Control Port Interfaces
          --------------------------------------------------------
          soSHL_LsnReq_tdata     => soSHL_Nts_Udp_LsnReq_tdata ,
          soSHL_LsnReq_tvalid    => soSHL_Nts_Udp_LsnReq_tvalid,
          soSHL_LsnReq_tready    => soSHL_Nts_Udp_LsnReq_tready,
          --
          siSHL_LsnRep_tdata     => siSHL_Nts_Udp_LsnRep_tdata ,
          siSHL_LsnRep_tvalid    => siSHL_Nts_Udp_LsnRep_tvalid,
          siSHL_LsnRep_tready    => siSHL_Nts_Udp_LsnRep_tready,
          --
          soSHL_ClsReq_tdata     => soSHL_Nts_Udp_ClsReq_tdata ,
          soSHL_ClsReq_tvalid    => soSHL_Nts_Udp_ClsReq_tvalid,
          soSHL_ClsReq_tready    => soSHL_Nts_Udp_ClsReq_tready,
          --
          siSHL_ClsRep_tdata     => siSHL_Nts_Udp_ClsRep_tdata ,
          siSHL_ClsRep_tvalid    => siSHL_Nts_Udp_ClsRep_tvalid,
          siSHL_ClsRep_tready    => siSHL_Nts_Udp_ClsRep_tready,
          --------------------------------------------------------
          -- SHELL / UDP Rx Data Interfaces
          --------------------------------------------------------
          siSHL_Data_tdata       => siSHL_Nts_Udp_Data_tdata,
          siSHL_Data_tkeep       => siSHL_Nts_Udp_Data_tkeep,       
          siSHL_Data_tlast       => siSHL_Nts_Udp_Data_tlast,
          siSHL_Data_tvalid      => siSHL_Nts_Udp_Data_tvalid,
          siSHL_Data_tready      => siSHL_Nts_Udp_Data_tready,
          --
          siSHL_Meta_tdata       => siSHL_Nts_Udp_Meta_tdata,
          siSHL_Meta_tvalid      => siSHL_Nts_Udp_Meta_tvalid,
          siSHL_Meta_tready      => siSHL_Nts_Udp_Meta_tready,
          --
          siSHL_DLen_tdata       => siSHL_Nts_Udp_DLen_tdata,
          siSHL_DLen_tvalid      => siSHL_Nts_Udp_DLen_tvalid,
          siSHL_DLen_tready      => siSHL_Nts_Udp_DLen_tready,
          --------------------------------------------------------
          -- SHELL / UDP Tx Data Interfaces
          --------------------------------------------------------
          soSHL_Data_tdata       => soSHL_Nts_Udp_Data_tdata,
          soSHL_Data_tkeep       => soSHL_Nts_Udp_Data_tkeep,       
          soSHL_Data_tlast       => soSHL_Nts_Udp_Data_tlast,
          soSHL_Data_tvalid      => soSHL_Nts_Udp_Data_tvalid,
          soSHL_Data_tready      => soSHL_Nts_Udp_Data_tready,
          --      
          soSHL_Meta_tdata       => soSHL_Nts_Udp_Meta_tdata,
          soSHL_Meta_tvalid      => soSHL_Nts_Udp_Meta_tvalid,
          soSHL_Meta_tready      => soSHL_Nts_Udp_Meta_tready,
          --
          soSHL_DLen_tdata       => soSHL_Nts_Udp_DLen_tdata,
          soSHL_DLen_tvalid      => soSHL_Nts_Udp_DLen_tvalid,
          soSHL_DLen_tready      => soSHL_Nts_Udp_DLen_tready,
          --------------------------------------------------------
          -- UAF / UDP Tx Data Interfaces
          --------------------------------------------------------
          siUAF_Data_tdata       => ssUAF_USIF_Data_tdata,
          siUAF_Data_tkeep       => ssUAF_USIF_Data_tkeep,
          siUAF_Data_tlast       => ssUAF_USIF_Data_tlast,
          siUAF_Data_tvalid      => ssUAF_USIF_Data_tvalid,
          siUAF_Data_tready      => ssUAF_USIF_Data_tready,
          -- 
          siUAF_Meta_tdata       => ssUAF_USIF_Meta_tdata,
          siUAF_Meta_tvalid      => ssUAF_USIF_Meta_tvalid,
          siUAF_Meta_tready      => ssUAF_USIF_Meta_tready,
          --
          siUAF_DLen_tdata       => ssUAF_USIF_DLen_tdata,
          siUAF_DLen_tvalid      => ssUAF_USIF_DLen_tvalid,
          siUAF_DLen_tready      => ssUAF_USIF_DLen_tready,
          --------------------------------------------------------
          -- UAF / UDP Rx Data Interfaces
          --------------------------------------------------------
          soUAF_Data_tdata       => ssUSIF_UAF_Data_tdata,
          soUAF_Data_tkeep       => ssUSIF_UAF_Data_tkeep,
          soUAF_Data_tlast       => ssUSIF_UAF_Data_tlast,
          soUAF_Data_tvalid      => ssUSIF_UAF_Data_tvalid,
          soUAF_Data_tready      => ssUSIF_UAF_Data_tready,
          --
          soUAF_Meta_tdata       => ssUSIF_UAF_Meta_tdata,
          soUAF_Meta_tvalid      => ssUSIF_UAF_Meta_tvalid,
          soUAF_Meta_tready      => ssUSIF_UAF_Meta_tready,
          --
          soUAF_DLen_tdata       => ssUSIF_UAF_DLen_tdata,
          soUAF_DLen_tvalid      => ssUSIF_UAF_DLen_tvalid,
          soUAF_DLen_tready      => ssUSIF_UAF_DLen_tready
        ); -- End-of: UdpShellInterface_Deprecated
  else generate
      USIF : UdpShellInterface
        port map (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        ap_clk                  => piSHL_156_25Clk,
        ap_rst_n                => not piSHL_Mmio_Ly7Rst,
        --------------------------------------------------------
        -- SHELL / Mmio Interface
        --------------------------------------------------------
        piSHL_Mmio_En_V         => piSHL_Mmio_Ly7En,
        --------------------------------------------------------
        -- SHELL / UDP Control Port Interfaces
        --------------------------------------------------------
        soSHL_LsnReq_V_V_tdata  => soSHL_Nts_Udp_LsnReq_tdata ,
        soSHL_LsnReq_V_V_tvalid => soSHL_Nts_Udp_LsnReq_tvalid,
        soSHL_LsnReq_V_V_tready => soSHL_Nts_Udp_LsnReq_tready,
        --
        siSHL_LsnRep_V_tdata    => siSHL_Nts_Udp_LsnRep_tdata ,
        siSHL_LsnRep_V_tvalid   => siSHL_Nts_Udp_LsnRep_tvalid,
        siSHL_LsnRep_V_tready   => siSHL_Nts_Udp_LsnRep_tready,
        --
        soSHL_ClsReq_V_V_tdata  => soSHL_Nts_Udp_ClsReq_tdata ,
        soSHL_ClsReq_V_V_tvalid => soSHL_Nts_Udp_ClsReq_tvalid,
        soSHL_ClsReq_V_V_tready => soSHL_Nts_Udp_ClsReq_tready,
        --
        siSHL_ClsRep_V_tdata    => siSHL_Nts_Udp_ClsRep_tdata ,
        siSHL_ClsRep_V_tvalid   => siSHL_Nts_Udp_ClsRep_tvalid,
        siSHL_ClsRep_V_tready   => siSHL_Nts_Udp_ClsRep_tready,
        --------------------------------------------------------
        -- SHELL / UDP Rx Data Interfaces
        --------------------------------------------------------
        siSHL_Data_tdata        => siSHL_Nts_Udp_Data_tdata,
        siSHL_Data_tkeep        => siSHL_Nts_Udp_Data_tkeep,       
        siSHL_Data_tlast        => siSHL_Nts_Udp_Data_tlast,
        siSHL_Data_tvalid       => siSHL_Nts_Udp_Data_tvalid,
        siSHL_Data_tready       => siSHL_Nts_Udp_Data_tready,
      
        siSHL_Meta_V_tdata      => siSHL_Nts_Udp_Meta_tdata,
        siSHL_Meta_V_tvalid     => siSHL_Nts_Udp_Meta_tvalid,
        siSHL_Meta_V_tready     => siSHL_Nts_Udp_Meta_tready,
      
        siSHL_DLen_V_V_tdata    => siSHL_Nts_Udp_DLen_tdata,
        siSHL_DLen_V_V_tvalid   => siSHL_Nts_Udp_DLen_tvalid,
        siSHL_DLen_V_V_tready   => siSHL_Nts_Udp_DLen_tready,
        --------------------------------------------------------
        -- SHELL / UDP Tx Data Interfaces
        --------------------------------------------------------
        soSHL_Data_tdata        => soSHL_Nts_Udp_Data_tdata,
        soSHL_Data_tkeep        => soSHL_Nts_Udp_Data_tkeep,       
        soSHL_Data_tlast        => soSHL_Nts_Udp_Data_tlast,
        soSHL_Data_tvalid       => soSHL_Nts_Udp_Data_tvalid,
        soSHL_Data_tready       => soSHL_Nts_Udp_Data_tready,
        --   
        soSHL_Meta_V_tdata      => soSHL_Nts_Udp_Meta_tdata,
        soSHL_Meta_V_tvalid     => soSHL_Nts_Udp_Meta_tvalid,
        soSHL_Meta_V_tready     => soSHL_Nts_Udp_Meta_tready,
       
        soSHL_DLen_V_V_tdata    => soSHL_Nts_Udp_DLen_tdata,
        soSHL_DLen_V_V_tvalid   => soSHL_Nts_Udp_DLen_tvalid,
        soSHL_DLen_V_V_tready   => soSHL_Nts_Udp_DLen_tready,
        --------------------------------------------------------
        -- UAF / UDP Tx Data Interfaces
        --------------------------------------------------------
        siUAF_Data_tdata        => ssUARS_USIF_Data_tdata,
        siUAF_Data_tkeep        => ssUARS_USIF_Data_tkeep,
        siUAF_Data_tlast        => ssUARS_USIF_Data_tlast,
        siUAF_Data_tvalid       => ssUARS_USIF_Data_tvalid,
        siUAF_Data_tready       => ssUARS_USIF_Data_tready,
        -- 
        siUAF_Meta_V_tdata      => ssUARS_USIF_Meta_tdata,
        siUAF_Meta_V_tvalid     => ssUARS_USIF_Meta_tvalid,
        siUAF_Meta_V_tready     => ssUARS_USIF_Meta_tready,
        --
        siUAF_DLen_V_V_tdata    => ssUARS_USIF_DLen_tdata,
        siUAF_DLen_V_V_tvalid   => ssUARS_USIF_DLen_tvalid,
        siUAF_DLen_V_V_tready   => ssUARS_USIF_DLen_tready,
        --------------------------------------------------------
        -- UAF / UDP Rx Data Interfaces
        --------------------------------------------------------
        soUAF_Data_tdata        => ssUSIF_UARS_Data_tdata,
        soUAF_Data_tkeep        => ssUSIF_UARS_Data_tkeep,
        soUAF_Data_tlast        => ssUSIF_UARS_Data_tlast,
        soUAF_Data_tvalid       => ssUSIF_UARS_Data_tvalid,
        soUAF_Data_tready       => ssUSIF_UARS_Data_tready,
        --
        soUAF_Meta_V_tdata      => ssUSIF_UARS_Meta_tdata,
        soUAF_Meta_V_tvalid     => ssUSIF_UARS_Meta_tvalid,
        soUAF_Meta_V_tready     => ssUSIF_UARS_Meta_tready,
        --
        soUAF_DLen_V_V_tdata    => ssUSIF_UARS_DLen_tdata,
        soUAF_DLen_V_V_tvalid   => ssUSIF_UARS_DLen_tvalid,
        soUAF_DLen_V_V_tready   => ssUSIF_UARS_DLen_tready
      ); -- End-of: UdpShellInterface
  end generate;

  --###############################################################################
  --#                                                                             #
  --#    #     #  #####    ######     #####                                       #
  --#    #     #  #    #   #     #   #     # #####  #####                         #
  --#    #     #  #     #  #     #   #     # #    # #    #                        #
  --#    #     #  #     #  ######    ####### #####  #####                         #
  --#    #     #  #    #   #         #     # #      #                             #
  --#    #######  #####    #         #     # #      #                             #
  --#                                                                             #
  --###############################################################################
  
  --==========================================================================
  --==  INST: UDP-APPLICATION_FLASH (UAF) for cFp_HelloKale
  --==   This application implements a set of UDP-oriented tests. The [UAF]
  --==   connects to the SHELL via a UDP Shell Interface (USIF) block. The
  --==   main purpose of the [USIF] is to provide a placeholder for the 
  --==   opening of one or multiple listening port(s). The use of the [USIF] is
  --==   not a prerequisite, but it is provided here for sake of simplicity.
  --==========================================================================
  gUdpAppFlash : if gVivadoVersion = 2016 generate
    UAF : UdpApplicationFlash_Deprecated
      port map (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        aclk                      => piSHL_156_25Clk,
        aresetn                   => not piSHL_Mmio_Ly7Rst,
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------
        piSHL_Mmio_En_V(0)       => piSHL_Mmio_Ly7En,
        --[NOT_USED] piSHL_Mmio_EchoCtrl_V   => piSHL_Mmio_UdpEchoCtrl,
        --[NOT_USED] piSHL_Mmio_PostDgmEn_V  => piSHL_Mmio_UdpPostDgmEn,
        --[NOT_USED] piSHL_Mmio_CaptDgmEn_V  => piSHL_Mmio_UdpCaptDgmEn,
        --------------------------------------------------------
        -- From USIF / UDP Rx Data Interfaces
        --------------------------------------------------------
        siUSIF_Data_tdata     => ssUSIF_UAF_Data_tdata,
        siUSIF_Data_tkeep     => ssUSIF_UAF_Data_tkeep,
        siUSIF_Data_tlast     => ssUSIF_UAF_Data_tlast,
        siUSIF_Data_tvalid    => ssUSIF_UAF_Data_tvalid,
        siUSIF_Data_tready    => ssUSIF_UAF_Data_tready,
        --
        siUSIF_Meta_tdata     => ssUSIF_UAF_Meta_tdata,
        siUSIF_Meta_tvalid    => ssUSIF_UAF_Meta_tvalid,
        siUSIF_Meta_tready    => ssUSIF_UAF_Meta_tready,
        --
        siUSIF_DLen_tdata     => ssUSIF_UAF_DLen_tdata,
        siUSIF_DLen_tvalid    => ssUSIF_UAF_DLen_tvalid,
        siUSIF_DLen_tready    => ssUSIF_UAF_DLen_tready,
        --------------------------------------------------------
        -- To USIF / UDP Tx Data Interfaces
        --------------------------------------------------------
        soUSIF_Data_tdata     => ssUAF_USIF_Data_tdata ,
        soUSIF_Data_tkeep     => ssUAF_USIF_Data_tkeep ,
        soUSIF_Data_tlast     => ssUAF_USIF_Data_tlast ,
        soUSIF_Data_tvalid    => ssUAF_USIF_Data_tvalid,
        soUSIF_Data_tready    => ssUAF_USIF_Data_tready,
        --
        soUSIF_Meta_tdata     => ssUAF_USIF_Meta_tdata ,
        soUSIF_Meta_tvalid    => ssUAF_USIF_Meta_tvalid,
        soUSIF_Meta_tready    => ssUAF_USIF_Meta_tready,
        --
        soUSIF_DLen_tdata     => ssUAF_USIF_DLen_tdata ,
        soUSIF_DLen_tvalid    => ssUAF_USIF_DLen_tvalid,
        soUSIF_DLen_tready    => ssUAF_USIF_DLen_tready
      );
  else generate
    UAF : UdpApplicationFlash
      port map (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        ap_clk                 => piSHL_156_25Clk,
        ap_rst_n               => not piSHL_Mmio_Ly7Rst,
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------
        piSHL_Mmio_En_V(0)     => piSHL_Mmio_Ly7En,  
        --[NOT_USED] piSHL_Mmio_EchoCtrl_V   => piSHL_Mmio_UdpEchoCtrl,
        --[NOT_USED] piSHL_Mmio_PostDgmEn_V  => piSHL_Mmio_UdpPostDgmEn,
        --[NOT_USED] piSHL_Mmio_CaptDgmEn_V  => piSHL_Mmio_UdpCaptDgmEn,
                --------------------------------------------------------
        -- From USIF / UDP Rx Data Interfaces
        --------------------------------------------------------
        siUSIF_Data_tdata     => ssUARS_UAF_Data_tdata,
        siUSIF_Data_tkeep     => ssUARS_UAF_Data_tkeep,
        siUSIF_Data_tlast     => ssUARS_UAF_Data_tlast,
        siUSIF_Data_tvalid    => ssUARS_UAF_Data_tvalid,
        siUSIF_Data_tready    => ssUARS_UAF_Data_tready,
        --
        siUSIF_Meta_V_tdata   => ssUARS_UAF_Meta_tdata,
        siUSIF_Meta_V_tvalid  => ssUARS_UAF_Meta_tvalid,
        siUSIF_Meta_V_tready  => ssUARS_UAF_Meta_tready,
        --
        siUSIF_DLen_V_V_tdata => ssUARS_UAF_DLen_tdata,
        siUSIF_DLen_V_V_tvalid=> ssUARS_UAF_DLen_tvalid,
        siUSIF_DLen_V_V_tready=> ssUARS_UAF_DLen_tready,
        --------------------------------------------------------
        -- To USIF / UDP Tx Data Interfaces
        --------------------------------------------------------
        soUSIF_Data_tdata     => ssUAF_UARS_Data_tdata ,
        soUSIF_Data_tkeep     => ssUAF_UARS_Data_tkeep ,
        soUSIF_Data_tlast     => ssUAF_UARS_Data_tlast ,
        soUSIF_Data_tvalid    => ssUAF_UARS_Data_tvalid,
        soUSIF_Data_tready    => ssUAF_UARS_Data_tready,
        --
        soUSIF_Meta_V_tdata   => ssUAF_UARS_Meta_tdata ,
        soUSIF_Meta_V_tvalid  => ssUAF_UARS_Meta_tvalid,
        soUSIF_Meta_V_tready  => ssUAF_UARS_Meta_tready,
        --
        soUSIF_DLen_V_V_tdata  => ssUAF_UARS_DLen_tdata ,
        soUSIF_DLen_V_V_tvalid => ssUAF_UARS_DLen_tvalid,
        soUSIF_DLen_V_V_tready => ssUAF_UARS_DLen_tready
      );
  end generate;
  
  --###############################################################################
  --#                                                                             #
  --#    #     #  #####    ######    ######                                       #
  --#    #     #  #    #   #     #   #      #  #####  ####   ####                 #
  --#    #     #  #     #  #     #   #         #     #    # #                     #
  --#    #     #  #     #  ######    #####  #  ####  #    # #####                 #
  --#    #     #  #    #   #         #      #  #     #    #     #                 #
  --#    #######  #####    #         #      #  #      ####  ####                  #
  --#                                                                             #
  --###############################################################################
  gUdpTxFifos : if gVivadoVersion /= 2016 generate
    ARS_UDP_RX_DATA : AxisRegisterSlice_64_8_1
      port map (
        aclk          => piSHL_156_25Clk,
        aresetn       => not piSHL_Mmio_Ly7Rst,
        s_axis_tdata  => ssUSIF_UARS_Data_tdata,
        s_axis_tkeep  => ssUSIF_UARS_Data_tkeep,
        s_axis_tlast  => ssUSIF_UARS_Data_tlast,
        s_axis_tvalid => ssUSIF_UARS_Data_tvalid,
        s_axis_tready => ssUSIF_UARS_Data_tready,
        --
        m_axis_tdata  => ssUARS_UAF_Data_tdata,
        m_axis_tkeep  => ssUARS_UAF_Data_tkeep,
        m_axis_tlast  => ssUARS_UAF_Data_tlast,
        m_axis_tvalid => ssUARS_UAF_Data_tvalid,
        m_axis_tready => ssUARS_UAF_Data_tready
      );
    ARS_UDP_RX_META : AxisRegisterSlice_96
      port map (
        aclk          => piSHL_156_25Clk,
        aresetn       => not piSHL_Mmio_Ly7Rst,
        s_axis_tdata  => ssUSIF_UARS_Meta_tdata,
        s_axis_tvalid => ssUSIF_UARS_Meta_tvalid,
        s_axis_tready => ssUSIF_UARS_Meta_tready,
        --
        m_axis_tdata  => ssUARS_UAF_Meta_tdata, 
        m_axis_tvalid => ssUARS_UAF_Meta_tvalid,
        m_axis_tready => ssUARS_UAF_Meta_tready
      );
    --
    ARS_UDP_RX_DLEN : AxisRegisterSlice_16
      port map (
        aclk          => piSHL_156_25Clk,
        aresetn       => not piSHL_Mmio_Ly7Rst,
        s_axis_tdata  => ssUSIF_UARS_DLen_tdata,
        s_axis_tvalid => ssUSIF_UARS_DLen_tvalid,
        s_axis_tready => ssUSIF_UARS_DLen_tready,
        --
        m_axis_tdata  => ssUARS_UAF_DLen_tdata, 
        m_axis_tvalid => ssUARS_UAF_DLen_tvalid,
        m_axis_tready => ssUARS_UAF_DLen_tready
      );
    --
    ARS_UDP_TX_DATA : AxisRegisterSlice_64_8_1
      port map (
        aclk          => piSHL_156_25Clk,
        aresetn       => not piSHL_Mmio_Ly7Rst,
        s_axis_tdata  => ssUAF_UARS_Data_tdata,
        s_axis_tkeep  => ssUAF_UARS_Data_tkeep,
        s_axis_tlast  => ssUAF_UARS_Data_tlast,
        s_axis_tvalid => ssUAF_UARS_Data_tvalid,
        s_axis_tready => ssUAF_UARS_Data_tready,
        --
        m_axis_tdata  => ssUARS_USIF_Data_tdata,
        m_axis_tkeep  => ssUARS_USIF_Data_tkeep,
        m_axis_tlast  => ssUARS_USIF_Data_tlast,
        m_axis_tvalid => ssUARS_USIF_Data_tvalid,
        m_axis_tready => ssUARS_USIF_Data_tready
      );
    ARS_UDP_TX_META : AxisRegisterSlice_96
      port map (
        aclk          => piSHL_156_25Clk,
        aresetn       => not piSHL_Mmio_Ly7Rst,
        s_axis_tdata  => ssUAF_UARS_Meta_tdata,
        s_axis_tvalid => ssUAF_UARS_Meta_tvalid,
        s_axis_tready => ssUAF_UARS_Meta_tready,
        --
        m_axis_tdata  => ssUARS_USIF_Meta_tdata, 
        m_axis_tvalid => ssUARS_USIF_Meta_tvalid,
        m_axis_tready => ssUARS_USIF_Meta_tready
      );
    ARS_UDP_TX_DLEN : AxisRegisterSlice_16
      port map (
        aclk          => piSHL_156_25Clk,
        aresetn       => not piSHL_Mmio_Ly7Rst,
        s_axis_tdata  => ssUAF_UARS_DLen_tdata,
        s_axis_tvalid => ssUAF_UARS_DLen_tvalid,
        s_axis_tready => ssUAF_UARS_DLen_tready,
        --
        m_axis_tdata  => ssUARS_USIF_DLen_tdata, 
        m_axis_tvalid => ssUARS_USIF_DLen_tvalid,
        m_axis_tready => ssUARS_USIF_DLen_tready
      );
--    FIFO_UDP_RX_DATA : Fifo_16x73
--      port map (
--        clk          => piSHL_156_25Clk,
--        srst         => piSHL_Mmio_Ly7Rst,
--        din          => ssUSIF_FIFO_Udp_Data_data,
--        wr_en        => ssUSIF_FIFO_Udp_Data_write,
--        full         => ssUSIF_FIFO_Udp_Data_full,
--        --           
--        dout         => ssFIFO_UAF_Udp_Data_data,
--        rd_en        => ssFIFO_UAF_Udp_Data_read,
--        empty        => ssFIFO_UAF_Udp_Data_empty,
--        wr_rst_busy  => open,
--        rd_rst_busy  => open
--      );
--    FIFO_UDP_RX_META : Fifo_16x96                   
--      port map (                                    
--        clk          => piSHL_156_25Clk,            
--        srst         => piSHL_Mmio_Ly7Rst,          
--        din          => ssUSIF_FIFO_Udp_Meta_data,  
--        wr_en        => ssUSIF_FIFO_Udp_Meta_write, 
--        full         => ssUSIF_FIFO_Udp_Meta_full,  
--        --                                          
--        dout         => ssFIFO_UAF_Udp_Meta_data,  
--        rd_en        => ssFIFO_UAF_Udp_Meta_read,   
--        empty        => ssFIFO_UAF_Udp_Meta_empty,
--        wr_rst_busy  => open,                       
--        rd_rst_busy  => open                        
--      );
--    --
--    FIFO_UDP_TX_DATA : Fifo_16x73
--      port map (
--        clk          => piSHL_156_25Clk,
--        srst         => piSHL_Mmio_Ly7Rst,
--        din          => ssUAF_FIFO_Udp_Data_data,
--        wr_en        => ssUAF_FIFO_Udp_Data_write,
--        full         => ssUAF_FIFO_Udp_Data_full,
--        --                      
--        dout         => ssFIFO_USIF_Udp_Data_data,
--        rd_en        => ssFIFO_USIF_Udp_Data_read,
--        empty        => ssFIFO_USIF_Udp_Data_empty,
--        wr_rst_busy  => open,
--        rd_rst_busy  => open
--    );
--    FIFO_UDP_TX_META : Fifo_16x96
--      port map (
--        clk          => piSHL_156_25Clk,
--        srst         => piSHL_Mmio_Ly7Rst,
--        din          => ssUAF_FIFO_Udp_Meta_data,
--        wr_en        => ssUAF_FIFO_Udp_Meta_write,
--        full         => ssUAF_FIFO_Udp_Meta_full,
--        --
--        dout         => ssFIFO_USIF_Udp_Meta_data,
--        rd_en        => ssFIFO_USIF_Udp_Meta_read,
--        empty        => ssFIFO_USIF_Udp_Meta_empty,
--        wr_rst_busy  => open,
--        rd_rst_busy  => open
--      );
--    FIFO_UDP_TX_DLEN : Fifo_16x16
--      port map (
--        clk          => piSHL_156_25Clk,
--        srst         => piSHL_Mmio_Ly7Rst,
--        din          => ssUAF_FIFO_Udp_DLen_data,
--        wr_en        => ssUAF_FIFO_Udp_DLen_write,
--        full         => ssUAF_FIFO_Udp_DLen_full,
--        --
--        dout         => ssFIFO_USIF_Udp_DLen_data,
--        rd_en        => ssFIFO_USIF_Udp_DLen_read,
--        empty        => ssFIFO_USIF_Udp_DLen_empty,
--        wr_rst_busy  => open,
--        rd_rst_busy  => open
--      );       
  end generate;

  --################################################################################
  --#                                                                              #
  --#    #######  ######  ###  #######                                             #
  --#       #     #        #   #                                                   #
  --#       #     #        #   #                                                   #
  --#       #     ######   #   ####                                                #
  --#       #          #   #   #                                                   #
  --#       #     ######  ###  #                                                   #
  --#                                                                              #
  --################################################################################
  gTcpShellInterface : if gVivadoVersion = 2016 generate
    TSIF : TcpShellInterface_Deprecated
      port map (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        aclk                      => piSHL_156_25Clk,
        aresetn                   => not piSHL_Mmio_Ly7Rst,
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------       
        piSHL_Mmio_En_V           => piSHL_Mmio_Ly7En,
        ------------------------------------------------------
        -- TAF (via TARS) / Tx Data Interfaces (APP-->SHELL)
        ------------------------------------------------------
        siTAF_Data_tdata          => ssTARS_TSIF_Data_tdata,
        siTAF_Data_tkeep          => ssTARS_TSIF_Data_tkeep,
        siTAF_Data_tlast          => ssTARS_TSIF_Data_tlast,
        siTAF_Data_tvalid         => ssTARS_TSIF_Data_tvalid,
        siTAF_Data_tready         => ssTARS_TSIF_Data_tready,
        --
        siTAF_SessId_tdata        => ssTARS_TSIF_SessId_tdata,
        siTAF_SessId_tvalid       => ssTARS_TSIF_SessId_tvalid,
        siTAF_SessId_tready       => ssTARS_TSIF_SessId_tready,
        --
        siTAF_DatLen_tdata        => ssTARS_TSIF_DatLen_tdata,
        siTAF_DatLen_tvalid       => ssTARS_TSIF_DatLen_tvalid,
        siTAF_DatLen_tready       => ssTARS_TSIF_DatLen_tready,
        ------------------------------------------------------
        -- TAF (via TARS) / Rx Data Interfaces (SHELL-->APP)
        ------------------------------------------------------
        soTAF_Data_tdata          => ssTSIF_TARS_Data_tdata,
        soTAF_Data_tkeep          => ssTSIF_TARS_Data_tkeep,
        soTAF_Data_tlast          => ssTSIF_TARS_Data_tlast,
        soTAF_Data_tvalid         => ssTSIF_TARS_Data_tvalid,
        soTAF_Data_tready         => ssTSIF_TARS_Data_tready,
        --
        soTAF_SessId_tdata        => ssTSIF_TARS_SessId_tdata,
        soTAF_SessId_tvalid       => ssTSIF_TARS_SessId_tvalid,
        soTAF_SessId_tready       => ssTSIF_TARS_SessId_tready,
        --
        soTAF_DatLen_tdata        => ssTSIF_TARS_DatLen_tdata,
        soTAF_DatLen_tvalid       => ssTSIF_TARS_DatLen_tvalid,
        soTAF_DatLen_tready       => ssTSIF_TARS_DatLen_tready,
        ------------------------------------------------------
        -- SHELL / RxP Data Flow Interfaces
        ------------------------------------------------------
        ---- TCP Data Stream Notification 
        siSHL_Notif_tdata         => siSHL_Nts_Tcp_Notif_tdata,
        siSHL_Notif_tvalid        => siSHL_Nts_Tcp_Notif_tvalid,
        siSHL_Notif_tready        => siSHL_Nts_Tcp_Notif_tready,
        ---- TCP Data Request Stream -----
        soSHL_DReq_tdata          => soSHL_Nts_Tcp_DReq_tdata,
        soSHL_DReq_tvalid         => soSHL_Nts_Tcp_DReq_tvalid,
        soSHL_DReq_tready         => soSHL_Nts_Tcp_DReq_tready,
        ---- TCP Data Stream  ------------
        siSHL_Data_tdata          => siSHL_Nts_Tcp_Data_tdata, 
        siSHL_Data_tkeep          => siSHL_Nts_Tcp_Data_tkeep, 
        siSHL_Data_tlast          => siSHL_Nts_Tcp_Data_tlast, 
        siSHL_Data_tvalid         => siSHL_Nts_Tcp_Data_tvalid,
        siSHL_Data_tready         => siSHL_Nts_Tcp_Data_tready,
        ---- TCP Meta Stream -------------
        siSHL_Meta_tdata          => siSHL_Nts_Tcp_Meta_tdata,
        siSHL_Meta_tvalid         => siSHL_Nts_Tcp_Meta_tvalid,
        siSHL_Meta_tready         => siSHL_Nts_Tcp_Meta_tready,
        ------------------------------------------------------
        -- SHELL / RxP Ctlr Flow Interfaces
        ------------------------------------------------------
        -- FPGA Receive Path (SHELL-->APP) --------
        ---- TCP Listen Request Stream -----
        soSHL_LsnReq_tdata        => soSHL_Nts_Tcp_LsnReq_tdata,
        soSHL_LsnReq_tvalid       => soSHL_Nts_Tcp_LsnReq_tvalid,
        soSHL_LsnReq_tready       => soSHL_Nts_Tcp_LsnReq_tready,
        ---- TCP Listen Rep Stream ---------
        siSHL_LsnRep_tdata        => siSHL_Nts_Tcp_LsnRep_tdata,
        siSHL_LsnRep_tvalid       => siSHL_Nts_Tcp_LsnRep_tvalid, 
        siSHL_LsnRep_tready       => siSHL_Nts_Tcp_LsnRep_tready, 
        ------------------------------------------------------
        -- SHELL / TxP Data Flow Interfaces
        ------------------------------------------------------
        ---- TCP Data Stream ------------- 
        soSHL_Data_tdata          => soSHL_Nts_Tcp_Data_tdata, 
        soSHL_Data_tkeep          => soSHL_Nts_Tcp_Data_tkeep, 
        soSHL_Data_tlast          => soSHL_Nts_Tcp_Data_tlast, 
        soSHL_Data_tvalid         => soSHL_Nts_Tcp_Data_tvalid,
        soSHL_Data_tready         => soSHL_Nts_Tcp_Data_tready,
        ---- TCP Send Request ------------
        soSHL_SndReq_tdata        => soSHL_Nts_Tcp_SndReq_tdata,
        soSHL_SndReq_tvalid       => soSHL_Nts_Tcp_SndReq_tvalid,
        soSHL_SndReq_tready       => soSHL_Nts_Tcp_SndReq_tready,
        ---- TCP Send Reply --------------
        siSHL_SndRep_tdata        => siSHL_Nts_Tcp_SndRep_tdata,
        siSHL_SndRep_tvalid       => siSHL_Nts_Tcp_SndRep_tvalid,
        siSHL_SndRep_tready       => siSHL_Nts_Tcp_SndRep_tready,
        ------------------------------------------------------
        -- SHELL / TxP Ctlr Flow Interfaces
        ------------------------------------------------------
        -- FPGA Transmit Path (APP-->SHELL) -------
        ---- TCP Open Session Request Stream 
        soSHL_OpnReq_tdata        => soSHL_Nts_Tcp_OpnReq_tdata, 
        soSHL_OpnReq_tvalid       => soSHL_Nts_Tcp_OpnReq_tvalid,
        soSHL_OpnReq_tready       => soSHL_Nts_Tcp_OpnReq_tready,
        ---- TCP Open Session Status Stream  
        siSHL_OpnRep_tdata        => siSHL_Nts_Tcp_OpnRep_tdata,  
        siSHL_OpnRep_tvalid       => siSHL_Nts_Tcp_OpnRep_tvalid,
        siSHL_OpnRep_tready       => siSHL_Nts_Tcp_OpnRep_tready,
        ---- TCP Close Request Stream  ---
        soSHL_ClsReq_tdata        => soSHL_Nts_Tcp_ClsReq_tdata,
        soSHL_ClsReq_tvalid       => soSHL_Nts_Tcp_ClsReq_tvalid,
        soSHL_ClsReq_tready       => soSHL_Nts_Tcp_ClsReq_tready
      ); -- End of: TcpShellInterface_Deprecated
  else generate
    TSIF : TcpShellInterface
      port map (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        ap_clk                    => piSHL_156_25Clk,
        ap_rst_n                  => not piSHL_Mmio_Ly7Rst,
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------
        piSHL_Mmio_En_V           => piSHL_Mmio_Ly7En,
        ------------------------------------------------------
        -- TAF (via TARS) / TxP Data Flow Interfaces (APP-->SHELL)
        ------------------------------------------------------
        siTAF_Data_tdata          => ssTARS_TSIF_Data_tdata,
        siTAF_Data_tkeep          => ssTARS_TSIF_Data_tkeep,
        siTAF_Data_tlast          => ssTARS_TSIF_Data_tlast,
        siTAF_Data_tvalid         => ssTARS_TSIF_Data_tvalid,
        siTAF_Data_tready         => ssTARS_TSIF_Data_tready,
        --
        siTAF_SessId_V_V_tdata    => ssTARS_TSIF_SessId_tdata,
        siTAF_SessId_V_V_tvalid   => ssTARS_TSIF_SessId_tvalid,
        siTAF_SessId_V_V_tready   => ssTARS_TSIF_SessId_tready,
        --
        siTAF_DatLen_V_V_tdata    => ssTARS_TSIF_DatLen_tdata,
        siTAF_DatLen_V_V_tvalid   => ssTARS_TSIF_DatLen_tvalid,
        siTAF_DatLen_V_V_tready   => ssTARS_TSIF_DatLen_tready,
        ------------------------------------------------------
        -- TAF (via TARS) / RxP Data Flow Interfaces (SHELL-->APP)
        ------------------------------------------------------  
        soTAF_Data_tdata          => ssTSIF_TARS_Data_tdata,
        soTAF_Data_tkeep          => ssTSIF_TARS_Data_tkeep,
        soTAF_Data_tlast          => ssTSIF_TARS_Data_tlast,
        soTAF_Data_tvalid         => ssTSIF_TARS_Data_tvalid,
        soTAF_Data_tready         => ssTSIF_TARS_Data_tready,
        --
        soTAF_SessId_V_V_tdata    => ssTSIF_TARS_SessId_tdata,
        soTAF_SessId_V_V_tvalid   => ssTSIF_TARS_SessId_tvalid,
        soTAF_SessId_V_V_tready   => ssTSIF_TARS_SessId_tready,
        --
        soTAF_DatLen_V_V_tdata    => ssTSIF_TARS_DatLen_tdata,
        soTAF_DatLen_V_V_tvalid   => ssTSIF_TARS_DatLen_tvalid,
        soTAF_DatLen_V_V_tready   => ssTSIF_TARS_DatLen_tready,
        ------------------------------------------------------
        -- SHELL / RxP Data Flow Interfaces
        ------------------------------------------------------
        ---- TCP Data Notification Stream
        siSHL_Notif_V_tdata       => siSHL_Nts_Tcp_Notif_tdata,
        siSHL_Notif_V_tvalid      => siSHL_Nts_Tcp_Notif_tvalid,
        siSHL_Notif_V_tready      => siSHL_Nts_Tcp_Notif_tready,
        ---- TCP Data Request Stream
        soSHL_DReq_V_tdata        => soSHL_Nts_Tcp_DReq_tdata,
        soSHL_DReq_V_tvalid       => soSHL_Nts_Tcp_DReq_tvalid,
        soSHL_DReq_V_tready       => soSHL_Nts_Tcp_DReq_tready,
        ---- TCP Data Stream
        siSHL_Data_tdata          => siSHL_Nts_Tcp_Data_tdata, 
        siSHL_Data_tkeep          => siSHL_Nts_Tcp_Data_tkeep, 
        siSHL_Data_tlast          => siSHL_Nts_Tcp_Data_tlast, 
        siSHL_Data_tvalid         => siSHL_Nts_Tcp_Data_tvalid,
        siSHL_Data_tready         => siSHL_Nts_Tcp_Data_tready,
        ---- TCP Metadata Stream 
        siSHL_Meta_V_V_tdata      => siSHL_Nts_Tcp_Meta_tdata,
        siSHL_Meta_V_V_tvalid     => siSHL_Nts_Tcp_Meta_tvalid,
        siSHL_Meta_V_V_tready     => siSHL_Nts_Tcp_Meta_tready,
        ------------------------------------------------------
        -- SHELL / RxP Ctlr Flow Interfaces
        ------------------------------------------------------
        -- FPGA Receive Path (SHELL-->APP) ------- :
        ---- TCP Listen Request Stream 
        soSHL_LsnReq_V_V_tdata    => soSHL_Nts_Tcp_LsnReq_tdata,
        soSHL_LsnReq_V_V_tvalid   => soSHL_Nts_Tcp_LsnReq_tvalid,
        soSHL_LsnReq_V_V_tready   => soSHL_Nts_Tcp_LsnReq_tready,
        ---- TCP Listen Stream 
        siSHL_LsnRep_V_tdata      => siSHL_Nts_Tcp_LsnRep_tdata,     
        siSHL_LsnRep_V_tvalid     => siSHL_Nts_Tcp_LsnRep_tvalid, 
        siSHL_LsnRep_V_tready     => siSHL_Nts_Tcp_LsnRep_tready,
        ------------------------------------------------------
        -- SHELL / TxP Data Flow Interfaces
        ------------------------------------------------------
        ---- TCP Data Stream 
        soSHL_Data_tdata          => soSHL_Nts_Tcp_Data_tdata, 
        soSHL_Data_tkeep          => soSHL_Nts_Tcp_Data_tkeep, 
        soSHL_Data_tlast          => soSHL_Nts_Tcp_Data_tlast, 
        soSHL_Data_tvalid         => soSHL_Nts_Tcp_Data_tvalid,
        soSHL_Data_tready         => soSHL_Nts_Tcp_Data_tready,
        ---- TCP Send Request 
        soSHL_SndReq_V_tdata      => soSHL_Nts_Tcp_SndReq_tdata,  
        soSHL_SndReq_V_tvalid     => soSHL_Nts_Tcp_SndReq_tvalid,
        soSHL_SndReq_V_tready     => soSHL_Nts_Tcp_SndReq_tready,
        ---- TCP Send Reply
        siSHL_SndRep_V_tdata      => siSHL_Nts_Tcp_SndRep_tdata,
        siSHL_SndRep_V_tvalid     => siSHL_Nts_Tcp_SndRep_tvalid,
        siSHL_SndRep_V_tready     => siSHL_Nts_Tcp_SndRep_tready,
        ------------------------------------------------------
        -- SHELL / TxP Ctlr Flow Interfaces
        ------------------------------------------------------
        -- FPGA Transmit Path (APP-->SHELL) ------
        ---- TCP Open Session Request Stream 
        soSHL_OpnReq_V_tdata      => soSHL_Nts_Tcp_OpnReq_tdata,
        soSHL_OpnReq_V_tvalid     => soSHL_Nts_Tcp_OpnReq_tvalid,
        soSHL_OpnReq_V_tready     => soSHL_Nts_Tcp_OpnReq_tready,
        ---- TCP Open Session Status Stream  
        siSHL_OpnRep_V_tdata      => siSHL_Nts_Tcp_OpnRep_tdata,  
        siSHL_OpnRep_V_tvalid     => siSHL_Nts_Tcp_OpnRep_tvalid,
        siSHL_OpnRep_V_tready     => siSHL_Nts_Tcp_OpnRep_tready,
        ---- TCP Close Request Stream 
        soSHL_ClsReq_V_V_tdata    => soSHL_Nts_Tcp_ClsReq_tdata,
        soSHL_ClsReq_V_V_tvalid   => soSHL_Nts_Tcp_ClsReq_tvalid,
        soSHL_ClsReq_V_V_tready   => soSHL_Nts_Tcp_ClsReq_tready,
        ------------------------------------------------------
        -- DEBUG Interfaces
        ------------------------------------------------------
        ---- Sink Counter Stream
        soDBG_SinkCnt_V_V_tdata      => ssTSIF_ARS_SinkCnt_tdata,
        soDBG_SinkCnt_V_V_tvalid     => ssTSIF_ARS_SinkCnt_tvalid,
        soDBG_SinkCnt_V_V_tready     => ssTSIF_ARS_SinkCnt_tready,
        ---- Input Buffer Space Stream
        soDBG_InpBufSpace_V_V_tdata  => sTSIF_DBG_InpBufSpace,
        soDBG_InpBufSpace_V_V_tvalid => open,
        soDBG_InpBufSpace_V_V_tready => '1'
      ); -- End of: TcpShellInterface
  end generate;
  
  --###############################################################################
  --#                                                                             #
  --#    #######    ####   ######     ###                                         #
  --#       #      #       #     #   #   #  ####   ####                           #
  --#       #     #        #     #   #   #  #   # #                               #
  --#       #     #        ######    #####  ####  #####                           #
  --#       #      #       #         #   #  # #       #                           #
  --#       #       ####   #         #   #  #  #  ####                            #
  --#                                                                             #
  --###############################################################################
  gArsTcp : if gVivadoVersion /= 2016 generate
    ARS_TCP_RX_DATA   : AxisRegisterSlice_64_8_1
      port map (
        aclk          => piSHL_156_25Clk,
        aresetn       => not piSHL_Mmio_Ly7Rst,
        s_axis_tdata  => ssTSIF_TARS_Data_tdata,
        s_axis_tkeep  => ssTSIF_TARS_Data_tkeep,
        s_axis_tlast  => ssTSIF_TARS_Data_tlast,
        s_axis_tvalid => ssTSIF_TARS_Data_tvalid,
        s_axis_tready => ssTSIF_TARS_Data_tready,
        --
        m_axis_tdata  => ssTARS_TAF_Data_tdata,
        m_axis_tkeep  => ssTARS_TAF_Data_tkeep,
        m_axis_tlast  => ssTARS_TAF_Data_tlast,
        m_axis_tvalid => ssTARS_TAF_Data_tvalid,
        m_axis_tready => ssTARS_TAF_Data_tready
      );
    ARS_TCP_RX_SESSID : AxisRegisterSlice_16
      port map (
        aclk          => piSHL_156_25Clk,
        aresetn       => not piSHL_Mmio_Ly7Rst,
        s_axis_tdata  => ssTSIF_TARS_SessId_tdata,
        s_axis_tvalid => ssTSIF_TARS_SessId_tvalid,
        s_axis_tready => ssTSIF_TARS_SessId_tready,
        --
        m_axis_tdata  => ssTARS_TAF_SessId_tdata, 
        m_axis_tvalid => ssTARS_TAF_SessId_tvalid,
        m_axis_tready => ssTARS_TAF_SessId_tready
      );
    ARS_TCP_RX_DATLEN : AxisRegisterSlice_16
      port map (
        aclk          => piSHL_156_25Clk,
        aresetn       => not piSHL_Mmio_Ly7Rst,
        s_axis_tdata  => ssTSIF_TARS_DatLen_tdata,
        s_axis_tvalid => ssTSIF_TARS_DatLen_tvalid,
        s_axis_tready => ssTSIF_TARS_DatLen_tready,
        --
        m_axis_tdata  => ssTARS_TAF_DatLen_tdata, 
        m_axis_tvalid => ssTARS_TAF_DatLen_tvalid,
        m_axis_tready => ssTARS_TAF_DatLen_tready
      );
    --
    ARS_TCP_TX_DATA   : AxisRegisterSlice_64_8_1
      port map (
        aclk          => piSHL_156_25Clk,
        aresetn       => not piSHL_Mmio_Ly7Rst,
        s_axis_tdata  => ssTAF_TARS_Data_tdata,
        s_axis_tkeep  => ssTAF_TARS_Data_tkeep,
        s_axis_tlast  => ssTAF_TARS_Data_tlast,
        s_axis_tvalid => ssTAF_TARS_Data_tvalid,
        s_axis_tready => ssTAF_TARS_Data_tready,
        --
        m_axis_tdata  => ssTARS_TSIF_Data_tdata,
        m_axis_tkeep  => ssTARS_TSIF_Data_tkeep,
        m_axis_tlast  => ssTARS_TSIF_Data_tlast,
        m_axis_tvalid => ssTARS_TSIF_Data_tvalid,
        m_axis_tready => ssTARS_TSIF_Data_tready
      );
    ARS_TCP_TX_SESSID : AxisRegisterSlice_16
      port map (
        aclk          => piSHL_156_25Clk,
        aresetn       => not piSHL_Mmio_Ly7Rst,
        s_axis_tdata  => ssTAF_TARS_SessId_tdata,
        s_axis_tvalid => ssTAF_TARS_SessId_tvalid,
        s_axis_tready => ssTAF_TARS_SessId_tready,
        --
        m_axis_tdata  => ssTARS_TSIF_SessId_tdata, 
        m_axis_tvalid => ssTARS_TSIF_SessId_tvalid,
        m_axis_tready => ssTARS_TSIF_SessId_tready
      );
    ARS_TCP_TX_DATLEN : AxisRegisterSlice_16
      port map (
        aclk          => piSHL_156_25Clk,
        aresetn       => not piSHL_Mmio_Ly7Rst,
        s_axis_tdata  => ssTAF_TARS_DatLen_tdata,
        s_axis_tvalid => ssTAF_TARS_DatLen_tvalid,
        s_axis_tready => ssTAF_TARS_DatLen_tready,
        --
        m_axis_tdata  => ssTARS_TSIF_DatLen_tdata, 
        m_axis_tvalid => ssTARS_TSIF_DatLen_tvalid,
        m_axis_tready => ssTARS_TSIF_DatLen_tready
      );
    -- 
    ARS_TCP_DBG_SINK_CNT : AxisRegisterSlice_32
      port map (
        aclk          => piSHL_156_25Clk,
        aresetn       => not piSHL_Mmio_Ly7Rst,
        s_axis_tdata  => ssTSIF_ARS_SinkCnt_tdata,
        s_axis_tvalid => ssTSIF_ARS_SinkCnt_tvalid,
        s_axis_tready => ssTSIF_ARS_SinkCnt_tready,
        --
        m_axis_tdata  => sTSIF_DBG_SinkCnt,
        m_axis_tvalid => open,
        m_axis_tready => '1'
      );
  end generate;
    
  --################################################################################
  --#                                                                              #
  --#    #######    ####   ######     #####                                        #
  --#       #      #       #     #   #     # #####   #####                         #
  --#       #     #        #     #   #     # #    #  #    #                        #
  --#       #     #        ######    ####### #####   #####                         #
  --#       #      #       #         #     # #       #                             #
  --#       #       ####   #         #     # #       #                             #
  --#                                                                              #
  --################################################################################
  
  --==========================================================================
  --==  INST: TCP-APPLICATION_FLASH (TAF) for cFp_HelloKale
  --==   This application implements a set of TCP-oriented tests. The [TAF]
  --==   connects to the SHELL via a TCP Shell Interface (TSIF) block. The
  --==   main purpose of the [TSIF] is to provide a placeholder for the 
  --==   opening of one or multiple listening port(s). The use of the [TSIF] is
  --==   not a prerequisite, but it is provided here for sake of simplicity.
  --==========================================================================
  gTcpAppFlash : if gVivadoVersion = 2016 generate
    TAF : TcpApplicationFlash_Deprecated
      port map (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        aclk                 => piSHL_156_25Clk,
        aresetn              => not piSHL_Mmio_Ly7Rst,
        -------------------- ------------------------------------
        -- From SHELL / Mmio  Interfaces
        -------------------- ------------------------------------
        --[NOT_USED] piSHL_MmioEchoCtrl_V   => piSHL_Mmio_TcpEchoCtrl,
        --[NOT_USED] piSHL_MmioPostSegEn_V  => piSHL_Mmio_TcpPostSegEn,
        --[NOT_USED] piSHL_MmioCaptSegEn_V  => piSHL_Mmio_TcpCaptSegEn,
        --------------------- -----------------------------------
        -- From TSIF (via TARS) / Tcp Rx Data Interfaces
        --------------------- -----------------------------------
        siTSIF_Data_tdata    => ssTARS_TAF_Data_tdata,
        siTSIF_Data_tkeep    => ssTARS_TAF_Data_tkeep,
        siTSIF_Data_tlast    => ssTARS_TAF_Data_tlast,
        siTSIF_Data_tvalid   => ssTARS_TAF_Data_tvalid,
        siTSIF_Data_tready   => ssTARS_TAF_Data_tready,
        --
        siTSIF_SessId_tdata  => ssTARS_TAF_SessId_tdata,
        siTSIF_SessId_tvalid => ssTARS_TAF_SessId_tvalid,
        siTSIF_SessId_tready => ssTARS_TAF_SessId_tready,
        --TSIF
        siTSIF_DatLen_tdata  => ssTARS_TAF_DatLen_tdata,
        siTSIF_DatLen_tvalid => ssTARS_TAF_DatLen_tvalid,
        siTSIF_DatLen_tready => ssTARS_TAF_DatLen_tready,
        --------------------- -----------------------------------
        -- To TSIF (via TARS) / Tcp Tx Data Interfaces
        --------------------- -----------------------------------
        soTSIF_Data_tdata    => ssTAF_TARS_Data_tdata,
        soTSIF_Data_tkeep    => ssTAF_TARS_Data_tkeep,
        soTSIF_Data_tlast    => ssTAF_TARS_Data_tlast,
        soTSIF_Data_tvalid   => ssTAF_TARS_Data_tvalid,
        soTSIF_Data_tready   => ssTAF_TARS_Data_tready,
        --
        soTSIF_SessId_tdata  => ssTAF_TARS_SessId_tdata,
        soTSIF_SessId_tvalid => ssTAF_TARS_SessId_tvalid,
        soTSIF_SessId_tready => ssTAF_TARS_SessId_tready,
        --
        soTSIF_DatLen_tdata  => ssTAF_TARS_DatLen_tdata,
        soTSIF_DatLen_tvalid => ssTAF_TARS_DatLen_tvalid,
        soTSIF_DatLen_tready => ssTAF_TARS_DatLen_tready
      );  
  else generate
    TAF : TcpApplicationFlash
      port map (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        ap_clk                  => piSHL_156_25Clk,
        ap_rst_n                => not (piSHL_Mmio_Ly7Rst),
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------       
        --[NOT_USED] piSHL_MmioEchoCtrl_V  => piSHL_Mmio_TcpEchoCtrl,
        --[NOT_USED] piSHL_MmioPostSegEn_V => piSHL_Mmio_TcpPostSegEn,
        --[NOT_USED] piSHL_MmioCaptSegEn   => piSHL_Mmio_TcpCaptSegEn,
        --------------------------------------------------------
        -- From TSIF (via TARS) / Tcp Rx Data Interfaces
        --------------------------------------------------------
        siTSIF_Data_tdata        => ssTARS_TAF_Data_tdata,
        siTSIF_Data_tkeep        => ssTARS_TAF_Data_tkeep,
        siTSIF_Data_tlast        => ssTARS_TAF_Data_tlast,
        siTSIF_Data_tvalid       => ssTARS_TAF_Data_tvalid,
        siTSIF_Data_tready       => ssTARS_TAF_Data_tready,
        --
        siTSIF_SessId_V_V_tdata  => ssTARS_TAF_SessId_tdata,
        siTSIF_SessId_V_V_tvalid => ssTARS_TAF_SessId_tvalid,
        siTSIF_SessId_V_V_tready => ssTARS_TAF_SessId_tready,
        --
        siTSIF_DatLen_V_V_tdata  => ssTARS_TAF_DatLen_tdata,
        siTSIF_DatLen_V_V_tvalid => ssTARS_TAF_DatLen_tvalid,
        siTSIF_DatLen_V_V_tready => ssTARS_TAF_DatLen_tready,
        --------------------------------------------------------
        -- To TSIF (via TARS) / Tcp Tx Data Interfaces
        --------------------------------------------------------
        soTSIF_Data_tdata        => ssTAF_TARS_Data_tdata,
        soTSIF_Data_tkeep        => ssTAF_TARS_Data_tkeep,
        soTSIF_Data_tlast        => ssTAF_TARS_Data_tlast,
        soTSIF_Data_tvalid       => ssTAF_TARS_Data_tvalid,
        soTSIF_Data_tready       => ssTAF_TARS_Data_tready,
        --
        soTSIF_SessId_V_V_tdata  => ssTAF_TARS_SessId_tdata,
        soTSIF_SessId_V_V_tvalid => ssTAF_TARS_SessId_tvalid,
        soTSIF_SessId_V_V_tready => ssTAF_TARS_SessId_tready,
        --
        soTSIF_DatLen_V_V_tdata  => ssTAF_TARS_DatLen_tdata,
        soTSIF_DatLen_V_V_tvalid => ssTAF_TARS_DatLen_tvalid,
        soTSIF_DatLen_V_V_tready => ssTAF_TARS_DatLen_tready
      );
  end generate;

  -- ========================================================================
  -- == Generation of a delayed reset for the MemTest core
  -- ==  [TODO: Can we get ret rid of this reset]
  -- ========================================================================
  process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      if piSHL_156_25Rst = '1' then
        s156_25Rst_delayed <= '0';
        sRstDelayCounter <= (others => '0');
      else
       if unsigned(sRstDelayCounter) <= 20 then 
          s156_25Rst_delayed <= '1';
          sRstDelayCounter <= std_logic_vector(unsigned(sRstDelayCounter) + 1);
       else
          s156_25Rst_delayed <= '0';
        end if;
      end if;
    end if;
  end process;


  --################################################################################
  --#                                                                              #
  --#    #    #  ######  #    #  ######                         #####    ####      #
  --#    ##  ##  #       ##  ##    #    ###### ###### ######    #    #  #   ##     #
  --#    # ## #  #####   # ## #    #    #      #        #       #####   #  # #     #
  --#    #    #  #       #    #    #    ####   ######   #       #       # #  #     #
  --#    #    #  #       #    #    #    #           #   #       #       ##   #     #
  --#    #    #  ######  #    #    #    ###### ######   #       #        ####      #
  --#                                                                              #
  --################################################################################

  MEM_TEST: MemTestFlash
    port map(
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                     => piSHL_156_25Clk,
      ap_rst_n                   => not piSHL_Mmio_Ly7Rst,
      ------------------------------------------------------
      -- BLock-Level I/O Protocol
      ------------------------------------------------------
      ap_start                   => '1',
      ap_done                    => open,
      ap_idle                    => open,
      ap_ready                   => open,
      ------------------------------------------------------
      -- From ROLE / Delayed Reset
      ------------------------------------------------------
      piSysReset_V               => fVectorize(s156_25Rst_delayed),
      piSysReset_V_ap_vld        => '1',
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piMMIO_diag_ctrl_V         => piSHL_Mmio_Mc1_MemTestCtrl,
      piMMIO_diag_ctrl_V_ap_vld  => '1',
      poMMIO_diag_stat_V         => poSHL_Mmio_Mc1_MemTestStat,
      poDebug_V                  => poSHL_Mmio_RdReg,
      --------------------------------------------------------
      -- SHELL / Mem / Mp0 Interface
      --------------------------------------------------------
      ---- Stream Read Command ---------
      soMemRdCmdP0_TDATA         => soSHL_Mem_Mp0_RdCmd_tdata,
      soMemRdCmdP0_TVALID        => soSHL_Mem_Mp0_RdCmd_tvalid,
      soMemRdCmdP0_TREADY        => soSHL_Mem_Mp0_RdCmd_tready,
      ---- Stream Read Status ----------
      siMemRdStsP0_TDATA         => siSHL_Mem_Mp0_RdSts_tdata,
      siMemRdStsP0_TVALID        => siSHL_Mem_Mp0_RdSts_tvalid,
      siMemRdStsP0_TREADY        => siSHL_Mem_Mp0_RdSts_tready,
      ---- Stream Read Data ------------    
      siMemReadP0_TDATA          => siSHL_Mem_Mp0_Read_tdata,
      siMemReadP0_TVALID         => siSHL_Mem_Mp0_Read_tvalid,
      siMemReadP0_TREADY         => siSHL_Mem_Mp0_Read_tready,
      siMemReadP0_TKEEP          => siSHL_Mem_Mp0_Read_tkeep,
      siMemReadP0_TLAST          => fVectorize(siSHL_Mem_Mp0_Read_tlast),
      ---- Stream Write Command --------     
      soMemWrCmdP0_TDATA         => soSHL_Mem_Mp0_WrCmd_tdata,
      soMemWrCmdP0_TVALID        => soSHL_Mem_Mp0_WrCmd_tvalid,
      soMemWrCmdP0_TREADY        => soSHL_Mem_Mp0_WrCmd_tready,
      ---- Stream Write Status ---------
      siMemWrStsP0_TDATA         => siSHL_Mem_Mp0_WrSts_tdata,
      siMemWrStsP0_TVALID        => siSHL_Mem_Mp0_WrSts_tvalid,
      siMemWrStsP0_TREADY        => siSHL_Mem_Mp0_WrSts_tready,
      ---- Stream Write Data ---------
      soMemWriteP0_TDATA         => soSHL_Mem_Mp0_Write_tdata,
      soMemWriteP0_TVALID        => soSHL_Mem_Mp0_Write_tvalid,
      soMemWriteP0_TREADY        => soSHL_Mem_Mp0_Write_tready,
      soMemWriteP0_TKEEP         => soSHL_Mem_Mp0_Write_tkeep,
      soMemWriteP0_TLAST         => sSHL_Mem_Mp0_Write_tlast
    ); -- End-of: MemTestFlash
    
    soSHL_Mem_Mp0_Write_tlast <= fScalarize(sSHL_Mem_Mp0_Write_tlast);
    
    --################################################################################
    --#                                                                              #
    --#    #    #  ######  #    #  ######                         #####     #        #
    --#    ##  ##  #       ##  ##    #    ###### ###### ######    #    #   ##        #
    --#    # ## #  #####   # ## #    #    #      #        #       #####   # #        #
    --#    #    #  #       #    #    #    ####   ######   #       #         #        #
    --#    #    #  #       #    #    #    #           #   #       #         #        #
    --#    #    #  ######  #    #    #    ###### ######   #       #       #####      #
    --#                                                                              #
    --################################################################################
    
    --------------------------------------------------------
    -- SHELL / Mem / Mp1 Interface
    --------------------------------------------------------
    ---- Write Address Channel -------------
    moSHL_Mem_Mp1_AWID    <= (others => '0');
    moSHL_Mem_Mp1_AWADDR  <= (others => '0');
    moSHL_Mem_Mp1_AWLEN   <= (others => '0');
    moSHL_Mem_Mp1_AWSIZE  <= (others => '0');
    moSHL_Mem_Mp1_AWBURST <= (others => '0');
    moSHL_Mem_Mp1_AWVALID <= '0'            ;
    ---- Write Data Channel ----------------
    moSHL_Mem_Mp1_WDATA   <= (others => '0');
    moSHL_Mem_Mp1_WSTRB   <= (others => '0');
    moSHL_Mem_Mp1_WLAST   <= '0'            ; 
    moSHL_Mem_Mp1_WVALID  <= '0'            ;
    ---- Write Response Channel ------------
    moSHL_Mem_Mp1_BREADY  <= '0'            ;
    ---- Read Address Channel --------------
    moSHL_Mem_Mp1_ARID    <= (others => '0');
    moSHL_Mem_Mp1_ARADDR  <= (others => '0');
    moSHL_Mem_Mp1_ARLEN   <= (others => '0');
    moSHL_Mem_Mp1_ARSIZE  <= (others => '0');
    moSHL_Mem_Mp1_ARBURST <= (others => '0');
    moSHL_Mem_Mp1_ARVALID   <= '0'          ;
    ---- Read Data Channel -----------------
    moSHL_Mem_Mp1_RREADY    <= '0'          ;
    
end architecture BringUp;

