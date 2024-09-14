#!/bin/bash

# root権限で実行されているか確認
if [ "$(id -u)" -ne 0 ]; then
  echo "このスクリプトはroot権限で実行する必要があります。"
  exit 1
fi

echo "KVM/QEMUをインストールします"

apt update
apt install -y qemu-kvm libvirt-daemon-system virtinst libosinfo-bin virt-manager

mkdir -p /home/iwai/Downloads
wget -P /home/iwai/Downloads  https://www.ksl.ci.kyutech.ac.jp/~iwai/ubuntu-22.04.2-live-server-amd64.iso
