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

# STEP-1: make a monolithic build
cd $root
make monolithic


