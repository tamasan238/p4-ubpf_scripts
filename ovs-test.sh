#!/bin/bash

BRIDGE_NAME="ovs-br1"
IFACE_NAME="dummy-if"

MY_IP_ADDRESS=192.168.122.33	   # ubpf vm
DUMMY_IP_ADDRESS=192.168.254.254
REMOTE_IP_ADDRESS=192.168.122.173  # p4 vm

#cd ~iwai/ovs
#make
#make install

export PATH=$PATH:/usr/local/share/openvswitch/scripts
ovs-ctl start

ovs-vsctl add-br $BRIDGE_NAME
ovs-vsctl set bridge $BRIDGE_NAME datapath_type=netdev

ip link add name $IFACE_NAME type dummy
ip addr add $DUMMY_IP_ADDRESS/24 dev $IFACE_NAME
ip link set $IFACE_NAME up
ovs-vsctl add-port $BRIDGE_NAME $IFACE_NAME

echo "ovs-vsctl show:"
ovs-vsctl show

ip link set $BRIDGE_NAME up
ip -br a

# tcpdump -i $IFACE_NAME &
tcpdump -i $BRIDGE_NAME &
TCPDUMP_PID=$!

ssh iwai@$REMOTE_IP_ADDRESS "sudo ip route add $DUMMY_IP_ADDRESS via $MY_IP_ADDRESS"
ssh iwai@$REMOTE_IP_ADDRESS "ping $DUMMY_IP_ADDRESS -c 3"
ssh iwai@$REMOTE_IP_ADDRESS "sudo ip route del $DUMMY_IP_ADDRESS via $MY_IP_ADDRESS"

#sleep 10

kill $TCPDUMP_PID

ovs-vsctl del-br $BRIDGE_NAME
ip link del $IFACE_NAME
ovs-ctl stop

echo "Done."

