# *
# * Copyright 2016 -- 2020 IBM Corporation
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
# * 
# * Description : A Tcl script to simulate, synthesize and package the current
# *   HLS core as an IP. This script handles two projects:
# *     1) 'ip_core' which is used to csim, csynth and cosim, 
# *     2) 'ip_top'  which is used to export and package the IP. 
# * 
# * Synopsis: 
# *    vivado_hls -f <this_file> [ip_core|ip_top]
# *
# * Reference documents:
# *  - UG902 / Ch.4 / High-Level Synthesis Reference Guide.
# *
# ******************************************************************************

# User defined settings
#-------------------------------------------------
set ipName         "udp_app_flash"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"

set ipDisplayName  "UDP Application Flash"
set ipDescription  "A set of tests and functions embedded into the bring-up role of the FMKU60"
set ipVendor       "IBM"
set ipLibrary      "hls"
set ipVersion      "1.0"
set ipPkgFormat    "ip_catalog"
set ipRtl          "verilog"

# Retrieve the arguments passed to this script
#-------------------------------------------------
if { $argc == 2 } {
    set ipProjectName "ip_core"
} elseif { $argc == 3 } {
    set ipProjectName [lindex $argv 2]
    if { ![string equal ${ipProjectName} ip_core] && ![string equal ${ipProjectName} ip_top] } {
        puts "####"
        puts "####  ERROR: Unknown project name passed as 3rd argument."
        puts "####    Expected: -f run_hls.tcl [ip_core|ip_top]"
        puts "####    Received: $argv"
        puts "####"
        exit 2  
    }
} else {
    puts "####"
    puts "####  ERROR: This script requires 2 or 3 arguments."
    puts "####    Expected: -f run_hls.tcl [a-project-name]"
    puts "####    Received: $argv"
    puts "####"
    exit 2
}

# Retrieve the HLS target goals from ENV
#-------------------------------------------------
set hlsCSim        $::env(hlsCSim)
set hlsCSynth      $::env(hlsCSynth)
set hlsCoSim       $::env(hlsCoSim)
set hlsRtl         $::env(hlsRtl)

# Set Project Environment Variables  
#-------------------------------------------------
set currDir        [pwd]
set srcDir         ${currDir}/src
set testDir        ${currDir}/test

#-------------------------------------------------
# Retrieve the HLS testbench mode from ENV
#-------------------------------------------------
# FYI, the UAF is specified to use the block-level IO protocol 'ap_ctrl_none'. It also uses
# a few configurations signals which are not stream-based and which prevent the design from
# being verified in C/RTL co-simulation mode. In oder to comply with the 'Interface Synthesis
# Requirements' of UG902, the design is compiled with a preprocessor macro that statically 
# defines the testbench mode of operation.
# This avoids the following error issued when trying to C/RTL co-simulate this component:
#   @E [SIM-345] Cosim only supports the following 'ap_ctrl_none' designs: (1) combinational
#                designs; (2) pipelined design with task interval of 1; (3) designs with array
#                streaming or hls_stream ports.
#   @E [SIM-4] *** C/RTL co-simulation finished: FAIL ***
#---------------------------------------------------------------------------------------------
if { [info exists ::env(hlsTbMode)] } { set hlsTbMode  $::env(hlsTbMode) } else { set hlsTbMode 0 }

# Open Project
#-------------------------------------------------
open_project  ${ipProjectName}_prj

# Add files
#-------------------------------------------------
add_files          ${currDir}/src/${ipName}.cpp
add_files          ${currDir}/../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts_utils.cpp
add_files          ${currDir}/../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/SimNtsUtils.cpp

if { [string equal ${ipProjectName} ip_core] } {
    set_top       ${ipName}
    add_files -tb ${testDir}/test_${ipName}.cpp -cflags -DTB_MODE=${hlsTbMode}
} elseif { [string equal ${ipProjectName} ip_top] } {
    set_top       ${ipName}_top
    add_files     ${srcDir}/${ipName}_top.cpp
    add_files -tb ${testDir}/test_${ipName}_top.cpp -cflags -DTB_MODE=${hlsTbMode} 
}

# Create a solution
#-------------------------------------------------
open_solution      ${solutionName}
set_part           ${xilPartName}
create_clock       -period 6.4 -name default

#--------------------------------------------
# Controlling the Reset Behavior (see UG902)
#--------------------------------------------
#  - control: This is the default and ensures all control registers are reset. Control registers 
#             are those used in state machines and to generate I/O protocol signals. This setting 
#             ensures the design can immediately start its operation state.
#  - state  : This option adds a reset to control registers (as in the control setting) plus any 
#             registers or memories derived from static and global variables in the C code. This 
#             setting ensures static and global variable initialized in the C code are reset to
#             their initialized value after the reset is applied.
#------------------------------------------------------------------------------------------------
config_rtl -reset control

#--------------------------------------------
# Specifying Compiler-FIFO Depth (see UG902)
#--------------------------------------------
# Start Propagation 
#  - disable: : The compiler might automatically create a start FIFO to propagate a start token
#               to an internal process. Such FIFOs can sometimes be a bottleneck for performance,
#               in which case you can increase the default size (fixed to 2). However, if an
#               unbounded slack between producer and consumer is needed, and internal processes
#               can run forever, fully and safely driven by their inputs or outputs (FIFOs or
#               PIPOs), these start FIFOs can be removed, at user's risk, locally for a given 
#               dataflow region.
#------------------------------------------------------------------------------------------------
set VIVADO_VERSION [file tail $::env(XILINX_VIVADO)]
if { [format "%.1f" ${VIVADO_VERSION}] > 2017.4 } { 
	config_rtl -disable_start_propagation
}

#----------------------------------------------------
# Configuring the behavior of the front-end compiler
#----------------------------------------------------
#  -name_max_length: Specify the maximum length of the function names. If the length of one name
#                    is over the threshold, the last part of the name will be truncated.
#  -pipeline_loops : Specify the lower threshold used during pipelining loops automatically. The
#                    default is '0' for no automatic loop pipelining. 
#------------------------------------------------------------------------------------------------
config_compile -name_max_length 256 -pipeline_loops 0


#-------------------------------------------------
# Run C Simulation (refer to UG902)
#-------------------------------------------------
if { $hlsCSim} {
    csim_design -setup -clean -compiler gcc
    puts "#############################################################"
    puts "####                                                     ####"
    puts "####          SUCCESSFUL END OF COMPILATION              ####"
    puts "####                                                     ####"
    puts "#############################################################"
    csim_design -argv "$hlsTbMode ../../../../test/testVectors/siUSIF_OneDatagram.dat"
    csim_design -argv "$hlsTbMode ../../../../test/testVectors/siUSIF_TwoDatagrams.dat"
    csim_design -argv "$hlsTbMode ../../../../test/testVectors/siUSIF_RampDgrmSize.dat"
    puts "#############################################################"
    puts "####                                                     ####"
    puts "####          SUCCESSFUL END OF C SIMULATION             ####"
    puts "####                                                     ####"
    puts "#############################################################"
}  

#-------------------------------------------------
# Run C Synthesis (refer to UG902)
#-------------------------------------------------
if { $hlsCSynth} { 
    csynth_design
    puts "#############################################################"
    puts "####                                                     ####"
    puts "####          SUCCESSFUL END OF SYNTHESIS                ####"
    puts "####                                                     ####"
    puts "#############################################################"
}

#-------------------------------------------------
# Run C/RTL CoSimulation (refer to UG902)
#-------------------------------------------------
if { $hlsCoSim } {
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "$hlsTbMode ../../../../test/testVectors/siUSIF_OneDatagram.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "$hlsTbMode ../../../../test/testVectors/siUSIF_TwoDatagrams.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "$hlsTbMode ../../../../test/testVectors/siUSIF_RampDgrmSize.dat"
    puts "#############################################################"
    puts "####                                                     ####"
    puts "####          SUCCESSFUL END OF CO-SIMULATION            ####"
    puts "####                                                     ####"
    puts "#############################################################"
}

#-----------------------------
# Export RTL (refer to UG902)
#-----------------------------
#
# -description <string>
#    Provides a description for the generated IP Catalog IP.
# -display_name <string>
#    Provides a display name for the generated IP.
# -flow (syn|impl)
#    Obtains more accurate timing and utilization data for the specified HDL using RTL synthesis.
# -format (ip_catalog|sysgen|syn_dcp)
#    Specifies the format to package the IP.
# -ip_name <string>
#    Provides an IP name for the generated IP.
# -library <string>
#    Specifies  the library name for the generated IP catalog IP.
# -rtl (verilog|vhdl)
#    Selects which HDL is used when the '-flow' option is executed. If not specified, verilog is
#    the default language.
# -vendor <string>
#    Specifies the vendor string for the generated IP catalog IP.
# -version <string>
#    Specifies the version string for the generated IP catalog.
# -vivado_synth_design_args {args...}
#    Specifies the value to pass to 'synth_design' within the export_design -evaluate Vivado synthesis run.
# -vivado_report_level <value>
#    Specifies the utilization and timing report options.
#---------------------------------------------------------------------------------------------------
if { $hlsRtl } {
    switch $hlsRtl {
        1 {
            export_design            -rtl ${ipRtl} -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
        }
        2 {
            export_design -flow syn  -rtl ${ipRtl} -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
        }
        3 {
            export_design -flow impl -rtl ${ipRtl} -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
        }
        default { 
            puts "####  INVALID VALUE ($hlsRtl) ####"
            exit 1
        }
    }
    puts "#############################################################"
    puts "####                                                     ####"
    puts "####          SUCCESSFUL EXPORT OF THE DESIGN            ####"
    puts "####                                                     ####"
    puts "#############################################################"

}

#-------------------------------------------------
# Exit Vivado HLS
#-------------------------------------------------
exit


