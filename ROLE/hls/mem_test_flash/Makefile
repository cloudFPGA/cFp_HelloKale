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
# *
# * Description : A makefile that synthesizes and packages the current HLS core
# *               as an IP.
# * 
# * Synopsis:
# *   make help    : Displays a help message.
# *   make clean   : Cleanup of the current directory.
# *   make         : Runs the default build for this core.
# *
# ******************************************************************************

# Name of the HLS core to build
ipName =mem_test_flash

# Dependencies
SRC_DEPS := $(shell find ./src/ -type f)

.PHONY: all clean project csim cosim cosim_view csynth regression help ipClean ipProject ip ipSyn ipImpl

${ipName}_prj/solution1/impl/ip: $(SRC_DEPS)
	@#rm -rf $@
	rm -rf ${ipName}_prj
	@cp ./src/memory_size_real.hpp ./src/dynamic.hpp
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=1; vivado_hls -f run_hls.tcl
	@#touch ${ipName}_prj
	@rm -f ./src/dynamic.hpp
	@touch $@
	@touch ../../.ip_guard

#-- Targets --------------------------------------------------------------------

all: ${ipName}_prj/solution1/impl/ip

clean: ## Cleans the current project
	${RM} -rf ${ipName}_prj
	${RM} vivado*.log

project: ## Creates the HLS project
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl	

csim: ## Runs the HLS C simulation
	@cp ./src/memory_size_debug.hpp ./src/dynamic.hpp
	export hlsCSim=1; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl
	@rm -f ./src/dynamic.hpp

cosim: ## Runs the HLS C/RTL cosimulation
	@#rm -rf ${ipName}_prj
	@/bin/echo -e "This CoSim runs with smaller test sizes than the synthezised design!\nSo it must be rebuild again for the synthesis.\n"
	@cp ./src/memory_size_debug.hpp ./src/dynamic.hpp
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=1; export hlsRtl=0; vivado_hls -f run_hls.tcl
	@rm -rf ${ipName}_prj/solution1/impl/ip
	@rm -f ./src/dynamic.hpp

cosim_view: ## Opens the wave viewer
	@/bin/echo -e "current_fileset\nopen_wave_database $(ipName)_main.wdb\n" > ./$(ipName)_prj/solution1/sim/verilog/open_wave.tcl
	cd ./$(ipName)_prj/solution1/sim/verilog/; vivado -source open_wave.tcl

regression:
	@echo '[TODO]'

help: ## Shows this help message
    # This target is for self documentation of the Makefile. 
    # Every text starting with '##' and placed after a target will be considered as helper text.
	@echo
	@echo 'Usage:'
	@echo '    make [target]'
	@echo	
	@echo 'Targets:'
	@egrep '^(.+)\:\ ##\ (.+)' ${MAKEFILE_LIST} | column -t -c 2 -s ':#' | sed -e 's/^/    /' 
	@echo

ipClean:                 ## Cleans the IP-TOP project
	$(MAKE) clean
	@echo '[TODO]'

ipProject:               ## Creates the IP-TOP project for exporting and packaging
	@echo '[TODO]'

ip:                      ## Exports the RTL design and packages it as an IP
	$(MAKE) ipClean
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=1; vivado_hls -f run_hls.tcl

ipSyn:                   ## Exports the RTL design and executes logic synthesis to obtain more accurate numbers
	$(MAKE) ipClean
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=2; vivado_hls -f run_hls.tcl ip_top

ipImpl:                  ## Exports the RTL design and performs both RTL synthesis and implementation
	$(MAKE) ipClean
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=3; vivado_hls -f run_hls.tcl ip_top









