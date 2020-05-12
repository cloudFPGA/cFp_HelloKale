#!/bin/bash

#vivado -mode batch -source Sim/top.handle_vivado.tcl -notrace -log handle_vivado.synth.log -tclargs -full_src -force -create -synth

if [[  $# -gt 0 ]]; then
  echo "<$0> The RTLSim is passed as an argument:"
  echo "<$0> Given mode = '$1' "
fi

if [[ $1 -eq 'MacLoopback' ]]; then
  echo "<$0> Testing MacLoopback "
fi
if [[ $1 -eq 'ARPping' ]]; then
  echo "<$0> Testing ARPping "
fi

if [[ $1 -eq 'ICMPping' ]]; then
  echo "<$0> Testing ICMPping"
fi

if [[ $1 -eq 'UDPLoopback' ]]; then
  echo "<$0> Testing UDPLoopback "
fi

if [[ $1 -eq 'TCPLoopback' ]]; then
  echo "<$0> Testing TCPLoopback "
fi

echo "vivado -mode batch -source Sim/top.handle_vivado.tcl -notrace -log Sim/handle_vivado.sim.log -tclargs -RTLsim -feature $1 "
vivado -mode batch -source Sim/top.handle_vivado.tcl -notrace -log Sim/handle_vivado.sim.log -tclargs -RTLsim -feature $1 
#vivado -mode batch -source Sim/handle_vivado.tcl -notrace -log Sim/handle_vivado.log
