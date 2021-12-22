# *
# * Copyright 2016 -- 2021 IBM Corporation
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *     http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *


# *****************************************************************************
# *                            cloudFPGA
# *----------------------------------------------------------------------------
# * Created : Jan 2018
# * Authors :  Francois Abel, Burkhard Ringlein
# * 
# * Description : A Tcl script for the HLS batch syhthesis 
# * 
# * Synopsis : vivado_hls -f <this_file>
# * Version: 2.0
# *
# * Reference documents:
# *  - UG902 / Ch.4 / High-Level Synthesis Reference Guide.
# *
# *-----------------------------------------------------------------------------
# * Modification History:
# ******************************************************************************

# User defined settings
#-------------------------------------------------
set appName        "mem_test_flash"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"
set projectName    "${appName}"

set ipName         ${projectName}
set ipDisplayName  "MemTestFlash"
set ipDescription  "DDR4 Meomry Test (single channel)"
set ipVendor       "IBM"
set ipLibrary      "hls"
set ipVersion      "1.0"
set ipPkgFormat    "ip_catalog"

# Set Project Environment Variables  
#-------------------------------------------------
set currDir      [pwd]
set srcDir       ${currDir}/src
set tbDir        ${currDir}/tb
#set implDir      ${currDir}/${appName}_prj/${solutionName}/impl/ip 
#set repoDir      ${currDir}/../../ip


# Get targets out of env  
#-------------------------------------------------
set hlsSim       $::env(hlsCSim)
set hlsCSynth    $::env(hlsCSynth)
set hlsCoSim     $::env(hlsCoSim)
set hlsRtl       $::env(hlsRtl)

# Open and Setup Project
#-------------------------------------------------
open_project  ${projectName}_prj
set_top       ${appName}_main
#set_top       mpi_wrapper

# Add files
#-------------------------------------------------
#for DEBUG flag 
#if { $hlsSim || $hlsCoSim } { 
#  add_files     ${srcDir}/${appName}.cpp -cflags "-DCOSIM -DDEBUG"
#  add_files     ${srcDir}/${appName}.hpp -cflags "-DCOSIM -DDEBUG"
#  
#  add_files -tb ${tbDir}/tb_${appName}.cpp -cflags "-DDEBUG"
#
#} else {
  add_files     ${srcDir}/${appName}.hpp -cflags "-DCOSIM"
  add_files     ${srcDir}/${appName}.cpp -cflags "-DCOSIM"

  add_files -tb ${tbDir}/tb_${appName}.cpp 
#}

#since DEBUG flag won't work....
add_files ${srcDir}/dynamic.hpp

# Create a solution
#-------------------------------------------------
open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

# Set reset behavior to control mode (see UG902)
#-------------------------------------------------
config_rtl -reset control

# Disable start propagation FIFOs (see UG902)
#-------------------------------------------------
# [TODO - Check vivado_hls version and only enable this command if >= 2018]
# config_rtl -disable_start_propagation

# Run C Simulation and Synthesis
#-------------------------------------------------
if { $hlsSim } { 
  csim_design -compiler gcc -clean
}

# Run C Synthesis (refer to UG902)
#-------------------------------------------------
if { $hlsCSynth} { 
    csynth_design
}

# Run C/RTL CoSimulation (refer to UG902)
#-------------------------------------------------
if { $hlsCoSim } {
    cosim_design -compiler gcc -trace_level all
}
  
# Export RTL (refer to UG902)
#   -format ( sysgen | ip_catalog | syn_dcp )
#-------------------------------------------------
if { $hlsRtl } {
    export_design -rtl vhdl -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
}

# Exit Vivado HLS
#--------------------------------------------------
exit
