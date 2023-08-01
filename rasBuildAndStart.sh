#!/bin/bash

# Change to the directory containing the read_and_send program
cd /home/dan/project

# Build the read_and_send program
gcc -o /home/dan/project/read_and_send /home/dan/project/read_and_send.c -lcurl -ljansson -lmysqlclient

chmod a+x /home/dan/project/read_and_send

# Start the read_and_send program using nohup
nohup /home/dan/project/read_and_send > /dev/null 2>&1 &

# Clean up object files and executables
rm read_and_send
