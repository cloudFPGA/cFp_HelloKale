#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Apr 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *       Makefile to create vivado commands (in .vivado_basecmd.sh)
#  *


TOP_NAME = top$(cFpMOD)
# NO HEADING OR TRAILING SPACES!!
ROLE_DIR_1 =$(usedRoleDir)
ROLE_DIR_2 =$(usedRole2Dir)
SHELL_DIR =$(cFpRootDir)/cFDK/SRA/LIB/SHELL/$(cFpSRAtype)/
MOD_DIR =$(cFpRootDir)/cFDK/MOD/$(cFpMOD)/
#XPR_PROJ = ../xpr/topFMKU60_Flash.xpr
XPR_PROJ =$(cFpXprDir)/top$(cFpMOD).xpr

MONO_LOCK =$(cFpXprDir)/.project_monolithic.stamp

CLEAN_TYPES = *.log *.jou *.str

VEIRFY_RETVAL=47

ifdef cFpMidlwIpDir
MIDLW_DIR=$(cFpRootDir)/cFDK/SRA/LIB/MIDLW/$(cFpSRAtype)/
endif

.PHONY: all clean full_src full_src_pr pr_verify full_src_pr_all monolithic monolithic_incr save_mono_incr 
.PHONY: finalcmd incrcmd onlycmd basecmd monobasecmd monofinalcmd
#.PHONY: full_src_pr_all_mpi monolithic_mpi monolithic_incr_mpi full_src_pr_only_mpi full_src_pr_2_only_mpi mpicmd
# full_src_pr_2_incr_only full_src_pr_incr_only full_src_pr_incr full_src_pr_2_incr 
.PHONY: full_src_pr_only full_src_pr_2_only   monolithic_incr_debug monolithic_debug debugcmd
all: full_src_pr


clean:
	rm -rf $(CLEAN_TYPES)


$(cFpDcpDir): 
	mkdir -p $(cFpDcpDir) 


basecmd:
	@/bin/echo -e '#!/bin/bash\nlength=$$(cat $$0 | grep -v "only" | grep -v "useMPI" | grep -v "vivado" | grep -v "incr" | grep -v "bash" | grep -v "date" | grep -v "echo" | grep -v "grep" | grep -v "more" | wc -l)\nif [  $$length -lt 12 ]; then\n\techo "handle_vivado.tcl: Nothing to do for target. Stop."\nelse\nmore +7 $$0| head -n9 \nvivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -full_src -force \\' > .vivado_basecmd.sh
	@/bin/echo -e $(shell date) > start.date

finalcmd:
	@#more +2 .vivado_basecmd.sh
	@/bin/echo -e '\n\n\n\n\n\n\n\nfi\nretVal=$$?\necho "handle_vivado.tcl returned with $$retVal"\nif [ $$retVal -ne $(VEIRFY_RETVAL) ]; then\n  echo "\n\tERROR: handle_vivado.tcl finished with an unexpected value!"\n  exit -1\nelse\n  retVal=0\nfi\n\ndate > end.date\nexit $$retVal' >> .vivado_basecmd.sh
	@bash .vivado_basecmd.sh
	@rm .vivado_basecmd.sh

incrcmd:
	@/bin/echo -e " -use_incr \\" >> .vivado_basecmd.sh

onlycmd:
	@/bin/echo -e " -only_pr_bitgen \\" >> .vivado_basecmd.sh

#mpicmd:
#	@/bin/echo -e " -useMPI \\" >> .vivado_basecmd.sh

debugcmd:
	@/bin/echo -e " -insert_ila \\" >> .vivado_basecmd.sh

full_src: basecmd $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName1).bit finalcmd

full_src_pr: basecmd $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName1)_pblock_ROLE_partial.bit finalcmd | $(cFpDcpDir)

pr_verify: 
	vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -pr_verify

full_src_pr_2: basecmd $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName2)_pblock_ROLE_partial.bit finalcmd | $(cFpDcpDir)

full_src_pr_all: basecmd $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName1)_pblock_ROLE_partial.bit $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName2)_pblock_ROLE_partial.bit finalcmd | $(cFpDcpDir)

#full_src_pr_incr: basecmd $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName1)_pblock_ROLE_partial.bit incrcmd finalcmd | $(cFpDcpDir)

#full_src_pr_2_incr: basecmd $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName2)_pblock_ROLE_partial.bit incrcmd finalcmd | $(cFpDcpDir)

#full_src_pr_all_incr: basecmd $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName1)_pblock_ROLE_partial.bit $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName2)_pblock_ROLE_partial.bit incrcmd finalcmd | $(cFpDcpDir)


full_src_pr_only: basecmd $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName1)_pblock_ROLE_partial.bit onlycmd finalcmd | $(cFpDcpDir)

full_src_pr_2_only: basecmd $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName2)_pblock_ROLE_partial.bit onlycmd finalcmd | $(cFpDcpDir)

#full_src_pr_incr_only: basecmd $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName1)_pblock_ROLE_partial.bit incrcmd onlycmd finalcmd | $(cFpDcpDir)

#full_src_pr_2_incr_only: basecmd $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName2)_pblock_ROLE_partial.bit incrcmd onlycmd finalcmd | $(cFpDcpDir)


#full_src_pr_all_mpi: basecmd $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName1)_pblock_ROLE_partial.bit $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName2)_pblock_ROLE_partial.bit mpicmd finalcmd | $(cFpDcpDir)

#full_src_pr_only_mpi: basecmd $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName1)_pblock_ROLE_partial.bit onlycmd mpicmd finalcmd | $(cFpDcpDir)

#full_src_pr_2_only_mpi: basecmd $(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName2)_pblock_ROLE_partial.bit onlycmd mpicmd finalcmd | $(cFpDcpDir)


###########################################
# Monolithic
###########################################


$(MONO_LOCK):  $(ROLE_DIR_1)/* $(cFpIpDir) $(SHELL_DIR)/Shell.v | $(XPR_PROJ) $(cFpDcpDir)
	@/bin/echo "-create -synth \\" >> .vivado_basecmd.sh

monobasecmd: basecmd
	@/bin/echo -e "-forceWithoutBB  -synth \\" >> .vivado_basecmd.sh

monofinalcmd: 
	@#more +2 .vivado_basecmd.sh
	@/bin/echo -e '\n\n\n\n\n\n\n\nfi\nretVal=$$?\necho "handle_vivado.tcl returned with $$retVal"\nif [ $$retVal -ne $(VEIRFY_RETVAL) ]; then\n  echo "\n\tERROR: handle_vivado.tcl finished with an unexpected value!"\n  exit -1\nelse\n  retVal=0\nfi\n\ndate > end.date' >> .vivado_basecmd.sh
	@/bin/echo -e "echo \"last monolithic build was successfull at \" > $(MONO_LOCK)" >> .vivado_basecmd.sh
	@/bin/echo -e "date  >> $(MONO_LOCK)" >> .vivado_basecmd.sh
	@/bin/echo -e 'exit $$retVal' >> .vivado_basecmd.sh
	@bash .vivado_basecmd.sh
	@rm .vivado_basecmd.sh

monolithic: monobasecmd $(MONO_LOCK) monofinalcmd

monolithic_proj: basecmd 
	@/bin/echo -e "-forceWithoutBB  -create \\" >> .vivado_basecmd.sh
	@/bin/echo -e "\n\n\n\n\n\n\n\nfi\ndate > end.date" >> .vivado_basecmd.sh
	@/bin/echo -e "\n\ndate > end.date" >> .vivado_basecmd.sh
	@/bin/echo -e "echo \"last monolithic build was successfull at \" > $(MONO_LOCK)" >> .vivado_basecmd.sh
	@/bin/echo -e "date  >> $(MONO_LOCK)" >> .vivado_basecmd.sh
	@bash .vivado_basecmd.sh
	@rm .vivado_basecmd.sh
	

monolithic_incr: monobasecmd $(MONO_LOCK) incrcmd monofinalcmd

#monolithic_mpi: monobasecmd $(MONO_LOCK) mpicmd monofinalcmd
#
#monolithic_incr_mpi: monobasecmd $(MONO_LOCK) incrcmd mpicmd monofinalcmd
#
#monolithic_incr_debug: monobasecmd debugcmd $(MONO_LOCK) monofinalcmd

monolithic_debug: monobasecmd debugcmd $(MONO_LOCK) monofinalcmd

save_mono_incr: 
	@# no dependencies, vivado should better fail on itslef 
	vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs  -forceWithoutBB -save_incr || [ "$$?" -eq "47" ]


#without pr 
$(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName1).bit: $(cFpDcpDir)/2_$(TOP_NAME)_impl_$(roleName1)_complete.dcp
	@/bin/echo "-bitgen  \\" >> .vivado_basecmd.sh
	rm -f $@

$(cFpDcpDir)/2_$(TOP_NAME)_impl_$(roleName1)_complete.dcp: $(cFpIpDir) | $(XPR_PROJ)
	@/bin/echo "-impl  \\" >> .vivado_basecmd.sh
	rm -f $@

#$(cFpDcpDir)/1_$(TOP_NAME)_linked.dcp: $(cFpDcpDir)/0_$(TOP_NAME)_static_without_role.dcp  $(ROLE_DIR)/$(ROLE_NAME_1)/*.dcp
#	@/bin/echo "-link  \\" >> .vivado_basecmd.sh
#	rm -f $@

#both 
#$(XPR_PROJ): ../xdc ../hdl $(SHELL_DIR)/ip $(SHELL_DIR)/hdl | $(cFpDcpDir) 
$(XPR_PROJ): $(MOD_DIR)/xdc ../hdl $(cFpIpDir) $(SHELL_DIR)/Shell.v | $(cFpDcpDir) 
	@/bin/echo "-create -synth \\" >> .vivado_basecmd.sh

##with pr:
$(cFpDcpDir)/0_$(TOP_NAME)_static_without_role.dcp: $(MOD_DIR)/xdc ../hdl $(cFpIpDir) $(SHELL_DIR)/Shell.v | $(XPR_PROJ)
	@/bin/echo "-synth \\" >> .vivado_basecmd.sh
	rm -f $@

# treating Middleware, better avoid PHONY target...
ifdef cFpMidlwIpDir
$(cFpDcpDir)/1_$(TOP_NAME)_linked_pr.dcp: $(cFpDcpDir)/0_$(TOP_NAME)_static_without_role.dcp $(ROLE_DIR_1)/*.dcp $(MIDLW_DIR)/*.dcp
	@/bin/echo "-link  -pr \\" >> .vivado_basecmd.sh
	rm -f $@
else
$(cFpDcpDir)/1_$(TOP_NAME)_linked_pr.dcp: $(cFpDcpDir)/0_$(TOP_NAME)_static_without_role.dcp $(ROLE_DIR_1)/*.dcp
	@/bin/echo "-link  -pr \\" >> .vivado_basecmd.sh
	rm -f $@
endif

$(cFpDcpDir)/2_$(TOP_NAME)_impl_$(roleName1)_complete_pr.dcp: $(cFpDcpDir)/1_$(TOP_NAME)_linked_pr.dcp $(ROLE_DIR_1)/*.dcp $(cFpDcpDir)/0_$(TOP_NAME)_static_without_role.dcp 
	@/bin/echo "-impl  -pr \\" >> .vivado_basecmd.sh
	rm -f $@

$(cFpDcpDir)/3_$(TOP_NAME)_STATIC.dcp: $(cFpDcpDir)/1_$(TOP_NAME)_linked_pr.dcp $(cFpDcpDir)/2_$(TOP_NAME)_impl_$(roleName1)_complete_pr.dcp
	rm -f $@ 

$(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName1)_pblock_ROLE_partial.bit: $(cFpDcpDir)/2_$(TOP_NAME)_impl_$(roleName1)_complete_pr.dcp 
	@/bin/echo "-bitgen  -pr \\" >> .vivado_basecmd.sh 
	rm -f $@


$(cFpDcpDir)/4_$(TOP_NAME)_impl_$(roleName2)_pblock_ROLE_partial.bit: $(cFpDcpDir)/2_$(TOP_NAME)_impl_$(roleName2)_complete_pr.dcp 
	@/bin/echo "-bitgen2 -pr -pr_verify  \\" >> .vivado_basecmd.sh
	rm -f $@

$(cFpDcpDir)/2_$(TOP_NAME)_impl_$(roleName2)_complete_pr.dcp: $(cFpDcpDir)/3_$(TOP_NAME)_STATIC.dcp $(ROLE_DIR_2)/*.dcp $(cFpDcpDir)/0_$(TOP_NAME)_static_without_role.dcp 
	@/bin/echo "-impl2 -pr  \\" >> .vivado_basecmd.sh
	rm -f $@ 

$(cFpDcpDir)/4_$(TOP_NAME)_impl_grey_box.bit: $(cFpDcpDir)/3_$(TOP_NAME)_static_with_grey_box.dcp
	@/bin/echo "-bitgenGrey  -pr -pr_verify \\" >> .vivado_basecmd.sh
	rm -f $@

$(cFpDcpDir)/3_$(TOP_NAME)_static_with_grey_box.dcp: $(cFpDcpDir)/3_$(TOP_NAME)_STATIC.dcp
	@/bin/echo "-implGrey  -pr \\" >> .vivado_basecmd.sh
	rm -f $@

