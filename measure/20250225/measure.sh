#!/bin/bash

# 記録対象のパケットサイズ一覧（計測用）
TCP_PACKET_SIZES=(6 70 198 454 966 1222 1460 1990 4038 8134 16326 32710 65477)
UDP_PACKET_SIZES=(18 82 210 466 978 1234 1472 2002 4050 8146 16338 32722 65489)

# 記録するファイル名（共通）
FILE_SIZES=(64 128 256 512 1024 1280 1518 2048 4096 8192 16384 32768 65535)

# ホスト名
HOST="olive-vm1.ksl.ci.kyutech.ac.jp"

# タグ名
TAG="OVS-Batch"

# TCP測定
for i in "${!TCP_PACKET_SIZES[@]}"; do
    SIZE=${TCP_PACKET_SIZES[$i]}
    FILE_SIZE=${FILE_SIZES[$i]}
    echo "パケットサイズ $SIZE (TCP) の測定を開始します..."

    # ログを記録するファイル名
    LOG_FILE="tcp_${FILE_SIZE}.txt"

    # journalctl をバックグラウンドで実行し、そのプロセスIDを記録
    journalctl -f -t "$TAG" | awk '{print $NF}' > "$LOG_FILE" &
    JPID=$!

    # netperf 実行 (TCP)
    netperf -H "$HOST" -t TCP_STREAM -- -m "$SIZE"

    # journalctl を終了
    kill "$JPID"

    echo "パケットサイズ $SIZE (TCP) の測定が終了しました。ログを ${LOG_FILE} に保存しました。"

    # 5秒間待機
    echo "次の測定まで5秒間待機します..."
    sleep 5
done

# UDP測定
for i in "${!UDP_PACKET_SIZES[@]}"; do
    SIZE=${UDP_PACKET_SIZES[$i]}
    FILE_SIZE=${FILE_SIZES[$i]}
    echo "パケットサイズ $SIZE (UDP) の測定を開始します..."

    # ログを記録するファイル名
    LOG_FILE="udp_${FILE_SIZE}.txt"

    # journalctl をバックグラウンドで実行し、そのプロセスIDを記録
    journalctl -f -t "$TAG" | awk '{print $NF}' > "$LOG_FILE" &
    JPID=$!

    # netperf 実行 (UDP)
    netperf -H "$HOST" -t UDP_STREAM -- -m "$SIZE"

    # journalctl を終了
    kill "$JPID"

    echo "パケットサイズ $SIZE (UDP) の測定が終了しました。ログを ${LOG_FILE} に保存しました。"

    # 5秒間待機
    echo "次の測定まで5秒間待機します..."
    sleep 5
done

echo "全ての測定が完了しました。"

