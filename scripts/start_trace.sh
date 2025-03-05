#!/bin/bash

# Check if the server name variable is provided
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <server_name> <profile_name> [duration]"
    exit 1
fi

# Get the server name and duration from the arguments
SERVER_NAME=$1
PROFILE_NAME=$2
DURATION=${3:-300} # default trace for 5 mins 
DURATION=$(($DURATION + 0))
OUTPUT_DIR="../data/profiles"

# Get the PID of the server. 3 because my script spawns taskset which spawns the actual process
PID=$(pgrep -f http_server_$SERVER_NAME | sed -n '2p')

# Check if the PID is available
if [ -z "$PID" ]; then
    echo "Error: Could not find the PID for http_server_$SERVER_NAME."
    exit 1
fi

# Using nperf for now from koute/not-perf
# Start the nperf profiler and pin it to CPU core 0
echo "Starting nperf profiler for $SERVER_NAME on CPU core 0 for $DURATION seconds..."
# sudo profile-bpfcc -f -F 99 --pid $PID $DURATION > $OUTPUT_DIR/out_${SERVER_NAME}_${PROFILE_NAME}.profile-folded
nperf record -p $PID --do-not-send-sigstop -l $DURATION -o $OUTPUT_DIR/out_${SERVER_NAME}_${PROFILE_NAME}

echo "Profiling complete. Results saved to out_${SERVER_NAME}_${PROFILE_NAME}"