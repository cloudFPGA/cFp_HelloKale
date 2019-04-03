# ******************************************************************************
# *
# *                        Zurich cloudFPGA
# *            All rights reserved -- Property of IBM
# *
# *-----------------------------------------------------------------------------
# *
# * Title   : Default timing constraint file for the Flash version of the ROLE.
# *
# * File    : roleFlash.xdc
# *
# * Created : Feb. 2018
# * Authors : Francois Abel <fab@zurich.ibm.com>
# *
# * Devices : xcku060-ffva1156-2-i
# * Tools   : Vivado v2016.4, 2017.4 (64-bit)
# * Depends : None
# *
# * Description : This file contains all the timing constraints for synthesizing
# *   the Flash version of the ROLE embedded into the FMKU60 Flash device. These
# *   constraints are defined for the ROLE as if is was a standalone design. 
# *
# *-----------------------------------------------------------------------------
# * Comments:
# *  - According to UG1119-Lab1, "If the created clock is internal to the IP 
# *    (GT), or if the IP contains an input buffer (IBUF), the create_clock 
# *    constraint should stay in the IP XDC file because it is needed to define 
# *    local clocks."
# *
# ******************************************************************************


#=====================================================================
# Main synchronous clock (generated by the SHELL). 
#=====================================================================

create_clock -period 6.400 [ get_ports piSHL_156_25Clk ]

#=====================================================================
# Free running clock (generated by the TOP). 
#=====================================================================

create_clock -period 6.400 [ get_ports piTOP_250_00Clk ]

