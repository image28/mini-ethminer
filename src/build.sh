#!/bin/bash

# programs and flags
CUR=`pwd`  
CXX="/usr/bin/gcc"
CFLAGS="-Wno-error=parentheses -march=native -g"
LIBS="-L/usr/lib -lOpenCL -lpthread -lcrypto" # 
INC="-I. -I/usr/include"

# OPENCL KERNEL
$CUR/cl2h.sh "ethash.cl" "ethash.h"
# Miner
$CXX $INC $CXXFLAGS main.c -o ../bin/phiMiner $LIBS &
# strip --strip-debug --strip-unneeded ./bin/phiMiner
