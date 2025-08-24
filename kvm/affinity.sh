#!/bin/bash

pin_to_cpuset() {
    local cgname=$1
    local cores=$2
    local pattern=$3

    CGROUP_PATH="/sys/fs/cgroup/cpuset/$cgname"
    sudo mkdir -p $CGROUP_PATH
    echo $cores | sudo tee $CGROUP_PATH/cpuset.cpus
    echo 0 | sudo tee $CGROUP_PATH/cpuset.mems
    sudo bash -c 'echo 1 > '$CGROUP_PATH'/cpuset.cpu_exclusive' || true

    for PID in $(pgrep -f "$pattern"); do
        echo $PID | sudo tee $CGROUP_PATH/tasks
    done
}

pin_to_cpuset "ovs_vswitchd" "0-3" "ovs-vswitchd"
pin_to_cpuset "guest_p4" "4-5" "guest=p4"
pin_to_cpuset "ivshmem_server" "6" "ivshmem-server"
pin_to_cpuset "guest_user_dpdk" "6-7" "guest=user_dpdk"
