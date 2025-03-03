#!/usr/bin/env bash

URL="http://64.23.188.215:4221/json"
NUM_REQUESTS=1000
CONCURRENCY=10
DURATION="30s"  # Duration of the test

wrk -t$CONCURRENCY -c$CONCURRENCY -d$DURATION -s post.lua $URL