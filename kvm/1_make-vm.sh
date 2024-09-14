#!/bin/bash

# root権限で実行されているか確認
if [ "$(id -u)" -ne 0 ]; then
  echo "このスクリプトはroot権限で実行する必要があります。"
  exit 1
fi

echo "Ubuntu 22.04 Server版のVMを作成します．"

# ユーザーからの入力を受け取る
read -p "ホスト名を入力してください: " hostname
read -p "メモリ量 (MB単位) を入力してください: " memory
read -p "vCPUの数を入力してください: " vcpus
read -p "ディスクサイズ (GB単位) を入力してください: " disk_size

# ディスクのパス
disk_path="/var/lib/libvirt/images/${hostname}.qcow2"

# virt-installコマンドを実行
sudo virt-install \
  --virt-type kvm \
  --cdrom /home/iwai/Downloads/ubuntu-22.04.2-live-server-amd64.iso \
  --os-variant ubuntu22.04 \
  --ram ${memory} \
  --vcpus ${vcpus} \
  --disk path=${disk_path},size=${disk_size} \
  --name ${hostname} \
  --network bridge=virbr0 \
  --noautoconsole

# 実行後にホスト名と関連する情報を表示
echo "ホスト名: ${hostname}"
echo "メモリ: ${memory} MB"
echo "vCPU数: ${vcpus}"
echo "ディスクサイズ: ${disk_size} GB"
echo "仮想マシンが作成されました。"
