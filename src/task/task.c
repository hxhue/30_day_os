#include "support/tree.h"
#include "task/task.h"
#include "memory/memory.h"
#include <task/task.h>
#include <support/tree.h>

typedef struct task_mgr_t {
	// tree_t process_tree;
} task_mgr_t;

task_mgr_t g_task_mgr;

int process_cmp(void *a, void *b) {
  process_t *pa = (process_t *)a;
  process_t *pb = (process_t *)b;
  return pa->pid - pb->pid;
}

void init_task_mgr() {
  // tree_init(&g_task_mgr.process_tree, sizeof(process_t), alloc_mem2, 
  //           reclaim_mem2, process_cmp);
  
}
