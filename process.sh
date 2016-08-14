#!/bin/bash

echo "#cpu orig new"
for c in 1 2 4 8 16 32 48 64 96 128 256; do
    echo $c $(grep elapsed orig-$c.dat | sed -e 's/.*: //' | datastat | tail -n +2) $(grep elapsed new-$c.dat | sed -e 's/.*: //' | datastat | tail -n +2);
done
