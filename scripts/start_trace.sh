#!/bin/bash

# Check if the server name variable is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <server_name> [duration]"
    exit 1
fi

# Get the server name and duration from the arguments
SERVER_NAME=$1
DURATION=${2:-300} # default trace for 5 mins 
DURATION=$(($DURATION + 0))
OUTPUT_DIR="../data/profiles"

# Get the PID of the server
PID=$(pgrep -f http_server_$SERVER_NAME | sed -n '3p')

# Check if the PID is available
if [ -z "$PID" ]; then
    echo "Error: Could not find the PID for http_server_$SERVER_NAME."
    exit 1
fi

# Start the eBPF profiler and pin it to CPU core 0
echo "Starting eBPF profiler for $SERVER_NAME on CPU core 0 for $DURATION seconds..."
sudo profile-bpfcc -f -F 99 --pid $(pgrep -f http_server_$SERVER_NAME | sed -n '3p') $DURATION > $OUTPUT_DIR/out_${SERVER_NAME}.profile-folded

echo "Profiling complete. Results saved to out_${SERVER_NAME}.profile-folded."