echo "[TCP]"
echo "netperf -t omni -H olive-vm1.ksl.ci.kyutech.ac.jp -- -d rr -T TCP -k MIN_LATENCY,MEAN_LATENCY,P90_LATENCY,P99_LATENCY,MAX_LATENCY,STDDEV_LATENCY"
netperf -t omni -H olive-vm1.ksl.ci.kyutech.ac.jp -- -d rr -T TCP -k MIN_LATENCY,MEAN_LATENCY,P90_LATENCY,P99_LATENCY,MAX_LATENCY,STDDEV_LATENCY
sleep 10

echo "[UDP]"
echo "netperf -t omni -H olive-vm1.ksl.ci.kyutech.ac.jp -- -d rr -T UDP -k MIN_LATENCY,MEAN_LATENCY,P90_LATENCY,P99_LATENCY,MAX_LATENCY,STDDEV_LATENCY"
netperf -t omni -H olive-vm1.ksl.ci.kyutech.ac.jp -- -d rr -T UDP -k MIN_LATENCY,MEAN_LATENCY,P90_LATENCY,P99_LATENCY,MAX_LATENCY,STDDEV_LATENCY
sleep 10

echo "[TCP]"
echo "netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m xxx"
echo "6, 1460, 65477"
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 6
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 1460
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 65477
sleep 10

echo "[UDP]"
echo "netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m xxx"
echo "18, 1472, 65489"
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 18
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 1472
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 65489
sleep 10

