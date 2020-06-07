-- *****************************************************************************
-- *
-- *                             cloudFPGA
-- *            All rights reserved -- Property of IBM
-- *
-- *----------------------------------------------------------------------------
-- *
-- * Title : Role for the bring-up of the FMKU2595 when equipped with a XCKU060.
-- *
-- * File    : Role.vhdl
-- *
-- * Created : Feb 2018
-- * Authors : Francois Abel <fab@zurich.ibm.com>
-- *           Beat Weiss <wei@zurich.ibm.com>
-- *           Burkhard Ringlein <ngl@zurich.ibm.com>
-- *
-- * Devices : xcku060-ffva1156-2-i
-- * Tools   : Vivado v2016.4, 2017.4 (64-bit)
-- * Depends : None
-- *
-- * Description : In cloudFPGA, the user application is referred to as a 'ROLE'    
-- *    and is integrated along with a 'SHELL' that abstracts the HW components
-- *    of the FPGA module. 
-- *    The current ROLE implements a set of TCP, UDP and DDR4 tests for the  
-- *    bring-up of the FPGA module FMKU60. This ROLE is typically paired with
-- *    SHELL 'Kale' by the cloudFPGA project 'cFp_BringUp'.
-- *
-- *****************************************************************************

--******************************************************************************
--**  CONTEXT CLAUSE  **  FMKU60 ROLE(Flash)
--******************************************************************************
library IEEE;
use     IEEE.std_logic_1164.all;
use     IEEE.numeric_std.all;

library UNISIM; 
use     UNISIM.vcomponents.all;

--******************************************************************************
--**  ENTITY  **  FMKU60 ROLE
--******************************************************************************

entity Role_Kale is
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
    ---- Axi4-Stream TCP Metadata -----------
    soSHL_Nts_Tcp_Meta_tdata            : out   std_ulogic_vector( 15 downto 0);
    soSHL_Nts_Tcp_Meta_tvalid           : out   std_ulogic;
    soSHL_Nts_Tcp_Meta_tready           : in    std_ulogic;
    ---- Axi4-Stream TCP Data Status --------
    siSHL_Nts_Tcp_DSts_tdata            : in    std_ulogic_vector( 23 downto 0);
    siSHL_Nts_Tcp_DSts_tvalid           : in    std_ulogic;
    siSHL_Nts_Tcp_DSts_tready           : out   std_ulogic;
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
    ----  Axi4-Stream TCP Listen Ack --------
    siSHL_Nts_Tcp_LsnAck_tdata          : in    std_ulogic_vector(  7 downto 0); 
    siSHL_Nts_Tcp_LsnAck_tvalid         : in    std_ulogic;
    siSHL_Nts_Tcp_LsnAck_tready         : out   std_ulogic;
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
    moSHL_Mem_Mp1_AWID                  : out   std_ulogic_vector(  3 downto 0);
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
    moSHL_Mem_Mp1_BID                   : in    std_ulogic_vector(  3 downto 0);
    moSHL_Mem_Mp1_BRESP                 : in    std_ulogic_vector(  1 downto 0);
    moSHL_Mem_Mp1_BVALID                : in    std_ulogic;
    moSHL_Mem_Mp1_BREADY                : out   std_ulogic;
    ---- Read Address Channel ----------
    moSHL_Mem_Mp1_ARID                  : out   std_ulogic_vector(  3 downto 0);
    moSHL_Mem_Mp1_ARADDR                : out   std_ulogic_vector( 32 downto 0);
    moSHL_Mem_Mp1_ARLEN                 : out   std_ulogic_vector(  7 downto 0);
    moSHL_Mem_Mp1_ARSIZE                : out   std_ulogic_vector(  2 downto 0);
    moSHL_Mem_Mp1_ARBURST               : out   std_ulogic_vector(  1 downto 0);
    moSHL_Mem_Mp1_ARVALID               : out   std_ulogic;
    moSHL_Mem_Mp1_ARREADY               : in    std_ulogic;
    ---- Read Data Channel -------------
    moSHL_Mem_Mp1_RID                   : in    std_ulogic_vector(  3 downto 0);
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
    piSHL_Mmio_UdpEchoCtrl              : in    std_ulogic_vector(  1 downto 0);
    piSHL_Mmio_UdpPostDgmEn             : in    std_ulogic;
    piSHL_Mmio_UdpCaptDgmEn             : in    std_ulogic;
    piSHL_Mmio_TcpEchoCtrl              : in    std_ulogic_vector(  1 downto 0);
    piSHL_Mmio_TcpPostSegEn             : in    std_ulogic;
    piSHL_Mmio_TcpCaptSegEn             : in    std_ulogic;
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
-- **  ARCHITECTURE  **  FLASH of ROLE 
-- *****************************************************************************

architecture Flash of Role_Kale is

  constant cTCP_APP_DEPRECATED_DIRECTIVES  : boolean := true;
  constant cUDP_APP_DEPRECATED_DIRECTIVES  : boolean := true;
  constant cTCP_SIF_DEPRECATED_DIRECTIVES  : boolean := true;
  constant cUDP_SIF_DEPRECATED_DIRECTIVES  : boolean := true;

  --============================================================================
  --  SIGNAL DECLARATIONS
  --============================================================================  
  -- Delayed reset signal and counter 
  signal s156_25Rst_delayed          : std_ulogic;
  signal sRstDelayCounter            : std_ulogic_vector(5 downto 0);

  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : TSIF <--> TAF 
  --------------------------------------------------------
  -- Session Connect Id Interface
  signal sTSIF_TAF_SessConId        : std_ulogic_vector( 15 downto 0);
  -- TCP Receive Path (TSIF->TAF) ------
  ---- Stream TCP Data -------
  signal ssTSIF_TAF_Data_tdata      : std_ulogic_vector( 63 downto 0);
  signal ssTSIF_TAF_Data_tkeep      : std_ulogic_vector(  7 downto 0);
  signal ssTSIF_TAF_Data_tlast      : std_ulogic;
  signal ssTSIF_TAF_Data_tvalid     : std_ulogic;
  signal ssTSIF_TAF_Data_tready     : std_ulogic;
  ---- Stream TCP Metadata -
  signal ssTSIF_TAF_Meta_tdata      : std_ulogic_vector( 15 downto 0);
  signal ssTSIF_TAF_Meta_tvalid     : std_ulogic;
  signal ssTSIF_TAF_Meta_tready     : std_ulogic;
  -- TCP Transmit Path (TAF-->TSIF) ----
  ---- Stream TCP Data -------
  signal ssTAF_TSIF_Data_tdata      : std_ulogic_vector( 63 downto 0);
  signal ssTAF_TSIF_Data_tkeep      : std_ulogic_vector(  7 downto 0);
  signal ssTAF_TSIF_Data_tlast      : std_ulogic;
  signal ssTAF_TSIF_Data_tvalid     : std_ulogic;
  signal ssTAF_TSIF_Data_tready     : std_ulogic;
  ---- Stream TCP Metadata --
  signal ssTAF_TSIF_Meta_tdata      : std_ulogic_vector( 15 downto 0);
  signal ssTAF_TSIF_Meta_tvalid     : std_ulogic;
  signal ssTAF_TSIF_Meta_tready     : std_ulogic;
 
  signal sSHL_Mem_Mp0_Write_tlast   : std_ulogic_vector(0 downto 0);
  
  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : USIF <--> UAF 
  --------------------------------------------------------
  -- UAF<->USIF / UDP Control Port Interfaces
  --signal ssUAF_USIF_LsnReq_tdata    : std_logic_vector(15 downto 0);
  --signal ssUAF_USIF_LsnReq_tvalid   : std_logic;
  --signal ssUAF_USIF_LsnReq_tready   : std_logic;
  --
  --signal ssUSIF_UAF_LsnRep_tdata    : std_logic_vector( 7 downto 0);
  --signal ssUSIF_UAF_LsnRep_tvalid   : std_logic;
  --signal ssUSIF_UAF_LsnRep_tready   : std_logic;
  --
  --signal ssUAF_USIF_ClsReq_tdata    : std_logic_vector(15 downto 0);
  --signal ssUAF_USIF_ClsReq_tvalid   : std_logic;
  --signal ssUAF_USIF_ClsReq_tready   : std_logic;
  --
  --signal ssUSIF_UAF_ClsRep_tdata    : std_logic_vector( 7 downto 0);
  --signal ssUSIF_UAF_ClsRep_tvalid   : std_logic;
  --signal ssUSIF_UAF_ClsRep_tready   : std_logic;
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

  --============================================================================
  --  VARIABLE DECLARATIONS
  --============================================================================  
   
  --===========================================================================
  --== COMPONENT DECLARATIONS
  --===========================================================================
  component UdpApplicationFlash is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock, Reset
      ------------------------------------------------------
      aclk                    : in  std_logic;
      aresetn                 : in  std_logic; 
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------
      piSHL_Mmio_EchoCtrl_V  : in  std_logic_vector(  1 downto 0);
      --[TODO] piSHL_MmioPostDgmEn_V  : in  std_logic;
      --[TODO] piSHL_MmioCaptDgmEn_V  : in  std_logic;
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
  end component UdpApplicationFlash;
  
  component UdpApplicationFlashTodo is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                    : in  std_logic;
      ap_rst_n                  : in  std_logic;
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piSHL_Mmio_EchoCtrl_V     : in  std_logic_vector(  1 downto 0);
      --[TODO] piSHL_Mmio_PostDgmEn_V : in  std_logic;
      --[TODO] piSHL_Mmio_CaptDgmEn_V : in  std_logic;
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
      soUSIF_DLen_tdata      : out std_logic_vector(95 downto 0);
      soUSIF_DLen_tvalid     : out std_logic;
      soUSIF_DLen_tready     : in  std_logic
    );
  end component UdpApplicationFlashTodo;
  
  component TcpApplicationFlash is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      aclk                  : in  std_logic;
      aresetn               : in  std_logic;    
      ------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      ------------------------------------------------------       
      piSHL_MmioEchoCtrl_V  : in  std_logic_vector(  1 downto 0);
      piSHL_MmioPostSegEn_V : in  std_logic;
      --[TODO] piSHL_MmioCaptSegEn_V  : in  std_logic;      
      ------------------------------------------------------
      -- From TSIF / Session Connect Id Interface
      ------------------------------------------------------
      piTSIF_SConnectId_V   : in  std_logic_vector( 15 downto 0);
      --------------------------------------------------------
      -- From SHELL / Tcp Data Interfaces
      --------------------------------------------------------
      siSHL_Data_tdata      : in  std_logic_vector( 63 downto 0);
      siSHL_Data_tkeep      : in  std_logic_vector(  7 downto 0);
      siSHL_Data_tlast      : in  std_logic;
      siSHL_Data_tvalid     : in  std_logic;
      siSHL_Data_tready     : out std_logic;
      --
      siSHL_SessId_tdata    : in  std_logic_vector( 15 downto 0);
      siSHL_SessId_tvalid   : in  std_logic;
      siSHL_SessId_tready   : out std_logic;
      
      --------------------------------------------------------
      -- To SHELL / Tcp Data Interfaces
      --------------------------------------------------------
      soSHL_Data_tdata      : out std_logic_vector( 63 downto 0);
      soSHL_Data_tkeep      : out std_logic_vector(  7 downto 0);
      soSHL_Data_tlast      : out std_logic;
      soSHL_Data_tvalid     : out std_logic;
      soSHL_Data_tready     : in  std_logic;
      --
      soSHL_SessId_tdata    : out std_logic_vector( 15 downto 0);
      soSHL_SessId_tvalid   : out std_logic;
      soSHL_SessId_tready   : in  std_logic
    );
  end component TcpApplicationFlash;
  
  component UdpShellInterface is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      aclk                  : in  std_logic;
      aresetn               : in  std_logic;
      --------------------------------------------------------
      -- SHELL / Mmio Interface
      --------------------------------------------------------
      piSHL_Mmio_En_V       : in  std_logic;
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
      soUAF_Meta_tready     : in  std_logic
    );
  end component UdpShellInterface;
  
  component TcpApplicationFlashTodo is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                : in  std_logic;
      ap_rst_n              : in  std_logic;
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piSHL_MmioEchoCtrl_V  : in  std_logic_vector(  1 downto 0);
      piSHL_MmioPostSegEn_V : in  std_logic;
      --[TODO] piSHL_MmioCaptSegEn  : in  std_logic;
      ------------------------------------------------------
      -- From TSIF / Session Connect Id Interface
      ------------------------------------------------------
      piTSIF_SConnectId_V   : in  std_logic_vector( 15 downto 0);
      --------------------------------------------------------
      -- From SHELL / Tcp Data Interfaces
      --------------------------------------------------------
      siSHL_Data_tdata      : in  std_logic_vector( 63 downto 0);
      siSHL_Data_tkeep      : in  std_logic_vector(  7 downto 0);
      siSHL_Data_tlast      : in  std_logic;
      siSHL_Data_tvalid     : in  std_logic;
      siSHL_Data_tready     : out std_logic;
      --
      siSHL_SessId_tdata    : in  std_logic_vector( 15 downto 0);
      siSHL_SessId_tkeep    : in  std_logic_vector(  1 downto 0);
      siSHL_SessId_tlast    : in  std_logic;
      siSHL_SessId_tvalid   : in  std_logic;
      siSHL_SessId_tready   : out std_logic;
      --------------------------------------------------------
      -- To SHELL / Tcp Data Interfaces
      --------------------------------------------------------
      soSHL_Data_tdata      : out std_logic_vector( 63 downto 0);
      soSHL_Data_tkeep      : out std_logic_vector(  7 downto 0);
      soSHL_Data_tlast      : out std_logic;
      soSHL_Data_tvalid     : out std_logic;
      soSHL_Data_tready     : in  std_logic;
      --
      soSHL_SessId_tdata    : out std_logic_vector( 15 downto 0);
      soSHL_SessId_tkeep    : out std_logic_vector(  1 downto 0);
      soSHL_SessId_tlast    : out std_logic;
      soSHL_SessId_tvalid   : out std_logic;
      soSHL_SessId_tready   : in  std_logic;
      ------------------------------------------------------
      -- ROLE / Session Connect Id Interface
      ------------------------------------------------------
      poROLE_SConId_V       : out std_ulogic_vector( 15 downto 0)
    );
  end component TcpApplicationFlashTodo;
 
  component TcpShellInterface is
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
      -- ROLE / TxP Data Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (ROLE-->SHELL) ---------
      ---- TCP Data Stream   
      siTAF_Data_tdata      : in  std_ulogic_vector( 63 downto 0);
      siTAF_Data_tkeep      : in  std_ulogic_vector(  7 downto 0);
      siTAF_Data_tlast      : in  std_ulogic;
      siTAF_Data_tvalid     : in  std_ulogic;
      siTAF_Data_tready     : out std_ulogic;
      ---- TCP Metadata Stream 
      siTAF_SessId_tdata    : in  std_ulogic_vector( 15 downto 0);
      siTAF_SessId_tkeep    : in  std_ulogic_vector(  1 downto 0);
      siTAF_SessId_tlast    : in  std_ulogic;
      siTAF_SessId_tvalid   : in  std_ulogic;
      siTAF_SessId_tready   : out std_ulogic; 
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
      ---- TCP Metadata Stream 
      soTAF_SessId_tdata    : out std_ulogic_vector( 15 downto 0);
      soTAF_SessId_tkeep    : out std_ulogic_vector(  1 downto 0);
      soTAF_SessId_tlast    : out std_ulogic;
      soTAF_SessId_tvalid   : out std_ulogic;
      soTAF_SessId_tready   : in  std_ulogic;
      ------------------------------------------------------
      -- SHELL / RxP Data Interfaces
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
      siSHL_SessId_tdata    : in  std_ulogic_vector( 15 downto 0);
      siSHL_SessId_tkeep    : in  std_ulogic_vector(  1 downto 0);
      siSHL_SessId_tlast    : in  std_ulogic;
      siSHL_SessId_tvalid   : in  std_ulogic;
      siSHL_SessId_tready   : out std_ulogic;
      ------------------------------------------------------
      -- SHELL / RxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Receive Path (SHELL-->APP) -------
      ---- TCP Listen Request Stream 
      soSHL_LsnReq_tdata    : out std_ulogic_vector( 15 downto 0);
      soSHL_LsnReq_tkeep    : out std_ulogic_vector(  1 downto 0);
      soSHL_LsnReq_tlast    : out std_ulogic;
      soSHL_LsnReq_tvalid   : out std_ulogic;
      soSHL_LsnReq_tready   : in  std_ulogic;
      ---- TCP Listen Status Stream 
      siSHL_LsnAck_tdata    : in  std_ulogic_vector(  7 downto 0);
      siSHL_LsnAck_tkeep    : in  std_ulogic;
      siSHL_LsnAck_tlast    : in  std_ulogic;
      siSHL_LsnAck_tvalid   : in  std_ulogic;
      siSHL_LsnAck_tready   : out std_ulogic;
      ------------------------------------------------------
      -- SHELL / TxP Data Interfaces
      ------------------------------------------------------
      ---- TCP Data Stream 
      soSHL_Data_tdata      : out std_ulogic_vector( 63 downto 0);
      soSHL_Data_tkeep      : out std_ulogic_vector(  7 downto 0);
      soSHL_Data_tlast      : out std_ulogic;
      soSHL_Data_tvalid     : out std_ulogic;
      soSHL_Data_tready     : in  std_ulogic;
      ---- TCP Metadata Stream 
      soSHL_SessId_tdata    : out std_ulogic_vector( 15 downto 0);
      soSHL_SessId_tkeep    : out std_ulogic_vector(  1 downto 0);
      soSHL_SessId_tlast    : out std_ulogic;
      soSHL_SessId_tvalid   : out std_ulogic;
      soSHL_SessId_tready   : in  std_ulogic;
      ---- TCP Data Status Stream 
      siSHL_DSts_tdata      : in  std_ulogic_vector( 23 downto 0);
      siSHL_DSts_tvalid     : in  std_ulogic;
      siSHL_DSts_tready     : out std_ulogic;
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
      soSHL_ClsReq_tkeep    : out std_ulogic_vector(  1 downto 0);
      soSHL_ClsReq_tlast    : out std_ulogic;
      soSHL_ClsReq_tvalid   : out std_ulogic;
      soSHL_ClsReq_tready   : in  std_ulogic;
      ------------------------------------------------------
      -- TAF / Session Connect Id Interface
      ------------------------------------------------------
      poTAF_SConId_V        : out std_ulogic_vector( 15 downto 0);
      poTAF_SConId_V_ap_vld : out std_ulogic
    );
  end component TcpShellInterface;
 
  component TcpShellInterfaceTodo is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                : in  std_ulogic;
      ap_rst_n              : in  std_ulogic;
       --------------------------------------------------------
       -- From SHELL / Mmio Interfaces
       --------------------------------------------------------       
       piSHL_Mmio_En_V      : in  std_ulogic;
      ------------------------------------------------------
      -- TAF / TxP Data Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (APP-->SHELL) ---------
      ---- TCP Data Stream 
      siTAF_Data_tdata      : in  std_ulogic_vector( 63 downto 0);
      siTAF_Data_tkeep      : in  std_ulogic_vector(  7 downto 0);
      siTAF_Data_tlast      : in  std_ulogic;
      siTAF_Data_tvalid     : in  std_ulogic;
      siTAF_Data_tready     : out std_ulogic;
      ---- TCP Metadata Stream
      siTAF_SessId_tdata    : in  std_ulogic_vector( 15 downto 0);
      siTAF_SessId_tkeep    : in  std_ulogic_vector(  1 downto 0);
      siTAF_SessId_tlast    : in  std_ulogic;
      siTAF_SessId_tvalid   : in  std_ulogic;
      siTAF_SessId_tready   : out std_ulogic; 
      ------------------------------------------------------               
      -- TAF / RxP Data Flow Interfaces                      
      ------------------------------------------------------               
      -- FPGA Transmit Path (SHELL-->APP) --------                      
      ---- TCP Data Stream 
      soTAF_Data_tdata      : out std_ulogic_vector( 63 downto 0);
      soTAF_Data_tkeep      : out std_ulogic_vector(  7 downto 0);
      soTAF_Data_tlast      : out std_ulogic;
      soTAF_Data_tvalid     : out std_ulogic;
      soTAF_Data_tready     : in  std_ulogic;
      ----  TCP Metadata Stream
      soTAF_SessId_tdata    : out std_ulogic_vector( 15 downto 0);
      soTAF_SessId_tkeep    : out std_ulogic_vector(  1 downto 0);
      soTAF_SessId_tlast    : out std_ulogic;
      soTAF_SessId_tvalid   : out std_ulogic;
      soTAF_SessId_tready   : in  std_ulogic;
      ------------------------------------------------------
      -- SHELL / RxP Data Interfaces
      ------------------------------------------------------
      ---- TCP Data Notification Stream  
      siSHL_Notif_V_tdata   : in  std_ulogic_vector( 87 downto 0);
      siSHL_Notif_V_tvalid  : in  std_ulogic;
      siSHL_Notif_V_tready  : out std_ulogic;
      ---- TCP Data Request Stream 
      soSHL_DReq_V_tdata    : out std_ulogic_vector( 31 downto 0);
      soSHL_DReq_V_tvalid   : out std_ulogic;
      soSHL_DReq_V_tready   : in  std_ulogic;
      ---- TCP Data Stream 
      siSHL_Data_tdata      : in  std_ulogic_vector( 63 downto 0);
      siSHL_Data_tkeep      : in  std_ulogic_vector(  7 downto 0);
      siSHL_Data_tlast      : in  std_ulogic;
      siSHL_Data_tvalid     : in  std_ulogic;
      siSHL_Data_tready     : out std_ulogic;
      ---- TCP Metadata Stream 
      siSHL_SessId_tdata    : in  std_ulogic_vector( 15 downto 0);
      siSHL_SessId_tkeep    : in  std_ulogic_vector(  1 downto 0);
      siSHL_SessId_tlast    : in  std_ulogic;
      siSHL_SessId_tvalid   : in  std_ulogic;
      siSHL_SessId_tready   : out std_ulogic;
      ------------------------------------------------------
      -- SHELL / RxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Receive Path (SHELL-->APP) -------
      ---- TCP Listen Request Stream 
      soSHL_LsnReq_tdata    : out std_ulogic_vector( 15 downto 0);
      soSHL_LsnReq_tkeep    : out std_ulogic_vector(  1 downto 0);
      soSHL_LsnReq_tlast    : out std_ulogic;
      soSHL_LsnReq_tvalid   : out std_ulogic;
      soSHL_LsnReq_tready   : in  std_ulogic;
      ---- TCP Listen Status Stream 
      siSHL_LsnAck_tdata    : in  std_ulogic_vector(  7 downto 0);
      siSHL_LsnAck_tkeep    : in  std_ulogic;
      siSHL_LsnAck_tlast    : in  std_ulogic;
      siSHL_LsnAck_tvalid   : in  std_ulogic;
      siSHL_LsnAck_tready   : out std_ulogic;
      ------------------------------------------------------
      -- SHELL / TxP Data Interfaces
      ------------------------------------------------------
      ---- TCP Data Stream 
      soSHL_Data_tdata      : out std_ulogic_vector( 63 downto 0);
      soSHL_Data_tkeep      : out std_ulogic_vector(  7 downto 0);
      soSHL_Data_tlast      : out std_ulogic;
      soSHL_Data_tvalid     : out std_ulogic;
      soSHL_Data_tready     : in  std_ulogic;
      ---- TCP Metadata Stream 
      soSHL_SessId_tdata    : out std_ulogic_vector( 15 downto 0);
      soSHL_SessId_tkeep    : out std_ulogic_vector(  1 downto 0);
      soSHL_SessId_tlast    : out std_ulogic;
      soSHL_SessId_tvalid   : out std_ulogic;
      soSHL_SessId_tready   : in  std_ulogic;
      ---- TCP Data Status Stream 
      siSHL_DSts_V_tdata    : in  std_ulogic_vector( 23 downto 0);
      siSHL_DSts_V_tvalid   : in  std_ulogic;
      siSHL_DSts_V_tready   : out std_ulogic;
      ------------------------------------------------------
      -- SHELL / TxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (APP-->SHELL) ------
      ---- TCP Open Session Request Stream 
      soSHL_OpnReq_V_tdata  : out std_ulogic_vector( 47 downto 0);
      soSHL_OpnReq_V_tvalid : out std_ulogic;
      soSHL_OpnReq_V_tready : in  std_ulogic;
      ---- TCP Open Session Status Stream  
      siSHL_OpnRep_V_tdata  : in  std_ulogic_vector( 23 downto 0);
      siSHL_OpnRep_V_tvalid : in  std_ulogic;
      siSHL_OpnRep_V_tready : out std_ulogic;
      ---- TCP Close Request Stream 
      soSHL_ClsReq_tdata    : out std_ulogic_vector( 15 downto 0);
      soSHL_ClsReq_tkeep    : out std_ulogic_vector(  1 downto 0);
      soSHL_ClsReq_tlast    : out std_ulogic;
      soSHL_ClsReq_tvalid   : out std_ulogic;
      soSHL_ClsReq_tready   : in  std_ulogic;
      ------------------------------------------------------
      -- TAF / Session Connect Id Interface
      ------------------------------------------------------
      poTAF_SConId_V        : out std_ulogic_vector( 15 downto 0);
      poTAF_SConId_V_ap_vld : out std_ulogic
    );
  end component TcpShellInterfaceTodo;
  
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
  --#    #######  ######  ###  #######                                             #
  --#       #     #        #   #                                                   #
  --#       #     #        #   #                                                   #
  --#       #     ######   #   ####                                                #
  --#       #          #   #   #                                                   #
  --#       #     ######  ###  #                                                   #
  --#                                                                              #
  --################################################################################
  
  gTcpShellInterface : if cTCP_SIF_DEPRECATED_DIRECTIVES = true
    generate
      TSIF : TcpShellInterface
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
          -- TAF / TxP Data Flow Interfaces
          ------------------------------------------------------
          -- FPGA Transmit Path (APP-->SHELL) ---------
          ---- TCP Data Stream -------------
          siTAF_Data_tdata          => ssTAF_TSIF_Data_tdata,
          siTAF_Data_tkeep          => ssTAF_TSIF_Data_tkeep,
          siTAF_Data_tlast          => ssTAF_TSIF_Data_tlast,
          siTAF_Data_tvalid         => ssTAF_TSIF_Data_tvalid,
          siTAF_Data_tready         => ssTAF_TSIF_Data_tready,
          ---- TCP Meta Stream -------------
          siTAF_SessId_tdata        => ssTAF_TSIF_Meta_tdata,
          siTAF_SessId_tkeep        => (others=>'1'), -- [TODO: Until SHELL I/F is compliant]
          siTAF_SessId_tlast        => '1',           -- [TODO: Until SHELL I/F is compliant]
          siTAF_SessId_tvalid       => ssTAF_TSIF_Meta_tvalid,
          siTAF_SessId_tready       => ssTAF_TSIF_Meta_tready,
          ------------------------------------------------------               
          -- TAF / RxP Data Flow Interfaces                      
          ------------------------------------------------------               
          -- FPGA Transmit Path (SHELL-->APP) --------                      
          ---- TCP Data Stream --------------
          soTAF_Data_tdata          => ssTSIF_TAF_Data_tdata,
          soTAF_Data_tkeep          => ssTSIF_TAF_Data_tkeep,
          soTAF_Data_tlast          => ssTSIF_TAF_Data_tlast,
          soTAF_Data_tvalid         => ssTSIF_TAF_Data_tvalid,
          soTAF_Data_tready         => ssTSIF_TAF_Data_tready,
          ---- TCP Meta Stream --------------
          soTAF_SessId_tdata        => ssTSIF_TAF_Meta_tdata,
          soTAF_SessId_tkeep        => open,          -- [TODO: Until SHELL I/F is compliant]
          soTAF_SessId_tlast        => open,          -- [TODO: Until SHELL I/F is compliant]
          soTAF_SessId_tvalid       => ssTSIF_TAF_Meta_tvalid,
          soTAF_SessId_tready       => ssTSIF_TAF_Meta_tready,
          ------------------------------------------------------
          -- SHELL / RxP Data Interfaces
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
          siSHL_SessId_tdata        => siSHL_Nts_Tcp_Meta_tdata,
          siSHL_SessId_tkeep        => (others=>'1'), -- [TODO: Until SHELL I/F is compliant] 
          siSHL_SessId_tlast        => '1',           -- [TODO: Until SHELL I/F is compliant]
          siSHL_SessId_tvalid       => siSHL_Nts_Tcp_Meta_tvalid,
          siSHL_SessId_tready       => siSHL_Nts_Tcp_Meta_tready,
          ------------------------------------------------------
          -- SHELL / RxP Ctlr Flow Interfaces
          ------------------------------------------------------
          -- FPGA Receive Path (SHELL-->APP) --------
          ---- TCP Listen Request Stream -----
          soSHL_LsnReq_tdata        => soSHL_Nts_Tcp_LsnReq_tdata,
          soSHL_LsnReq_tkeep        => open,          -- [TODO: Until SHELL I/F is compliant]
          soSHL_LsnReq_tlast        => open,          -- [TODO: Until SHELL I/F is compliant]
          soSHL_LsnReq_tvalid       => soSHL_Nts_Tcp_LsnReq_tvalid,
          soSHL_LsnReq_tready       => soSHL_Nts_Tcp_LsnReq_tready,
          ---- TCP Listen Ack Stream ---------
          siSHL_LsnAck_tdata        => siSHL_Nts_Tcp_LsnAck_tdata,
          siSHL_LsnAck_tkeep        => '1',           -- [TODO: Until SHELL I/F is compliant] 
          siSHL_LsnAck_tlast        => '1',           -- [TODO: Until SHELL I/F is compliant]
          siSHL_LsnAck_tvalid       => siSHL_Nts_Tcp_LsnAck_tvalid, 
          siSHL_LsnAck_tready       => siSHL_Nts_Tcp_LsnAck_tready, 
          ------------------------------------------------------
          -- SHELL / TxP Data Interfaces
          ------------------------------------------------------
          ---- TCP Data Stream ------------- 
          soSHL_Data_tdata          => soSHL_Nts_Tcp_Data_tdata, 
          soSHL_Data_tkeep          => soSHL_Nts_Tcp_Data_tkeep, 
          soSHL_Data_tlast          => soSHL_Nts_Tcp_Data_tlast, 
          soSHL_Data_tvalid         => soSHL_Nts_Tcp_Data_tvalid,
          soSHL_Data_tready         => soSHL_Nts_Tcp_Data_tready,
          ---- TCP Meta Stream -------------
          soSHL_SessId_tdata        => soSHL_Nts_Tcp_Meta_tdata,
          soSHL_SessId_tkeep        => open,          -- [TODO: Until SHELL I/F is compliant]
          soSHL_SessId_tlast        => open,          -- [TODO: Until SHELL I/F is compliant]
          soSHL_SessId_tvalid       => soSHL_Nts_Tcp_Meta_tvalid,
          soSHL_SessId_tready       => soSHL_Nts_Tcp_Meta_tready,
          ---- TCP Data Status Stream ------
          siSHL_DSts_tdata          => siSHL_Nts_Tcp_DSts_tdata,
          siSHL_DSts_tvalid         => siSHL_Nts_Tcp_DSts_tvalid,
          siSHL_DSts_tready         => siSHL_Nts_Tcp_DSts_tready,  
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
          soSHL_ClsReq_tkeep        => open,        -- [TODO: Until SHELL I/F is compliant] 
          soSHL_ClsReq_tlast        => open,        -- [TODO: Until SHELL I/F is compliant]     
          soSHL_ClsReq_tvalid       => soSHL_Nts_Tcp_ClsReq_tvalid,
          soSHL_ClsReq_tready       => soSHL_Nts_Tcp_ClsReq_tready,
          ------------------------------------------------------
          -- TAF / Session Connect Id Interface
          ------------------------------------------------------
          poTAF_SConId_V           => sTSIF_TAF_SessConId,
          poTAF_SConId_V_ap_vld    => open
        ); -- End of: TcpShellInterface
  else generate
    TSIF : TcpShellInterfaceTodo
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
        -- TAF / TxP Data Flow Interfaces
        ------------------------------------------------------
        -- FPGA Transmit Path (APP-->SHELL) ---------
        ---- TCP Data Stream 
        siTAF_Data_tdata          => ssTAF_TSIF_Data_tdata,
        siTAF_Data_tkeep          => ssTAF_TSIF_Data_tkeep,
        siTAF_Data_tlast          => ssTAF_TSIF_Data_tlast,
        siTAF_Data_tvalid         => ssTAF_TSIF_Data_tvalid,
        siTAF_Data_tready         => ssTAF_TSIF_Data_tready,
        ---- TCP Metadata Stream 
        siTAF_SessId_tdata        => ssTAF_TSIF_Meta_tdata,
        siTAF_SessId_tkeep        => (others=>'1'), -- [TODO: Until SHELL I/F is compliant]
        siTAF_SessId_tlast        => '1',           -- [TODO: Until SHELL I/F is compliant]
        siTAF_SessId_tvalid       => ssTAF_TSIF_Meta_tvalid,
        siTAF_SessId_tready       => ssTAF_TSIF_Meta_tready,
        ------------------------------------------------------               
        -- TAF / RxP Data Flow Interfaces                      
        ------------------------------------------------------               
        -- FPGA Transmit Path (SHELL-->APP) --------                      
        ---- TCP Data Stream 
        soTAF_Data_tdata          => ssTSIF_TAF_Data_tdata,
        soTAF_Data_tkeep          => ssTSIF_TAF_Data_tkeep,
        soTAF_Data_tlast          => ssTSIF_TAF_Data_tlast,
        soTAF_Data_tvalid         => ssTSIF_TAF_Data_tvalid,
        soTAF_Data_tready         => ssTSIF_TAF_Data_tready,
        ---- TCP Metadata Stream 
        soTAF_SessId_tdata        => ssTSIF_TAF_Meta_tdata,
        soTAF_SessId_tkeep        => open,          -- [TODO: Until SHELL I/F is compliant]
        soTAF_SessId_tlast        => open,          -- [TODO: Until SHELL I/F is compliant]
        soTAF_SessId_tvalid       => ssTSIF_TAF_Meta_tvalid,
        soTAF_SessId_tready       => ssTSIF_TAF_Meta_tready,
        ------------------------------------------------------
        -- SHELL / RxP Data Interfaces
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
        siSHL_SessId_tdata        => siSHL_Nts_Tcp_Meta_tdata,
        siSHL_SessId_tkeep        => (others=>'1'), -- [TODO: Until SHELL I/F is compliant] 
        siSHL_SessId_tlast        => '1',           -- [TODO: Until SHELL I/F is compliant]
        siSHL_SessId_tvalid       => siSHL_Nts_Tcp_Meta_tvalid,
        siSHL_SessId_tready       => siSHL_Nts_Tcp_Meta_tready,
        ------------------------------------------------------
        -- SHELL / RxP Ctlr Flow Interfaces
        ------------------------------------------------------
        -- FPGA Receive Path (SHELL-->APP) ------- :
        ---- TCP Listen Request Stream 
        soSHL_LsnReq_tdata        => soSHL_Nts_Tcp_LsnReq_tdata,
        soSHL_LsnReq_tkeep        => open,          -- [TODO: Until SHELL I/F is compliant]
        soSHL_LsnReq_tlast        => open,          -- [TODO: Until SHELL I/F is compliant]
        soSHL_LsnReq_tvalid       => soSHL_Nts_Tcp_LsnReq_tvalid,
        soSHL_LsnReq_tready       => soSHL_Nts_Tcp_LsnReq_tready,
        ---- TCP Listen Stream 
        siSHL_LsnAck_tdata        => siSHL_Nts_Tcp_LsnAck_tdata,
        siSHL_LsnAck_tkeep        => '1',           -- [TODO: Until SHELL I/F is compliant] 
        siSHL_LsnAck_tlast        => '1',           -- [TODO: Until SHELL I/F is compliant]        
        siSHL_LsnAck_tvalid       => siSHL_Nts_Tcp_LsnAck_tvalid, 
        siSHL_LsnAck_tready       => siSHL_Nts_Tcp_LsnAck_tready,
        ------------------------------------------------------
        -- SHELL / TxP Data Interfaces
        ------------------------------------------------------
        ---- TCP Data Stream 
        soSHL_Data_tdata          => soSHL_Nts_Tcp_Data_tdata, 
        soSHL_Data_tkeep          => soSHL_Nts_Tcp_Data_tkeep, 
        soSHL_Data_tlast          => soSHL_Nts_Tcp_Data_tlast, 
        soSHL_Data_tvalid         => soSHL_Nts_Tcp_Data_tvalid,
        soSHL_Data_tready         => soSHL_Nts_Tcp_Data_tready,
        ---- TCP Metadata Stream 
        soSHL_SessId_tdata        => soSHL_Nts_Tcp_Meta_tdata,
        soSHL_SessId_tkeep        => open,          -- [TODO: Until SHELL I/F is compliant]
        soSHL_SessId_tlast        => open,          -- [TODO: Until SHELL I/F is compliant]        
        soSHL_SessId_tvalid       => soSHL_Nts_Tcp_Meta_tvalid,
        soSHL_SessId_tready       => soSHL_Nts_Tcp_Meta_tready,
        ---- TCP Data Status Stream 
        siSHL_DSts_V_tdata        => siSHL_Nts_Tcp_DSts_tdata,
        siSHL_DSts_V_tvalid       => siSHL_Nts_Tcp_DSts_tvalid,
        siSHL_DSts_V_tready       => siSHL_Nts_Tcp_DSts_tready,
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
        soSHL_ClsReq_tdata        => soSHL_Nts_Tcp_ClsReq_tdata,
        soSHL_ClsReq_tkeep        => open,        -- [TODO: Until SHELL I/F is compliant] 
        soSHL_ClsReq_tlast        => open,        -- [TODO: Until SHELL I/F is compliant]        
        soSHL_ClsReq_tvalid       => soSHL_Nts_Tcp_ClsReq_tvalid,
        soSHL_ClsReq_tready       => soSHL_Nts_Tcp_ClsReq_tready,
        ------------------------------------------------------
        -- TAF / Session Connect Id Interface
        ------------------------------------------------------
        poTAF_SConId_V           => sTSIF_TAF_SessConId,
        poTAF_SConId_V_ap_vld    => open
      ); -- End of: TcpShellInterface
  end generate;

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
  gUdpShellInterface : if cUDP_SIF_DEPRECATED_DIRECTIVES = true
    generate
      USIF : UdpShellInterface
        port map (
          ------------------------------------------------------
          -- From SHELL / Clock and Reset
          ------------------------------------------------------
          aclk                    => piSHL_156_25Clk,
          aresetn                 => not piSHL_Mmio_Ly7Rst,
          --------------------------------------------------------
          -- SHELL / Mmio Interface
          --------------------------------------------------------
          piSHL_Mmio_En_V         => piSHL_Mmio_Ly7En,
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
          soUAF_Meta_tready      => ssUSIF_UAF_Meta_tready
        ); -- End-of: UdpShellInterface
  end generate;

  --################################################################################
  --#                                                                              #
  --#    #     #  #####    ######     #####                                        #
  --#    #     #  #    #   #     #   #     # #####   #####                         #
  --#    #     #  #     #  #     #   #     # #    #  #    #                        #
  --#    #     #  #     #  ######    ####### #####   #####                         #
  --#    #     #  #    #   #         #     # #       #                             #
  --#    #######  #####    #         #     # #       #                             #
  --#                                                                              #
  --################################################################################

  gUdpAppFlashDepre : if cUDP_APP_DEPRECATED_DIRECTIVES = true generate
   
    --==========================================================================
    --==  INST: UDP-APPLICATION_FLASH (UAF) for cFp_BringUp
    --==   This application implements a set of UDP-oriented tests. The [UAF]
    --==   connects to the SHELL via a UDP Shell Interface (USIF) block. The
    --==   main purpose of the [USIF] is to provide a placeholder for the 
    --==   opening of one or multiple listening port(s). The us of the [USIF] is
    --==   not a prerequisite, but it is provided here for sake of simplicity.
    --==========================================================================
    UAF : UdpApplicationFlash
      port map (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        aclk                      => piSHL_156_25Clk,
        aresetn                   => not piSHL_Mmio_Ly7Rst,
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------      
        piSHL_Mmio_EchoCtrl_V => piSHL_Mmio_UdpEchoCtrl,
        --[TODO] piSHL_Mmio_PostDgmEn_V  => piSHL_Mmio_UdpPostDgmEn,
        --[TODO] piSHL_Mmio_CaptDgmEn_V  => piSHL_Mmio_UdpCaptDgmEn,
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
 
    --==========================================================================
    --==  INST: UDP-APPLICATION_FLASH for FMKU60
    --==   This version of the 'udp_app_flash' has the following interfaces:
    --==    - one bidirectionnal UDP data stream and one streaming MemoryPort. 
    --==========================================================================
    UAF : UdpApplicationFlashTodo
      port map (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        ap_clk                    => piSHL_156_25Clk,
        ap_rst_n                  => not piSHL_Mmio_Ly7Rst,
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------      
        piSHL_Mmio_EchoCtrl_V => piSHL_Mmio_UdpEchoCtrl,
        --[TODO] piSHL_Mmio_PostDgmEn_V  => piSHL_Mmio_UdpPostDgmEn,
        --[TODO] piSHL_Mmio_CaptDgmEn_V  => piSHL_Mmio_UdpCaptDgmEn,
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

  gTcpAppFlash : if cTCP_APP_DEPRECATED_DIRECTIVES = true generate
    
    --==========================================================================
    --==  INST: UDP-APPLICATION_FLASH for FMKU60
    --==   This version of the 'tcp_app_flash' has the following interfaces:
    --==    - one bidirectionnal TCP data stream and one streaming MemoryPort. 
    --==========================================================================
    TAF : TcpApplicationFlash
      port map (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        aclk                  => piSHL_156_25Clk,
        aresetn               => not piSHL_Mmio_Ly7Rst,  --OBSOLETE not (piSHL_156_25Rst),
        -------------------- ------------------------------------
        -- From SHELL / Mmio  Interfaces
        -------------------- ------------------------------------       
        piSHL_MmioEchoCtrl_V     => piSHL_Mmio_TcpEchoCtrl,
        piSHL_MmioPostSegEn_V    => piSHL_Mmio_TcpPostSegEn,
        --[TODO] piSHL_MmioCaptSegEn_V  => piSHL_Mmio_TcpCaptSegEn,
        ------------------------------------------------------
        -- From TSIF / Session Connect Id Interface
        ------------------------------------------------------
        piTSIF_SConnectId_V      => sTSIF_TAF_SessConId,
        --------------------- -----------------------------------
        -- From SHELL / Tcp Data & Session Id Interfaces
        --------------------- -----------------------------------
        siSHL_Data_tdata      => ssTSIF_TAF_Data_tdata,
        siSHL_Data_tkeep      => ssTSIF_TAF_Data_tkeep,
        siSHL_Data_tlast      => ssTSIF_TAF_Data_tlast,
        siSHL_Data_tvalid     => ssTSIF_TAF_Data_tvalid,
        siSHL_Data_tready     => ssTSIF_TAF_Data_tready,
        --
        siSHL_SessId_tdata    => ssTSIF_TAF_Meta_tdata,
        siSHL_SessId_tvalid   => ssTSIF_TAF_Meta_tvalid,
        siSHL_SessId_tready   => ssTSIF_TAF_Meta_tready,
        --------------------- -----------------------------------
        -- To SHELL / Tcp Data & Session Id Interfaces
        --------------------- -----------------------------------
        soSHL_Data_tdata      => ssTAF_TSIF_Data_tdata,
        soSHL_Data_tkeep      => ssTAF_TSIF_Data_tkeep,
        soSHL_Data_tlast      => ssTAF_TSIF_Data_tlast,
        soSHL_Data_tvalid     => ssTAF_TSIF_Data_tvalid,
        soSHL_Data_tready     => ssTAF_TSIF_Data_tready,
        --
        soSHL_SessId_tdata    => ssTAF_TSIF_Meta_tdata,
        soSHL_SessId_tvalid   => ssTAF_TSIF_Meta_tvalid,
        soSHL_SessId_tready   => ssTAF_TSIF_Meta_tready
      );
    
  else generate

    --==========================================================================
    --==  INST: TCP-APPLICATION_FLASH for FMKU60
    --==   This version of the 'tcp_app_flash' has the following interfaces:
    --==    - one bidirectionnal TCP data stream and one streaming MemoryPort. 
    --==========================================================================
    TAF : TcpApplicationFlashTodo
      port map (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        ap_clk                   => piSHL_156_25Clk,
        ap_rst_n                 => not (piSHL_Mmio_Ly7Rst),
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------       
        piSHL_MmioEchoCtrl_V     => piSHL_Mmio_TcpEchoCtrl,
        piSHL_MmioPostSegEn_V    => piSHL_Mmio_TcpPostSegEn,
        --[TODO] piSHL_MmioCaptSegEn  => piSHL_Mmio_TcpCaptSegEn,
        ------------------------------------------------------
        -- From TRIF / Session Connect Id Interface
        ------------------------------------------------------
        piTSIF_SConnectId_V      => sTSIF_TAF_SessConId,
        --------------------------------------------------------
        -- From SHELL / Tcp Interfaces
        --------------------------------------------------------
        siSHL_Data_tdata         => ssTSIF_TAF_Data_tdata,
        siSHL_Data_tkeep         => ssTSIF_TAF_Data_tkeep,
        siSHL_Data_tlast         => ssTSIF_TAF_Data_tlast,
        siSHL_Data_tvalid        => ssTSIF_TAF_Data_tvalid,
        siSHL_Data_tready        => ssTSIF_TAF_Data_tready,
        --
        siSHL_SessId_tdata       => ssTSIF_TAF_Meta_tdata,
        siSHL_SessId_tkeep       => (others=>'1'),             -- [TODO-ssTSIF_TAF_Meta_tkeep]
        siSHL_SessId_tlast       => '1',                       -- [TODO-ssTSIF_TAF_Meta_tlast]
        siSHL_SessId_tvalid      => ssTSIF_TAF_Meta_tvalid,
        siSHL_SessId_tready      => ssTSIF_TAF_Meta_tready,
        --------------------------------------------------------
        -- To SHELL / Tcp Data Interfaces
        --------------------------------------------------------
        soSHL_Data_tdata         => ssTAF_TSIF_Data_tdata,
        soSHL_Data_tkeep         => ssTAF_TSIF_Data_tkeep,
        soSHL_Data_tlast         => ssTAF_TSIF_Data_tlast,
        soSHL_Data_tvalid        => ssTAF_TSIF_Data_tvalid,
        soSHL_Data_tready        => ssTAF_TSIF_Data_tready,
        --
        soSHL_SessId_tdata       => ssTAF_TSIF_Meta_tdata,
        soSHL_SessId_tkeep       => open,                      -- [TODO-ssTAF_TSIF_Meta_tkeep]
        soSHL_SessId_tlast       => open,                      -- [TODO-ssTAF_TSIF_Meta_tlast]
        soSHL_SessId_tvalid      => ssTAF_TSIF_Meta_tvalid,
        soSHL_SessId_tready      => ssTAF_TSIF_Meta_tready
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
      ap_rst_n                   => not piSHL_Mmio_Ly7Rst,  --OBSOLETE not (piSHL_156_25Rst),
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
    
end architecture Flash;

