#!/bin/bash

sed -e 's/cpudl/mycpudl/g' \
    -e 's/WARN_ON(!cpu_present(cpu))/\/\/WARN_ON(!cpu_present(cpu))/' \
    -e 's/WARN_ON(idx == IDX_INVALID || !cpu_present(idx))/WARN_ON(idx == IDX_INVALID)/' \
    -e 's/BUG_ON(!cpu_present(cpu))/\/\/BUG_ON(!cpu_present(cpu))/' \
    -e 's/BUG_ON(idx == IDX_INVALID || !cpu_present(idx))/BUG_ON(idx == IDX_INVALID)/' \
    -e 's/mycpudl_init(struct mycpudl \*cp)/mycpudl_init(struct mycpudl *cp, int nr_cpu_ids)/' \
    -e 's/#include "cpudeadline.h"/#include "mycpudeadline_split.h"/' \
    -e 's/for_each_possible_cpu(i)/for (i = 0; i < nr_cpu_ids; i++)/' \
    cpudeadline-bugfix-fast-orig.c > cpudeadline-bugfix-fast.c

sed -e 's/cpudl/mycpudl/g' \
    -e 's/WARN_ON(!cpu_present(cpu))/\/\/WARN_ON(!cpu_present(cpu))/' \
    -e 's/WARN_ON(idx == IDX_INVALID || !cpu_present(idx))/WARN_ON(idx == IDX_INVALID)/' \
    -e 's/BUG_ON(!cpu_present(cpu))/\/\/BUG_ON(!cpu_present(cpu))/' \
    -e 's/BUG_ON(idx == IDX_INVALID || !cpu_present(idx))/BUG_ON(idx == IDX_INVALID)/' \
    -e 's/mycpudl_init(struct mycpudl \*cp)/mycpudl_init(struct mycpudl *cp, int nr_cpu_ids)/' \
    -e 's/#include "cpudeadline.h"/#include "mycpudeadline.h"/' \
    -e 's/for_each_possible_cpu(i)/for (i = 0; i < nr_cpu_ids; i++)/' \
    cpudeadline-bugfix-orig.c > cpudeadline-bugfix.c

cat <<EOF
Please, produce the cpudeadline*-debug.c files:

1) add manually the debugginf code:

static int num_cpus;
static void dump_heap(struct mycpudl *cp) {
  int i, size;
  printk("DUMP: size=%d\n", cp->size);
  for (i = 0; i < num_cpus; i++) {
    printk("DUMP: i=%d cpu=%d dl=%Lu idx=%d\n", i, cp->elements[i].cpu, cp->elements[i].dl, cp->elements[i].idx);
  }

  size = 0;
  for (i = 0; i < num_cpus; i++) {
    int idx = cp->elements[i].idx;
    if (idx != IDX_INVALID) {
      size++;
      if (cp->elements[idx].cpu != i)
	printk("ERROR: elems[%d].idx=%d but elems[%d].cpu=%d\n", i, idx, idx, cp->elements[idx].cpu);
    }
  }
  for (i = 1; i < cp->size; i++) {
    if (dl_time_before(cp->elements[(i-1)>>1].dl, cp->elements[i].dl))
      printk("ERROR: elems[%d].dl=%Lu before elems[%d].dl=%Lu\n", (i-1)>>1, cp->elements[(i-1)>>1].dl, i, cp->elements[i].dl);
  }
  if (size != cp->size)
    printk("ERROR: size=%d but cp->size=%d\n", size, cp->size);
}

2) add manually calls to dump_heap() at the end of cpudl_set() and cpudl_clear()

3) add manually in init():
  num_cpus=nr_cpu_ids;

EOF
