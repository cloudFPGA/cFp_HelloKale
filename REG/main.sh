#!/bin/bash
#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Nov 2019
#  *     Authors: FAB, WEI, NGL, POL
#  *
#  *     Description:
#  *        Main regressions script. This script is called by e.g. the Jenkins server.
#  *

# ATTENTION: 
#   This script EXPECTS THAT IT IS EXECUTED FROM THE REPOSITORY ROOT AND ALL NECESSARY
#    ENVIROMENT IS SOURCED!
#   And $root should bet set externally.

# @brief A function to check if previous step passed.
# @param[in] $1 the return value of the previous command.
function exit_on_error {
    if [ $1 -ne 0 ]; then
        echo "EXIT ON ERROR: Regression \'$0\' FAILED."
        echo "  Last return value was $1."
        exit $1
    fi
}

# STEP 0: We need to set the right environment
export rootDir=$root
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

echo "set cFp environment"
retval=1

echo "======== START of STEP-1: Build \'monolithic\' ========"
cd $root
make monolithic
exit_on_error $? 
echo "======== END of STEP-1: Build \'monolithic\' ========"


exit 0

