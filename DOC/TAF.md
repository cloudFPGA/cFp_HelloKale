### TCP Application Flash (TAF)

This page describes the set of TCP-oriented tests which are embedded into the 
_cFp_HelloKale/_[_Role_BringUp_](../BringUpRole.md).

#### Overview
The **TCP Application Flash (TAF)** connects to _cFp_HelloKale/_[_Shell_Kale_](https://github.com/cloudFPGA/cFDK/blob/main/DOC/Kale.md)
via the _TCP Shell Interface ([TSIF](./TSIF.md))_ block. 
The main purpose of the TSIF is to provide a placeholder for the control flow related functions. 
Its use is not a prerequisite, but it is provided here with the aim of decoupling the data-path from
the control flow.

A block diagram of the _TCP Application Flash_ is depicted in Figure 1. It features:
- a **Receive Path (RXp)** process that waits for new data segments from the shell and
forwards them to the process _EchoPathThrough (EPt)_ or the _EchoStoreAndForward (ESf)_ process
 upon the setting of the TCP destination port.
 - a **EchoStoreAndForward (ESf)** process that loops the incoming traffic back to the producer.
 The echo is said to operate in "store-and-forward" mode because every received datagram is
 stored into the DDR4 memory before being read again from the DDR4 and sent back.
 - a **EchoPathThrough (EPt)** process that loops the incoming traffic back to the producer in
 cut-through-mode. 
 - a **Write Path (WRp)** process that waits for new data segments from either the *ESf* or 
the *EPt* and forwards them to the *TSIF*.


![Block diagram of cFp_HelloKale/ROLE/TAF](./imgs/Fig-TAF-Structure.png#center)

<p align="center"><b>Figure-1: Block diagram of the TCP Application Flash</b></p>
<br>

#### List of Interfaces

| Acronym                     | Description                   | File
|:----------------------------|:------------------------------|:--------------
| **[TAF](./TAF.md)**         | TCP Application Flash         | [tcp_app_flash](../ROLE/hls/tcp_app_flash/src/tcp_app_flash.cpp)

<br>

#### List of HLS Components

| Acronym         | Description                    | Filename
|:----------------|:-------------------------------|:--------------
| **EPt**         | Echo Path Through process      | [tcp_app_flash](../ROLE/hls/tcp_app_flash/src/tcp_app_flash.cpp)
| **ESf**         | Echo Store And Forward process | [tcp_app_flash](../ROLE/hls/tcp_app_flash/src/tcp_app_flash.cpp)
| **RXp**         | Receive Path process           | [tcp_app_flash](../ROLE/hls/tcp_app_flash/src/tcp_app_flash.cpp)
| **TXp**         | Transmit Path process          | [tcp_app_flash](../ROLE/hls/tcp_app_flash/src/tcp_app_flash.cpp)

<br>
