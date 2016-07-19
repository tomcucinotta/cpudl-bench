#!/usr/bin/gnuplot

set terminal pdf color
set output 'cpudl.pdf'

set grid
set xlabel "CPUs"
set ylabel "TSCs"
set xtics (1,2,4,8,16,32,64,128,256)
set logscale x

set multiplot

set size 1,0.7
set origin 0,0.3

plot [2:] \
     'all.dat' u 1:2 t 'orig' w lp, \
     'all.dat' u 1:3 t 'new' w lp

set size 0.97,0.3
set origin 0.03,0

set ylabel "Speed-up (%)"
set ytics 2
unset xlabel

plot [2:] \
     'all.dat' u 1:(100*($2-$3)/$2) t 'speed-up' w lp
