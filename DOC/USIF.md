### UDP Shell Interface (USIF)

This page describes the UDP front-end process of the _cFp_Monolithic/_[_Role_BrinUp_](./BringUpRole.md)
 to the _cFp_Monolithic/_[_Shell_Kale_](https://github.ibm.com/cloudFPGA/cFDK/blob/master/DOC/Kale.md). 

#### Overview
The _**UDP Shell Interface (USIF)**_ handles the control flow interface between the SHELL and 
the ROLE. The main purpose of the USIF is to provide a placeholder for the opening of one or 
multiple listening port(s). The use of such a dedicated layer is not a prerequisite but it is 
provided here for sake of clarity and simplicity.

A block diagram of the _UDP Shell Interface_ is depicted in Figure 1. It features:
- a _**Listen (LSn)**_ process to request the SHELL/NTS/UOE to start listening for incoming 
connections on a specific port. Although the notion of 'listening' does not exist for the 
unconnected UDP mode, we keep that name for this process because it puts an FPGA receive
port on hold and ready to accept incoming traffic (.i.e, it opens a connection in server mode).
- a _**Close (CLs)**_ process to request the SHELL/NTS/UOE to close a previously opened port.
- a _**Read Path (RDp)**_ process that waits for new data segments from SHELL/NTS/UOE and forwards
them to the _**UDP Application Flash (UAF)**_ core.
- a _**Write Path (WRp)**_ process that waits for new data segments from the ROLE/UAF and forwards 
them to SHELL/NTS/UOE.


![Block diagram of cFp_Monolithic/ROLE/USIF](./imgs/Fig-USIF-Structure.png#center)

<p align="center"><b>Figure-1: Block diagram of the UDP Shell Interface</b></p>
<br>

#### List of Interfaces

| Acronym                             | Description                                | File
|:------------------------------------|:-------------------------------------------|:--------------
| **[SHELL](https://github.ibm.com/cloudFPGA/cFDK/blob/master/DOC/Kale.md)** | UDP application interface to shell _Kale_. | [Shell](https://github.ibm.com/cloudFPGA/cFDK/tree/master/SRA/LIB/SHELL/Kale/Shell.v)
| **[UAF](./UAF.md)**                 | UDP Application Flash interface            | [udp_app_flash](../ROLE/hls/udp_app_flash/src/udp_app_flash.hpp)

<br>

#### List of HLS Components

| Acronym         | Description                    | Filename
|:----------------|:-------------------------------|:--------------
| **CLs**         | Close process                  | [udp_shell_if](../ROLE/hls/udp_shell_if/src/udp_shell_if.cpp)
| **LSn**         | Listen process                 | [udp_shell_if](../ROLE/hls/udp_shell_if/src/udp_shell_if.cpp)
| **RDp**         | Read Path process              | [udp_shell_if](../ROLE/hls/udp_shell_if/src/udp_shell_if.cpp)
| **WRp**         | Write Path process             | [udp_shell_if](../ROLE/hls/udp_shell_if/src/udp_shell_if.cpp)

<br>
