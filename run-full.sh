#!/bin/bash

for c in $(seq 0 $[$(grep -c processor /proc/cpuinfo) - 1]); do
    sudo cpufreq-set -c $c -g performance;
done

for ops in 1000 10000 100000; do
    nohup sudo ./run-all.sh $ops;
    ./process.sh | tee all.dat
    ./draw-cpudl.gp
    mkdir newrun-$ops && mv nohup.out *.dat cpudl.pdf newrun-$ops/
done

for c in $(seq 0 $[$(grep -c processor /proc/cpuinfo) - 1]); do
    sudo cpufreq-set -c $c -g powersave;
done
