#!/bin/bash

processes=("ivshmem" "ovs-vswitchd" "qemu")

for proc in "${processes[@]}"; do
    for PID in $(pgrep "$proc"); do
        for TID in $(ps -L -p $PID -o tid=); do
            sudo chrt -f -p 99 $TID
            chrt -p $TID
        done
    done
done
