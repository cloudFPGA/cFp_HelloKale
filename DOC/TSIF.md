### TCP Shell Interface (TSIF)
This page describes the TCP front-end interface process to the _**Network Transport Stack (NTS)**_ 
of the SHELL used by the _cloudFPGA_ platform. 

#### Overview
The _**TCP Shell Interface (TSIF)**_ handles the control flow interface between the SHELL and 
the ROLE. The main purpose of the TSIF is to open port(s) for listening and for connecting to 
remote host(s). Its use is not a a prerequisite, but it is provided here for sake of clarity and 
simplicity.

A block diagram of the _TCP Shell Interface_ is depicted in Figure 1. It features:
- a _**Listen (LSn)**_ process to request the SHELL/NTS/TOE to start listening for incoming 
connections on a specific port (.i.e open connection for reception mode).
- a _**Connect (COn)**_ process to open a connection to a remote HOST or FPGA socket.
- a _**Read Request Handler (RRh)**_ that handles the notifications indicating availability 
of new data for the application layer, and accepts (or reject) them.
- a _**Read Path (RDp)**_ process that waits for new data segments from SHELL/NTS/TOE and forwards
them to the _**TCP Application Flash (TAF)**_ core.
- a _**Write Path (WRp)**_ process that waits for new data segments from the ROLE/TAF and forwards 
them to SHELL/NTS/TOE.


![Block diagram of cFp_BringUp/ROLE/TSIF](./imgs/Fig-TSIF-Structure.png#center)

<p align="center"><b>Figure-1: Block diagram of the TCP Shell Interface</b></p>
<br>

#### List of Interfaces

| Acronym                             | Description                                | File
|:------------------------------------|:-------------------------------------------|:--------------
| **[SHELL](../cFDK/DOC/Kale.md)**    | TCP application interface to shell _Kale_. | [Shell](../cFDK/SRA/LIB/SHELL/Kale/Shell.v)
| **[TAF](./TAF.md)**                 | TCP Application Flash                      | [tcp_app_flash](../ROLE/hls/tcp_app_flash/src/tcp_app_flash.hpp)

<br>

#### List of HLS Components

| Acronym         | Description                    | Filename
|:----------------|:-------------------------------|:--------------
| **COn**         | Connect process                | [tcp_shell_if](../ROLE/hls/tcp_shell_if/src/tcp_shell_if.cpp)
| **LSn**         | Listen process                 | [tcp_shell_if](../ROLE/hls/tcp_shell_if/src/tcp_shell_if.cpp)
| **RDp**         | Read Path process              | [tcp_shell_if](../ROLE/hls/tcp_shell_if/src/tcp_shell_if.cpp)
| **RRh**         | Read Request Handler process   | [tcp_shell_if](../ROLE/hls/tcp_shell_if/src/tcp_shell_if.cpp)
| **WRp**         | Write Path process             | [tcp_shell_if](../ROLE/hls/tcp_shell_if/src/tcp_shell_if.cpp)

<br>
