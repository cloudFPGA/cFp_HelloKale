cFp_BringUp
================
**cloudFPGA project (cFp) for the BringUp of an FPGA module**


## ABOUT
In cloudFPGA (cF), a user application is referred to as a **_ROLE_** and is integrated along
with a **_SHELL_** that abstracts the hardware components of the FPGA module. 
The combination of a specific ROLE and its associated SHELL into a toplevel design is
referred to as a _Shell-Role-Architecture (SRA)_. 

## Toplevel design
This section describes the toplevel design (**TOP**) of the _cFp_BringUp_ project which was developed for the bring-up and the testing of the FMKU2595 module when it is equipped with a XCKU060. 
As shown in Figure 1, the toplevel of _cFp_BringUp_ consists of:
  - a SHELL of type [_**Kale**_](https://github.ibm.com/cloudFPGA/cFDK/blob/master/DOC/Kale.md)
  - a ROLE of type [_**BringUp**_](https://github.ibm.com/cloudFPGA/cFp_BringUp/blob/master/ROLE/doc/BringUpRole.md) which implements a set of TCP-, UDP- and DDR4-oriented tests and functions for the bring-up the cloudFPGA module.
  

![Block diagram of the BringUpTop](https://github.ibm.com/cloudFPGA/cFp_BringUp/blob/master/ROLE/doc/imgs/Fig-TOP-BringUp.png#center)
<p align="center"><b>Figure-1: Toplevel block diagram of the cfP_BringUp project</b></p>
<br>


## How To Build the project 

The current directory contains a _Makefile_ which handles all the required steps to generate a _bitfile_. 

During the build, both SHELL and ROLE dependencies are analyzed to solely re-compile and and re-synthesize
the components that must be recreated.
 
Finally, to further reduce the place-and-route implementation time, an _incremental compile_ options cans be specified.

##### Step-1: Generate a bitfile 
```
make monolithic
```
##### Step-2: Save a checkpoint (optional)

If the design was successfully implemented, you may want to save its corresponding checkpoint in the '_./dcps_' directory
to later exploit incremental place-and-route in Vivado (see step-3).
```
make save_mono_incr
``` 
##### Step-3: Generate an incremental implementation (optional)

```
make monolithic_incr
```

Note: If ```make monolithic_incr``` gets invoked in the absence of a saved checkpoint, the build will execute the default
default monolithic flow of step-1. 




