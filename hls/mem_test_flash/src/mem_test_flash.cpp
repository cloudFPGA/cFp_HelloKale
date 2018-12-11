// *****************************************************************************
// *
// *                             cloudFPGA
// *            All rights reserved -- Property of IBM
// *
// *----------------------------------------------------------------------------
// *
// * Title : Toplevel of the echo application in store-and-forward mode.
// *
// * File    : echo_store_and_forward.cpp
// *
// * Created : Apr. 2018
// * Authors : Francois Abel <fab@zurich.ibm.com>
// *
// * Devices : xcku060-ffva1156-2-i
// * Tools   : Vivado v2017.4 (64-bit)
// * Depends : None
// *
// * Description : This role application implements an echo loopback on the UDP
// *   and TCP connections. The application is said to be operating in
// *   "store-and-forward" mode because every received packet is first stored
// *   in the DDR4 before being read from that memory and being sent back.
// *
// * Infos & Remarks:
// *   The SHELL provides two logical memory ports for the ROLE to access
// *   a physical channel of the DDR4. The interfaces of these two ports enable
// *   high throughput transfer of data between the AXI4 memory-mapped domain of
// *   the DDR4 and the AXI4-Stream domains of the ROLE. The interfaces are
// *   based on an underlying AXI-DataMover-IP from Xilinx which provides
// *   specific Memory-Map-to-Stream (MM2S) and a Stream-to-Memory-Map (S2MM)
// *   channels for handling the transaction between the two domains.
// *   For more details, refer to Xilinx/LogiCORE-IP-Product-Guide (PG022).
// *
// * Conventions:
// *   <pi> stands for "PortIn".
// *   <po> stands for "PortOut".
// *   <si> stands for "StreamIn".
// *   <so> stands for "StreamOut".
// *   <si><SRC>_<Itf1>_<Itf1.1>_<Itf1.1.1>_tdata stands for the "data" signals
// *        of an Axi4-Stream generated by the source (i.e. master) "SRC", out
// *        of its interface "Itf1" and its sub-interfaces "Itf1.1" and
// *        "Itf1.1.1".
// *
// *****************************************************************************

#include "mem_test_flash.hpp"

using namespace hls;


/*****************************************************************************/
/* @brief     Counts the number of 1s an 8-bit value.
 * @ingroup   RoleEchoHls
 *
 * @param[in] keepVal is the parameter to check.
 *
 * @return    The number of bit set in the .
 *****************************************************************************/
ap_uint<4> keepToLen(ap_uint<8> keepVal) {
  ap_uint<4> count = 0;

  switch(keepVal){
    case 0x01: count = 1; break;
    case 0x03: count = 2; break;
    case 0x07: count = 3; break;
    case 0x0F: count = 4; break;
    case 0x1F: count = 5; break;
    case 0x3F: count = 6; break;
    case 0x7F: count = 7; break;
    case 0xFF: count = 8; break;
  }
  return (count);
}



/*****************************************************************************/
/* @brief   Main process of the ECHO application.
 * @ingroup RoleEchoHls
 *
 * @param[in]     siUdp is the incoming UDP stream (from SHELL/Nts)
 * @param[out]    soUdp is the outgoing UDP stream (to   SHELL/Nts)
 * @param[in]     siTcp is the incoming TCP stream (from SHELL/Nts)
 * @param[out]    soTcp is the outgoing TCP stream (to   SHELL/Nts)
 * @param[out]    soMemRdCmd0 is the outgoing memory read command for port 0
 * @param[in]     siMemRdSts0 is the incoming memory read status for port 0
 * @param[in]     siMemRead0  is the incoming data read from port 0
 * @param[out]    soMemWrCmd0 is the outgoing memory write command for port 0
 * @param[in]     siMemWrSts0 is the incoming memory write status for port 0
 * @param[out]    soMemWrite0 is the outgoing data write for port 0
 * @param[out]    soMemRdCmd1 is the outgoing memory read command for port 1
 * @param[in]     siMemRdSts1 is the incoming memory read status for port 1
 * @param[in]     siMemRead1  is the incoming data read from port 1
 * @param[out]    soMemWrCmd1 is the outgoing memory write command for port 1
 * @param[in]     siMemWrSts1 is the incoming memory write status for port 1
 * @param[out]    soMemWrite1 is the outgoing data write for port 1
 *
 * @return { description of the return value }.
 *****************************************************************************/

void mem_test_flash_main(

		// ----- system reset ---
		    ap_uint<1> sys_reset,
			// ----- MMIO ------
			ap_uint<2> DIAG_CTRL_IN,
			ap_uint<2> *DIAG_STAT_OUT,

  //------------------------------------------------------
  //-- SHELL / Role / Mem / Mp0 Interface
  //------------------------------------------------------
  //---- Read Path (MM2S) ------------
  stream<DmCmd>       &soMemRdCmdP0,
  stream<DmSts>       &siMemRdStsP0,
  stream<Axis<512 > > &siMemReadP0,
  //---- Write Path (S2MM) -----------
  stream<DmCmd>       &soMemWrCmdP0,
  stream<DmSts>       &siMemWrStsP0,
  stream<Axis<512> >  &soMemWriteP0

) {

#pragma HLS INTERFACE ap_vld register port=DIAG_CTRL_IN name=piMMIO_diag_ctrl
#pragma HLS INTERFACE ap_ovld register port=DIAG_STAT_OUT name=poMMIO_diag_stat

  // Bundling: SHELL / Role / Mem / Mp0 / Read Interface
  #pragma HLS INTERFACE axis register both port=soMemRdCmdP0
  #pragma HLS INTERFACE axis register both port=siMemRdStsP0
  #pragma HLS INTERFACE axis register both port=siMemReadP0

  #pragma HLS DATA_PACK variable=soMemRdCmdP0 instance=soMemRdCmdP0
  #pragma HLS DATA_PACK variable=siMemRdStsP0 instance=siMemRdStsP0

  // Bundling: SHELL / Role / Mem / Mp0 / Write Interface
  #pragma HLS INTERFACE axis register both port=soMemWrCmdP0
  #pragma HLS INTERFACE axis register both port=siMemWrStsP0
  #pragma HLS INTERFACE axis register both port=soMemWriteP0

  #pragma HLS DATA_PACK variable=soMemWrCmdP0 instance=soMemWrCmdP0
  #pragma HLS DATA_PACK variable=siMemWrStsP0 instance=siMemWrStsP0


  #pragma HLS INTERFACE ap_ctrl_none port=return

  #pragma HLS DATAFLOW //interval=1

  Axis<512>              memP0;
  DmSts                  memRdStsP0;
  DmSts                  memWrStsP0;

  static ap_uint<16>     cntUdpRxBytes = 0;
  static ap_uint<16>     cntTcpRxBytes = 0;
  static ap_uint<32>     cUDP_BUF_BASE_ADDR = 0x00000000; // The address of the UDP buffer in DDR4
  static ap_uint<32>     cTCP_BUF_BASE_ADDR = 0x00010000; // The address of the TCP buffer in DDR4

/*
  //------------------------------------------------------
  //-- UDP STATE MACHINE
  //------------------------------------------------------
  switch(udpState) {

  case FSM_UDP_RX_IDLE:
    if (!siUdp.empty() && !udpRxStream.full()) {
      //-- Read data from SHELL/Nts/Udp
      siUdp.read(udpWord);
      udpRxStream.write(udpWord);
      cntUdpRxBytes = cntUdpRxBytes + keepToLen(udpWord.tkeep);

      if(udpWord.tlast)
        udpState = FSM_MEM_WR_CMD_P0;
    }
    break;

  case FSM_MEM_WR_CMD_P0:
    if (!soMemWrCmdP0.full()) {
      //-- Post a memory write command to SHELL/Mem/Mp0
      soMemWrCmdP0.write(DmCmd(cUDP_BUF_BASE_ADDR , cntUdpRxBytes));
      udpState = FSM_MEM_WRITE_P0;
    }
    break;

  case FSM_MEM_WRITE_P0:
    if (!udpRxStream.empty() && !soMemWriteP0.full()) {
      //------------------------------------------------------
      //-- SHELL / Role / Mem / Mp1 Interface
      //------------------------------------------------------
      //-- Assemble a memory word and write it to DRAM
      udpRxStream.read(udpWord);
      memP0.tdata = (0x0000000000000000, 0x0000000000000000, 0x0000000000000000, udpWord.tdata(63,0));
      memP0.tkeep = (0x00, 0x00, 0x00, udpWord.tkeep);
      memP0.tlast = udpWord.tlast;
      soMemWriteP0.write(memP0);

      if (udpWord.tlast)
        udpState = FSM_MEM_WR_STS_P0;
    }
    break;

  case FSM_MEM_WR_STS_P0:
    if (!siMemWrStsP0.empty()) {
      //-- Get the memory write status for Mem/Mp0
      siMemWrStsP0.read(memWrStsP0);
      udpState = FSM_MEM_RD_CMD_P0;
    }
    break;

  case FSM_MEM_RD_CMD_P0:
    if (!soMemRdCmdP0.full()) {
      //-- Post a memory read command to SHELL/Mem/Mp0
      soMemRdCmdP0.write(DmCmd(cUDP_BUF_BASE_ADDR, cntUdpRxBytes));
      udpState = FSM_MEM_READ_P0;
    }
    break;

  case FSM_MEM_READ_P0:
    if (!siMemReadP0.empty() && !memRdP0Stream.full()) {
      //-- Read a memory word from DRAM
      siMemReadP0.read(memP0);
      udpWord.tdata(63,0) = memP0.tdata(63,0);
      udpWord.tkeep       = memP0.tkeep(7,0);
      udpWord.tlast       = memP0.tlast;
      memRdP0Stream.write(udpWord);

      if (udpWord.tlast)
        udpState = FSM_MEM_RD_STS_P0;
    }
    break;

  case FSM_MEM_RD_STS_P0:
  if (!siMemRdStsP0.empty()) {
        //-- Get the memory read status for Mem/Mp0
        siMemRdStsP0.read(memRdStsP0);
        udpState = FSM_UDP_TX;
      }
      break;

  case FSM_UDP_TX:
    if (!memRdP0Stream.empty() && !soUdp.full()) {
      //-- Write data to SHELL/Nts/Udp
      memRdP0Stream.read(udpWord);
      soUdp.write(udpWord);
      if (udpWord.tlast) {
        udpState = FSM_UDP_RX_IDLE;
        cntUdpRxBytes = 0;
      }
    }
    break;

  }  // End: switch


  //------------------------------------------------------
  //-- TCP STATE MACHINE
  //------------------------------------------------------
  switch(tcpState) {

  case FSM_TCP_RX_IDLE:
    if (!siTcp.empty() && !tcpRxStream.full()) {
      //-- Read data from SHELL/Nts/Tcp
      siTcp.read(tcpWord);
      tcpRxStream.write(tcpWord);
      cntTcpRxBytes = cntTcpRxBytes + keepToLen(tcpWord.tkeep);

      if(tcpWord.tlast)
        tcpState = FSM_MEM_WR_CMD_P1;
    }
    break;

  case FSM_MEM_WR_CMD_P1:
    if (!soMemWrCmdP1.full()) {
      //-- Post a memory write command to SHELL/Mem/Mp1
      soMemWrCmdP1.write(DmCmd(cTCP_BUF_BASE_ADDR , cntTcpRxBytes));
      tcpState = FSM_MEM_WRITE_P1;
    }
    break;

  case FSM_MEM_WRITE_P1:
    if (!tcpRxStream.empty() && !soMemWriteP1.full()) {
      //------------------------------------------------------
      ///-- SHELL / Role / Mem / Mp1 Interface
      //------------------------------------------------------
      //-- Assemble a memory word and write it to DRAM
      tcpRxStream.read(tcpWord);
      memP1.tdata = (0x0000000000000000, 0x0000000000000000, 0x0000000000000000, tcpWord.tdata(63,0));
      memP1.tkeep = (0x00, 0x00, 0x00, tcpWord.tkeep);
      memP1.tlast = tcpWord.tlast;
      soMemWriteP1.write(memP1);

      if (tcpWord.tlast)
        tcpState = FSM_MEM_WR_STS_P1;
    }
    break;

  case FSM_MEM_WR_STS_P1:
    if (!siMemWrStsP1.empty()) {
      //-- Get the memory write status for Mem/Mp1
      siMemWrStsP1.read(memWrStsP1);
      tcpState = FSM_MEM_RD_CMD_P1;
    }
    break;

  case FSM_MEM_RD_CMD_P1:
    if (!soMemRdCmdP1.full()) {
      //-- Post a memory read command to SHELL/Mem/Mp1
      soMemRdCmdP1.write(DmCmd(cTCP_BUF_BASE_ADDR, cntTcpRxBytes));
      tcpState = FSM_MEM_READ_P1;
    }
    break;

  case FSM_MEM_READ_P1:
    if (!siMemReadP1.empty() && !memRdP1Stream.full()) {
      //-- Read a memory word from DRAM
      siMemReadP1.read(memP1);
      tcpWord.tdata(63,0) = memP1.tdata(63,0);
      tcpWord.tkeep       = memP1.tkeep(7,0);
      tcpWord.tlast       = memP1.tlast;
      memRdP1Stream.write(tcpWord);

      if (tcpWord.tlast)
        tcpState = FSM_MEM_RD_STS_P1;
    }
    break;

  case FSM_MEM_RD_STS_P1:
  if (!siMemRdStsP1.empty()) {
        //-- Get the memory read status for Mem/Mp1
        siMemRdStsP1.read(memRdStsP1);
        tcpState = FSM_TCP_TX;
      }
      break;

  case FSM_TCP_TX:
    if (!memRdP1Stream.empty() && !soTcp.full()) {
      //-- Write data to SHELL/Nts/Tcp
      memRdP1Stream.read(tcpWord);
      soTcp.write(tcpWord);
      if (tcpWord.tlast) {
        tcpState = FSM_TCP_RX_IDLE;
        cntTcpRxBytes = 0;
      }
    }
    break;

  }*/
}
