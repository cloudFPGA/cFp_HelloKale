#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Apr 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *        TCL file execute the Vivado commands
#  *




package require cmdline

# Set the Global Settings used by the SHELL Project
#-------------------------------------------------------------------------------
#source xpr_settings.tcl
source ../../cFDK/SRA/LIB/tcl/xpr_settings.tcl

# import environment Variables
set usedRole $env(roleName1)
set usedRole2 $env(roleName2)

# Set the Local Settings used by this Script
#-------------------------------------------------------------------------------
set dbgLvl_1         1
set dbgLvl_2         2
set dbgLvl_3         3

#TODO not used any longer?
#source extra_procs.tcl

################################################################################
#                                                                              #
#                        *     *                                               #
#                        **   **    **       *    *    *                       #
#                        * * * *   *  *      *    **   *                       #
#                        *  *  *  *    *     *    * *  *                       #
#                        *     *  ******     *    *  * *                       #
#                        *     *  *    *     *    *   **                       #
#                        *     *  *    *     *    *    *                       #
#                                                                              #
################################################################################

my_info_puts "usedRole  is set to $usedRole"
my_info_puts "usedRole2 is set to $usedRole2"

open_project ${xprDir}/${xprName}.xpr
set_property SOURCE_SET sources_1 [get_filesets sim_1]
set_property -name {xsim.simulate.runtime} -value {50us} -objects [get_filesets sim_1]
add_files -fileset sim_1 -norecurse /${rootDir}/Sim/tb_topFMKU60.sv
update_compile_order -fileset sim_1
update_compile_order -fileset sim_1
launch_simulation


