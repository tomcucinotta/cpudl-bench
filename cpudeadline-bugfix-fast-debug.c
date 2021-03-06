/*
 *  kernel/sched/mycpudl.c
 *
 *  Global CPU deadline management
 *
 *  Author: Juri Lelli <j.lelli@sssup.it>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; version 2
 *  of the License.
 */

#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include "mycpudeadline_split.h"

#define __TOM__
#ifdef __TOM__
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
#endif

static inline int parent(int i)
{
	return (i - 1) >> 1;
}

static inline int left_child(int i)
{
	return (i << 1) + 1;
}

static inline int right_child(int i)
{
	return (i << 1) + 2;
}

static void mycpudl_heapify_down(struct mycpudl *cp, int idx)
{
	int l, r, largest;

	int orig_cpu = cp->elements[idx].cpu;
	u64 orig_dl = cp->elements[idx].dl;

	if (left_child(idx) >= cp->size)
		return;

	/* adapted from lib/prio_heap.c */
	while(1) {
		u64 largest_dl;
		l = left_child(idx);
		r = right_child(idx);
		largest = idx;
		largest_dl = orig_dl;

		if ((l < cp->size) && dl_time_before(orig_dl,
						cp->elements[l].dl)) {
			largest = l;
			largest_dl = cp->elements[l].dl;
		}
		if ((r < cp->size) && dl_time_before(largest_dl,
						cp->elements[r].dl))
			largest = r;

		if (largest == idx)
			break;

		/* pull largest child onto idx */
		cp->elements[idx].cpu = cp->elements[largest].cpu;
		cp->elements[idx].dl = cp->elements[largest].dl;
		cp->elements[cp->elements[idx].cpu].idx = idx;
		idx = largest;
	}
	/* actual push down of saved original values orig_* */
	cp->elements[idx].cpu = orig_cpu;
	cp->elements[idx].dl = orig_dl;
	cp->elements[cp->elements[idx].cpu].idx = idx;
}

static void mycpudl_heapify_up(struct mycpudl *cp, int idx)
{
	int p;

	int orig_cpu = cp->elements[idx].cpu;
	u64 orig_dl = cp->elements[idx].dl;

	if (idx == 0)
		return;

	do {
		p = parent(idx);
		printk("orig: cpu=%d, dl=%Lu; idx=%d: cpu=%d, dl=%Lu; p=%d: cpu=%d, dl=%Lu\n",
		       orig_cpu, orig_dl, idx, cp->elements[idx].cpu, cp->elements[idx].dl, p, cp->elements[p].cpu, cp->elements[p].dl);
		if (dl_time_before(orig_dl, cp->elements[p].dl))
			break;
		/* pull parent onto idx */
		cp->elements[idx].cpu = cp->elements[p].cpu;
		cp->elements[idx].dl = cp->elements[p].dl;
		cp->elements[cp->elements[idx].cpu].idx = idx;
		idx = p;
	} while (idx != 0);
	/* actual push up of saved original values orig_* */
	cp->elements[idx].cpu = orig_cpu;
	cp->elements[idx].dl = orig_dl;
	cp->elements[cp->elements[idx].cpu].idx = idx;
}

static void mycpudl_heapify(struct mycpudl *cp, int idx)
{
	BUG_ON(idx == IDX_INVALID);

	if (idx > 0 && dl_time_before(cp->elements[parent(idx)].dl,
				cp->elements[idx].dl))
		mycpudl_heapify_up(cp, idx);
	else
		mycpudl_heapify_down(cp, idx);
}

static inline int mycpudl_maximum(struct mycpudl *cp)
{
	return cp->elements[0].cpu;
}

/*
 * mycpudl_find - find the best (later-dl) CPU in the system
 * @cp: the mycpudl max-heap context
 * @p: the task
 * @later_mask: a mask to fill in with the selected CPUs (or NULL)
 *
 * Returns: int - best CPU (heap maximum if suitable)
 */
int mycpudl_find(struct mycpudl *cp, struct task_struct *p,
	       struct cpumask *later_mask)
{
	int best_cpu = -1;
	const struct sched_dl_entity *dl_se = &p->dl;

	if (later_mask &&
	    cpumask_and(later_mask, cp->free_cpus, tsk_cpus_allowed(p))) {
		best_cpu = cpumask_any(later_mask);
		goto out;
	} else if (cpumask_test_cpu(mycpudl_maximum(cp), tsk_cpus_allowed(p)) &&
			dl_time_before(dl_se->deadline, cp->elements[0].dl)) {
		best_cpu = mycpudl_maximum(cp);
		if (later_mask)
			cpumask_set_cpu(best_cpu, later_mask);
	}

out:
	WARN_ON(best_cpu != -1 && !cpu_present(best_cpu));

	return best_cpu;
}

/*
 * mycpudl_clear - remove a cpu from the mycpudl max-heap
 * @cp: the mycpudl max-heap context
 * @cpu: the target cpu
 *
 * Notes: assumes cpu_rq(cpu)->lock is locked
 *
 * Returns: (void)
 */
void mycpudl_clear(struct mycpudl *cp, int cpu)
{
	int old_idx, new_cpu;
	unsigned long flags;

	//WARN_ON(!cpu_present(cpu));

	raw_spin_lock_irqsave(&cp->lock, flags);

	old_idx = cp->elements[cpu].idx;
	if (old_idx == IDX_INVALID) {
		/*
		 * Nothing to remove if old_idx was invalid.
		 * This could happen if a rq_offline_dl is
		 * called for a CPU without -dl tasks running.
		 */
	} else {
		new_cpu = cp->elements[cp->size - 1].cpu;
		cp->elements[old_idx].dl = cp->elements[cp->size - 1].dl;
		cp->elements[old_idx].cpu = new_cpu;
		cp->size--;
		cp->elements[new_cpu].idx = old_idx;
		cp->elements[cpu].idx = IDX_INVALID;
		mycpudl_heapify(cp, old_idx);

		cpumask_set_cpu(cpu, cp->free_cpus);
	}
	raw_spin_unlock_irqrestore(&cp->lock, flags);

#ifdef __TOM__
	dump_heap(cp);
#endif
}

/*
 * mycpudl_set - update the mycpudl max-heap
 * @cp: the mycpudl max-heap context
 * @cpu: the target cpu
 * @dl: the new earliest deadline for this cpu
 *
 * Notes: assumes cpu_rq(cpu)->lock is locked
 *
 * Returns: (void)
 */
void mycpudl_set(struct mycpudl *cp, int cpu, u64 dl)
{
	int old_idx;
	unsigned long flags;

	//WARN_ON(!cpu_present(cpu));

	raw_spin_lock_irqsave(&cp->lock, flags);

	old_idx = cp->elements[cpu].idx;
	if (old_idx == IDX_INVALID) {
		int new_idx = cp->size++;
		cp->elements[new_idx].dl = dl;
		cp->elements[new_idx].cpu = cpu;
		cp->elements[cpu].idx = new_idx;
		mycpudl_heapify_up(cp, new_idx);
		cpumask_clear_cpu(cpu, cp->free_cpus);
	} else {
		cp->elements[old_idx].dl = dl;
		mycpudl_heapify(cp, old_idx);
	}

	raw_spin_unlock_irqrestore(&cp->lock, flags);

#ifdef __TOM__
	dump_heap(cp);
#endif
}

/*
 * mycpudl_set_freecpu - Set the mycpudl.free_cpus
 * @cp: the mycpudl max-heap context
 * @cpu: rd attached cpu
 */
void mycpudl_set_freecpu(struct mycpudl *cp, int cpu)
{
	cpumask_set_cpu(cpu, cp->free_cpus);
}

/*
 * mycpudl_clear_freecpu - Clear the mycpudl.free_cpus
 * @cp: the mycpudl max-heap context
 * @cpu: rd attached cpu
 */
void mycpudl_clear_freecpu(struct mycpudl *cp, int cpu)
{
	cpumask_clear_cpu(cpu, cp->free_cpus);
}

/*
 * mycpudl_init - initialize the mycpudl structure
 * @cp: the mycpudl max-heap context
 */
int mycpudl_init(struct mycpudl *cp, int nr_cpu_ids)
{
	int i;

	memset(cp, 0, sizeof(*cp));
	raw_spin_lock_init(&cp->lock);
	cp->size = 0;

	cp->elements = kcalloc(nr_cpu_ids,
			       sizeof(struct mycpudl_item),
			       GFP_KERNEL);
	if (!cp->elements)
		return -ENOMEM;

	if (!zalloc_cpumask_var(&cp->free_cpus, GFP_KERNEL)) {
		kfree(cp->elements);
		return -ENOMEM;
	}

	for (i = 0; i < nr_cpu_ids; i++)
		cp->elements[i].idx = IDX_INVALID;

#ifdef __TOM__
	num_cpus = nr_cpu_ids;
	dump_heap(cp);
#endif

	return 0;
}

/*
 * mycpudl_cleanup - clean up the mycpudl structure
 * @cp: the mycpudl max-heap context
 */
void mycpudl_cleanup(struct mycpudl *cp)
{
	free_cpumask_var(cp->free_cpus);
	kfree(cp->elements);
}
