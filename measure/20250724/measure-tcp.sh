#!/bin/bash

TIMESTAMP=$(date +%Y%m%d%H%M%S)
HOSTS=("ksl-iwai.ksl.ci.kyutech.ac.jp" "tashiro2.ksl.ci.kyutech.ac.jp")
SIZES=(6 70 198 454 966 1222 1460 1990 4038 8134 16326 32710 65477)

echo "TCP testing..."

for SIZE in "${SIZES[@]}"; do
  echo "Testing with message size: $SIZE"
  for HOST in "${HOSTS[@]}"; do
    SAFE_HOST=$(echo "$HOST" | sed 's/[^a-zA-Z0-9]/_/g')
    LOGFILE="netperf-${SAFE_HOST}-${TIMESTAMP}.log"
    ssh "$HOST" "netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m $SIZE" >> "$LOGFILE" 2>&1 &
  done
  wait
done

echo "done."

