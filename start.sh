#!/bin/bash

# Function to check if a program started successfully
function check_process {
    if [ $? -ne 0 ]; then
        echo "Failed to start $1."
        exit 1
    fi
}

# ���� ./keysupplyserver Ŀ¼�µ� ./server������¼PID
cd ./keysupplyserver || exit 1
./server <<EOF > ../log1 2>&1 &
7
EOF
echo $! > ../server.pid
cd - || exit 1

check_process "server"

# �ȴ� server �������
sleep 2

# ���� ./keymanagement Ŀ¼�µ� ./km������¼PID
cd ./keymanagement || exit 1
./km 192.168.8.184 50003 > ../log2 2>&1 &
echo $! > ../km.pid
cd - || exit 1

check_process "km"

# �ȴ� km �������
sleep 2

# ���� ./quantum_key_interface Ŀ¼�µ� ./qki������¼PID
cd ./quantum_key_interface || exit 1
./qki 192.168.8.184 192.168.8.126 > ../log3 2>&1 &
echo $! > ../qki.pid
cd - || exit 1

check_process "qki"

# �ȴ� qki �������
sleep 2

echo "ALL programs are now running. Redirect to log1, log2, and log3. PIDs are logged."