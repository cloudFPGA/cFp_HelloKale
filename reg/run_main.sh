#!/bin/bash
#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Nov 2019
#  *     Authors: FAB, WEI, NGL, POL
#  *
#  *     Description:
#  *        Main cFp_Monolithic regression script.
#  *
#  *     Synopsis:
#  *        run_main <cFpMonolithicRootDir>
#  *
#  *     Details:
#  *       - This script is typically called by the Jenkins server.
#  *       - It expects to be executed from the cFp_Monolithic root directory.
#  *       - The '$cFpMonolithicRootDir' variable must be set externally or passed as a parameter. 
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


# STEP-1a: Check if '$cFpMonolithicRootDir' is passed as a parameter
if [[  $# -eq 1 ]]; then
    echo "<$0> The regression root directory is passed as an argument:"
    echo "<$0>   Regression root directory = '$1' "
    export cFpMonolithicRootDir=$1
fi

# STEP-1b: Confirm that '$cFpMonolithicRootDir' is set
if [[ -z $cFpMonolithicRootDir ]]; then
    echo "<$0> STOP ON ERROR: "
    echo "<$0>   You must provide the path of the regression root directory as an argument, "
    echo "<$0>   or you must set the environment variable '\$cFpMonolithicRootDir'."
    exit_on_error 1
fi

# STEP-2a: Set the environment variables
export rootDir=$cFpMonolithicRootDir 
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

# STEP-2b: Add your floating licence servers for the Xilinx tools here
# (.e.g export XILINXD_LICENSE_FILE=<port>@<server1>.<domain>.ibm.com:<port>@<server2>.<domain>.ibm.com)
# echo "<$0> Done with the setting of the environment."

retval=1

echo "<$0> ################################################################"
echo "<$0> ###                                         " 
echo "<$0> ###   START OF 'cFp_Monolithic' REGRESSION: " 
echo "<$0> ###                                         " 
echo "<$0> ################################################################"

echo "<$0> ================================================================"
echo "<$0> ===   REGRESSION - ROLE - START OF CSIM "
echo "<$0> ================================================================"
export usedRoleDir=$cFpMonolithicRootDir/ROLE
cd $usedRoleDir
sh $usedRoleDir/reg/run_csim_reg.sh
exit_on_error $? 
echo "<$0> ----------------------------------------------------------------"
echo "<$0> ---   REGRESSION - ROLE - END OF CSIM "
echo "<$0> ----------------------------------------------------------------"


echo "<$0> ================================================================"
echo "<$0> ===   REGRESSION - ROLE - START OF COSIM "
echo "<$0> ================================================================"
export usedRoleDir=$cFpMonolithicRootDir/ROLE
cd $usedRoleDir
sh $usedRoleDir/reg/run_cosim_reg.sh
exit_on_error $? 
echo "<$0> ----------------------------------------------------------------"
echo "<$0> ---   REGRESSION - ROLE - END OF COSIM "
echo "<$0> ----------------------------------------------------------------"


echo "<$0> ################################################################"
echo "<$0> ###                                       " 
echo "<$0> ###   END OF 'cFp_Monolithic' REGRESSION: " 
echo "<$0> ###                                       " 
echo "<$0> ################################################################"

exit 0

