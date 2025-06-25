#!/bin/bash

# 対象ディレクトリ一覧
dirs=(0 1 5 10 50 100 500)

# user_dpdk VMのIPアドレス（必要に応じて変更してください）
USER_DPDK_IP="olive-vm1.ksl.ci.kyutech.ac.jp"

# Host AのSSH接続先
HOST_A="ksl-iwai.local"

for dir in "${dirs[@]}"; do
  ovs_dir="$dir/ovs"

  # ログファイル名を設定
  log_file="${dir}_ovs.log"

  if [ ! -d "$ovs_dir" ]; then
    echo "ディレクトリ $ovs_dir が存在しません。スキップします。" | tee -a "$log_file"
    continue
  fi

  # ログファイルと標準出力の両方に出力するために、サブシェルで処理をまとめる
  (
    echo "=== $ovs_dir の処理を開始します ==="

    # make install 実行
    (
      cd "$ovs_dir"
      echo "make install を実行中..."
      sudo make install
    )

    # /usr/local/share/openvswitch/scripts/ovs-ctl start
    echo "/usr/local/share/openvswitch/scripts/ovs-ctl をスタートします..."
    sudo /usr/local/share/openvswitch/scripts/ovs-ctl start

    # user_dpdk VM起動
    echo "user_dpdk VM を起動します..."
    virsh start user_dpdk
    sleep 15

    # netperfによる起動確認 (Host AにSSH接続し、user_dpdk VMへnetperfを実行)
    echo "ksl-iwaiから、user_dpdk VMに対してnetperf実行中..."
    ssh "$HOST_A" bash -c "'
      echo \"user_dpdk VM (${USER_DPDK_IP}) に対して netperf テスト実行中...\"
      ping -c 5 ${USER_DPDK_IP}
      ./test2.sh
    '"

    # user_dpdk VMをシャットダウン
    echo "user_dpdk VM をシャットダウンします..."
    virsh shutdown user_dpdk
    sleep 15

    # /usr/local/share/openvswitch/scripts/ovs-ctl stop
    echo "/usr/local/share/openvswitch/scripts/ovs-ctl を停止します..."
    sudo /usr/local/share/openvswitch/scripts/ovs-ctl stop
    sleep 5

    echo "=== $ovs_dir の処理が完了しました ==="
  ) 2>&1 | tee -a "$log_file"

done

echo "すべての処理が完了しました。"

