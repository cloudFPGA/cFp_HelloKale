#!/bin/bash
# /*******************************************************************************
#  * Copyright 2016 -- 2021 IBM Corporation
#  *
#  * Licensed under the Apache License, Version 2.0 (the "License");
#  * you may not use this file except in compliance with the License.
#  * You may obtain a copy of the License at
#  *
#  *     http://www.apache.org/licenses/LICENSE-2.0
#  *
#  * Unless required by applicable law or agreed to in writing, software
#  * distributed under the License is distributed on an "AS IS" BASIS,
#  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  * See the License for the specific language governing permissions and
#  * limitations under the License.
# *******************************************************************************/

#  *
#  *                       cloudFPGA
#  *    =============================================
#  *     Created: Nov 2019
#  *     Authors: FAB, WEI, NGL, POL
#  *
#  *     Description:
#  *        Main cFp_HelloKale regression script.
#  *
#  *     Synopsis:
#  *        run_main <cFpHelloKaleRootDir>
#  *
#  *     Details:
#  *       - This script is typically called by the Jenkins server.
#  *       - It expects to be executed from the cFp_HelloKale root directory.
#  *       - The '$cFpHElloKaleRootDir' variable must be set externally or passed as a parameter. 
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


# STEP-1a: Check if '$cFpHelloKaleRootDir' is passed as a parameter
if [[  $# -eq 1 ]]; then
    echo "<$0> The regression root directory is passed as an argument:"
    echo "<$0>   Regression root directory = '$1' "
    export cFpHElloKaleRootDir=$1
fi

# STEP-1b: Confirm that '$cFpHelloKaleRootDir' is set
if [[ -z $cFpHelloKaleRootDir ]]; then
    echo "<$0> STOP ON ERROR: "
    echo "<$0>   You must provide the path of the regression root directory as an argument, "
    echo "<$0>   or you must set the environment variable '\$cFpHelloKaleRootDir'."
    exit_on_error 1
fi

# STEP-2a: Set the environment variables
export rootDir=$cFpHelloKaleRootDir 
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
echo "<$0> ###   START OF 'cFp_HelloKale' REGRESSION: " 
echo "<$0> ###                                         " 
echo "<$0> ################################################################"

echo "<$0> ================================================================"
echo "<$0> ===   REGRESSION - ROLE - START OF CSIM "
echo "<$0> ================================================================"
export usedRoleDir=$cFpHelloKaleRootDir/ROLE
cd $usedRoleDir
bash $usedRoleDir/reg/run_csim_reg.sh
exit_on_error $? 
echo "<$0> ----------------------------------------------------------------"
echo "<$0> ---   REGRESSION - ROLE - END OF CSIM "
echo "<$0> ----------------------------------------------------------------"


echo "<$0> ================================================================"
echo "<$0> ===   REGRESSION - ROLE - START OF COSIM "
echo "<$0> ================================================================"
export usedRoleDir=$cFpHellloKaleRootDir/ROLE
cd $usedRoleDir
bash $usedRoleDir/reg/run_cosim_reg.sh
exit_on_error $? 
echo "<$0> ----------------------------------------------------------------"
echo "<$0> ---   REGRESSION - ROLE - END OF COSIM "
echo "<$0> ----------------------------------------------------------------"


echo "<$0> ################################################################"
echo "<$0> ###                                       " 
echo "<$0> ###   END OF 'cFp_HelloKale' REGRESSION: " 
echo "<$0> ###                                       " 
echo "<$0> ################################################################"

exit 0

