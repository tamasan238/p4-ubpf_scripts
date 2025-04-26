#!/bin/bash

if pgrep -f "ivshmem-server" > /dev/null; then
    sudo kill $(pgrep ivshmem-server) 2> /dev/null
    echo "ivshmem-server stopped."
fi

for FILE in /dev/shm/ivshmem /tmp/ivshmem_socket; do
    sudo rm "$FILE" 2> /dev/null || true
done
