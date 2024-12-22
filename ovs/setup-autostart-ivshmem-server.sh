#!/bin/bash

sudo cp ~iwai/p4-ubpf_scripts/ovs/ivshmem-start.sh ~root/
sudo cp ~iwai/p4-ubpf_scripts/ovs/ivshmem-stop.sh ~root/
sudo chmod +x ~root/ivshmem-start.sh
sudo chmod +x ~root/ivshmem-stop.sh
sudo cp ~iwai/p4-ubpf_scripts/ovs/ivshmem.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable ivshmem.service
sudo systemctl start ivshmem.service