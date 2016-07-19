#ifndef _LINUX_CPUDL_H
#define _LINUX_CPUDL_H

#include <linux/sched.h>
#include <linux/sched/deadline.h>

#define IDX_INVALID     -1

struct mycpudl_item {
	u64 dl;
	int cpu;
	int idx;
};

struct mycpudl {
	raw_spinlock_t lock;
	int size;
	cpumask_var_t free_cpus;
	struct mycpudl_item *elements;
};


#ifdef CONFIG_SMP
int mycpudl_find(struct mycpudl *cp, struct task_struct *p,
	       struct cpumask *later_mask);
void mycpudl_set(struct mycpudl *cp, int cpu, u64 dl, int is_valid);
int mycpudl_init(struct mycpudl *cp, int nr_cpu_ids);
void mycpudl_set_freecpu(struct mycpudl *cp, int cpu);
void mycpudl_clear_freecpu(struct mycpudl *cp, int cpu);
void mycpudl_cleanup(struct mycpudl *cp);
#endif /* CONFIG_SMP */

#endif /* _LINUX_CPUDL_H */
