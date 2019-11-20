#!/bin/bash
#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Nov 2019
#  *     Authors: FAB, WEI, NGL, POL
#  *
#  *     Description:
#  *        Main cFp_Flash regressions script.
#  *
#  *     Details:
#  *       - This script is typically called the Jenkins server.
#  *       - It expects to be executed from the cFDK root directory.
#  *       - The '$cFpFlashRootDir' variable must be set externally. 
#  *       - All environment variables must be sourced beforehand.
#  *

# @brief A function to check if previous step passed.
# @param[in] $1 the return value of the previous command.
function exit_on_error {
    if [ $1 -ne 0 ]; then
        echo "EXIT ON ERROR: Regression \'$0\' FAILED."
        echo "  Last return value was $1."
        exit $1
    fi
}

# STEP-0: We need to set the right environment
export rootDir=$cFpFlashRootDir 
export cFpRootDir="$rootDir/"
export cFpIpDir="$rootDir/ip/"
export cFpMOD="FMKU60"
export usedRoleDir="$rootDir/ROLE/"
export usedRole2Dir="$rootDir/ROLE/"
export cFpSRAtype="Themisto"
export cFpXprDir="$rootDir/xpr/"
export cFpDcpDir="$rootDir/dcps/"
export roleName1="default"
export roleName2="unused"

echo "Set cFp environment."
retval=1


echo "================================================================"
echo "===   START OF REGRESSION: $0"
echo "================================================================"

echo "================================================================"
echo "===   REGRESSION $0 - START OF BUILD: \'monolithic\' "
echo "================================================================"
cd $cFpFlashRootDir 
make monolithic
exit_on_error $? 
echo "================================================================"
echo "===   REGRESSION $0 - END OF BUILD  : \'monolithic\' "
echo "================================================================"

echo "================================================================"
echo "===   REGRESSION $0 - START OF COSIM "
echo "================================================================"
export cFdkRootDir=$cFpFlashRootDir/cFDK
cd $cFdkRootDir 
sh $cFdkRootDir/run_cosim_reg.sh
exit_on_error $? 
echo "================================================================"
echo "===   REGRESSION $0 - END OF COSIM "
echo "================================================================"

exit 0

