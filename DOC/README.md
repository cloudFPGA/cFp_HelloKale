## Bring-Up ROLE

### Preliminary
In cloudFPGA (cF), the user application is referred to as a **_ROLE_** and is integrated along
with a **_SHELL_** that abstracts the hardware components of the FPGA module.

### Overview
The current role implements a set of functions that were developed as IP cores to support the 
test and the bring-up of the TCP, UDP and DDR4 components of the cF module FMKU2595 when equipped 
with a Xilinx FPGA XCKU060. 
This role is typically paired with the shell [_**Kale**_](https://github.com/cloudFPGA/cFDK/tree/master/DOC/Kale.md). 

A block diagram of the ROLE is depicted in Figure 1. It features:
- a _UDP Shell Interface_ (**USIF**) core that handles the control flow interface between the 
 SHELL and the ROLE. The main purpose of the USIF is to provide a placeholder for the opening of 
 one or multiple port(s).
- a _UDP Application Flash_ (**UAF**) core that implements a set of UDP-oriented tests.
- a _TCP Shell Interface_ (**TSIF**) core that handles the control flow interface between the 
 SHELL and the ROLE. The main purpose of the TSIF is to open port(s) for listening and for 
 connecting to remote host(s).
 - a _TCP Application Flash_ (**TAF**) core that implements a set of TCP-oriented tests.
 - a _Memory Test Application_ (**MTA**) core that implements a memory test for the DDR4.
 
 
![Block diagram of Bring-up Role](./imgs/Fig-ROLE-Structure.png#center)
<p align="center"><b>Figure-1: Block diagram of Bring-up Role</b></p>
<br>

### List of Interfaces

| Acronym                             | Description                        | File
|:------------------------------------|:-----------------------------------|:--------------
| **[SHELL](https://github.com/cloudFPGA/cFDK/tree/master/DOC/Kale.md)** | Interface to shell _Kale_.    | [Shell](https://github.com/cloudFPGA/cFDK/blob/main/SRA/LIB/SHELL/Kale/Shell.v)
           
<br>

### List of HLS Components

| Acronym                     | Description                | File
|:----------------------------|:---------------------------|:--------------
| **[TSIF](./TSIF.md)**       | TCP Shell InterFace        | [tcp_shell_if](../ROLE/hls/tcp_shell_if/src/tcp_shell_if.hpp)
| **[USIF](./USIF.md)**       | UDP Shell InterFace        | [udp_shell_if](../ROLE/hls/udp_shell_if/src/udp_shell_if.hpp)
| **[TAF](./TAF.md)**         | TCP Application Flash      | [tcp_app_flash](../ROLE/hls/tcp_app_flash/src/tcp_app_flash.hpp)
| **[UAF](./UAF.md)**         | UDP Application Flash      | [udp_app_flash](../ROLE/hls/udp_app_flash/src/udp_app_flash.hpp)
| **[MTA](./MTA.md)**         | Memory Test Application    | [FIXME-TODO](../ROLE/hls/mem_test_app/src/mem_test_app.hpp)
<br>




