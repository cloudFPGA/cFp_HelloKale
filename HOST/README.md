cFp_BringUp / HOST
===================
**Host code for the current cloudFPGA project**

## ABOUT
This directory contains the CPU software counter part of the HLS application built into the role
of the _cFp_BringUp_ project.

The _cFp_BringUp_ project was developed for the bring-up and the testing of the _FMKU2595_ module 
when it is equipped with a Xilinx XCKU060 FPGA. As such, the role of _cFp_BringUp_ implements a 
set of TCP-, UDP- and DDR4-oriented tests and functions that can be called or exercised from a
remote host. 

## Toplevel design


## How To Python 
The **_py_** directory contains a set of python3 scripts for interacting with the role of the 
_cFp_BringUp_ project. 

##### Step-0: Do you have a valid bitfile?
If not, refer to section **_How To Build the project_** in file [README.md](../README.md) and  
generate a new bitfile for the current cloudFPGA project.

##### Step-1: Request and program an FPGA instance
Follow the procedure described [TBD](TODO) and write down your _**'image_ip'**_ address and 
your _**'instance_id'**.
 
##### Step-2a: Ping your FPGA instance
It is good practise to ping your FPGA instance before trying to run any further test.
```
    $ ping <image_ip>        (.e.g ping 10.12.200.21) 
````
##### Step-2b: Test the establishment of a TCP connection with Netcat
```
    $ nc <image_ip> 8803     (.e.g nc 10.12.200.163 8803)
```
##### Step-2c: Test the establishment of a UDP connection with Netcat
```
    $ nc -u <image_ip> 8803     (.e.g nc -u 10.12.200.163 8803)
```

##### Step-3a: Execute a TCP echo test
```
    $ cd py
    $ source venv/bin/activate
    $ python3 tc_TcpEcho.py --help
    $ python3 tc_TcpEcho.py --fpga_ipv4 <11.22.33.44> --inst_id <42> --user_name <xyz> --user_passwd <xyz's_pw>
```

##### Step-3b: Execute a UDP echo test
```
    $ cd py
    $ source venv/bin/activate
    $ python3 tc_UdpEcho.py --help
    $ python3 tc_UdpEcho.py --fpga_ipv4 <11.22.33.44> --inst_id <42> --user_name <xyz> --user_passwd <xyz's_pw> --loop_count 250 -v
 
```

##### Step-3b: Execute a TCP transmit test
The _**iperf**_ network performance measurement tool can be used in client mode to create data streams between the host and the port **8800** of the FPGA instance. 
```
    $ iperf --help
    $ iperf -c <11.22.33.44> -p 8800
    ------------------------------------------------------------
    Client connecting to 10.12.200.11, TCP port 8800
    TCP window size:  117 KByte (default)
    ------------------------------------------------------------
    [  3] local 10.2.0.10 port 52074 connected with 10.12.200.11 port 8800
    [ ID] Interval       Transfer     Bandwidth
    [  3]  0.0-30.0 sec  1.20 GBytes   345 Mbits/sec
```


##### Step-3c: Execute a TCP receive test

##### Step-4a: Execute a TCP echo test

##### Step-4b: Execute a TCP transmit test

##### Step-4c: Execute a TCP receive test





