HOST / py / testFiles
==========================
**Test files for troubleshooting the TCP network interface of the cFp_Monolithic project.**

## ABOUT
This directory contains a _Python_ script for generating a set of test files for use as data payload in bidiretcionnal data transfers between the HOST and an FPGA module. 


## Step-1: Environment requirements
To ease the execution of the local python test scripts, we recommend _Python v3.6_ or 
higher, and the use of a virtual environment. 
If you have not created a Virtual Environment yet, refer to the step-by-step procedure in ../README.md.   

## Step-2: How to generate a test file
The _Python_ script `genTestFile.py` can be used to generate the content of a test file for use with the socat command.  The syntax is fairly simple and is as follows:
```
    $ python3 genTestFile.py <size> <pattern>
``` 
with <size> being the size of the file to create (e.g. 128, 256K, 512M, 2G),  and <pattern> specifying the content of the file (e.g. -inc,-dec,-rnd).
Here are a few examples: 
```
    $ python3 genTestFile.py -sz 128   -inc     # Generates a ramp of incremented numbers
    $ python3 genTestFile.py -sz 256K  -dec     # Generates a ramp of decremented numbers
    $ python3 genTestFile.py -sz 512M  -rnd     # Generates random numbers
```
By default, the script creates a file which name is generated by concatenating the <pattern> and <size>. You can overwrite this default naming by passing the option `-f <filename>`.

## Step-3: How to socat
The socat utility is a relay for bidirectional data transfers between two independent data channels. The following command uses `socat` to channel a test file between STDIO and a TCP connection to port 8800 on a an IP address 10.12.200.162 of an FPGA module.
```
    $ cat <testFile> | socat stdio,shut-none tcp:10.12.200.162:8800,shut-none
```