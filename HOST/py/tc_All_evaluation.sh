#!/bin/bash
# Note important difference between /bin/sh and /bin/bash
# https://askubuntu.com/q/141928/295286

# This script evaluates a tc_*ALL*.log file. It writes the summary of analysis for each test 
# TCP_SEND TCP_RECV TCP_ECHO UDP_SEND UDP_RECV UDP_ECHO IPERF
# in separated files for further evaluation in a spreadsheet or similar

# define functions (the function definition must be placed before any calls to the function)
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

file=${1}
if [ -f "$1" ]
then
    echo " "
    echo "going to evaluate ${file}"
    echo " "
#    echo "new file is going to be named ${file/.*}_success.txt"
else
    echo "file does not exist"
    echo "usage: tc_All_evaluation.sh filename.log"
    exit
fi

filename=${file/.*}
ext="${file#*.}"

if [[ 0 == 1 ]]  # set 0 == 1 to disable, 1 == 1 to enable this check
then
	# if [ ${file: -4} == ".log" ]
	if [[ $file == *"ALL"*".log" ]]
	then
	   echo "file = ${file}"
	   echo "filename = ${filename}"
	   echo "ext = ${ext}"
	else
		echo "usage: tc_All_evaluation.sh filename.log"
		echo "the file has to be a tc_*ALL*.log file"
	fi
fi
grep -v "STOP_ERROR" $file > temporary_file.txt

filename_ROUND_NR="${filename}-ROUND_NR.txt"
# echo "filename_ROUND_NR = ${filename_ROUND_NR}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_ROUND_NR
grep "round nr." temporary_file.txt > $filename_ROUND_NR
printf "ROUND NR. count:               "; grep -c "round nr." $filename_ROUND_NR
# STOP_HERE   # uncomment to stop execution here

filename_END_OF_TESTS="${filename}-END_OF_TESTS.txt"
# echo "filename_END_OF_TESTS = ${filename_END_OF_TESTS}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_END_OF_TESTS
grep -A 1 "END OF TESTS" temporary_file.txt > temp_file.txt
grep "RESOURCE_ID =" temp_file.txt > $filename_END_OF_TESTS
rm temp_file.txt
printf "END OF TESTS count:            "; grep -c "RESOURCE_ID" $filename_END_OF_TESTS
# STOP_HERE   # uncomment to stop execution here

filename_AVAILABLE="${filename}_AVAILABLE.txt"
touch $filename_AVAILABLE
grep "is AVAILABLE" temporary_file.txt > $filename_AVAILABLE
printf "is AVAILABLE count:            "; grep -c "is AVAILABLE" $filename_AVAILABLE
# STOP_HERE   # uncomment to stop execution here

filename_NO_RESPONSE="${filename}_NO_RESPONSE.txt"
touch $filename_NO_RESPONSE
grep "NO RESPONSE" temporary_file.txt > $filename_NO_RESPONSE
printf "NO RESPONSE count:             "; grep -c "NO RESPONSE" $filename_NO_RESPONSE
# STOP_HERE   # uncomment to stop execution here

echo "--------------------------"
echo "counting programming ERROR"
filename_ERROR="${filename}_ERROR.txt"
touch $filename_ERROR
grep "ERROR " temporary_file.txt > $filename_ERROR
printf "ERROR count:                   "; grep -c "ERROR" $filename_ERROR
# STOP_HERE   # uncomment to stop execution here
filename_JTAG_ERROR="${filename}_JTAG_ERROR.txt"
touch $filename_JTAG_ERROR
grep "JTAG" $filename_ERROR > $filename_JTAG_ERROR
printf "JTAG ERROR count:              "; grep -c "JTAG" $filename_JTAG_ERROR
# STOP_HERE   # uncomment to stop execution here

filename_programming_ERROR="${filename}-programming_ERROR.txt"
grep "programming ERROR" temporary_file.txt > $filename_programming_ERROR
printf "programming ERROR count:       "; grep -c "programming ERROR" $filename_programming_ERROR
# STOP_HERE   # uncomment to stop execution here
echo "which includes"
filename_WARNING="${filename}_WARNING.txt"
touch $filename_WARNING
grep "WARNING" temporary_file.txt > $filename_WARNING
printf "WARNING count:                 "; grep -c "WARNING" $filename_WARNING
# STOP_HERE   # uncomment to stop execution here
echo "which includes"
filename_PHY_REGS="${filename}_PHY_REGS.txt"
touch $filename_PHY_REGS
grep "PHY_REGS" $filename_WARNING > $filename_PHY_REGS
printf "PHY_REGS WARNING count:        "; grep -c "PHY_REGS" $filename_PHY_REGS
# STOP_HERE   # uncomment to stop execution here
echo "Rest of programming ERROR count not further defined"
# if non explainable high programming error count, check filespace on sled manager!

echo "--------------------------"
echo "counting TCP_SEND"
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
printf "TCP_SEND success count:        "; grep -c "size = " $filename_TCP_SEND_SUCCESS_SIZES

filename_TCP_SEND_SUCCESS_BANDWIDTH="${filename}_TCP_SEND_SUCCESS_BANDWIDTH.txt"
touch $filename_TCP_SEND_SUCCESS_BANDWIDTH
grep "bandwidth = " $filename_TCP_SEND_SUCCESS > $filename_TCP_SEND_SUCCESS_BANDWIDTH
printf "TCP_SEND BANDWIDTH count:      "; grep -c "bandwidth = " $filename_TCP_SEND_SUCCESS_BANDWIDTH

echo "--------------------------"
echo "counting TCP_RECV"
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
printf "TCP_RECV success count:        "; grep -c "size = " $filename_TCP_RECV_SUCCESS_SIZES

filename_TCP_RECV_SUCCESS_BANDWIDTH="${filename}_TCP_RECV_SUCCESS_BANDWIDTH.txt"
touch $filename_TCP_RECV_SUCCESS_BANDWIDTH
grep "bandwidth = " $filename_TCP_RECV_SUCCESS > $filename_TCP_RECV_SUCCESS_BANDWIDTH
printf "TCP_RECV BANDWIDTH count:      "; grep -c "bandwidth = " $filename_TCP_RECV_SUCCESS_BANDWIDTH

echo "--------------------------"
echo "counting UDP_SEND"
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
printf "UDP_SEND success count:        "; grep -c "size = " $filename_UDP_SEND_SUCCESS_SIZES

filename_UDP_SEND_SUCCESS_BANDWIDTH="${filename}_UDP_SEND_SUCCESS_BANDWIDTH.txt"
touch $filename_UDP_SEND_SUCCESS_BANDWIDTH
grep "bandwidth = " $filename_UDP_SEND_SUCCESS > $filename_UDP_SEND_SUCCESS_BANDWIDTH
printf "UDP_SEND BANDWIDTH count:      "; grep -c "bandwidth = " $filename_UDP_SEND_SUCCESS_BANDWIDTH

echo "--------------------------"
echo "counting UDP_RECV"
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
printf "UDP_RECV success count:        "; grep -c "size = " $filename_UDP_RECV_SUCCESS_SIZES

filename_UDP_RECV_SUCCESS_BANDWIDTH="${filename}_UDP_RECV_SUCCESS_BANDWIDTH.txt"
touch $filename_UDP_RECV_SUCCESS_BANDWIDTH
grep "bandwidth = " $filename_UDP_RECV_SUCCESS > $filename_UDP_RECV_SUCCESS_BANDWIDTH
printf "UDP_RECV BANDWIDTH count:      "; grep -c "bandwidth = " $filename_UDP_RECV_SUCCESS_BANDWIDTH

echo "--------------------------"
echo "counting UDP_ECHO"
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
printf "UDP_SEND_ECHO success count:   "; grep -c "size = " $filename_UDP_SEND_ECHO_SUCCESS_SIZES

filename_UDP_SEND_ECHO_SUCCESS_BANDWIDTH="${filename}_UDP_SEND_ECHO_SUCCESS_BANDWIDTH.txt"
touch $filename_UDP_SEND_ECHO_SUCCESS_BANDWIDTH
grep "UDP TX DONE with bandwidth =" $filename_UDP_SEND_ECHO_SUCCESS > $filename_UDP_SEND_ECHO_SUCCESS_BANDWIDTH
printf "UDP_SEND_ECHO BANDWIDTH count: "; grep -c "bandwidth = " $filename_UDP_SEND_ECHO_SUCCESS_BANDWIDTH

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
printf "UDP_RECV_ECHO success count:   "; grep -c "size = " $filename_UDP_RECV_ECHO_SUCCESS_SIZES

filename_UDP_RECV_ECHO_SUCCESS_BANDWIDTH="${filename}_UDP_RECV_ECHO_SUCCESS_BANDWIDTH.txt"
touch $filename_UDP_RECV_ECHO_SUCCESS_BANDWIDTH
grep "UDP RX DONE with bandwidth =" $filename_UDP_RECV_ECHO_SUCCESS > $filename_UDP_RECV_ECHO_SUCCESS_BANDWIDTH
printf "UDP_RECV_ECHO BANDWIDTH count: "; grep -c "bandwidth = " $filename_UDP_RECV_ECHO_SUCCESS_BANDWIDTH

echo "--------------------------"
echo "counting SOCKET_ERROR"
filename_SOCKET_ERROR="${filename}_SOCKET_ERROR.txt"
touch $filename_SOCKET_ERROR
grep -A 6 -B 10 "Socket reading error" temporary_file.txt > $filename_SOCKET_ERROR
printf "SOCKET ERROR count:            "; grep -c "Socket reading error" $filename_SOCKET_ERROR
echo "which includes"
filename_UDP_RECV_3_ERROR="${filename}_UDP_RECV_3_ERROR.txt"
touch $filename_UDP_RECV_3_ERROR
grep -A 6 -B 10 "UDP_RECV_3 ERROR" temporary_file.txt > $filename_UDP_RECV_3_ERROR
printf "UDP_RECV_3 ERROR count:        "; grep -c "UDP_RECV_3 ERROR" $filename_UDP_RECV_3_ERROR
echo "Rest of SOCKET_ERROR count not further defined"
filename_SOCKET_ERROR_SIZES="${filename}_SOCKET_ERROR_SIZES.txt"
touch $filename_SOCKET_ERROR_SIZES
grep "size = " $filename_SOCKET_ERROR > $filename_SOCKET_ERROR_SIZES
printf "SOCKET ERROR SIZES count:      "; grep -c "size = " $filename_SOCKET_ERROR_SIZES

filename_SOCKET_ERROR_SIZE_NUMBERS="${filename}_SOCKET_ERROR_SIZE_NUMBERS.txt"
touch $filename_SOCKET_ERROR_SIZE_NUMBERS
# echo "a problem with awk ...."
awk -F'[=\n]' '{print $2}' $filename_SOCKET_ERROR_SIZES > $filename_SOCKET_ERROR_SIZE_NUMBERS
# printf "SOCKET_ERROR_SIZES_NUMBERS count:     "; grep -c "size = " $filename_SOCKET_ERROR_SIZE_NUMBERS

rm temporary_file.txt
