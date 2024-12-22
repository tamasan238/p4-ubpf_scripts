#!/bin/bash

sudo kill $(pgrep ivshmem-server)
sudo rm /dev/shm/ivshmem
sudo rm /tmp/ivshmem_socket