#!/bin/bash

sudo ~iwai/qemu/build/contrib/ivshmem-server/ivshmem-server -F &
sleep 1
sudo chown libvirt-qemu:kvm /dev/shm/ivshmem /tmp/ivshmem_socket