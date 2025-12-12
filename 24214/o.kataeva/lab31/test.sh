#!/bin/bash

SOCKFILE="socket"
if [ ! -e "$SOCKFILE" ]; then
    echo "Server socket not found"
    exit 1
fi


MSG1="hello"
MSG2="world"
MSG3="message"

echo "$MSG1" | ./client &
PID1=$!
echo "$MSG2" | ./client &
PID2=$!
echo "$MSG3" | ./client &
PID3=$!

wait $PID1 $PID2 $PID3

echo "All clients finished."
