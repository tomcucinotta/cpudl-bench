#!/bin/bash

ops=100000

if [ "$1" != "" ]; then
    ops=$1
fi

rmmod cpudl-bench > /dev/null 2>&1
rmmod mycpudl-bench > /dev/null 2>&1
rmmod cpudl-bugfix > /dev/null 2>&1
rmmod cpudl-bugfix-fast > /dev/null 2>&1

for cpus in 1 2 4 8 16 32 48 64 96 128 256; do
    echo "Trying: cpudl-bench.ko num_cpus=$cpus num_ops=$ops"
    insmod ./cpudl-bugfix.ko num_cpus=$cpus num_ops=$ops
    dmesg | tail -$[$ops+2] > orig-$cpus.dat
    rmmod cpudl-bugfix
done

for cpus in 1 2 4 8 16 32 48 64 96 128 256; do
    echo "Trying: mycpudl-bench.ko num_cpus=$cpus num_ops=$ops"
    insmod ./cpudl-bugfix-fast.ko num_cpus=$cpus num_ops=$ops
    dmesg | tail -$[$ops+2] > new-$cpus.dat
    rmmod cpudl-bugfix-fast
done
