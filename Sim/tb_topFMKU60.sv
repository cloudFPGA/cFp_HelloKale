// ----------------------------------------------------------------------------
// (c) Copyright 2014 Xilinx, Inc. All rights reserved.
//
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
//
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
//
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
//
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES.
// ----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Title      : Demo testbench
// Project    : 10G Gigabit Ethernet
//-----------------------------------------------------------------------------
// File       : TenGigEthSubSys_X1Y8_R_demo_tb.v
// Author     : Xilinx Inc.
//-----------------------------------------------------------------------------
// Description: This testbench will exercise the ports of the core to
//              demonstrate the functionality.
//-----------------------------------------------------------------------------
// This testbench performs the following operations on the MAC core:
// The clock divide register is set for MDIO operation.
// Flow control is disabled on both transmit and receive
// The client AXI Streamning port is wired as a loopback, so that transmitted frames
// are then injected into the receiver.
// Four frames are pushed into the receiver. The first is a minimum
// length frame, the second is slightly longer, the third has an error
// asserted and the fourth frame only sends 22 bytes of data: the remainder
// of the data field is padded up to the minimum frame length i.e. 46 bytes.
//
//
// These frames are then looped back and sent out by the transmitter.
//-----------------------------------------------------------------------------
//
//----------------------------------------------------------------------
//                  DEMONSTRATION TESTBENCH                            |
//                                                                     |
//                                                                     |
//     ----------------------------------------------                  |
//     |           TOP LEVEL WRAPPER (DUT)          |                  |
//     |  -------------------    ----------------   |                  |
//     |  | USER LOOPBACK/  |    | 10G          |   |                  |
//     |  | PAT GENERATOR & |    | ETHERNET     |   |                  |
//     |  | CHECKER         |    | CORE         |   |                  |
//     |  | DESIGN EXAMPLE  |    |              |   |       Monitor    |
//     |  |         ------->|--->|          Tx  |-------->  Frames     |
//     |  |         |       |    |       Serial |   |                  |
//     |  |         |       |    |          I/F |   |                  |
//     |  |         |       |    |              |   |                  |
//     |  |         |       |    |              |   |                  |
//     |  |         |       |    |              |   |                  |
//     |  |         |       |    |          Rx  |   |                  |
//     |  |         |       |    |       Serial |   |                  |
//     |  |         --------|<---|          I/F |<-------- Generate    |
//     |  |                 |    |              |   |      Frames      |
//     |  -------------------    ----------------   |                  |
//     |                               ^            |                  |
//     |                               |            |                  |
//     |                         AXI-Lite Config    |                  |
//     ----------------------------------------------                  |
//                                                                     |
//----------------------------------------------------------------------


`timescale 1ps / 1ps
`define FRAME_TYP [32*32+32*4+16:1]
// Define the refclk period in ps - in reality this should be 6400 ps = 66*96.969696..
// but that is not possible to model accurately, so use the next closest number...
`define BITPERIOD 98 // Closest even number above 96.969696..
`define PERIODCORECLK 66*98 // this is the clock for 156.25MHz based on bit period of 98ps
`define PERIODDCLK 8000 // this is the DCLK running at 125MHz
`define PERIODSAMPLECLK 66*98 // this is the coreclk running at 156.25MHz
`define LOCK_INIT 0
`define RESET_CNT 1
`define TEST_SH   2
`define PERIODEmifCLK 1700*98 // clock period 166.667 ns



// FRAME_GEN_MULTIPLIER constant determines the number of word blocks after DA/SA to be sent
// For instance if index of last complete ctrl column (eg. ctrl column[14] == 4'b0000)
// then the block size is (14 + 1) - 3 = 12.
// That means that 12 * 4 = 48 bytes are contained in one block
// If FRAME_GEN_MULTIPLIER is set to 2 then 2*12*4 = 96 bytes are sent after DA/SA
// and the same 48 byte will be sent 2 times.
// In order to get correct frames through the TYPE/LENGTH field is to be set to 2*48 - 2 = 94 (0h5E)
// in this case the general formula for LENGTH/TYPE field is as follows:
// [[(index of last complete ctrl column + 1) - 3] * 4 * FRAME_GEN_MULTIPLIER ]- 2 +
// (0,1,2 or 3 depending from the value of the ctrl column after the last complete ctrl column)
// Note that this multiplier constant is applied to every frame inserted into RX therefore the L/T field
// is to be set appropriately for every frame unless the frame is a control frame.
`define FRAME_GEN_MULTIPLIER 1


module tb_topFMKU60;

parameter TB_MODE = "DEMO_TB";
//parameter TB_MODE = "BIST";

    // Frame data....
    frame_typ_TenGigEthSubSys_X1Y8_R frame0();
    frame_typ_TenGigEthSubSys_X1Y8_R frame1();
    frame_typ_TenGigEthSubSys_X1Y8_R frame2();
    frame_typ_TenGigEthSubSys_X1Y8_R frame3();

    frame_typ_TenGigEthSubSys_X1Y8_R frame4();
    frame_typ_TenGigEthSubSys_X1Y8_R frame5();
//ARP
    frame_typ_TenGigEthSubSys_X1Y8_R frame6();
//ICMP
    frame_typ_TenGigEthSubSys_X1Y8_R frame7();
//UDP
    frame_typ_TenGigEthSubSys_X1Y8_R frame8();
    frame_typ_TenGigEthSubSys_X1Y8_R frame9();
    frame_typ_TenGigEthSubSys_X1Y8_R frame10();
    frame_typ_TenGigEthSubSys_X1Y8_R frame11();

    frame_typ_TenGigEthSubSys_X1Y8_R frame12();
    frame_typ_TenGigEthSubSys_X1Y8_R frame13();
    frame_typ_TenGigEthSubSys_X1Y8_R frame14();
    frame_typ_TenGigEthSubSys_X1Y8_R frame15();


    frame_typ_TenGigEthSubSys_X1Y8_R frameR();


    frame_typ_TenGigEthSubSys_X1Y8_R rx_stimulus_working_frame();

  // Store the frame data etc....
    initial
    begin

        frame0.data[0]  = 32'h04030201;  // <---
        frame0.data[1]  = 32'h02020605;  //    |
        frame0.data[2]  = 32'h06050403;  //    |  This part of the frame is looped
        frame0.data[3]  = 32'h55AA2E00;  //    |  if FRAME_GEN_MULTIPLIER is set to
        frame0.data[4]  = 32'hAA55AA55;  //    |  more than 1
        frame0.data[5]  = 32'h55AA55AA;  //    |
        frame0.data[6]  = 32'hAA55AA55;  //    |
        frame0.data[7]  = 32'h55AA55AA;  //    |
        frame0.data[8]  = 32'hAA55AA55;  //    |
        frame0.data[9]  = 32'h55AA55AA;  //    |
        frame0.data[10] = 32'hAA55AA55;  //    |
        frame0.data[11] = 32'h55AA55AA;  //    |
        frame0.data[12] = 32'hAA55AA55;  //    |
        frame0.data[13] = 32'h55AA55AA;  //    |
        frame0.data[14] = 32'hAA55AA55;  // <---
        frame0.data[15] = 32'h07070707;
        frame0.data[16] = 32'h07070707;
        frame0.data[17] = 32'h07070707;
        frame0.data[18] = 32'h07070707;
        frame0.data[19] = 32'h07070707;
        frame0.data[20] = 32'h07070707;
        frame0.data[21] = 32'h07070707;
        frame0.data[22] = 32'h07070707;
        frame0.data[23] = 32'h07070707;
        frame0.data[24] = 32'h07070707;
        frame0.data[25] = 32'h07070707;
        frame0.data[26] = 32'h07070707;
        frame0.data[27] = 32'h07070707;
        frame0.data[28] = 32'h07070707;
        frame0.data[29] = 32'h07070707;
        frame0.data[30] = 32'h07070707;
        frame0.data[31] = 32'h07070707;
        frame0.ctrl[0]  = 4'b0000;
        frame0.ctrl[1]  = 4'b0000;
        frame0.ctrl[2]  = 4'b0000;
        frame0.ctrl[3]  = 4'b0000;
        frame0.ctrl[4]  = 4'b0000;
        frame0.ctrl[5]  = 4'b0000;
        frame0.ctrl[6]  = 4'b0000;
        frame0.ctrl[7]  = 4'b0000;
        frame0.ctrl[8]  = 4'b0000;
        frame0.ctrl[9]  = 4'b0000;
        frame0.ctrl[10] = 4'b0000;
        frame0.ctrl[11] = 4'b0000;
        frame0.ctrl[12] = 4'b0000;
        frame0.ctrl[13] = 4'b0000;
        frame0.ctrl[14] = 4'b0000;
        frame0.ctrl[15] = 4'b1111;
        frame0.ctrl[16] = 4'b1111;
        frame0.ctrl[17] = 4'b1111;
        frame0.ctrl[18] = 4'b1111;
        frame0.ctrl[19] = 4'b1111;
        frame0.ctrl[20] = 4'b1111;
        frame0.ctrl[21] = 4'b1111;
        frame0.ctrl[22] = 4'b1111;
        frame0.ctrl[23] = 4'b1111;
        frame0.ctrl[24] = 4'b1111;
        frame0.ctrl[25] = 4'b1111;
        frame0.ctrl[26] = 4'b1111;
        frame0.ctrl[27] = 4'b1111;
        frame0.ctrl[28] = 4'b1111;
        frame0.ctrl[29] = 4'b1111;
        frame0.ctrl[30] = 4'b1111;
        frame0.ctrl[31] = 4'b1111;

        frame0.error = 1'b0;

        //Frame 1
        frame1.data[0]  = 32'h04030201;  // <---
        frame1.data[1]  = 32'h02020605;  //    |
        frame1.data[2]  = 32'h06050403;  //    |  This part of the frame is looped
        frame1.data[3]  = 32'h55AA0081;  //    |  if FRAME_GEN_MULTIPLIER is set to
        frame1.data[4]  = 32'hAA55AA55;  //    |  more than 1
        frame1.data[5]  = 32'h55AA55AA;  //    |
        frame1.data[6]  = 32'hAA55AA55;  //    |
        frame1.data[7]  = 32'h55AA55AA;  //    |
        frame1.data[8]  = 32'hAA55AA55;  //    |
        frame1.data[9]  = 32'h55AA55AA;  //    |
        frame1.data[10] = 32'hAA55AA55;  //    |
        frame1.data[11] = 32'h55AA55AA;  //    |
        frame1.data[12] = 32'hAA55AA55;  //    |
        frame1.data[13] = 32'h55AA55AA;  //    |
        frame1.data[14] = 32'hAA55AA55;  // <---
        frame1.data[15] = 32'h07070707;
        frame1.data[16] = 32'h07070707;
        frame1.data[17] = 32'h07070707;
        frame1.data[18] = 32'h07070707;
        frame1.data[19] = 32'h07070707;
        frame1.data[20] = 32'h07070707;
        frame1.data[21] = 32'h07070707;
        frame1.data[22] = 32'h07070707;
        frame1.data[23] = 32'h07070707;
        frame1.data[24] = 32'h07070707;
        frame1.data[25] = 32'h07070707;
        frame1.data[26] = 32'h07070707;
        frame1.data[27] = 32'h07070707;
        frame1.data[28] = 32'h07070707;
        frame1.data[29] = 32'h07070707;
        frame1.data[30] = 32'h07070707;
        frame1.data[31] = 32'h07070707;
        frame1.ctrl[0]  = 4'b0000;
        frame1.ctrl[1]  = 4'b0000;
        frame1.ctrl[2]  = 4'b0000;
        frame1.ctrl[3]  = 4'b0000;
        frame1.ctrl[4]  = 4'b0000;
        frame1.ctrl[5]  = 4'b0000;
        frame1.ctrl[6]  = 4'b0000;
        frame1.ctrl[7]  = 4'b0000;
        frame1.ctrl[8]  = 4'b0000;
        frame1.ctrl[9]  = 4'b0000;
        frame1.ctrl[10] = 4'b0000;
        frame1.ctrl[11] = 4'b0000;
        frame1.ctrl[12] = 4'b0000;
        frame1.ctrl[13] = 4'b0000;
        frame1.ctrl[14] = 4'b0000;
        frame1.ctrl[15] = 4'b0000;
        frame1.ctrl[16] = 4'b0000;
        frame1.ctrl[17] = 4'b0000;
        frame1.ctrl[18] = 4'b0000;
        frame1.ctrl[19] = 4'b0000;
        frame1.ctrl[20] = 4'b0000;
        frame1.ctrl[21] = 4'b1100;
        frame1.ctrl[22] = 4'b1111;
        frame1.ctrl[23] = 4'b1111;
        frame1.ctrl[24] = 4'b1111;
        frame1.ctrl[25] = 4'b1111;
        frame1.ctrl[26] = 4'b1111;
        frame1.ctrl[27] = 4'b1111;
        frame1.ctrl[28] = 4'b1111;
        frame1.ctrl[29] = 4'b1111;
        frame1.ctrl[30] = 4'b1111;
        frame1.ctrl[31] = 4'b1111;

        frame1.error = 1'b0;

        //Frame 2
        frame2.data[0]  = 32'h04030201;  // <---
        frame2.data[1]  = 32'h02020605;  //    |
        frame2.data[2]  = 32'h06050403;  //    |  This part of the frame is looped
        frame2.data[3]  = 32'h55AA3700;  //    |  if FRAME_GEN_MULTIPLIER is set to
        frame2.data[4]  = 32'hAA55AA55;  //    |  more than 1
        frame2.data[5]  = 32'h55AA55AA;  //    |
        frame2.data[6]  = 32'hAA55AA55;  //    |
        frame2.data[7]  = 32'h55AA55AA;  //    |
        frame2.data[8]  = 32'hAA55AA55;  //    |
        frame2.data[9]  = 32'h55AA55AA;  //    |
        frame2.data[10] = 32'hAA55AA55;  //    |
        frame2.data[11] = 32'h55AA55AA;  //    |
        frame2.data[12] = 32'hAA55AA55;  //    |
        frame2.data[13] = 32'h55AA55AA;  //    |
        frame2.data[14] = 32'hAA55AA55;  //    |
        frame2.data[15] = 32'h11EEbfca;  //    |
        frame2.data[16] = 32'hCBbcAEDD;  // <---
        frame2.data[17] = 32'h070707ff;
        frame2.data[18] = 32'h07070707;
        frame2.data[19] = 32'h07070707;
        frame2.data[20] = 32'h07070707;
        frame2.data[21] = 32'h07070707;
        frame2.data[22] = 32'h07070707;
        frame2.data[23] = 32'h07070707;
        frame2.data[24] = 32'h07070707;
        frame2.data[25] = 32'h07070707;
        frame2.data[26] = 32'h07070707;
        frame2.data[27] = 32'h07070707;
        frame2.data[28] = 32'h07070707;
        frame2.data[29] = 32'h07070707;
        frame2.data[30] = 32'h07070707;
        frame2.data[31] = 32'h07070707;
        frame2.ctrl[0]  = 4'b0000;
        frame2.ctrl[1]  = 4'b0000;
        frame2.ctrl[2]  = 4'b0000;
        frame2.ctrl[3]  = 4'b0000;
        frame2.ctrl[4]  = 4'b0000;
        frame2.ctrl[5]  = 4'b0000;
        frame2.ctrl[6]  = 4'b0000;
        frame2.ctrl[7]  = 4'b0000;
        frame2.ctrl[8]  = 4'b0000;
        frame2.ctrl[9]  = 4'b0000;
        frame2.ctrl[10] = 4'b0000;
        frame2.ctrl[11] = 4'b0000;
        frame2.ctrl[12] = 4'b0000;
        frame2.ctrl[13] = 4'b0000;
        frame2.ctrl[14] = 4'b0000;
        frame2.ctrl[15] = 4'b0000;
        frame2.ctrl[16] = 4'b0000;
        frame2.ctrl[17] = 4'b1110;
        frame2.ctrl[18] = 4'b1111;
        frame2.ctrl[19] = 4'b1111;
        frame2.ctrl[20] = 4'b1111;
        frame2.ctrl[21] = 4'b1111;
        frame2.ctrl[22] = 4'b1111;
        frame2.ctrl[23] = 4'b1111;
        frame2.ctrl[24] = 4'b1111;
        frame2.ctrl[25] = 4'b1111;
        frame2.ctrl[26] = 4'b1111;
        frame2.ctrl[27] = 4'b1111;
        frame2.ctrl[28] = 4'b1111;
        frame2.ctrl[29] = 4'b1111;
        frame2.ctrl[30] = 4'b1111;
        frame2.ctrl[31] = 4'b1111;

        frame2.error = 1'b0;

        //Frame 3
        frame3.data[0]  = 32'h04030201;  // <---
        frame3.data[1]  = 32'h02020605;  //    |
        frame3.data[2]  = 32'h06050403;  //    |  This part of the frame is looped
        frame3.data[3]  = 32'h55AA1600;  //    |  if FRAME_GEN_MULTIPLIER is set to
        frame3.data[4]  = 32'hAA55AA55;  //    |  more than 1
        frame3.data[5]  = 32'h55AA55AA;  //    |
        frame3.data[6]  = 32'hAA55AA55;  //    |
        frame3.data[7]  = 32'h55AA55AA;  //    |
        frame3.data[8]  = 32'hAA55AA55;  //    |
        frame3.data[9]  = 32'h55AA55AA;  //    |
        frame3.data[10] = 32'hAA55AA55;  //    |
        frame3.data[11] = 32'h55AA55AA;  //    |
        frame3.data[12] = 32'hAA55AA55;  //    |
        frame3.data[13] = 32'h55AA55AA;  //    |
        frame3.data[14] = 32'hAA55AA55;  // <---
        frame3.data[15] = 32'h07070707;
        frame3.data[16] = 32'h07070707;
        frame3.data[17] = 32'h07070707;
        frame3.data[18] = 32'h07070707;
        frame3.data[19] = 32'h07070707;
        frame3.data[20] = 32'h07070707;
        frame3.data[21] = 32'h07070707;
        frame3.data[22] = 32'h07070707;
        frame3.data[23] = 32'h07070707;
        frame3.data[24] = 32'h07070707;
        frame3.data[25] = 32'h07070707;
        frame3.data[26] = 32'h07070707;
        frame3.data[27] = 32'h07070707;
        frame3.data[28] = 32'h07070707;
        frame3.data[29] = 32'h07070707;
        frame3.data[30] = 32'h07070707;
        frame3.data[31] = 32'h07070707;
        frame3.ctrl[0]  = 4'b0000;
        frame3.ctrl[1]  = 4'b0000;
        frame3.ctrl[2]  = 4'b0000;
        frame3.ctrl[3]  = 4'b0000;
        frame3.ctrl[4]  = 4'b0000;
        frame3.ctrl[5]  = 4'b0000;
        frame3.ctrl[6]  = 4'b0000;
        frame3.ctrl[7]  = 4'b0000;
        frame3.ctrl[8]  = 4'b0000;
        frame3.ctrl[9]  = 4'b0000;
        frame3.ctrl[10] = 4'b0000;
        frame3.ctrl[11] = 4'b0000;
        frame3.ctrl[12] = 4'b0000;
        frame3.ctrl[13] = 4'b0000;
        frame3.ctrl[14] = 4'b0000;
        frame3.ctrl[15] = 4'b1111;
        frame3.ctrl[16] = 4'b1111;
        frame3.ctrl[17] = 4'b1111;
        frame3.ctrl[18] = 4'b1111;
        frame3.ctrl[19] = 4'b1111;
        frame3.ctrl[20] = 4'b1111;
        frame3.ctrl[21] = 4'b1111;
        frame3.ctrl[22] = 4'b1111;
        frame3.ctrl[23] = 4'b1111;
        frame3.ctrl[24] = 4'b1111;
        frame3.ctrl[25] = 4'b1111;
        frame3.ctrl[26] = 4'b1111;
        frame3.ctrl[27] = 4'b1111;
        frame3.ctrl[28] = 4'b1111;
        frame3.ctrl[29] = 4'b1111;
        frame3.ctrl[30] = 4'b1111;
        frame3.ctrl[31] = 4'b1111;

        frame3.error = 1'b0;

// the frames so far were looped back from ELP as MacLoopbackEn was set to '1'

        //ARP ping request inverted
        frame6.data[0]  = 32'hffffffff;  // <---
        frame6.data[1]  = 32'h16faffff;  //    |
        frame6.data[2]  = 32'h19509d3e;  //    |  This part of the frame is looped

        frame6.data[3]  = 32'h01000608;  //    |  if FRAME_GEN_MULTIPLIER is set to
        frame6.data[4]  = 32'h04060008; // more than 1
        frame6.data[5]  = 32'h16fa0100;  //    |
        frame6.data[6]  = 32'h19509d3e;  //    |
        frame6.data[7]  = 32'h01c80c0a;  //    |
        frame6.data[8]  = 32'h00000000;  //    |
        frame6.data[9]  = 32'h0c0a0000;//    |
        frame6.data[10] = 32'h000013c8;  //    |
        frame6.data[11] = 32'h07070707;  //    |
        frame6.data[12] = 32'h07070707;  //    |
        frame6.data[13] = 32'h07070707;  //    |
        frame6.data[14] = 32'h00001011;  //    |
        frame6.data[15] = 32'h12131415;  //    |
        frame6.data[16] = 32'h16171819;  // <---
        frame6.data[17] = 32'h1a1b1c1d;
        frame6.data[18] = 32'h1e1f2021;
        frame6.data[19] = 32'h22232425;
        frame6.data[20] = 32'h26272829;
        frame6.data[21] = 32'h2a2b2c2d;
        frame6.data[22] = 32'h2e2f3031;
        frame6.data[23] = 32'h32333435;
        frame6.data[24] = 32'h36370707;
        frame6.data[25] = 32'h07070707;
        frame6.data[26] = 32'h07070707;
        frame6.data[27] = 32'h07070707;
        frame6.data[28] = 32'h07070707;
        frame6.data[29] = 32'h07070707;
        frame6.data[30] = 32'h07070707;
        frame6.data[31] = 32'h07070707;
        frame6.ctrl[0]  = 4'b0000;
        frame6.ctrl[1]  = 4'b0000;
        frame6.ctrl[2]  = 4'b0000;
        frame6.ctrl[3]  = 4'b0000;
        frame6.ctrl[4]  = 4'b0000;
        frame6.ctrl[5]  = 4'b0000;
        frame6.ctrl[6]  = 4'b0000;
        frame6.ctrl[7]  = 4'b0000;
        frame6.ctrl[8]  = 4'b0000;
        frame6.ctrl[9]  = 4'b0000;
        frame6.ctrl[10] = 4'b0000;
        frame6.ctrl[11] = 4'b0000;
        frame6.ctrl[12] = 4'b0000;
        frame6.ctrl[13] = 4'b0000;
        frame6.ctrl[14] = 4'b0000;
        frame6.ctrl[15] = 4'b1111;
        frame6.ctrl[16] = 4'b1111;
        frame6.ctrl[17] = 4'b1111;
        frame6.ctrl[18] = 4'b1111;
        frame6.ctrl[19] = 4'b1111;
        frame6.ctrl[20] = 4'b1111;
        frame6.ctrl[21] = 4'b1111;
        frame6.ctrl[22] = 4'b1111;
        frame6.ctrl[23] = 4'b1111;
        frame6.ctrl[24] = 4'b1111;
        frame6.ctrl[25] = 4'b1111;
        frame6.ctrl[26] = 4'b1111;
        frame6.ctrl[27] = 4'b1111;
        frame6.ctrl[28] = 4'b1111;
        frame6.ctrl[29] = 4'b1111;
        frame6.ctrl[30] = 4'b1111;
        frame6.ctrl[31] = 4'b1111;
        frame6.error = 1'b0;

//ICMP ping
        frame7.data[0]  = 32'h003c1baa;  // <---
        frame7.data[1]  = 32'h16fa555e;  //    |
        frame7.data[2]  = 32'h19509d3e;  //    |  This part of the frame is looped

        frame7.data[3]  = 32'h00450008;  //    |  if FRAME_GEN_MULTIPLIER is set to
        frame7.data[4]  = 32'hf5be5400; // more than 1
        frame7.data[5]  = 32'h01400040;  //    |
        frame7.data[6]  = 32'h0c0a86d7;  //    |
        frame7.data[7]  = 32'h0c0a01c8;  //    |
        frame7.data[8]  = 32'h000813c8;  //    |
        frame7.data[9]  = 32'hd64b5a10;  //    |
        frame7.data[10] = 32'h210b0900;  //    |
        frame7.data[11] = 32'h00005e62;  //    |
        frame7.data[12] = 32'h746e0000;  //    |
        frame7.data[13] = 32'h00000000;  //    |
        frame7.data[14] = 32'h11100000;  //    |
        frame7.data[15] = 32'h15151312;  //    |
        frame7.data[16] = 32'h19181716;  // <---
        frame7.data[17] = 32'h1d1c1b1a;
        frame7.data[18] = 32'h21201f1e;
        frame7.data[19] = 32'h25242322;
        frame7.data[20] = 32'h29282726;
        frame7.data[21] = 32'h2d2c2b2a;
        frame7.data[22] = 32'h31302f2e;
        frame7.data[23] = 32'h35343332;
        frame7.data[24] = 32'h07073736;
        frame7.data[25] = 32'h07070707;
        frame7.data[26] = 32'h07070707;
        frame7.data[27] = 32'h07070707;
        frame7.data[28] = 32'h07070707;
        frame7.data[29] = 32'h07070707;
        frame7.data[30] = 32'h07070707;
        frame7.data[31] = 32'h07070707;
        frame7.ctrl[0]  = 4'b0000;
        frame7.ctrl[1]  = 4'b0000;
        frame7.ctrl[2]  = 4'b0000;
        frame7.ctrl[3]  = 4'b0000;
        frame7.ctrl[4]  = 4'b0000;
        frame7.ctrl[5]  = 4'b0000;
        frame7.ctrl[6]  = 4'b0000;
        frame7.ctrl[7]  = 4'b0000;
        frame7.ctrl[8]  = 4'b0000;
        frame7.ctrl[9]  = 4'b0000;
        frame7.ctrl[10] = 4'b0000;
        frame7.ctrl[11] = 4'b0000;
        frame7.ctrl[12] = 4'b0000;
        frame7.ctrl[13] = 4'b0000;
        frame7.ctrl[14] = 4'b0000;
        frame7.ctrl[15] = 4'b0000;
        frame7.ctrl[16] = 4'b0000;
        frame7.ctrl[17] = 4'b0000;
        frame7.ctrl[18] = 4'b0000;
        frame7.ctrl[19] = 4'b0000;
        frame7.ctrl[20] = 4'b0000;
        frame7.ctrl[21] = 4'b0000;
        frame7.ctrl[22] = 4'b0000;
        frame7.ctrl[23] = 4'b0000;
        frame7.ctrl[24] = 4'b1100;
        frame7.ctrl[25] = 4'b1111;
        frame7.ctrl[26] = 4'b1111;
        frame7.ctrl[27] = 4'b1111;
        frame7.ctrl[28] = 4'b1111;
        frame7.ctrl[29] = 4'b1111;
        frame7.ctrl[30] = 4'b1111;
        frame7.ctrl[31] = 4'b1111;
        frame7.error = 1'b0;

//UDP packet, francois' changes
        frame8.data[0]  = 32'h003c1baa;  // <---
        frame8.data[1]  = 32'h16fa555e;  //    |
        frame8.data[2]  = 32'h19509d3e;  //    |  This part of the frame is looped
        frame8.data[3]  = 32'h00450008;  //    |  if FRAME_GEN_MULTIPLIER is set to
        frame8.data[4]  = 32'hca2c1e00; // more than 1
        frame8.data[5]  = 32'h11400040;  //    |
        frame8.data[6]  = 32'h0c0ad869;  //    |
        frame8.data[7]  = 32'h0c0a01c8;  //    |
        frame8.data[8]  = 32'h430013c8;  //    |
        frame8.data[9]  = 32'h0a005000;  //    |
        frame8.data[10] = 32'h0a541007;  //9c5d    |
        frame8.data[11] = 32'h00000000;  //    |
        frame8.data[12] = 32'h00000000;  //    |
        frame8.data[13] = 32'h00000000;  //    |
        frame8.data[14] = 32'h00000000;  //    |
        frame8.data[15] = 32'h00000000;  //    |
        frame8.data[16] = 32'h00000000;  // <---
        frame8.data[17] = 32'h1d1c1b1a;
        frame8.data[18] = 32'h21201f1e;
        frame8.data[19] = 32'h25242322;
        frame8.data[20] = 32'h29282726;
        frame8.data[21] = 32'h2d2c2b2a;
        frame8.data[22] = 32'h31302f2e;
        frame8.data[23] = 32'h35343332;
        frame8.data[24] = 32'h07073736;
        frame8.data[25] = 32'h07070707;
        frame8.data[26] = 32'h07070707;
        frame8.data[27] = 32'h07070707;
        frame8.data[28] = 32'h07070707;
        frame8.data[29] = 32'h07070707;
        frame8.data[30] = 32'h07070707;
        frame8.data[31] = 32'h07070707;
        frame8.ctrl[0]  = 4'b0000;
        frame8.ctrl[1]  = 4'b0000;
        frame8.ctrl[2]  = 4'b0000;
        frame8.ctrl[3]  = 4'b0000;
        frame8.ctrl[4]  = 4'b0000;
        frame8.ctrl[5]  = 4'b0000;
        frame8.ctrl[6]  = 4'b0000;
        frame8.ctrl[7]  = 4'b0000;
        frame8.ctrl[8]  = 4'b0000;
        frame8.ctrl[9]  = 4'b0000;
        frame8.ctrl[10] = 4'b0000;
        frame8.ctrl[11] = 4'b0000;
        frame8.ctrl[12] = 4'b0000;
        frame8.ctrl[13] = 4'b0000;
        frame8.ctrl[14] = 4'b0000;
        frame8.ctrl[15] = 4'b1111;
        frame8.ctrl[16] = 4'b1111;
        frame8.ctrl[17] = 4'b1111;
        frame8.ctrl[18] = 4'b1111;
        frame8.ctrl[19] = 4'b1111;
        frame8.ctrl[20] = 4'b1111;
        frame8.ctrl[21] = 4'b1111;
        frame8.ctrl[22] = 4'b1111;
        frame8.ctrl[23] = 4'b1111;
        frame8.ctrl[24] = 4'b1111;
        frame8.ctrl[25] = 4'b1111;
        frame8.ctrl[26] = 4'b1111;
        frame8.ctrl[27] = 4'b1111;
        frame8.ctrl[28] = 4'b1111;
        frame8.ctrl[29] = 4'b1111;
        frame8.ctrl[30] = 4'b1111;
        frame8.ctrl[31] = 4'b1111;
        frame8.error = 1'b0;

//UDP packet
        frame9.data[0]  = 32'h003c1baa;  // <---
        frame9.data[1]  = 32'h16fa555e;  //    |
        frame9.data[2]  = 32'h19509d3e;  //    |  This part of the frame is looped
        frame9.data[3]  = 32'h00450008;  //    |  if FRAME_GEN_MULTIPLIER is set to
        frame9.data[4]  = 32'hca2c1e00; // more than 1
        frame9.data[5]  = 32'h11400040;  //    |
        frame9.data[6]  = 32'h0c0ad869;  //    |
        frame9.data[7]  = 32'h0c0a01c8;  //    |
        frame9.data[8]  = 32'h430013c8;  //    |
        frame9.data[9]  = 32'h0a005000;  //    |
        frame9.data[10] = 32'h0a540000;  //9c5d    |
        frame9.data[11] = 32'h00000000;  //    |
        frame9.data[12] = 32'h00000000;  //    |
        frame9.data[13] = 32'h00000000;  //    |
        frame9.data[14] = 32'h00000000;  //    |
        frame9.data[15] = 32'h00000000;  //    |
        frame9.data[16] = 32'h00000000;  // <---
        frame9.data[17] = 32'h1d1c1b1a;
        frame9.data[18] = 32'h21201f1e;
        frame9.data[19] = 32'h25242322;
        frame9.data[20] = 32'h29282726;
        frame9.data[21] = 32'h2d2c2b2a;
        frame9.data[22] = 32'h31302f2e;
        frame9.data[23] = 32'h35343332;
        frame9.data[24] = 32'h07073736;
        frame9.data[25] = 32'h07070707;
        frame9.data[26] = 32'h07070707;
        frame9.data[27] = 32'h07070707;
        frame9.data[28] = 32'h07070707;
        frame9.data[29] = 32'h07070707;
        frame9.data[30] = 32'h07070707;
        frame9.data[31] = 32'h07070707;
        frame9.ctrl[0]  = 4'b0000;
        frame9.ctrl[1]  = 4'b0000;
        frame9.ctrl[2]  = 4'b0000;
        frame9.ctrl[3]  = 4'b0000;
        frame9.ctrl[4]  = 4'b0000;
        frame9.ctrl[5]  = 4'b0000;
        frame9.ctrl[6]  = 4'b0000;
        frame9.ctrl[7]  = 4'b0000;
        frame9.ctrl[8]  = 4'b0000;
        frame9.ctrl[9]  = 4'b0000;
        frame9.ctrl[10] = 4'b0000;
        frame9.ctrl[11] = 4'b0000;
        frame9.ctrl[12] = 4'b0000;
        frame9.ctrl[13] = 4'b0000;
        frame9.ctrl[14] = 4'b0000;
        frame9.ctrl[15] = 4'b1111;
        frame9.ctrl[16] = 4'b1111;
        frame9.ctrl[17] = 4'b1111;
        frame9.ctrl[18] = 4'b1111;
        frame9.ctrl[19] = 4'b1111;
        frame9.ctrl[20] = 4'b1111;
        frame9.ctrl[21] = 4'b1111;
        frame9.ctrl[22] = 4'b1111;
        frame9.ctrl[23] = 4'b1111;
        frame9.ctrl[24] = 4'b1111;
        frame9.ctrl[25] = 4'b1111;
        frame9.ctrl[26] = 4'b1111;
        frame9.ctrl[27] = 4'b1111;
        frame9.ctrl[28] = 4'b1111;
        frame9.ctrl[29] = 4'b1111;
        frame9.ctrl[30] = 4'b1111;
        frame9.ctrl[31] = 4'b1111;
        frame9.error = 1'b0;

//UDP packet
        frame10.data[0]  = 32'h003c1baa;  // <---
        frame10.data[1]  = 32'h16fa555e;  //    |
        frame10.data[2]  = 32'h19509d3e;  //    |  This part of the frame is looped
        frame10.data[3]  = 32'h00450008;  //    |  if FRAME_GEN_MULTIPLIER is set to
        frame10.data[4]  = 32'hca2c1e00; // more than 1
        frame10.data[5]  = 32'h11400040;  //    |
        frame10.data[6]  = 32'h0c0ad869;  //    |
        frame10.data[7]  = 32'h0c0a01c8;  //    |
        frame10.data[8]  = 32'h430013c8;  //    |
        frame10.data[9]  = 32'h0a005000;  //    |
        frame10.data[10] = 32'h0a542007;  //9c5d    |
        frame10.data[11] = 32'h00000000;  //    |
        frame10.data[12] = 32'h00000000;  //    |
        frame10.data[13] = 32'h00000000;  //    |
        frame10.data[14] = 32'h00000000;  //    |
        frame10.data[15] = 32'h00000000;  //    |
        frame10.data[16] = 32'h00000000;  // <---
        frame10.data[17] = 32'h1d1c1b1a;
        frame10.data[18] = 32'h21201f1e;
        frame10.data[19] = 32'h25242322;
        frame10.data[20] = 32'h29282726;
        frame10.data[21] = 32'h2d2c2b2a;
        frame10.data[22] = 32'h31302f2e;
        frame10.data[23] = 32'h35343332;
        frame10.data[24] = 32'h07073736;
        frame10.data[25] = 32'h07070707;
        frame10.data[26] = 32'h07070707;
        frame10.data[27] = 32'h07070707;
        frame10.data[28] = 32'h07070707;
        frame10.data[29] = 32'h07070707;
        frame10.data[30] = 32'h07070707;
        frame10.data[31] = 32'h07070707;
        frame10.ctrl[0]  = 4'b0000;
        frame10.ctrl[1]  = 4'b0000;
        frame10.ctrl[2]  = 4'b0000;
        frame10.ctrl[3]  = 4'b0000;
        frame10.ctrl[4]  = 4'b0000;
        frame10.ctrl[5]  = 4'b0000;
        frame10.ctrl[6]  = 4'b0000;
        frame10.ctrl[7]  = 4'b0000;
        frame10.ctrl[8]  = 4'b0000;
        frame10.ctrl[9]  = 4'b0000;
        frame10.ctrl[10] = 4'b0000;
        frame10.ctrl[11] = 4'b0000;
        frame10.ctrl[12] = 4'b0000;
        frame10.ctrl[13] = 4'b0000;
        frame10.ctrl[14] = 4'b0000;
        frame10.ctrl[15] = 4'b1111;
        frame10.ctrl[16] = 4'b1111;
        frame10.ctrl[17] = 4'b1111;
        frame10.ctrl[18] = 4'b1111;
        frame10.ctrl[19] = 4'b1111;
        frame10.ctrl[20] = 4'b1111;
        frame10.ctrl[21] = 4'b1111;
        frame10.ctrl[22] = 4'b1111;
        frame10.ctrl[23] = 4'b1111;
        frame10.ctrl[24] = 4'b1111;
        frame10.ctrl[25] = 4'b1111;
        frame10.ctrl[26] = 4'b1111;
        frame10.ctrl[27] = 4'b1111;
        frame10.ctrl[28] = 4'b1111;
        frame10.ctrl[29] = 4'b1111;
        frame10.ctrl[30] = 4'b1111;
        frame10.ctrl[31] = 4'b1111;
        frame10.error = 1'b0;

//large UDP packet
        frame11.data[0]  = 32'h003c1baa;  // <---
        frame11.data[1]  = 32'h16fa555e;  //    |
        frame11.data[2]  = 32'h19509d3e;  //    |  This part of the frame is looped
        frame11.data[3]  = 32'h00450008;  //    |  if FRAME_GEN_MULTIPLIER is set to
        frame11.data[4]  = 32'hca2c2c00; // more than 1
        frame11.data[5]  = 32'h11400040;  //    |
        frame11.data[6]  = 32'h0c0aca69;  //    |
        frame11.data[7]  = 32'h0c0a01c8;  //    |
        frame11.data[8]  = 32'h430013c8;  //    |
        frame11.data[9]  = 32'h18005000;  //    |
        frame11.data[10] = 32'h68540000;  //9c5d , a85d??
        frame11.data[11] = 32'h69207369;  //    |
        frame11.data[12] = 32'h796d2073;  //    |
        frame11.data[13] = 32'h74616420;  //    |
        frame11.data[14] = 32'h00000a61;  //    |
        frame11.data[15] = 32'h00000000;  //    |
        frame11.data[16] = 32'h00000000;  // <---
        frame11.data[17] = 32'h1d1c1b1a;
        frame11.data[18] = 32'h21201f1e;
        frame11.data[19] = 32'h25242322;
        frame11.data[20] = 32'h29282726;
        frame11.data[21] = 32'h2d2c2b2a;
        frame11.data[22] = 32'h31302f2e;
        frame11.data[23] = 32'h35343332;
        frame11.data[24] = 32'h07073736;
        frame11.data[25] = 32'h07070707;
        frame11.data[26] = 32'h07070707;
        frame11.data[27] = 32'h07070707;
        frame11.data[28] = 32'h07070707;
        frame11.data[29] = 32'h07070707;
        frame11.data[30] = 32'h07070707;
        frame11.data[31] = 32'h07070707;
        frame11.ctrl[0]  = 4'b0000;
        frame11.ctrl[1]  = 4'b0000;
        frame11.ctrl[2]  = 4'b0000;
        frame11.ctrl[3]  = 4'b0000;
        frame11.ctrl[4]  = 4'b0000;
        frame11.ctrl[5]  = 4'b0000;
        frame11.ctrl[6]  = 4'b0000;
        frame11.ctrl[7]  = 4'b0000;
        frame11.ctrl[8]  = 4'b0000;
        frame11.ctrl[9]  = 4'b0000;
        frame11.ctrl[10] = 4'b0000;
        frame11.ctrl[11] = 4'b0000;
        frame11.ctrl[12] = 4'b0000;
        frame11.ctrl[13] = 4'b0000;
        frame11.ctrl[14] = 4'b0000;
        frame11.ctrl[15] = 4'b1111;
        frame11.ctrl[16] = 4'b1111;
        frame11.ctrl[17] = 4'b1111;
        frame11.ctrl[18] = 4'b1111;
        frame11.ctrl[19] = 4'b1111;
        frame11.ctrl[20] = 4'b1111;
        frame11.ctrl[21] = 4'b1111;
        frame11.ctrl[22] = 4'b1111;
        frame11.ctrl[23] = 4'b1111;
        frame11.ctrl[24] = 4'b1111;
        frame11.ctrl[25] = 4'b1111;
        frame11.ctrl[26] = 4'b1111;
        frame11.ctrl[27] = 4'b1111;
        frame11.ctrl[28] = 4'b1111;
        frame11.ctrl[29] = 4'b1111;
        frame11.ctrl[30] = 4'b1111;
        frame11.ctrl[31] = 4'b1111;
        frame11.error = 1'b0;

//artificially increased packet length of frame8
        frame12.data[0]  = 32'h003c1baa;  // <---
        frame12.data[1]  = 32'h16fa555e;  //    |
        frame12.data[2]  = 32'h19509d3e;  //    |  This part of the frame is looped
        frame12.data[3]  = 32'h00450008;  //    |  if FRAME_GEN_MULTIPLIER is set to
        frame12.data[4]  = 32'hca2c2c00; // more than 1
        frame12.data[5]  = 32'h11400040;  //    |
        frame12.data[6]  = 32'h0c0aca69;  //    |
        frame12.data[7]  = 32'h0c0a01c8;  //    |
        frame12.data[8]  = 32'h430013c8;  //    |
        frame12.data[9]  = 32'h18005000;  //    |
        frame12.data[10] = 32'h0a541007;  //9c5d    |
        frame12.data[11] = 32'h00000000;  //    |
        frame12.data[12] = 32'h00000000;  //    |
        frame12.data[13] = 32'h00000000;  //    |
        frame12.data[14] = 32'h00000000;  //    |
        frame12.data[15] = 32'h00000000;  //    |
        frame12.data[16] = 32'h00000000;  // <---
        frame12.data[17] = 32'h1d1c1b1a;
        frame12.data[18] = 32'h21201f1e;
        frame12.data[19] = 32'h25242322;
        frame12.data[20] = 32'h29282726;
        frame12.data[21] = 32'h2d2c2b2a;
        frame12.data[22] = 32'h31302f2e;
        frame12.data[23] = 32'h35343332;
        frame12.data[24] = 32'h07073736;
        frame12.data[25] = 32'h07070707;
        frame12.data[26] = 32'h07070707;
        frame12.data[27] = 32'h07070707;
        frame12.data[28] = 32'h07070707;
        frame12.data[29] = 32'h07070707;
        frame12.data[30] = 32'h07070707;
        frame12.data[31] = 32'h07070707;
        frame12.ctrl[0]  = 4'b0000;
        frame12.ctrl[1]  = 4'b0000;
        frame12.ctrl[2]  = 4'b0000;
        frame12.ctrl[3]  = 4'b0000;
        frame12.ctrl[4]  = 4'b0000;
        frame12.ctrl[5]  = 4'b0000;
        frame12.ctrl[6]  = 4'b0000;
        frame12.ctrl[7]  = 4'b0000;
        frame12.ctrl[8]  = 4'b0000;
        frame12.ctrl[9]  = 4'b0000;
        frame12.ctrl[10] = 4'b0000;
        frame12.ctrl[11] = 4'b0000;
        frame12.ctrl[12] = 4'b0000;
        frame12.ctrl[13] = 4'b0000;
        frame12.ctrl[14] = 4'b0000;
        frame12.ctrl[15] = 4'b1111;
        frame12.ctrl[16] = 4'b1111;
        frame12.ctrl[17] = 4'b1111;
        frame12.ctrl[18] = 4'b1111;
        frame12.ctrl[19] = 4'b1111;
        frame12.ctrl[20] = 4'b1111;
        frame12.ctrl[21] = 4'b1111;
        frame12.ctrl[22] = 4'b1111;
        frame12.ctrl[23] = 4'b1111;
        frame12.ctrl[24] = 4'b1111;
        frame12.ctrl[25] = 4'b1111;
        frame12.ctrl[26] = 4'b1111;
        frame12.ctrl[27] = 4'b1111;
        frame12.ctrl[28] = 4'b1111;
        frame12.ctrl[29] = 4'b1111;
        frame12.ctrl[30] = 4'b1111;
        frame12.ctrl[31] = 4'b1111;
        frame12.error = 1'b0;
//frame 12 with wrong checksum
        frame13.data[0]  = 32'h003c1baa;  // <---
        frame13.data[1]  = 32'h16fa555e;  //    |
        frame13.data[2]  = 32'h19509d3e;  //    |  This part of the frame is looped
        frame13.data[3]  = 32'h00450008;  //    |  if FRAME_GEN_MULTIPLIER is set to
        frame13.data[4]  = 32'hca2c2c00; // more than 1
        frame13.data[5]  = 32'h11400040;  //    |
        frame13.data[6]  = 32'h0c0aca69;  //    |
        frame13.data[7]  = 32'h0c0a01c8;  //    |
        frame13.data[8]  = 32'h430013c8;  //    |
        frame13.data[9]  = 32'h18005000;  //    |
        frame13.data[10] = 32'h0a542007;  //9c5d    |
        frame13.data[11] = 32'h00000000;  //    |
        frame13.data[12] = 32'h00000000;  //    |
        frame13.data[13] = 32'h00000000;  //    |
        frame13.data[14] = 32'h00000000;  //    |
        frame13.data[15] = 32'h00000000;  //    |
        frame13.data[16] = 32'h00000000;  // <---
        frame13.data[17] = 32'h1d1c1b1a;
        frame13.data[18] = 32'h21201f1e;
        frame13.data[19] = 32'h25242322;
        frame13.data[20] = 32'h29282726;
        frame13.data[21] = 32'h2d2c2b2a;
        frame13.data[22] = 32'h31302f2e;
        frame13.data[23] = 32'h35343332;
        frame13.data[24] = 32'h07073736;
        frame13.data[25] = 32'h07070707;
        frame13.data[26] = 32'h07070707;
        frame13.data[27] = 32'h07070707;
        frame13.data[28] = 32'h07070707;
        frame13.data[29] = 32'h07070707;
        frame13.data[30] = 32'h07070707;
        frame13.data[31] = 32'h07070707;
        frame13.ctrl[0]  = 4'b0000;
        frame13.ctrl[1]  = 4'b0000;
        frame13.ctrl[2]  = 4'b0000;
        frame13.ctrl[3]  = 4'b0000;
        frame13.ctrl[4]  = 4'b0000;
        frame13.ctrl[5]  = 4'b0000;
        frame13.ctrl[6]  = 4'b0000;
        frame13.ctrl[7]  = 4'b0000;
        frame13.ctrl[8]  = 4'b0000;
        frame13.ctrl[9]  = 4'b0000;
        frame13.ctrl[10] = 4'b0000;
        frame13.ctrl[11] = 4'b0000;
        frame13.ctrl[12] = 4'b0000;
        frame13.ctrl[13] = 4'b0000;
        frame13.ctrl[14] = 4'b0000;
        frame13.ctrl[15] = 4'b1111;
        frame13.ctrl[16] = 4'b1111;
        frame13.ctrl[17] = 4'b1111;
        frame13.ctrl[18] = 4'b1111;
        frame13.ctrl[19] = 4'b1111;
        frame13.ctrl[20] = 4'b1111;
        frame13.ctrl[21] = 4'b1111;
        frame13.ctrl[22] = 4'b1111;
        frame13.ctrl[23] = 4'b1111;
        frame13.ctrl[24] = 4'b1111;
        frame13.ctrl[25] = 4'b1111;
        frame13.ctrl[26] = 4'b1111;
        frame13.ctrl[27] = 4'b1111;
        frame13.ctrl[28] = 4'b1111;
        frame13.ctrl[29] = 4'b1111;
        frame13.ctrl[30] = 4'b1111;
        frame13.ctrl[31] = 4'b1111;
        frame13.error = 1'b0;


//frame sent Francois..wrong IP packet length
       frame14.data[0]  = 32'h003c1baa;  // <---
        frame14.data[1]  = 32'h16fa555e;  //    |
        frame14.data[2]  = 32'h19509d3e;  //    |  This part of the frame is looped
        frame14.data[3]  = 32'h00450008;  //    |  if FRAME_GEN_MULTIPLIER is set to
        frame14.data[4]  = 32'hca2c2c00; // more than 1
        frame14.data[5]  = 32'h11400040;  //    |
        frame14.data[6]  = 32'h0c0aca69;  //    |
        frame14.data[7]  = 32'h0c0a01c8;  //    |
        frame14.data[8]  = 32'h430013c8;  //    |
        frame14.data[9]  = 32'h0a005000;  //    |
        frame14.data[10] = 32'h0a542007;  //9c5d    |
        frame14.data[11] = 32'h00000000;  //    |
        frame14.data[12] = 32'h00000000;  //    |
        frame14.data[13] = 32'h00000000;  //    |
        frame14.data[14] = 32'h00000000;  //    |
        frame14.data[15] = 32'h00000000;  //    |
        frame14.data[16] = 32'h00000000;  // <---
        frame14.data[17] = 32'h1d1c1b1a;
        frame14.data[18] = 32'h21201f1e;
        frame14.data[19] = 32'h25242322;
        frame14.data[20] = 32'h29282726;
        frame14.data[21] = 32'h2d2c2b2a;
        frame14.data[22] = 32'h31302f2e;
        frame14.data[23] = 32'h35343332;
        frame14.data[24] = 32'h07073736;
        frame14.data[25] = 32'h07070707;
        frame14.data[26] = 32'h07070707;
        frame14.data[27] = 32'h07070707;
        frame14.data[28] = 32'h07070707;
        frame14.data[29] = 32'h07070707;
        frame14.data[30] = 32'h07070707;
        frame14.data[31] = 32'h07070707;
        frame14.ctrl[0]  = 4'b0000;
        frame14.ctrl[1]  = 4'b0000;
        frame14.ctrl[2]  = 4'b0000;
        frame14.ctrl[3]  = 4'b0000;
        frame14.ctrl[4]  = 4'b0000;
        frame14.ctrl[5]  = 4'b0000;
        frame14.ctrl[6]  = 4'b0000;
        frame14.ctrl[7]  = 4'b0000;
        frame14.ctrl[8]  = 4'b0000;
        frame14.ctrl[9]  = 4'b0000;
        frame14.ctrl[10] = 4'b0000;
        frame14.ctrl[11] = 4'b0000;
        frame14.ctrl[12] = 4'b0000;
        frame14.ctrl[13] = 4'b0000;
        frame14.ctrl[14] = 4'b0000;
        frame14.ctrl[15] = 4'b1111;
        frame14.ctrl[16] = 4'b1111;
        frame14.ctrl[17] = 4'b1111;
        frame14.ctrl[18] = 4'b1111;
        frame14.ctrl[19] = 4'b1111;
        frame14.ctrl[20] = 4'b1111;
        frame14.ctrl[21] = 4'b1111;
        frame14.ctrl[22] = 4'b1111;
        frame14.ctrl[23] = 4'b1111;
        frame14.ctrl[24] = 4'b1111;
        frame14.ctrl[25] = 4'b1111;
        frame14.ctrl[26] = 4'b1111;
        frame14.ctrl[27] = 4'b1111;
        frame14.ctrl[28] = 4'b1111;
        frame14.ctrl[29] = 4'b1111;
        frame14.ctrl[30] = 4'b1111;
        frame14.ctrl[31] = 4'b1111;
        frame14.error = 1'b0;

        frame15.data[0]  = 32'h003c1baa;  // <---
        frame15.data[1]  = 32'h16fa555e;  //    |
        frame15.data[2]  = 32'h19509d3e;  //    |  This part of the frame is looped
        frame15.data[3]  = 32'h00450008;  //    |  if FRAME_GEN_MULTIPLIER is set to
        frame15.data[4]  = 32'hca2c2c00; // more than 1
        frame15.data[5]  = 32'h11400040;  //    |
        frame15.data[6]  = 32'h0c0aca69;  //    |
        frame15.data[7]  = 32'h0c0a01c8;  //    |
        frame15.data[8]  = 32'h430013c8;  //    |
        frame15.data[9]  = 32'h18005000;  //    |
        frame15.data[10] = 32'h68543cb9;  //9c5d , a85d??
        frame15.data[11] = 32'h69207369;  //    |
        frame15.data[12] = 32'h796d2073;  //    |
        frame15.data[13] = 32'h74616420;  //    |
        frame15.data[14] = 32'h00000a61;  //    |
        frame15.data[15] = 32'h00000000;  //    |
        frame15.data[16] = 32'h00000000;  // <---
        frame15.data[17] = 32'h1d1c1b1a;
        frame15.data[18] = 32'h21201f1e;
        frame15.data[19] = 32'h25242322;
        frame15.data[20] = 32'h29282726;
        frame15.data[21] = 32'h2d2c2b2a;
        frame15.data[22] = 32'h31302f2e;
        frame15.data[23] = 32'h35343332;
        frame15.data[24] = 32'h07073736;
        frame15.data[25] = 32'h07070707;
        frame15.data[26] = 32'h07070707;
        frame15.data[27] = 32'h07070707;
        frame15.data[28] = 32'h07070707;
        frame15.data[29] = 32'h07070707;
        frame15.data[30] = 32'h07070707;
        frame15.data[31] = 32'h07070707;
        frame15.ctrl[0]  = 4'b0000;
        frame15.ctrl[1]  = 4'b0000;
        frame15.ctrl[2]  = 4'b0000;
        frame15.ctrl[3]  = 4'b0000;
        frame15.ctrl[4]  = 4'b0000;
        frame15.ctrl[5]  = 4'b0000;
        frame15.ctrl[6]  = 4'b0000;
        frame15.ctrl[7]  = 4'b0000;
        frame15.ctrl[8]  = 4'b0000;
        frame15.ctrl[9]  = 4'b0000;
        frame15.ctrl[10] = 4'b0000;
        frame15.ctrl[11] = 4'b0000;
        frame15.ctrl[12] = 4'b0000;
        frame15.ctrl[13] = 4'b0000;
        frame15.ctrl[14] = 4'b0000;
        frame15.ctrl[15] = 4'b1111;
        frame15.ctrl[16] = 4'b1111;
        frame15.ctrl[17] = 4'b1111;
        frame15.ctrl[18] = 4'b1111;
        frame15.ctrl[19] = 4'b1111;
        frame15.ctrl[20] = 4'b1111;
        frame15.ctrl[21] = 4'b1111;
        frame15.ctrl[22] = 4'b1111;
        frame15.ctrl[23] = 4'b1111;
        frame15.ctrl[24] = 4'b1111;
        frame15.ctrl[25] = 4'b1111;
        frame15.ctrl[26] = 4'b1111;
        frame15.ctrl[27] = 4'b1111;
        frame15.ctrl[28] = 4'b1111;
        frame15.ctrl[29] = 4'b1111;
        frame15.ctrl[30] = 4'b1111;
        frame15.ctrl[31] = 4'b1111;
        frame15.error = 1'b0;






    end // initialise the frame contents


/*  function bit [15:0]  sum16;
     input [15:0] data_in_1;
     input [15:0] data_in_2;
     
     output [15:0] crc;
  begin
      carry = 1'b0;
      for (i=0; i< 16; i++) begin
        crc[i] = data_in_1[i] ^ data_in_2[i] ^ carry;
        carry = (data_in_1[i] & data_in_2[i]) | ((data_in_1[i] ^ data_in_2[i]) & carry);
      end 
      if(carry == 1'b1) begin
        return sum16({16'b0000000000000001}, crc);
      end
      return crc;
  endfunction
*/
  //--------------------------------------------------------------------
  // CRC engine
  //--------------------------------------------------------------------
  task calc_crc;
     input  [2 :0] size;
     input  [31:0] data_in;
     inout  [31:0] fcs;

     reg    [31:0] crc;
     reg           crc_feedback;

     reg    [7:0]  data [0:3];
     integer       I,J;
  begin
     data[0] = data_in[7:0];
     data[1] = data_in[15:8];
     data[2] = data_in[23:16];
     data[3] = data_in[31:24];

     crc = ~ fcs;
     for (J = 0; J < size; J = J + 1)
     begin
        for (I = 0; I < 8; I = I + 1)
        begin
          crc_feedback = crc[0] ^ data[J][I];

          crc[0]       = crc[1];
          crc[1]       = crc[2];
          crc[2]       = crc[3];
          crc[3]       = crc[4];
          crc[4]       = crc[5];
          crc[5]       = crc[6]  ^ crc_feedback;
          crc[6]       = crc[7];
          crc[7]       = crc[8];
          crc[8]       = crc[9]  ^ crc_feedback;
          crc[9]       = crc[10] ^ crc_feedback;
          crc[10]      = crc[11];
          crc[11]      = crc[12];
          crc[12]      = crc[13];
          crc[13]      = crc[14];
          crc[14]      = crc[15];
          crc[15]      = crc[16] ^ crc_feedback;
          crc[16]      = crc[17];
          crc[17]      = crc[18];
          crc[18]      = crc[19];
          crc[19]      = crc[20] ^ crc_feedback;
          crc[20]      = crc[21] ^ crc_feedback;
          crc[21]      = crc[22] ^ crc_feedback;
          crc[22]      = crc[23];
          crc[23]      = crc[24] ^ crc_feedback;
          crc[24]      = crc[25] ^ crc_feedback;
          crc[25]      = crc[26];
          crc[26]      = crc[27] ^ crc_feedback;
          crc[27]      = crc[28] ^ crc_feedback;
          crc[28]      = crc[29];
          crc[29]      = crc[30] ^ crc_feedback;
          crc[30]      = crc[31] ^ crc_feedback;
          crc[31]      =           crc_feedback;
        end
     end

     fcs = ~ crc;

     end
  endtask // calc_crc

  //--------------------------------------------------------------------
  // Testbench signals
  //--------------------------------------------------------------------
  reg         reset;

  reg         Emifclk;
  reg EmifCs_n;
  reg EmifOe_n;

  reg EmifWe_n;
  reg [7:0] EmifAddr;
  wire [7:0] EmifData_dut;
  reg [7:0] EmifData;

  reg         sysclk;
  reg         sim_speedup_control_pulse;
  reg         refclk;
  wire        coreclk_out;
  reg         sampleclk;
  reg         bitclk;
  reg         memclk;

  reg         tx_monitor_finished = 1'b0;
  reg         tx_monitor_error = 1'b0;
  wire        simulation_finished;
  wire        simulation_error;

  wire        frame_error;
  wire        core_ready;
  reg         tx_monitor_block_lock;
  wire        reset_error;

  wire        txp;
  wire        txn;
  wire        rxp_dut;
  wire        rxn_dut;
  wire        enable_pat_gen;
  wire        enable_pat_check;
  reg         rxp;
  wire        rxn;

  reg         test_sh = 0;
  reg         slip = 0;
  integer     BLSTATE;
  integer     next_blstate;
  reg [65:0]  RxD;
  reg [65:0]  RxD_aligned;
  integer     nbits = 0;
  integer     sh_cnt;
  integer     sh_invalid_cnt;
  integer     i;

  // To aid the TX data checking code
  reg in_a_frame = 0;
  reg write_loopback_reg = 0;
  reg loopback_disabled = 0;
  reg set_mac_ip_address = 0; 
  reg [15:0] oldsum;
  reg [15:0] newsum;
  reg [15:0] checksum;

  reg sent_ARPframe = 1'b0;
  integer placeholderARP = 0;

  integer frame7column_index; 
 integer placeholder;

   task sum16;
     input [15:0] data_in_1;
     input [15:0] data_in_2;
     
     inout [15:0] crc;
     reg carry1;
     reg[15:0] temp; 
   begin
      carry1 = 1'b0;
      for (i=0; i< 16; i++) begin
        crc[i] = data_in_1[i] ^ data_in_2[i] ^ carry1;
        carry1 = (data_in_1[i] & data_in_2[i]) | ((data_in_1[i] ^ data_in_2[i]) & carry1);
      end 
      if(carry1 === 1'b1) begin
      temp = crc;
         sum16(16'h1, temp, crc);
      end
   end
   endtask

/*   task checkICMPresponse;
   input  `FRAME_TYP frame1;

   reg [47:0] fpgaMAC;
   reg [47:0] fpgaMACfromARP;
   reg [47:0] hostMAC;
   reg [47:0] hostMACfromARP;

 reg [15:0] frametype;
 reg [15:0] hardwaretype ;
 reg [15:0] protocoltype
 reg [15:0] opcode;
 reg [31:0] fpgaIPfromARP;
 reg [31:0] hostIPfromARP;
   begin
     rx_stimulus_working_frame.frombits(frame1);
     hostMAC =  {rx_stimulus_working_frame.data[0][7:0], rx_stimulus_working_frame.data[0][15:8], rx_stimulus_working_frame.data[0][23:16], rx_stimulus_working_frame.data[0][31:24], rx_stimulus_working_frame.data[1][7:0],rx_stimulus_working_frame.data[1][15:8]}; 
     if (hostMAC != 48'hfa163e9d5019)
       $display("ICMP Packet response HOST MAC addres incorrect! Expecting fa163e9d5019, Received %h", hostMAC);

     fpgaMAC = {rx_stimulus_working_frame.data[1][23:16],rx_stimulus_working_frame.data[1][31:24], rx_stimulus_working_frame.data[2][7:0],  rx_stimulus_working_frame.data[2][15:8], rx_stimulus_working_frame.data[2][23:16], rx_stimulus_working_frame.data[2][31:24]};

     if (fpgaMAC != 48'haa1b3c005e55)
       $display("ICMP Packet response FPGA MAC addres incorrect! Expecting aa1b3c005e55, Received %h", fpgaMAC);

    frametype = {rx_stimulus_working_frame.data[3][7:0], rx_stimulus_working_frame[3].data[15:8]}
    if ({frametype!= 16'h0800)
       $display("Packet type is not IPv4, expecting 0x0800, received %h", frametype) ;
 
    protocoltype = rx_stimulus_working_frame.data[5][31:23];
    if ({protocoltype!= 8'h01)
       $display("Protocol is not ICMP, expecting 0x01, received %h", protocoltype) ;


    fpgaIP = {rx_stimulus_working_frame.data[5][23:16],rx_stimulus_working_frame.data[5][31:24], rx_stimulus_working_frame.data[6][7:0],  rx_stimulus_working_frame.data[6][15:8], rx_stimulus_working_frame.data[6][23:16], rx_stimulus_working_frame.data[6][31:24]};
     if (fpgaIP != 32'h0a0cc813)
       $display("ICMP Packet response Sender/FPGA IP addres incorrect! Expecting 0x0a0cc813, Received %h", fpgaIP);

     hostIP = {rx_stimulus_working_frame[7][7:0], rx_stimulus_working_frame[7][15:8], rx_stimulus_working_frame[7][23:16],  rx_stimulus_working_frame[7][31:24]};

     if( fpgaIPfromARP != 32h'0a0cc813)
       $display("ARP Packet response sender/fpga IP addres incorrect! Expecting 0x0a0cc813, Received %h", hostIPfromARP);

     hostMACfromARP =  {rx_stimulus_working_frame.data[8][7:0], rx_stimulus_working_frame.data[8][15:8], rx_stimulus_working_frame.data[8][23:16], rx_stimulus_working_frame.data[8][31:24], rx_stimulus_working_frame.data[9][7:0],rx_stimulus_working_frame.data[5][15:8]}; 
     if (hostMACfromARP != 48'hfa163e9d5019)
       $display("ARP Packet response target/host MAC addres incorrect! Expecting 0xfa163e9d5019, Received %h", hostMACfromARP);

     hostIPfromARP = {rx_stimulus_working_frame[9][23:16], rx_stimulus_working_frame[9][31:24], rx_stimulus_working_frame[10][7:0],  rx_stimulus_working_frame[10][15:8]};

     if( fpgaIPfromARP != 32h'0a0cc801)
       $display("ARP Packet response sender/fpga IP addres incorrect! Expecting 0x0a0cc801, Received %h", hostIPfromARP);

   end 
   endtask
 */
   task checkARPresponse;
   input  `FRAME_TYP frame1;

   reg [47:0] fpgaMAC;
   reg [47:0] fpgaMACfromARP;
   reg [47:0] hostMAC;
   reg [47:0] hostMACfromARP;

 reg [15:0] frametype;
 reg [15:0] hardwaretype ;
 reg [15:0] protocoltype;
 reg [15:0] opcode;
 reg [31:0] fpgaIPfromARP;
 reg [31:0] hostIPfromARP;
   begin
     rx_stimulus_working_frame.frombits(frame1);
     hostMAC =  {rx_stimulus_working_frame.data[0][7:0], rx_stimulus_working_frame.data[0][15:8], rx_stimulus_working_frame.data[0][23:16], rx_stimulus_working_frame.data[0][31:24], rx_stimulus_working_frame.data[1][7:0],rx_stimulus_working_frame.data[1][15:8]}; 
     if (hostMAC != 48'hfa163e9d5019) 
       $display("ARP Packet response HOST MAC addres incorrect!!! Expecting fa163e9d5019, Received %h", hostMAC);
     else
       $display("ARP Packet response HOST MAC addres correct! Expecting fa163e9d5019, Received %h", hostMAC);
    
     fpgaMAC = {rx_stimulus_working_frame.data[1][23:16],rx_stimulus_working_frame.data[1][31:24], rx_stimulus_working_frame.data[2][7:0],  rx_stimulus_working_frame.data[2][15:8], rx_stimulus_working_frame.data[2][23:16], rx_stimulus_working_frame.data[2][31:24]};

     if (fpgaMAC != 48'haa1b3c005e55)
       $display("ARP Packet response FPGA MAC addres incorrect!!! Expecting aa1b3c005e55, Received %h", fpgaMAC);
     else
       $display("ARP Packet response FPGA MAC addres correct! Expecting aa1b3c005e55, Received %h", fpgaMAC);

    frametype = {rx_stimulus_working_frame.data[3][7:0], rx_stimulus_working_frame.data[3][15:8]};
    if (frametype != 16'h0806)
       $display("Packet typ is not ARP, expecting 0x0806, received %h", frametype) ;
    else
       $display("Packet typ is ARP, expecting 0x0806, received %h", frametype) ;

    hardwaretype = {rx_stimulus_working_frame.data[3][23:16], rx_stimulus_working_frame.data[3][31:24]};
    if (hardwaretype != 16'h0001)
       $display("Hardware typ is not Ethernet, expecting 0x0001, received %h", hardwaretype) ;
    else
       $display("Hardware typ is Ethernet, expecting 0x0001, received %h", hardwaretype) ;

    protocoltype = {rx_stimulus_working_frame.data[4][7:0], rx_stimulus_working_frame.data[4][15:8]};
    if (protocoltype != 16'h0800)
       $display("Hardware typ is not IPv4, expecting 0x0800, received %h", protocoltype) ;
    else
       $display("Hardware typ is IPv4, expecting 0x0800, received %h", protocoltype) ;

    opcode = {rx_stimulus_working_frame.data[5][7:0], rx_stimulus_working_frame.data[5][15:8]};
    if ( opcode != 16'h0002)
       $display("Opcode does not correspond to reply, expecting 0x0002, received %h", opcode) ;
    else
       $display("Opcode corresponds to reply, expecting 0x0002, received %h", opcode) ;


    fpgaMACfromARP = {rx_stimulus_working_frame.data[5][23:16],rx_stimulus_working_frame.data[5][31:24], rx_stimulus_working_frame.data[6][7:0],  rx_stimulus_working_frame.data[6][15:8], rx_stimulus_working_frame.data[6][23:16], rx_stimulus_working_frame.data[6][31:24]};
     if (fpgaMACfromARP != 48'haa1b3c005e55)
       $display("ARP Packet response Sender/FPGA MAC address incorrect!!! Expecting 0xaa1b3c005e55, Received %h", fpgaMACfromARP);
    else
       $display("ARP Packet response Sender/FPGA MAC address correct! Expecting 0xaa1b3c005e55, Received %h", fpgaMACfromARP);

     fpgaIPfromARP = {rx_stimulus_working_frame.data[7][7:0],   rx_stimulus_working_frame.data[7][15:8], 
                      rx_stimulus_working_frame.data[7][23:16], rx_stimulus_working_frame.data[7][31:24]};

     if( fpgaIPfromARP != 32'h0a0cc813)
       $display("ARP Packet response sender/fpga IP addres incorrect!!! Expecting 0x0a0cc813, Received %h", hostIPfromARP);
     else
       $display("ARP Packet response sender/fpga IP addres correct! Expecting 0x0a0cc813, Received %h", hostIPfromARP);

     hostMACfromARP =  {rx_stimulus_working_frame.data[8][7:0], rx_stimulus_working_frame.data[8][15:8], rx_stimulus_working_frame.data[8][23:16], rx_stimulus_working_frame.data[8][31:24], rx_stimulus_working_frame.data[9][7:0],rx_stimulus_working_frame.data[5][15:8]}; 
     if (hostMACfromARP != 48'hfa163e9d5019)
       $display("ARP Packet response target/host MAC addres incorrect!!! Expecting 0xfa163e9d5019, Received %h", hostMACfromARP);
     else
       $display("ARP Packet response target/host MAC addres correct! Expecting 0xfa163e9d5019, Received %h", hostMACfromARP);

     hostIPfromARP = {rx_stimulus_working_frame.data[9][23:16], rx_stimulus_working_frame.data[9][31:24], rx_stimulus_working_frame.data[10][7:0],  rx_stimulus_working_frame.data[10][15:8]};

     if( hostIPfromARP != 32'h0a0cc801)
       $display("ARP Packet response sender/fpga IP address incorrect!!! Expecting 0x0a0cc801, Received %h", hostIPfromARP);
     else
       $display("ARP Packet response sender/fpga IP address correct! Expecting 0x0a0cc801, Received %h", hostIPfromARP);


   end 
   endtask
   
   task CalchecksumsUDP;
     input `FRAME_TYP frame1;
     output [15:0] UDPcheck;
     output [15:0] IPcheck;
     input integer frameno;

     reg [15:0] oldsum1;
     reg [15:0] newsum1;
     reg [15:0] checksum1;
     integer frame7column_index1; 

   begin

           rx_stimulus_working_frame.frombits(frame1);
     
           //claculate IP header checksum for frame
           sum16(rx_stimulus_working_frame.data[3][31:16], rx_stimulus_working_frame.data[4][15:0], newsum1);
           oldsum1 = newsum1;
           sum16( oldsum1, rx_stimulus_working_frame.data[4][31:16], newsum1);
           oldsum1 = newsum1;
           sum16( oldsum1, rx_stimulus_working_frame.data[5][15:0], newsum1);
           oldsum1 = newsum1;
           sum16( oldsum1, rx_stimulus_working_frame.data[5][31:16], newsum1);
           oldsum1 = newsum1;
           sum16( oldsum1, rx_stimulus_working_frame.data[6][31:16], newsum1);
           oldsum1 = newsum1;
           sum16( oldsum1, rx_stimulus_working_frame.data[7][15:0], newsum1);
           oldsum1 = newsum1;
           sum16( oldsum1, rx_stimulus_working_frame.data[7][31:16], newsum1);
           oldsum1 = newsum1;
           sum16( oldsum1, rx_stimulus_working_frame.data[8][15:0], newsum1);
 
           checksum1 =~ newsum1; 
           IPcheck = checksum1;
           $display("IP checksum for frame %d is %h %h", frameno, newsum1, checksum1);
 

           //calculate UDP checksum for frame
           
           frame7column_index1 = 11;
           newsum1 = 16'd0;
           oldsum1 = 16'd0;
           sum16(oldsum1, rx_stimulus_working_frame.data[6][31:16], newsum1);
           oldsum1 = newsum1;
           sum16(oldsum1, rx_stimulus_working_frame.data[7][15:0], newsum1);
           oldsum1 = newsum1;
           sum16(oldsum1, rx_stimulus_working_frame.data[7][31:16], newsum1);
           oldsum1 = newsum1;
           sum16(oldsum1, rx_stimulus_working_frame.data[8][15:0], newsum1);
           oldsum1 = newsum1;
           sum16(oldsum1, rx_stimulus_working_frame.data[8][31:16], newsum1);
           oldsum1 = newsum1;
           sum16(oldsum1, rx_stimulus_working_frame.data[9][31:16], newsum1);
           oldsum1 = newsum1;
           sum16(oldsum1, rx_stimulus_working_frame.data[9][15:0], newsum1);
           oldsum1 = newsum1;
           sum16(oldsum1, rx_stimulus_working_frame.data[10][31:16], newsum1);
           oldsum1 = newsum1;
           sum16(oldsum1, {rx_stimulus_working_frame.data[5][31:24], 8'd0} , newsum1);
           oldsum1 = newsum1;
           sum16(oldsum1, rx_stimulus_working_frame.data[9][31:16] , newsum1);



           while(frame7column_index1 <= 14) begin
            oldsum1 = newsum1;
            sum16(oldsum1, rx_stimulus_working_frame.data[frame7column_index1][15:0], newsum1);
            oldsum1 = newsum1;
             sum16(oldsum1, rx_stimulus_working_frame.data[frame7column_index1][31:16], newsum1);
            frame7column_index1 = frame7column_index1 + 1;
           end
           checksum1 =~ newsum1; 
           UDPcheck = checksum1;
           $display("UDP checksum for frame %d is %h %h", frameno, newsum1, checksum1);
  end
  endtask



// select between loopback or local data
  assign rxp_dut  = (TB_MODE == "BIST") ? txp : rxp;
  assign rxn_dut  = (TB_MODE == "BIST") ? txn : rxn;

  // example design pattern generator and checker modules
  // are enabled only when Built In Self Test is selected
  assign enable_pat_gen    = (TB_MODE == "BIST") ? 1'b1 : 1'b0;
  assign enable_pat_check  = (TB_MODE == "BIST") ? 1'b1 : 1'b0;

  assign reset_error = 1'b0;

//  reg send_rx_stimulus = 1'b0;
  /*---------------------------------------------------------------------------
  -- wire up Device Under Test (dut)
  ---------------------------------------------------------------------------*/
  /*
   TenGigEthSubSys_X1Y8_R_example_design dut
    (
      .reset                  (reset),
      .reset_error            (reset_error),

      .refclk_p               (refclk),         // Transcevier reference clock source
      .refclk_n               (!refclk),
      .coreclk_out            (coreclk_out),

      .clk_in_p               (sysclk),         // Freerunning clock source
      .clk_in_n               (!sysclk),

      .enable_custom_preamble (1'b0),

      .pcs_loopback           (1'b0),
      .insert_error           (1'b0),

      .enable_pat_gen         (enable_pat_gen),
      .enable_pat_check       (enable_pat_check),

      .frame_error            (frame_error),
      .gen_active_flash       (),
      .sim_speedup_control    (sim_speedup_control_pulse),
      .check_active_flash     (),
      .qplllock_out           (),
      .serialized_stats       (),
      .core_ready             (core_ready),
      .txp                    (txp),
      .txn                    (txn),
      .rxp                    (rxp_dut),
      .rxn                    (rxn_dut)
   );
*/

topFMKU60
  #(
    // Synthesis parameters ----------------------
    .gBitstreamUsage("flash"),  // "user" or "flash"
    .gSecurityPriviledges("super"),  // "user" or "super"
    // Build date --------------------------------
    .gTopDateYear(0),   //  Not used w/ Xilinx parts (see USR_ACCESSE2)
    .gTopDateMonth(0),  //  Not used w/ Xilinx parts (see USR_ACCESSE2)
    .gTopDateDay(0),    //  Not used w/ Xilinx parts (see USR_ACCESSE2)
    // External Memory Interface (EMIF) ----------
    .gEmifAddrWidth(8),
    .gEmifDataWidth(8)
  )
  IDUT (
    //----------------------------------------------------
    // PSOC / FPGA Configuration Interface (Fcfg)
    //  System reset controlled by the PSoC.
    //----------------------------------------------------  
    .piPSOC_Fcfg_Rst_n(~reset),

    //----------------------------------------------------
    // CLKT / DRAM clocks 0 and 1 (Mem. Channels 0 and 1)
    //----------------------------------------------------     
    .piCLKT_Mem0Clk_n(memclk),
    .piCLKT_Mem0Clk_p(~memclk),
    .piCLKT_Mem1Clk_n(memclk),
    .piCLKT_Mem1Clk_p(~memclk),
 
    //----------------------------------------------------     
    // CLKT / GTH clocks (10Ge, Sata, Gtio Interfaces)
    //----------------------------------------------------     
    .piCLKT_10GeClk_n(refclk),
    .piCLKT_10GeClk_p(~refclk),

    //----------------------------------------------------     
    // CLKT / User clocks 0 and 1 (156.25MHz, 250MHz)
    //----------------------------------------------------
    .piCLKT_Usr0Clk_n(sysclk),
    .piCLKT_Usr0Clk_p(~sysclk),
    .piCLKT_Usr1Clk_n(),
    .piCLKT_Usr1Clk_p(),
       
    //----------------------------------------------------
    // PSOC / External Memory Interface (Emif)
    //----------------------------------------------------
    .piPSOC_Emif_Clk(Emifclk),
    .piPSOC_Emif_Cs_n(EmifCs_n),//1'b1),
    .piPSOC_Emif_We_n(EmifWe_n),//1'b1),
    .piPSOC_Emif_Oe_n(EmifOe_n),//1'b1),
    .piPSOC_Emif_AdS_n(1'b1),//1'b1),
    .piPSOC_Emif_Addr(EmifAddr),//8'd0),
    .pioPSOC_Emif_Data(EmifData_dut),
  
    //----------------------------------------------------
    // LED / Heart Beat Interface (Yellow LED)
    //----------------------------------------------------
    .poLED_HeartBeat_n(),
  
    //----------------------------------------------------
    // -- DDR(4) / Memory Channel 0 Interface (Mc0)
    //----------------------------------------------------
    .pioDDR4_Mem_Mc0_DmDbi_n(),
    .pioDDR4_Mem_Mc0_Dq(),
    .pioDDR4_Mem_Mc0_Dqs_p(),
    .pioDDR4_Mem_Mc0_Dqs_n(),
    .poDDR4_Mem_Mc0_Act_n(),
    .poDDR4_Mem_Mc0_Adr(),
    .poDDR4_Mem_Mc0_Ba(),
    .poDDR4_Mem_Mc0_Bg(),
    .poDDR4_Mem_Mc0_Cke(),
    .poDDR4_Mem_Mc0_Odt(),
    .poDDR4_Mem_Mc0_Cs_n(),
    .poDDR4_Mem_Mc0_Ck_p(),
    .poDDR4_Mem_Mc0_Ck_n(),
    .poDDR4_Mem_Mc0_Reset_n(),

    //----------------------------------------------------
    // DDR(4) / Memory Channel 1 Interface (Mc1)
    //----------------------------------------------------
    .pioDDR4_Mem_Mc1_DmDbi_n(),
    .pioDDR4_Mem_Mc1_Dq(),
    .pioDDR4_Mem_Mc1_Dqs_p(),
    .pioDDR4_Mem_Mc1_Dqs_n(),
    .poDDR4_Mem_Mc1_Act_n(),
    .poDDR4_Mem_Mc1_Adr(),
    .poDDR4_Mem_Mc1_Ba(),
    .poDDR4_Mem_Mc1_Bg(),
    .poDDR4_Mem_Mc1_Cke(),
    .poDDR4_Mem_Mc1_Odt(),
    .poDDR4_Mem_Mc1_Cs_n(),
    .poDDR4_Mem_Mc1_Ck_p(),
    .poDDR4_Mem_Mc1_Ck_n(),
    .poDDR4_Mem_Mc1_Reset_n(),

    //----------------------------------------------------
    // ECON / Edge Connector Interface (SPD08-200)
    //----------------------------------------------------
    .piECON_Eth_10Ge0_n(rxn_dut),
    .piECON_Eth_10Ge0_p(rxp_dut),
    .poECON_Eth_10Ge0_n(txn),
    .poECON_Eth_10Ge0_p(txp),
    .poCORE_Clk(coreclk_out)

  );

//-----------------------------------------------------------------------------
// Clock Drivers
//-----------------------------------------------------------------------------
 initial
  begin
     Emifclk <= 0;
     forever
     begin
        Emifclk <= 0;
        #(`PERIODEmifCLK/2);
        Emifclk <= 1;
        #(`PERIODEmifCLK/2);
     end
  end // initial begin



  initial                 // clock pretens to be SYSCK input to the MMCM of the example design
  begin
     sysclk <= 0;
     forever
     begin
        //#1666;               // 300 MHz MMCM in CLK
        #(`PERIODCORECLK/2); // 156.25 MHz User clock
        sysclk <= 1;
        //#1667;
        #(`PERIODCORECLK/2);
        sysclk <= 0;
     end
  end

 initial                 // stimulate sim_speedup_control port
  begin
     sim_speedup_control_pulse <= 0;
     forever
     begin
        #210000;
        sim_speedup_control_pulse <= 1;
     end
  end

  // Generate the refclk
  initial
  begin
     refclk <= 0;
     forever
     begin
        refclk <= 0;
        #(`PERIODCORECLK/2);
        refclk <= 1;
        #(`PERIODCORECLK/2);
     end
  end // initial begin

  // Generate the sampleclk
  initial
  begin
     sampleclk <= 1'b0;
     forever
     begin
        sampleclk <= 1'b0;
        #(`PERIODCORECLK/2);
        sampleclk <= 1'b1;
        #(`PERIODCORECLK/2);
     end
  end

  // Generate the bit clock
  initial
  begin
     bitclk <= 1'b0;
     #(`BITPERIOD/4);
     forever
     begin
        bitclk <= 1'b0;
        #(`BITPERIOD/2);
        bitclk <= 1'b1;
        #(`BITPERIOD/2);
     end
  end

// Generate memory clock
  initial
  begin
     memclk <= 0;
     forever
     begin
        #1666;
        memclk <= 1;
        #1667;
        memclk <= 0;
     end
  end


   // reset process
  initial
  begin
    $display("Resetting the core...");
        reset <= 1;
        //#503067;
        wait (coreclk_out == 1'b1);
        #10000;
        reset <= 0;
  end


  assign simulation_finished = tx_monitor_finished;
  assign simulation_error    = tx_monitor_error;


  // Main initial block to start and stop simulation

    //----------------------------------------------------------------
    // Receive Stimulus code.....
    //----------------------------------------------------------------

    // Support code for transmitting frames to core rxn/p
    reg [65:0] TxEnc;
    reg [31:0] d0;
    reg [3:0] c0;
    reg [63:0] d;
    reg [7:0] c;
    reg decided_clk_edge = 0;
    reg clk_edge;

    // Encode next 64 bits of frame;
    task rx_stimulus_send_column;
    input [31:0] d1;
    input [ 3:0] c1;
    begin : rx_stimulus_send_column
        @(posedge coreclk_out or negedge coreclk_out);
        d0 <= d1;
        c0 <= c1;

        assign d = {d1, d0};
        assign c = {c1, c0};

        // Need to know when to apply the encoded data to the scrambler
        if(!decided_clk_edge && |c0 && (coreclk_out !== 1'bx)) // Found first full 64 bit word
        begin
          clk_edge <= !coreclk_out;
          decided_clk_edge <= 1;
        end

        // Detect column of IDLEs vs T code in byte 0
        if(&c && d[7:0] !== 8'hFD) // Column of IDLEs
        begin
            TxEnc[1:0] = 2'b01;
            TxEnc[65:2] = 64'h00000000001E;
        end
        else if(|c) // Control code somewhere
        begin
            TxEnc[1:0] = 2'b01;

            if(c == 8'b00000001) // Start code
            begin
                TxEnc[9:2] = 8'h78;
                TxEnc[65:10] = d[63:8];
            end
            if(c == 8'b00011111) // Start code
            begin
                TxEnc[9:2] = 8'h33;
                TxEnc[41:10] = 32'h00000000;
                TxEnc[65:42] = d[63:40];
            end
            else if(c == 8'b10000000 && d[63:56] == 8'hFD) // End code) // End code
            begin
                TxEnc[9:2] = 8'hFF;
                TxEnc[65:10] = d[55:0];
            end
            else if(c == 8'b11000000 && d[55:48] == 8'hFD) // End code
            begin
                TxEnc[9:2] = 8'hE1;
                TxEnc[57:10] = d[47:0];
                TxEnc[65:58] = 8'h00;
            end
            else if(c == 8'b11100000 && d[47:40] == 8'hFD) // End code
            begin
                TxEnc[9:2] = 8'hD2;
                TxEnc[49:10] = d[39:0];
                TxEnc[65:50] = 16'h0000;
            end
            else if(c == 8'b11110000 && d[39:32] == 8'hFD) // End code
            begin
                TxEnc[9:2] = 8'hCC;
                TxEnc[41:10] = d[31:0];
                TxEnc[65:42] = 24'h000000;
            end
            else if(c == 8'b11111000 && d0[31:24] == 8'hFD) // End code
            begin
                TxEnc[9:2] = 8'hB4;
                TxEnc[33:10] = d[23:0];
                TxEnc[65:34] = 32'h00000000;
            end
            else if(c == 8'b11111100 && d0[23:16] == 8'hFD) // End code
            begin
                TxEnc[9:2] = 8'hAA;
                TxEnc[25:10] = d[15:0];
                TxEnc[65:26] = 40'h0000000000;
            end
            else if(c == 8'b11111110 && d0[15:8] == 8'hFD) // End code
            begin
                TxEnc[9:2] = 8'h99;
                TxEnc[17:10] = d[7:0];
                TxEnc[65:18] = 48'h000000000000;
            end
            else if(c == 8'b11111111 && d0[7:0] == 8'hFD) // End code
            begin
                TxEnc[9:2] = 8'h87;
                TxEnc[65:10] = 56'h00000000000000;
            end
        end
        else // all data
        begin
            TxEnc[1:0] = 2'b10;
            TxEnc[65:2] = d;
        end
    end
    endtask // rx_stimulus_send_column

    task rx_stimulus_send_idle;
    begin
        rx_stimulus_send_column(32'h07070707, 4'b1111);
    end
    endtask // rx_stimulus_send_idle

    task rx_stimulus_send_frame;
    input `FRAME_TYP frame;
    integer column_index, lane_index, byte_count, I, J;
    integer    data_block_index;
    reg [31:0] scratch_column_data, current_column_data;
    reg [3 :0] scratch_column_ctrl, current_column_ctrl;

    reg [31:0] fcs;

    begin
        rx_stimulus_working_frame.frombits(frame);
        column_index = 0;
        lane_index   = 0;
        byte_count   = 0;

        fcs          = 32'b0;
        // send preamble
        rx_stimulus_send_column(32'h555555FB, 4'b0001);
        rx_stimulus_send_column(32'hD5555555, 4'b0000);
        // send columns
        while (rx_stimulus_working_frame.ctrl[column_index] === 4'b0000 & column_index <= 2)
            begin
                rx_stimulus_send_column(rx_stimulus_working_frame.data[column_index], 4'b0000);
                calc_crc(4,rx_stimulus_working_frame.data[column_index], fcs);
                column_index = column_index + 1;
                byte_count = byte_count + 4;
            end
         // send 32-bit data word blocks after DA/SA
         // eg. if ctrl column[14] == 4'b0000 then the block size is (14 + 1) - 3 = 12
         // that means that 12 * 4 = 48 bytes are contained in one block
         // If `FRAME_GEN_MULTIPLIER is set to 2 then 2*12*4 = 96 bytes are sent after DA/SA
         // and the same 48 byte will be sent 2 times.
         // In order to get correct frames through the TYPE/LENGTH field is to be set to 94 in this case
         // the general formula for LENGTH/TYPE field is as follows:
         // [[(index of last complete ctrl column + 1) - 3] * 4 * FRAME_GEN_MULTIPLIER ]- 2 +
         // (0,1,2 or 3 depending from the value of the ctrl column after the last complete ctrl column)
         for (data_block_index = 0; data_block_index < `FRAME_GEN_MULTIPLIER; data_block_index = data_block_index + 1)
            begin
            column_index = 3;
               while (rx_stimulus_working_frame.ctrl[column_index] === 4'b0000)
                  begin
                     rx_stimulus_send_column(rx_stimulus_working_frame.data[column_index],4'b0000);
                     calc_crc(4,rx_stimulus_working_frame.data[column_index], fcs);
                     column_index = column_index + 1;
                     byte_count = byte_count + 4;
                  end
            end
         // calculate CRC for partial columns
         // Inject bad CRC when error bit is set

         if (rx_stimulus_working_frame.ctrl[column_index] === 4'b1000 ) begin
            calc_crc(3,rx_stimulus_working_frame.data[column_index], fcs);
            if (rx_stimulus_working_frame.error)
               rx_stimulus_send_column({~fcs[7:0],rx_stimulus_working_frame.data[column_index][23:0]}, 4'b0000);
            else
               rx_stimulus_send_column({fcs[7:0],rx_stimulus_working_frame.data[column_index][23:0]}, 4'b0000);
            rx_stimulus_send_column({8'hFD,fcs[31:8]},4'b1000);
         end
         else if (rx_stimulus_working_frame.ctrl[column_index] === 4'b1100 ) begin
            calc_crc(2,rx_stimulus_working_frame.data[column_index], fcs);
            if (rx_stimulus_working_frame.error)
               rx_stimulus_send_column({~fcs[15:0],rx_stimulus_working_frame.data[column_index][15:0]}, 4'b0000);
            else
               rx_stimulus_send_column({fcs[15:0],rx_stimulus_working_frame.data[column_index][15:0]}, 4'b0000);
            rx_stimulus_send_column({8'h07,8'hFD,fcs[31:16]},4'b1100);
         end
         else if (rx_stimulus_working_frame.ctrl[column_index] === 4'b1110 ) begin
            calc_crc(1,rx_stimulus_working_frame.data[column_index], fcs);
            if (rx_stimulus_working_frame.error)
               rx_stimulus_send_column({~fcs[23:0],rx_stimulus_working_frame.data[column_index][7:0]}, 4'b0000);
            else
               rx_stimulus_send_column({fcs[23:0],rx_stimulus_working_frame.data[column_index][7:0]}, 4'b0000);
            rx_stimulus_send_column({16'h0707,8'hFD,fcs[31:24]},4'b1110);
         end
         else if (rx_stimulus_working_frame.ctrl[column_index] === 4'b1111 ) begin
            if (rx_stimulus_working_frame.error)
               rx_stimulus_send_column(~fcs, 4'b0000);
            else
               rx_stimulus_send_column(fcs, 4'b0000);
            rx_stimulus_send_column({24'h070707,8'hFD}, 4'b1111);
         end

        $display("Receiver: frame inserted into Serial interface");
    end
    endtask // rx_stimulus_send_frame

/*    initial begin: write_reg
              EmifCs_n <= 1'b1;
              EmifWe_n <= 1'b1;
              EmifAddr <= 8'd0;
              EmifData <= 8'd0;
              EmifOe_n <= 1'b1;
          while ( IDUT.SHELL.SuperCfg.ETH0.ETH.poMMIO_CoreReady != 1'b1)
              @(negedge Emifclk);
              if( write_loopback_reg == 1) begin
	        EmifCs_n <= 1'b0;
                EmifWe_n <= 1'b0;
                EmifAddr <= 8'd74;
                EmifData <= 8'd2;
                #10000;
                write_loopback_reg = 1;
              end
    end

*/


/*  initial
  begin
    if (TB_MODE == "BIST") begin
       #70000000;
       if (!frame_error) begin
         $display("** Test completed successfully");
       end
       else
         $display("** Error: Frame checker detected an error");
         $stop;
    end
    else begin
       fork: sim_in_progress
         @(posedge simulation_finished) disable sim_in_progress;
         #90000000                      disable sim_in_progress;
       join
       if (simulation_finished && !simulation_error)
         $display("**** Test completed successfully");
         else if(simulation_error)
           $display("**** Error: Testbench had errors");
         else
           $display("**** Error: Testbench timed out");
         $stop;
    end
  end
*/

    initial
    begin : write_reg 
      if (TB_MODE != "BIST") begin
 
        EmifOe_n <= 1'b1;
	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        while ( IDUT.SHELL.SuperCfg.ETH0.ETH.poMMIO_CoreReady !== 1'b1)
//          $display("waiting for the core to be ready");
//          while ( IDUT.SHELL.sMMIO_ETH0_MacLoopbackEn !== 1'b1)

//        @(posedge IDUT.SHELL.SuperCfg.ETH0.ETH.poMMIO_CoreReady);
        // let us write the MMIO register to enable Macloopbacken before sending frames
        @(negedge Emifclk);
	EmifCs_n <= 1'b0;
        EmifWe_n <= 1'b0;
        EmifAddr <= 8'd116;
        EmifData <= 8'd2;
        @(negedge Emifclk);
       	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        #3000;
        write_loopback_reg <= 1;

	wait(simulation_finished == 1'b1);
        if (!simulation_error) begin
          $display("**** MacLoopbackEn Test completed successfully");
        end
        else if(simulation_error) begin
          $display("**** MacLoopbackEn Error: Testbench had errors");
          $stop;
	end
        else begin
          $display("**** MacLoopbackEn Error: Testbench timed out");
          $stop;
        end
 //disable loopback
        @(negedge Emifclk);
        EmifCs_n <= 1'b0;
        EmifWe_n <= 1'b0;
        EmifAddr <= 8'd116;
        EmifData <= 8'd0;
        @(negedge Emifclk);
       	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        #20000;
//        while(IDUT.SHELL.MMIO.poETH0_MacLoopbackEn !== 1'b0)
//          $display("**** waiting for MacLoopbackEn to be disabled");


        if(IDUT.SHELL.MMIO.poETH0_MacLoopbackEn  == 1'b1) begin
          $display("**** MacLoopbackEn NOt yet Disabled");
          $stop;
        end
        
          $display("**** MacLoopbackEn Disabled");
        loopback_disabled <= 1;
        write_loopback_reg <= 0;
        //write MAC address
        @(negedge Emifclk);
        EmifCs_n <= 1'b0;
        EmifWe_n <= 1'b0;
        EmifAddr <= 8'h22;
        EmifData <= 8'haa;
        @(negedge Emifclk);
       	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        #20000;

        @(negedge Emifclk);
        EmifCs_n <= 1'b0;
        EmifWe_n <= 1'b0;
        EmifAddr <= 8'h23;
        EmifData <= 8'h1b;
        @(negedge Emifclk);
       	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        #20000;

        @(negedge Emifclk);
        EmifCs_n <= 1'b0;
        EmifWe_n <= 1'b0;
        EmifAddr <= 8'h24;
        EmifData <= 8'h3c;
        @(negedge Emifclk);
       	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        #20000;

        @(negedge Emifclk);
        EmifCs_n <= 1'b0;
        EmifWe_n <= 1'b0;
        EmifAddr <= 8'h25;
        EmifData <= 8'h00;
        @(negedge Emifclk);
       	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        #20000;

        @(negedge Emifclk);
        EmifCs_n <= 1'b0;
        EmifWe_n <= 1'b0;
        EmifAddr <= 8'h26;
        EmifData <= 8'h5e;
        @(negedge Emifclk);
       	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        #20000;

       @(negedge Emifclk);
        EmifCs_n <= 1'b0;
        EmifWe_n <= 1'b0;
        EmifAddr <= 8'h27;
        EmifData <= 8'h55;
        @(negedge Emifclk);
       	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        #20000;


        //write the ip address 
        @(negedge Emifclk);
        EmifCs_n <= 1'b0;
        EmifWe_n <= 1'b0;
        EmifAddr <= 8'h34;
        EmifData <= 8'd10;
        @(negedge Emifclk);
       	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        #20000;

        @(negedge Emifclk);
        EmifCs_n <= 1'b0;
        EmifWe_n <= 1'b0;
        EmifAddr <= 8'h35;
        EmifData <= 8'd12;
        @(negedge Emifclk);
       	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        #20000;

        @(negedge Emifclk);
        EmifCs_n <= 1'b0;
        EmifWe_n <= 1'b0;
        EmifAddr <= 8'h36;
        EmifData <= 8'd200;
        @(negedge Emifclk);
       	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        #20000;

        @(negedge Emifclk);
        EmifCs_n <= 1'b0;
        EmifWe_n <= 1'b0;
        EmifAddr <= 8'h37;
        EmifData <= 8'd19;
        @(negedge Emifclk);
       	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        #20000;
        set_mac_ip_address <= 1;


        @(negedge Emifclk);
        EmifCs_n <= 1'b0;
        EmifWe_n <= 1'b0;
        EmifAddr <= 8'h18;
        EmifData <= 8'h00;
        @(negedge Emifclk);
       	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        #20000;

        @(negedge Emifclk);
        EmifCs_n <= 1'b0;
        EmifWe_n <= 1'b0;
        EmifAddr <= 8'h19;
        EmifData <= 8'h7F;
        @(negedge Emifclk);
       	EmifCs_n <= 1'b1;
        EmifWe_n <= 1'b1;
        EmifAddr <= 8'd0;
        EmifData <= 8'd0;
        #20000;


//let us check TCP loopback
      end 
    end // block: p_rx_stimulus

//procedure to check ARP response
    always @(posedge coreclk_out) begin
      if(sent_ARPframe == 1'b1) begin
        if (IDUT.SHELL.ssNTS0_ETH0_Data_tvalid == 1'b1) begin
        //save what is on  IDUT.SHELL.ssNTS0_ETH0_Data_tdata 
          $display("NTS data is %h", IDUT.SHELL.ssNTS0_ETH0_Data_tdata[31:0]);
          $display("NTS data is %h", IDUT.SHELL.ssNTS0_ETH0_Data_tdata[63:32]);

          frameR.data[placeholderARP] = IDUT.SHELL.ssNTS0_ETH0_Data_tdata[31:0];

          frameR.data[placeholderARP+1] = IDUT.SHELL.ssNTS0_ETH0_Data_tdata[63:32];
          $display("frame data at %d is %h", placeholder, frameR.data[placeholderARP]);
          $display("frame data at %d is %h", placeholder+1, frameR.data[placeholderARP+1]);
          if (IDUT.SHELL.ssNTS0_ETH0_Data_tlast == 1'b1) begin
            checkARPresponse(frameR.tobits(0));
            placeholderARP = 0; 
          end
          else
            placeholderARP = placeholderARP + 2; 

       end 
      end
    end

    assign EmifData_dut = EmifData;
    initial
       begin : p_rx_stimulus
       if (TB_MODE != "BIST") begin
             $display("DUT frame stimulus is from the testbench");

           checkARPresponse(frame6.tobits(0));
         
           frame7.tobits(0);
           //claculate IP header checksum for frame7
           sum16(frame7.data[3][31:16], frame7.data[4][15:0], newsum);
           oldsum = newsum;
           sum16( oldsum, frame7.data[4][31:16], newsum);
           oldsum = newsum;
           sum16( oldsum, frame7.data[5][15:0], newsum);
           oldsum = newsum;
           sum16( oldsum, frame7.data[5][31:16], newsum);
           oldsum = newsum;
           sum16( oldsum, frame7.data[6][31:16], newsum);
           oldsum = newsum;
           sum16( oldsum, frame7.data[7][15:0], newsum);
           oldsum = newsum;
           sum16( oldsum, frame7.data[7][31:16], newsum);
           oldsum = newsum;
           sum16( oldsum, frame7.data[8][15:0], newsum);
 
           checksum =~ newsum; 
           $display("IP checksum for frame7 %h %h", newsum, checksum);
 





 
           //calculate ICMP checksum for frame7
           
           frame7column_index = 10;
           sum16(frame7.data[8][31:16], frame7.data[9][31:16], newsum);
           while(frame7column_index <= 23) begin
            oldsum = newsum;
            sum16(oldsum, frame7.data[frame7column_index][15:0], newsum);
            oldsum = newsum;
             sum16(oldsum, frame7.data[frame7column_index][31:16], newsum);
            frame7column_index = frame7column_index + 1;
           end
           oldsum = newsum;
           sum16(oldsum, frame7.data[24][15:0], newsum);
           checksum =~ newsum; 
           $display("ICMP checksum for frame7 %h %h", newsum, checksum);
   
          CalchecksumsUDP(frame8.tobits(0), oldsum, newsum, 8);
          CalchecksumsUDP(frame9.tobits(0), oldsum, newsum, 9);
          CalchecksumsUDP(frame10.tobits(0), oldsum, newsum, 10);
          CalchecksumsUDP(frame11.tobits(0), oldsum, newsum, 11);
          CalchecksumsUDP(frame12.tobits(0), oldsum, newsum, 12);
          CalchecksumsUDP(frame13.tobits(0), oldsum, newsum, 13);
          CalchecksumsUDP(frame14.tobits(0), oldsum, newsum, 14);


            while(1) begin
              rx_stimulus_send_idle;
              if ( write_loopback_reg == 1)
                break;
            end

            rx_stimulus_send_idle;
            rx_stimulus_send_frame(frame0.tobits(0));
            rx_stimulus_send_idle;
            rx_stimulus_send_idle;
            rx_stimulus_send_frame(frame1.tobits(0));
            rx_stimulus_send_idle;
            rx_stimulus_send_idle;
            rx_stimulus_send_frame(frame2.tobits(0));
            rx_stimulus_send_idle;
            rx_stimulus_send_idle;
            rx_stimulus_send_frame(frame3.tobits(0));
              
       //     while (loopback_disabled !== 1)
       //       rx_stimulus_send_idle;
            while(1) begin
              rx_stimulus_send_idle;
              if ( write_loopback_reg == 0 && set_mac_ip_address == 1)
                break;
            end

         //check if TOE is ready
            while(1) begin
              rx_stimulus_send_idle;
              if (IDUT.SHELL.sNTS0_MMIO_ToeReady == 1'b1)
                break;
            end


            //rx_stimulus_send_idle;
            //rx_stimulus_send_frame(frame0.tobits(0));

            rx_stimulus_send_idle;
            rx_stimulus_send_idle;
            rx_stimulus_send_frame(frame6.tobits(0));

            sent_ARPframe = 1'b1;
            while(1) begin
              if (IDUT.SHELL.ssNTS0_ETH0_Data_tlast == 1'b1) begin
                sent_ARPframe = 1'b0;
                break;
              end
            end

            //rx_stimulus_send_idle;
            //rx_stimulus_send_idle;
            //rx_stimulus_send_frame(frame6.tobits(0));


            rx_stimulus_send_idle;
            rx_stimulus_send_idle;
//            rx_stimulus_send_frame(frame7.tobits(0));
            while(1) begin
              rx_stimulus_send_idle;
              if (IDUT.SHELL.ssNTS0_ETH0_Data_tvalid == 1'b1)
                break;
            end

            while(1) begin
              rx_stimulus_send_idle;
              if (IDUT.SHELL.ssNTS0_ETH0_Data_tlast == 1'b1)
                break;
            end

  
/*          rx_stimulus_send_idle;
            rx_stimulus_send_idle;
            rx_stimulus_send_frame(frame8.tobits(0)); // checksum correct
            while(1) begin
              rx_stimulus_send_idle;
              if (IDUT.SHELL.ssNTS0_ETH0_Data_tvalid == 1'b1)
                break;
            end
            $display("ssNTS0_ETH0_Data_tvalid is high for frame8");

   
            while(1) begin
              rx_stimulus_send_idle;
              if (IDUT.SHELL.ssNTS0_ETH0_Data_tlast == 1'b1)
                break;
            end
            $display("ssNTS0_ETH0_Data_tlast is high for frame8");
*/

 
  //          rx_stimulus_send_idle;
  //          rx_stimulus_send_idle;
            //rx_stimulus_send_frame(frame14.tobits(0)); // frame8 as sent to francois
 

 /*           rx_stimulus_send_idle;
            rx_stimulus_send_idle;
            rx_stimulus_send_frame(frame9.tobits(0)); //frame8 plus checksum disabled 000
            while(1) begin
              rx_stimulus_send_idle;
              if (IDUT.SHELL.ssNTS0_ETH0_Data_tvalid == 1'b1)
                break;
            end
            $display("ssNTS0_ETH0_Data_tvalid is high for frame9");

   
            while(1) begin
              rx_stimulus_send_idle;
              if (IDUT.SHELL.ssNTS0_ETH0_Data_tlast == 1'b1)
                break;
            end
            $display("ssNTS0_ETH0_Data_tlast is high for frame9");
*/

            rx_stimulus_send_idle;
            rx_stimulus_send_idle;
//            rx_stimulus_send_frame(frame15.tobits(0)); //large packet plus checksum disabled 000
            while(1) begin
              rx_stimulus_send_idle;
              if (IDUT.SHELL.ssNTS0_ETH0_Data_tvalid == 1'b1)
                break;
            end
            $display("ssNTS0_ETH0_Data_tvalid is high for frame15");

   
            while(1) begin
              rx_stimulus_send_idle;
              if (IDUT.SHELL.ssNTS0_ETH0_Data_tlast == 1'b1)
                break;
            end
            $display("ssNTS0_ETH0_Data_tlast is high for frame15");


            //rx_stimulus_send_idle;
            //rx_stimulus_send_idle;
            //rx_stimulus_send_frame(frame10.tobits(0)); //frame8, checksum wrong
 
            rx_stimulus_send_idle;
            rx_stimulus_send_idle;
//            rx_stimulus_send_frame(frame11.tobits(0)); //large UDP packet, checksum disabled
            while(1) begin
              rx_stimulus_send_idle;
              if (IDUT.SHELL.ssNTS0_ETH0_Data_tvalid == 1'b1)
                break;
            end
            $display("ssNTS0_ETH0_Data_tvalid is high for frame11");

   
            while(1) begin
              rx_stimulus_send_idle;
              if (IDUT.SHELL.ssNTS0_ETH0_Data_tlast == 1'b1)
                break;
            end
            $display("ssNTS0_ETH0_Data_tlast is high for frame11");


            //rx_stimulus_send_idle;
            //rx_stimulus_send_idle;
            //rx_stimulus_send_frame(frame12.tobits(0)); //frame8, small UDP packet, fake length
 
            //rx_stimulus_send_idle;
            //rx_stimulus_send_idle;
            //rx_stimulus_send_frame(frame13.tobits(0)); //large UDP packet, fake length, wrong checksum
 
 
            while(1)
              rx_stimulus_send_idle;
 
       end
       else
            $display("DUT frame stimulus is pattern generator inside the example design");
    end // block: p_rx_stimulus

        // Capture the 66 bit data for scrambling...
    reg [65:0] TxEnc_Data = 66'h79;
    wire TxEnc_clock;

    assign TxEnc_clock = clk_edge ? coreclk_out : !coreclk_out;
    always @(posedge TxEnc_clock) begin
        TxEnc_Data <= TxEnc;
    end



    reg [65:0] TXD_Scr = 66'h2;

    reg [57:0]    Scrambler_Register = 58'h3;
    reg [63:0]    TXD_input = 0;
    reg [1:0]     Sync_header = 2'b10;
    wire [63:0]   Scr_wire;

    // Scramble the TxEnc_Data before applying to rxn/p
    assign Scr_wire[0] = TXD_input[0]^Scrambler_Register[38]^Scrambler_Register[57];
    assign Scr_wire[1] = TXD_input[1]^Scrambler_Register[37]^Scrambler_Register[56];
    assign Scr_wire[2] = TXD_input[2]^Scrambler_Register[36]^Scrambler_Register[55];
    assign Scr_wire[3] = TXD_input[3]^Scrambler_Register[35]^Scrambler_Register[54];
    assign Scr_wire[4] = TXD_input[4]^Scrambler_Register[34]^Scrambler_Register[53];
    assign Scr_wire[5] = TXD_input[5]^Scrambler_Register[33]^Scrambler_Register[52];
    assign Scr_wire[6] = TXD_input[6]^Scrambler_Register[32]^Scrambler_Register[51];
    assign Scr_wire[7] = TXD_input[7]^Scrambler_Register[31]^Scrambler_Register[50];

    assign Scr_wire[8] = TXD_input[8]^Scrambler_Register[30]^Scrambler_Register[49];
    assign Scr_wire[9] = TXD_input[9]^Scrambler_Register[29]^Scrambler_Register[48];
    assign Scr_wire[10] = TXD_input[10]^Scrambler_Register[28]^Scrambler_Register[47];
    assign Scr_wire[11] = TXD_input[11]^Scrambler_Register[27]^Scrambler_Register[46];
    assign Scr_wire[12] = TXD_input[12]^Scrambler_Register[26]^Scrambler_Register[45];
    assign Scr_wire[13] = TXD_input[13]^Scrambler_Register[25]^Scrambler_Register[44];
    assign Scr_wire[14] = TXD_input[14]^Scrambler_Register[24]^Scrambler_Register[43];
    assign Scr_wire[15] = TXD_input[15]^Scrambler_Register[23]^Scrambler_Register[42];

    assign Scr_wire[16] = TXD_input[16]^Scrambler_Register[22]^Scrambler_Register[41];
    assign Scr_wire[17] = TXD_input[17]^Scrambler_Register[21]^Scrambler_Register[40];
    assign Scr_wire[18] = TXD_input[18]^Scrambler_Register[20]^Scrambler_Register[39];
    assign Scr_wire[19] = TXD_input[19]^Scrambler_Register[19]^Scrambler_Register[38];
    assign Scr_wire[20] = TXD_input[20]^Scrambler_Register[18]^Scrambler_Register[37];
    assign Scr_wire[21] = TXD_input[21]^Scrambler_Register[17]^Scrambler_Register[36];
    assign Scr_wire[22] = TXD_input[22]^Scrambler_Register[16]^Scrambler_Register[35];
    assign Scr_wire[23] = TXD_input[23]^Scrambler_Register[15]^Scrambler_Register[34];

    assign Scr_wire[24] = TXD_input[24]^Scrambler_Register[14]^Scrambler_Register[33];
    assign Scr_wire[25] = TXD_input[25]^Scrambler_Register[13]^Scrambler_Register[32];
    assign Scr_wire[26] = TXD_input[26]^Scrambler_Register[12]^Scrambler_Register[31];
    assign Scr_wire[27] = TXD_input[27]^Scrambler_Register[11]^Scrambler_Register[30];
    assign Scr_wire[28] = TXD_input[28]^Scrambler_Register[10]^Scrambler_Register[29];
    assign Scr_wire[29] = TXD_input[29]^Scrambler_Register[9]^Scrambler_Register[28];
    assign Scr_wire[30] = TXD_input[30]^Scrambler_Register[8]^Scrambler_Register[27];
    assign Scr_wire[31] = TXD_input[31]^Scrambler_Register[7]^Scrambler_Register[26];

    assign Scr_wire[32] = TXD_input[32]^Scrambler_Register[6]^Scrambler_Register[25];
    assign Scr_wire[33] = TXD_input[33]^Scrambler_Register[5]^Scrambler_Register[24];
    assign Scr_wire[34] = TXD_input[34]^Scrambler_Register[4]^Scrambler_Register[23];
    assign Scr_wire[35] = TXD_input[35]^Scrambler_Register[3]^Scrambler_Register[22];
    assign Scr_wire[36] = TXD_input[36]^Scrambler_Register[2]^Scrambler_Register[21];
    assign Scr_wire[37] = TXD_input[37]^Scrambler_Register[1]^Scrambler_Register[20];
    assign Scr_wire[38] = TXD_input[38]^Scrambler_Register[0]^Scrambler_Register[19];
    assign Scr_wire[39] = TXD_input[39]^TXD_input[0]^Scrambler_Register[38]^Scrambler_Register[57]^Scrambler_Register[18];
    assign Scr_wire[40] = TXD_input[40]^(TXD_input[1]^Scrambler_Register[37]^Scrambler_Register[56])^Scrambler_Register[17];
    assign Scr_wire[41] = TXD_input[41]^(TXD_input[2]^Scrambler_Register[36]^Scrambler_Register[55])^Scrambler_Register[16];
    assign Scr_wire[42] = TXD_input[42]^(TXD_input[3]^Scrambler_Register[35]^Scrambler_Register[54])^Scrambler_Register[15];
    assign Scr_wire[43] = TXD_input[43]^(TXD_input[4]^Scrambler_Register[34]^Scrambler_Register[53])^Scrambler_Register[14];
    assign Scr_wire[44] = TXD_input[44]^(TXD_input[5]^Scrambler_Register[33]^Scrambler_Register[52])^Scrambler_Register[13];
    assign Scr_wire[45] = TXD_input[45]^(TXD_input[6]^Scrambler_Register[32]^Scrambler_Register[51])^Scrambler_Register[12];
    assign Scr_wire[46] = TXD_input[46]^(TXD_input[7]^Scrambler_Register[31]^Scrambler_Register[50])^Scrambler_Register[11];
    assign Scr_wire[47] = TXD_input[47]^(TXD_input[8]^Scrambler_Register[30]^Scrambler_Register[49])^Scrambler_Register[10];

    assign Scr_wire[48] = TXD_input[48]^(TXD_input[9]^Scrambler_Register[29]^Scrambler_Register[48])^Scrambler_Register[9];
    assign Scr_wire[49] = TXD_input[49]^(TXD_input[10]^Scrambler_Register[28]^Scrambler_Register[47])^Scrambler_Register[8];
    assign Scr_wire[50] = TXD_input[50]^(TXD_input[11]^Scrambler_Register[27]^Scrambler_Register[46])^Scrambler_Register[7];
    assign Scr_wire[51] = TXD_input[51]^(TXD_input[12]^Scrambler_Register[26]^Scrambler_Register[45])^Scrambler_Register[6];
    assign Scr_wire[52] = TXD_input[52]^(TXD_input[13]^Scrambler_Register[25]^Scrambler_Register[44])^Scrambler_Register[5];
    assign Scr_wire[53] = TXD_input[53]^(TXD_input[14]^Scrambler_Register[24]^Scrambler_Register[43])^Scrambler_Register[4];
    assign Scr_wire[54] = TXD_input[54]^(TXD_input[15]^Scrambler_Register[23]^Scrambler_Register[42])^Scrambler_Register[3];
    assign Scr_wire[55] = TXD_input[55]^(TXD_input[16]^Scrambler_Register[22]^Scrambler_Register[41])^Scrambler_Register[2];

    assign Scr_wire[56] = TXD_input[56]^(TXD_input[17]^Scrambler_Register[21]^Scrambler_Register[40])^Scrambler_Register[1];
    assign Scr_wire[57] = TXD_input[57]^(TXD_input[18]^Scrambler_Register[20]^Scrambler_Register[39])^Scrambler_Register[0];
    assign Scr_wire[58] = TXD_input[58]^(TXD_input[19]^Scrambler_Register[19]^Scrambler_Register[38])^(TXD_input[0]^Scrambler_Register[38]^Scrambler_Register[57]);
    assign Scr_wire[59] = TXD_input[59]^(TXD_input[20]^Scrambler_Register[18]^Scrambler_Register[37])^(TXD_input[1]^Scrambler_Register[37]^Scrambler_Register[56]);
    assign Scr_wire[60] = TXD_input[60]^(TXD_input[21]^Scrambler_Register[17]^Scrambler_Register[36])^(TXD_input[2]^Scrambler_Register[36]^Scrambler_Register[55]);
    assign Scr_wire[61] = TXD_input[61]^(TXD_input[22]^Scrambler_Register[16]^Scrambler_Register[35])^(TXD_input[3]^Scrambler_Register[35]^Scrambler_Register[54]);
    assign Scr_wire[62] = TXD_input[62]^(TXD_input[23]^Scrambler_Register[15]^Scrambler_Register[34])^(TXD_input[4]^Scrambler_Register[34]^Scrambler_Register[53]);
    assign Scr_wire[63] = TXD_input[63]^(TXD_input[24]^Scrambler_Register[14]^Scrambler_Register[33])^(TXD_input[5]^Scrambler_Register[33]^Scrambler_Register[52]);


    always @(posedge TxEnc_clock) begin
        TXD_input[63:0] <= TxEnc_Data[65:2];
        Sync_header[1:0] <= TxEnc_Data[1:0];
        TXD_Scr[65:0] <= {Scr_wire[63:0], Sync_header[1:0]};
        for (i=0; i<58; i=i+1) begin
            Scrambler_Register[i] <= Scr_wire[63-i];
        end
    end

    // Serialize the RX stimulus
    assign rxn = !rxp;

    reg[65:0] serial_word = 66'h0;
    integer rxbitno = 'd0;

    always @(posedge bitclk) begin : rx_serialize
        rxp <= serial_word[rxbitno];
        rxbitno <= (rxbitno + 1) % 66;
        // Pull in the next word when we have sent 66 bits
        if (rxbitno == 'd65) begin
            serial_word <= TXD_Scr;
        end
    end // rx_serialize

   //----------------------------------------------------------------
   // Transmit Monitor code.....
   //----------------------------------------------------------------

   // Fill RxD with 66 bits...
   always @(posedge bitclk)
   begin : p_tx_serial_capture
     if(!slip)
     begin // Just grab next 66 bits
       RxD[64:0] <= RxD[65:1];
       RxD[65] <= txp;
       if(nbits < 65)
       begin
         nbits <= nbits + 1;
         test_sh <= 0;
       end
       else
       begin
         nbits <= 0;
         test_sh <= 1;
       end
     end
     else // SLIP!!
     begin // Just grab single bit
       RxD[64:0] <= RxD[65:1];
       RxD[65] <= txp;
       test_sh <= 1;
       nbits <= 0;
     end
   end // p_tx_serial_capture


   // Implement the block lock state machine on serial TX...
   always @(BLSTATE or test_sh or RxD)
     begin : p_tx_block_lock

     case (BLSTATE)
       `LOCK_INIT : begin
         tx_monitor_block_lock <= 1'b0;
         next_blstate <= `RESET_CNT;
         slip <= 0;
         sh_cnt <= 0;
         sh_invalid_cnt <= 0;
       end
       `RESET_CNT : begin
         slip <= 0;
         if(test_sh)
           next_blstate <= `TEST_SH;
         else
           next_blstate <= `RESET_CNT;
       end
       `TEST_SH : begin
         slip <= 0;
         next_blstate <= `TEST_SH;
         if(test_sh && (RxD[0] != RxD[1])) // Good sync header candidate
         begin
           sh_cnt <= sh_cnt + 1; // Immediate update!
           if(sh_cnt < 64)
             next_blstate <= `TEST_SH;
           else if(sh_cnt == 64 && sh_invalid_cnt > 0)
           begin
             next_blstate <= `RESET_CNT;
             sh_cnt <= 0;
             sh_invalid_cnt <= 0;
           end
           else if(sh_cnt == 64 && sh_invalid_cnt == 0)
           begin
             tx_monitor_block_lock <= 1;
             next_blstate <= `RESET_CNT;
             sh_cnt <= 0;
             sh_invalid_cnt <= 0;
           end
         end
         else if(test_sh)// Bad sync header
         begin
           sh_cnt <= sh_cnt + 1;
           sh_invalid_cnt <= sh_invalid_cnt + 1;
           if(sh_cnt == 64 && sh_invalid_cnt < 16 && tx_monitor_block_lock)
           begin
             next_blstate <= `RESET_CNT;
             sh_cnt <= 0;
             sh_invalid_cnt <= 0;
           end
           else if(sh_cnt < 64 && sh_invalid_cnt < 16 && test_sh && tx_monitor_block_lock)
             next_blstate <= `TEST_SH;
           else if(sh_invalid_cnt == 16 || !tx_monitor_block_lock)
           begin
             tx_monitor_block_lock <= 0;
             slip <= 1;
             sh_cnt <= 0;
             sh_invalid_cnt <= 0;
             next_blstate <= `RESET_CNT;
           end
         end
       end
     endcase
   end // p_tx_block_lock

   // Implement the block lock state machine on serial TX
   // And capture the aligned 66 bit words....
   always @(posedge bitclk)
     begin : p_tx_block_lock_next_blstate
       if(reset)
         BLSTATE <= `LOCK_INIT;
       else
         BLSTATE <= next_blstate;

       if(test_sh && tx_monitor_block_lock)
         RxD_aligned <= RxD;

     end // p_tx_block_lock_next_blstate

    // Descramble the TX serial data
    reg    [57:0] DeScrambler_Register = 58'h3;
    reg    [63:0] RXD_input = 64'h0;
    reg    [1:0]  RX_Sync_header = 2'b01;
    wire   [63:0] DeScr_wire;
    reg    [65:0] DeScr_RXD = 66'h79;

    assign DeScr_wire[0] = RXD_input[0]^DeScrambler_Register[38]^DeScrambler_Register[57];
    assign DeScr_wire[1] = RXD_input[1]^DeScrambler_Register[37]^DeScrambler_Register[56];
    assign DeScr_wire[2] = RXD_input[2]^DeScrambler_Register[36]^DeScrambler_Register[55];
    assign DeScr_wire[3] = RXD_input[3]^DeScrambler_Register[35]^DeScrambler_Register[54];
    assign DeScr_wire[4] = RXD_input[4]^DeScrambler_Register[34]^DeScrambler_Register[53];
    assign DeScr_wire[5] = RXD_input[5]^DeScrambler_Register[33]^DeScrambler_Register[52];
    assign DeScr_wire[6] = RXD_input[6]^DeScrambler_Register[32]^DeScrambler_Register[51];
    assign DeScr_wire[7] = RXD_input[7]^DeScrambler_Register[31]^DeScrambler_Register[50];

    assign  DeScr_wire[8] = RXD_input[8]^DeScrambler_Register[30]^DeScrambler_Register[49];
    assign  DeScr_wire[9] = RXD_input[9]^DeScrambler_Register[29]^DeScrambler_Register[48];
    assign  DeScr_wire[10] = RXD_input[10]^DeScrambler_Register[28]^DeScrambler_Register[47];
    assign  DeScr_wire[11] = RXD_input[11]^DeScrambler_Register[27]^DeScrambler_Register[46];
    assign  DeScr_wire[12] = RXD_input[12]^DeScrambler_Register[26]^DeScrambler_Register[45];
    assign  DeScr_wire[13] = RXD_input[13]^DeScrambler_Register[25]^DeScrambler_Register[44];
    assign  DeScr_wire[14] = RXD_input[14]^DeScrambler_Register[24]^DeScrambler_Register[43];
    assign  DeScr_wire[15] = RXD_input[15]^DeScrambler_Register[23]^DeScrambler_Register[42];

    assign  DeScr_wire[16] = RXD_input[16]^DeScrambler_Register[22]^DeScrambler_Register[41];
    assign  DeScr_wire[17] = RXD_input[17]^DeScrambler_Register[21]^DeScrambler_Register[40];
    assign  DeScr_wire[18] = RXD_input[18]^DeScrambler_Register[20]^DeScrambler_Register[39];
    assign  DeScr_wire[19] = RXD_input[19]^DeScrambler_Register[19]^DeScrambler_Register[38];
    assign  DeScr_wire[20] = RXD_input[20]^DeScrambler_Register[18]^DeScrambler_Register[37];
    assign  DeScr_wire[21] = RXD_input[21]^DeScrambler_Register[17]^DeScrambler_Register[36];
    assign  DeScr_wire[22] = RXD_input[22]^DeScrambler_Register[16]^DeScrambler_Register[35];
    assign  DeScr_wire[23] = RXD_input[23]^DeScrambler_Register[15]^DeScrambler_Register[34];

    assign  DeScr_wire[24] = RXD_input[24]^DeScrambler_Register[14]^DeScrambler_Register[33];
    assign  DeScr_wire[25] = RXD_input[25]^DeScrambler_Register[13]^DeScrambler_Register[32];
    assign  DeScr_wire[26] = RXD_input[26]^DeScrambler_Register[12]^DeScrambler_Register[31];
    assign  DeScr_wire[27] = RXD_input[27]^DeScrambler_Register[11]^DeScrambler_Register[30];
    assign  DeScr_wire[28] = RXD_input[28]^DeScrambler_Register[10]^DeScrambler_Register[29];
    assign  DeScr_wire[29] = RXD_input[29]^DeScrambler_Register[9]^DeScrambler_Register[28];
    assign  DeScr_wire[30] = RXD_input[30]^DeScrambler_Register[8]^DeScrambler_Register[27];
    assign  DeScr_wire[31] = RXD_input[31]^DeScrambler_Register[7]^DeScrambler_Register[26];

    assign  DeScr_wire[32] = RXD_input[32]^DeScrambler_Register[6]^DeScrambler_Register[25];
    assign  DeScr_wire[33] = RXD_input[33]^DeScrambler_Register[5]^DeScrambler_Register[24];
    assign  DeScr_wire[34] = RXD_input[34]^DeScrambler_Register[4]^DeScrambler_Register[23];
    assign  DeScr_wire[35] = RXD_input[35]^DeScrambler_Register[3]^DeScrambler_Register[22];
    assign  DeScr_wire[36] = RXD_input[36]^DeScrambler_Register[2]^DeScrambler_Register[21];
    assign  DeScr_wire[37] = RXD_input[37]^DeScrambler_Register[1]^DeScrambler_Register[20];
    assign  DeScr_wire[38] = RXD_input[38]^DeScrambler_Register[0]^DeScrambler_Register[19];

    assign  DeScr_wire[39] = RXD_input[39]^RXD_input[0]^DeScrambler_Register[18];
    assign  DeScr_wire[40] = RXD_input[40]^RXD_input[1]^DeScrambler_Register[17];
    assign  DeScr_wire[41] = RXD_input[41]^RXD_input[2]^DeScrambler_Register[16];
    assign  DeScr_wire[42] = RXD_input[42]^RXD_input[3]^DeScrambler_Register[15];
    assign  DeScr_wire[43] = RXD_input[43]^RXD_input[4]^DeScrambler_Register[14];
    assign  DeScr_wire[44] = RXD_input[44]^RXD_input[5]^DeScrambler_Register[13];
    assign  DeScr_wire[45] = RXD_input[45]^RXD_input[6]^DeScrambler_Register[12];
    assign  DeScr_wire[46] = RXD_input[46]^RXD_input[7]^DeScrambler_Register[11];
    assign  DeScr_wire[47] = RXD_input[47]^RXD_input[8]^DeScrambler_Register[10];

    assign  DeScr_wire[48] = RXD_input[48]^RXD_input[9]^DeScrambler_Register[9];
    assign  DeScr_wire[49] = RXD_input[49]^RXD_input[10]^DeScrambler_Register[8];
    assign  DeScr_wire[50] = RXD_input[50]^RXD_input[11]^DeScrambler_Register[7];
    assign  DeScr_wire[51] = RXD_input[51]^RXD_input[12]^DeScrambler_Register[6];
    assign  DeScr_wire[52] = RXD_input[52]^RXD_input[13]^DeScrambler_Register[5];
    assign  DeScr_wire[53] = RXD_input[53]^RXD_input[14]^DeScrambler_Register[4];
    assign  DeScr_wire[54] = RXD_input[54]^RXD_input[15]^DeScrambler_Register[3];

    assign  DeScr_wire[55] = RXD_input[55]^RXD_input[16]^DeScrambler_Register[2];
    assign  DeScr_wire[56] = RXD_input[56]^RXD_input[17]^DeScrambler_Register[1];
    assign  DeScr_wire[57] = RXD_input[57]^RXD_input[18]^DeScrambler_Register[0];
    assign  DeScr_wire[58] = RXD_input[58]^RXD_input[19]^RXD_input[0];
    assign  DeScr_wire[59] = RXD_input[59]^RXD_input[20]^RXD_input[1];
    assign  DeScr_wire[60] = RXD_input[60]^RXD_input[21]^RXD_input[2];
    assign  DeScr_wire[61] = RXD_input[61]^RXD_input[22]^RXD_input[3];
    assign  DeScr_wire[62] = RXD_input[62]^RXD_input[23]^RXD_input[4];
    assign  DeScr_wire[63] = RXD_input[63]^RXD_input[24]^RXD_input[5];

    // Synchronous part of descrambler
    always @(posedge coreclk_out) begin
        RXD_input[63:0] <= RxD_aligned[65:2];
        RX_Sync_header <= RxD_aligned[1:0];
        DeScr_RXD[65:0] <= {DeScr_wire[63:0],RX_Sync_header[1:0]};
        for (i = 0; i < 58; i = i+1) begin
            DeScrambler_Register[i] <= RXD_input[63-i];
        end
    end

    // Decode and check the Descrambled TX data...
    // This is not a complete decoder: It only decodes the
    // block words we expect to see.
    always @(posedge coreclk_out) begin : check_tx
        integer frame_no;
        integer word_no;

         reg [4:0]  tx_column_index;
         reg        start_code_byte_4 ;
         reg        first_data_word ;
         reg [31:0] tx_fcs;
         reg [31:0] delayed_rxd_high;
         reg [31:0] delayed_rxd_low;

        if(reset || ! IDUT.SHELL.SuperCfg.ETH0.ETH.poMMIO_CoreReady) begin
            frame_no <= 0;
            word_no <= 0;
            tx_fcs <= 0;
            delayed_rxd_high <= 0;
            delayed_rxd_low <= 0;
            tx_column_index <= 0;
            start_code_byte_4 <= 0;
            first_data_word <= 0;
            tx_monitor_error <= 1'b0;
        end
        else if(DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'h33)         // Wait for a Start code...
        begin // Start code in byte 4, data in bytes 5, 6, 7
         if(({DeScr_RXD[65:42], 8'hFB} !== 32'h555555FB))
         begin
             $display("Tx data check ERROR!!, frame %d, MonData = %8x at %t ps",
                      frame_no,{DeScr_RXD[65:42],8'h00}, $time);
            tx_monitor_error <= 1'b1;
         end
         else
         begin
            in_a_frame <= 1;
         end
            start_code_byte_4 <= 1;
       end
       else if(DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'h78)
       begin // Start code in byte 0, data on bytes 1..7
         if(({DeScr_RXD[33:10], 8'hFB} !== 32'h555555FB) && (DeScr_RXD[65:34] !== 32'hD5555555))
         begin
            $display("Tx preamble check ERROR!!, frame %d, MonData = %8x at %t ps",
                      frame_no, {DeScr_RXD[33:10],8'h00}, $time);
            tx_monitor_error <= 1'b1;
         end
         else
         begin
            in_a_frame <= 1;
         end
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'hFF))
       begin // T code in 7th byte, data in bytes 1..7
                     calc_crc(4,delayed_rxd_low, tx_fcs);
            calc_crc(4,delayed_rxd_high[31:0], tx_fcs);
            calc_crc(3,DeScr_RXD[33:10], tx_fcs);
            calc_crc(4,~DeScr_RXD[65:34], tx_fcs);
               if (tx_fcs == 32'hffffffff) begin
                  $display("Transmitter: data and CRC check PASS for frame number %d",frame_no);
               end
               else begin
                  $display("CRC mismatch at %t ps", $time);
                  tx_monitor_error <= 1'b1;
               end
            in_a_frame <= 0;
            frame_no <= frame_no + 1;
            delayed_rxd_high <= 0;
            delayed_rxd_low <= 0;
            tx_fcs <= 0;
            first_data_word <= 0;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'hE1))
       begin // T code in 6th byte, data in bytes 1..6
            calc_crc(4,delayed_rxd_low, tx_fcs);
            calc_crc(4,delayed_rxd_high[31:0], tx_fcs);
            calc_crc(2,DeScr_RXD[25:10], tx_fcs);
            calc_crc(1,~DeScr_RXD[33:26], tx_fcs);
            calc_crc(3,~DeScr_RXD[57:34], tx_fcs);
               if (tx_fcs == 32'hffffffff) begin
                  $display("Transmitter: data and CRC check PASS for frame number %d",frame_no);
               end
               else begin
                  $display("CRC mismatch at %t ps", $time);
                  tx_monitor_error <= 1'b1;
               end
            in_a_frame <= 0;
            frame_no <= frame_no + 1;
            delayed_rxd_high <= 0;
            delayed_rxd_low <= 0;
            tx_fcs <= 0;
            first_data_word <= 0;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'hD2))
       begin // T code in 5th byte, data in bytes 1..5
            calc_crc(4,delayed_rxd_low, tx_fcs);
            calc_crc(4,delayed_rxd_high[31:0], tx_fcs);
            calc_crc(1,DeScr_RXD[17:10], tx_fcs);
            calc_crc(2,~DeScr_RXD[33:18], tx_fcs);
            calc_crc(2,~DeScr_RXD[49:34], tx_fcs);
               if (tx_fcs == 32'hffffffff) begin
                  $display("Transmitter: data and CRC check PASS for frame number %d",frame_no);
               end
               else begin
                  $display("CRC mismatch at %t ps", $time);
                  tx_monitor_error <= 1'b1;
               end
            in_a_frame <= 0;
            frame_no <= frame_no + 1;
            delayed_rxd_high <= 0;
            delayed_rxd_low <= 0;
            tx_fcs <= 0;
            first_data_word <= 0;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'hCC))
       begin // T code in 4th byte, data in bytes 1..4
            calc_crc(4,delayed_rxd_low, tx_fcs);
            calc_crc(4,delayed_rxd_high[31:0], tx_fcs);
            calc_crc(3,~DeScr_RXD[33:10], tx_fcs);
            calc_crc(1,~DeScr_RXD[41:34], tx_fcs);
               if (tx_fcs == 32'hffffffff) begin
                  $display("Transmitter: data and CRC check PASS for frame number %d",frame_no);
               end
               else begin
                  $display("CRC mismatch at %t ps", $time);
                  tx_monitor_error <= 1'b1;
               end
            in_a_frame <= 0;
            frame_no <= frame_no + 1;
            delayed_rxd_high <= 0;
            delayed_rxd_low <= 0;
            tx_fcs <= 0;
            first_data_word <= 0;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'hB4))
       begin // T code in 3rd byte, data in bytes 1..3
            calc_crc(4,delayed_rxd_low, tx_fcs);
            calc_crc(3,delayed_rxd_high[23:0], tx_fcs);
            calc_crc(1,~delayed_rxd_high[31:24], tx_fcs);
            calc_crc(3,~DeScr_RXD[33:10], tx_fcs);
               if (tx_fcs == 32'hffffffff) begin
                  $display("Transmitter: data and CRC check PASS for frame number %d",frame_no);
               end
               else begin
                  $display("CRC mismatch at %t ps", $time);
                  tx_monitor_error <= 1'b1;
               end
            in_a_frame <= 0;
            frame_no <= frame_no + 1;
            delayed_rxd_high <= 0;
            delayed_rxd_low <= 0;
            tx_fcs <= 0;
            first_data_word <= 0;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'hAA))
       begin // T code in 2nd byte, data in bytes 1..2
            calc_crc(4,delayed_rxd_low, tx_fcs);
            calc_crc(2,delayed_rxd_high[15:0], tx_fcs);
            calc_crc(2,~delayed_rxd_high[31:16], tx_fcs);
            calc_crc(2,~DeScr_RXD[25:10], tx_fcs);
               if (tx_fcs == 32'hffffffff) begin
                  $display("Transmitter: data and CRC check PASS for frame number %d",frame_no);
               end
               else begin
                  $display("CRC mismatch at %t ps", $time);
                  tx_monitor_error <= 1'b1;
               end
            in_a_frame <= 0;
            frame_no <= frame_no + 1;
            delayed_rxd_high <= 0;
            delayed_rxd_low <= 0;
            tx_fcs <= 0;
            first_data_word <= 0;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'h99))
       begin // T code in 1st byte, data in byte 1
            calc_crc(4,delayed_rxd_low, tx_fcs);
            calc_crc(1,delayed_rxd_high[7:0], tx_fcs);
            calc_crc(3,~delayed_rxd_high[31:8], tx_fcs);
            calc_crc(1,~DeScr_RXD[17:10], tx_fcs);
               if (tx_fcs == 32'hffffffff) begin
                  $display("Transmitter: data and CRC check PASS for frame number %d",frame_no);
               end
               else begin
                  $display("CRC mismatch at %t ps", $time);
                  tx_monitor_error <= 1'b1;
               end
            in_a_frame <= 0;
            frame_no <= frame_no + 1;
            delayed_rxd_high <= 0;
            delayed_rxd_low <= 0;
            tx_fcs <= 0;
            first_data_word <= 0;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'h87))
       begin // T code in byte 0, no data
            calc_crc(4,delayed_rxd_low, tx_fcs);
            calc_crc(4,~delayed_rxd_high, tx_fcs);
               if (tx_fcs == 32'hffffffff) begin
                  $display("Transmitter: data and CRC check PASS for frame number %d",frame_no);
               end
               else begin
                  $display("CRC mismatch at %t ps", $time);
                  tx_monitor_error <= 1'b1;
               end
            in_a_frame <= 0;
            frame_no <= frame_no + 1;
            delayed_rxd_high <= 0;
            delayed_rxd_low <= 0;
            tx_fcs <= 0;
            first_data_word <= 0;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b10)) // All data
       begin
          if (first_data_word) begin
             if (!start_code_byte_4) begin
                calc_crc(4,delayed_rxd_low, tx_fcs);
             end
             calc_crc(4,delayed_rxd_high, tx_fcs);
             start_code_byte_4 <= 0;
          end
          delayed_rxd_high <= DeScr_RXD[65:34];
          delayed_rxd_low <= DeScr_RXD[33:2];
          first_data_word <= 1;
       end
       if (TB_MODE != "BIST") begin
          if(frame_no == 3) // We're done!
             tx_monitor_finished <= 1'b1;
       end
     end

endmodule


module frame_typ_TenGigEthSubSys_X1Y8_R;
    // This module abstracts the frame data for simpler manipulation
    reg [31:0] data [0:31];
    reg [ 3:0] ctrl [0:31];
    reg        error;

    reg `FRAME_TYP bits;

    function `FRAME_TYP tobits;
    input dummy;
    begin
        bits = {data[ 0], data[ 1], data[ 2], data[ 3], data[ 4],
                data[ 5], data[ 6], data[ 7], data[ 8], data[ 9],
                data[10], data[11], data[12], data[13], data[14],
                data[15], data[16], data[17], data[18], data[19],
                data[20], data[21], data[22], data[23], data[24],
                data[25], data[26], data[27], data[28], data[29],
                data[30], data[31], ctrl[ 0], ctrl[ 1], ctrl[ 2],
                ctrl[ 3], ctrl[ 4], ctrl[ 5], ctrl[ 6], ctrl[ 7],
                ctrl[ 8], ctrl[ 9], ctrl[10], ctrl[11], ctrl[12],
                ctrl[13], ctrl[14], ctrl[15], ctrl[16], ctrl[17],
                ctrl[18], ctrl[19], ctrl[20], ctrl[21], ctrl[22],
                ctrl[23], ctrl[24], ctrl[25], ctrl[26], ctrl[27],
                ctrl[28], ctrl[29], ctrl[30], ctrl[31], error};
                tobits = bits;
    end
    endfunction // tobits

    task frombits;
    input `FRAME_TYP frame;
    begin
        bits = frame;
        {data[ 0], data[ 1], data[ 2], data[ 3], data[ 4], data[ 5],
         data[ 6], data[ 7], data[ 8], data[ 9], data[10], data[11],
         data[12], data[13], data[14], data[15], data[16], data[17],
         data[18], data[19], data[20], data[21], data[22], data[23],
         data[24], data[25], data[26], data[27], data[28], data[29],
         data[30], data[31], ctrl[ 0], ctrl[ 1], ctrl[ 2], ctrl[ 3],
         ctrl[ 4], ctrl[ 5], ctrl[ 6], ctrl[ 7], ctrl[ 8], ctrl[ 9],
         ctrl[10], ctrl[11], ctrl[12], ctrl[13], ctrl[14], ctrl[15],
         ctrl[16], ctrl[17], ctrl[18], ctrl[19], ctrl[20], ctrl[21],
         ctrl[22], ctrl[23], ctrl[24], ctrl[25], ctrl[26], ctrl[27],
         ctrl[28], ctrl[29], ctrl[30], ctrl[31], error} = bits;
    end
    endtask // frombits
endmodule // frame_typ
