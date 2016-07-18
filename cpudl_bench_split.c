#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/slab.h>
#include "mycpudeadline_split.h"

static long long *times;

static int num_ops = 1000;
module_param(num_ops, int, 0);

static int num_cpus = 64;
module_param(num_cpus, int, 0);

static __inline__ unsigned long long tick(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

static struct mycpudl _cpudl;

static int cpudl_bench_init(void)
{
   int i;
   long long beg, t1;
   u64 dl = 123456789;
   int inserts = 0;

   printk("init: num_cpus=%d num_ops=%d\n", num_cpus, num_ops);

   if (mycpudl_init(&_cpudl, num_cpus) != 0) {
     printk("mycpudl_init() failed!\n");
     return -1;
   }

   times = kmalloc(sizeof(times[0]) * num_ops, GFP_KERNEL);
   if (times == NULL) {
     printk("init failed\n");
     return -1;
   }

   beg = tick();
   for (i = 0; i < num_ops; i++) {
     if ((dl & 5) != 5) {
       printk("%d set: %d %Lu\n", i, (i % num_cpus), dl);
       mycpudl_set(&_cpudl, i % num_cpus, dl);
       inserts++;
     } else {
       printk("%d clr: %d\n", i, (i % num_cpus));
       mycpudl_clear(&_cpudl, i % num_cpus);
     }
     dl = dl * 1103515245 + 7917;
     times[i] = tick();
   }

   printk("cpudl_bench: performed %d inserts over %d\n", inserts, num_ops);

   t1 = beg;
   for (i = 0; i < num_ops; i++) {
     printk("elapsed: %Ld\n", times[i] - t1);
     t1 = times[i];
   }

   return 0;
}


static void cpudl_bench_cleanup(void)
{
  printk("cleanup\n");
  mycpudl_cleanup(&_cpudl);
  
  if (times != NULL) {
    kfree(times);
    times = NULL;
  }
}  

module_init(cpudl_bench_init);
module_exit(cpudl_bench_cleanup);

MODULE_LICENSE("GPL");
