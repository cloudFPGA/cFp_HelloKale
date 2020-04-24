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
source ./cFDK/SRA/LIB/tcl/xpr_settings.tcl

set usedRoleType  "Role_${cFpSRAtype}"
set usedShellType "Shell_${cFpSRAtype}"


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

open_project ${xprDir}/${xprName}.xpr
set_property SOURCE_SET sources_1 [get_filesets sim_1]
set_property -name {xsim.simulate.runtime} -value {50us} -objects [get_filesets sim_1]

remove_files ${rootDir}/TOP/hdl/top.vhdl

add_files -norecurse ${rootDir}/Sim/top.vhdl
set_property file_type {VHDL 2008} [get_files  ${rootDir}/Sim/top.vhdl]
 
update_compile_order -fileset sources_1
update_compile_order -fileset sources_1

add_files -fileset sim_1 -norecurse ${rootDir}/Sim/tb_topFMKU60.sv
update_compile_order -fileset sim_1
update_compile_order -fileset sim_1
launch_simulation
