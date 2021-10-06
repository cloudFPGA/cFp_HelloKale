#/*
# * Copyright 2016 -- 2020 IBM Corporation
# * 
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *     http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# */

# *****************************************************************************
# * @file       : tc_utils.py
# * @brief      : A python module with some helper functions for the UDP and
# *               TCP test-cases.
# * System:     : cloudFPGA
# * Component   : cFp_BringUp/ROLE
# * Language    : Python 3
# *
# *****************************************************************************

# ### REQUIRED PYTHON PACKAGES ################################################
import ipaddress
import os
import random
import requests
import string

# Return codes for the function calls
ACM_OK = 0    # ACM = Accelerator Module (alias for FM = Fpga Module)
ACM_KO = 1

MTU         = 1500  # ETHERNET - Maximum Transfer Unit
MTU_ZYC2    = 1450  # ETHERNET - MTU in ZYC2 = 1500-20-8-8-14
IP4_HDR_LEN =   20  # IPv4 Header Length
TCP_HDR_LEN =   20  # TCP Header Length
ZYC2_MSS = (MTU_ZYC2 - 92) & ~0x7  # ZYC2 Maximum Segment Size (1352)
UDP_HDR_LEN =    8  # UDP Header Length
UDP_MDS = (MTU_ZYC2 - IP4_HDR_LEN - UDP_HDR_LEN) & ~0x7  # Maximum Datagram Size (modulo 8)

# -------------------------------------------------------------------
# -- DEFAULT LISTENING PORTS
# --  By default, the following port numbers will be used by the
# --  UdpShellInterface (unless user specifies new ones via TBD).
# --  Default listen ports:
# --  --> 5001 : Traffic received on this port is [TODO-TBD].
# --             It is used to emulate IPERF V2.
# --  --> 5201 : Traffic received on this port is [TODO-TBD].
# --             It is used to emulate IPREF V3.
# --  --> 8800 : Traffic received on this port is systematically
# --             dumped. It is used to test the Rx part of UOE.
# --  --> 8801 : A message received on this port triggers the
# --             transmission of 'nr' bytes from the FPGA to the host.
# --             It is used to test the Tx part of UOE.
# --  --> 8803 : Traffic received on this port is looped backed and
# --             echoed to the sender.
# -------------------------------------------------------------------
RECV_MODE_LSN_PORT  = 8800    # 0x2260
XMIT_MODE_LSN_PORT  = 8801    # 0x2261
BIDIR_MODE_LSN_PORT = 8802    # 0x2262
ECHO_MODE_LSN_PORT  = 8803    # 0x2263
IPERF_LSN_PORT      = 5001    # 0x1389
IPREF3_LSN_PORT     = 5201    # 0x1451

def num_to_char(num):
    """ Function to map a number to a character."""
    switcher = {
         0: '0',  1: '1',  2: '2',  3: '3',  4: '4',  5: '5',  6: '6',  7: '7',
         8: '8',  9: '9', 10: 'a', 11: 'b', 12: 'c', 13: 'd', 14: 'e', 15: 'f'
    }
    return switcher.get(num, ' ')  # ' ' is default


def str_static_gen(size):
    """Returns an encoded static string of length 'size'."""
    msg = '\n'
    msg += '__________Hello_World__________'
    while (len(msg)) < (size):
        msg += num_to_char(len(msg) % 16)
    msg = (msg[:size]) if len(msg) > size else msg
    return msg.encode()


def str_rand_gen(size):
    """Returns an encoded random string of length 'size'."""
    msg = '\n'
    msg += "".join(random.choice(string.ascii_lowercase + string.digits) for _ in range(size-1))
    return msg.encode()


def getFpgaIpv4(args):
    """Retrieve the IPv4 address of the FPGA module.
    :param args  The options passed as arguments to the script.
    :return      The IPv4 address as an 'ipaddress.IPv4Address'."""
    ipFpgaStr = args.fpga_ipv4
    while True:
        if args.fpga_ipv4 == '':
            # Take an IP address from the console
            print("Enter the IPv4 address of the FPGA module to connect to (e.g. 10.12.200.21)")
            ipFpgaStr = input()
        try:
            ipFpga = ipaddress.ip_address(ipFpgaStr)
        except ValueError:
            print('[ERROR] Unrecognized IPv4 address.')
        else:
            return ipFpga


def getFpgaPort(args):
    """Retrieve the UDP listen port of the FPGA.
    :param args The options passed as arguments to the script.
    :return     The UDP port number as an integer."""
    portFpga = args.fpga_port
    if portFpga != 8803:
        print("[ERROR] The current version of the cFp_BringUp role always listens on port #8803.\n")
        exit(1)
    return portFpga


def getResourceManagerIpv4(args):
    """Retrieve the IPv4 address of the cloudFPGA Resource Manager.
    :param args The options passed as arguments to the script.
    :return     The IP address as an 'ipaddress.IPv4Address'."""
    ipResMngrStr = args.mngr_ipv4
    while True:
        if args.mngr_ipv4 == '':
            # Take an IP address from the console
            print("Enter the IPv4 address of the cloudFPGA Resource Manager (e.g. 10.12.0.132)")
            ipResMngrStr = input()
        try:
            ipResMngr = ipaddress.ip_address(ipResMngrStr)
        except ValueError:
            print('[ERROR] Unrecognized IPv4 address.')
        else:
            return ipResMngr


def getResourceManagerPort(args):
    """Retrieve the TCP port of the cloudFPGA Resource Manager.
    :param args The options passed as arguments to the script.
    :return     The TCP port number as an integer."""
    portMngr = args.mngr_port
    if portMngr != 8080:
        print("[ERROR] The current version of the cloudFPGA Resource manager always listens on port #8080.\n")
        exit(1)
    return portMngr


def getInstanceId(args):
    """Retrieve the instance Id that was assigned by the cloudFPGA Resource Manager.
    :param args The options passed as arguments to the script.
    :return     The instance Id as an integer."""
    instId = args.inst_id
    while True:
        if not 1 <= args.inst_id:   # Take an instance Id from the console
            print("Enter the instance Id that was assigned by the cloudFPGA Resource Manager (e.g. 42)")
            instIdStr = input()
            try:
                instId = int(instIdStr)
            except ValueError:
                print("ERROR: Bad format for the instance Id.")
                print("\tEnter a new instance Id > 0.\n")
            else:
                break
        else:
            break;
    return instId


def restartApp(instId, ipResMngr, portResMngr, user_name, user_passwd):
    """Trigger the role of an FPGA to restart (i.e. perform a SW reset of the role)
    :param instId:      The instance Id to restart.
    :param ipResMngr:   The IPv4 address of the cF resource manager.
    :param portResMngr: The TCP port number of the cF resource manager.
    :param user_name:   The user name as used to log in ZYC2.
    :param user_passwd: The ZYC2 password attached to the user name.
    :return:            Nothing
    """
    print("\nNow: Requesting the application of FPGA instance #%d to restart." % instId)
    try:
        # We must build a request that is formatted as follows:
        #  http://10.12.0.132:8080/instances/13/app_restart?username=fab&password=secret
        reqUrl = "http://" + str(ipResMngr) + ":" + str(portResMngr) + "/instances/" \
                 + str(instId) + "/app_restart?username=" + user_name \
                 + "&password=" + user_passwd
        # DEBUG print("Generated request URL = ", reqUrl)
        r1 = requests.patch(reqUrl)
        print(r1.content.decode())
    except Exception as e:
        print("ERROR: Failed to reset the FPGA role")
        print(str(e))
        exit(1)


def pingFpga(ipFpga):
    """Ping an FPGA.
    :param ipFpga:  The IPv4 address of the FPGA.
    :return:        Nothing
    """
    print("Now: Trying to \'ping\' the FPGA: ")
    # Send 2 PINGs and wait 2 seconds max for each of them (for a total max of 4s)
    rc = os.system("ping -c 2 -W 2 " + str(ipFpga))
    if rc != 0:
        print("[ERROR] FPGA does not reply to \'ping\'!")
        exit(1)

def display_throughput(byteCount, elapseTime):
    """Display the throughput in human readable form.
     :param byteCount:  The number of bytes transferred.
     :param elapseTime: The duration of the transfer.
     :return:           Nothing
     """
    if byteCount < 1000000:
        print("[INFO] Transferred a total of %d bytes." % byteCount)
    elif byteCount < 1000000000:
        megaBytes = (byteCount * 1.0) / (1024 * 1024 * 1.0)
        print("[INFO] Transferred a total of %.1f MB." % megaBytes)
    else:
        gigaBytes = (byteCount * 1.0) / (1024 * 1024 * 1024 * 1.0)
        print("[INFO] Transferred a total of %.1f GB." % gigaBytes)
    throughput = (byteCount * 8 * 1.0) / (elapseTime.total_seconds() * 1024 * 1024)

    strMsg = ""
    if throughput < 1000:
        strMsg = "#### DONE with throughput = %.1f Mb/s ####" % throughput
    else:
        throughput = throughput / 1000
        strMsg = "#### DONE with throughput = %.1f Gb/s ####" % throughput

    strHashLine = ""
    for i in range(0, len(strMsg)):
        strHashLine += "#"

    print(strHashLine)
    print(strMsg)
    print(strHashLine)
    print()


