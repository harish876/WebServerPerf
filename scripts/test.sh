#!/bin/bash
# filepath: /root/WebServerPerf/scripts/test.sh

num_requests=1000
concurrency=100
url="http://147.182.205.94:4221/json"
payload_file="data.json"
content_length=$(wc -c < "$payload_file")

ab -n $num_requests -c $concurrency -t 300 -p $payload_file -s 30000 -T 'application/json' -H "Content-Length: $content_length" $url