#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Jan 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description: A multi-threaded script that sends and receives traffic
#  *                on the TCP connection of an FPGA module.
#  *
#  *
#  *
#  *

# ### REQUIRED PYTHON PACKAGES ################################################
import argparse
import datetime
import ipaddress
import os
import random
import requests
import socket
import string
import sys
import threading
import time

# Return codes for the function calls
ACM_OK = 0    # ACM = Accelerator Module (alias for FM = Fpga Module)
ACM_KO = 1


def str_static_gen(size):
    """Generates a static string with length >= 'size'."""
    msg = '        Hello World     '
    while (len(msg)) < (size-2):
        msg += '01234567'
        msg += '89abcdef'
    msg += '\n\n'
    return msg.encode()


def str_rand_gen(size):
    """Generates a random string of length 'size'."""
    msg = "".join(random.choice(string.ascii_lowercase + string.digits) for _ in range(size-2))
    msg += '\n\n'
    return msg.encode()


def tcp_tx(sock, message, count):
    """TCP Tx Thread.
    :param sock,       the socket to send to.
    :param message,    the random string to sent.
    :param count,      the number of segments to send.
    :return None"""
    print("The following message of %d bytes will be sent out %d times:\n  Message=%s\n" % (len(message), count, message.decode()))
    c = 0
    startTime = datetime.datetime.now()
    while c < count:
        try:
            sock.sendall(message)
        finally:
            pass
        c += 1
    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime;
    bandwidth  = len(message) * 8 * count * 1.0 / (elapseTime.total_seconds() * 1024 * 1024)
    print("##################################################")
    print("#### TCP TX DONE with bandwidth = %6.1f Mb/s ####" % bandwidth)
    print("##################################################")


def tcp_rx(sock, message, count):
    """TCP Rx Thread.
     :param sock,       the socket to receive from.
     :param message,    the expected string message to be received.
     :param count,      the number of segment to receive.
     :return None"""
    loop = 0
    byteCnt = 0
    nrErr = 0
    while byteCnt < count*len(message):
        try:
            data = sock.recv(len(message))
            byteCnt += len(data)
            print("Loop=%d | RxBytes=%d" % (loop, byteCnt))
            #if data == message:
            #    print("Loop=%d | RxBytes=%d" % (loop, byteCnt))
            #else:
            #    print(" KO | Received %d bytes: Message=%s" % (len(data), data.decode()))
            #    # print("    | Was expecting    : Message=%s" % message)
            #    nrErr += 1
        finally:
            pass
        loop += 1
    print()
    print("########################################")
    print("#### TCP RX DONE with %4d error(s) ####" % nrErr)
    print("########################################")


def ipStrToList(ipAddrStr):
    """Convert an IP address from dotted string to a list.
    :return a list of integers.
    """
    ipAddrList = []
    # Split the dotted IP address using the 'dot' separator
    strFieldList = ipAddrStr.split(".")
    # Turn the dotted IP address into a list
    for o in range(0, len(strFieldList)):
        try:
            octet = int(strFieldList[o])
        except ValueError as ex:
            break
        else:
            ipAddrList.append(octet)
    return ipAddrList

def read_instance_id_from_console():
    """Read and return an instance Id from the console.
    :return the instance Id as an integer.
    """
    while True:
        instId = input()
        if not 1 <= int(instId) <= 32:
            print("ERROR: Bad format for the instance Id.")
            print("\tEnter a new instance Id in the range 1-32.\n")
        else:
            break
    return int(instId)

def read_ipv4_from_console():
    """Read and return an IPv4 address from the console.
    :return the IPV4 as a list on integers.
    """
    while True:
        ipAddrList = []
        ipAddrStr = input()
        ipAddrList = ipStrToList(ipAddrStr)
        if len(ipAddrList) != 4:
            print("ERROR: Bad format for the IPv4 address.")
        else:
            break
    return ipAddrList


def getFpgaIpv4(args):
    """Retrieve the IP address of the FPGA module.
    :param args, the options passed as arguments to the script.
    :return the IP address as an ipaddress.IPv4Address."""
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


###############################################################################
#                                                                             #
#                                 MAIN                                        #
#                                                                             #
###############################################################################

#  STEP-1: Parse the command line strings into Python objects
# -----------------------------------------------------------------------------
parser = argparse.ArgumentParser(description='A script to send/receive TCP data to/from an FPGA module.')
parser.add_argument('-fi', '--fpga_ipv4',  type=str, default='',
                           help='The destination IPv4 address of the FPGA (a.k.a image_ip / e.g. 10.12.200.163)')
parser.add_argument('-fp', '--fpga_port',  type=int, default=8803,
                           help='The TCP destination port of the FPGA (default is 8803)')
parser.add_argument('-ii', '--inst_id',    type=int, default=0,
                           help='The instance ID assigned by the cloudFPGA Resource Manager (range is 1-32)')
parser.add_argument('-lc', '--loop_count', type=int, default=10,
                           help='The number of times to run run the test (default is 10)')
parser.add_argument('-mi', '--mngr_ipv4',  type=str, default='10.12.0.132',
                           help='The IP address of the cloudFPGA Resource Manager (default is 10.12.0.132)')
parser.add_argument('-mp', '--mngr_port',  type=int, default=8080,
                           help='The TCP port of the cloudFPGA Resource Manager (default is 8080)')
parser.add_argument('-pw', '--password',   type=str, default='',
                           help='The ZYC2 password attached to the user name')
parser.add_argument('-un', '--user_name',  type=str, default='',
                           help='A user name as used to log in ZYC2 (.e.g \'fab\')')
parser.add_argument('-v',  '--verbose',    action="store_true",
                           help='Enable verbosity')

args = parser.parse_args()

if args.user_name == '' or args.password == "":
    print("\nWARNING: You must provide a ZYC2 user name and the corresponding password for this script to execute.\n")
    exit(1)

#  STEP-2a: Retrieve the IP address of the FPGA module (this will be the SERVER)
# -----------------------------------------------------------------------------
# OBSOLETE_20200108 ipFpgaStr = getFpgaIpv4(args)
# OBSOLETE_20200108 ipFpgaStr = args.fpga_ipv4
# OBSOLETE_20200108 if ipFpgaStr == '':
# OBSOLETE_20200108     print("Enter the IPv4 address of the FPGA module to connect to (e.g. 10.12.200.21)")
# OBSOLETE_20200108     ipDaList = read_ipv4_from_console()
# OBSOLETE_20200108 else:
# OBSOLETE_20200108     ipDaList = ipStrToList(ipFpgaStr)
# OBSOLETE_20200108     if len(ipDaList) != 4:
# OBSOLETE_20200108         print("ERROR: Unrecognized IPv4 address.\n")
# OBSOLETE_20200108         exit(1)
ipFpga = getFpgaIpv4(args)
print(">>>> ipFpga = %s = %d " % (str(ipFpga), int(ipFpga)))

#  STEP-2b: Retrieve the instance Id assigned by the cF Resource Manager
# -----------------------------------------------------------------------------
instId = args.inst_id
if instId == 0:
    print("Enter the instance Id that was assigned by the cloudFPGA Resource Manager (e.g. 1-32)")
    instId = read_instance_id_from_console()

#  STEP-2c: Retrieve the IP address of the cF Resource Manager
# -----------------------------------------------------------------------------
ipResManStr = args.mngr_ipv4
if ipResManStr == '':
    print("Enter the IP address of the cloudFPGA Resource Manager (e.g. 10.12.0.132)")
    ipResManList = read_ipv4_from_console()
else:
    ipResManList = ipStrToList(ipResManStr)
    if len(ipResManList) != 4:
        print("ERROR: Unrecognized IPv4 address.\n")
        exit(1)

#  STEP-3a: Retrieve the TCP destination port of the FPGA server
# -----------------------------------------------------------------------------
portFpga = args.fpga_port
if portFpga != 8803:
    print("ERROR: The current version of the cFp_BringUp role always listens on port #8803.\n")
    exit(1)

#  STEP-3b: Ask the TCP port of the cloudFPGA Resource Manager
# -----------------------------------------------------------------------------
tcpResMan = args.mngr_port
if tcpResMan != 8080:
    print("ERROR: The current version of the cloudFPGA Resource manager always listens on port #8080.\n")
    exit(1)

#  STEP-?: Configure the application registers
# -----------------------------------------------------------------------------
# TODO print("\nNow: Configuring the application registers.")
# TODO tcpEchoPathThruMode = (0x0 << 4)  # See DIAG_CTRL_2 register

#  STEP-4: Trigger the FPGA role to restart (i.e. perform SW reset of the role)
# -----------------------------------------------------------------------------
print("\nNow: Requesting the FPGA application to restart.")
try:
    # Build a request that is formatted as follows:
    #  http://10.12.0.132:8080/instances/13/app_restart?username=fab&password=secret
    reqUrl = "http://" + ipResManStr + ":" + str(tcpResMan) + "/instances/" \
                       + str(instId) + "/app_restart?username=" + args.user_name \
                       + "&password=" + args.password
    # DEBUG print("Generated request URL = ", reqUrl)
    r1 = requests.patch(reqUrl)
    print(r1.content.decode())
except Exception as e:
    print("ERROR: Failed to reset the FPGA role")
    print(str(e))
    exit(1)

#  STEP-5: Ping the FPGA
# -----------------------------------------------------------------------------
print("\nNow: Trying to \'ping\' the FPGA:")
rc = os.system("ping -c 2 " + str(ipFpga))
if rc != 0:
    print("ERROR: FPGA does not reply to \'ping\'!")
    exit(1)

#  STEP-6a: Set the FPGA socket association
# -----------------------------------------------------------------------------
tcpDP = 8803     # 8803=0x2263 and 0x6322=25378
fpgaAssociation = (str(ipFpga), tcpDP)

#  STEP-6b: Set the HOST socket association (optional)
#    Info: Linux selects a source port from an ephemeral port range, which by
#           default is a set to range from 32768 to 61000. You can check it
#           with the command:
#               > cat /proc/sys/net/ipv4/ip_local_port_range
#           If we want to force the source port ourselves, we must use the
#           "bind before connect" trick.
# -----------------------------------------------------------------------------
if 0:
    tcpSP = tcpDP + 49152  # 8803 + 0xC000
    hostAssociation = (ipSaStr, tcpSP)

#  STEP-7: Check if current socket is in TIME_WAIT state
# -------------------------------------------------------
wait = True
# Example: rc = os.system("netstat | grep '10.12.200.163:8803' | grep TIME_WAIT")
cmdStr = "netstat | grep \'" + str(ipFpga) + ":" + str(portFpga) + "\' | grep \'TIME_WAIT\|FIN_WAIT\' "
while wait:
    rc = os.system(cmdStr)
    if rc == 0:
        print("[INFO] Cannot reuse this socket as long as it is in the \'TIME_WAIT\' or \'FIN_WATIT\' state.")
        print("       Let's sleep for 5 sec...")
        time.sleep(5)
    else:
        wait = False

#  STEP-8a: Create a TCP/IP socket for the TCP/IP connection
# -----------------------------------------------------------------------------
try:
    tcpSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except Exception as exc:  # socket.error as exc:
    print("[EXCEPTION] %s" % exc)
    exit(1)

# STEP-8b: Bind before connect (optional).
#  This trick enables us to ask the kernel to select a specific source IP and
#  source PORT by calling bind() before calling connect().
# -----------------------------------------------------------------------------
if 0:
    try:
        tcpSock.bind(hostAssociation)
        print('Binding the socket address of the HOST to {%s, %d}' % hostAssociation)
    except Exception as exc:  # socket.error as exc:
        print("[EXCEPTION] %s" % exc)
        exit(1)

#  STEP-9: Connect to the remote FPGA
# -----------------------------------------------------------------------------
try:
    tcpSock.connect(fpgaAssociation)
except Exception as exc:
    print("[EXCEPTION] %s" % exc)
    exit(1)
else:
    print('\nSuccessful connection with socket address of FPGA at {%s, %d} \n' % fpgaAssociation)

#  STEP-10: Run the test
# -------------------------------
size = 128
count = args.loop_count

if 0:
    message = str_rand_gen(size)
else:
    message = str_static_gen(size)

#  STEP-11: Create Rx and Tx threads
# ----------------------------------
tx_thread = threading.Thread(target=tcp_tx, args=(tcpSock, message, count))
rx_thread = threading.Thread(target=tcp_rx, args=(tcpSock, message, count))

#  STEP-12: Start the threads
# ---------------------------
tx_thread.start()
rx_thread.start()

#  STEP-13: Wait for threads to terminate
# ----------------------------------------
tx_thread.join()
rx_thread.join()

#  STEP-14: Close socket
# -----------------------
tcpSock.close()
