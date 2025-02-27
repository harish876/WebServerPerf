#!/usr/bin/env bash

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <language> <mode> [num_runs] [total_requests]"
  exit 1
fi

LANGUAGE=$1
MODE=$2
URL="http://64.23.188.215:4221/echo/hellp"
concurrency_values=(250 500 750)
OUTPUT_DIR="../data/tests/echo"
NUM_RUNS=${3:-3} #Default to 3 if not pased 
TOTAL_REQUESTS=${4:-10000}  # Default to 10,000 if not passed

# Convert TOTAL_REQUESTS to an integer
TOTAL_REQUESTS=$(($TOTAL_REQUESTS + 0))
NUM_RUNS=$(($NUM_RUNS + 0))

# Create the output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Clear the contents of the output files before starting the runs
for concurrency in "${concurrency_values[@]}"; do
  OUTPUT_FILE="${OUTPUT_DIR}/${LANGUAGE}_${MODE}_${concurrency}_${NUM_RUNS}.txt"
  > "$OUTPUT_FILE"
done

for run in $(seq 1 $NUM_RUNS); do
  echo "Starting run $run..."
  for concurrency in "${concurrency_values[@]}"; do
    if [ "$concurrency" -gt "$TOTAL_REQUESTS" ]; then
      echo "Skipping concurrency level $concurrency as it exceeds the max concurrency value $MAX_CONCURRENCY."
      continue
    fi
    OUTPUT_FILE="${OUTPUT_DIR}/${LANGUAGE}_${MODE}_${concurrency}_${NUM_RUNS}.txt"
    echo "Run $run:" >> "$OUTPUT_FILE"
    ab -n $TOTAL_REQUESTS -c "$concurrency" -s 60 "$URL" >> "$OUTPUT_FILE"
    sleep 10
  done
  echo "Completed run $run. Waiting for 20 seconds before the next run..."
  sleep 20
done