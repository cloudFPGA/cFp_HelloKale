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
# * @file       : tc_TcpRecv.py
# * @brief      : A single-threaded script to receive traffic on the TCP
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

# ### REQUIRED TESTCASE MODULES ###############################################
from tc_utils import *


def tcp_rx_loop(clientSock, serverSock, size, activePort, count, verbose=False):
    """TCP Rx Single-Thread Ramp.
     Requests the FPGA to open a new active port and expects to receive 'count' segments
      of 'size' bytes. Each segment is made of the following repetitive pattern
      '48692066726f6d200x464d4b553630210a' which decodes into the string "Hi from FMKU60\n".
     :param clientSock The socket to send to.
     :param serverSock The socket to receive from.
     :param size       The size of the expected segment.
     :param activePort The active port number that the fPGA is requested to open.
     :param count      The number of segments to receive.
     :param verbose    Enables verbosity.
     :return           None"""
    if verbose:
        print("[INFO] Requesting the FPGA to send %d segments of %d bytes on TCP port number %d.\n" % (count, size, activePort))
    nrErr = 0
    loop = 0
    totalReceivedBytes = 0

    # Set the server socket non-blocking
    # --------------------------------------
    serverSock.setblocking(False)
    serverSock.settimeout(5)

    # case
    # RXP_SEND_8801: // FORWARD
    # TCP
    # PORT
    # AND
    # TX
    # DATA
    # LENGTH
    # REQUEST
    # TO[TSIF]
    # if (!soTSIF_Data.full()) {
    # printInfo(myRxpName, "Requesting Tx test mode to generate a segment of length=%d and to send it to port=%d.\n",
    # toe_txByteCnt.to_int(), testPort);
    # appData.setLE_TData(byteSwap16(testPort), 15, 0);
    # appData.setLE_TData(byteSwap16(toe_txByteCnt), 31, 16);

    # Turn the size into an unsigned short binary data for transmission
    reqMsgAsBytes = struct.pack(">HH", activePort, size)
    print("\n\n>>> reqMsgAsBytes = %s" % reqMsgAsBytes)

    startTime = datetime.datetime.now()
    while loop < count:
            # SEND message length request to FPGA
            # ------------------------------------
            try:
                clientSock.sendall(reqMsgAsBytes)
            except socket.error as exception:
                # Any exception
                print("[EXCEPTION] Socket error while transmitting :: %s" % exception)
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
                        print('[ERROR] Socket reading error: {}'.format(str(e)))
                        exit(1)
                    # We just did not receive anything
                    continue
                except socket.error as exc:
                    # Any other exception
                    print("[EXCEPTION] Socket error while receiving :: %s" % exc)
                    exit(1)
                else:
                    currRxByteCnt += len(data)
                    if verbose:
                        print("Loop=%d | RxBytes=%d | RxData=%s\n" % (loop, len(data), data))
                        # [FIXME-TODO: assess the stream of received bytes]
                finally:
                    pass
            totalReceivedBytes += currRxByteCnt;
            loop += 1
    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime;

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
        print("#### TCP Rx DONE with bandwidth = %6.1f Mb/s ####" % bandwidth)
    else:
        bandwidth = bandwidth / 1000
        print("#### TCP Rx DONE with bandwidth = %2.1f Gb/s ####" % bandwidth)
    print("#####################################################")
    print()


###############################################################################
#                                                                             #
#                                 MAIN                                        #
#                                                                             #
###############################################################################

#  STEP-1: Parse the command line strings into Python objects
# -----------------------------------------------------------------------------
parser = argparse.ArgumentParser(description='A script to receive TCP data from an FPGA module.')
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

#  STEP-3a: Set the TCP listen port of the FPGA server (this one is static)
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

#  STEP-6a: Set the socket association for sending to the FPGA server
# -----------------------------------------------------------------------------
fpgaServerAssociation = (str(ipFpga), portFpgaServer)

#  STEP-6b: Set the socket association for receiving from the FPGA client
# -----------------------------------------------------------------------------
portFpgaClient = portFpgaServer + 0x8000  # By default for this test-case
fpgaClientAssociation = (str(ipFpga), portFpgaClient)

#  STEP-7a: Create a TCP/IP client socket for sending to FPGA server
# -----------------------------------------------------------------------------
try:
    tcpClientSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except Exception as exc:
    print("[EXCEPTION] %s" % exc)
    exit(1)

#  STEP-7b: Create a TCP/IP server socket for listening to FPGA client
# -----------------------------------------------------------------------------
try:
    tcpSeverSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except Exception as exc:
    print("[EXCEPTION] %s" % exc)
    exit(1)

# STEP-8a: Bind the client socket before connect (optional).
#  This trick enables us to ask the kernel to select a specific source IP and
#  source PORT by calling bind() before calling connect().
# -----------------------------------------------------------------------------
if 0:
    try:
        tcpClientSock.bind(hostClientAssociation)
        print('Binding the socket address of the HOST to {%s, %d}' % hostClientAssociation)
    except Exception as exc:
        print("[EXCEPTION] %s" % exc)
        exit(1)

# STEP-8b: Bind the server socket before connect (compulsory).
#  Issued here to associate the server socket with a specific remote IP address and TCP port.
# -----------------------------------------------------------------------------
try:
    tcpServerSock.bind(fpgaClientAssociation)
    print('Binding the socket address of the HOST to {%s, %d}' % fpgaClientAssociation)
except Exception as exc:
    print("[EXCEPTION] %s" % exc)
    exit(1)

#  STEP-9a: Connect the host client to the remote FPGA server.
# -----------------------------------------------------------------------------
try:
    tcpClientSock.connect(fpgaServerAssociation)
except Exception as exc:
    print("[EXCEPTION] %s" % exc)
    exit(1)
else:
    print('\nSuccessful client connection with socket address of FPGA server at {%s, %d} \n' % fpgaServerAssociation)

# STEP-9b: Enable the host server to to accept connections from remote FPGA client.
# -----------------------------------------------------------------------------
tcpServerSock.listen()
print('Host server ready to start listening on socket address {%s, %d} \n' % fpgaClientAssociation)
tcpServerSock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

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
    size = random.randint(1, 0xFFFF)
elif size > 0xFFFF:
    print('\nERROR: ')
    print("[ERROR] This test limits the size of the received segments to %d bytes.\n" % 0xFFFF)
    exit(1)
print("\t\t size = %d" % size)

count = args.loop_count
print("\t\t loop = %d" % count)

verbose = args.verbose

#  STEP-11: Run the test
# -------------------------------
print("[INFO] This run is executed in single-threading mode.\n")
tcp_rx_loop(tcpClientSock, tcpServerSock, size, portFpgaClient, count, args.verbose)

#  STEP-14: Close sockets
# -----------------------
tcpClientSock.close()
tcpServerSock.close()


