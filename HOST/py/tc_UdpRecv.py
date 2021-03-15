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
import struct
import netifaces as ni
import time

# ### REQUIRED TESTCASE MODULES ###############################################
from tc_utils import *


def udp_rx_loop(clientSock, serverSock, size, ip_da, udp_dp, count, verbose=False):
    """UDP Rx Single-Thread Ramp.
    Requests the FPGA to open a new active port and expects to receive 'count' datagrams
      of 'size' bytes. Each datagram is made of the following repetitive pattern
      '48692066726f6d200x464d4b553630210a' which decodes into the string "Hi from FMKU60\n".
    :param clientSock The socket to send to.
    :param serverSock The socket to receive from.
    :param size       The size of the expected segment.
    :param ip_da      The destination address of the host.
    :param udp_dp     The active destination port number that the FPGA is requested to open.
    :param count      The number of datagrams to receive.
    :param verbose    Enables verbosity.
    :return           None"""
    if verbose:
        print("[INFO] Requesting the FPGA to send %d datagrams of %d bytes.\n" % (count, size))
    nrErr = 0
    loop = 0
    totalReceivedBytes = 0

    #  Set the server socket non-blocking
    # --------------------------------------
    serverSock.setblocking(False)
    serverSock.settimeout(5)

    # Request the test to generate a datagram of length='size' and to send it to
    # socket={ip_da, tcp_dp}. This is done by sending 'ip_da', 'tcp_dp' and 'size'
    # to the FPGA UDP_DP=8801 while turning the 'ip_da' into an unsigned int binary
    # and 'tcp_dp' and 'size' into an unsigned short binary data.
    reqMsgAsBytes = struct.pack(">IHH", ip_da, udp_dp, size)
    if verbose:
        print("\n\n[DEBUG] reqMsgAsBytes = %s" % reqMsgAsBytes)

    startTime = datetime.datetime.now()
    while loop < count:
            # SEND message length request to FPGA
            # ------------------------------------
            try:
                clientSock.sendall(reqMsgAsBytes)
            except socket.error as exc:
                # Any exception
                print("[EXCEPTION] Socket error while transmitting :: %s" % exc)
                exit(1)
            finally:
                pass

            # RECEIVE message length bytes from FPGA
            # ---------------------------------------
            currRxByteCnt = 0
            while currRxByteCnt < size:
                try:
                    data = serverSock.recv(MTU)
                except IOError as e:
                    # On non blocking connections - when there are no incoming data, error is going to be raised
                    # Some operating systems will indicate that using AGAIN, and some using WOULDBLOCK error code
                    # We are going to check for both - if one of them - that's expected, means no incoming data,
                    # continue as normal. If we got different error code - something happened
                    if e.errno != errno.EAGAIN and e.errno != errno.EWOULDBLOCK:
                        print("[ERROR] Socket reading error: {}".format(str(e)))
                    # We just did not receive anything
                    print("\t[INFO] So far we received %d bytes out of %d." % (currRxByteCnt, size))
                    count = loop
                    break
                except socket.error as exc:
                    # Any other exception
                    print("[EXCEPTION] Socket error while receiving :: %s" % exc)
                    exit(1)
                else:
                    currRxByteCnt += len(data)
                    if verbose:
                        print("[INFO] Loop=%d | RxBytes=%d | RxData=%s\n" % (loop, len(data), data))
                        # [FIXME-TODO: assess the stream of received bytes]
                finally:
                    pass
            totalReceivedBytes += currRxByteCnt;
            loop += 1
    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime

    if totalReceivedBytes < 1000000:
        print("[INFO] Received a total of %d bytes." % totalReceivedBytes)
    elif totalReceivedBytes < 1000000000:
        megaBytes = (totalReceivedBytes * 1.0) / (1024 * 1024 * 1.0)
        print("[INFO] Received a total of %.1f MB." % megaBytes)
    else:
        gigaBytes = (totalReceivedBytes * 1.0) / (1024 * 1024 * 1024 * 1.0)
        print("[INFO] Transferred a total of %.1f GB." % gigaBytes)

    bandwidth = (totalReceivedBytes * 8 * 1.0) / (elapseTime.total_seconds() * 1024 * 1024)
    print("#####################################################")
    if bandwidth < 1000:
        print("#### UDP Rx DONE with bandwidth = %6.1f Mb/s " % bandwidth)
    else:
        bandwidth = bandwidth / 1000
        print("#### UDP Rx DONE with bandwidth = %2.1f Gb/s " % bandwidth)
    if (totalReceivedBytes != (size * count)):
        print("####  [WARNING] UDP data loss = %.1f%%" % (1-(totalReceivedBytes)/(size*count)))
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
    print("\n[WARNING] You must provide a ZYC2 user name and the corresponding password for this script to execute.\n")
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
portFpgaServer = XMIT_MODE_LSN_PORT  # 8801

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
fpgaServerAssociation = (str(ipFpga), portFpgaServer)

#  STEP-6b: Set the socket association for receiving from the FPGA client
# -----------------------------------------------------------------------------
dpHost = 2718  # Default UDP cF-Themisto ports are in range 2718-2750
fpgaClientAssociation = (str(ipFpga), dpHost)

#  STEP-7a: Create a UDP/IP client socket for sending to FPGA server
# -----------------------------------------------------------------------------
try:
    udpClientSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
except Exception as exc:
    print("[EXCEPTION] %s" % exc)
    exit(1)

#  STEP-7b: Create a UDP/IP server socket for listening to FPGA client
# -----------------------------------------------------------------------------
try:
    udpServerSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
except Exception as exc:
    print("[EXCEPTION] %s" % exc)
    exit(1)

# STEP-8a: Bind the client socket before connect (optional).
#  This trick enables us to ask the kernel to select a specific source IP and
#  source PORT by calling bind() before calling connect().
# -----------------------------------------------------------------------------
if 0:
    try:
        udpClientSock.bind(hostClientAssociation)
        print('[INFO] Binding the socket address of the HOST to {%s, %d}' % hostClientAssociation)
    except Exception as exc:
        print("[EXCEPTION] %s" % exc)
        exit(1)

# STEP-8b: Bind the server socket before connect (compulsory).
#  Issued here to associate the server socket with a specific remote IP address and UDP port.
# -----------------------------------------------------------------------------
hostname  = socket.gethostname()
ipHostStr = socket.gethostbyname(hostname)
if ipHostStr.startswith('9.4.'):
    # Search the IP address of the OpenVPN tunnel (FYI: the user-VPN always starts with 10.2.X.X)
    for itf in ni.interfaces():
        # DEBUG print("[INFO] Itf=%s " % itf)
        if itf.startswith('tun'):
            ip4Str = ni.ifaddresses(itf)[AF_INET][0]['addr']
            if ip4Str.startswith('10.2.'):
                ipHostStr = ip4Str
                break
            else:
                ipHostStr = ""
    if ipHostStr == "":
        print("[ERROR] Could not find IPv4 address of the tunnel associated with the user-VPN.\n")
        exit(1)
ipHost = int(ipaddress.IPv4Address(ipHostStr))
if args.verbose:
    print("[INFO] Hostname = %s | IP is %s (0x%8.8X) \n" % (hostname, ipHostStr, ipHost))

try:
    udpServerSock.bind((ipHostStr, dpHost))
    print('[INFO] Binding the socket address of the HOST to {%s, %d}' % (ipHostStr, dpHost))
except Exception as exc:
    print("[EXCEPTION] %s" % exc)
    exit(1)

#  STEP-9a: Connect the host client to the remote FPGA server.
#   Info: Although UDP is connectionless, 'connect()' might still be called. This enables
#         the OS kernel to set the default destination address for the send, whick makes it
#         faster to send a message.
# -----------------------------------------------------------------------------
try:
    udpClientSock.connect(fpgaServerAssociation)
except Exception as exc:
    print("[EXCEPTION] %s" % exc)
    exit(1)
else:
    print('[INFO] Successful connection with socket address of FPGA at {%s, %d}' % fpgaServerAssociation)

# STEP-9b: Enable the host server to to accept connections from remote FPGA client.
# -----------------------------------------------------------------------------
print('[INFO] Host server ready to start listening on socket address {%s, %d}' % fpgaClientAssociation)

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
    size = random.randint(1, 0xFFFF)
elif size > 0xFFFF:
    print('\nERROR: ')
    print("[ERROR] This test limits the size of the received datagram to %d bytes.\n" % 0xFFFF)
    exit(1)
print("\t\t size = %d" % size)

count = args.loop_count
print("\t\t loop = %d" % count)

#  STEP-11: Run the test
# -------------------------------
print("[INFO] This testcase is sending traffic from FPGA-to-HOST.")
print("[INFO] This run is executed in single-threading mode.\n")
udp_rx_loop(udpClientSock, udpServerSock, size, ipHost, dpHost, count, args.verbose)

#  STEP-14: Close sockets
# -----------------------
time.sleep(2)
udpClientSock.close()
udpServerSock.close()



