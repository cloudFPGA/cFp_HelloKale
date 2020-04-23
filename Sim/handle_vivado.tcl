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
#source ../../../cFDK/SRA/LIB/tcl/xpr_settings.tcl
set cFpIpDir $env(cFpIpDir)
set cFpMOD   $env(cFpMOD)
set usedRoleDir $env(usedRoleDir)
set usedRole2Dir $env(usedRole2Dir)
set cFpSRAtype $env(cFpSRAtype)
set cFpRootDir $env(cFpRootDir)
set cFpXprDir $env(cFpXprDir)
set cFpDcpDir $env(cFpDcpDir)

#source constants and functions 
#source xpr_constants.tcl
#source ${cFpRootDir}/cFDK/SRA/LIB/tcl/xpr_constants.tcl
# source board specific settings 
#source ../../../MOD/${cFpMOD}/tcl/xpr_settings.tcl
#source ${cFpRootDir}/cFDK/MOD/${cFpMOD}/tcl/xpr_settings.tcl

#-------------------------------------------------------------------------------
# TOP  Project Settings  
#-------------------------------------------------------------------------------
#set currDir      [pwd]
set rootDir      ${cFpRootDir} 
#set hdlDir       ${rootDir}/hdl
#set hlsDir       ${rootDir}/hls
#set ipDir        ${rootDir}/../../IP/
set ipDir        ${cFpIpDir}
#set tclDir       ${rootDir}/tcl
set tcpTopDir    ${cFpRootDir}/TOP/tcl/
#set xdcDir       ${rootDir}/xdc
#set xdcDir       ../../../MOD/${cFpMOD}/xdc/
set xdcDir       ${cFpRootDir}cFDK/MOD/${cFpMOD}/xdc/
#set xprDir       ${rootDir}/xpr 
set xprDir       ${cFpXprDir}
#set dcpDir       ${rootDir}/dcps
set dcpDir       ${cFpDcpDir}

# Not used: set ipXprDir     ${ipDir}/managed_ip_project
# Not used:set ipXprName    "managed_ip_project"
# Not used: set ipXprFile    [file join ${ipXprDir} ${ipXprName}.xpr ]



#-------------------------------------------------------------------------------
# Xilinx Project Settings
#-------------------------------------------------------------------------------
set xprName      "top${cFpMOD}"

set topName      "top${cFpMOD}"
set topFile      "top.vhdl"

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
add_files -fileset sim_1 -norecurse /${rootDir}/Sim/tb_topFMKU60.sv
update_compile_order -fileset sim_1
update_compile_order -fileset sim_1
launch_simulation
