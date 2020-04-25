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
create_fileset -simset sim_1
set_property SOURCE_SET sources_1 [get_filesets sim_1]
current_fileset -simset [ get_filesets sim_1 ]

set_property -name {xsim.simulate.runtime} -value {50us} -objects [get_filesets sim_1]

export_ip_user_files -of_objects  [get_files ${rootDir}/TOP/hdl/top.vhdl] -no_script -reset -force -quiet
remove_files ${rootDir}/TOP/hdl/top.vhdl -verbose
update_compile_order -fileset sources_1 -verbose
update_compile_order -fileset sources_1 -verbose


add_files -norecurse ${rootDir}/Sim/top.vhdl -verbose
update_compile_order -fileset sources_1 -verbose
update_compile_order -fileset sources_1 -verbose

set_property top topFMKU60 [get_filesets sim_1]
set_property top_lib xil_defaultlib [get_filesets sim_1]
set_property top               topFMKU60           [ current_fileset ] -verbose
set_property top_file          ${rootDir}/TOP/hdl/top.vhdl [ current_fileset ] -verbose
update_ip_catalog


set_property file_type {VHDL 2008} [get_files  ${rootDir}/Sim/top.vhdl]
update_compile_order -fileset sources_1 -verbose
update_compile_order -fileset sources_1 -verbose

set_property SOURCE_SET sources_1 [get_filesets sim_1]

add_files -fileset sim_1 -norecurse ${rootDir}/Sim/tb_topFMKU60.sv
update_compile_order -fileset sim_1
update_compile_order -fileset sim_1

generate_target Simulation [get_files ${rootDir}/ROLE/ip/MemTestFlash/MemTestFlash.xci]
export_ip_user_files -of_objects [get_files ${rootDir}/ROLE/ip/MemTestFlash/MemTestFlash.xci] -no_script -sync -force
export_simulation -of_objects [get_files ${rootDir}/ROLE/ip/MemTestFlash/MemTestFlash.xci] -directory ${rootDir}/xpr/topFMKU60.ip_user_files/sim_scripts -ip_user_files_dir ${rootDir}/xpr/topFMKU60.ip_user_files -ipstatic_source_dir ${rootDir}/xpr/topFMKU60.ip_user_files/ipstatic -lib_map_path [list {modelsim=${rootDir}/xpr/topFMKU60.cache/compile_simlib/modelsim} {questa=${rootDir}/xpr/topFMKU60.cache/compile_simlib/questa} {ies=${rootDir}/xpr/topFMKU60.cache/compile_simlib/ies} {vcs=${rootDir}/xpr/topFMKU60.cache/compile_simlib/vcs} {riviera=${rootDir}/xpr/topFMKU60.cache/compile_simlib/riviera}] -use_ip_compiled_libs -force

generate_target Simulation [get_files ${rootDir}/ROLE/ip/TcpApplicationFlash/TcpApplicationFlash.xci]

export_ip_user_files -of_objects [get_files ${rootDir}/ROLE/ip/TcpApplicationFlash/TcpApplicationFlash.xci] -no_script -sync -force
export_simulation -of_objects [get_files ${rootDir}/ROLE/ip/TcpApplicationFlash/TcpApplicationFlash.xci] -directory ${rootDir}/xpr/topFMKU60.ip_user_files/sim_scripts -ip_user_files_dir ${rootDir}/xpr/topFMKU60.ip_user_files -ipstatic_source_dir ${rootDir}/xpr/topFMKU60.ip_user_files/ipstatic -lib_map_path [list {modelsim=${rootDir}/xpr/topFMKU60.cache/compile_simlib/modelsim} {questa=${rootDir}/xpr/topFMKU60.cache/compile_simlib/questa} {ies=${rootDir}/xpr/topFMKU60.cache/compile_simlib/ies} {vcs=${rootDir}/xpr/topFMKU60.cache/compile_simlib/vcs} {riviera=${rootDir}/xpr/topFMKU60.cache/compile_simlib/riviera}] -use_ip_compiled_libs -force -quiet
generate_target Simulation [get_files ${rootDir}/ROLE/ip/UdpApplicationFlash/UdpApplicationFlash.xci]

export_ip_user_files -of_objects [get_files ${rootDir}/ROLE/ip/UdpApplicationFlash/UdpApplicationFlash.xci] -no_script -sync -force
export_simulation -of_objects [get_files ${rootDir}/ROLE/ip/UdpApplicationFlash/UdpApplicationFlash.xci] -directory ${rootDir}/xpr/topFMKU60.ip_user_files/sim_scripts -ip_user_files_dir ${rootDir}/xpr/topFMKU60.ip_user_files -ipstatic_source_dir ${rootDir}/xpr/topFMKU60.ip_user_files/ipstatic -lib_map_path [list {modelsim=${rootDir}/xpr/topFMKU60.cache/compile_simlib/modelsim} {questa=${rootDir}/xpr/topFMKU60.cache/compile_simlib/questa} {ies=${rootDir}/xpr/topFMKU60.cache/compile_simlib/ies} {vcs=${rootDir}/xpr/topFMKU60.cache/compile_simlib/vcs} {riviera=${rootDir}/xpr/topFMKU60.cache/compile_simlib/riviera}] -use_ip_compiled_libs -force -quiet
generate_target Simulation [get_files ${rootDir}/ROLE/ip/TcpShellInterface/TcpShellInterface.xci]

export_ip_user_files -of_objects [get_files ${rootDir}/ROLE/ip/TcpShellInterface/TcpShellInterface.xci] -no_script -sync -force 
export_simulation -of_objects [get_files ${rootDir}/ROLE/ip/TcpShellInterface/TcpShellInterface.xci] -directory ${rootDir}/xpr/topFMKU60.ip_user_files/sim_scripts -ip_user_files_dir ${rootDir}/xpr/topFMKU60.ip_user_files -ipstatic_source_dir ${rootDir}/xpr/topFMKU60.ip_user_files/ipstatic -lib_map_path [list {modelsim=${rootDir}/xpr/topFMKU60.cache/compile_simlib/modelsim} {questa=${rootDir}/xpr/topFMKU60.cache/compile_simlib/questa} {ies=${rootDir}/xpr/topFMKU60.cache/compile_simlib/ies} {vcs=${rootDir}/xpr/topFMKU60.cache/compile_simlib/vcs} {riviera=${rootDir}/xpr/topFMKU60.cache/compile_simlib/riviera}] -use_ip_compiled_libs -force -quiet
generate_target Simulation [get_files ${rootDir}/ip/TenGigEthSubSys_X1Y8_R/TenGigEthSubSys_X1Y8_R.xci]

launch_simulation
