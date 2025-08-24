#!/bin/bash

for PID in $(pgrep ivshmem); do
    sudo chrt -f -p 99 $PID
    chrt -p $PID
done

for PID in $(pgrep ovs-vswitchd); do
    sudo chrt -f -p 99 $PID
    chrt -p $PID
done

for PID in $(pgrep qemu); do
    sudo chrt -f -p 99 $PID
    chrt -p $PID
done