#!/bin/bash

H=$1
D=/tmp/p4shield/netperf

rm -rf "$D"
mkdir -p "$D"

echo ""
echo "## Latency ##"
echo ""

echo "[TCP]"
echo "netperf -t omni -H $H -- -d rr -T TCP -k MIN_LATENCY,MEAN_LATENCY,P90_LATENCY,P99_LATENCY,MAX_LATENCY,STDDEV_LATENCY"
netperf -t omni -H $H -- -d rr -T TCP -k MIN_LATENCY,MEAN_LATENCY,P90_LATENCY,P99_LATENCY,MAX_LATENCY,STDDEV_LATENCY

echo "[UDP]"
echo "netperf -t omni -H $H -- -d rr -T UDP -k MIN_LATENCY,MEAN_LATENCY,P90_LATENCY,P99_LATENCY,MAX_LATENCY,STDDEV_LATENCY"
netperf -t omni -H $H -- -d rr -T UDP -k MIN_LATENCY,MEAN_LATENCY,P90_LATENCY,P99_LATENCY,MAX_LATENCY,STDDEV_LATENCY

echo ""
echo "## Throughput ##"
echo ""

echo "[TCP]"
for M in 6 70 198 454 966 1222 1460 1990 4038 8134 16326 32710 65477
do
  echo "$ netperf -H $H -t TCP_STREAM -- -m $M"
  netperf -H "$H" -t TCP_STREAM -- -m "$M" | tee "$D"/tcp_${M}.log \
  | tee >(python3 parse-bps.py "$D"/tcp-bps.log)
  echo ""
  sleep 5
done

echo "[UDP]"
for M in 18 82 210 466 978 1234 1472 2002 4050 8146 16338 32722 65489
do
  echo "$ netperf -H $H -t UDP_STREAM -- -m $M"
  netperf -H "$H" -t UDP_STREAM -- -m "$M" | tee "$D"/udp_${M}.log \
  | tee >(python3 parse-bps.py "$D"/udp-bps.log) \
  | tee >(python3 parse-udp-pps.py "$D"/udp-pps.log)
  sleep 5
done

echo "[TCP: m, Mbps]"
echo "6, 70, 198, 454, 966, 1222, 1460, 1990, 4038, 8134, 16326, 32710, 65477"
cat "$D"/tcp-bps.log | paste -sd ',' -

echo "[UDP: m, Mbps]"
echo "18, 82, 210, 466, 978, 1234, 1472, 2002, 4050, 8146, 16338, 32722, 65489"
cat "$D"/udp-bps.log | paste -sd ',' -

echo "[UDP: m, pps]"
echo "18, 82, 210, 466, 978, 1234, 1472, 2002, 4050, 8146, 16338, 32722, 65489"
cat "$D"/udp-pps.log | paste -sd ',' -
