#!/bin/bash

HELPER_FUNCTION(){
echo "********************************************************************************************"
echo -e " "
echo HELP on how to use this script:
echo -e " "
echo ./cFp_PeriodicTest-Zyc2.sh {-h STOP_ERROR TCP_SEND TCP_RECV TCP_ECHO UDP_SEND UDP_RECV UDP_ECHO}
echo -e " "
echo -h prints this HELP message
echo options to set ERROR behaviour: 
echo STOP_ERROR: if given the script stops on ERROR
echo options to set a test: if given it runs this tests
echo TCP_SEND TCP_RECV TCP_ECHO UDP_SEND UDP_RECV UDP_ECHO
echo  -e " "
echo "To test a series of FPGAs you may use (in bash):"
echo -e " "
echo "for n in {1..500} ;do (echo round nr. $n; ./cFp_PeriodicTest-Zyc2.sh {OPTIONS}; sleep 10 ) | tee -a cFp_PeriodicTest-Zyc2.log ;done"
echo -e " "
echo "********************************************************************************************"
echo -e " "
}

# HELPER_FUNCTION

STOP_ERROR=false	# if true, exit script if ERROR occurs, else false

TCP_SEND=false   # change to true to run the TcpSend tests
TCP_RECV=false   # change to true to run the TcpRecv tests
TCP_ECHO=false   # change to true to run the TcpEcho tests

UDP_SEND=false    # change to true to run the UdpSend tests
UDP_RECV=false    # change to true to run the UdpRecv tests
UDP_ECHO=false    # change to true to run the UdpEcho tests

# set given parameters to true
for var in "$@" 
do 
	if [ $var = "-h" ]; then
		HELPER_FUNCTION
	else
		eval $var=true
    fi
done

# echo $STOP_ERROR $TCP_SEND $TCP_RECV $TCP_ECHO $UDP_SEND $UDP_RECV $UDP_ECHO

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

# define functions
# delete_instance
DELETE_INSTANCE(){
	echo -e " "
	echo -e "           **** The instance is going to be deleted ****"
	echo -e " "
	REPLY=$(curl -X DELETE --header 'Accept: application/json' "http://10.12.0.132:8080/instances/${INSTANCE_ID}?username=${ZYC2_USER}&password=${ZYC2_PASS}")
	# echo ${REPLY}
}

# test_instance
PUT_TEST_INSTANCE(){
	REPLY=$(curl -X PUT --header 'Content-Type: application/x-www-form-urlencoded' --header 'Accept: application/json' -d ${IMAGE_ID_IN} "http://10.12.0.132:8080/administration/test_instance/${INSTANCE_ID}?username=${ZYC2_USER}&password=${ZYC2_PASS}&dont_verify_memory=0")
	# echo ${REPLY}
}

# set_status_available
SET_STATUS_AVAILABLE(){
	echo -e " "
	echo -e "**** After a successful test, the status of the FPGA is changed back to available ****"
	echo -e " "

	REPLY=$(curl -X PUT --header 'Content-Type: application/json' --header 'Accept: application/json' "http://10.12.0.132:8080/resources/${INSTANCE_ID}/status/?username=${ZYC2_USER}&password=${ZYC2_PASS}&new_status=AVAILABLE")
	# echo ${REPLY}
}

# get_resource_status
GET_RESOURCE_STATUS(){
	# check status of resource
	REPLY=$(curl -X GET --header 'Accept: application/json' "http://10.12.0.132:8080/resources/${INSTANCE_ID}/status/?username=${ZYC2_USER}&password=${ZYC2_PASS}")
	# echo ${REPLY}
}

date
id 
# copy credentials script
# cp -f ${ZYC2_USER_VPN_CONFIG} /tmp/zyc2-user-vpn.conf
# cp -f ${ZYC2_USER_VPN_CREDENTIALS} /tmp/zyc2-user-vpn.credentials

# generate a random INSTANCE_ID in the range of 1..98, 38 and 64 are not valid resources
while :; do ran=$RANDOM; (($ran < 32760)) && echo $(((ran%98)+1)) && break; done
INSTANCE_ID=$(((ran%98)+1))
# INSTANCE_ID=16

echo -e " "
echo -e "#####################################################################"
echo -e "###                  Instance_ID = ${INSTANCE_ID}                               "
echo -e "#####################################################################"
echo -e " "

if true ; then # change to true to run user VPN check, false otherwise
	# Send 4 PINGs and wait 4 seconds max for each of them (for a total max of 16s)  
	ping -c 4 -W 2 10.12.0.1
	if [ $? -ne 0 ]; then 
		# Connect to VPN if not yet
		sudo openvpn --config /tmp/zyc2-user-vpn.conf --auth-user-pass /tmp/zyc2-user-vpn.credentials --log /tmp/zyc2-user-vpn.log --writepid /tmp/zyc2-user-vpn.pid --daemon
		sleep 10
		# Send 4 PINGs and wait 4 seconds max for each of them (for a total max of 16s)  
		ping -c 4 -W 2 10.12.0.1
	fi
	if [ $? -ne 0 ]; then
		echo -e " "
		echo -e "###                  user VPN cannot be reached ####"
		echo -e " "
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
echo -e "#### Resource-Nr ${INSTANCE_ID} is ${RESOURCE_STATUS} ####"

if true ; then # change to true to run the `resource is used` check, false otherwise
	if [ "$RESOURCE_STATUS" = "USED" ]; then
		# exit here if resource is used
		echo -e " "
		echo -e "#### Resource-Nr ${INSTANCE_ID} is used and not going to be tested! ####"
		echo -e " "
		exit 1
	else 
		echo -e " "
		echo "#### Resource-Nr ${INSTANCE_ID} is not used and going to be tested! ####"
		echo -e " "
	fi
fi

# Test instance programming and get the IP address
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
	echo -e "#### programming ERROR Resource-Nr ${INSTANCE_ID} ####"; DELETE_INSTANCE
	exit 1
else echo -e "#### Resource-Nr ${INSTANCE_ID} is ${RESOURCE_STATUS} ####"
fi

# RESP_CODE=$(echo ${REPLY:421:3})
# echo RESP_CODE = ${RESP_CODE}
# if [ $RESP_CODE > 299 ]; then echo -e "#### ERROR while programming instance Resource-Nr ${INSTANCE_ID} ####"; exit 1; fi; \

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

STOP_ERROR=false	# if true, exit script if ERROR occurs, else false

# TCP_SEND=false   # change to true to run the TcpSend tests
# TCP_RECV=false   # change to true to run the TcpRecv tests
# TCP_ECHO=false   # change to true to run the TcpEcho tests

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
# IPERF - Host-2-Fpga 
# iperf -c ${ROLE_IP} -p 8800 -t 10
# echo -e "Done with 10 seconds of Iperf. \n"

# UDP_SEND=true    # change to true to run the UdpSend tests
# UDP_RECV=true    # change to true to run the UdpRecv tests
# UDP_ECHO=true    # change to true to run the UdpEcho tests

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
echo -e " "
echo -e "#####################################################################"
echo -e "###                 IPERF - Host-2-Fpga TEST                      ###"
echo -e "#####################################################################"
echo -e " "
iperf -c ${ROLE_IP} -p 8800 -u -t 30;
if [ $? -eq 0 ]; then echo -e "Done with 5 minutes of Iperf. \n"; else  echo "#### ERROR ####"; DELETE_INSTANCE; exit 1; fi;

echo -e " "
echo -e "#####################################################################"
echo -e "###                    END OF TESTS                               ###"
echo -e "###                Instance_ID = ${INSTANCE_ID}                               ###"
echo -e "#####################################################################"
echo -e " "

DELETE_INSTANCE

SET_STATUS_AVAILABLE
# echo -e " "
# echo -e "**** After a successful test, the status of the FPGA is changed back to available ****"
# echo -e " "

REPLY=$(curl -X PUT --header 'Content-Type: application/json' --header 'Accept: application/json' "http://10.12.0.132:8080/resources/${INSTANCE_ID}/status/?username=${ZYC2_USER}&password=${ZYC2_PASS}&new_status=AVAILABLE")
echo ${REPLY}

# sudo kill $(cat /tmp/zyc2-user-vpn.pid)
# rm -f /tmp/zyc2-user-vpn.credentials
# rm -f /tmp/zyc2-user-vpn.conf