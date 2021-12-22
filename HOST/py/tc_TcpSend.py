# *
# * Copyright 2016 -- 2021 IBM Corporation
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

    display_throughput(txByteCnt, elapseTime)

def tcp_tx_slowpace(sock, message, count, pause, verbose=False):
    """TCP Tx test at reduce speed (by inserting a sleep duration in between two transmissions)
     :param sock     The socket to send/receive to/from.
     :param message  The message string to sent.
     :param count    The number of segments to send.
     :param pause    The idle duration between two segments (in seconds)
     :param verbose  Enables verbosity.
     :return         None"""
    if verbose:
        print("[INFO] TCP-TX-SLOW-PACE: The following message of %d bytes will be sent out %d times with an inter-gap time of %f seconds:\n  Message=%s\n" %
              (len(message), count, pause, message.decode()))
    nrErr = 0
    loop  = 0
    totalByteCnt = 0
    txWinByteCnt = 0
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
        txWinByteCnt += len(message)
        totalByteCnt += len(message)
        if verbose:
            print("Loop=%6d | TotalTxBytes=%6d | Pause=%4f sec" % (loop+1, totalByteCnt, pause))
        time.sleep(pause)
        txWinByteCnt = 0
        loop += 1
    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime

    display_throughput(totalByteCnt, elapseTime)
    

def tcp_tx_payload_ramp(sock, message, count, pause=0.0, verbose=False):
    """TCP Tx Single-Thread Ramp. Send a buffer of bytes with 64-bit unsigned integer numbers
         ramping up from 1 to len(message).
     :param sock     The socket to send/receive to/from.
     :param message  The message string to sent.
     :param count    The number of segments to send.
     :param pause    The idle duration between two segments (in seconds)
     :param verbose  Enables verbosity.
     :return         None"""
    strStream = ""
    size = len(message) * count
    if size <= 8:
        for i in range(0, size-1):
            strStream += '0'
    else:
        rampSize = int(size/8)
        for i in range(0, rampSize):
            strTmp = "{:08d}".format(i)
            # Swap the generated 8 bytes
            strStream += strTmp[7]
            strStream += strTmp[6]
            strStream += strTmp[5]
            strStream += strTmp[4]
            strStream += strTmp[3]
            strStream += strTmp[2]
            strStream += strTmp[1]
            strStream += strTmp[0]
        for i in range(0, int(size % 8)):
            strStream += 'E'
    bytStream = strStream.encode()

    if verbose:
        print("[INFO] The following stream of %d bytes will be sent out:\n  Message=%s\n" %
              (len(message), bytStream.decode()))

    startTime = datetime.datetime.now()

    if pause == 0.0:
        # --------------------------------
        #  Send the entire stream at once
        # --------------------------------
        try:
            sock.sendall(bytStream)
        except socket.error as exc:
            # Any exception
            print("[EXCEPTION] Socket error while transmitting :: %s" % exc)
            exit(1)
        finally:
            pass
    else:
        # --------------------------------------
        #  Send the stream in pieces of 8 bytes.
        #  The use of a 'pause' enforces the TCP 'PSH'
        # --------------------------------------
        for i in range(0, rampSize):
            subStr = bytStream[i*8: i*8+8]
            try:
                sock.sendall(subStr)
            except socket.error as exc:
                # Any exception
                print("[EXCEPTION] Socket error while transmitting :: %s" % exc)
                exit(1)
            finally:
                pass
            time.sleep(pause)
        if size % 8:
            subStr = bytStream[(size/8)*8: (size/8)*8 + (size % 8)]
            try:
                sock.sendall(subStr)
            except socket.error as exc:
                # Any exception
                print("[EXCEPTION] Socket error while transmitting :: %s" % exc)
                exit(1)
            finally:
                pass
            time.sleep(pause)

    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime

    txByteCnt = len(message) * count
    display_throughput(txByteCnt, elapseTime)

def tcp_tx_seg_size_ramp(sock, message, count, pause=0.0, verbose=False):
    """Send a ramp of increasing segment sizes starting from 1 bytes up to len(message).
     :param sock     The socket to send/receive to/from.
     :param message  The message string to sent.
     :param count    The number of segments to send.
     :param pause    The idle duration between two segments (in seconds)
     :param verbose  Enables verbosity.
     :return         None"""

    nrErr = 0
    loop = 0
    txByteCnt = 0
    startTime = datetime.datetime.now()

    while loop < count:
        for i in range(1, len(message)):
            #  Send segment of length 'i'
            subMsg = message[0: i]
            try:
                sock.sendall(subMsg)
            except socket.error as exc:
                # Any exception
                print("[EXCEPTION] Socket error while transmitting :: %s" % exc)
                exit(1)
            finally:
                pass
            txByteCnt += len(message)
            if verbose:
                print("Loop=%4.4d | SegmentLength=%4.4d" % (loop, i))
            if pause != 0.0:
                time.sleep(pause)
            loop += 1

    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime

    display_throughput(txByteCnt, elapseTime)


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
parser.add_argument('-st', '--sleep_time',  type=float, default=0.0,
                           help='Enforce a sleep time in between two segments (in seconds)')
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

if seed % 2:
    message = str_rand_gen(size)
else:
    message = str_static_gen(size)

verbose = args.verbose

#  STEP-11: Run the test
# -------------------------------
print("[INFO] This testcase is sending traffic from HOST-to-FPGA. ")
print("[INFO] It is run in single-threading mode.\n")
if seed == 0:
    # SEND A PAYLOAD WITH INCREMENTAL DATA NUMBERS
    message = "X" * size
    tcp_tx_payload_ramp(tcpSock, message, count, args.sleep_time, args.verbose)
elif seed == 1:
    # SEND A RAMP OF INCREMENTAL SEGMENT SIZES
    tcp_tx_seg_size_ramp(tcpSock, message, count, args.sleep_time, args.verbose)
else:
    if args.sleep_time > 0.0:
        # RUN THE TEST AT LOW SPEED
        tcp_tx_slowpace(tcpSock, message, count, args.sleep_time, args.verbose)
    else:
        tcp_tx_loop(tcpSock, message, count, args.verbose)

#  STEP-14: Close socket
# -----------------------
time.sleep(2)
tcpSock.close()


