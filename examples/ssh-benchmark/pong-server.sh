#!/bin/sh
# Ping/pong server — reads lines from stdin, responds "pong" to each "ping".
# Run indefinitely until stdin is closed (EOF).
while IFS= read -r line; do
    if [ "$line" = "ping" ]; then
        echo "pong"
    fi
done
