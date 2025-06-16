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
echo "6, 70, 198, 454, 966, 1222, 1460, 1990, 4038, 8134, 16326, 32710, 65477"
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 6
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 70
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 198
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 454
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 966
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 1222
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 1460
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 1990
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 4038
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 8134
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 16326
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 32710
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t TCP_STREAM -- -m 65477
sleep 10

echo "[UDP]"
echo "netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m xxx"
echo "18, 82, 210, 466, 978, 1234, 1472, 2002, 4050, 8146, 16338, 32722, 65489"
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 18
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 82
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 210
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 466
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 978
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 1234
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 1472
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 2002
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 4050
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 8146
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 16338
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 32722
sleep 10
netperf -H olive-vm1.ksl.ci.kyutech.ac.jp -t UDP_STREAM -- -m 65489
sleep 10

