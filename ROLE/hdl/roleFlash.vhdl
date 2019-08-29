-- *****************************************************************************
-- *
-- *                             cloudFPGA
-- *            All rights reserved -- Property of IBM
-- *
-- *----------------------------------------------------------------------------
-- *
-- * Title : Flash for the FMKU2595 when equipped with a XCKU060.
-- *
-- * File    : roleFlash.vhdl
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
-- *    The current module contains the boot Flash application of the FPGA card
-- *    that is specified here as a 'ROLE'. Such a role is referred to as a
-- *    "superuser" role because it cannot be instantiated by a non-priviledged
-- *    cloudFPGA user. 
-- *
-- *    As the name of the entity indicates, this ROLE implements the following
-- *    interfaces with the SHELL:
-- *      - one UDP port interface (based on the AXI4-Stream interface), 
-- *      - one TCP port interface (based on the AXI4-Stream interface),
-- *      - two Memory Port interfaces (based on the MM2S and S2MM AXI4-Stream
-- *        interfaces described in PG022-AXI-DataMover).
-- *
-- * Parameters: None.
-- *
-- * Comments:
-- *  [FIXME] - Why is 'sROL_Shl_Nts0_Udp_Axis_tdata[63:0]' only active every 
-- *            second clock cycle?
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

-- library XIL_DEFAULTLIB;
-- use     XIL_DEFAULTLIB.all;


--******************************************************************************
--**  ENTITY  **  FMKU60 ROLE
--******************************************************************************

entity Role_x1Udp_x1Tcp_x2Mp is
  port (

    --------------------------------------------------------
    -- SHELL / Clock, Reset and Enable Interface
    --------------------------------------------------------
    piSHL_156_25Clk                     : in    std_ulogic;
    piSHL_156_25Rst                     : in    std_ulogic;
    --OBSOLETE-20190826 piTOP_156_25Rst_delayed             : in    std_ulogic; -- [TODO - Get rid of this delayed reset]

    --------------------------------------------------------
    -- SHELL / Nts / Udp Interface
    --------------------------------------------------------
    -- Input UDP Data (AXI4S) ----------
    siSHL_Nts_Udp_Data_tdata            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Nts_Udp_Data_tkeep            : in    std_ulogic_vector(  7 downto 0);
    siSHL_Nts_Udp_Data_tlast            : in    std_ulogic;
    siSHL_Nts_Udp_Data_tvalid           : in    std_ulogic;  
    siSHL_Nts_Udp_Data_tready           : out   std_ulogic;
    -- Output UDP Data (AXI4S) ---------
    soSHL_Nts_Udp_Data_tdata            : out   std_ulogic_vector( 63 downto 0);
    soSHL_Nts_Udp_Data_tkeep            : out   std_ulogic_vector(  7 downto 0);
    soSHL_Nts_Udp_Data_tlast            : out   std_ulogic;
    soSHL_Nts_Udp_Data_tvalid           : out   std_ulogic;
    soSHL_Nts_Udp_Data_tready           : in    std_ulogic;
       
    ------------------------------------------------------
    -- SHELL / Nts / Tcp / TxP Data Flow Interfaces
    ------------------------------------------------------
    -- FPGA Transmit Path (ROLE-->SHELL) ---------
    ---- Stream TCP Data ---------------
    soSHL_Nts_Tcp_Data_tdata            : out   std_ulogic_vector( 63 downto 0);
    soSHL_Nts_Tcp_Data_tkeep            : out   std_ulogic_vector(  7 downto 0);
    soSHL_Nts_Tcp_Data_tlast            : out   std_ulogic;
    soSHL_Nts_Tcp_Data_tvalid           : out   std_ulogic;
    soSHL_Nts_Tcp_Data_tready           : in    std_ulogic;
    ---- Stream TCP Metadata -----------
    soSHL_Nts_Tcp_Meta_tdata            : out   std_ulogic_vector( 15 downto 0);
    soSHL_Nts_Tcp_Meta_tvalid           : out   std_ulogic;
    soSHL_Nts_Tcp_Meta_tready           : in    std_ulogic;
    ---- Stream TCP Data Status --------
    siSHL_Nts_Tcp_DSts_tdata            : in    std_ulogic_vector( 23 downto 0);
    siSHL_Nts_Tcp_DSts_tvalid           : in    std_ulogic;
    siSHL_Nts_Tcp_DSts_tready           : out   std_ulogic;

    --------------------------------------------------------
    -- SHELL / Nts / Tcp / RxP Data Flow Interfaces
    --------------------------------------------------------
    -- FPGA Receive Path (SHELL-->ROLE) ----------
    ---- Stream TCP Data ---------------
    siSHL_Nts_Tcp_Data_tdata            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Nts_Tcp_Data_tkeep            : in    std_ulogic_vector(  7 downto 0);
    siSHL_Nts_Tcp_Data_tlast            : in    std_ulogic;
    siSHL_Nts_Tcp_Data_tvalid           : in    std_ulogic;
    siSHL_Nts_Tcp_Data_tready           : out   std_ulogic;
    ---- Stream TCP Meta ---------------
    siSHL_Nts_Tcp_Meta_tdata            : in    std_ulogic_vector( 15 downto 0);
    siSHL_Nts_Tcp_Meta_tvalid           : in    std_ulogic;
    siSHL_Nts_Tcp_Meta_tready           : out   std_ulogic;
    ---- Stream TCP Data Notification --
    siSHL_Nts_Tcp_Notif_tdata           : in    std_ulogic_vector( 87 downto 0);
    siSHL_Nts_Tcp_Notif_tvalid          : in    std_ulogic;
    siSHL_Nts_Tcp_Notif_tready          : out   std_ulogic;
    ---- Stream TCP Data Request -------
    soSHL_Nts_Tcp_DReq_tdata            : out   std_ulogic_vector( 31 downto 0); 
    soSHL_Nts_Tcp_DReq_tvalid           : out   std_ulogic;       
    soSHL_Nts_Tcp_DReq_tready           : in    std_ulogic;

    ------------------------------------------------------
    -- SHELL / Nts / Tcp / TxP Ctlr Flow Interfaces
    ------------------------------------------------------
    -- FPGA Transmit Path (ROLE-->SHELL) ---------
    ---- Stream TCP Open Session Request
    soSHL_Nts_Tcp_OpnReq_tdata          : out   std_ulogic_vector( 47 downto 0);  
    soSHL_Nts_Tcp_OpnReq_tvalid         : out   std_ulogic;
    soSHL_Nts_Tcp_OpnReq_tready         : in    std_ulogic;
    ---- Stream TCP Open Session Status  
    siSHL_Nts_Tcp_OpnRep_tdata          : in    std_ulogic_vector( 23 downto 0); 
    siSHL_Nts_Tcp_OpnRep_tvalid         : in    std_ulogic;
    siSHL_Nts_Tcp_OpnRep_tready         : out   std_ulogic;
    ---- Stream TCP Close Request ------
    soSHL_Nts_Tcp_ClsReq_tdata          : out   std_ulogic_vector( 15 downto 0);  
    soSHL_Nts_Tcp_ClsReq_tvalid         : out   std_ulogic;
    soSHL_Nts_Tcp_ClsReq_tready         : in    std_ulogic;

    ------------------------------------------------------
    -- SHELL / Nts / Tcp / RxP Ctlr Flow Interfaces
    ------------------------------------------------------
    -- FPGA Receive Path (SHELL-->ROLE) ----------
    ---- Stream TCP Listen Request -----
    soSHL_Nts_Tcp_LsnReq_tdata          : out   std_ulogic_vector( 15 downto 0);  
    soSHL_Nts_Tcp_LsnReq_tvalid         : out   std_ulogic;
    soSHL_Nts_Tcp_LsnReq_tready         : in    std_ulogic;
    ---- Stream TCP Listen Status ----
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
    ---- Memory Port #1 / S2MM-AXIS ------------------   
    ------ Stream Read Command ---------
    soSHL_Mem_Mp1_RdCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp1_RdCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp1_RdCmd_tready          : in    std_ulogic;
    ------ Stream Read Status ----------
    siSHL_Mem_Mp1_RdSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp1_RdSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp1_RdSts_tready          : out   std_ulogic;
    ------ Stream Data Input Channel ---
    siSHL_Mem_Mp1_Read_tdata            : in    std_ulogic_vector(511 downto 0);
    siSHL_Mem_Mp1_Read_tkeep            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Mem_Mp1_Read_tlast            : in    std_ulogic;
    siSHL_Mem_Mp1_Read_tvalid           : in    std_ulogic;
    siSHL_Mem_Mp1_Read_tready           : out   std_ulogic;
    ------ Stream Write Command --------
    soSHL_Mem_Mp1_WrCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp1_WrCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp1_WrCmd_tready          : in    std_ulogic;
    ------ Stream Write Status ---------
    siSHL_Mem_Mp1_WrSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp1_WrSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp1_WrSts_tready          : out   std_ulogic;
    ------ Stream Data Output Channel --
    soSHL_Mem_Mp1_Write_tdata           : out   std_ulogic_vector(511 downto 0);
    soSHL_Mem_Mp1_Write_tkeep           : out   std_ulogic_vector( 63 downto 0);
    soSHL_Mem_Mp1_Write_tlast           : out   std_ulogic;
    soSHL_Mem_Mp1_Write_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp1_Write_tready          : in    std_ulogic; 
    
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
    piTOP_250_00Clk                     : in    std_ulogic;  -- Freerunning

    poVoid                              : out   std_ulogic

  );
  
end Role_x1Udp_x1Tcp_x2Mp;


-- *****************************************************************************
-- **  ARCHITECTURE  **  FLASH of ROLE 
-- *****************************************************************************

architecture Flash of Role_x1Udp_x1Tcp_x2Mp is

  constant cAPP_USE_DEPRECATED_DIRECTIVES  : boolean := true;
  constant cUDP_USE_DEPRECATED_DIRECTIVES  : boolean := true;
  constant cTCP_USE_DEPRECATED_DIRECTIVES  : boolean := false;

  --============================================================================
  --  SIGNAL DECLARATIONS
  --============================================================================  

  -- Delayed reset signal and counter 
  signal s156_25Rst_delayed             : std_ulogic;
  signal sRstDelayCounter               : std_ulogic_vector(5 downto 0);

  ------------------------------------------------------
  -- UDP AXIS READ Register
  ------------------------------------------------------
  signal sUdpAxisReadReg_tdata          : std_ulogic_vector( 63 downto 0);
  signal sUdpAxisReadReg_tkeep          : std_ulogic_vector(  7 downto 0);
  signal sUdpAxisReadReg_tlast          : std_ulogic;
  signal sUdpAxisReadReg_tvalid         : std_ulogic;
   
  ------------------------------------------------------
  -- UDP PASS-THROUGH Register
  ------------------------------------------------------
  signal sUdpPassThruReg_tdata          : std_ulogic_vector( 63 downto 0);
  signal sUdpPassThruReg_tkeep          : std_ulogic_vector(  7 downto 0);
  signal sUdpPassThruReg_tlast          : std_ulogic;
  signal sUdpPassThruReg_tvalid         : std_ulogic;
   
  signal sUdpPassThruReg_isFull         : boolean;

  -- FPGA Transmit Path (TRIF->TAF) -------------
  ---- Stream TCP Data -------------
  signal ssTRIF_TAF_Data_tdata          : std_ulogic_vector( 63 downto 0);
  signal ssTRIF_TAF_Data_tkeep          : std_ulogic_vector(  7 downto 0);
  signal ssTRIF_TAF_Data_tlast          : std_ulogic;
  signal ssTRIF_TAF_Data_tvalid         : std_ulogic;
  signal ssTRIF_TAF_Data_tready         : std_ulogic;
  ---- Stream TCP Metadata --------
  signal ssTRIF_TAF_Meta_tdata          : std_ulogic_vector( 15 downto 0);
  signal ssTRIF_TAF_Meta_tvalid         : std_ulogic;
  signal ssTRIF_TAF_Meta_tready         : std_ulogic;
           
  -- FPGA Transmit Path (TAF-->TRIF) --------                      
  ---- Stream TCP Data -------------
  signal ssTAF_TRIF_Data_tdata          : std_ulogic_vector( 63 downto 0);
  signal ssTAF_TRIF_Data_tkeep          : std_ulogic_vector(  7 downto 0);
  signal ssTAF_TRIF_Data_tlast          : std_ulogic;
  signal ssTAF_TRIF_Data_tvalid         : std_ulogic;
  signal ssTAF_TRIF_Data_tready         : std_ulogic;
  ---- Stream TCP Metadata --------
  signal ssTAF_TRIF_Meta_tdata          : std_ulogic_vector( 15 downto 0);
  signal ssTAF_TRIF_Meta_tvalid         : std_ulogic;
  signal ssTAF_TRIF_Meta_tready         : std_ulogic;

  signal EMIF_inv                       : std_logic_vector(7 downto 0);

  -- I hate Vivado HLS 
  --OBSOLETE-20190826 signal sReadTlastAsVector             : std_logic_vector(0 downto 0);
  --OBSOLETE-20190826 signal sWriteTlastAsVector            : std_logic_vector(0 downto 0);
  --OBSOLETE-20190826 signal sResetAsVector                 : std_logic_vector(0 downto 0);
  signal sSHL_Mem_Mp0_Write_tlast       : std_ulogic_vector(0 downto 0); 

  signal sUdpPostCnt                    : std_ulogic_vector(9 downto 0);
  signal sTcpPostCnt                    : std_ulogic_vector(9 downto 0);

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
      aclk                      : in  std_logic;
      aresetn                   : in  std_logic;    
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piSHL_This_MmioEchoCtrl_V : in  std_logic_vector(  1 downto 0);
      --[TODO] piSHL_This_MmioPostDgmEn_V  : in  std_logic;
      --[TODO] piSHL_This_MmioCaptDgmEn_V  : in  std_logic;
      --------------------------------------------------------
      -- From SHELL / Udp Data Interfaces
      --------------------------------------------------------
      siSHL_This_Data_tdata     : in  std_logic_vector( 63 downto 0);
      siSHL_This_Data_tkeep     : in  std_logic_vector(  7 downto 0);
      siSHL_This_Data_tlast     : in  std_logic;
      siSHL_This_Data_tvalid    : in  std_logic;
      siSHL_This_Data_tready    : out std_logic;
      --------------------------------------------------------
      -- To SHELL / Udp Data Interfaces
      --------------------------------------------------------
      soTHIS_Shl_Data_tdata     : out std_logic_vector( 63 downto 0);
      soTHIS_Shl_Data_tkeep     : out std_logic_vector(  7 downto 0);
      soTHIS_Shl_Data_tlast     : out std_logic;
      soTHIS_Shl_Data_tvalid    : out std_logic;
      soTHIS_Shl_Data_tready    : in  std_logic
    );
  end component UdpApplicationFlash;

  component UdpApplicationFlashTodo is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                    : in  std_logic;
      ap_rst_n                  : in  std_logic;
      ------------------------------------------------------
      -- BLock-Level I/O Protocol
      ------------------------------------------------------
      --ap_start                : in  std_logic;
      --ap_ready                : out std_logic;
      --ap_done                 : out std_logic;
      --ap_idle                 : out std_logic;
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piSHL_This_MmioEchoCtrl_V : in  std_logic_vector(  1 downto 0);
      --[TODO] piSHL_This_MmioPostDgmEn_V  : in  std_logic;
      --[TODO] piSHL_This_MmioCaptDgmEn_V  : in  std_logic;
      --------------------------------------------------------
      -- From SHELL / Udp Data Interfaces
      --------------------------------------------------------
      siSHL_This_Data_tdata     : in  std_logic_vector( 63 downto 0);
      siSHL_This_Data_tkeep     : in  std_logic_vector(  7 downto 0);
      siSHL_This_Data_tlast     : in  std_logic_vector(  0 downto 0);
      siSHL_This_Data_tvalid    : in  std_logic;
      siSHL_This_Data_tready    : out std_logic;
      --------------------------------------------------------
      -- To SHELL / Udp Data Interfaces
      --------------------------------------------------------
      soTHIS_Shl_Data_tdata     : out std_logic_vector( 63 downto 0);
      soTHIS_Shl_Data_tkeep     : out std_logic_vector(  7 downto 0);
      soTHIS_Shl_Data_tlast     : out std_logic_vector(  0 downto 0);
      soTHIS_Shl_Data_tvalid    : out std_logic;
      soTHIS_Shl_Data_tready    : in  std_logic
    );
  end component UdpApplicationFlashTodo;
  
  component TcpApplicationFlash is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      aclk                  : in  std_logic;
      aresetn               : in  std_logic;    
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piSHL_MmioEchoCtrl_V  : in  std_logic_vector(  1 downto 0);
      piSHL_MmioPostSegEn_V : in  std_logic;
      --[TODO] piSHL_MmioCaptSegEn_V  : in  std_logic;
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
 
  component TcpApplicationFlashTodo is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                : in  std_logic;
      ap_rst_n              : in  std_logic;
      ------------------------------------------------------
      -- BLock-Level I/O Protocol
      ------------------------------------------------------
      --ap_start            : in  std_logic;
      --ap_ready            : out std_logic;
      --ap_done             : out std_logic;
      --ap_idle             : out std_logic;
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piSHL_MmioEchoCtrl_V  : in  std_logic_vector(  1 downto 0);
      piSHL_MmioPostSegEn_V : in  std_logic;
      --[TODO] piSHL_MmioCaptSegEn  : in  std_logic;
      
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
  end component TcpApplicationFlashTodo; 

  component TcpRoleInterfaceDepre is
    port (
      ------------------------------------------------------
      -- SHELL / Clock and Reset
      ------------------------------------------------------
      aclk                      : in  std_ulogic;
      aresetn                   : in  std_ulogic;
   
      --------------------------------------------------------
      -- SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piSHL_Mmio_En_V          : in  std_ulogic;
       
      ------------------------------------------------------
      -- ROLE / Nts / Tcp / TxP Data Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (ROLE-->SHELL) ---------
      ---- Stream TCP Data -------------
      siROL_Data_tdata          : in  std_ulogic_vector( 63 downto 0);
      siROL_Data_tkeep          : in  std_ulogic_vector(  7 downto 0);
      siROL_Data_tlast          : in  std_ulogic;
      siROL_Data_tvalid         : in  std_ulogic;
      siROL_Data_tready         : out std_ulogic;
      ---- Stream TCP Metadata ---------
      siROL_SessId_tdata        : in  std_ulogic_vector( 15 downto 0);
      siROL_SessId_tvalid       : in  std_ulogic;
      siROL_SessId_tready       : out std_ulogic; 
        
      ------------------------------------------------------               
      -- ROLE / Nts / Tcp / RxP Data Flow Interfaces                      
      ------------------------------------------------------               
      -- FPGA Transmit Path (SHELL-->ROLE) --------                      
      ---- Stream TCP Data -------------
      soROL_Data_tdata          : out std_ulogic_vector( 63 downto 0);
      soROL_Data_tkeep          : out std_ulogic_vector(  7 downto 0);
      soROL_Data_tlast          : out std_ulogic;
      soROL_Data_tvalid         : out std_ulogic;
      soROL_Data_tready         : in  std_ulogic;
      ---- Stream TCP Metadata ---------
      soROL_SessId_tdata        : out std_ulogic_vector( 15 downto 0);
      soROL_SessId_tvalid       : out std_ulogic;
      soROL_SessId_tready       : in  std_ulogic;
         
      ------------------------------------------------------
      -- TOE / RxP Data Interfaces
      ------------------------------------------------------
      ---- Stream TCP Data Notification 
      siTOE_Notif_tdata         : in  std_ulogic_vector( 87 downto 0);
      siTOE_Notif_tvalid        : in  std_ulogic;
      siTOE_Notif_tready        : out std_ulogic;
      ---- Stream TCP Data Request ---
      soTOE_DReq_tdata          : out std_ulogic_vector( 31 downto 0);
      soTOE_DReq_tvalid         : out std_ulogic;
      soTOE_DReq_tready         : in  std_ulogic;
      ---- Stream TCP Data -----------
      siTOE_Data_tdata          : in  std_ulogic_vector( 63 downto 0);
      siTOE_Data_tkeep          : in  std_ulogic_vector(  7 downto 0);
      siTOE_Data_tlast          : in  std_ulogic;
      siTOE_Data_tvalid         : in  std_ulogic;
      siTOE_Data_tready         : out std_ulogic;
      ---- Stream TCP Metadata -------
      siTOE_SessId_tdata        : in  std_ulogic_vector( 15 downto 0);
      siTOE_SessId_tvalid       : in  std_ulogic;
      siTOE_SessId_tready       : out std_ulogic;       
      
      ------------------------------------------------------
      -- TOE / RxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Receive Path (SHELL-->ROLE) -------
      ---- Stream TCP Listen Request -
      soTOE_LsnReq_tdata        : out std_ulogic_vector( 15 downto 0);   
      soTOE_LsnReq_tvalid       : out std_ulogic;
      soTOE_LsnReq_tready       : in  std_ulogic;
      ---- Stream TCP Listen Status --
      siTOE_LsnAck_tdata        : in  std_ulogic_vector(  7 downto 0);
      siTOE_LsnAck_tvalid       : in  std_ulogic;
      siTOE_LsnAck_tready       : out std_ulogic;
     
      ------------------------------------------------------
      -- TOE / TxP Data Interfaces
      ------------------------------------------------------
      ---- Stream TCP Data -----------
      soTOE_Data_tdata          : out std_ulogic_vector( 63 downto 0);
      soTOE_Data_tkeep          : out std_ulogic_vector(  7 downto 0);
      soTOE_Data_tlast          : out std_ulogic;
      soTOE_Data_tvalid         : out std_ulogic;
      soTOE_Data_tready         : in  std_ulogic;
      ---- Stream TCP Metadata -------
      soTOE_SessId_tdata        : out std_ulogic_vector( 15 downto 0);
      soTOE_SessId_tvalid       : out std_ulogic;
      soTOE_SessId_tready       : in  std_ulogic;
      ---- Stream TCP Data Status ----
      siTOE_DSts_tdata          : in  std_ulogic_vector( 23 downto 0);
      siTOE_DSts_tvalid         : in  std_ulogic;
      siTOE_DSts_tready         : out std_ulogic;
           
      ------------------------------------------------------
      -- TOE / TxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (ROLE-->SHELL) ------
      ---- Stream TCP Open Session Request
      soTOE_OpnReq_tdata        : out std_ulogic_vector( 47 downto 0);
      soTOE_OpnReq_tvalid       : out std_ulogic;
      soTOE_OpnReq_tready       : in  std_ulogic;
      ---- Stream TCP Open Session Status 
      siTOE_OpnRep_tdata        : in  std_ulogic_vector( 23 downto 0);
      siTOE_OpnRep_tvalid       : in  std_ulogic;
      siTOE_OpnRep_tready       : out std_ulogic;
      ---- Stream TCP Close Request --
      soTOE_ClsReq_tdata        : out std_ulogic_vector( 15 downto 0);
      soTOE_ClsReq_tvalid       : out std_ulogic;
      soTOE_ClsReq_tready       : in  std_ulogic
     
    );
  end component TcpRoleInterfaceDepre;
  
  component TcpRoleInterface is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                    : in  std_ulogic;
      ap_rst_n                  : in  std_ulogic;
    
       --------------------------------------------------------
       -- From SHELL / Mmio Interfaces
       --------------------------------------------------------       
       piSHL_Mmio_En_V          : in  std_ulogic;
       
      ------------------------------------------------------
      -- ROLE / Nts / Tcp / TxP Data Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (ROLE-->SHELL) ---------
      ---- Stream TCP Data -------------
      siROL_Data_tdata          : in  std_ulogic_vector( 63 downto 0);
      siROL_Data_tkeep          : in  std_ulogic_vector(  7 downto 0);
      siROL_Data_tlast          : in  std_ulogic;
      siROL_Data_tvalid         : in  std_ulogic;
      siROL_Data_tready         : out std_ulogic;
      ---- Stream TCP Metadata ---------
      siROL_SessId_V_V_tdata    : in  std_ulogic_vector( 15 downto 0);
      siROL_SessId_V_V_tvalid   : in  std_ulogic;
      siROL_SessId_V_v_tready   : out std_ulogic; 
        
      ------------------------------------------------------               
      -- ROLE / Nts / Tcp / RxP Data Flow Interfaces                      
      ------------------------------------------------------               
      -- FPGA Transmit Path (SHELL-->ROLE) --------                      
      ---- Stream TCP Data -------------
      soROL_Data_tdata          : out std_ulogic_vector( 63 downto 0);
      soROL_Data_tkeep          : out std_ulogic_vector(  7 downto 0);
      soROL_Data_tlast          : out std_ulogic;
      soROL_Data_tvalid         : out std_ulogic;
      soROL_Data_tready         : in  std_ulogic;
      ---- Stream TCP Metadata ---------
      soROL_SessId_V_V_tdata    : out std_ulogic_vector( 15 downto 0);
      soROL_SessId_V_V_tvalid   : out std_ulogic;
      soROL_SessId_V_V_tready   : in  std_ulogic;
         
      ------------------------------------------------------
      -- TOE / RxP Data Interfaces
      ------------------------------------------------------
      ---- Stream TCP Data Notification 
      siTOE_Notif_V_tdata       : in  std_ulogic_vector( 87 downto 0);
      siTOE_Notif_V_tvalid      : in  std_ulogic;
      siTOE_Notif_V_tready      : out std_ulogic;
      ---- Stream TCP Data Request ---
      soTOE_DReq_V_tdata        : out std_ulogic_vector( 31 downto 0);
      soTOE_DReq_V_tvalid       : out std_ulogic;
      soTOE_DReq_V_tready       : in  std_ulogic;
      ---- Stream TCP Data -----------
      siTOE_Data_tdata          : in  std_ulogic_vector( 63 downto 0);
      siTOE_Data_tkeep          : in  std_ulogic_vector(  7 downto 0);
      siTOE_Data_tlast          : in  std_ulogic;
      siTOE_Data_tvalid         : in  std_ulogic;
      siTOE_Data_tready         : out std_ulogic;
      ---- Stream TCP Metadata -------
      siTOE_SessId_V_V_tdata    : in  std_ulogic_vector( 15 downto 0);
      siTOE_SessId_V_V_tvalid   : in  std_ulogic;
      siTOE_SessId_V_V_tready   : out std_ulogic;       
      
      ------------------------------------------------------
      -- TOE / RxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Receive Path (SHELL-->ROLE) -------
      ---- Stream TCP Listen Request -
      soTOE_LsnReq_V_V_tdata    : out std_ulogic_vector( 15 downto 0);   
      soTOE_LsnReq_V_V_tvalid   : out std_ulogic;
      soTOE_LsnReq_V_V_tready   : in  std_ulogic;
      ---- Stream TCP Listen Status --
      siTOE_LsnAck_V_tdata      : in  std_ulogic_vector(  7 downto 0);
      siTOE_LsnAck_V_tvalid     : in  std_ulogic;
      siTOE_LsnAck_V_tready     : out std_ulogic;
     
      ------------------------------------------------------
      -- TOE / TxP Data Interfaces
      ------------------------------------------------------
      ---- Stream TCP Data -----------
      soTOE_Data_tdata          : out std_ulogic_vector( 63 downto 0);
      soTOE_Data_tkeep          : out std_ulogic_vector(  7 downto 0);
      soTOE_Data_tlast          : out std_ulogic;
      soTOE_Data_tvalid         : out std_ulogic;
      soTOE_Data_tready         : in  std_ulogic;
      ---- Stream TCP Metadata -------
      soTOE_SessId_V_V_tdata    : out std_ulogic_vector( 15 downto 0);
      soTOE_SessId_V_V_tvalid   : out std_ulogic;
      soTOE_SessId_V_V_tready   : in  std_ulogic;
      ---- Stream TCP Data Status ----
      siTOE_DSts_V_V_tdata      : in  std_ulogic_vector( 23 downto 0);
      siTOE_DSts_V_V_tvalid     : in  std_ulogic;
      siTOE_DSts_V_V_tready     : out std_ulogic;
           
      ------------------------------------------------------
      -- TOE / TxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (ROLE-->SHELL) ------
      ---- Stream TCP Open Session Request
      soTOE_OpnReq_V_tdata      : out std_ulogic_vector( 47 downto 0);
      soTOE_OpnReq_V_tvalid     : out std_ulogic;
      soTOE_OpnReq_V_tready     : in  std_ulogic;
      ---- Stream TCP Open Session Status 
      siTOE_OpnRep_V_tdata      : in  std_ulogic_vector( 23 downto 0);
      siTOE_OpnRep_V_tvalid     : in  std_ulogic;
      siTOE_OpnRep_V_tready     : out std_ulogic;
      ---- Stream TCP Close Request --
      soTOE_ClsReq_V_V_tdata    : out std_ulogic_vector( 15 downto 0);
      soTOE_ClsReq_V_V_tvalid   : out std_ulogic;
      soTOE_ClsReq_V_V_tready   : in  std_ulogic
     
    );
  end component TcpRoleInterface;
  
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
      
      soMemRdCmdP0_TDATA         : out std_logic_vector( 79 downto 0);
      soMemRdCmdP0_TVALID        : out std_logic;
      soMemRdCmdP0_TREADY        : in  std_logic;
      siMemRdStsP0_TDATA         : in  std_logic_vector(  7 downto 0);
      siMemRdStsP0_TVALID        : in  std_logic;
      siMemRdStsP0_TREADY        : out std_logic;
      siMemReadP0_TDATA          : in  std_logic_vector(511 downto 0);
      siMemReadP0_TVALID         : in  std_logic;
      siMemReadP0_TREADY         : out std_logic;
      siMemReadP0_TKEEP          : in  std_logic_vector( 63 downto 0);
      siMemReadP0_TLAST          : in  std_logic_vector(  0 downto 0);
      soMemWrCmdP0_TDATA         : out std_logic_vector( 79 downto 0);
      soMemWrCmdP0_TVALID        : out std_logic;
      soMemWrCmdP0_TREADY        : in  std_logic;
      siMemWrStsP0_TDATA         : in  std_logic_vector(  7 downto 0);
      siMemWrStsP0_TVALID        : in  std_logic;
      siMemWrStsP0_TREADY        : out std_logic;
      soMemWriteP0_TDATA         : out std_logic_vector(511 downto 0);
      soMemWriteP0_TVALID        : out std_logic;
      soMemWriteP0_TREADY        : in  std_logic;
      soMemWriteP0_TKEEP         : out std_logic_vector( 63 downto 0);
      soMemWriteP0_TLAST         : out std_logic_vector(  0 downto 0) 
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
  --#    #######  #####   ###  #######                                             #
  --#       #     ##   #   #   #                                                   #
  --#       #     #    #   #   #                                                   #
  --#       #     #####    #   ####                                                #
  --#       #     #   #    #   #                                                   #
  --#       #     #    #  ###  #                                                   #
  --#                                                                              #
  --################################################################################
  
  gTcpRoleInterface : if cTCP_USE_DEPRECATED_DIRECTIVES = true
    generate
         
      TRIF : TcpRoleInterfaceDepre
    
        port map (
          ------------------------------------------------------
          -- From SHELL / Clock and Reset
          ------------------------------------------------------
          aclk                      => piSHL_156_25Clk,
          aresetn                   => (not piSHL_156_25Rst or not piSHL_Mmio_Ly7Rst),
          
          --------------------------------------------------------
          -- From SHELL / Mmio Interfaces
          --------------------------------------------------------       
          piSHL_Mmio_En_V           => piSHL_Mmio_Ly7En,
          
          ------------------------------------------------------
          -- ROLE / Nts / Tcp / TxP Data Flow Interfaces
          ------------------------------------------------------
          -- FPGA Transmit Path (ROLE-->SHELL) ---------
          ---- Stream TCP Data -------------
          siROL_Data_tdata          => ssTAF_TRIF_Data_tdata,
          siROL_Data_tkeep          => ssTAF_TRIF_Data_tkeep,
          siROL_Data_tlast          => ssTAF_TRIF_Data_tlast,
          siROL_Data_tvalid         => ssTAF_TRIF_Data_tvalid,
          siROL_Data_tready         => ssTAF_TRIF_Data_tready,
          ---- Stream TCP Metadata ---------
          siROL_SessId_tdata        => ssTAF_TRIF_Meta_tdata,
          siROL_SessId_tvalid       => ssTAF_TRIF_Meta_tvalid,
          siROL_SessId_tready       => ssTAF_TRIF_Meta_tready,
            
          ------------------------------------------------------               
          -- ROLE / Nts / Tcp / RxP Data Flow Interfaces                      
          ------------------------------------------------------               
          -- FPGA Transmit Path (SHELL-->ROLE) --------                      
          ---- Stream TCP Data -------------
          soROL_Data_tdata          => ssTRIF_TAF_Data_tdata,
          soROL_Data_tkeep          => ssTRIF_TAF_Data_tkeep,
          soROL_Data_tlast          => ssTRIF_TAF_Data_tlast,
          soROL_Data_tvalid         => ssTRIF_TAF_Data_tvalid,
          soROL_Data_tready         => ssTRIF_TAF_Data_tready,
          ---- Stream TCP Metadata ---------
          soROL_SessId_tdata        => ssTRIF_TAF_Meta_tdata,
          soROL_SessId_tvalid       => ssTRIF_TAF_Meta_tvalid,
          soROL_SessId_tready       => ssTRIF_TAF_Meta_tready,
             
          ------------------------------------------------------
          -- TOE / RxP Data Interfaces
          ------------------------------------------------------
          ---- Stream TCP Data Notification 
          siTOE_Notif_tdata         => siSHL_Nts_Tcp_Notif_tdata,
          siTOE_Notif_tvalid        => siSHL_Nts_Tcp_Notif_tvalid,
          siTOE_Notif_tready        => siSHL_Nts_Tcp_Notif_tready,
          ---- Stream TCP Data Request -----
          soTOE_DReq_tdata          => soSHL_Nts_Tcp_DReq_tdata,
          soTOE_DReq_tvalid         => soSHL_Nts_Tcp_DReq_tvalid,
          soTOE_DReq_tready         => soSHL_Nts_Tcp_DReq_tready,
          ---- Stream TCP Data ------------- Nts_Tcp_
          siTOE_Data_tdata          => siSHL_Nts_Tcp_Data_tdata, 
          siTOE_Data_tkeep          => siSHL_Nts_Tcp_Data_tkeep, 
          siTOE_Data_tlast          => siSHL_Nts_Tcp_Data_tlast, 
          siTOE_Data_tvalid         => siSHL_Nts_Tcp_Data_tvalid,
          siTOE_Data_tready         => siSHL_Nts_Tcp_Data_tready,
          ---- Stream TCP Metadata ---------
          siTOE_SessId_tdata        => siSHL_Nts_Tcp_Meta_tdata, 
          siTOE_SessId_tvalid       => siSHL_Nts_Tcp_Meta_tvalid,
          siTOE_SessId_tready       => siSHL_Nts_Tcp_Meta_tready,
          
          ------------------------------------------------------
          -- TOE / RxP Ctlr Flow Interfaces
          ------------------------------------------------------
          -- FPGA Receive Path (SHELL-->ROLE) ------- :
          ---- Stream TCP Listen Request -----
          soTOE_LsnReq_tdata        => soSHL_Nts_Tcp_LsnReq_tdata, 
          soTOE_LsnReq_tvalid       => soSHL_Nts_Tcp_LsnReq_tvalid,
          soTOE_LsnReq_tready       => soSHL_Nts_Tcp_LsnReq_tready,
          ---- Stream TCP Listen -------------
          siTOE_LsnAck_tdata        => siSHL_Nts_Tcp_LsnAck_tdata, 
          siTOE_LsnAck_tvalid       => siSHL_Nts_Tcp_LsnAck_tvalid, 
          siTOE_LsnAck_tready       => siSHL_Nts_Tcp_LsnAck_tready, 
           
          ------------------------------------------------------
          -- TOE / TxP Data Interfaces
          ------------------------------------------------------
          ---- Stream TCP Data ------------ 
          soTOE_Data_tdata          => soSHL_Nts_Tcp_Data_tdata, 
          soTOE_Data_tkeep          => soSHL_Nts_Tcp_Data_tkeep, 
          soTOE_Data_tlast          => soSHL_Nts_Tcp_Data_tlast, 
          soTOE_Data_tvalid         => soSHL_Nts_Tcp_Data_tvalid,
          soTOE_Data_tready         => soSHL_Nts_Tcp_Data_tready,
          ---- Stream TCP Metadata ---------
          soTOE_SessId_tdata        => soSHL_Nts_Tcp_Meta_tdata,
          soTOE_SessId_tvalid       => soSHL_Nts_Tcp_Meta_tvalid,
          soTOE_SessId_tready       => soSHL_Nts_Tcp_Meta_tready,
          ---- Stream TCP Data Status ------
          siTOE_DSts_tdata          => siSHL_Nts_Tcp_DSts_tdata,
          siTOE_DSts_tvalid         => siSHL_Nts_Tcp_DSts_tvalid,
          siTOE_DSts_tready         => siSHL_Nts_Tcp_DSts_tready,
               
          ------------------------------------------------------
          -- TOE / TxP Ctlr Flow Interfaces
          ------------------------------------------------------
          -- FPGA Transmit Path (ROLE-->SHELL) ------
          ---- Stream TCP Open Session Request
          soTOE_OpnReq_tdata        => soSHL_Nts_Tcp_OpnReq_tdata, 
          soTOE_OpnReq_tvalid       => soSHL_Nts_Tcp_OpnReq_tvalid,
          soTOE_OpnReq_tready       => soSHL_Nts_Tcp_OpnReq_tready,
          ---- Stream TCP Open Session Status 
          siTOE_OpnRep_tdata        => siSHL_Nts_Tcp_OpnRep_tdata,  
          siTOE_OpnRep_tvalid       => siSHL_Nts_Tcp_OpnRep_tvalid,
          siTOE_OpnRep_tready       => siSHL_Nts_Tcp_OpnRep_tready,
          ---- Stream TCP Close Request ------
          soTOE_ClsReq_tdata        => soSHL_Nts_Tcp_ClsReq_tdata,
          soTOE_ClsReq_tvalid       => soSHL_Nts_Tcp_ClsReq_tvalid,
          soTOE_ClsReq_tready       => soSHL_Nts_Tcp_ClsReq_tready
    
        ); -- End of: TcpRoleInterfaceDepre

  else generate
    
    TRIF : TcpRoleInterface

      port map (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        ap_clk                    => piSHL_156_25Clk,
        ap_rst_n                  => (not piSHL_156_25Rst or not piSHL_Mmio_Ly7Rst),
        
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------
        piSHL_Mmio_En_V           => piSHL_Mmio_Ly7En,
        
        ------------------------------------------------------
        -- ROLE / Nts / Tcp / TxP Data Flow Interfaces
        ------------------------------------------------------
        -- FPGA Transmit Path (ROLE-->SHELL) ---------
        ---- Stream TCP Data -------------
        siROL_Data_tdata          => ssTAF_TRIF_Data_tdata,
        siROL_Data_tkeep          => ssTAF_TRIF_Data_tkeep,
        siROL_Data_tlast          => ssTAF_TRIF_Data_tlast,
        siROL_Data_tvalid         => ssTAF_TRIF_Data_tvalid,
        siROL_Data_tready         => ssTAF_TRIF_Data_tready,
        ---- Stream TCP Metadata ---------
        siROL_SessId_V_V_tdata    => ssTAF_TRIF_Meta_tdata,
        siROL_SessId_V_V_tvalid   => ssTAF_TRIF_Meta_tvalid,
        siROL_SessId_V_V_tready   => ssTAF_TRIF_Meta_tready,
          
        ------------------------------------------------------               
        -- ROLE / Nts / Tcp / RxP Data Flow Interfaces                      
        ------------------------------------------------------               
        -- FPGA Transmit Path (SHELL-->ROLE) --------                      
        ---- Stream TCP Data -------------
        soROL_Data_tdata          => ssTRIF_TAF_Data_tdata,
        soROL_Data_tkeep          => ssTRIF_TAF_Data_tkeep,
        soROL_Data_tlast          => ssTRIF_TAF_Data_tlast,
        soROL_Data_tvalid         => ssTRIF_TAF_Data_tvalid,
        soROL_Data_tready         => ssTRIF_TAF_Data_tready,
        ---- Stream TCP Metadata ---------
        soROL_SessId_V_V_tdata    => ssTRIF_TAF_Meta_tdata,
        soROL_SessId_V_V_tvalid   => ssTRIF_TAF_Meta_tvalid,
        soROL_SessId_V_V_tready   => ssTRIF_TAF_Meta_tready,
           
        ------------------------------------------------------
        -- TOE / RxP Data Interfaces
        ------------------------------------------------------
        ---- Stream TCP Data Notification 
        siTOE_Notif_V_tdata       => siSHL_Nts_Tcp_Notif_tdata,
        siTOE_Notif_V_tvalid      => siSHL_Nts_Tcp_Notif_tvalid,
        siTOE_Notif_V_tready      => siSHL_Nts_Tcp_Notif_tready,
        ---- Stream TCP Data Request -----
        soTOE_DReq_V_tdata        => soSHL_Nts_Tcp_DReq_tdata,
        soTOE_DReq_V_tvalid       => soSHL_Nts_Tcp_DReq_tvalid,
        soTOE_DReq_V_tready       => soSHL_Nts_Tcp_DReq_tready,
        ---- Stream TCP Data ------------- Nts_Tcp_
        siTOE_Data_tdata          => siSHL_Nts_Tcp_Data_tdata, 
        siTOE_Data_tkeep          => siSHL_Nts_Tcp_Data_tkeep, 
        siTOE_Data_tlast          => siSHL_Nts_Tcp_Data_tlast, 
        siTOE_Data_tvalid         => siSHL_Nts_Tcp_Data_tvalid,
        siTOE_Data_tready         => siSHL_Nts_Tcp_Data_tready,
        ---- Stream TCP Metadata ---------
        siTOE_SessId_V_V_tdata    => siSHL_Nts_Tcp_Meta_tdata, 
        siTOE_SessId_V_V_tvalid   => siSHL_Nts_Tcp_Meta_tvalid,
        siTOE_SessId_V_V_tready   => siSHL_Nts_Tcp_Meta_tready,
        
        ------------------------------------------------------
        -- TOE / RxP Ctlr Flow Interfaces
        ------------------------------------------------------
        -- FPGA Receive Path (SHELL-->ROLE) ------- :
        ---- Stream TCP Listen Request -----
        soTOE_LsnReq_V_V_tdata    => soSHL_Nts_Tcp_LsnReq_tdata, 
        soTOE_LsnReq_V_V_tvalid   => soSHL_Nts_Tcp_LsnReq_tvalid,
        soTOE_LsnReq_V_V_tready   => soSHL_Nts_Tcp_LsnReq_tready,
        ---- Stream TCP Listen -------------
        siTOE_LsnAck_V_tdata      => siSHL_Nts_Tcp_LsnAck_tdata, 
        siTOE_LsnAck_V_tvalid     => siSHL_Nts_Tcp_LsnAck_tvalid, 
        siTOE_LsnAck_V_tready     => siSHL_Nts_Tcp_LsnAck_tready, 
       
        ------------------------------------------------------
        -- TOE / TxP Data Interfaces
        ------------------------------------------------------
        ---- Stream TCP Data ------------ 
        soTOE_Data_tdata          => soSHL_Nts_Tcp_Data_tdata, 
        soTOE_Data_tkeep          => soSHL_Nts_Tcp_Data_tkeep, 
        soTOE_Data_tlast          => soSHL_Nts_Tcp_Data_tlast, 
        soTOE_Data_tvalid         => soSHL_Nts_Tcp_Data_tvalid,
        soTOE_Data_tready         => soSHL_Nts_Tcp_Data_tready,
        ---- Stream TCP Metadata ---------
        soTOE_SessId_V_V_tdata    => soSHL_Nts_Tcp_Meta_tdata,
        soTOE_SessId_V_V_tvalid   => soSHL_Nts_Tcp_Meta_tvalid,
        soTOE_SessId_V_V_tready   => soSHL_Nts_Tcp_Meta_tready,
        ---- Stream TCP Data Status ------
        siTOE_DSts_V_V_tdata      => siSHL_Nts_Tcp_DSts_tdata,
        siTOE_DSts_V_V_tvalid     => siSHL_Nts_Tcp_DSts_tvalid,
        siTOE_DSts_V_V_tready     => siSHL_Nts_Tcp_DSts_tready,
             
        ------------------------------------------------------
        -- TOE / TxP Ctlr Flow Interfaces
        ------------------------------------------------------
        -- FPGA Transmit Path (ROLE-->SHELL) ------
        ---- Stream TCP Open Session Request
        soTOE_OpnReq_V_tdata      => soSHL_Nts_Tcp_OpnReq_tdata, 
        soTOE_OpnReq_V_tvalid     => soSHL_Nts_Tcp_OpnReq_tvalid,
        soTOE_OpnReq_V_tready     => soSHL_Nts_Tcp_OpnReq_tready,
        ---- Stream TCP Open Session Status 
        siTOE_OpnRep_V_tdata      => siSHL_Nts_Tcp_OpnRep_tdata,  
        siTOE_OpnRep_V_tvalid     => siSHL_Nts_Tcp_OpnRep_tvalid,
        siTOE_OpnRep_V_tready     => siSHL_Nts_Tcp_OpnRep_tready,
        ---- Stream TCP Close Request ------
        soTOE_ClsReq_V_V_tdata    => soSHL_Nts_Tcp_ClsReq_tdata,
        soTOE_ClsReq_V_V_tvalid   => soSHL_Nts_Tcp_ClsReq_tvalid,
        soTOE_ClsReq_V_V_tready   => soSHL_Nts_Tcp_ClsReq_tready
  
      ); -- End of: TcpRoleInterface  
  
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

  gUdpAppFlashDepre : if cUDP_USE_DEPRECATED_DIRECTIVES = true generate
   
    --==========================================================================
    --==  INST: UDP-APPLICATION_FLASH for FMKU60
    --==   This version of the 'udp_app_flash' has the following interfaces:
    --==    - one bidirectionnal UDP data stream and one streaming MemoryPort. 
    --==========================================================================
    UAF : UdpApplicationFlash
      port map (
      
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        aclk                      => piSHL_156_25Clk,
        aresetn                   => (not piSHL_156_25Rst or not piSHL_Mmio_Ly7Rst),
        
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------      
        piSHL_This_MmioEchoCtrl_V => piSHL_Mmio_UdpEchoCtrl,
        --[TODO] piSHL_This_MmioPostDgmEn_V  => piSHL_Mmio_UdpPostDgmEn,
        --[TODO] piSHL_This_MmioCaptDgmEn_V  => piSHL_Mmio_UdpCaptDgmEn,
        
        --------------------------------------------------------
        -- From SHELL / Udp Data Interfaces
        --------------------------------------------------------
        siSHL_This_Data_tdata     => siSHL_Nts_Udp_Data_tdata,
        siSHL_This_Data_tkeep     => siSHL_Nts_Udp_Data_tkeep,
        siSHL_This_Data_tlast     => siSHL_Nts_Udp_Data_tlast,
        siSHL_This_Data_tvalid    => siSHL_Nts_Udp_Data_tvalid,
        siSHL_This_Data_tready    => siSHL_Nts_Udp_Data_tready,
        --------------------------------------------------------
        -- To SHELL / Udp Data Interfaces
        --------------------------------------------------------
        soTHIS_Shl_Data_tdata     => soSHL_Nts_Udp_Data_tdata,
        soTHIS_Shl_Data_tkeep     => soSHL_Nts_Udp_Data_tkeep,
        soTHIS_Shl_Data_tlast     => soSHL_Nts_Udp_Data_tlast,
        soTHIS_Shl_Data_tvalid    => soSHL_Nts_Udp_Data_tvalid,
        soTHIS_Shl_Data_tready    => soSHL_Nts_Udp_Data_tready
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
        ap_rst_n                  => (not piSHL_156_25Rst or not piSHL_Mmio_Ly7Rst),
        
        ------------------------------------------------------
        -- BLock-Level I/O Protocol
        ------------------------------------------------------
        --ap_start                => (not piSHL_156_25Rst),
        --ap_ready                => open,
        --ap_done                 => open,
        --ap_idle                 => open,
        
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------       
        piSHL_This_MmioEchoCtrl_V => piSHL_Mmio_UdpEchoCtrl,
        --[TODO] piSHL_This_MmioPostDgmEn_V  => piSHL_Mmio_UdpPostDgmEn,
        --[TODO] piSHL_This_MmioCaptDgmEn_V  => piSHL_Mmio_UdpCaptDgmEn,
        
        --------------------------------------------------------
        -- From SHELL / Udp Data Interfaces
        --------------------------------------------------------
        siSHL_This_Data_tdata     => siSHL_Nts_Udp_Data_tdata,
        siSHL_This_Data_tkeep     => siSHL_Nts_Udp_Data_tkeep,
        siSHL_This_Data_tlast     => fVectorize(siSHL_Nts_Udp_Data_tlast),
        siSHL_This_Data_tvalid    => siSHL_Nts_Udp_Data_tvalid,
        siSHL_This_Data_tready    => siSHL_Nts_Udp_Data_tready,
        --------------------------------------------------------
        -- To SHELL / Udp Data Interfaces
        --------------------------------------------------------
        soTHIS_Shl_Data_tdata     => soSHL_Nts_Udp_Data_tdata,
        soTHIS_Shl_Data_tkeep     => soSHL_Nts_Udp_Data_tkeep,
        fScalarize(soTHIS_Shl_Data_tlast) => soSHL_Nts_Udp_Data_tlast,
        soTHIS_Shl_Data_tvalid    => soSHL_Nts_Udp_Data_tvalid,
        soTHIS_Shl_Data_tready    => soSHL_Nts_Udp_Data_tready
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

  gTcpAppFlash : if cAPP_USE_DEPRECATED_DIRECTIVES = true generate
    
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
        aresetn               => (not piSHL_156_25Rst or not piSHL_Mmio_Ly7Rst),
        
         -------------------- ------------------------------------
         -- From SHELL / Mmio  Interfaces
         -------------------- ------------------------------------       
        piSHL_MmioEchoCtrl_V     => piSHL_Mmio_TcpEchoCtrl,
        piSHL_MmioPostSegEn_V    => piSHL_Mmio_TcpPostSegEn,
        --[TODO] piSHL_MmioCaptSegEn_V  => piSHL_Mmio_TcpCaptSegEn,
        
        --------------------- -----------------------------------
        -- From SHELL / Tcp D ata & Session Id Interfaces
        --------------------- -----------------------------------
        siSHL_Data_tdata      => ssTRIF_TAF_Data_tdata,
        siSHL_Data_tkeep      => ssTRIF_TAF_Data_tkeep,
        siSHL_Data_tlast      => ssTRIF_TAF_Data_tlast,
        siSHL_Data_tvalid     => ssTRIF_TAF_Data_tvalid,
        siSHL_Data_tready     => ssTRIF_TAF_Data_tready,
        --
        siSHL_SessId_tdata    => ssTRIF_TAF_Meta_tdata,
        siSHL_SessId_tvalid   => ssTRIF_TAF_Meta_tvalid,
        siSHL_SessId_tready   => ssTRIF_TAF_Meta_tready,

        --------------------- -----------------------------------
        -- To SHELL / Tcp Dat a & Session Id Interfaces
        --------------------- -----------------------------------
        soSHL_Data_tdata      => ssTAF_TRIF_Data_tdata,
        soSHL_Data_tkeep      => ssTAF_TRIF_Data_tkeep,
        soSHL_Data_tlast      => ssTAF_TRIF_Data_tlast,
        soSHL_Data_tvalid     => ssTAF_TRIF_Data_tvalid,
        soSHL_Data_tready     => ssTAF_TRIF_Data_tready,
        --
        soSHL_SessId_tdata    => ssTAF_TRIF_Meta_tdata,
        soSHL_SessId_tvalid   => ssTAF_TRIF_Meta_tvalid,
        soSHL_SessId_tready   => ssTAF_TRIF_Meta_tready
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
        ap_rst_n                 => (not piSHL_156_25Rst or not piSHL_Mmio_Ly7Rst),
        
        ------------------------------------------------------
        -- BLock-Level I/O Protocol
        ------------------------------------------------------
        --ap_start               => (not piSHL_156_25Rst),
        --ap_ready               => open,
        --ap_done                => open,
        --ap_idle                => open,
        
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------       
        piSHL_MmioEchoCtrl_V     => piSHL_Mmio_TcpEchoCtrl,
        piSHL_MmioPostSegEn_V    => piSHL_Mmio_TcpPostSegEn,
        --[TODO] piSHL_MmioCaptSegEn  => piSHL_Mmio_TcpCaptSegEn,
        
        --------------------------------------------------------
        -- From SHELL / Tcp Interfaces
        --------------------------------------------------------
        siSHL_Data_tdata         => siSHL_Nts_Tcp_Data_tdata,
        siSHL_Data_tkeep         => siSHL_Nts_Tcp_Data_tkeep,
        siSHL_Data_tlast         => siSHL_Nts_Tcp_Data_tlast,
        siSHL_Data_tvalid        => siSHL_Nts_Tcp_Data_tvalid,
        siSHL_Data_tready        => siSHL_Nts_Tcp_Data_tready,
        --
        siSHL_SessId_tdata       => siSHL_Nts_Tcp_Meta_tdata,
        siSHL_SessId_tvalid      => siSHL_Nts_Tcp_Meta_tvalid,
        siSHL_SessId_tready      => siSHL_Nts_Tcp_Meta_tready,
        
        --------------------------------------------------------
        -- To SHELL / Tcp Data Interfaces
        --------------------------------------------------------
        soSHL_Data_tdata         => soSHL_Nts_Tcp_Data_tdata,
        soSHL_Data_tkeep         => soSHL_Nts_Tcp_Data_tkeep,
        soSHL_Data_tlast         => soSHL_Nts_Tcp_Data_tlast,
        soSHL_Data_tvalid        => soSHL_Nts_Tcp_Data_tvalid,
        soSHL_Data_tready        => soSHL_Nts_Tcp_Data_tready,
        --
        soSHL_SessId_tdata       => soSHL_Nts_Tcp_Meta_tdata,
        soSHL_SessId_tvalid      => soSHL_Nts_Tcp_Meta_tvalid,
        soSHL_SessId_tready      => soSHL_Nts_Tcp_Meta_tready
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
  --#    #    #  ######  #    #  ######                                            #
  --#    ##  ##  #       ##  ##    #    ###### ###### ######                       #
  --#    # ## #  #####   # ## #    #    #      #        #                          #
  --#    #    #  #       #    #    #    ####   ######   #                          #
  --#    #    #  #       #    #    #    #           #   #                          #
  --#    #    #  ######  #    #    #    ###### ######   #                          #
  --#                                                                              #
  --################################################################################

  --OBSOLETE-20190826 sReadTlastAsVector(0)     <= siSHL_Mem_Mp0_Read_tlast;
  --OBSOLETE-20190826 soSHL_Mem_Mp0_Write_tlast <= sWriteTlastAsVector(0);
  --OBSOLETE-20190826 sResetAsVector(0)         <= piTOP_156_25Rst_delayed;

  MEM_TEST: MemTestFlash
    port map(
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                     => piSHL_156_25Clk,
      ap_rst_n                   => (not piSHL_156_25Rst or not piSHL_Mmio_Ly7Rst),
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
    );
    
    soSHL_Mem_Mp0_Write_tlast <= fScalarize(sSHL_Mem_Mp0_Write_tlast);
  
end architecture Flash;
  
