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
# * @file       : tc_TcpEcho.py
# * @brief      : A multi-threaded script to send and receive traffic on the
# *               TCP connection of an FPGA module.
# *
# * System:     : cloudFPGA
# * Component   : cFp_BringUp/ROLE
# * Language    : Python 3
# *
# *****************************************************************************

# ### REQUIRED PYTHON PACKAGES ################################################
import argparse
import datetime
import errno
import socket
import threading
import time

# ### REQUIRED TESTCASE MODULES ###############################################
from tc_utils import *


def tcp_tx(sock, message, count, verbose=False):
    """TCP Tx Thread.
    :param sock,       the socket to send to.
    :param message,    the random string to sent.
    :param count,      the number of segments to send.
    :param verbose,    enables verbosity.
    :return None"""
    if verbose:
        print("The following message of %d bytes will be sent out %d times:\n  Message=%s\n" %
              (len(message), count, message.decode()))
    loop = 0
    startTime = datetime.datetime.now()
    while loop < count:
        try:
            sock.sendall(message)
        finally:
            pass
        loop += 1
    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime;
    bandwidth  = len(message) * 8 * count * 1.0 / (elapseTime.total_seconds() * 1024 * 1024)
    print("##################################################")
    print("#### TCP TX DONE with bandwidth = %6.1f Mb/s ####" % bandwidth)
    print("##################################################")
    print()

def tcp_rx(sock, message, count, verbose):
    """TCP Rx Thread.
     :param sock,       the socket to receive from.
     :param message,    the expected string message to be received.
     :param count,      the number of segment to receive.
     :param verbose,    enables verbosity.
     :return None"""
    loop = 0
    rxBytes = 0
    nrErr = 0
    startTime = datetime.datetime.now()
    while rxBytes < count*len(message):
        try:
            data = sock.recv(len(message))
            rxBytes += len(data)
        except socket.error as exc:
            print("[EXCEPTION] Socket error while receiving :: %s" % exc)
        else:
            if data == message:
                if verbose:
                    print("Loop=%d | RxBytes=%d" % (loop, rxBytes))
            else:
                print("Loop=%d | RxBytes=%d" % (loop, rxBytes))
                print(" KO | Received  Message=%s" % data.decode())
                print("    | Expecting Message=%s" % message)
                nrErr += 1
        loop += 1
    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime;
    bandwidth  = len(message) * 8 * count * 1.0 / (elapseTime.total_seconds() * 1024 * 1024)
    print("##################################################")
    print("#### TCP RX DONE with bandwidth = %6.1f Mb/s ####" % bandwidth)
    print("##################################################")
    print()

def waitUntilSocketPairCanBeReused(ipFpga, portFpga):
    """Check and wait until the a socket pair can be reused.
        [INFO] When a client or a server initiates an active close, then the same destination socket
          (i.e. the same IP address / TCP port number) cannot be re-used immediately because
          of security issues. Therefore, a closed connection must linger in a 'TIME_WAIT' or
          'FIN_WAIT' state for as long as 2xMSL (Maximum Segment Lifetime), which corresponds
          to twice the time a TCP segment might exist in the internet system. The MSL is
          arbitrarily defined to be 2 minutes long.
    :param ipFpga:   the IP address of FPGA.
    :param portFpga: the TCP port of the FPGA.
    :return: nothing
    """
    wait = True
    # NETSTAT example: rc = os.system("netstat | grep '10.12.200.163:8803' | grep TIME_WAIT")
    cmdStr = "netstat | grep \'" + str(ipFpga) + ":" + str(portFpga) + "\' | grep \'TIME_WAIT\|FIN_WAIT\' "
    while wait:
        rc = os.system(cmdStr)
        if rc == 0:
            print("[INFO] Cannot reuse this socket as long as it is in the \'TIME_WAIT\' or \'FIN_WAIT\' state.")
            print("       Let's sleep for 5 sec...")
            time.sleep(5)
        else:
            wait = False

def tcp_txrx_loop(sock, message, count, verbose=False):
    """TCP Tx-Rx Single-Thread Loop.
     :param sock     The socket to send/receive to/from.
     :param message  The message string to sent.
     :param count    The number of segments send.
     :param verbose  Enables verbosity.
     :return         None"""
    if verbose:
        print("[INFO] The following message of %d bytes will be sent out %d times:\n  Message=%s\n" %
              (len(message), count, message.decode()))
    nrErr = 0
    txMssgCnt = 0
    rxMssgCnt = 0
    rxByteCnt = 0
    txStream = ""
    rxStream = ""

    # Init the Tx reference stream
    for i in range(count):
        txStream = txStream + message.decode()

    startTime = datetime.datetime.now()

    while rxByteCnt < (count * len(message)):
        if txMssgCnt < count:
            #  Send a new message
            # ------------------------
            try:
                tcpSock.sendall(message)
                txMssgCnt += 1
            finally:
                pass

        #  Receive a segment
        # --------------------
        try:
            data = tcpSock.recv(len(message))
            rxByteCnt += len(data)
            rxMssgCnt += 1
        except IOError as e:
            # On non blocking connections - when there are no incoming data, error is going to be
            # raised. Some operating systems will indicate that using AGAIN, and some using
            # WOULDBLOCK error code.  We are going to check for both - if one of them - that's
            # expected, means no incoming data, continue as normal. If we got different error code,
            # something happened
            if e.errno != errno.EAGAIN and e.errno != errno.EWOULDBLOCK:
                print('[ERROR] Socket reading error: {}'.format(str(e)))
                exit(1)
            # We just did not receive anything
            continue
        except socket.error as exc:
            # Any other exception
            print("[EXCEPTION] Socket error while receiving :: %s" % exc)
            # exit(1)
        finally:
            pass
        rxStream = rxStream + data.decode()

    endTime = datetime.datetime.now()

    # Compare Tx and Rx stream
    if rxStream != txStream:
        print(" KO | Received stream = %s" % data.decode())
        print("    | Expected stream = %s" % rxStream)
        nrErr += 1
    elif verbose:
        print(" OK | Received %d bytes in %d messages." % (rxByteCnt, rxMssgCnt))

    elapseTime = endTime - startTime;
    bandwidth  = len(message) * 8 * count * 1.0 / (elapseTime.total_seconds() * 1024 * 1024)
    print("[INFO] Transferred a total of %d bytes." % rxByteCnt)
    print("#####################################################")
    print("#### TCP Tx/Rx DONE with bandwidth = %6.1f Mb/s ####" % bandwidth)
    print("#####################################################")
    print()

def tcp_txrx_ramp(sock, message, count, verbose=False):
    """TCP Tx-Rx Single-Thread Ramp.
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
    rxByteCnt = 0
    startTime = datetime.datetime.now()
    while loop < count:
        i = 12  # [FIXME- Must be '1']
        print("[FIXME - Message size starts at 12 bytes instead of 1! ]")
        while i <= len(message):
            subMsg = message[0:i]
            #  Send datagram
            # -------------------
            try:
                tcpSock.sendall(subMsg)
            finally:
                pass
            #  Receive datagram
            # -------------------
            try:
                data = tcpSock.recv(len(subMsg))
                rxByteCnt += len(data)
                if data == subMsg:
                    if verbose:
                        print("Loop=%d | RxBytes=%d" % (loop, len(data)))
                else:
                    print("Loop=%d | RxBytes=%d" % (loop, len(data)))
                    print(" KO | Received  Message=%s" % data.decode())
                    print("    | Expecting Message=%s" % subMsg)
                    nrErr += 1
            except IOError as e:
                # On non blocking connections - when there are no incoming data, error is going to be raised
                # Some operating systems will indicate that using AGAIN, and some using WOULDBLOCK error code
                # We are going to check for both - if one of them - that's expected, means no incoming data,
                # continue as normal. If we got different error code - something happened
                if e.errno != errno.EAGAIN and e.errno != errno.EWOULDBLOCK:
                    print('[ERROR] Socket reading error: {}'.format(str(e)))
                    exit(1)
                # We just did not receive anything
                continue
            except socket.error as exc:
                # Any other exception
                print("[EXCEPTION] Socket error while receiving :: %s" % exc)
                # exit(1)
            finally:
                pass
            i += 1
        loop += 1
    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime
    bandwidth  = (rxByteCnt * 8 * count * 1.0) / (elapseTime.total_seconds() * 1024 * 1024)
    megaBytes  = (rxByteCnt * 1.0) / (1024 * 1024 * 1.0)
    print("[INFO] Transferred a total of %.1f MB." % megaBytes)
    print("#####################################################")
    print("#### TCP Tx/Rx DONE with bandwidth = %6.1f Mb/s ####" % bandwidth)
    print("#####################################################")
    print()


###############################################################################
#                                                                             #
#                                 MAIN                                        #
#                                                                             #
###############################################################################

#  STEP-1: Parse the command line strings into Python objects
# -----------------------------------------------------------------------------
parser = argparse.ArgumentParser(description='A script to send/receive TCP data to/from an FPGA module.')
parser.add_argument('-fi', '--fpga_ipv4',   type=str, default='',
                           help='The destination IPv4 address of the FPGA (a.k.a image_ip / e.g. 10.12.200.163)')
parser.add_argument('-fp', '--fpga_port',   type=int, default=8803,
                           help='The TCP destination port of the FPGA (default is 8803)')
parser.add_argument('-ii', '--inst_id',     type=int, default=0,
                           help='The instance ID assigned by the cloudFPGA Resource Manager (range is 1-32)')
parser.add_argument('-lc', '--loop_count',  type=int, default=10,
                           help='The number of times to run run the test (default is 10)')
parser.add_argument('-mi', '--mngr_ipv4',   type=str, default='10.12.0.132',
                           help='The IP address of the cloudFPGA Resource Manager (default is 10.12.0.132)')
parser.add_argument('-mp', '--mngr_port',   type=int, default=8080,
                           help='The TCP port of the cloudFPGA Resource Manager (default is 8080)')
parser.add_argument('-mt', '--multi_threading',       action="store_true",
                           help='Enable multi_threading')
parser.add_argument('-sd', '--seed',        type=int, default=-1,
                           help='The initial number to seed the pseudo-random number generator.')
parser.add_argument('-sz', '--size',        type=int, default=-1,
                           help='The size of the segment to generate.')
parser.add_argument('-un', '--user_name',   type=str, default='',
                           help='A user name as used to log in ZYC2 (.e.g \'fab\')')
parser.add_argument('-up', '--user_passwd', type=str, default='',
                           help='The ZYC2 password attached to the user name')
parser.add_argument('-v',  '--verbose',     action="store_true",
                           help='Enable verbosity')

args = parser.parse_args()

if args.user_name == '' or args.user_passwd == '':
    print("\nWARNING: You must provide a ZYC2 user name and the corresponding password for this script to execute.\n")
    exit(1)

#  STEP-2a: Retrieve the IP address of the FPGA module (this will be the SERVER)
# ------------------------------------------------------------------------------
ipFpga = getFpgaIpv4(args)

#  STEP-2b: Retrieve the instance Id assigned by the cloudFPGA Resource Manager
# -----------------------------------------------------------------------------
instId = getInstanceId(args)

#  STEP-2c: Retrieve the IP address of the cF Resource Manager
# -----------------------------------------------------------------------------
ipResMngr = getResourceManagerIpv4(args)

#  STEP-3a: Retrieve the TCP port of the FPGA server
# -----------------------------------------------------------------------------
portFpga = getFpgaPort(args)

#  STEP-3b: Retrieve the TCP port of the cloudFPGA Resource Manager
# -----------------------------------------------------------------------------
portResMngr = getResourceManagerPort(args)

#  STEP-?: Configure the application registers
# -----------------------------------------------------------------------------
# TODO print("\nNow: Configuring the application registers.")
# TODO tcpEchoPathThruMode = (0x0 << 4)  # See DIAG_CTRL_2 register

#  STEP-4: Trigger the FPGA role to restart (i.e. perform SW reset of the role)
# -----------------------------------------------------------------------------
restartApp(instId, ipResMngr, portResMngr, args.user_name, args.user_passwd)

#  STEP-5: Ping the FPGA
# -----------------------------------------------------------------------------
pingFpga(ipFpga)

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

#  STEP-7: Wait until the current socket can be reused
# -----------------------------------------------------------------------------
if 0:
    waitUntilSocketPairCanBeReused(ipFpga, portFpga)

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

# STEP-8c: Bind before connect (optional).
#  This trick enables us to ask the kernel to select a specific source IP and
#  source PORT by calling bind() before calling connect().
# -----------------------------------------------------------------------------
if 0:
    try:
        tcpSock.bind(hostAssociation)
        print('Binding the socket address of the HOST to {%s, %d}' % hostAssociation)
    except Exception as exc:
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
    message = str_static_gen(size)
else:
    message = str_rand_gen(size)

verbose = args.verbose

print("[INFO] This testcase is sending traffic from HOST-to-FPGA and back from FPGA-to-HOST.")
if args.multi_threading:
    print("[INFO] This run is executed in multi-threading mode.\n")
    #  STEP-11: Create Rx and Tx threads
    # ----------------------------------
    tx_thread = threading.Thread(target=tcp_tx, args=(tcpSock, message, count, args.verbose))
    rx_thread = threading.Thread(target=tcp_rx, args=(tcpSock, message, count, args.verbose))
    #  STEP-12: Start the threads
    # ---------------------------
    tx_thread.start()
    rx_thread.start()
    #  STEP-13: Wait for threads to terminate
    # ----------------------------------------
    tx_thread.join()
    rx_thread.join()
else:
    print("[INFO] The run is executed in single-threading mode.\n")
    #  STEP-11: Set the socket in non-blocking mode
    # ----------------------------------------------
    tcpSock.setblocking(False)
    tcpSock.settimeout(5)
    if seed == 0:
        tcp_txrx_ramp(tcpSock, message, count, args.verbose)
    else:
        tcp_txrx_loop(tcpSock, message, count, args.verbose)

# STEP-14: Close socket
# -----------------------
time.sleep(2)
tcpSock.close()
