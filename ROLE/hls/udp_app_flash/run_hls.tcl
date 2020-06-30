# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Sep 2018
# * Authors : Francois Abel  
# * 
# * Description : A Tcl script for the HLS batch syhthesis of the UDP applica-
# *   tion embedded into the  bring-up role of the cloudFPGA module FMKU60.
# * 
# * Synopsis : vivado_hls -f <this_file>
# *
# * Reference documents:
# *  - UG902 / Ch.4 / High-Level Synthesis Reference Guide.
# *
# ******************************************************************************

# User defined settings
#-------------------------------------------------
set projectName    "udp_app_flash"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"

set ipName         ${projectName}
set ipDisplayName  "UDP Application for cFp_BringUp/Role"
set ipDescription  "A set of tests and functions embedded into the bring-up role of the FMKU60."
set ipVendor       "IBM"
set ipLibrary      "hls"
set ipVersion      "1.0"
set ipPkgFormat    "ip_catalog"
set ipRtl          "vhdl"

# Set Project Environment Variables  
#-------------------------------------------------
set currDir        [pwd]
set srcDir         ${currDir}/src
set testDir        ${currDir}/test
set implDir        ${currDir}/${projectName}_prj/${solutionName}/impl/ip 
set repoDir        ${currDir}/../../ip

# Retrieve the HLS target goals from ENV
#-------------------------------------------------
set hlsCSim        $::env(hlsCSim)
set hlsCSynth      $::env(hlsCSynth)
set hlsCoSim       $::env(hlsCoSim)
set hlsRtl         $::env(hlsRtl)

#-------------------------------------------------
# Retrieve the HLS testbench mode from ENV
#-------------------------------------------------
# FYI, the UAF is specified to use the block-level IO protocol 'ap_ctrl_none'. It also uses
# a few configurations signals which are not stream-based and which prevent the design from
# being verified in C/RTL co-simulation mode. In oder to comply with the 'Interface Synthesis
# Requirements' of UG902, the design is compiled with a preprocessor macro that statically 
# defines the testbench mode of operation.
# This avoid the following error issued when trying to C/RTL co-simulate this component:
#   @E [SIM-345] Cosim only supports the following 'ap_ctrl_none' designs: (1) combinational
#                designs; (2) pipelined design with task interval of 1; (3) designs with array
#                streaming or hls_stream ports.
#   @E [SIM-4] *** C/RTL co-simulation finished: FAIL ***
#---------------------------------------------------------------------------------------------
if { [info exists ::env(hlsTbMode)] } { set hlsTbMode  $::env(hlsTbMode) } else { set hlsTbMode 0 }

# Open and Setup Project
#-------------------------------------------------
open_project       ${projectName}_prj
set_top            ${projectName}

# Add files
#-------------------------------------------------
add_files          ${currDir}/src/${projectName}.cpp
add_files          ${currDir}/../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/nts_utils.cpp
add_files          ${currDir}/../../../cFDK/SRA/LIB/SHELL/LIB/hls/NTS/SimNtsUtils.cpp
add_files -tb      ${currDir}/test/test_${projectName}.cpp -cflags -DTB_MODE=${hlsTbMode}

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
# [TODO - Check vivado_hls version and only enable this command if >= 2018]
# config_rtl -disable_start_propagation

#----------------------------------------------------
# Configuring the behavior of the front-end compiler
#----------------------------------------------------
#  -name_max_length: Specify the maximum length of the function names. If the length of one name
#                    is over the threshold, the last part of the name will be truncated.
#  -pipeline_loops : Specify the lower threshold used during pipelining loops automatically. The
#                    default is '0' for no automatic loop pipelining. 
#------------------------------------------------------------------------------------------------
config_compile -name_max_length 128 -pipeline_loops 0


# Run C Simulation (refer to UG902)
#-------------------------------------------------
if { $hlsCSim} {
    csim_design -setup -clean -compiler gcc
    csim_design -argv "$hlsTbMode ../../../../test/testVectors/siUSIF_OneDatagram.dat"
    csim_design -argv "$hlsTbMode ../../../../test/testVectors/siUSIF_RampDgrmSize.dat"
    puts "#############################################################"
    puts "####                                                     ####"
    puts "####          SUCCESSFUL END OF C SIMULATION             ####"
    puts "####                                                     ####"
    puts "#############################################################"
}  

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

# Run C/RTL CoSimulation (refer to UG902)
#-------------------------------------------------
if { $hlsCoSim } {
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "$hlsTbMode ../../../../test/testVectors/siUSIF_OneDatagram.dat"
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
            export_design -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
        }
        2 {
            export_design -flow syn -rtl verilog -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
        }
        3 {
            export_design -flow impl -rtl verilog -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
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

# Exit Vivado HLS
#--------------------------------------------------
exit


