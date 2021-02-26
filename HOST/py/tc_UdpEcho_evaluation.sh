#!/bin/bash
# Note important difference between /bin/sh and /bin/bash
# https://askubuntu.com/q/141928/295286

# This script evaluates a tc_*UDP_ECHO*.log file. It writes the summary of analysis for 
# UDP_ECHO tests 
# in separated files for further evaluation in a spreadsheet or similar

# define functions (the function definition must be placed before any calls to the function)
# STOP_HERE function
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
    echo "usage: tc_tc_UdpEcho_evaluation.sh filename.log"
    exit
fi

filename=${file/.*}
ext="${file#*.}"

if [[ 0 == 1 ]]  # set 0 == 1 to disable, 1 == 1 to enable this check
then
	# if [ ${file: -4} == ".log" ]
	if [[ $file == *"UDP_ECHO"*".log" ]]
	then
	   echo "file = ${file}"
	   echo "filename = ${filename}"
	   echo "ext = ${ext}"
	else
		echo "usage: tc_UdpEcho_evaluation.sh filename.log"
		echo "the file has to be a tc_*UDP_ECHO*.log file"
		exit 1
	fi
fi
grep -v "STOP_ERROR" $file > temporary_file.txt

# find image_id
filename_IMAGE_ID="${filename}-IMAGE_ID.txt"
# echo "filename_IMAGE_ID = ${filename_IMAGE_ID}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_IMAGE_ID
grep "Programming with  image_id=" temporary_file.txt > $filename_IMAGE_ID
grep -m1 "Programming with  image_id=" $filename_IMAGE_ID
# STOP_HERE   # uncomment to stop execution here

filename_ROUND_NR="${filename}-ROUND_NR.txt"
# echo "filename_ROUND_NR = ${filename_ROUND_NR}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_ROUND_NR
grep "round nr." temporary_file.txt > $filename_ROUND_NR
printf "ROUND NR. count:              , "; grep -c "round nr." $filename_ROUND_NR
# STOP_HERE   # uncomment to stop execution here

filename_END_OF_TESTS="${filename}-END_OF_TESTS.txt"
# echo "filename_END_OF_TESTS = ${filename_END_OF_TESTS}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_END_OF_TESTS
grep -A 1 "END OF TESTS" temporary_file.txt > temp_file.txt
grep "RESOURCE_ID =" temp_file.txt > $filename_END_OF_TESTS
rm temp_file.txt
printf "END OF TESTS count:           , "; grep -c "RESOURCE_ID" $filename_END_OF_TESTS
# STOP_HERE   # uncomment to stop execution here

filename_AVAILABLE="${filename}_AVAILABLE.txt"
touch $filename_AVAILABLE
grep "is AVAILABLE" temporary_file.txt > $filename_AVAILABLE
printf "is AVAILABLE count:           , "; grep -c "is AVAILABLE" $filename_AVAILABLE
# STOP_HERE   # uncomment to stop execution here

filename_NO_RESPONSE="${filename}_NO_RESPONSE.txt"
touch $filename_NO_RESPONSE
grep "NO RESPONSE" temporary_file.txt > $filename_NO_RESPONSE
printf "NO RESPONSE count:            , "; grep -c "NO RESPONSE" $filename_NO_RESPONSE
# STOP_HERE   # uncomment to stop execution here

echo "--------------------------"
echo "counting programming ERROR"
filename_ERROR="${filename}_ERROR.txt"
touch $filename_ERROR
grep "ERROR " temporary_file.txt > $filename_ERROR
printf "ERROR count:                  , "; grep -c "ERROR" $filename_ERROR
# STOP_HERE   # uncomment to stop execution here
filename_JTAG_ERROR="${filename}_JTAG_ERROR.txt"
touch $filename_JTAG_ERROR
grep "JTAG" $filename_ERROR > $filename_JTAG_ERROR
printf "JTAG ERROR count:             , "; grep -c "JTAG" $filename_JTAG_ERROR
# STOP_HERE   # uncomment to stop execution here

filename_programming_ERROR="${filename}-programming_ERROR.txt"
grep "programming ERROR" temporary_file.txt > $filename_programming_ERROR
printf "programming ERROR count:      , "; grep -c "programming ERROR" $filename_programming_ERROR
# STOP_HERE   # uncomment to stop execution here
echo "which includes"
filename_WARNING="${filename}_WARNING.txt"
touch $filename_WARNING
grep "WARNING" temporary_file.txt > $filename_WARNING
printf "WARNING count:                , "; grep -c "WARNING" $filename_WARNING
# STOP_HERE   # uncomment to stop execution here
echo "which includes"
filename_PHY_REGS="${filename}_PHY_REGS.txt"
touch $filename_PHY_REGS
grep "PHY_REGS" $filename_WARNING > $filename_PHY_REGS
printf "PHY_REGS WARNING count:       , "; grep -c "PHY_REGS" $filename_PHY_REGS
printf "containing mc0: 0 counts:     , "; grep -c "mc0: 0," $filename_PHY_REGS
printf "containing mc1: 0 counts:     , "; grep -c "mc1: 0," $filename_PHY_REGS
# STOP_HERE   # uncomment to stop execution here
echo "Rest of programming ERROR count not further defined"
# if non explainable high programming error count, check filespace on sled manager!

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
printf "UDP_SEND_ECHO success count:  , "; grep -c "size = " $filename_UDP_SEND_ECHO_SUCCESS_SIZES

filename_UDP_SEND_ECHO_SUCCESS_BANDWIDTH="${filename}_UDP_SEND_ECHO_SUCCESS_BANDWIDTH.txt"
touch $filename_UDP_SEND_ECHO_SUCCESS_BANDWIDTH
grep "UDP TX DONE with bandwidth =" $filename_UDP_SEND_ECHO_SUCCESS > $filename_UDP_SEND_ECHO_SUCCESS_BANDWIDTH
printf "UDP_SEND_ECHO BANDWIDTH count:, "; grep -c "bandwidth = " $filename_UDP_SEND_ECHO_SUCCESS_BANDWIDTH

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
printf "UDP_RECV_ECHO success count:  , "; grep -c "size = " $filename_UDP_RECV_ECHO_SUCCESS_SIZES

filename_UDP_RECV_ECHO_SUCCESS_BANDWIDTH="${filename}_UDP_RECV_ECHO_SUCCESS_BANDWIDTH.txt"
touch $filename_UDP_RECV_ECHO_SUCCESS_BANDWIDTH
grep "UDP RX DONE with bandwidth =" $filename_UDP_RECV_ECHO_SUCCESS > $filename_UDP_RECV_ECHO_SUCCESS_BANDWIDTH
printf "UDP_RECV_ECHO BANDWIDTH count:, "; grep -c "bandwidth = " $filename_UDP_RECV_ECHO_SUCCESS_BANDWIDTH

filename_UDP_ECHO_1_ERROR="${filename}_UDP_ECHO_1_ERROR.txt"
touch $filename_UDP_ECHO_1_ERROR
grep -A 6 -B 10 "UDP_ECHO_1 ERROR" temporary_file.txt > $filename_UDP_ECHO_1_ERROR
printf "UDP_ECHO_1 ERROR count:       , "; grep -c "UDP_ECHO_1 ERROR" $filename_UDP_ECHO_1_ERROR
filename_UDP_ECHO_2_ERROR="${filename}_UDP_ECHO_2_ERROR.txt"
touch $filename_UDP_ECHO_2_ERROR
grep -A 6 -B 10 "UDP_ECHO_2 ERROR" temporary_file.txt > $filename_UDP_ECHO_2_ERROR
printf "UDP_ECHO_2 ERROR count:       , "; grep -c "UDP_ECHO_2 ERROR" $filename_UDP_ECHO_2_ERROR
filename_UDP_ECHO_3_ERROR="${filename}_UDP_ECHO_3_ERROR.txt"

echo "--------------------------"
echo "counting SOCKET_ERROR"
filename_SOCKET_ERROR="${filename}_SOCKET_ERROR.txt"
touch $filename_SOCKET_ERROR
grep -A 6 -B 10 "Socket reading error" temporary_file.txt > $filename_SOCKET_ERROR
printf "SOCKET ERROR count:           , "; grep -c "Socket reading error" $filename_SOCKET_ERROR
# echo "which includes"
# echo "Rest of SOCKET_ERROR count not further defined"
filename_SOCKET_ERROR_SIZES="${filename}_SOCKET_ERROR_SIZES.txt"
touch $filename_SOCKET_ERROR_SIZES
grep "size = " $filename_SOCKET_ERROR > $filename_SOCKET_ERROR_SIZES
printf "SOCKET ERROR SIZES count:     , "; grep -c "size = " $filename_SOCKET_ERROR_SIZES

filename_SOCKET_ERROR_SIZE_NUMBERS="${filename}_SOCKET_ERROR_SIZE_NUMBERS.txt"
touch $filename_SOCKET_ERROR_SIZE_NUMBERS
# echo "a problem with awk ...."
awk -F'[=\n]' '{print $2}' $filename_SOCKET_ERROR_SIZES > $filename_SOCKET_ERROR_SIZE_NUMBERS
# printf "SOCKET_ERROR_SIZES_NUMBERS count:    , "; grep -c "size = " $filename_SOCKET_ERROR_SIZE_NUMBERS

rm temporary_file.txt
