#!/bin/bash

dmesg -C
insmod ./cpudl-bugfix-debug.ko num_cpus=32 num_ops=10000
dmesg | grep 'DUMP\|set:\|clr:\|ERROR' | sed -e 's/^.*DUMP: //' -e 's/^.*set:/set:/' -e 's/^.*clr:/clr:/' > bugfix.out
rmmod cpudl-bugfix-debug

dmesg -C
insmod ./cpudl-bugfix-fast-debug.ko num_cpus=32 num_ops=10000
dmesg | grep 'DUMP\|set:\|clr:\|ERROR' | sed -e 's/^.*DUMP: //' -e 's/^.*set:/set:/' -e 's/^.*clr:/clr:/' > bugfix-fast.out
rmmod cpudl-bugfix-fast-debug
