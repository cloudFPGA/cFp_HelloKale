cFp_HelloKale / HOST
===================
**Host code for the current cloudFPGA project**

## ABOUT
This directory contains the CPU software counter part of the HLS application built into 
the role of the _cFp_HelloKale_ project.

The _cFp_HelloKale project was developed for the bring-up and the testing of the _FMKU2595_ 
module when it is equipped with a Xilinx XCKU060 FPGA. As such, the role of _cFp_HelloKale_ 
implements a set of TCP-, UDP- and DDR4-oriented tests and functions that can be called or 
exercised from a remote host. 

## How To Ping
It is good practise to ping your FPGA instance before trying to run any further test.

##### Step-1: Do you have a valid bitfile?
If not, refer to section [How to build the project](../README.md#how-to-build-the-project) 
in this [file](../README.md) and generate a new bitfile for the current cloudFPGA project.

##### Step-2: Request and program an FPGA instance
Follow the procedure described in the section 
[How to deploy a cloudFPGA instance](../README.md#how-to-deploy-a-cloudfpga-instance) and 
write down your _'instance_ip'_ address and your _'instance_id'_.

##### Step-3: Ping your FPGA instance
You can `ping` your FPGA instance with the following command:
```
    $ ping <instance_ip>        (e.g. ping 10.12.200.21)
````

## How To Netcat
_Netcat_ (often abbreviated to `nc`) is a computer networking utility for reading from and 
writing to a network connection using TCP or UDP. If the tool is installed on your machine, 
you can use it to test the establishment of a connection with your FPGA instance when it is 
programmed with a _cFp_HelloKale_ bitstream.   

##### Step-1: To test the establishment of a TCP connection
Enter the following command and type in a stream of characters at the console. That 
stream will be echoed back by the FPGA instance.
```
    $ nc <image_ip> 8803     (e.g. nc 10.12.200.163 8803)
```
##### Step-2: To test the establishment of a UDP connection
Enter the following command and type in a stream of characters at the console. That 
stream will be echoed back by the FPGA instance.
```
    $ nc -u <image_ip> 8803     (e.g. nc -u 10.12.200.163 8803)
```

## How To Socat
_Socat_ is considered an advanced version of the _Netcat_ tool. It offers more 
functionality, such as permitting multiple clients to listen on a port, or reusing 
connections.

##### Step-1: To test the establishment of a TCP connection
Enter the following command and type in a stream of characters at the console. That 
stream will be echoed back by the FPGA instance.
```
    $ socat - TCP4:<image_ip>:8803    (e.g. socat - TCP4:10.12.200.163:8803)
```

##### Step-2: To test the establishment of a UDP connection
Enter the following command and type in a stream of characters at the console. That 
stream will be echoed back by the FPGA instance.
```
    $ socat - UDP4:<image_ip>:8803    (e.g. socat - UDP4:10.12.200.163:8803)
```

## How To iPerf 
_Iperf_ is a tool for network performance measurement and tuning. If the tool is 
installed on your machine, you can use it to measure the throughput between the 
host and an FPGA instance that is programmed with a _cFp_HelloKale_ bitstream. 

##### Step-1: To run iPerf in TCP client mode
Enter the following command to connect with your FPGA instance on TCP port *8800*.
```
    $ iperf --help
    $ iperf -c <11.22.33.44> -p 8800
    ------------------------------------------------------------
    Client connecting to <11.22.33.44>, TCP port 8800
    TCP window size:  117 KByte (default)
    ------------------------------------------------------------
    [  3] local 10.2.0.10 port 52074 connected with <11.22.33.44> port 8800
    [ ID] Interval       Transfer     Bandwidth
    [  3]  0.0-30.0 sec  1.20 GBytes   345 Mbits/sec
```

##### Step-2: To run iPerf in UDP client mode
Enter the following command to connect with your FPGA instance on UDP port *8800*.
```
    $ iperf --help
    $ iperf -c <11.22.33.44> -p 8800 -u
    ------------------------------------------------------------
    Client connecting to <11.22.33.44>, UDP port 8800
    Sending 1470 byte datagrams, IPG target: 11215.21 us (kalman adjust)
    UDP buffer size:  208 KByte (default)
    ------------------------------------------------------------
    [  3] local 10.2.0.10 port 37878 connected with <11.22.33.44> port 8800
    [  3] WARNING: did not receive ack of last datagram after 10 tries.
    [ ID] Interval       Transfer     Bandwidth
    [  3]  0.0-10.0 sec  1.25 MBytes  1.05 Mbits/sec
    [  3] Sent 892 datagrams
```

## How to Socket Programming with Python 
If you want to experiment with socket communications between a host and a cloudFPGA module,
visit the [HOST/py](./py/README.md) directory which contains a set of _Python_ scripts with
socket programming examples for interacting with the role of the _cFp_HelloKale_ project. 
 

