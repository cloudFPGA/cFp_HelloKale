# cFp_BringUp / Role


## Overview
In cloudFPGA (cF), the user application is referred to as a **_ROLE_** and is integrated along
with a **_ROLE_** that abstracts the hardware components of the FPGA module. 

This document describes the design of the role which was developped for the bring-up and the
testing of the cF module FMKU2595 when equipped with a XCKU060. 

Figure 1 shows a block diagram of the role. It implements the following blocks as IP cores:
  - [TODO] 
  - [TODO] a dual 8GB DDR4 memory subsystem (MEM) as described in PG150,
  - [TODO] one network, transport and session (NTS) core based on the TCP/IP protocol,

![Block diagram of the role](./imgs/Fig-ROLE.png#center)
<p align="center"><b>Figure-1: Block diagram of the role</b></p>
<br>

## HDL Coding Style and Naming Conventions
Please consider reading the [**HDL Naming Conventions**](./hdl-naming-conventions.md) document if you intend to deploy this shell or you want to contribute to this part of the cloudFPGA project.
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **CLKT**        | CLocK Tree interface                                  | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **DDR4**        | Double Data Rate 4 memory interface                   | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **ECON**        | Edge CONnnector interface                             | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **PSOC**        | Programmable System-On-Chip interface                 | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **ROLE**        | Test and bring-up application interface               | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
<br>

## List of HDL Components

| Acronym                                          | Description                | Filename
|:-------------------------------------------------|:---------------------------|:--------------
| **[ETH](../SRA/LIB/SHELL/LIB/hdl/eth/ETH.md)**   | 10G ETHernet subsystem     | [tenGigEth.v](../SRA/LIB/SHELL/LIB/hdl/eth/tenGigEth.v)
| **[MEM](../SRA/LIB/SHELL/LIB/hdl/eth/MEM.md)**   | DDR4 MEMory subsystem      | [memSubSys.v](../SRA/LIB/SHELL/LIB/hdl/mem/memSubSys.v)
| **[MMIO](../SRA/LIB/SHELL/LIB/hdl/mmio/MMIO.md)**| Memory Mapped IOs          | [mmioClient_A8_D8.v](../SRA/LIB/SHELL/LIB/hdl/mmio/mmioClient_A8_D8.v)
| **[NTS](../SRA/LIB/SHELL/LIB/hdl/nts/NTS.md)**   | Network Transport Stack    | [nts_TcpIp.v](../SRA/LIB/SHELL/LIB/hdl/nts/nts_TcpIp.v)
<br>

---
**Trivia**: The [moon Kale](https://en.wikipedia.org/wiki/Kale_(moon))


