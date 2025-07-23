#!/bin/bash

TIMESTAMP=$(date +%Y%m%d%H%M%S)
HOSTS=("ksl-iwai.ksl.ci.kyutech.ac.jp" "tashiro2.ksl.ci.kyutech.ac.jp")
SIZES=(18 82 210 466 978 1234 1472 2002 4050 8146 16338 32722 65489)

echo "UDP testing..."

for SIZE in "${SIZES[@]}"; do
  echo "Testing with message size: $SIZE"
  for HOST in "${HOSTS[@]}"; do
    SAFE_HOST=$(echo "$HOST" | sed 's/[^a-zA-Z0-9]/_/g')
    LOGFILE="netperf-${SAFE_HOST}-${TIMESTAMP}.log"
    ssh "$HOST" "netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m $SIZE" >> "$LOGFILE" 2>&1 &
  done
  wait
done

echo "done."

