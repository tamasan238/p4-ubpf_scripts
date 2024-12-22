#!/bin/bash

sudo kill $(pgrep ivshmem-server)
sudo rm /dev/shm/ivshmem 2> /dev/null
sudo rm /tmp/ivshmem_socket 2> /dev/null