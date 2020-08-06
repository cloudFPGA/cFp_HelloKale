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
# * @file       : tc_UdpRecv.py
# * @brief      : A single-threaded script to receive traffic on the UDP
# *               connection of an FPGA module.
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

# ### REQUIRED TESTCASE MODULES ###############################################
from tc_utils import *

def udp_rx_loop(udpSock, size, count, verbose=False):
    """UDP Rx Single-Thread Ramp.
     Expects to receive 'count' datagrams of 'size' bytes. Each datagram is made
     of the following repetitive pattern '000102030405060708090A0B0C0D0E0F'.
     :param sock     The socket to receive from.
     :param size     The size of the expected datagram.
     :param count    The number of datagrams to receive.
     :param verbose  Enables verbosity.
     :return         None"""
    if verbose:
        print("[INFO] Requesting the FPGA to send %d datagrams of %d bytes.\n", (count, size))
    nrErr = 0
    loop = 0
    rxByteCnt = 0
    requestMsg = '{:04X}'.format(size)

    startTime = datetime.datetime.now()
    while loop < count:
            # SEND message length request to FPGA
            # ------------------------------------
            try:
                udpSock.sendall(requestMsg)
            except socket.error as exc:
                # Any exception
                print("[EXCEPTION] Socket error while transmitting :: %s" % exc)
                exit(1)
            finally:
                pass
            # RECEIVE message length bytes from FPGA
            # ---------------------------------------
            try:
                data = udpSock.recv(size)
                rxByteCnt += len(data)
                if verbose:
                    print("Loop=%d | RxBytes=%d | RxData=%s" % (loop, rxByteCnt, data))
                    # [FIXME-TODO: assess the stream of received bytes]
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
            loop += 1
    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime;

    if rxByteCnt < 1000000:
        print("[INFO] Received a total of %d bytes." % rxByteCnt)
    elif rxByteCnt < 1000000000:
        megaBytes = (rxByteCnt * 1.0) / (1024 * 1024 * 1.0)
        print("[INFO] Received a total of %.1f MB." % megaBytes)
    else:
        gigaBytes = (rxByteCnt * 1.0) / (1024 * 1024 * 1024 * 1.0)
        print("[INFO] Transferred a total of %.1f GB." % gigaBytes)

    bandwidth = (rxByteCnt * 8 * 1.0) / (elapseTime.total_seconds() * 1024 * 1024)
    print("#####################################################")
    if bandwidth < 1000:
        print("#### UDP Rx DONE with bandwidth = %6.1f Mb/s ####" % bandwidth)
    else:
        bandwidth = bandwidth / 1000
        print("#### UDP Rx DONE with bandwidth = %2.1f Gb/s ####" % bandwidth)
    print("#####################################################")
    print()


###############################################################################
#                                                                             #
#                                 MAIN                                        #
#                                                                             #
###############################################################################

#  STEP-1: Parse the command line strings into Python objects
# -----------------------------------------------------------------------------
parser = argparse.ArgumentParser(description='A script to receive UDP data from an FPGA module.')
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
                           help='The size of the datagram to receive.')
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

#  STEP-3a: Set the UDP listen port of the FPGA server (this one is static)
# -----------------------------------------------------------------------------
portFpga = XMIT_MODE_LSN_PORT  # 8801

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
    udpSP = portFpga + 49152  # 8803 + 0xC000
    hostAssociation = (ipSaStr, udpSP)

#  STEP-8a: Create a UDP/IP socket
# -----------------------------------------------------------------------------
try:
    udpSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
except Exception as exc:
    print("[EXCEPTION] %s" % exc)
    exit(1)

# STEP-8b: Bind before connect (optional).
#  This trick enables us to ask the kernel to select a specific source IP and
#  source PORT by calling bind() before calling connect().
# -----------------------------------------------------------------------------
if 0:
    try:
        udpSock.bind(hostAssociation)
        print('Binding the socket address of the HOST to {%s, %d}' % hostAssociation)
    except Exception as exc:
        print("[EXCEPTION] %s" % exc)
        exit(1)

#  STEP-9a: Connect to the remote FPGA
#   Info: Although UDP is connectionless, 'connect()' might still be called. This enables
#         the OS kernel to set the default destination address for the send, whick makes it
#         faster to send a message.
# -----------------------------------------------------------------------------
try:
    udpSock.connect(fpgaAssociation)
except Exception as exc:
    print("[EXCEPTION] %s" % exc)
    exit(1)
else:
    print('\nSuccessful connection with socket address of FPGA at {%s, %d} \n' % fpgaAssociation)

#  STEP-9b: Set the socket non-blocking
# --------------------------------------
udpSock.setblocking(False)
udpSock.settimeout(5)

#  STEP-10: Setup the test
# -------------------------------
seed = args.seed
if seed == -1:
    seed = random.randint(0, 100000)
random.seed(seed)
print("[INFO] This testcase is run with:")
print("\t\t seed = %d" % seed)

size = args.size
if size == -1:
    size = random.randint(1, MAX_DGR_LEN)
elif size > MAX_DGR_LEN:
    print('\nERROR: ')
    print("[ERROR] The UDP stack does not support the reception of datagrams larger than %d bytes.\n" % MAX_DGR_LEN)
    exit(1)
print("\t\t size = %d" % size)

count = args.loop_count
print("\t\t loop = %d" % count)

verbose = args.verbose

#  STEP-11: Run the test
# -------------------------------
print("[INFO] This run is executed in single-threading mode.\n")
udp_rx_ramp(udpSock, size, count, args.verbose)

#  STEP-14: Close socket
# -----------------------
udpSock.close()


