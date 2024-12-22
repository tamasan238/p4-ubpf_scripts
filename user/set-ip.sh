#!/bin/bash

INTERFACE="enp1s0"
NETPLAN_CONFIG="/etc/netplan/01-netcfg.yaml"

cat <<EOF | sudo tee $NETPLAN_CONFIG > /dev/null
network:
  version: 2
  renderer: networkd
  ethernets:
    $INTERFACE:
      dhcp4: no
      addresses:
        - 192.168.1.119/24
      gateway4: 192.168.1.1
      nameservers:
        addresses:
          - 131.206.203.254
EOF

if [ $? -ne 0 ]; then
  echo "エラー: Netplan設定ファイルを書き込む権限がありません。"
  exit 1
fi

echo "Netplan設定を適用しています..."
sudo netplan apply

if [ $? -eq 0 ]; then
  echo "Netplan設定が正常に適用されました。"
else
  echo "エラー: Netplan設定の適用に失敗しました。"
  exit 1
fi

echo "現在のネットワーク設定:"
ip addr show $INTERFACE
