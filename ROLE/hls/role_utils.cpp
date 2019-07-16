/*****************************************************************************
 * @file       : role_utils.cpp
 * @brief      : Utilities for the user application role (ROLE|ROL).
 *
 * System:     : cloudFPGA
 * Component   : Role
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include <queue>
#include <string>

#include "role.hpp"

using namespace std;


/*****************************************************************************
 * @brief Swap the two bytes of a word (.i.e, 16 bits).
 *
 * @param[in] inpWord, the 16-bit unsigned data to swap.
 *
 * @return a 16-bit unsigned data.
 *****************************************************************************/
ap_uint<16> byteSwap16(ap_uint<16> inputVector) {
    return (inputVector.range(7,0), inputVector(15, 8));
}

/*****************************************************************************
 * @brief Swap the four bytes of a double-word (.i.e, 32 bits).
 *
 * @param[in] inpDWord, a 32-bit unsigned data.
 *
 * @return a 32-bit unsigned data.
 *****************************************************************************/
ap_uint<32> byteSwap32(ap_uint<32> inputVector) {
    return (inputVector.range( 7, 0), inputVector(15,  8),
        inputVector.range(23,16), inputVector(31, 24));
}

/*****************************************************************************
 * @brief Returns the number of valid bytes in an AxiWord.
 * @param[in] The 'tkeep' field of the AxiWord.
 *****************************************************************************/
ap_uint<4> keepToLen(ap_uint<8> keepValue) {
    ap_uint<4> count = 0;
    switch(keepValue){
        case 0x01: count = 1; break;
        case 0x03: count = 2; break;
        case 0x07: count = 3; break;
        case 0x0F: count = 4; break;
        case 0x1F: count = 5; break;
        case 0x3F: count = 6; break;
        case 0x7F: count = 7; break;
        case 0xFF: count = 8; break;
    }
    return count;
}

/*****************************************************************************
 * @brief Returns the 'tkeep' field of an AxiWord as a function of the number
 *  of valid bytes in that word.
 * @param[in] The number of valid bytes in an AxiWord.
 *****************************************************************************/
ap_uint<8> lenToKeep(ap_uint<4> noValidBytes) {
    ap_uint<8> keep = 0;

    switch(noValidBytes) {
        case 1: keep = 0x01; break;
        case 2: keep = 0x03; break;
        case 3: keep = 0x07; break;
        case 4: keep = 0x0F; break;
        case 5: keep = 0x1F; break;
        case 6: keep = 0x3F; break;
        case 7: keep = 0x7F; break;
        case 8: keep = 0xFF; break;
    }
    return keep;
}



