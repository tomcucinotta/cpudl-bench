obj-m := cpudl-bench.o cpudl-bugfix.o cpudl-bugfix-fast.o
obj-m += cpudl-bugfix-debug.o cpudl-bugfix-fast-debug.o

cpudl-bench-y := cpudl_bench.o mycpudeadline_bak.o

cpudl-bugfix-y := cpudl_bench.o cpudeadline-bugfix.o

cpudl-bugfix-fast-y := cpudl_bench_split.o cpudeadline-bugfix-fast.o

cpudl-bugfix-debug-y := cpudl_bench.o cpudeadline-bugfix-debug.o

cpudl-bugfix-fast-debug-y := cpudl_bench_split.o cpudeadline-bugfix-fast-debug.o

mod:
	make -C /lib/modules/`uname -r`/build M=$$PWD

clean:
	make -C /lib/modules/`uname -r`/build M=$$PWD $@
