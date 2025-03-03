#!/bin/bash

# Check if the server name variable is provided
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <server_name> <profile_name>"
    exit 1
fi

# Get the server name from the first argument
SERVER_NAME=$1
PROFILE_NAME=$2
OUTPUT_DIR="../data/profiles"

# Define the input and output file paths
INPUT_FILE="${OUTPUT_DIR}/out_${SERVER_NAME}_${PROFILE_NAME}"
FLAMEGRAPH_FILE="${OUTPUT_DIR}/flamegraph_${SERVER_NAME}_${PROFILE_NAME}.svg"

# Generate the flame graph
echo "Generating flame graph..."
nperf flamegraph "$INPUT_FILE" > "$FLAMEGRAPH_FILE"

# Check if the flame graph was created
if [ ! -f "$FLAMEGRAPH_FILE" ]; then
    echo "Error: Failed to create flame graph $FLAMEGRAPH_FILE."
    exit 1
fi

echo "Flame graph generated: $FLAMEGRAPH_FILE"