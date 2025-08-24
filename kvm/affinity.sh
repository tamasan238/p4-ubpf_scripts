#!/bin/bash

pin_to_cpu() {
    local cores=$1
    local pattern=$2

    for PID in $(pgrep -f "$pattern"); do
        taskset -cp $cores $PID
        for TID in $(ls /proc/$PID/task); do
            taskset -cp $cores $TID
        done
    done
}

# pin_to_cpu "0,1,2,3" "ovs-vswitchd"
# pin_to_cpu "4,5" "guest=p4"
# # pin_to_cpu "4,5" "ivshmem-server" 効かない．
# pin_to_cpu "6,7" "guest=user_dpdk"


# 仮
pin_to_cpu "0,1,2,3" "ovs-vswitchd"
pin_to_cpu "4,5,6,7" "guest=p4"
# pin_to_cpu "4,5" "ivshmem-server" 効かない．
pin_to_cpu "8,9,10,11" "guest=user_dpdk"