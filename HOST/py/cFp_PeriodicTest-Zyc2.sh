#!/bin/bash
# Script to test a series of FPGAs

echo -e " "
date
# id 
echo -e " "

# define functions (the function definition must be placed before any calls to the function)
# helper function
HELPER_FUNCTION(){
echo -e "********************************************************************************************"
echo -e " "
echo -e "HELP on how to use this script:"
echo -e " "
echo -e "./cFp_PeriodicTest-Zyc2.sh {-h STOP_ERROR TCP_SEND TCP_RECV TCP_ECHO UDP_SEND UDP_RECV UDP_ECHO}{-ri RESOURCE_ID}"
echo -e " "
echo -e "-h prints this HELP message"
echo -e "options to set ERROR behaviour: "
echo -e "STOP_ERROR: if given the script stops on ERROR"
echo -e "options to set a test: if given it runs this tests"
echo -e "TCP_SEND TCP_RECV TCP_ECHO UDP_SEND UDP_RECV UDP_ECHO"
echo -e " "
echo -e "-ri RESOURCE_ID a specific RESOURCE to test (instead of a random number)"
echo -e "*** this (pair of) option must be the last when calling the script ***"
echo -e " "
echo -e "To test a series of FPGAs you may use (in bash):"
echo -e " "
echo -e "for n in {1..500} ;do (echo round nr. $n; ./cFp_PeriodicTest-Zyc2.sh {OPTIONS}; sleep 10 ) | tee -a cFp_PeriodicTest-Zyc2.log ;done"
echo -e " "
echo -e "********************************************************************************************"
echo -e " "
}

# delete_instance
DELETE_INSTANCE(){
	echo -e " "
	echo -e "           **** The instance is going to be deleted ****"
	echo -e " "
	REPLY=$(curl -X DELETE --header 'Accept: application/json' "http://10.12.0.132:8080/instances/${RESOURCE_ID}?username=${ZYC2_USER}&password=${ZYC2_PASS}")
	# echo ${REPLY}
	
	if $VPN_CHECK ; then 
		# delete credentials
		sudo kill $(cat /tmp/zyc2-user-vpn.pid)
		rm -f /tmp/zyc2-user-vpn.credentials
		rm -f /tmp/zyc2-user-vpn.conf
	fi
}

# test_instance
PUT_TEST_INSTANCE(){
	REPLY=$(curl -X PUT --header 'Content-Type: application/x-www-form-urlencoded' --header 'Accept: application/json' -d ${IMAGE_ID_IN} "http://10.12.0.132:8080/administration/test_instance/${RESOURCE_ID}?username=${ZYC2_USER}&password=${ZYC2_PASS}&dont_verify_memory=0")
	# echo ${REPLY}
}

# set_status_available
SET_STATUS_AVAILABLE(){
	echo -e " "
	echo -e "**** After a successful test, the status of the FPGA is changed back to available ****"
	echo -e " "

	REPLY=$(curl -X PUT --header 'Content-Type: application/json' --header 'Accept: application/json' "http://10.12.0.132:8080/resources/${RESOURCE_ID}/status/?username=${ZYC2_USER}&password=${ZYC2_PASS}&new_status=AVAILABLE")
	# echo ${REPLY}
}

# get_resource_status
GET_RESOURCE_STATUS(){
	# check status of resource
	REPLY=$(curl -X GET --header 'Accept: application/json' "http://10.12.0.132:8080/resources/${RESOURCE_ID}/status/?username=${ZYC2_USER}&password=${ZYC2_PASS}")
	# echo ${REPLY}
}

# get the last argument passed to this script
NTHARG() {
    shift $1
    printf '%s\n' "$1"
}

# pre-define variables
STOP_ERROR=false	# if true, exit script if ERROR occurs, else false

TCP_SEND=false  	# change to true to run the TcpSend tests
TCP_RECV=false  	# change to true to run the TcpRecv tests
TCP_ECHO=false  	# change to true to run the TcpEcho tests

UDP_SEND=false  	# change to true to run the UdpSend tests
UDP_RECV=false  	# change to true to run the UdpRecv tests
UDP_ECHO=false  	# change to true to run the UdpEcho tests

IPERF=true    		# change to false to change running the IPERF test by default
VPN_CHECK=false		# change to false to change not runnning user_VPN check by default
RAND=true			# change to false to choose the resource_id not randomly

# set given parameters to true
for var in "$@" 
do 
	if [ $var = "-h" ]; then
		HELPER_FUNCTION
	elif [ $var = "-ri" ]; then
		# get last argument, '-ri' must be the second last
		RESOURCE_ID=`NTHARG $# "$@"`
		RAND=false
	else
		eval $var=true
    fi
done

echo $STOP_ERROR $TCP_SEND $TCP_RECV $TCP_ECHO $UDP_SEND $UDP_RECV $UDP_ECHO $IPERF $VPN_CHECK $RAND $RESOURCE_ID

# Used for testing up to here
# echo "Press any key to continue"
# while [ true ] ; do
	# read -t 3 -n 1
	# if [ $? = 0 ] ; then
		# exit
	# else
		# echo "waiting for the keypress"
	# fi
# done

if $RAND ; then 
	# generate a random RESOURCE_ID in the range of 1..98, 38 and 64 are not valid resources
	while :; do ran=$RANDOM; (($ran < 32760)) && echo $(((ran%98)+1)) && break; done
	RESOURCE_ID=$(((ran%98)+1))
fi

echo -e " "
echo -e "#####################################################################"
echo -e "###                  RESOURCE_ID = ${RESOURCE_ID}                               "
echo -e "#####################################################################"
echo -e " "

if $VPN_CHECK ; then 
	# Send 4 PINGs and wait 4 seconds max for each of them (for a total max of 16s)  
	ping -c 4 -W 2 10.12.0.1
	if [ $? -ne 0 ]; then 
		# Connect to VPN if not yet
		# copy credentials
		cp -f ${ZYC2_USER_VPN_CONFIG} /tmp/zyc2-user-vpn.conf
		cp -f ${ZYC2_USER_VPN_CREDENTIALS} /tmp/zyc2-user-vpn.credentials
		sudo openvpn --config /tmp/zyc2-user-vpn.conf --auth-user-pass /tmp/zyc2-user-vpn.credentials --log /tmp/zyc2-user-vpn.log --writepid /tmp/zyc2-user-vpn.pid --daemon
		sleep 10
		# Send 4 PINGs and wait 4 seconds max for each of them (for a total max of 16s)  
		ping -c 4 -W 2 10.12.0.1
	fi
	if [ $? -ne 0 ]; then
		echo -e " "
		echo -e "###                  user VPN cannot be reached ####"
		echo -e " "
		# delete credentials
		sudo kill $(cat /tmp/zyc2-user-vpn.pid)
		rm -f /tmp/zyc2-user-vpn.credentials
		rm -f /tmp/zyc2-user-vpn.conf
		exit 1
	else 
		echo -e " "
		echo -e "###                  user VPN reached ####" 
		echo -e " "
	fi
fi

GET_RESOURCE_STATUS

RESOURCE_STATUS=$(echo $REPLY | jq .status)
RESOURCE_STATUS="${RESOURCE_STATUS%\"}"
RESOURCE_STATUS="${RESOURCE_STATUS#\"}"
echo -e "#### Resource-Nr ${RESOURCE_ID} is ${RESOURCE_STATUS} ####"

if true ; then # change to true to run the `resource is used` check, false otherwise
	if [ "$RESOURCE_STATUS" = "USED" ]; then
		# exit here if resource is used
		echo -e " "
		echo -e "#### Resource-Nr ${RESOURCE_ID} is used and not going to be tested! ####"
		echo -e " "
		exit 1
	else 
		echo -e " "
		echo "#### Resource-Nr ${RESOURCE_ID} is not used and going to be tested! ####"
		echo -e " "
	fi
fi

# Test programming the instance and get the IP address
RESOURCE_STATUS="empty"
echo -e " "
echo -e "#####################################################################"
echo -e "###     Test instance programming and get the IP address          ###"
echo -e "#####################################################################"
echo -e " "
echo -e "**** TODO: Choose automatically the image_id with the latest successful bitfile from cFp_BringUp-ZYC2 ****"
# 2020-09-03
# IMAGE_ID_IN='image_id=f372e409-6f12-497e-afe4-d32d31f0bbc8'
# 2021-01-20
IMAGE_ID_IN='image_id=866268f3-df01-42e1-8055-625ebead666f'
echo ${IMAGE_ID_IN}

PUT_TEST_INSTANCE

RESOURCE_STATUS=$(echo $REPLY | jq .status)
RESOURCE_STATUS="${RESOURCE_STATUS%\"}"
RESOURCE_STATUS="${RESOURCE_STATUS#\"}"

if [ "$RESOURCE_STATUS" != "Successfully deployed" ]; then 
	echo -e "#### programming ERROR Resource-Nr ${RESOURCE_ID} ####"
	FAILURE_MSG=$(echo $REPLY | jq .failure_msg)
	# FAILURE_MSG="${FAILURE_MSG%\"}"
	# FAILURE_MSG="${FAILURE_MSG#\"}"
	echo ${FAILURE_MSG}
	DELETE_INSTANCE
	exit 1
else echo -e "#### Resource-Nr ${RESOURCE_ID} is ${RESOURCE_STATUS} ####"
fi

ROLE_IP=$(echo $REPLY | jq .role_ip)
ROLE_IP="${ROLE_IP%\"}"
ROLE_IP="${ROLE_IP#\"}"
echo ROLE_IP = ${ROLE_IP}
INSTANCE_ID=$(echo $REPLY | jq .instance_id)
echo INSTANCE_ID = ${INSTANCE_ID}
IMAGE_ID=$(echo $REPLY | jq .image_id)
echo IMAGE_ID = ${IMAGE_ID}

# echo "Press any key to continue"
# while [ true ] ; do
# 	read -t 3 -n 1
# 	if [ $? = 0 ] ; then
# 		exit
# 	else
# 		echo "waiting for the keypress"
# 	fi
# done

# Send 2 PINGs and wait 2 seconds max for each of them (for a total max of 4s)  
ping -c 2 -W 2 $ROLE_IP 
### Setting up venv ####
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt

LOOP=2
ZYC2_MSS=1352

if $TCP_SEND ; then
	echo -e " "
	echo -e "#####################################################################"
	echo -e "###                      TCP_SEND TESTS                           ###"
	echo -e "#####################################################################"
	echo -e " "
fi
# SEND - Ramp from 1 to size
if $TCP_SEND ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_TcpSend.py -sd 0 -lc 5 -sz 100     -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "#### TCP_SEND_1 ERROR ####"; if $STOP_ERROR; then DELETE_INSTANCE; exit 1; fi; fi; \
  done
fi
# SEND - Random size
if $TCP_SEND ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_TcpSend.py -lc 10                   -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "#### TCP_SEND_2 ERROR ####"; if $STOP_ERROR; then DELETE_INSTANCE; exit 1; fi; fi; \
  done
fi
if $TCP_RECV ; then
	echo -e " "
	echo -e "#####################################################################"
	echo -e "###                      TCP_RECV TESTS                           ###"
	echo -e "#####################################################################"
	echo -e " "
fi
# RECV - Ramp
if $TCP_RECV ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_TcpRecv.py -sd 0 -sz 256            -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "#### TCP_RECV_1 ERROR ####"; if $STOP_ERROR; then DELETE_INSTANCE; exit 1; fi; fi; \
  done
fi
# RECV - Fixed size
if $TCP_RECV ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_TcpRecv.py -lc 64 -sz 1024          -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "####TCP_RECV_2 ERROR ####"; if $STOP_ERROR; then DELETE_INSTANCE; exit 1; fi; fi; \
  done
fi
# RECV - Fixed size
if $TCP_RECV ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_TcpRecv.py -lc 32 -sz 2048          -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "#### TCP_RECV_3 ERROR ####"; if $STOP_ERROR; then DELETE_INSTANCE; exit 1; fi; fi; \
  done
fi
# RECV - Random size
if $TCP_RECV ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_TcpRecv.py -lc 10                   -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "#### TCP_RECV_4 ERROR ####"; if $STOP_ERROR; then DELETE_INSTANCE; exit 1; fi; fi; \
  done
fi
if $TCP_ECHO ; then
	echo -e " "
	echo -e "#####################################################################"
	echo -e "###                      TCP_ECHO TESTS                           ###"
	echo -e "#####################################################################"
	echo -e " "
fi
# ECHO - Fixed size
if $TCP_ECHO ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_TcpEcho.py -lc 10 -sz ${ZYC2_MSS}   -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "#### TCP_ECHO_1 ERROR ####"; if $STOP_ERROR; then DELETE_INSTANCE; exit 1; fi; fi; \
  done
fi
# ECHO - Random size
if $TCP_ECHO ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_TcpEcho.py -lc 10                   -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "#### TCP_ECHO_2 ERROR ####"; if $STOP_ERROR; then DELETE_INSTANCE; exit 1; fi; fi; \
  done
fi

if $UDP_SEND ; then
	echo -e " "
	echo -e "#####################################################################"
	echo -e "###                      UDP_SEND TESTS                           ###"
	echo -e "#####################################################################"
	echo -e " "
fi

# SEND - Ramp from 1 to size
if $UDP_SEND ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_UdpSend.py -lc 100  -sz 1416 -sd 0  -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "#### UDP_SEND_1 ERROR ####"; DELETE_INSTANCE; exit 2; fi; \
  done
fi
# SEND - Random size
if $UDP_SEND ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_UdpSend.py -lc 100                  -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "#### UDP_SEND_2 ERROR ####"; DELETE_INSTANCE; exit 2; fi; \
  done
fi
if $UDP_RECV ; then
	echo -e " "
	echo -e "#####################################################################"
	echo -e "###                      UDP_RECV TESTS                           ###"
	echo -e "#####################################################################"
	echo -e " "
fi
# RECV - Fixed size
if $UDP_RECV ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_UdpRecv.py -lc 10 -sz 1416          -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "#### UDP_RECV_1 ERROR ####"; DELETE_INSTANCE; exit 2; fi; \
  done
fi
# RECV - Fixed
if $UDP_RECV ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_UdpRecv.py -lc 10 -sz 50000         -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "#### UDP_RECV_2 ERROR ####"; DELETE_INSTANCE; exit 2; fi; \
  done
fi
# RECV - Random
if $UDP_RECV ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_UdpRecv.py -lc 10                   -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "#### UDP_RECV_3 ERROR ####"; DELETE_INSTANCE; exit 2; fi; \
  done
fi
if $UDP_ECHO ; then
	echo -e " "
	echo -e "#####################################################################"
	echo -e "###                      UDP_ECHO TESTS                           ###"
	echo -e "#####################################################################"
	echo -e " "
fi
# ECHO - Ramp from 1 to size
if $UDP_ECHO ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_UdpEcho.py -lc 1 -sz 1024 -sd 0 -mt  -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "#### UDP_ECHO_1 ERROR ####"; DELETE_INSTANCE; exit 2; fi; \
  done  
fi
# ECHO - Random size w/ multi-threading
if $UDP_ECHO ; then
for value in {1..${LOOP}}; \
  do \
    python3 tc_UdpEcho.py -lc 100 -mt              -un ${ZYC2_USER} -up ${ZYC2_PASS} -fi ${ROLE_IP} -ii ${INSTANCE_ID} ; \
    if [ $? -ne 0 ]; then echo -e "#### UDP_ECHO_2 ERROR ####"; DELETE_INSTANCE; exit 2; fi; \
    done
fi
# IPERF - Host-2-Fpga
if $IPERF ; then
	echo -e " "
	echo -e "#####################################################################"
	echo -e "###                 IPERF - Host-2-Fpga TEST                      ###"
	echo -e "#####################################################################"
	echo -e " "
	iperf -c ${ROLE_IP} -p 8800 -u -t 30;
	if [ $? -eq 0 ]; then echo -e "Done with Iperf. \n"; else  echo "#### ERROR ####"; DELETE_INSTANCE; exit 1; fi;
fi

echo -e " "
echo -e "#####################################################################"
echo -e "###                    END OF TESTS                               ###"
echo -e "###                RESOURCE_ID = ${RESOURCE_ID}                               ###"
echo -e "#####################################################################"
echo -e " "

SET_STATUS_AVAILABLE
# echo -e " "
# echo -e "**** After a successful test, the status of the FPGA is changed back to available ****"
# echo -e " "

REPLY=$(curl -X PUT --header 'Content-Type: application/json' --header 'Accept: application/json' "http://10.12.0.132:8080/resources/${RESOURCE_ID}/status/?username=${ZYC2_USER}&password=${ZYC2_PASS}&new_status=AVAILABLE")
echo ${REPLY}

DELETE_INSTANCE
