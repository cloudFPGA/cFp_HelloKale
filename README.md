# cFp_HelloKale

A cloudFPGA project (cFp) built upon the shell _Kale_. 

## Toplevel design
In cloudFPGA (cF), a user application is referred to as a **_ROLE_** and is integrated 
along with a **_SHELL_** that abstracts the hardware components of the FPGA module. 
The combination of a specific ROLE and its associated SHELL into a toplevel design is
referred to as a _Shell-Role-Architecture (SRA)_. 

This section describes the toplevel design (**TOP**) of the _cFp_HelloKale_ project which 
was developed for the bring-up and the testing of the FMKU2595 module when it is equipped 
with a XCKU060. 
As shown in the figure below, the toplevel of _cFp_HelloKale_ consists of:
  - a SHELL of type [_**Kale**_](https://github.com/cloudFPGA/cFDK/blob/master/DOC/Kale.md) 
  which is a shell with minimalist support for accessing the hardware components of the 
  FPGA card. 
  - a ROLE of type [_**BringUp**_](./DOC/BringUpRole.md) which implements a set of TCP-, 
  UDP- and DDR4-oriented tests and functions for the bring-up the cloudFPGA module.

![Block diagram of the BringUpTop](./DOC/imgs/Fig-TOP-BringUp.png#center)
<p align="center"><b>Toplevel block diagram of the cFp_HelloKale project</b></p>
<br>

**Info/Warning**
  - The shell [_**Kale**_](https://github.com/cloudFPGA/cFDK/blob/master/DOC/Kale.md) does 
  not support Partial Reconfiguration (PR). 
  It was specifically developed for the bring-up of a new FPGA module or for the deployment 
  of a static implementation. As a result, the generated bitstream is always a static 
  bitstream. 
  Refer to the project [_cFp_HelloThemisto_](https://github.com/cloudFPGA/cFp_HelloThemisto) 
  for an example of shell that supports PR.
  - The static nature of the generated bitstream precludes its deployment over the 
  datacenter network and requires the use of a JTAG interface to download and configure 
  the FPGA.

## How to build the project

The current directory contains a _Makefile_ which handles all the required steps to generate 
a _bitfile_ (a.k.a bitstream). 

During the build, both SHELL and ROLE dependencies are analyzed to solely re-compile and 
re-synthesize the components that must be recreated.
 
Finally, to further reduce the place-and-route implementation time, an _incremental compile_ 
option can be specified.

#### Step-1: Clone and Configure the project
```
$ git clone git@github.com:cloudFPGA/cFp_HelloKale.git
$ cd cFp_HelloKale
$ git submodule init
$ git submodule update
```
#### Step-2: Setup your environment
```
$ source env/setenv.sh
```
#### Step-3: Generate a static bitstream 
```
$ make monolithic
```
##### Step-3.1: Save a checkpoint (optional)
If the design was successfully implemented, you can opt to save its corresponding 
checkpoint in the '_./dcps_' directory. This will accelerate the next build by exploiting 
the incremental place-and-route features of Vivado (see step-3.2).
```
$ make save_mono_incr
``` 
##### Step-3.2: Generate an incremental implementation (optional)
```
$ make monolithic_incr
```
Note: If ```make monolithic_incr``` gets invoked in the absence of a saved checkpoint, 
the build will execute the default default monolithic flow of step-3. 

## How to deploy a cloudFPGA instance

#### Step-4: Upload the generated bitstream
In order to program a cloudFPGA instance with your newly generated bitfile, you first 
need to upload it to the cloudFPGA Resource Manager (cFRM).
[ TODO - How-to-upload-a-bitfile ]
 
#### Step-5: Request a cloudFPGA instance and deploy it
Request a cloudFPGA instance to be programmed and deployed with your previously uploaded 
bitfile.
[ TODO - How-to-request-an-instance ]

#### Step-6: Ping the deployed FPGA
It is good practise to ping your deployed FPGA to assess its availability.

Use the following command to ping the FPGA with its IP address (see Step-5).
```
$ ping <instance_ip>        (e.g. ping 10.12.200.21)
```

## How to test the features of the cFp_HelloKale
As mentioned above, the role of the project _cFp_HelloKale_ implements a set of TCP-, UDP- 
and DDR4-oriented tests and functions. These features can be called or exercised from a 
remote host as explained in section [_**HOST**_](./HOST/README.md).
