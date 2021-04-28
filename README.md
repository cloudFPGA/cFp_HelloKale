## cFp_Monolithic

**A cloudFPGA project (cFp) to generate a static bitstream (.i.e without Partial Reconfiguration). 
Such a project is typically used for the bring-up of a new FPGA module or a static implementation of an application.**

**Warning**: the static nature of the bitstream precludes its deployment over the datacenter network and requires the use of a JTAG interface to download and configure the FPGA.    

### Toplevel design
In cloudFPGA (cF), a user application is referred to as a **_ROLE_** and is integrated along
with a **_SHELL_** that abstracts the hardware components of the FPGA module. 
The combination of a specific ROLE and its associated SHELL into a toplevel design is
referred to as a _Shell-Role-Architecture (SRA)_. 

This section describes the toplevel design (**TOP**) of the _cFp_Monolithic_ project which was developed for the bring-up and the testing of the FMKU2595 module when it is equipped with a XCKU060. 
As shown in the figure below, the toplevel of _cFp_Monolithic_ consists of:
  - a SHELL of type [_**Kale**_](https://github.ibm.com/cloudFPGA/cFDK/blob/master/DOC/Kale.md) which is a shell with minimalist support for accessing the hardware components of the FPGA card. 
  - a ROLE of type [_**BringUp**_](./DOC/BringUpRole.md) which implements a set of TCP-, UDP- and DDR4-oriented tests and functions for the bring-up the cloudFPGA module.


![Block diagram of the BringUpTop](./DOC/imgs/Fig-TOP-BringUp.png#center)
<p align="center"><b>Toplevel block diagram of the cfP_BringUp project</b></p>
<br>


### How to build the project 

The current directory contains a _Makefile_ which handles all the required steps to generate a _bitfile_. 

During the build, both SHELL and ROLE dependencies are analyzed to solely re-compile and and re-synthesize
the components that must be recreated.
 
Finally, to further reduce the place-and-route implementation time, an _incremental compile_ options cans be specified.

##### Step-1: Clone and Configure the cFp_Monolithic project
```
$ git clone git@github.ibm.com:cloudFPGA/cFp_Monolithic.git
$ git submodule init
$ git submodule update
$ cd cFDK
$ git checkout master
$ cd ..
```
##### Step-2: Setup your environment
```
$ source env/setenv.sh
```
##### Step-3: Generate a static bitfile 
```
$ make monolithic
```
##### Step-4: Save a checkpoint (optional)
If the design was successfully implemented, you may want to save its corresponding checkpoint in the '_./dcps_' directory
to later exploit incremental place-and-route in Vivado (see step-3).
```
$ make save_mono_incr
``` 
##### Step-5: Generate an incremental implementation (optional)
```
$ make monolithic_incr
```

Note: If ```make monolithic_incr``` gets invoked in the absence of a saved checkpoint, the build will execute the default
default monolithic flow of step-1. 

### How to test the features of the static bitfile

TODO - Refer to section [_**HOST**_](./HOST/README.md) of the _cFp_Monolithic_ project.
