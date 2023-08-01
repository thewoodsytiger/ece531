#!/bin/bash

# Change to the directory containing the thermostat program
cd /home/dan/project

# Build the thermostat program using the makefile
make -f /home/dan/project/makefile-tc-x86_64

chmod a+x /home/dan/project/tcsimd

# Start the thermostat program using nohup
nohup /home/dan/project/tcsimd > /dev/null 2>&1 &

# Clean up object files and executables
make -f makefile-tc-x86_64 clean
