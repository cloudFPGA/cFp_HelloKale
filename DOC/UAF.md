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
| **[USIF](./USIF.md)**       | UDP Shell Interface           | [udp_shell_if](../ROLE/hls/udp_shell_if/src/udp_shell_if.cpp)

<br>

#### List of HLS Components

| Acronym         | Description                    | Filename
|:----------------|:-------------------------------|:--------------
| **EPt**         | Echo Path Through process      | [udp_app_flash](../ROLE/hls/udp_app_flash/src/udp_app_flash.cpp)
| **ESf**         | Echo Store And Forward process | [udp_app_flash](../ROLE/hls/udp_app_flash/src/udp_app_flash.cpp)
| **RXp**         | Receive Path process           | [udp_app_flash](../ROLE/hls/udp_app_flash/src/udp_app_flash.cpp)
| **TXp**         | Transmit Path process          | [udp_app_flash](../ROLE/hls/udp_app_flash/src/udp_app_flash.cpp)

<br>
