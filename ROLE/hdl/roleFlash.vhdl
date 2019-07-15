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
    -- SHELL / Global Input Clock and Reset Interface
    --------------------------------------------------------
    piSHL_156_25Clk                     : in    std_ulogic;
    piSHL_156_25Rst                     : in    std_ulogic;   
    piTOP_156_25Rst_delayed             : in    std_ulogic; -- [TODO - Get rid of this delayed reset]

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
    siSHL_Nts_Tcp_DSts_tdata            : out   std_ulogic_vector( 23 downto 0);
    siSHL_Nts_Tcp_DSts_tvalid           : out   std_ulogic;
    siSHL_Nts_Tcp_DSts_tready           : in    std_ulogic;

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
    siSHL_Nts_Tcp_Notif_tdata           : out   std_ulogic_vector( 87 downto 0);
    siSHL_Nts_Tcp_Notif_tvalid          : out   std_ulogic;
    siSHL_Nts_Tcp_Notif_tready          : in    std_ulogic;
    ---- Stream TCP Data Request -------
    soSHL_Nts_Tcp_DReq_tdata            : in    std_ulogic_vector( 31 downto 0); 
    soSHL_Nts_Tcp_DReq_tvalid           : in    std_ulogic;       
    soSHL_Nts_Tcp_DReq_tready           : out   std_ulogic;

    ------------------------------------------------------
    -- SHELL / Nts / Tcp / TxP Ctlr Flow Interfaces
    ------------------------------------------------------
    -- FPGA Transmit Path (ROLE-->SHELL) ---------
    ---- Stream TCP Open Session Request
    soSHL_Nts_Tcp_OpnReq_tdata          : in    std_ulogic_vector( 47 downto 0);  
    soSHL_Nts_Tcp_OpnReq_tvalid         : in    std_ulogic;
    soSHL_Nts_Tcp_OpnReq_tready         : out   std_ulogic;
    ---- Stream TCP Open Session Status  
    siSHL_Nts_Tcp_OpnSts_tdata          : out   std_ulogic_vector( 47 downto 0); 
    siSHL_Nts_Tcp_OpnSts_tvalid         : out   std_ulogic;
    siSHL_Nts_Tcp_OpnSts_tready         : in    std_ulogic;
    ---- Stream TCP Close Request ------
    soSHL_Nts_Tcp_ClsReq_tdata          : in     std_ulogic_vector( 47 downto 0);  
    soSHL_Nts_Tcp_ClsReq_tvalid         : in     std_ulogic;
    soSHL_Nts_Tcp_ClsReq_tready         : out    std_ulogic;

    ------------------------------------------------------
    -- SHELL / Nts / Tcp / RxP Ctlr Flow Interfaces
    ------------------------------------------------------
    -- FPGA Receive Path (SHELL-->ROLE) ----------
    ---- Stream TCP Listen Request -----
    soSHL_Nts_Tcp_LsnReq_tdata          : in     std_ulogic_vector( 15 downto 0);  
    soSHL_Nts_Tcp_LsnReq_tvalid         : in     std_ulogic;
    soSHL_Nts_Tcp_LsnReq_tready         : out    std_ulogic;
    ---- Stream TCP Listen Status ----
    siSHL_Nts_Tcp_LsnAck_tdata          : out    std_ulogic_vector( 47 downto 0); 
    siSHL_Nts_Tcp_LsnAck_tvalid         : out    std_ulogic;
    siSHL_Nts_Tcp_LsnAck_tready         : in     std_ulogic;
    
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
    ------ Stream Data Input Channel ---
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
    ------ Stream Data Output Channel --
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
    ---- [DIAG_CTRL_1] -----------------
    piSHL_Mmio_Mc1_MemTestCtrl          : in    std_ulogic_vector(1 downto 0);
    ---- [DIAG_STAT_1] -----------------
    poSHL_Mmio_Mc1_MemTestStat          : out   std_ulogic_vector(1 downto 0);
    ---- [DIAG_CTRL_2] -----------------
    piSHL_Mmio_UdpEchoCtrl              : in    std_ulogic_vector(  1 downto 0);
    piSHL_Mmio_UdpPostDgmEn             : in    std_ulogic;
    piSHL_Mmio_UdpCaptDgmEn             : in    std_ulogic;
    piSHL_Mmio_TcpEchoCtrl              : in    std_ulogic_vector(  1 downto 0);
    piSHL_Mmio_TcpPostSegEn             : in    std_ulogic;
    piSHL_Mmio_TcpCaptSegEn             : in    std_ulogic;
    ---- [APP_RDROL] -------------------
    poSHL_Mmio_RdReg                    : out  std_logic_vector( 15 downto 0);
    --- [APP_WRROL] --------------------
    piSHL_Mmio_WrReg                    : in   std_logic_vector( 15 downto 0);

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

  constant cUSE_DEPRECATED_DIRECTIVES       : boolean := true;

  --============================================================================
  --  SIGNAL DECLARATIONS
  --============================================================================  

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
 
  signal EMIF_inv                       : std_logic_vector(7 downto 0);

  -- I hate Vivado HLS 
  signal sReadTlastAsVector             : std_logic_vector(0 downto 0);
  signal sWriteTlastAsVector            : std_logic_vector(0 downto 0);
  signal sResetAsVector                 : std_logic_vector(0 downto 0);

  --============================================================================
  --  VARIABLE DECLARATIONS
  --============================================================================  
  signal sUdpPostCnt                    : std_ulogic_vector(9 downto 0);
  signal sTcpPostCnt                    : std_ulogic_vector(9 downto 0);
 
  --===========================================================================
  --== COMPONENT DECLARATIONS
  --===========================================================================
  component UdpApplicationFlash is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
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
 
 
  component UdpApplicationFlashFail is
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
  end component UdpApplicationFlashFail; 

  
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
 

  component TcpApplicationFlashFail is
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
  end component TcpApplicationFlashFail; 

  component MemTestFlash is
    port (
      ap_clk                     : IN STD_LOGIC;
      ap_rst_n                   : IN STD_LOGIC;
      ap_start                   : IN STD_LOGIC;
      ap_done                    : OUT STD_LOGIC;
      ap_idle                    : OUT STD_LOGIC;
      ap_ready                   : OUT STD_LOGIC;
      piSysReset_V               : IN STD_LOGIC_VECTOR (0 downto 0);
      piSysReset_V_ap_vld        : IN STD_LOGIC;
      piMMIO_diag_ctrl_V         : IN STD_LOGIC_VECTOR (1 downto 0);
      piMMIO_diag_ctrl_V_ap_vld  : IN STD_LOGIC;
      poMMIO_diag_stat_V         : OUT STD_LOGIC_VECTOR (1 downto 0);
      poMMIO_diag_stat_V_ap_vld  : OUT STD_LOGIC;
      poDebug_V                  : OUT STD_LOGIC_VECTOR (15 downto 0);
      poDebug_V_ap_vld           : OUT STD_LOGIC;
      soMemRdCmdP0_TDATA         : OUT STD_LOGIC_VECTOR (79 downto 0);
      soMemRdCmdP0_TVALID        : OUT STD_LOGIC;
      soMemRdCmdP0_TREADY        : IN STD_LOGIC;
      siMemRdStsP0_TDATA         : IN STD_LOGIC_VECTOR (7 downto 0);
      siMemRdStsP0_TVALID        : IN STD_LOGIC;
      siMemRdStsP0_TREADY        : OUT STD_LOGIC;
      siMemReadP0_TDATA          : IN STD_LOGIC_VECTOR (511 downto 0);
      siMemReadP0_TVALID         : IN STD_LOGIC;
      siMemReadP0_TREADY         : OUT STD_LOGIC;
      siMemReadP0_TKEEP          : IN STD_LOGIC_VECTOR (63 downto 0);
      siMemReadP0_TLAST          : IN STD_LOGIC_VECTOR (0 downto 0);
      soMemWrCmdP0_TDATA         : OUT STD_LOGIC_VECTOR (79 downto 0);
      soMemWrCmdP0_TVALID        : OUT STD_LOGIC;
      soMemWrCmdP0_TREADY        : IN STD_LOGIC;
      siMemWrStsP0_TDATA         : IN STD_LOGIC_VECTOR (7 downto 0);
      siMemWrStsP0_TVALID        : IN STD_LOGIC;
      siMemWrStsP0_TREADY        : OUT STD_LOGIC;
      soMemWriteP0_TDATA         : OUT STD_LOGIC_VECTOR (511 downto 0);
      soMemWriteP0_TVALID        : OUT STD_LOGIC;
      soMemWriteP0_TREADY        : IN STD_LOGIC;
      soMemWriteP0_TKEEP         : OUT STD_LOGIC_VECTOR (63 downto 0);
      soMemWriteP0_TLAST         : OUT STD_LOGIC_VECTOR (0 downto 0) 
    );
  end component MemTestFlash;

  
  --===========================================================================
  --== FUNCTION DECLARATIONS  [TODO-Move to a package]
  --===========================================================================
  function fVectorize(s: std_logic) return std_logic_vector is
    variable v: std_logic_vector(0 downto 0);
  begin
    v(0) := s;
    return v;
  end fVectorize;
  
  function fScalarize(v: in std_logic_vector) return std_ulogic is
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
  --#    #     #  #####    ######     #####                                        #
  --#    #     #  #    #   #     #   #     # #####   #####                         #
  --#    #     #  #     #  #     #   #     # #    #  #    #                        #
  --#    #     #  #     #  ######    ####### #####   #####                         #
  --#    #     #  #    #   #         #     # #       #                             #
  --#    #######  #####    #         #     # #       #                             #
  --#                                                                              #
  --################################################################################

  gUdpAppFlashDepre : if cUSE_DEPRECATED_DIRECTIVES generate
    
    begin
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
          aresetn                   => (not piSHL_156_25Rst),
          
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
    
  end generate;

  gUdpAppFlash : if cUSE_DEPRECATED_DIRECTIVES=false generate
    begin
      --==========================================================================
      --==  INST: UDP-APPLICATION_FLASH for FMKU60
      --==   This version of the 'udp_app_flash' has the following interfaces:
      --==    - one bidirectionnal UDP data stream and one streaming MemoryPort. 
      --==========================================================================
      UAF : UdpApplicationFlashFail
        port map (
        
          ------------------------------------------------------
          -- From SHELL / Clock and Reset
          ------------------------------------------------------
          ap_clk                    => piSHL_156_25Clk,
          ap_rst_n                  => (not piSHL_156_25Rst),
          
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

  gTcpAppFlashDepre : if cUSE_DEPRECATED_DIRECTIVES generate
    
    begin
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
          aresetn               => (not piSHL_156_25Rst),
          
           -------------------- ------------------------------------
           -- From SHELL / Mmio  Interfaces
           -------------------- ------------------------------------       
          piSHL_MmioEchoCtrl_V     => piSHL_Mmio_TcpEchoCtrl,
          piSHL_MmioPostSegEn_V    => piSHL_Mmio_TcpPostSegEn,
          --[TODO] piSHL_MmioCaptSegEn_V  => piSHL_Mmio_TcpCaptSegEn,
          
          --------------------- -----------------------------------
          -- From SHELL / Tcp D ata & Session Id Interfaces
          --------------------- -----------------------------------
          siSHL_Data_tdata      => siSHL_Nts_Tcp_Data_tdata,
          siSHL_Data_tkeep      => siSHL_Nts_Tcp_Data_tkeep,
          siSHL_Data_tlast      => siSHL_Nts_Tcp_Data_tlast,
          siSHL_Data_tvalid     => siSHL_Nts_Tcp_Data_tvalid,
          siSHL_Data_tready     => siSHL_Nts_Tcp_Data_tready,
          --
          siSHL_SessId_tdata    => siSHL_Nts_Tcp_Meta_tdata,
          siSHL_SessId_tvalid   => siSHL_Nts_Tcp_Meta_tvalid,
          siSHL_SessId_tready   => siSHL_Nts_Tcp_Meta_tready,

          --------------------- -----------------------------------
          -- To SHELL / Tcp Dat a & Session Id Interfaces
          --------------------- -----------------------------------
          soSHL_Data_tdata      => soSHL_Nts_Tcp_Data_tdata,
          soSHL_Data_tkeep      => soSHL_Nts_Tcp_Data_tkeep,
          soSHL_Data_tlast      => soSHL_Nts_Tcp_Data_tlast,
          soSHL_Data_tvalid     => soSHL_Nts_Tcp_Data_tvalid,
          soSHL_Data_tready     => soSHL_Nts_Tcp_Data_tready,
          --
          soSHL_SessId_tdata    => soSHL_Nts_Tcp_Meta_tdata,
          soSHL_SessId_tvalid   => soSHL_Nts_Tcp_Meta_tvalid,
          soSHL_SessId_tready   => soSHL_Nts_Tcp_Meta_tready
        );
    
  end generate;

  gTcpAppFlash : if cUSE_DEPRECATED_DIRECTIVES=false generate
    begin
      --==========================================================================
      --==  INST: TCP-APPLICATION_FLASH for FMKU60
      --==   This version of the 'tcp_app_flash' has the following interfaces:
      --==    - one bidirectionnal TCP data stream and one streaming MemoryPort. 
      --==========================================================================
      TAF : TcpApplicationFlashFail
        port map (
        
          ------------------------------------------------------
          -- From SHELL / Clock and Reset
          ------------------------------------------------------
          ap_clk                   => piSHL_156_25Clk,
          ap_rst_n                 => (not piSHL_156_25Rst),
          
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

  sReadTlastAsVector(0)     <= siSHL_Mem_Mp0_Read_tlast;
  soSHL_Mem_Mp0_Write_tlast <= sWriteTlastAsVector(0);
  --sResetAsVector(0) <= piSHL_156_25Rst;
  --sResetAsVector(0) <= piSHL_ROL_EMIF_2B_Reg(0);
  sResetAsVector(0) <= piTOP_156_25Rst_delayed;

  MEM_TEST: MemTestFlash 
    port map(
           ap_clk                     => piSHL_156_25Clk,
           ap_rst_n                   => (not piSHL_156_25Rst),
           ap_start                   => '1',
           piSysReset_V               => sResetAsVector,
           piSysReset_V_ap_vld        => '1',
           piMMIO_diag_ctrl_V         => piSHL_Mmio_Mc1_MemTestCtrl,
           piMMIO_diag_ctrl_V_ap_vld  => '1',
           poMMIO_diag_stat_V         => poSHL_Mmio_Mc1_MemTestStat,
           poDebug_V                  => poSHL_Mmio_RdReg,
           
           soMemRdCmdP0_TDATA         => soSHL_Mem_Mp0_RdCmd_tdata,
           soMemRdCmdP0_TVALID        => soSHL_Mem_Mp0_RdCmd_tvalid,
           soMemRdCmdP0_TREADY        => soSHL_Mem_Mp0_RdCmd_tready,
           
           siMemRdStsP0_TDATA         => siSHL_Mem_Mp0_RdSts_tdata,
           siMemRdStsP0_TVALID        => siSHL_Mem_Mp0_RdSts_tvalid,
           siMemRdStsP0_TREADY        => siSHL_Mem_Mp0_RdSts_tready,
           
           siMemReadP0_TDATA          => siSHL_Mem_Mp0_Read_tdata ,
           siMemReadP0_TVALID         => siSHL_Mem_Mp0_Read_tvalid,
           siMemReadP0_TREADY         => siSHL_Mem_Mp0_Read_tready,
           siMemReadP0_TKEEP          => siSHL_Mem_Mp0_Read_tkeep,
           siMemReadP0_TLAST          => sReadTlastAsVector,
           
           soMemWrCmdP0_TDATA         => soSHL_Mem_Mp0_WrCmd_tdata ,
           soMemWrCmdP0_TVALID        => soSHL_Mem_Mp0_WrCmd_tvalid,
           soMemWrCmdP0_TREADY        => soSHL_Mem_Mp0_WrCmd_tready,
           
           siMemWrStsP0_TDATA         => siSHL_Mem_Mp0_WrSts_tdata ,
           siMemWrStsP0_TVALID        => siSHL_Mem_Mp0_WrSts_tvalid,
           siMemWrStsP0_TREADY        => siSHL_Mem_Mp0_WrSts_tready,
           
           soMemWriteP0_TDATA         => soSHL_Mem_Mp0_Write_tdata ,
           soMemWriteP0_TVALID        => soSHL_Mem_Mp0_Write_tvalid,
           soMemWriteP0_TREADY        => soSHL_Mem_Mp0_Write_tready,
           soMemWriteP0_TKEEP         => soSHL_Mem_Mp0_Write_tkeep ,
           soMemWriteP0_TLAST         => sWriteTlastAsVector
         );
  
end architecture Flash;
  
