#!/bin/bash

# Function to check if a program started successfully
function check_process {
    if [ $? -ne 0 ]; then
        echo "Failed to start $1."
        exit 1
    fi
}

# 启动 ./keysupplyserver 目录下的 ./server，并记录PID
cd ./keysupplyserver || exit 1
./server <<EOF > ../log1 2>&1 &
7
EOF
echo $! > ../server.pid
cd - || exit 1

check_process "server"

# 等待 server 启动完成
sleep 2

# 启动 ./keymanagement 目录下的 ./km，并记录PID
cd ./keymanagement || exit 1
./km 192.168.8.184 50003 > ../log2 2>&1 &
echo $! > ../km.pid
cd - || exit 1

check_process "km"

# 等待 km 启动完成
sleep 2

# 启动 ./quantum_key_interface 目录下的 ./qki，并记录PID
cd ./quantum_key_interface || exit 1
./qki 192.168.8.184 192.168.8.126 > ../log3 2>&1 &
echo $! > ../qki.pid
cd - || exit 1

check_process "qki"

# 等待 qki 启动完成
sleep 2

echo "ALL programs are now running. Redirect to log1, log2, and log3. PIDs are logged."