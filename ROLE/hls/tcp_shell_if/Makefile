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
# * Description : A makefile to simulate, synthesize and package the current
# *   HLS core as an IP. This build consist of two projects:
# *     1) 'ip_core' which is used to csim, csynth and cosim, 
# *     2) 'ip_top'  which is used to export and package the IP.
# * 
# * Synopsis:
# *   make help
# *
# ******************************************************************************

# Dependencies
SRC_DEPS := $(shell find ./src/ -type f)

.PHONY: all clean project csim cosim csynth regression help ipClean ipProject ip ipSyn ipImpl

.csim_guard: $(SRC_DEPS)
	$(MAKE) clean
	@touch $@

.synth_guard: $(SRC_DEPS)
	$(MAKE) clean
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl ip_core
	@touch $@

ip_top_prj/solution1/impl/ip: $(SRC_DEPS)
	$(MAKE) ipClean
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=1; vivado_hls -f run_hls.tcl ip_top
	@touch $@
	@touch ../../../ROLE/.ip_guard

#-- Targets --------------------------------------------------------------------

all: ip

clean:                   ## Cleans the IP-CORE project
	${RM} -rf ip_core_prj
	${RM} vivado*.log
	@${RM} .csim_guard
	@${RM} .synth_guard

project:                 ## Creates the IP-CORE project for simulation, synthesis and co-simulation
	$(MAKE) clean
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl ip_core

csim: .csim_guard        ## Runs the HLS C simulation of the IP-CORE project
	export hlsCSim=1; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl

csynth: .synth_guard     ## Runs the HLS C synthesis of the IP-CORE project

cosim: .synth_guard      ## Runs the HLS C/RTL cosimulation of the IP-CORE project 
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=1; export hlsRtl=0; vivado_hls -f run_hls.tcl

regression: .synth_guard ## Runs a sequence of HLS csim, csynth and cosim over the IP-CORE project
	export hlsCSim=1; export hlsCSynth=1; export hlsCoSim=1; export hlsRtl=0; vivado_hls -f run_hls.tcl

help:                    ## Shows this help message
    # This target is for self documentation of the Makefile. 
    # Every text starting with '##' and placed after a target will be considered as helper text.
	@echo
	@echo 'Usage:'
	@echo '    make [target]'
	@echo
	@echo 'Targets:'
	@egrep '^(.+)\:\ .*##\ (.+)' ${MAKEFILE_LIST} | sed -n 's/\([a-zA-Z0-9]*\): \(.*\)##\(.*\)/\1 ;\3/p' | column -t -c 2 -s ';' | sed -e 's/^/    /'
	@echo

ipClean:                 ## Cleans the IP-TOP project
	${RM} -rf ip_top_prj
	${RM} vivado*.log

ipProject:               ## Creates the IP-TOP project for exporting and packaging
	$(MAKE) ipClean
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl ip_top

ip:                      ## Exports the RTL design and packages it as an IP
	$(MAKE) ip_top_prj/solution1/impl/ip

ipSyn:                   ## Exports the RTL design and executes logic synthesis to obtain more accurate numbers
	$(MAKE) ipClean
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=2; vivado_hls -f run_hls.tcl ip_top

ipImpl:                  ## Exports the RTL design and performs both RTL synthesis and implementation
	$(MAKE) ipClean
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=3; vivado_hls -f run_hls.tcl ip_top

