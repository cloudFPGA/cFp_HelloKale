#vivado -mode batch -source Sim/top.handle_vivado.tcl -notrace -log handle_vivado.synth.log -tclargs -full_src -force -create -synth
$PWD
vivado -mode batch -source Sim/top.handle_vivado.tcl -notrace -log handle_vivado.sim.log -tclargs -full_src -force -RTLsim
#vivado -mode batch -source Sim/handle_vivado.tcl -notrace -log Sim/handle_vivado.log
