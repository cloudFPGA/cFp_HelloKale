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

# ATTENTION: This script EXPECTS THAT IT IS EXECUTED FROM THE REPOISOTORY ROOT AND ALL NECESSARY ENVIROMENT IS SOURCED! 
# $root should bet set externally

# STEP 0: we need to set the right environment

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

# STEP-1: make a monolithic build
echo "======== START of STEP-1 ========"
cd $root
make monolithic
echo "======== END of STEP-1 ========"


