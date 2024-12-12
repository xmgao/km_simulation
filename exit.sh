#!/bin/bash

# Function to kill process and its children using PID from a file
function kill_process {
    if [ -f "$1" ]; then
        PID=$(cat "$1")
        if ps -p $PID > /dev/null 2>&1; then
            pkill -TERM -P $PID  # Terminate all child processes
            kill $PID            # Terminate the main process
            echo "Process with PID $PID from $1 has been terminated along with any child processes."
        else
            echo "No running process found with PID $PID from $1. It might have already exited."
        fi
        # Optionally remove the PID file
        rm "$1"
    else
        echo "PID file $1 does not exist."
    fi
}

# Terminate each process using its associated PID file
kill_process "server.pid"
kill_process "km.pid"
kill_process "qki.pid"