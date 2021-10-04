#!/bin/bash

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
# * @file       : runTcpReg.sh
# * @brief      : A shell script to run a suite of TCP tests between the host
# *               and an FPGA instance.
# *
# * System:     : cloudFPGA
# * Component   : cFp_BringUp/ROLE
# * Language    : bash
# *
# * Synopsis    : ./runTcpReg.sh -h|--HELP
# *
# * Info/Note   : You may need to install 'jq', is a lightweight and flexible
# *                 command-line JSON processor. 
# *****************************************************************************

# -----------------------------------------------------------------------------
#  CONSTANTS (Do not touch)
# -----------------------------------------------------------------------------
cZYC2_MSS=1352
cIPv4_RES_MNGR="10.12.0.132"
cPORT_RES_MNGR="8080"

# -----------------------------------------------------------------------------
#  GLOBAL VARIABLES INITIALIZATION
# -----------------------------------------------------------------------------
ERRORS=0
LOOP=1
INSTANCE_IP=""
CURL_REPLY=""

# -----------------------------------------------------------------------------
#  GLOBAL VARIABLES THAT MIGHT BE PASSED AS ARGUMENT
# -----------------------------------------------------------------------------
INSTANCE_ID=""
USER_NAME=""
USER_PASS=""
MAX_ERRORS=1
RUN_SEND=false
RUN_RECV=false
RUN_ECHO=false
RUN_IPERF=false
VERBOSE=false

#===============================================================================
# FUNCTION - Display Help
#===============================================================================
fDisplayHelp() {
   echo -e "NAME\n\t ${0}  A script to run a suite of TCP tests between the host and an FPGA instance."  
   echo -e "USAGE\n\t ${0} [-h|--HELP] [-ii|--INSTANCE_ID] [-un|--USER_NAME] [-up|--USER_PASSWD] [--ECHO] [--IPERF] [--RECV] [--SEND]"
   echo -e "OPTIONS"
   echo -e "\t[-h |--HELP]        Prints this and exits"
   echo -e "\t[-ii|--INSTANCE_ID] The id of the FPGA instance to test (e.g. 42)"
   echo -e "\t[-un|--USER_NAME]   A user-name as used to log in ZYC2 (e.g. \'fab\')" 
   echo -e "\t[-up|--USER_PASSWD] The ZYC2 password attached to the user-name"
   echo -e "\t[    --ECHO]        Sets the echo    test mode to \'true\' (default is false)"
   echo -e "\t[    --IPERF]       Sets the iperf   test mode to \'true\' (default is false)"
   echo -e "\t[    --RECV]        Sets the receive test mode to \'true\' (default is false)"
   echo -e "\t[    --SEND]        Sets the send    test mode to \'true\' (default is false)"
   echo -e "\t[-v |--VERBOSE]     Enable verbose mode (default is false)"
}

#===============================================================================
# FUNCTION - fRetreiveInstanceIp - Retrieves the IP address of the FPGA instance.
#  @param[in]  $1 The INSTANCE_ID number.
#  @param[in]  $2 The ZYC2_USER name.
#  @param[in]  $3 The ZYC2_PASS word. 
#
#  @param[out]    Set the global variable ${INSTANCE_IP}
#  @returns       0 if OK, otherwise 1.
#===============================================================================
fRetrieveInstanceIp() {
    instance=$1; if [ "${VERBOSE}" = true ]; then echo -e "<V> instance=$1"; fi
    username=$2; if [ "${VERBOSE}" = true ]; then echo -e "<V> username=$2"; fi
    userpass=$3; if [ "${VERBOSE}" = true ]; then echo -e "<V> userpass=$3"; fi

    reqUrl="http://"${cIPv4_RES_MNGR}":"${cPORT_RES_MNGR}"/instances/"${instance}"?username="${username}'&password='${userpass}
    if [ "${VERBOSE}" = true ]; then echo -e "<V> reqURL=${reqUrl}"; fi
    curl_reply=$(curl -X GET --header 'Accept: application/json' "${reqUrl}")
    if [ "${VERBOSE}" = true ]; then echo -e "<V> curl_reply=${curl_reply}"; fi

    INSTANCE_IP=$(echo ${curl_reply} | jq .role_ip)
    INSTANCE_IP="${INSTANCE_IP%\"}"
    INSTANCE_IP="${INSTANCE_IP#\"}"
    if [ "${VERBOSE}" = true ]; then echo -e "<V> INSTANCE_IP=${INSTANCE_IP}"; fi
}


###############################################################################
#                                                                             #
#                                 MAIN                                        #
#                                                                             #
###############################################################################

echo -e "\n###  THIS SCRIPT IS RUN WITH THE FOLLOWING PARAMETERS ###############" 

# -----------------------------------------------------------------------------
#  STEP-1.0 : Parse the command line strings into bash objects 
# -----------------------------------------------------------------------------
while [[ $# -gt 0 ]]
do 
    arg="$1"
    case ${arg} in
        -h | --HELP)
            echo -e "${arg}"; fDisplayHelp; exit;
            ;;
        -ii | --INSTANCE_ID)
            INSTANCE_ID="$2"; echo -e "INSTANCE_ID = ${INSTANCE_ID}"; shift; shift
            ;;
        -ma | --MAX_ERRORS)
            MAX_ERRORS="$2";  echo -e "MAX_ERRORS  = ${MAX_ERRORS}";  shift; shift
            ;;
        -un | --USER_NAME)
            ZYC2_USER="$2";   echo -e "ZYC2_USER   = ${ZYC2_USER}";   shift; shift
            ;;
        -up | --USER_PASS)
            ZYC2_PASS="$2";  
            HIDE_PASS=""; 
            for (( i=1; i<=${#ZYC2_PASS}; i++ )); do 
                HIDE_PASS+="*"; 
            done;             echo -e "ZYC2_PASS   = ${HIDE_PASS}";   shift; shift
            ;;
        --ECHO)
            RUN_ECHO=true;    echo -e "TCP_ECHO    = true";           shift
            ;;
        --IPERF)
            RUN_IPERF=true;   echo -e "TCP_IPERF   = true";           shift
            ;;
        --RECV)
            RUN_RECV=true;    echo -e "TCP_RECV    = true";           shift
            ;;
        --SEND)
            RUN_SEND=true;    echo -e "TCP_SEND    = true";           shift
            ;;
        -v | --VERBOSE)
            VERBOSE=true;     echo -e "VERBOSE     = true";           shift
            ;;
        *)  # Unknown option
            echo -e "\nERROR - Unknown option:  ${arg}\n"; fDisplayHelp; exit 1
            ;;
    esac
done

# -----------------------------------------------------------------------------
#  STEP-1.1 : Assess the variables that must be set 
# -----------------------------------------------------------------------------
if test -z "${INSTANCE_ID}"; then echo -e "\nERROR: Missing parameter: -ii | --INSTANCE_ID"; fDisplayHelp; exit 1; fi
if test -z "${ZYC2_USER}";   then echo -e "\nERROR: Missing parameter: -un | --USER_NAME";   fDisplayHelp; exit 1; fi
if test -z "${ZYC2_PASS}";   then echo -e "\nERROR: Missing parameter: -up | --USR_PASS";    fDisplayHelp; exit 1; fi

# -----------------------------------------------------------------------------
#  STEP-1.2 : Retriece the IPv4 Address of the FPGA Instance
# -----------------------------------------------------------------------------
fRetrieveInstanceIp ${INSTANCE_ID} ${ZYC2_USER} ${ZYC2_PASS}


echo -e
echo -e "#####################################################################"
echo -e "###                      TCP TESTS                                ###"
echo -e "#####################################################################"
echo -e "#"
echo -e "#        FPGA INSTANCE = ${INSTANCE_ID} / ${INSTANCE_IP} "
echo -e "#"    
echo -e "#                       LOOP = ${LOOP} "
echo -e "#"    
echo -e "#####################################################################"
echo -e
echo -e

if [ $RUN_SEND = true ]; then
  echo -e "###  TCP SEND  ######################################################" 

  # SEND - Ramp from 1 to size
  for value in {1..${LOOP}}; \
  do \
    if [ ${ERRORS} -lt ${MAX_ERRORS} ]; then \
      python3 tc_TcpSend.py -sd 0 -lc 5 -sz 100     -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${INSTANCE_IP} -ii ${INSTANCE_ID} ; \
      if [ $? -ne 0 ]; then ((ERRORS++)); echo -e "#### ERRORS=${ERRORS} ####";  fi; \
    fi
  done

  # SEND - Random size
  for value in {1..${LOOP}}; \
  do \
    if [ ${ERRORS} -lt ${MAX_ERRORS} ]; then \
      python3 tc_TcpSend.py -lc 10                   -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${INSTANCE_IP} -ii ${INSTANCE_ID} ; \
      if [ $? -ne 0 ]; then ((ERRORS++)); echo -e "#### ERRORS=${ERRORS} ####";  fi; \
    fi
  done

  # SEND - Ramp of 1 MB
  for value in {1..${LOOP}}; \
  do \
    if [ ${ERRORS} -lt ${MAX_ERRORS} ]; then \
      python3 tc_TcpSend.py                          -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${INSTANCE_IP} -ii ${INSTANCE_ID} -sd 0 -sz 1024 -lc 1024
      if [ $? -ne 0 ]; then ((ERRORS++)); echo -e "#### ERRORS=${ERRORS} ####";  fi; \
    fi
  done

  # SEND - 10MB
  for value in {1..${LOOP}}; \
  do \
    if [ ${ERRORS} -lt ${MAX_ERRORS} ]; then \
python3 tc_TcpSend.py -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${INSTANCE_IP} -ii ${INSTANCE_ID} -sd 8 -sz 1024 -lc 10240  -st 0.00000
      if [ $? -ne 0 ]; then ((ERRORS++)); echo -e "#### ERRORS=${ERRORS} ####";  fi; \
    fi
  done

  # SEND - 100MB
  for value in {1..${LOOP}}; \
  do \
    if [ ${ERRORS} -lt ${MAX_ERRORS} ]; then \
python3 tc_TcpSend.py -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${INSTANCE_IP} -ii ${INSTANCE_ID} -sd 7 -sz 1024 -lc 102400  -st 0.00000
      if [ $? -ne 0 ]; then ((ERRORS++)); echo -e "#### ERRORS=${ERRORS} ####";  fi; \
    fi
  done

  # SEND - 1GB
  for value in {1..${LOOP}}; \
  do \
    if [ ${ERRORS} -lt ${MAX_ERRORS} ]; then \
python3 tc_TcpSend.py -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${INSTANCE_IP} -ii ${INSTANCE_ID} -sd 8 -sz 1024 -lc 1024000 -st 0.0000
      if [ $? -ne 0 ]; then ((ERRORS++)); echo -e "#### ERRORS=${ERRORS} ####";  fi; \
    fi
  done

  # SEND - 10GB at slow rate 
  for value in {1..${LOOP}}; \
  do \
    if [ ${ERRORS} -lt ${MAX_ERRORS} ]; then \
python3 tc_TcpSend.py -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${INSTANCE_IP} -ii ${INSTANCE_ID} -sd 7 -sz 1024 -lc 10240000 -st 0.0000
      if [ $? -ne 0 ]; then ((ERRORS++)); echo -e "#### ERRORS=${ERRORS} ####";  fi; \
    fi
  done
fi

if [ $RUN_RECV = true ]; then
  echo -e "###  TCP RECV  ######################################################" 

  # RECV - Ramp
  for value in {1..${LOOP}}; \
  do \
    if [ ${ERRORS} -lt ${MAX_ERRORS} ]; then \
      python3 tc_TcpRecv.py -sd 0 -sz 256            -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${INSTANCE_IP} -ii ${INSTANCE_ID} ; \
      if [ $? -ne 0 ]; then ((ERRORS++)); echo -e "#### ERRORS=${ERRORS} ####";  fi; \
    fi
  done

  # RECV - Fixed size
  for value in {1..${LOOP}}; \
  do \
    if [ ${ERRORS} -lt ${MAX_ERRORS} ]; then \
      python3 tc_TcpRecv.py -lc 64 -sz 1024          -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${INSTANCE_IP} -ii ${INSTANCE_ID} ; \
      if [ $? -ne 0 ]; then ((ERRORS++)); echo -e "#### ERRORS=${ERRORS} ####";  fi; \
    fi
  done

  # RECV - Fixed size
  for value in {1..${LOOP}}; \
  do \
    if [ ${ERRORS} -lt ${MAX_ERRORS} ]; then \
      python3 tc_TcpRecv.py -lc 32 -sz 2048          -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${INSTANCE_IP} -ii ${INSTANCE_ID} ; \
      if [ $? -ne 0 ]; then ((ERRORS++)); echo -e "#### ERRORS=${ERRORS} ####";  fi; \
    fi
  done

  # RECV - Random size
  for value in {1..${LOOP}}; \
  do \
    if [ ${ERRORS} -lt ${MAX_ERRORS} ]; then \
      python3 tc_TcpRecv.py -lc 10                   -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${INSTANCE_IP} -ii ${INSTANCE_ID} ; \
      if [ $? -ne 0 ]; then ((ERRORS++)); echo -e "#### ERRORS=${ERRORS} ####";  fi; \
    fi
  done
fi


if [ $RUN_ECHO = true ]; then
  echo -e "###  TCP ECHO  ######################################################" 

  # ECHO - Fixed size
  for value in {1..${LOOP}}; \
  do \
    if [ ${ERRORS} -lt ${MAX_ERRORS} ]; then \
      python3 tc_TcpEcho.py -lc 10 -sz ${cZYC2_MSS}   -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${INSTANCE_IP} -ii ${INSTANCE_ID} ; \
      if [ $? -ne 0 ]; then ((ERRORS++)); echo -e "#### ERRORS=${ERRORS} ####";  fi; \
    fi
  done
  
  # ECHO - Random size
  for value in {1..${LOOP}}; \
  do \
    if [ ${ERRORS} -lt ${MAX_ERRORS} ]; then \
      python3 tc_TcpEcho.py -lc 10                   -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${INSTANCE_IP} -ii ${INSTANCE_ID} ; \
      if [ $? -ne 0 ]; then ((ERRORS++)); echo -e "#### ERRORS=${ERRORS} ####";  fi; \
    fi
  done  
fi


if [ $RUN_IPERF = true ]; then
  echo -e "###  TCP IPERF ######################################################" 
  
  # IPERF - Host-2-Fpga 
  iperf -c ${INSTANCE_IP} -p 8800 -t 10
  echo -e "Done with 10 seconds of Iperf. \n"
  if [ $? -eq 0 ]; \
  then \
    echo -e "Done with ${LENGTH_OF_IPREF_TEST} seconds of Iperf. \n"; \
  else \
    ((ERRORS++)); echo "#### ERRORS=${ERRORS} ####"; fi; \
  
fi

echo -e "#####################################################################"
echo -e "###                   END OF TCP TESTS                            ###"
echo -e "###                      (ERRORS=${ERRORS})                               ###"
echo -e "#####################################################################"


### FAILING TCP CASES #####
# for value in {1..10}; do python3 tc_TcpEcho.py -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${INSTANCE_IP} -ii ${INSTANCE_ID} -lc 1000               ; done # Random Size
