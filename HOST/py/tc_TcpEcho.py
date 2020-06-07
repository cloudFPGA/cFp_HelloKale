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


def tcp_tx(sock, message, count, verbose=False):
    """TCP Tx Thread.
    :param sock,       the socket to send to.
    :param message,    the random string to sent.
    :param count,      the number of segments to send.
    :param verbose,    enables verbosity.
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
    print()


def tcp_rx(sock, message, count, verbose):
    """TCP Rx Thread.
     :param sock,       the socket to receive from.
     :param message,    the expected string message to be received.
     :param count,      the number of segment to receive.
     :param verbose,    enables verbosity.
     :return None"""
    loop = 0
    byteCnt = 0
    nrErr = 0
    startTime = datetime.datetime.now()
    while byteCnt < count*len(message):
        try:
            data = sock.recv(len(message))
            byteCnt += len(data)
            if verbose:
                print("Loop=%d | RxBytes=%d" % (loop, byteCnt))
                # if data == message:
                #     print("Loop=%d | RxBytes=%d" % (loop, byteCnt))
                # else:
                #    print(" KO | Received  Message=%s" % data.decode())
                #     print("    | Expecting Message=%s" % message)
                #     nrErr += 1
        except socket.error as exc:
            print("[EXCEPTION] Socket error while receiving :: %s" % exc)
        finally:
            pass
        loop += 1
    endTime = datetime.datetime.now()
    elapseTime = endTime - startTime;
    bandwidth  = len(message) * 8 * count * 1.0 / (elapseTime.total_seconds() * 1024 * 1024)
    print("##################################################")
    print("#### TCP RX DONE with bandwidth = %6.1f Mb/s ####" % bandwidth)
    print("##################################################")
    print()


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


def getFpgaPort(args):
    """Retrieve the TCP destination port of the FPGA.
    :param args, the options passed as arguments to the script.
    :return the TCP port number as an integer."""
    portFpga = args.fpga_port
    if portFpga != 8803:
        print("[ERROR] The current version of the cFp_BringUp role always listens on port #8803.\n")
        exit(1)
    return portFpga


def getResourceManagerIpv4(args):
    """Retrieve the IP address of the cloudFPGA Resource Manager.
    :param args, the options passed as arguments to the script.
    :return the IP address as an ipaddress.IPv4Address."""
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
    :param args, the options passed as arguments to the script.
    :return the TCP port number as an integer."""
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
    :param instId:      the instance Id to restart (must be 1-32).
    :param ipResMngr:   the IPv4 address of the cF resource manager.
    :param portResMngr: the TCP port number of the cF resource manager.
    :param user_name:   the user name as used to log in ZYC2.
    :param user_passwd: the ZYC2 password attached to the user name.
    :return: nothing
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
    :param ipFpga: the IPv4 address of the FPGA.
    :return: nothing
    """
    print("Now: Trying to \'ping\' the FPGA: ")
    rc = os.system("ping -c 2 " + str(ipFpga))
    if rc != 0:
        print("[ERROR] FPGA does not reply to \'ping\'!")
        exit(1)


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
# -----------------------------------------------------------------------------
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
waitUntilSocketPairCanBeReused(ipFpga, portFpga)

#  STEP-8a: Create a TCP/IP socket for the TCP/IP connection
# -----------------------------------------------------------------------------
try:
    tcpSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except Exception as exc:
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

#  STEP-10: Run the test
# -------------------------------
size = 128
count = args.loop_count

if args.verbose:
    message = str_static_gen(size)
else:
    message = str_rand_gen(size)

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

#  STEP-14: Close socket
# -----------------------
tcpSock.close()
