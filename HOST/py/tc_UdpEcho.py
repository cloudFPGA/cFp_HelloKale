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
# * @file       : tc_UdpEcho.py
# * @brief      : A multi-threaded script to send and receive traffic on the
# *               UDP connection of an FPGA module.
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

# ### REQUIRED TESTCASE MODULES ###############################################
from tc_utils import *


def udp_tx(sock, message, count, lock, verbose=False):
    """UDP Tx Thread.
    :param sock     The socket to send to.
    :param message  The message string to sent.
    :param count    The number of datagrams to send.
    :param lock     A semaphore to access the global variable 'gBytesInFlight'.
    :param verbose  Enables verbosity.
    :return         None"""
    global gBytesInFlight

    if verbose:
        print("The following message of %d bytes will be sent out %d times:\n  Message=%s\n" %
                                                                (len(message), count, message.decode()))
    loop = 0
    startTime = datetime.datetime.now()
    while loop < count:
        if gBytesInFlight < 4096:
            # FYI - UDP does not provide any flow-control.
            #   If we push bytes into the FPGA faster than we drain, some bytes might be dropped.
            try:
                sock.sendall(message)
            except socket.error as exc:
                # Any exception
                print("[EXCEPTION] Socket error while receiving :: %s" % exc)
                exit(1)
            else:
                lock.acquire()
                gBytesInFlight += len(message)
                # print("TX - %d" % gBytesInFlight)
                lock.release()
            loop += 1
    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime;
    bandwidth  = len(message) * 8 * count * 1.0 / (elapseTime.total_seconds() * 1024 * 1024)
    print("[INFO] Sent a total of     %d bytes." % (count*len(message)))
    print("##################################################")
    print("#### UDP TX DONE with bandwidth = %6.1f Mb/s ####" % bandwidth)
    print("##################################################")
    print()


def udp_rx(sock, message, count, lock, verbose=False):
    """UDP Rx Thread.
     :param sock        The socket to receive from.
     :param message     The expected string message to be received.
     :param count       The number of datagrams to receive.
     :param lock     A semaphore to access the global variable 'gBytesInFlight'.
     :param verbose,    Enables verbosity.
     :return            None"""
    global gBytesInFlight

    loop = 0
    rxBytes = 0
    nrErr = 0
    startTime = datetime.datetime.now()
    while rxBytes < count*len(message):
        try:
            data = sock.recv(len(message))
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
            lock.acquire()
            gBytesInFlight -= len(data)
            # print("RX - %d" % gBytesInFlight)
            lock.release()
            rxBytes += len(data)
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
    print("[INFO] Received a total of %d bytes." % rxBytes)
    print("##################################################")
    print("#### UDP RX DONE with bandwidth = %6.1f Mb/s ####" % bandwidth)
    print("##################################################")
    print()


def udp_txrx_loop(sock, message, count, verbose=False):
    """UDP Tx-Rx Single-Thread Loop.
     :param sock     The socket to send/receive to/from.
     :param message  The message string to sent.
     :param count    The number of datagrams to send.
     :param verbose  Enables verbosity.
     :return         None"""
    if verbose:
        print("[INFO] The following message of %d bytes will be sent out %d times:\n  Message=%s\n" %
              (len(message), count, message.decode()))
    nrErr = 0
    loop = 0
    rxByteCnt = 0
    startTime = datetime.datetime.now()
    while loop < count:
        #  Send datagram
        # -------------------
        try:
            udpSock.sendall(message)
        finally:
            pass
        #  Receive datagram
        # -------------------
        try:
            data = udpSock.recv(len(message))
            rxByteCnt += len(data)
            if data == message:
                if verbose:
                    print("Loop=%d | RxBytes=%d" % (loop, rxByteCnt))
            else:
                print("Loop=%d | RxBytes=%d" % (loop, rxByteCnt))
                print(" KO | Received  Message=%s" % data.decode())
                print("    | Expecting Message=%s" % message)
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
        loop += 1
    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime;
    bandwidth  = len(message) * 8 * count * 1.0 / (elapseTime.total_seconds() * 1024 * 1024)
    print("[INFO] Transferred a total of %d bytes." % rxByteCnt)
    print("#####################################################")
    print("#### UDP Tx/Rx DONE with bandwidth = %6.1f Mb/s ####" % bandwidth)
    print("#####################################################")
    print()


def udp_txrx_ramp(sock, message, count, verbose=False):
    """UDP Tx-Rx Single-Thread Ramp.
     :param sock     The socket to send/receive to/from.
     :param message  The message string to sent.
     :param count    The number of datagrams to send.
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
        i = 1
        while i <= len(message):
            subMsg = message[0:i]

            #  Send datagram
            # -------------------
            try:
                udpSock.sendall(subMsg)
            finally:
                pass
            #  Receive datagram
            # -------------------
            try:
                data = udpSock.recv(len(subMsg))
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
    elapseTime = endTime - startTime;
    bandwidth  = (rxByteCnt * 8 * count * 1.0) / (elapseTime.total_seconds() * 1024 * 1024)
    megaBytes  = (rxByteCnt * 1.0) / (1024 * 1024 * 1.0)
    print("[INFO] Transferred a total of %.1f MB." % megaBytes)
    print("#####################################################")
    print("#### UDP Tx/Rx DONE with bandwidth = %6.1f Mb/s ####" % bandwidth)
    print("#####################################################")
    print()


###############################################################################
#                                                                             #
#                                 MAIN                                        #
#                                                                             #
###############################################################################

#  STEP-1: Parse the command line strings into Python objects
# -----------------------------------------------------------------------------
parser = argparse.ArgumentParser(description='A script to send/receive UDP data to/from an FPGA module.')
parser.add_argument('-fi', '--fpga_ipv4',   type=str, default='',
                           help='The IPv4 address of the FPGA (a.k.a image_ip / e.g. 10.12.200.163)')
parser.add_argument('-fp', '--fpga_port',   type=int, default=8803,
                           help='The UDP  port of the FPGA (default is 8803)')
parser.add_argument('-ii', '--inst_id',     type=int, default=0,
                           help='The instance ID assigned by the cloudFPGA Resource Manager (e.g. 42)')
parser.add_argument('-lc', '--loop_count',  type=int, default=10,
                           help='The number of test runs (default is 10)')
parser.add_argument('-mi', '--mngr_ipv4',   type=str, default='10.12.0.132',
                           help='The IPv4 address of the cloudFPGA Resource Manager (default is 10.12.0.132)')
parser.add_argument('-mp', '--mngr_port',   type=int, default=8080,
                           help='The TCP port of the cloudFPGA Resource Manager (default is 8080)')
parser.add_argument('-mt', '--multi_threading',       action="store_true",
                           help='Enable multi_threading')
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

#  STEP-3a: Retrieve the UDP listen port of the FPGA server
# -----------------------------------------------------------------------------
portFpga = getFpgaPort(args)

#  STEP-3b: Retrieve the TCP port of the cloudFPGA Resource Manager
# -----------------------------------------------------------------------------
portResMngr = getResourceManagerPort(args)

#  STEP-?: Configure the application registers
# -----------------------------------------------------------------------------
# TODO print("\nNow: Configuring the application registers.")
# TODO udpEchoPathThruMode = (0x0 << 0)  # See DIAG_CTRL_2 register

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


# [FIXME - Disable the IP fragmentation via setsockopt()]
#   See for also: 
#     https://stackoverflow.com/questions/973439/how-to-set-the-dont-fragment-df-flag-on-a-socket
#     https://stackoverflow.com/questions/26440761/why-isnt-dont-fragment-flag-getting-disabled


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

if seed % 1:
    message = str_static_gen(size)
else:
    message = str_rand_gen(size)

verbose = args.verbose

if args.multi_threading:
    print("[INFO] This run is executed in multi-threading mode.\n")
    # Global variable
    gBytesInFlight = 0
    # Creating a lock
    lock = threading.Lock()
    #  STEP-11: Create Rx and Tx threads
    # -----------------------------------
    tx_thread = threading.Thread(target=udp_tx, args=(udpSock, message, count, lock, args.verbose))
    rx_thread = threading.Thread(target=udp_rx, args=(udpSock, message, count, lock, args.verbose))
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
    if seed == 0:
        udp_txrx_ramp(udpSock, message, count, args.verbose)
    else:
        udp_txrx_loop(udpSock, message, count, args.verbose)

#  STEP-14: Close socket
# -----------------------
udpSock.close()


