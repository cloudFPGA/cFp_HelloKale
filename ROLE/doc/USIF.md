### UDP Shell Interface (USIF)
This page describes the UDP front-end interface process to the _**Network Transport Stack (NTS)**_ 
of the SHELL used by the _cloudFPGA_ platform. 

#### Overview
The _**UDP Shell Interface (USIF)**_ handles the control flow interface between the SHELL and 
the ROLE. The main purpose of the USIF is to provide a placeholder for the opening of one or 
multiple listening port(s). The use of such a dedicated layer is not a prerequisite but it is 
provided here for sake of clarity and simplicity.

A block diagram of the _UDP Shell Interface_ is depicted in Figure 1. It features:
- a _**Listen (LSn)**_ process to request the SHELL/NTS/UOE to start listening for incoming 
connections on a specific port (.i.e open connection for reception mode).
- a _**Close (CLs)**_ process to request the SHELL/NTS/UOE to close a previously opened port.
- a _**Read Path (RDp)**_ process that waits for new data segments from SHELL/NTS/UOE and forwards
them to the _**UDP Application Flash (UAF)**_ core.
- a _**Write Path (WRp)**_ process that waits for new data segments from the ROLE/UAF and forwards 
them to SHELL/NTS/UOE.


![Block diagram of cFp_BringUp/ROLE/USIF](./imgs/Fig-USIF-Structure.png#center)

<p align="center"><b>Figure-1: Block diagram of the UDP Shell Interface</b></p>
<br>

#### List of Interfaces

| Acronym                             | Description                                | File
|:------------------------------------|:-------------------------------------------|:--------------
| **[SHELL](../../cFDK/DOC/Kale.md)** | UDP application interface to shell _Kale_  | [Shell](../../cFDK/SRA/LIB/SHELL/Kale/Shell.v)
| **[UAF](./UAF.md)**                 | UDP Application Flash interface            | [udp_app_flash](../hls/udp_app_flash/src/udp_app_flash.hpp)

<br>

#### List of HLS Components

| Acronym         | Description                    | Filename
|:----------------|:-------------------------------|:--------------
| **CLs**         | Close process                  | [udp_shell_if](../hls/udp_shell_if/src/udp_shell_if.cpp)
| **LSn**         | Listen process                 | [udp_shell_if](../hls/udp_shell_if/src/udp_shell_if.cpp)
| **RDp**         | Read Path process              | [udp_shell_if](../hls/udp_shell_if/src/udp_shell_if.cpp)
| **WRp**         | Write Path process             | [udp_shell_if](../hls/udp_shell_if/src/udp_shell_if.cpp)

<br>
