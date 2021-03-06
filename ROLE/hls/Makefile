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
# * Description : A makefile to invoke all the HLS synthesis batches located 
# *   in the current sub-directories.
# *
# ******************************************************************************

.PHONY: all clean csim cosim regression help ip mem_test_flash tcp_app_flash tcp_shell_if udp_app_flash udp_shell_if 


#-- Targets --------------------------------------------------------------------
all:	mem_test_flash        tcp_app_flash       tcp_shell_if       udp_app_flash       udp_shell_if     ## Run the HLS synthesis and IP Core export on all sub-directories

ip: all  ## Exports the RTL design and packages it as an IP

clean: mem_test_flash.clean tcp_app_flash.clean tcp_shell_if.clean udp_app_flash.clean udp_shell_if.clean ## Cleans all the local HLS projects

csim:                       tcp_app_flash.csim  tcp_shell_if.csim  udp_app_flash.csim  udp_shell_if.csim  ## Runs HLS C simulation on all sub-directories

cosim:                      tcp_app_flash.cosim tcp_shell_if.cosim udp_app_flash.cosim udp_shell_if.cosim ## Runs HLS C/RTL cosimulation on all sub-directories

regression:                 tcp_app_flash.reg   tcp_shell_if.reg   udp_app_flash.reg   udp_shell_if.reg   ## Runs a sequence of HLS csim, csynth and cosim on all all sub-directories

*:
	@echo ---- Building [ $@ ] ----------------------------
	@$(MAKE) -C $@

%.ip:
	@echo ---- Building [ $@ ] ----------------------------
	@$(MAKE) -C $@

%.clean:
	@echo ---- Cleaning [ $@ ] ----------------------------
	@$(MAKE) -C $* clean
	@$(MAKE) -C $* ipClean

%.csim: 
	@echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $* csim

%.cosim: 
	@echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $* cosim

%.reg: 
	@echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $* regression

help:                      ## Shows this help message
    # This target is for self documentation of the Makefile. 
    # Every text starting with '##' and placed after a target will be considered as helper text.
	@echo
	@echo 'Usage:'
	@echo '    make [target]'
	@echo	
	@echo 'Targets:'
	@egrep '^(.+)\:\ .*##\ (.+)' ${MAKEFILE_LIST} | /bin/sed -n 's/\([a-zA-Z0-9]*\): \(.*\)##\(.*\)/\1 ;\3/p' | column -t -c 2 -s ';' | /bin/sed -e 's/^/    /'
	@echo
