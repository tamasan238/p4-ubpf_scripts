#!/bin/bash

# root権限で実行されているか確認
if [ "$(id -u)" -ne 0 ]; then
  echo "このスクリプトはroot権限で実行する必要があります。"
  exit 1
fi

echo "Ubuntu 22.04 Server版のVMを作成します．"
chmod o+x /home/iwai

# ユーザーからの入力を受け取る
read -p "ホスト名を入力してください: " hostname
read -p "メモリ量 (MB単位) を入力してください: " memory
read -p "vCPUの数を入力してください: " vcpus
read -p "ディスクサイズ (GB単位) を入力してください: " disk_size

# ファイルパス
disk_path="/var/lib/libvirt/images/${hostname}.qcow2"
iso_path="/home/iwai/Downloads/ubuntu-22.04.2-live-server-amd64.iso"
template="./autoinstall-config.yaml"

# virt-installコマンドを実行
virt-install \
  --virt-type kvm \
  --cdrom ${iso_path} \
  --os-variant ubuntu22.04 \
  --ram ${memory} \
  --vcpus ${vcpus} \
  --disk path=${disk_path},size=${disk_size} \
  --name ${hostname} \
  --network bridge=virbr0 \
  --noautoconsole \
  --boot cdrom,hd \
  --extra-args "autoinstall ds=nocloud-net;s=/etc/cloud/autoinstall/user-data"

# 自動インストール設定ファイルをVMにコピー
mkdir -p /var/lib/libvirt/images/${hostname}/autoinstall
cp ${autoinstall_config} /var/lib/libvirt/images/${hostname}/autoinstall/user-data

# 仮想マシンの設定を再読み込み
virsh define /etc/libvirt/qemu/${hostname}.xml

# 実行後にホスト名と関連する情報を表示
echo "ホスト名: ${hostname}"
echo "メモリ: ${memory} MB"
echo "vCPU数: ${vcpus}"
echo "ディスクサイズ: ${disk_size} GB"
echo "仮想マシンが作成されました。"
