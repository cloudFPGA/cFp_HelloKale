# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Sep 2018
# * Authors : Francois Abel, Burkhard Ringlein
# *
# * Description : A makefile that invokes all the HLS synthesis batches located 
# *   in the current sub-directories.
# *
# ******************************************************************************

.PHONY: all clean mem_test_flash tcp_app_flash tcp_shell_if udp_app_flash udp_shell_if


#-- Targets --------------------------------------------------------------------
all:   mem_test_flash tcp_app_flash       tcp_shell_if       udp_app_flash       udp_shell_if       ## Run the HLS synthesis and IP Core export on all sub-directories

clean:                tcp_app_flash.clean tcp_shell_if.clean udp_app_flash.clean udp_shell_if.clean ## Cleans all the local HLS projects

csim:                 tcp_app_flash.csim  tcp_shell_if.csim  udp_app_flash.csim  udp_shell_if.csim  ## Runs HLS C simulation on all sub-directories

cosim:                tcp_app_flash.cosim tcp_shell_if.cosim udp_app_flash.cosim udp_shell_if.cosim ## Runs HLS C/RTL cosimulation on all sub-directories

*:
	@echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $@

%.clean:
	@echo ---- Cleaning [ $@ ] ---------------------------- 
	@$(MAKE) -C $* clean

%.csim: 
	@echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $* csim

%.cosim: 
	@echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $* cosim


help:                              ## Shows this help message
    # This target is for self documentation of the Makefile. 
    # Every text starting with '##' and placed after a target will be considered as helper text.
	@echo
	@echo 'Usage:'
	@echo '    make [target]'
	@echo	
	@echo 'Targets:'
	@egrep '^(.+)\:\ .*##\ (.+)' ${MAKEFILE_LIST} | /bin/sed -n 's/\([a-zA-Z0-9]*\): \(.*\)##\(.*\)/\1 ;\3/p' | column -t -c 2 -s ';' | /bin/sed -e 's/^/    /'
	@echo
