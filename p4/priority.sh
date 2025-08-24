#!/bin/bash

for PID in $(pgrep ubpf); do
    sudo chrt -f -p 99 $PID
    chrt -p $PID
done