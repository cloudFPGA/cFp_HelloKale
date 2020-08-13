### UDP Application Flash (UAF)

This page describes the set of UDP-oriented tests which are embedded into the 
_cFp_Monolithic/_[_Role_BringUp_](../BringUpRole.md).

#### Overview
The _**UDP Application Flash (UAF)**_ connects to _cFp_Monolithic/_[_Shell_Kale_](../../cFDK/DOC/Kale.md)
via the _UDP Shell Interface ([USIF](./USIF.md))_ block. 
The main purpose of the USIF is to provide a placeholder for the control flow related functions. 
Its use is not a prerequisite, but it is provided here with the aim of decoupling the data-path from
the control flow.

A block diagram of the _UDP Application Flash_ is depicted in Figure 1. It features:
- a _**Receive Path (RXp)**_ process that waits for new data segments from the shell and
forwards them to the process _EchoPathThrough (EPt)_ or the _EchoStoreAndForward (ESf)_ process
 upon the setting of the UDP destination port.
 - a _**EchoStoreAndForward (ESf)**_ process that loops the incoming traffic back to the producer.
 The echo is said to operate in "store-and-forward" mode because every received datagram is
 stored into the DDR4 memory before being read again from the DDR4 and sent back.
 - a _**EchoPathThrough (EPt)**_ process that loops the incoming traffic back to the producer in
 cut-through-mode. 
 - a _**Write Path (WRp)**_ process that waits for new data segments from either the *ESf* or 
the *EPt* and forwards them to the *USIF*.


![Block diagram of cFp_Monolithic/ROLE/UAF](./imgs/Fig-UAF-Structure.png#center)

<p align="center"><b>Figure-1: Block diagram of the UDP Application Flash</b></p>
<br>

#### List of Interfaces

| Acronym                     | Description                   | File
|:----------------------------|:------------------------------|:--------------
| **[USIF](./USIF.md)**       | UDP Shell Interface           | [udp_shell_if](../hls/udp_shell_if/src/udp_shell_if.cpp)

<br>

#### List of HLS Components

| Acronym         | Description                    | Filename
|:----------------|:-------------------------------|:--------------
| **EPt**         | Echo Path Through process      | [udp_app_flash](../hls/udp_app_flash/src/udp_app_flash.cpp)
| **ESf**         | Echo Store And Forward process | [udp_app_flash](../hls/udp_app_flash/src/udp_app_flash.cpp)
| **RXp**         | Receive Path process           | [udp_app_flash](../hls/udp_app_flash/src/udp_app_flash.cpp)
| **TXp**         | Transmit Path process          | [udp_app_flash](../hls/udp_app_flash/src/udp_app_flash.cpp)

<br>

 

































[TODO] This process transmits the data streams of established connections. Before transmitting, it is also used to create a new active connection. 
Active connections are the ones opened by the FPGA as client and they make use of dynamically assigned or ephemeral ports in the range 32,768 to
65,535. A block diagram of the **TAi** is depicted in Figure 1.

The basic handshake between the [TAi] and the application is as follows:
- After data and metadata are received from [TRIF], the state of the connection is checked and the segment memory pointer is loaded into
  the APP pointer table.
- Next, data are written into the DDR4 buffer memory and the application is notified about the successful or failing 
write to the buffer memory.
 
![Block diagram of the TOE/TAi](./images/Fig-TOE-TAi-Structure.bmp#center)
<p align="center"><b>Figure-1: Block diagram of the Tx Application Interface</b></p>
<br>

#### List of Interfaces

| Acronym                    | Description                             | Filename
|:---------------------------|:----------------------------------------|:--------------
|  **[EVe](./EVe.md)**       | Event Engine interface                  | [event_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/event_engine/event_engine.cpp)
|  **MEM**                   | MEMory sub-system (data-mover to DDR4)  | [memSubSys](../../SRA/LIB/SHELL/LIB/hdl/mem/memSubSys.v)
|  **[PRt](./PRt.md)**       | PoRt table                              | [port_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/port_table/port_table.cpp)
|  **[RXe](./RXe.md)**       | RX engine                               | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine/src/rx_engine.cpp)
|  **[SLc](./SLc.md)**       | Session Lookup Controller interface     | [session_lookup_controller](../../SRA/LIB/SHELL/LIB/hls/toe/src/session_lookup_controller/session_lookup_controller.cpp)
|  **[STt](./STt.md)**       | State Table interface                   | [state_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/state_table/state_table.cpp)  
|  **[TSt](./TSt.md)**       | Tx Sar table                            | [tx_sar_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_sar_table/tx_sar_table.cpp)
|  **TRIF**                  | Tcp Role InterFace (alias APP)          | 

<br>

#### List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **Emx**         | Event multiplexer processs                            | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
| **Mwr**         | Memory writer processs                                | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
| **Slg**         | Stream length generator processs                      | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
| **Sml**         | Stream metadata loader processs                       | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
| **Taa**         | Tx application accept processs                        | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
| **Tash**        | Tx application status handler process                 | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
| **Tat**         | Tx application table process                          | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)

<br>
