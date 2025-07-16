#!/bin/bash

export P4SAMPLE=~iwai/p4c/testdata/p4_16_samples
export UBPFBIN=~iwai/ubpf/build/bin
export UBPFRUNTIME=~iwai/p4c/backends/ubpf/runtime
export P4FILENAME=$P4SAMPLE/ubpf

if [ $# -ne 1 ]; then
  echo "使用法: $0 {p4-c|c-ubpf|run-ubpf|p4u|build-ubpf}"
  exit 1
fi

case "$1" in
  p4-c)
    p4c-ubpf "$P4FILENAME.p4" -o "$P4FILENAME.c" --emit-externs
    ;;
  c-ubpf)
    clang -O2 -target bpf -c "${P4FILENAME}_ext.c" -o "$P4FILENAME.o" \
      -I "$UBPFRUNTIME" \
      -include "${P4FILENAME}_ext.h" \
      -include "$P4FILENAME.c"
    ;;
  run-ubpf)
    # if pgrep -f "ubpf_test" > /dev/null; then
    #     sudo kill $(pgrep ubpf_test) 2> /dev/null
    #     echo "ubpf_test restarting..."
    # fi
    "$UBPFBIN/ubpf_test" "$P4FILENAME.o" </dev/null >/dev/null 2>&1 &
    ;;
  p4u)
    "$0" p4-c && "$0" c-ubpf && "$0" run-ubpf
    ;;
  build-ubpf)
    sudo cmake --build build -v --target install
    ;;
  *)
    echo "不正なコマンド: $1"
    echo "使用法: $0 {p4-c|c-ubpf|run-ubpf|p4u|build-ubpf}"
    exit 1
    ;;
esac
