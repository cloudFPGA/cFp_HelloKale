# *
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
# *

# *****************************************************************************
# * @file       : tc_TcpSend.py
# * @brief      : A single-threaded script to send traffic on the TCP
# *               connection of an FPGA module (i.e. HOST --> FPGA).
# *
# * System:     : cloudFPGA
# * Component   : cFp_Monolithic/ROLE
# * Language    : Python 3
# *
# *****************************************************************************

# ### REQUIRED PYTHON PACKAGES ################################################
import argparse
import datetime
import socket
import time

# ### REQUIRED TESTCASE MODULES ###############################################
from tc_utils import *


def tcp_tx_loop(sock, message, count, verbose=False):
    """TCP Tx Single-Thread Loop.
     :param sock     The socket to send/receive to/from.
     :param message  The message string to sent.
     :param count    The number of segments to send.
     :param verbose  Enables verbosity.
     :return         None"""
    if verbose:
        print("[INFO] The following message of %d bytes will be sent out %d times:\n  Message=%s\n" %
              (len(message), count, message.decode()))
    nrErr = 0
    loop = 0
    txByteCnt = 0
    startTime = datetime.datetime.now()
    while loop < count:
        #  Send segment
        # -------------------
        try:
            sock.sendall(message)
        except socket.error as exc:
            # Any exception
            print("[EXCEPTION] Socket error while transmitting :: %s" % exc)
            exit(1)
        finally:
            pass
        txByteCnt += len(message)
        if verbose:
            print("Loop=%d | TxBytes=%d" % (loop, txByteCnt))
            # if loop > 10:
            #     time.sleep(0.1)
            #     input('Hit <Enter> to continue:')
        loop += 1
    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime

    if txByteCnt < 1000000:
        print("[INFO] Transferred a total of %d bytes." % txByteCnt)
    elif txByteCnt < 1000000000:
        megaBytes = (txByteCnt * 1.0) / (1024 * 1024 * 1.0)
        print("[INFO] Transferred a total of %.1f MB." % megaBytes)
    else:
        gigaBytes = (txByteCnt * 1.0) / (1024 * 1024 * 1024 * 1.0)
        print("[INFO] Transferred a total of %.1f GB." % gigaBytes)

    bandwidth = (txByteCnt * 8 * 1.0) / (elapseTime.total_seconds() * 1024 * 1024)
    print("##################################################")
    if bandwidth < 1000:
        print("#### TCP Tx DONE with bandwidth = %6.1f Mb/s ####" % bandwidth)
    else:
        bandwidth = bandwidth / 1000
        print("#### TCP Tx DONE with bandwidth = %6.1f Gb/s ####" % bandwidth)
    print("##################################################")
    print()

def tcp_tx_ramp(sock, message, count, verbose=False):
    """TCP Tx Single-Thread Ramp. Send an incremental number of bytes from 1 to len(message).
     :param sock     The socket to send/receive to/from.
     :param message  The message string to sent.
     :param count    The number of segments to send.
     :param verbose  Enables verbosity.
     :return         None"""
    if verbose:
        print("[INFO] The following message of %d bytes will be sent out incrementally %d times:\n  Message=%s\n" %
              (len(message), count, message.decode()))
    nrErr = 0
    loop = 0
    txByteCnt = 0
    startTime = datetime.datetime.now()
    while loop < count:
        i = 1
        while i <= len(message):
            subMsg = message[0:i]
            #  Send segment
            # -------------------
            try:
                sock.sendall(subMsg)
            except socket.error as exc:
                # Any exception
                print("[EXCEPTION] Socket error while transmitting :: %s" % exc)
                exit(1)
            finally:
                pass
            txByteCnt += len(subMsg)
            if verbose:
                print("Loop=%d | TxBytes=%d | Msg=%s" % (loop, len(subMsg), subMsg))
            i += 1
        loop += 1
    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime

    if txByteCnt < 1000000:
        print("[INFO] Transferred a total of %d bytes." % txByteCnt)
    elif txByteCnt < 1000000000:
        megaBytes = (txByteCnt * 1.0) / (1024 * 1024 * 1.0)
        print("[INFO] Transferred a total of %.1f MB." % megaBytes)
    else:
        gigaBytes = (txByteCnt * 1.0) / (1024 * 1024 * 1024 * 1.0)
        print("[INFO] Transferred a total of %.1f GB." % gigaBytes)

    bandwidth = (txByteCnt * 8 * 1.0) / (elapseTime.total_seconds() * 1024 * 1024)
    print("#####################################################")
    if bandwidth < 1000:
        print("#### TCP Tx DONE with bandwidth = %6.1f Mb/s ####" % bandwidth)
    else:
        bandwidth = bandwidth / 1000
        print("#### TCP Tx DONE with bandwidth = %6.1f Gb/s ####" % bandwidth)
    print("#####################################################")
    print()


###############################################################################
#                                                                             #
#                                 MAIN                                        #
#                                                                             #
###############################################################################

#  STEP-1: Parse the command line strings into Python objects
# -----------------------------------------------------------------------------
parser = argparse.ArgumentParser(description='A script to send TCP data to an FPGA module.')
parser.add_argument('-fi', '--fpga_ipv4',   type=str, default='',
                           help='The IPv4 address of the FPGA (a.k.a image_ip / e.g. 10.12.200.163)')
parser.add_argument('-ii', '--inst_id',     type=int, default=0,
                           help='The instance ID assigned by the cloudFPGA Resource Manager (e.g. 42)')
parser.add_argument('-lc', '--loop_count',  type=int, default=10,
                           help='The number of test runs (default is 10)')
parser.add_argument('-mi', '--mngr_ipv4',   type=str, default='10.12.0.132',
                           help='The IPv4 address of the cloudFPGA Resource Manager (default is 10.12.0.132)')
parser.add_argument('-mp', '--mngr_port',   type=int, default=8080,
                           help='The TCP port of the cloudFPGA Resource Manager (default is 8080)')
parser.add_argument('-sd', '--seed',        type=int, default=-1,
                           help='The initial number to seed the pseudorandom number generator.')
parser.add_argument('-sz', '--size',        type=int, default=-1,
                           help='The size of the datagram to generate.')
parser.add_argument('-un', '--user_name',   type=str, default='',
                           help='A user-name as used to log in ZYC2 (.e.g \'fab\')')
parser.add_argument('-up', '--user_passwd', type=str, default='',
                           help='The ZYC2 password attached to the user-name')
parser.add_argument('-v',  '--verbose',     action="store_true",
                           help='Enable verbosity')

args = parser.parse_args()

if args.user_name == '' or args.user_passwd == '':
    print("\nWARNING: You must provide a ZYC2 user name and the corresponding password for this script to execute.\n")
    exit(1)

#  STEP-2a: Retrieve the IP address of the FPGA module (this will be the SERVER)
# -----------------------------------------------------------------------------
ipFpga = getFpgaIpv4(args)

#  STEP-2b: Retrieve the instance Id assigned by the cloudFPGA Resource Manager
# -----------------------------------------------------------------------------
instId = getInstanceId(args)

#  STEP-2c: Retrieve the IP address of the cF Resource Manager
# -----------------------------------------------------------------------------
ipResMngr = getResourceManagerIpv4(args)

#  STEP-3a: Set the TCP listen port of the FPGA server (this one is static)
# -----------------------------------------------------------------------------
portFpga = RECV_MODE_LSN_PORT  # 8800

#  STEP-3b: Retrieve the TCP port of the cloudFPGA Resource Manager
# -----------------------------------------------------------------------------
portResMngr = getResourceManagerPort(args)

#  STEP-4: Trigger the FPGA role to restart (i.e. perform SW reset of the role)
# -----------------------------------------------------------------------------
restartApp(instId, ipResMngr, portResMngr, args.user_name, args.user_passwd)

#  STEP-5: Ping the FPGA
# -----------------------------------------------------------------------------
pingFpga(ipFpga)

#  STEP-6a: Set the FPGA socket association
# -----------------------------------------------------------------------------
fpgaAssociation = (str(ipFpga), portFpga)

#  STEP-6b: Set the HOST socket association (optional)
#    Info: Linux selects a source port from an ephemeral port range, which by
#           default is a set to range from 32768 to 61000. You can check it
#           with the command:
#               > cat /proc/sys/net/ipv4/ip_local_port_range
#           If we want to force the source port ourselves, we must use the
#           "bind before connect" trick.
# -----------------------------------------------------------------------------
if 0:
    tcpSP = portFpga + 49152  # 8803 + 0xC000
    hostAssociation = (ipSaStr, tcpSP)

#  STEP-8a: Create a TCP/IP socket for the TCP/IP connection
# -----------------------------------------------------------------------------
try:
    tcpSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except Exception as exc:
    print("[EXCEPTION] %s" % exc)
    exit(1)

#  Step-8b: Allow this socket to be re-used and disable the Nagle's algorithm
# ----------------------------------------------------------------------------
tcpSock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
tcpSock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, True)

#  STEP-8c: Bind before connect (optional).
#   This trick enables us to ask the kernel to select a specific source IP and
#   source PORT by calling bind() before calling connect().
# -----------------------------------------------------------------------------
if 0:
    try:
        tcpSock.bind(hostAssociation)
        print('Binding the socket address of the HOST to {%s, %d}' % hostAssociation)
    except Exception as exc:
        print("[EXCEPTION] %s" % exc)
        exit(1)

#  STEP-9a: Connect with remote FPGA
# -----------------------------------------------------------------------------
try:
    tcpSock.connect(fpgaAssociation)
except Exception as exc:
    print("[EXCEPTION] %s" % exc)
    exit(1)
else:
    print('\nSuccessful connection with socket address of FPGA at {%s, %d} \n' % fpgaAssociation)

#  STEP-9b: Set the socket non-blocking
# --------------------------------------
tcpSock.setblocking(False)
tcpSock.settimeout(5)

#  STEP-10: Setup the test
# -------------------------------
print("[INFO] Testcase `%s` is run with:" % (os.path.basename(__file__)))
seed = args.seed
if seed == -1:
    seed = random.randint(0, 100000)
random.seed(seed)
print("\t\t seed = %d" % seed)

size = args.size
if size == -1:
    size = random.randint(1, ZYC2_MSS)
elif size > ZYC2_MSS:
    print('\nERROR: ')
    print("[ERROR] This test-case expects the transfer of segment which are less or equal to MSS (.i.e %d bytes).\n" % ZYC2_MSS)
    exit(1)
print("\t\t size = %d" % size)

count = args.loop_count
print("\t\t loop = %d" % count)

if seed % 1:
    message = str_rand_gen(size)
else:
    message = str_static_gen(size)

verbose = args.verbose

#  STEP-11: Run the test
# -------------------------------
print("[INFO] This testcase is sending traffic from HOST-to-FPGA. ")
print("[INFO] It is run in single-threading mode.\n")
if seed == 0:
    tcp_tx_ramp(tcpSock, message, count, args.verbose)
else:
    tcp_tx_loop(tcpSock, message, count, args.verbose)

#  STEP-14: Close socket
# -----------------------
time.sleep(2)
tcpSock.close()


