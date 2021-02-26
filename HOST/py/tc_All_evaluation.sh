#!/bin/bash
# Note important difference between /bin/sh and /bin/bash
# https://askubuntu.com/q/141928/295286

# This script evaluates a tc_*ALL*.log file. It writes the summary of analysis for each test 
# TCP_SEND TCP_RECV TCP_ECHO UDP_SEND UDP_RECV UDP_ECHO IPERF
# in separated *.txt files for further evaluation in a spreadsheet or similar
# Passing the argument DELETE_FILES the *.txt are not written (in fact deleted after evaluation)
# 

# define functions (the function definition must be placed before any calls to the function)
# helper function
HELPER_FUNCTION(){
	echo -e "********************************************************************************************"
	echo -e " "
	echo -e "HELP on how to use this script:"
	echo -e " "
	echo -e "./tc_All_evaluation.sh filename.log {-h} {DELETE_FILES}"
	echo -e " "
	echo -e "-h prints this HELP message"
    echo -e "the file has to be a tc_cFp_PeriodicTest-Zyc2_*.log file"
	echo -e "Passing the argument DELETE_FILES the *.txt are not written"
	echo -e " "
	echo -e "The script writes the evaluation summary into filename_summary.csv"
	echo -e " "
	echo -e "********************************************************************************************"
	echo -e " "
	exit 1
}
# STOP_HERE   # uncomment to stop execution here function
STOP_HERE(){
    # Used for testing up to here and then stop
    echo "Press any key to continue"
    while [ true ] ; do
        read -t 3 -n 1
        if [ $? = 0 ] ; then
            exit
        else
            echo -ne "waiting for the keypress\r"
        fi
    done
}

# pre-define variables
DELETE_FILES=false  # if true, the *.txt files are deleted after evaluation

TCP_SEND=false      # change to true to run the TcpSend tests
TCP_RECV=false      # change to true to run the TcpRecv tests
TCP_ECHO=false      # change to true to run the TcpEcho tests

UDP_SEND=false      # change to true to run the UdpSend tests
UDP_RECV=false      # change to true to run the UdpRecv tests
UDP_ECHO=false      # change to true to run the UdpEcho tests

IPERF=false         # change to false to change running the IPERF test by default

# set given parameters to true
for var in "$@" 
do 
	if [ $var = "-h" ]; then
		HELPER_FUNCTION
	elif [ -f "$var" ]    # check if parameter is existing file 
		then 
			file=$var
		else
			eval $var=true
	fi
done

# file=${1}
# if [ -f "$1" ]
if [ -z "$file" ]   # check if empty
then
    echo "file does not exist"
	HELPER_FUNCTION
else
	filename=${file/.*}
	ext="${file#*.}"
	filename_summary="${filename}_summary.csv"	# writing summary to filename_summary.csv
	rm -f $filename_summary	# remove existing file
    echo " "
    echo "going to evaluate ${file}" | tee -a $filename_summary	#first line in *.csv file
    echo " " | tee -a $filename_summary
#    echo "new file is going to be named ${file/.*}_success.txt"
fi

if [[ 0 == 1 ]]  # set 0 == 1 to disable, 1 == 1 to enable this check
then
    # if [ ${file: -4} == ".log" ]
    if [[ $file == *"ALL"*".log" ]]
    then
       echo "file = ${file}"
       echo "filename = ${filename}"
       echo "ext = ${ext}"
    else
        echo "usage: ./tc_All_evaluation.sh filename.log"
        echo "the file has to be a tc_cFp_PeriodicTest-Zyc2_*.log file"
    fi
fi
grep -v "STOP_ERROR" $file > temporary_file.txt	# exlude lines including STOP_ERROR string

# find image_id
filename_IMAGE_ID="${filename}-IMAGE_ID.txt"
# echo "filename_IMAGE_ID = ${filename_IMAGE_ID}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_IMAGE_ID
grep "Programming with  image_id=" temporary_file.txt > $filename_IMAGE_ID
grep -m1 "Programming with  image_id=" $filename_IMAGE_ID | tee -a $filename_summary
# STOP_HERE   # uncomment to stop execution here

filename_ROUND_NR="${filename}-ROUND_NR.txt"
# echo "filename_ROUND_NR = ${filename_ROUND_NR}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_ROUND_NR
grep "round nr." temporary_file.txt > $filename_ROUND_NR
(printf "ROUND NR. count:              , "; grep -c "round nr." $filename_ROUND_NR) | tee -a $filename_summary
# STOP_HERE   # uncomment to stop execution here

filename_END_OF_TESTS="${filename}-END_OF_TESTS.txt"
# echo "filename_END_OF_TESTS = ${filename_END_OF_TESTS}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_END_OF_TESTS
grep -A 1 "END OF TESTS" temporary_file.txt > temp_file.txt
grep "RESOURCE_ID =" temp_file.txt > $filename_END_OF_TESTS
rm temp_file.txt
(printf "END OF TESTS count:           , "; grep -c "RESOURCE_ID" $filename_END_OF_TESTS) | tee -a $filename_summary
# STOP_HERE   # uncomment to stop execution here

filename_AVAILABLE="${filename}_AVAILABLE.txt"
touch $filename_AVAILABLE
grep "is AVAILABLE" temporary_file.txt > $filename_AVAILABLE
(printf "is AVAILABLE count:           , "; grep -c "is AVAILABLE" $filename_AVAILABLE) | tee -a $filename_summary
# STOP_HERE   # uncomment to stop execution here

filename_NO_RESPONSE="${filename}_NO_RESPONSE.txt"
touch $filename_NO_RESPONSE
grep "NO RESPONSE" temporary_file.txt > $filename_NO_RESPONSE
(printf "NO RESPONSE count:            , "; grep -c "NO RESPONSE" $filename_NO_RESPONSE) | tee -a $filename_summary
# STOP_HERE   # uncomment to stop execution here

echo "--------------------------" | tee -a $filename_summary
echo "counting programming ERROR" | tee -a $filename_summary
filename_ERROR="${filename}_ERROR.txt"
touch $filename_ERROR
grep "ERROR " temporary_file.txt > $filename_ERROR
(printf "ERROR count:                  , "; grep -c "ERROR" $filename_ERROR) | tee -a $filename_summary
# STOP_HERE   # uncomment to stop execution here
filename_JTAG_ERROR="${filename}_JTAG_ERROR.txt"
touch $filename_JTAG_ERROR
grep "JTAG" $filename_ERROR > $filename_JTAG_ERROR
(printf "JTAG ERROR count:             , "; grep -c "JTAG" $filename_JTAG_ERROR) | tee -a $filename_summary
# STOP_HERE   # uncomment to stop execution here

filename_programming_ERROR="${filename}-programming_ERROR.txt"
grep "programming ERROR" temporary_file.txt > $filename_programming_ERROR
(printf "programming ERROR count:      , "; grep -c "programming ERROR" $filename_programming_ERROR) | tee -a $filename_summary
# STOP_HERE   # uncomment to stop execution here
echo "which includes" | tee -a $filename_summary
filename_WARNING="${filename}_WARNING.txt"
touch $filename_WARNING
grep "WARNING" temporary_file.txt > $filename_WARNING
(printf "WARNING count:                , "; grep -c "WARNING" $filename_WARNING) | tee -a $filename_summary
# STOP_HERE   # uncomment to stop execution here
echo "which includes" | tee -a $filename_summary
filename_PHY_REGS="${filename}_PHY_REGS.txt"
touch $filename_PHY_REGS
grep "PHY_REGS" $filename_WARNING > $filename_PHY_REGS
(printf "PHY_REGS WARNING count:       , "; grep -c "PHY_REGS" $filename_PHY_REGS) | tee -a $filename_summary
(printf "containing mc0: 0 counts:     , "; grep -c "mc0: 0," $filename_PHY_REGS) | tee -a $filename_summary
(printf "containing mc1: 0 counts:     , "; grep -c "mc1: 0," $filename_PHY_REGS) | tee -a $filename_summary
# STOP_HERE   # uncomment to stop execution here
echo "Rest of programming ERROR count not further defined" | tee -a $filename_summary
# if non explainable high programming error count, check filespace on sled manager!

echo "--------------------------" | tee -a $filename_summary
echo "counting UDP_SEND" | tee -a $filename_summary
filename_UDP_SEND_DONE="${filename}_UDP_SEND_DONE.txt"
# echo "filename_UDP_SEND_DONE = ${filename_UDP_SEND_DONE}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_UDP_SEND_DONE
grep -i -A 1 -B 10 "UDP Tx DONE with bandwidth =" temporary_file.txt > $filename_UDP_SEND_DONE

filename_UDP_SEND_SUCCESS="${filename}_UDP_SEND_SUCCESS.txt"
# echo "filename_UDP_SEND_SUCCESS = ${filename_UDP_SEND_SUCCESS}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_UDP_SEND_SUCCESS
grep -A 1 -B 10 -P '^(?!.* 0.0 Mb/s).*UDP Tx DONE with bandwidth =' $filename_UDP_SEND_DONE > $filename_UDP_SEND_SUCCESS

filename_UDP_SEND_SUCCESS_SIZES="${filename}_UDP_SEND_SUCCESS_SIZES.txt"
touch $filename_UDP_SEND_SUCCESS_SIZES
grep "size = " $filename_UDP_SEND_SUCCESS > $filename_UDP_SEND_SUCCESS_SIZES
(printf "UDP_SEND success count:       , "; grep -c "size = " $filename_UDP_SEND_SUCCESS_SIZES) | tee -a $filename_summary

filename_UDP_SEND_SUCCESS_BANDWIDTH="${filename}_UDP_SEND_SUCCESS_BANDWIDTH.txt"
touch $filename_UDP_SEND_SUCCESS_BANDWIDTH
grep "bandwidth = " $filename_UDP_SEND_SUCCESS > $filename_UDP_SEND_SUCCESS_BANDWIDTH
(printf "UDP_SEND BANDWIDTH count:     , "; grep -c "bandwidth = " $filename_UDP_SEND_SUCCESS_BANDWIDTH) | tee -a $filename_summary

filename_UDP_SEND_1_ERROR="${filename}_UDP_SEND_1_ERROR.txt"
touch $filename_UDP_SEND_1_ERROR
grep -A 6 -B 10 "UDP_SEND_1 ERROR" temporary_file.txt > $filename_UDP_SEND_1_ERROR
(printf "UDP_SEND_1 ERROR count:       , "; grep -c "UDP_SEND_1 ERROR" $filename_UDP_SEND_1_ERROR) | tee -a $filename_summary

filename_UDP_SEND_2_ERROR="${filename}_UDP_SEND_2_ERROR.txt"
touch $filename_UDP_SEND_2_ERROR
grep -A 6 -B 10 "UDP_SEND_2 ERROR" temporary_file.txt > $filename_UDP_SEND_2_ERROR
(printf "UDP_SEND_2 ERROR count:       , "; grep -c "UDP_SEND_2 ERROR" $filename_UDP_SEND_2_ERROR) | tee -a $filename_summary

echo "--------------------------" | tee -a $filename_summary
echo "counting UDP_RECV" | tee -a $filename_summary
filename_UDP_RECV_DONE="${filename}_UDP_RECV_DONE.txt"
# echo "filename_UDP_RECV_DONE = ${filename_UDP_RECV_DONE}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_UDP_RECV_DONE
grep -i -A 1 -B 10 "UDP Rx DONE with bandwidth =" temporary_file.txt > $filename_UDP_RECV_DONE

filename_UDP_RECV_SUCCESS="${filename}_UDP_RECV_SUCCESS.txt"
# echo "filename_UDP_RECV_SUCCESS = ${filename_UDP_RECV_SUCCESS}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_UDP_RECV_SUCCESS
grep -A 1 -B 10 -P '^(?!.* 0.0 Mb/s).*UDP Rx DONE with bandwidth =' $filename_UDP_RECV_DONE > $filename_UDP_RECV_SUCCESS

filename_UDP_RECV_SUCCESS_SIZES="${filename}_UDP_RECV_SUCCESS_SIZES.txt"
touch $filename_UDP_RECV_SUCCESS_SIZES
grep "size = " $filename_UDP_RECV_SUCCESS > $filename_UDP_RECV_SUCCESS_SIZES
(printf "UDP_RECV success count:       , "; grep -c "size = " $filename_UDP_RECV_SUCCESS_SIZES) | tee -a $filename_summary

filename_UDP_RECV_SUCCESS_BANDWIDTH="${filename}_UDP_RECV_SUCCESS_BANDWIDTH.txt"
touch $filename_UDP_RECV_SUCCESS_BANDWIDTH
grep "bandwidth = " $filename_UDP_RECV_SUCCESS > $filename_UDP_RECV_SUCCESS_BANDWIDTH
(printf "UDP_RECV BANDWIDTH count:     , "; grep -c "bandwidth = " $filename_UDP_RECV_SUCCESS_BANDWIDTH) | tee -a $filename_summary

filename_UDP_RECV_1_ERROR="${filename}_UDP_RECV_1_ERROR.txt"
touch $filename_UDP_RECV_1_ERROR
grep -A 6 -B 10 "UDP_RECV_1 ERROR" temporary_file.txt > $filename_UDP_RECV_1_ERROR
(printf "UDP_RECV_1 ERROR count:       , "; grep -c "UDP_RECV_1 ERROR" $filename_UDP_RECV_1_ERROR) | tee -a $filename_summary

filename_UDP_RECV_2_ERROR="${filename}_UDP_RECV_2_ERROR.txt"
touch $filename_UDP_RECV_2_ERROR
grep -A 6 -B 10 "UDP_RECV_2 ERROR" temporary_file.txt > $filename_UDP_RECV_2_ERROR
(printf "UDP_RECV_2 ERROR count:       , "; grep -c "UDP_RECV_2 ERROR" $filename_UDP_RECV_2_ERROR) | tee -a $filename_summary

filename_UDP_RECV_3_ERROR="${filename}_UDP_RECV_3_ERROR.txt"
touch $filename_UDP_RECV_3_ERROR
grep -A 6 -B 10 "UDP_RECV_3 ERROR" temporary_file.txt > $filename_UDP_RECV_3_ERROR
(printf "UDP_RECV_3 ERROR count:       , "; grep -c "UDP_RECV_3 ERROR" $filename_UDP_RECV_3_ERROR) | tee -a $filename_summary

echo "--------------------------" | tee -a $filename_summary
echo "counting UDP_ECHO" | tee -a $filename_summary
filename_UDP_SEND_ECHO_DONE="${filename}_UDP_SEND_ECHO_DONE.txt"
# echo "filename_UDP_SEND_ECHO_DONE = ${filename_UDP_SEND_ECHO_DONE}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_UDP_SEND_ECHO_DONE
grep -A 1 -B 12 -P '^(?!.* 0.0 Mb/s).*UDP TX DONE with bandwidth =' temporary_file.txt > $filename_UDP_SEND_ECHO_DONE

filename_UDP_SEND_ECHO_SUCCESS="${filename}_UDP_SEND_ECHO_SUCCESS.txt"
# echo "filename_UDP_SEND_ECHO_SUCCESS = ${filename_UDP_SEND_ECHO_SUCCESS}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_UDP_SEND_ECHO_SUCCESS
grep -A 1 -B 12 -P '^(?!.* 0.0 Mb/s).*UDP TX DONE with bandwidth =' $filename_UDP_SEND_ECHO_DONE > $filename_UDP_SEND_ECHO_SUCCESS

filename_UDP_SEND_ECHO_SUCCESS_SIZES="${filename}_UDP_SEND_ECHO_SUCCESS_SIZES.txt"
touch $filename_UDP_SEND_ECHO_SUCCESS_SIZES
grep "size = " $filename_UDP_SEND_ECHO_SUCCESS > $filename_UDP_SEND_ECHO_SUCCESS_SIZES
(printf "UDP_SEND_ECHO success count:  , "; grep -c "size = " $filename_UDP_SEND_ECHO_SUCCESS_SIZES) | tee -a $filename_summary

filename_UDP_SEND_ECHO_SUCCESS_BANDWIDTH="${filename}_UDP_SEND_ECHO_SUCCESS_BANDWIDTH.txt"
touch $filename_UDP_SEND_ECHO_SUCCESS_BANDWIDTH
grep "UDP TX DONE with bandwidth =" $filename_UDP_SEND_ECHO_SUCCESS > $filename_UDP_SEND_ECHO_SUCCESS_BANDWIDTH
(printf "UDP_SEND_ECHO BANDWIDTH count:, "; grep -c "bandwidth = " $filename_UDP_SEND_ECHO_SUCCESS_BANDWIDTH) | tee -a $filename_summary

filename_UDP_RECV_ECHO_DONE="${filename}_UDP_RECV_ECHO_DONE.txt"
# echo "filename_UDP_RECV_ECHO_DONE = ${filename_UDP_RECV_ECHO_DONE}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_UDP_RECV_ECHO_DONE
grep -A 1 -B 12 -P '^(?!.* 0.0 Mb/s).*UDP RX DONE with bandwidth =' temporary_file.txt > $filename_UDP_RECV_ECHO_DONE

filename_UDP_RECV_ECHO_SUCCESS="${filename}_UDP_RECV_ECHO_SUCCESS.txt"
# echo "filename_UDP_RECV_ECHO_SUCCESS = ${filename_UDP_RECV_ECHO_SUCCESS}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_UDP_RECV_ECHO_SUCCESS
grep -A 1 -B 12 -P '^(?!.* 0.0 Mb/s).*UDP RX DONE with bandwidth =' $filename_UDP_RECV_ECHO_DONE > $filename_UDP_RECV_ECHO_SUCCESS

filename_UDP_RECV_ECHO_SUCCESS_SIZES="${filename}_UDP_RECV_ECHO_SUCCESS_SIZES.txt"
touch $filename_UDP_RECV_ECHO_SUCCESS_SIZES
grep "size = " $filename_UDP_RECV_ECHO_SUCCESS > $filename_UDP_RECV_ECHO_SUCCESS_SIZES
(printf "UDP_RECV_ECHO success count:  , "; grep -c "size = " $filename_UDP_RECV_ECHO_SUCCESS_SIZES) | tee -a $filename_summary

filename_UDP_RECV_ECHO_SUCCESS_BANDWIDTH="${filename}_UDP_RECV_ECHO_SUCCESS_BANDWIDTH.txt"
touch $filename_UDP_RECV_ECHO_SUCCESS_BANDWIDTH
grep "UDP RX DONE with bandwidth =" $filename_UDP_RECV_ECHO_SUCCESS > $filename_UDP_RECV_ECHO_SUCCESS_BANDWIDTH
(printf "UDP_RECV_ECHO BANDWIDTH count:, "; grep -c "bandwidth = " $filename_UDP_RECV_ECHO_SUCCESS_BANDWIDTH) | tee -a $filename_summary

filename_UDP_ECHO_1_ERROR="${filename}_UDP_ECHO_1_ERROR.txt"
touch $filename_UDP_ECHO_1_ERROR
grep -A 6 -B 10 "UDP_ECHO_1 ERROR" temporary_file.txt > $filename_UDP_ECHO_1_ERROR
(printf "UDP_ECHO_1 ERROR count:       , "; grep -c "UDP_ECHO_1 ERROR" $filename_UDP_ECHO_1_ERROR) | tee -a $filename_summary
filename_UDP_ECHO_2_ERROR="${filename}_UDP_ECHO_2_ERROR.txt"
touch $filename_UDP_ECHO_2_ERROR
grep -A 6 -B 10 "UDP_ECHO_2 ERROR" temporary_file.txt > $filename_UDP_ECHO_2_ERROR
(printf "UDP_ECHO_2 ERROR count:       , "; grep -c "UDP_ECHO_2 ERROR" $filename_UDP_ECHO_2_ERROR) | tee -a $filename_summary

echo "--------------------------" | tee -a $filename_summary
echo "counting TCP_SEND" | tee -a $filename_summary
filename_TCP_SEND_DONE="${filename}_TCP_SEND_DONE.txt"
# echo "filename_TCP_SEND_DONE = ${filename_TCP_SEND_DONE}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_TCP_SEND_DONE
grep -A 1 -B 10 "TCP Tx DONE with bandwidth =" temporary_file.txt > $filename_TCP_SEND_DONE
filename_TCP_SEND_SUCCESS="${filename}_TCP_SEND_SUCCESS.txt"
# echo "filename_TCP_SEND_SUCCESS = ${filename_TCP_SEND_SUCCESS}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_TCP_SEND_SUCCESS
grep -A 1 -B 10 -P '^(?!.* 0.0 Mb/s).*TCP Tx DONE with bandwidth =' $filename_TCP_SEND_DONE > $filename_TCP_SEND_SUCCESS
filename_TCP_SEND_SUCCESS_SIZES="${filename}_TCP_SEND_SUCCESS_SIZES.txt"
touch $filename_TCP_SEND_SUCCESS_SIZES
grep "size = " $filename_TCP_SEND_SUCCESS > $filename_TCP_SEND_SUCCESS_SIZES
(printf "TCP_SEND success count:       , "; grep -c "size = " $filename_TCP_SEND_SUCCESS_SIZES) | tee -a $filename_summary

filename_TCP_SEND_SUCCESS_BANDWIDTH="${filename}_TCP_SEND_SUCCESS_BANDWIDTH.txt"
touch $filename_TCP_SEND_SUCCESS_BANDWIDTH
grep "bandwidth = " $filename_TCP_SEND_SUCCESS > $filename_TCP_SEND_SUCCESS_BANDWIDTH
(printf "TCP_SEND BANDWIDTH count:     , "; grep -c "bandwidth = " $filename_TCP_SEND_SUCCESS_BANDWIDTH) | tee -a $filename_summary

filename_TCP_SEND_1_ERROR="${filename}_TCP_SEND_1_ERROR.txt"
touch $filename_TCP_SEND_1_ERROR
grep -A 6 -B 10 "TCP_SEND_1 ERROR" temporary_file.txt > $filename_TCP_SEND_1_ERROR
(printf "TCP_SEND_1 ERROR count:       , "; grep -c "TCP_SEND_1 ERROR" $filename_TCP_SEND_1_ERROR) | tee -a $filename_summary

filename_TCP_SEND_2_ERROR="${filename}_TCP_SEND_2_ERROR.txt"
touch $filename_TCP_SEND_2_ERROR
grep -A 6 -B 10 "TCP_SEND_2 ERROR" temporary_file.txt > $filename_TCP_SEND_2_ERROR
(printf "TCP_SEND_2 ERROR count:       , "; grep -c "TCP_SEND_2 ERROR" $filename_TCP_SEND_2_ERROR) | tee -a $filename_summary

echo "--------------------------" | tee -a $filename_summary
echo "counting TCP_RECV" | tee -a $filename_summary
filename_TCP_RECV_DONE="${filename}_TCP_RECV_DONE.txt"
# echo "filename_TCP_RECV_DONE = ${filename_TCP_RECV_DONE}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_TCP_RECV_DONE
grep -A 1 -B 10 "TCP Rx DONE with bandwidth =" temporary_file.txt > $filename_TCP_RECV_DONE
filename_TCP_RECV_SUCCESS="${filename}_TCP_RECV_SUCCESS.txt"
# echo "filename_TCP_RECV_SUCCESS = ${filename_TCP_RECV_SUCCESS}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_TCP_RECV_SUCCESS
grep -A 1 -B 10 -P '^(?!.* 0.0 Mb/s).*TCP Rx DONE with bandwidth =' $filename_TCP_RECV_DONE > $filename_TCP_RECV_SUCCESS
filename_TCP_RECV_SUCCESS_SIZES="${filename}_TCP_RECV_SUCCESS_SIZES.txt"
touch $filename_TCP_RECV_SUCCESS_SIZES
grep "size = " $filename_TCP_RECV_SUCCESS > $filename_TCP_RECV_SUCCESS_SIZES
(printf "TCP_RECV success count:       , "; grep -c "size = " $filename_TCP_RECV_SUCCESS_SIZES) | tee -a $filename_summary
filename_TCP_RECV_SUCCESS_BANDWIDTH="${filename}_TCP_RECV_SUCCESS_BANDWIDTH.txt"
touch $filename_TCP_RECV_SUCCESS_BANDWIDTH
grep "bandwidth = " $filename_TCP_RECV_SUCCESS > $filename_TCP_RECV_SUCCESS_BANDWIDTH
(printf "TCP_RECV BANDWIDTH count:     , "; grep -c "bandwidth = " $filename_TCP_RECV_SUCCESS_BANDWIDTH) | tee -a $filename_summary

filename_TCP_RECV_1_ERROR="${filename}_TCP_RECV_1_ERROR.txt"
touch $filename_TCP_RECV_1_ERROR
grep -A 6 -B 10 "TCP_RECV_1 ERROR" temporary_file.txt > $filename_TCP_RECV_1_ERROR
(printf "TCP_RECV_1 ERROR count:       , "; grep -c "TCP_RECV_1 ERROR" $filename_TCP_RECV_1_ERROR) | tee -a $filename_summary
filename_TCP_RECV_2_ERROR="${filename}_TCP_RECV_2_ERROR.txt"
touch $filename_TCP_RECV_2_ERROR
grep -A 6 -B 10 "TCP_RECV_2 ERROR" temporary_file.txt > $filename_TCP_RECV_2_ERROR
(printf "TCP_RECV_2 ERROR count:       , "; grep -c "TCP_RECV_2 ERROR" $filename_TCP_RECV_2_ERROR) | tee -a $filename_summary
filename_TCP_RECV_3_ERROR="${filename}_TCP_RECV_3_ERROR.txt"
touch $filename_TCP_RECV_3_ERROR
grep -A 6 -B 10 "TCP_RECV_3 ERROR" temporary_file.txt > $filename_TCP_RECV_3_ERROR
(printf "TCP_RECV_3 ERROR count:       , "; grep -c "TCP_RECV_3 ERROR" $filename_TCP_RECV_3_ERROR) | tee -a $filename_summary
filename_TCP_RECV_4_ERROR="${filename}_TCP_RECV_4_ERROR.txt"
touch $filename_TCP_RECV_4_ERROR
grep -A 6 -B 10 "TCP_RECV_4 ERROR" temporary_file.txt > $filename_TCP_RECV_4_ERROR
(printf "TCP_RECV_4 ERROR count:       , "; grep -c "TCP_RECV_4 ERROR" $filename_TCP_RECV_4_ERROR) | tee -a $filename_summary

echo "--------------------------" | tee -a $filename_summary
echo "counting TCP_TXRX_ECHO" | tee -a $filename_summary
filename_TCP_TXRX_ECHO_SUCCESS="${filename}_TCP_TXRX_ECHO_SUCCESS.txt"
# echo "filename_TCP_TXRX_ECHO_SUCCESS = ${filename_TCP_TXRX_ECHO_SUCCESS}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_TCP_TXRX_ECHO_SUCCESS
grep -A 1 -B 12 -P '^(?!.* 0.0 Mb/s).*TCP Tx/Rx DONE with bandwidth =' temporary_file.txt > $filename_TCP_TXRX_ECHO_SUCCESS

filename_TCP_TXRX_ECHO_SUCCESS_SIZES="${filename}_TCP_TXRX_ECHO_SUCCESS_SIZES.txt"
touch $filename_TCP_TXRX_ECHO_SUCCESS_SIZES
grep "size = " $filename_TCP_TXRX_ECHO_SUCCESS > $filename_TCP_TXRX_ECHO_SUCCESS_SIZES
(printf "TCP_TXRX_ECHO success count:  , "; grep -c "size = " $filename_TCP_TXRX_ECHO_SUCCESS_SIZES) | tee -a $filename_summary
(printf "TCP_TXRX_ECHO sizes count:    , "; grep -c "size = " $filename_TCP_TXRX_ECHO_SUCCESS_SIZES) | tee -a $filename_summary

filename_TCP_TXRX_ECHO_SUCCESS_BANDWIDTH="${filename}_TCP_TXRX_ECHO_SUCCESS_BANDWIDTH.txt"
touch $filename_TCP_TXRX_ECHO_SUCCESS_BANDWIDTH
grep "TCP Tx/Rx DONE with bandwidth =" $filename_TCP_TXRX_ECHO_SUCCESS > $filename_TCP_TXRX_ECHO_SUCCESS_BANDWIDTH
(printf "TCP_TXRX_ECHO BANDWIDTH count:, "; grep -c "bandwidth = " $filename_TCP_TXRX_ECHO_SUCCESS_BANDWIDTH) | tee -a $filename_summary

filename_TCP_ECHO_1_ERROR="${filename}_TCP_ECHO_1_ERROR.txt"
touch $filename_TCP_ECHO_1_ERROR
grep -A 6 -B 10 "TCP_ECHO_1 ERROR" temporary_file.txt > $filename_TCP_ECHO_1_ERROR
(printf "TCP_ECHO_1 ERROR count:       , "; grep -c "TCP_ECHO_1 ERROR" $filename_TCP_ECHO_1_ERROR) | tee -a $filename_summary
filename_TCP_ECHO_2_ERROR="${filename}_TCP_ECHO_2_ERROR.txt"
touch $filename_TCP_ECHO_2_ERROR
grep -A 6 -B 10 "TCP_ECHO_2 ERROR" temporary_file.txt > $filename_TCP_ECHO_2_ERROR
(printf "TCP_ECHO_2 ERROR count:       , "; grep -c "TCP_ECHO_2 ERROR" $filename_TCP_ECHO_2_ERROR) | tee -a $filename_summary

echo "--------------------------" | tee -a $filename_summary
echo "counting SOCKET_ERROR" | tee -a $filename_summary
filename_SOCKET_ERROR="${filename}_SOCKET_ERROR.txt"
touch $filename_SOCKET_ERROR
grep -A 6 -B 10 "Socket reading error" temporary_file.txt > $filename_SOCKET_ERROR
(printf "SOCKET ERROR count:           , "; grep -c "Socket reading error" $filename_SOCKET_ERROR) | tee -a $filename_summary
# echo "which includes"
# echo "Rest of SOCKET_ERROR count not further defined"
filename_SOCKET_ERROR_SIZES="${filename}_SOCKET_ERROR_SIZES.txt"
touch $filename_SOCKET_ERROR_SIZES
grep "size = " $filename_SOCKET_ERROR > $filename_SOCKET_ERROR_SIZES
(printf "SOCKET ERROR SIZES count:     , "; grep -c "size = " $filename_SOCKET_ERROR_SIZES) | tee -a $filename_summary

filename_SOCKET_ERROR_SIZE_NUMBERS="${filename}_SOCKET_ERROR_SIZE_NUMBERS.txt"
touch $filename_SOCKET_ERROR_SIZE_NUMBERS
# echo "a problem with awk ...."
awk -F'[=\n]' '{print $2}' $filename_SOCKET_ERROR_SIZES > $filename_SOCKET_ERROR_SIZE_NUMBERS
# (printf "SOCKET_ERROR_SIZES_NUMBERS count:    , "; grep -c "size = " $filename_SOCKET_ERROR_SIZE_NUMBERS) | tee -a $filename_summary

rm temporary_file.txt

if $DELETE_FILES ; then
    rm $filename_IMAGE_ID
    rm $filename_ROUND_NR
    rm $filename_END_OF_TESTS
    rm $filename_AVAILABLE
    rm $filename_NO_RESPONSE
    rm $filename_ERROR
    rm $filename_JTAG_ERROR
    rm $filename_programming_ERROR
    rm $filename_WARNING
    rm $filename_PHY_REGS
    rm $filename_UDP_SEND_DONE
    rm $filename_UDP_SEND_SUCCESS
    rm $filename_UDP_SEND_SUCCESS_SIZES
    rm $filename_UDP_SEND_SUCCESS_BANDWIDTH
    rm $filename_UDP_SEND_1_ERROR
    rm $filename_UDP_SEND_2_ERROR
    rm $filename_UDP_RECV_DONE
    rm $filename_UDP_RECV_SUCCESS
    rm $filename_UDP_RECV_SUCCESS_SIZES
    rm $filename_UDP_RECV_SUCCESS_BANDWIDTH
    rm $filename_UDP_RECV_1_ERROR
    rm $filename_UDP_RECV_2_ERROR
    rm $filename_UDP_RECV_3_ERROR
    rm $filename_UDP_SEND_ECHO_DONE
    rm $filename_UDP_SEND_ECHO_SUCCESS
    rm $filename_UDP_SEND_ECHO_SUCCESS_SIZES
    rm $filename_UDP_SEND_ECHO_SUCCESS_BANDWIDTH
    rm $filename_UDP_RECV_ECHO_DONE
    rm $filename_UDP_RECV_ECHO_SUCCESS
    rm $filename_UDP_RECV_ECHO_SUCCESS_SIZES
    rm $filename_UDP_RECV_ECHO_SUCCESS_BANDWIDTH
    rm $filename_UDP_ECHO_1_ERROR
    rm $filename_UDP_ECHO_2_ERROR
    rm $filename_TCP_SEND_DONE
    rm $filename_TCP_SEND_SUCCESS
    rm $filename_TCP_SEND_SUCCESS_SIZES
    rm $filename_TCP_SEND_SUCCESS_BANDWIDTH
    rm $filename_TCP_SEND_1_ERROR
    rm $filename_TCP_SEND_2_ERROR
    rm $filename_TCP_RECV_DONE
    rm $filename_TCP_RECV_SUCCESS
    rm $filename_TCP_RECV_SUCCESS_SIZES
    rm $filename_TCP_RECV_SUCCESS_BANDWIDTH
    rm $filename_TCP_RECV_1_ERROR
    rm $filename_TCP_RECV_2_ERROR
    rm $filename_TCP_RECV_3_ERROR
    rm $filename_TCP_RECV_4_ERROR
    rm $filename_TCP_TXRX_ECHO_SUCCESS
    rm $filename_TCP_TXRX_ECHO_SUCCESS_SIZES
    rm $filename_TCP_TXRX_ECHO_SUCCESS_BANDWIDTH
    rm $filename_TCP_ECHO_1_ERROR
    rm $filename_TCP_ECHO_2_ERROR
    rm $filename_SOCKET_ERROR
    rm $filename_SOCKET_ERROR_SIZES
    rm $filename_SOCKET_ERROR_SIZE_NUMBERS
fi
