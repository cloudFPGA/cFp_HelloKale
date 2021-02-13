#!/bin/bash
# Note important difference between /bin/sh and /bin/bash
# https://askubuntu.com/q/141928/295286


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
#    echo "new file is going to be named ${file/.*}-END_OF_TESTS.txt"
#    echo "new file is going to be named ${file/.*}-programming_ERROR.txt"
else
    echo "file does not exist"
    echo "usage: tc_Iperf_evaluation.sh filename.log"
    exit
fi

filename=${file/.*}
ext="${file#*.}"

if [[ 0 == 1 ]]  # set 0 == 1 to disable, 1 == 1 to enable this check
then
	if [ ${file: -4} == ".log" ]
	then
	   echo "file = ${file}"
	   echo "filename = ${filename}"
	   echo "ext = ${ext}"
	else
		echo "usage: tc_Iperf_evaluation.sh filename.log"
		echo "the file has to be a tc_*IPERF*.log file"
	fi
fi

grep -v "STOP_ERROR" $file > temporary_file.txt

filename_ROUND_NR="${filename}-ROUND_NR.txt"
# echo "filename_ROUND_NR = ${filename_ROUND_NR}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_ROUND_NR
grep "round nr." temporary_file.txt > $filename_ROUND_NR
printf "ROUND NR. count:           "; grep -c "round nr." $filename_ROUND_NR
# STOP_HERE   # uncomment to stop execution here

filename_END_OF_TESTS="${filename}-END_OF_TESTS.txt"
# echo "filename_END_OF_TESTS = ${filename_END_OF_TESTS}"
# STOP_HERE   # uncomment to stop execution here
touch $filename_END_OF_TESTS
grep -A 1 "END OF TESTS" temporary_file.txt > temp_file.txt
grep "RESOURCE_ID =" temp_file.txt > $filename_END_OF_TESTS
rm temp_file.txt
printf "END OF TESTS count:        "; grep -c "RESOURCE_ID" $filename_END_OF_TESTS
# STOP_HERE   # uncomment to stop execution here

filename_AVAILABLE="${filename}_AVAILABLE.txt"
touch $filename_AVAILABLE
grep "is AVAILABLE" temporary_file.txt > $filename_AVAILABLE
printf "is AVAILABLE count:        "; grep -c "is AVAILABLE" $filename_AVAILABLE
# STOP_HERE   # uncomment to stop execution here

filename_NO_RESPONSE="${filename}_NO_RESPONSE.txt"
touch $filename_NO_RESPONSE
grep "NO RESPONSE" temporary_file.txt > $filename_NO_RESPONSE
printf "NO RESPONSE count:         "; grep -c "NO RESPONSE" $filename_NO_RESPONSE
# STOP_HERE   # uncomment to stop execution here

echo "--------------------------"
echo "counting programming ERROR"
filename_ERROR="${filename}_ERROR.txt"
touch $filename_ERROR
grep "ERROR " temporary_file.txt > $filename_ERROR
printf "ERROR count:               "; grep -c "ERROR" $filename_ERROR
# STOP_HERE   # uncomment to stop execution here
filename_JTAG_ERROR="${filename}_JTAG_ERROR.txt"
touch $filename_JTAG_ERROR
grep "JTAG" $filename_ERROR > $filename_JTAG_ERROR
printf "JTAG ERROR count:          "; grep -c "JTAG" $filename_JTAG_ERROR
# STOP_HERE   # uncomment to stop execution here

filename_programming_ERROR="${filename}-programming_ERROR.txt"
grep "programming ERROR" temporary_file.txt > $filename_programming_ERROR
printf "programming ERROR count:   "; grep -c "programming ERROR" $filename_programming_ERROR
# STOP_HERE   # uncomment to stop execution here
echo "which includes"
filename_WARNING="${filename}_WARNING.txt"
touch $filename_WARNING
grep "WARNING" temporary_file.txt > $filename_WARNING
printf "WARNING count:             "; grep -c "WARNING" $filename_WARNING
# STOP_HERE   # uncomment to stop execution here
echo "which includes"
filename_PHY_REGS="${filename}_PHY_REGS.txt"
touch $filename_PHY_REGS
grep "PHY_REGS" $filename_WARNING > $filename_PHY_REGS
printf "PHY_REGS WARNING count:    "; grep -c "PHY_REGS" $filename_PHY_REGS
printf "containing mc0: 0, counts: "; grep -c "mc0: 0," $filename_PHY_REGS
printf "containing mc1: 0, counts: "; grep -c "mc1: 0," $filename_PHY_REGS
# STOP_HERE   # uncomment to stop execution here
echo "Rest of programming ERROR count not further defined"
# if non explainable high programming error count, check filespace on sled manager!
