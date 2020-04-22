#!/bin/bash
#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Apr 2020
#  *     Authors: MPU 
#  *
#  *     Description:
#  *        Main cFp_BringUp RTL simulation script.
#  *
#  *     Synopsis:
#  *        run_sim <cFpBringUpRootDir>
#  *
#  *     Details:
#  *       - This script is typically called by the Jenkins server.
#  *       - It expects to be executed from the cFp_BringUp root directory.
#  *       - The '$cFpBringUpRootDir' variable must be set externally or passed as a parameter. 
#  *       - All environment variables must be sourced beforehand.
#  *


#-----------------------------------------------------------
# @brief A function to check if previous step passed.
# @param[in] $1 the return value of the previous command.
#-----------------------------------------------------------
function exit_on_error {
    if [ $1 -ne 0 ]; then
        echo "<$0> EXIT ON ERROR: "
        echo "<$0>    Regression '$0' FAILED. "
        exit $1
    fi
}


# STEP-1a: Check if '$cFpBringUpRootDir' is passed as a parameter
if [[  $# -eq 1 ]]; then
    echo "<$0> The regression root directory is passed as an argument:"
    echo "<$0>   Regression root directory = '$1' "
    export cFpBringUpRootDir=$1
fi

# STEP-1b: Confirm that '$cFpBringUpRootDir' is set
if [[ -z $cFpBringUpRootDir ]]; then
    echo "<$0> STOP ON ERROR: "
    echo "<$0>   You must provide the path of the regression root directory as an argument, "
    echo "<$0>   or you must set the environment variable '\$cFpBringUpRootDir'."
    exit_on_error 1
fi

# STEP-2a: Set the environment variables
export rootDir=$cFpBringUpRootDir 
export cFpRootDir="$rootDir/"
export cFpIpDir="$rootDir/ip/"
export cFpMOD="FMKU60"
export usedRoleDir="$rootDir/ROLE/"
export usedRole2Dir="$rootDir/ROLE/"
export cFpSRAtype="Kale"
export cFpXprDir="$rootDir/xpr/"
export cFpDcpDir="$rootDir/dcps/"
export roleName1="default"
export roleName2="unused"
export simDir="$cFpBringUpRootDir/Sim"
# STEP-2b: Add floating licence servers for the Xilinx tools
export XILINXD_LICENSE_FILE=27001@boswil.zurich.ibm.com:27001@uzwil.zurich.ibm.com:27001@inwil.zurich.ibm.com:2100@pokwinlic1.pok.ibm.com:2100@pokwinlic2.pok.ibm.com:2100@pokwinlic3.pok.ibm.com:2100@csslab3.watson.ibm.com

echo "<$0> Done with the setting of the environment."
retval=1

echo "<$0> ################################################################"
echo "<$0> ###                        " 
echo "<$0> ###   START OF REGRESSION: " 
echo "<$0> ###                        " 
echo "<$0> ################################################################"


echo "<$0> ================================================================"
echo "<$0> ===   REGRESSION - START OF BUILD: 'monolithic' "
echo "<$0> ================================================================"
cd $cFpBringUpRootDir
# [DEBUG] make testError
make full_clean
make monolithic
exit_on_error $? 
echo "<$0> ----------------------------------------------------------------"
echo "<$0> ---   REGRESSION - END OF BUILD  : 'monolithic' "
echo "<$0> ----------------------------------------------------------------"


echo "<$0> ================================================================"
echo "<$0> ===   REGRESSION - ROLE - START OF RTLSIM "
echo "<$0> ================================================================"
sh $simDir/run_rtlsim.sh
exit_on_error $? 
echo "<$0> ================================================================"
echo "<$0> ===   REGRESSION - ROLE - START OF RTLSIM "
echo "<$0> ================================================================"
s
