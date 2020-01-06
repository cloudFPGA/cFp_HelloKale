cFp_BringUp / HOST
===================
**Host code for the current cloudFPGA project**

## ABOUT
This directory contains the CPU software counter part of the HLS application built into the role
of the _cFp_BringUp_ project.

The _cFp_BringUp_ project was developed for the bring-up and the testing of the _FMKU2595_ module 
when it is equipped with a Xilinx XCKU060 FPGA. As such, the role of _cFp_BringUp_ implements a 
set of TCP-, UDP- and DDR4-oriented tests and functions that can be called or exercised from a
remote host. 

## Toplevel design


## How To Python 
The _py_ directory contains a set of python3 scripts for interacting with the role of the 
_cFp_BringUp_ project. 

##### Step-0: Do you have a valid bitfile?
If not, refer to section **_How To Build the project_** in file [README.md](../README.md) and  
generate a new bitfile for the current cloudFPGA project.

##### Step-1: Request and program an FPGA instance
Follow the procedure described [TBD](TODO) and write down your _**'image_ip'**_ address and 
your _**'instance_id'**.
 
##### Step-2: Ping your FPGA instance
It is good practise to ping your FPGA instance before trying to run any further test.
```
ping <image_ip>        (.e.g., ping 10.12.200.21) 
````

##### Step-3a: Execute a TCP echo test
##### Step-3b: Execute a TCP transmit test
##### Step-3c: Execute a TCP receive test

##### Step-4a: Execute a TCP echo test
##### Step-4b: Execute a TCP transmit test
##### Step-4c: Execute a TCP receive test





