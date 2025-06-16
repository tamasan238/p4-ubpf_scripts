#!/bin/bash

set -e  # エラー時に終了

# 処理対象のディレクトリ
dirs=(0 1 5 10 50 100 500)

for dir in "${dirs[@]}"; do
  if [ -d "$dir/ovs" ]; then
    echo "=== $dir/ovs のビルドを開始します ==="
    (
      cd "$dir/ovs"
      sudo bash -c 'make clean ; ./boot.sh ; ./configure --with-dpdk=static ; make -j$(nproc) && make install'
    )
    echo "=== $dir/ovs のビルドが完了しました ==="
  else
    echo "ディレクトリ $dir/ovs が存在しません。スキップします。"
  fi
done

