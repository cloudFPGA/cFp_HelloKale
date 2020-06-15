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
import ipaddress
import os
import random
import requests
import socket
import string
import threading
import time

# Return codes for the function calls
ACM_OK = 0    # ACM = Accelerator Module (alias for FM = Fpga Module)
ACM_KO = 1

MTU         = 1500  # Maximum Transfer Unit
IP4_HDR_LEN =   20  # IPv4 Header Length
UDP_HDR_LEN =    8  # UDP Header Length
MAX_DGR_LEN = (MTU-IP4_HDR_LEN-UDP_HDR_LEN)


def num_to_char(num):
    """ Function to map a number to a character."""
    switcher = {
         0: '0',  1: '1',  2: '2',  3: '3',  4: '4',  5: '5',  6: '6',  7: '7',
         8: '8',  9: '9', 10: 'a', 11: 'b', 12: 'c', 13: 'd', 14: 'e', 15: 'f'
    }
    return switcher.get(num, ' ')  # ' ' is default

def str_static_gen(size):
    """Generates a static string with length >= 'size'."""
    msg = '           Hello World          '
    while (len(msg)) < (size-1):
        msg += num_to_char(len(msg) % 16)
    msg += '\n'
    return msg.encode()


def str_rand_gen(size):
    """Generates a random string of length 'size'."""
    msg = "".join(random.choice(string.ascii_lowercase + string.digits) for _ in range(size-1))
    msg += '\n'
    return msg.encode()


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
    :param instId:      The instance Id to restart (must be 1-32).
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
    rc = os.system("ping -c 2 " + str(ipFpga))
    if rc != 0:
        print("[ERROR] FPGA does not reply to \'ping\'!")
        exit(1)


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
udpDP = 8803     # 8803=0x2263
fpgaAssociation = (str(ipFpga), udpDP)

#  STEP-6b: Set the HOST socket association (optional)
#    Info: Linux selects a source port from an ephemeral port range, which by
#           default is a set to range from 32768 to 61000. You can check it
#           with the command:
#               > cat /proc/sys/net/ipv4/ip_local_port_range
#           If we want to force the source port ourselves, we must use the
#           "bind before connect" trick.
# -----------------------------------------------------------------------------
if 0:
    udpSP = udpDP + 49152  # 8803 + 0xC000
    hostAssociation = (ipSaStr, udpSP)

#OBSOLETE_20200601 #  STEP-7: Wait until the current socket can be reused
#OBSOLETE_20200601 # -----------------------------------------------------------------------------
#OBSOLETE_20200601 waitUntilSocketPairCanBeReused(ipFpga, portFpga)

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
size = random.randint(1, MAX_DGR_LEN)
if size > MAX_DGR_LEN:
    print('\nERROR: ')
    print("[ERROR] The UDP stack does not support the reception of datagrams larger than %d bytes.\n" % MAX_DGM_SIZE)
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

    if verbose:
        print("[INFO] The following message of %d bytes will be sent out %d times:\n  Message=%s\n" %
                                                          (len(message), count, message.decode()))
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

#  STEP-14: Close socket
# -----------------------
udpSock.close()


