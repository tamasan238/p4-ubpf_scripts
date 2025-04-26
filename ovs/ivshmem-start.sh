#!/bin/bash

if pgrep -f "ivshmem-server" > /dev/null; then
    echo "ivshmem-server is already running."
    exit 0
fi

for FILE in /dev/shm/ivshmem /tmp/ivshmem_socket; do
    sudo rm "$FILE" 2> /dev/null || true
done

sudo ~iwai/qemu/build/contrib/ivshmem-server/ivshmem-server -l 1024M &

sleep 1

for FILE in /dev/shm/ivshmem /tmp/ivshmem_socket; do
    sudo chown libvirt-qemu:kvm "$FILE" || true
done
