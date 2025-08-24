#!/bin/bash

PRIO=$(sched_get_priority_max SCHED_FIFO)

for PID in $(pgrep -x ovs-vswitchd qemu-system-x86_64); do
    for TID in $(ps -L -p $PID -o tid=); do
        sudo chrt -f $PRIO -p $TID
    done
    echo "Real-time priority $PRIO applied to all threads of PID $PID"
done
