# *
# * Copyright 2016 -- 2020 IBM Corporation
# * 
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *     http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *

# *****************************************************************************
# * @file       : genTestFile.py
# * @brief      : A script to generate test files for use with 'socat' command.
# *
# * System:     : cloudFPGA
# * Component   : cFp_Monolithic/ROLE
# * Language    : Python 3
# *
# *****************************************************************************


# ### REQUIRED PYTHON PACKAGES ################################################
import argparse
import datetime
import os.path
import random
import re
import time

# ### REQUIRED TESTCASE MODULES ###############################################

###############################################################################
#                                                                             #
#                                 MAIN                                        #
#                                                                             #
###############################################################################

#  STEP-1: Parse the command line strings into Python objects
# -----------------------------------------------------------------------------
parser = argparse.ArgumentParser(description='A script to generate the content of test files for use with the socat command.')
# Positional arguments
parser.add_argument('-f',  '--file',        required=False, type=str, default='',
                           help='The <filename> to generate.')
parser.add_argument('-sz', '--size',        required=True, type=str, default='',
                           help='The size of the file to generate (e.g. 512, 32K, 8M, 1G).')
# Optional arguments
parser.add_argument('-inc',  '--increment',  action="store_true",
                           help='Generate a ramp of incremented numbers')
parser.add_argument('-dec',  '--decrement',  action="store_true",
                           help='Generate a ramp of decremented numbers')
parser.add_argument('-rnd',  '--random',     action="store_true",
                           help='Generate random numbers')
parser.add_argument('-v',    '--verbose',    action="store_true",
                           help='Enable verbosity')
args = parser.parse_args()

# Parse the size argument
suffixFileName = ""
match = re.search("^[0-9]", args.size)
if match:
    charList = re.findall("[0-9]", args.size)
    strInt = ""
    for e in charList:
        strInt += e
    suffixFileName += strInt
    reqSize = int(strInt)
    if len(args.size) > len(charList):
        if args.size[len(charList)] == 'K':
            reqSize = reqSize*1024
            suffixFileName += 'K'
        elif args.size[len(charList)] == 'M':
            reqSize = reqSize*1024*1024
            suffixFileName += 'M'
        elif args.size[len(charList)] == 'G':
            reqSize = reqSize*1024*1024*1024
            suffixFileName += 'G'
        else:
            print("Unknown or un-supported prefix symbol '%c'.\n" % args.size[len(charList)])
            exit(1)
else:
    print("ERROR: The file size must start with a digit!\n")
    exit(1)

if reqSize % 8:
    print("\nERROR: The size must be a multiple of 8 bytes.")
    exit(1)

if args.increment == 0 and args.decrement == 0 and args.random == 0:
    print("\nERROR: A generator type is required (rnd, inc or dec).\n")
    exit(1)

#  STEP-2: Generate filename and check if file already exists
# -----------------------------------------------------------------------------
fileName = ""
if args.file == "":
    if args.increment:
        fileName = "inc_" + suffixFileName
    if args.decrement:
        fileName = "dec_" + suffixFileName
    if args.random:
        fileName = "rnd_" + suffixFileName
else:
    fileName = args.file

if os.path.exists(fileName):
    print("\nWARNING: File already exist.")
    answer = input("\tOverwrite (y/n): ")
    if answer != 'y':
        exit(0)
    else:
        os.remove(fileName)

#  STEP-3: Open file and generate content
# -----------------------------------------------------------------------------
outFile = open(fileName, 'w')

if args.increment:
    strStream = ""
    size = int(reqSize / 8)
    for x in range(0, size):
        swapStr = ""
        strTmp = "{:08d}".format(x)
        # Swap the generated 8 bytes
        swapStr += strTmp[7]
        swapStr += strTmp[6]
        swapStr += strTmp[5]
        swapStr += strTmp[4]
        swapStr += strTmp[3]
        swapStr += strTmp[2]
        swapStr += strTmp[1]
        swapStr += strTmp[0]
        # OBSOLETE_20210801 outFile.write("{:08d}".format(x))
        outFile.write(swapStr)
        if size < 1024:
            strStream += strTmp
    strStream += '\n'
    if size < 1024:
        print("STREAM=%s" % strStream)
elif args.decrement:
    size = int(reqSize / 8)
    strStream = ""
    for x in range(size-1, 0, -1):
        swapStr = ""
        strTmp = "{:08d}".format(x)
        # Swap the generated 8 bytes
        swapStr += strTmp[7]
        swapStr += strTmp[6]
        swapStr += strTmp[5]
        swapStr += strTmp[4]
        swapStr += strTmp[3]
        swapStr += strTmp[2]
        swapStr += strTmp[1]
        swapStr += strTmp[0]
        # OBSOLETE_20210801 outFile.write("{:08d}".format(x))
        outFile.write(swapStr)
        if size < 1024:
            strStream += strTmp
    strStream += '\n'
    if size < 1024:
        print("STREAM=%s" % strStream)
else:
    size = int(reqSize / 2)
    strStream = ""
    for x in range(0, size):
        y = random.randint(0, 255)
        outFile.write("{:02x}".format(y))

outFile.close()

print("\nINFO: The following file was created:")
os.system("ls -lh " + fileName)
