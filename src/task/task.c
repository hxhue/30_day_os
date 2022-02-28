#include "support/tree.h"
#include "task/task.h"
#include "graphics/layer.h"
#include "memory/memory.h"
#include "support/queue.h"
#include "support/type.h"
#include <string.h>
#include <task/task.h>
#include <support/tree.h>
#include <support/queue.h>
#include <stdlib.h>
#include <boot/def.h>

#define SCHEDULER_QUEUE_NUM     3
#define SCHEDULER_QUEUE_SIZE 1024

struct process_t {
	int sel, flags;
	int queue, tsmax, tsnow; // Schedule-related
	pid_t pid;
	tree_t layers;           // tree of layer_t *
  tree_t children;         // tree of pid
	char name[32];
	TSS32_t tss;
};

typedef struct task_mgr_t {
  queue_t queues[SCHEDULER_QUEUE_NUM]; // queue of process_t *
  tree_t process_tree;     // tree of process_t
} task_mgr_t;

task_mgr_t g_task_mgr;

int process_cmp(void *a, void *b) {
  process_t *pa = (process_t *)a;
  process_t *pb = (process_t *)b;
  return pa->pid - pb->pid;
}

int pid_cmp(void *a, void *b) {
  return *(pid_t *)a - *(pid_t *)b;
}

// TODO: create kernel task
void init_task_mgr() {
  tree_init(&g_task_mgr.process_tree, sizeof(process_t), alloc_mem2, 
            reclaim_mem2, process_cmp);
  for (int i = 0; i < SCHEDULER_QUEUE_NUM; ++i) {
    queue_init(&g_task_mgr.queues[i], sizeof(void *), SCHEDULER_QUEUE_SIZE, 
               alloc_mem2, reclaim_mem2);
  }
}

// To simplify implmentation: it holds that pid + 4 == sel
#define PID_MAX (GDT_LIMIT - 4)

// priority: [0, 9], higher -> more urgent
process_t *process_new(int priority, const char *name, void (*entry_point)()) {
  static int last_pid = 0;
  process_t *p = (process_t *)0;

  {
    // Search for a usable pid
    process_t process_data;
    process_data.pid = (last_pid + 1) % PID_MAX; // Ignore overflows
    while (tree_find(&g_task_mgr.process_tree, &process_data) != (void *)0) {
      if ((process_data.pid = (process_data.pid + 1) % PID_MAX) == last_pid) {
        return (process_t *)0;
      }
    }

    // Insert into the tree
    p = (process_t *)tree_insert(&g_task_mgr.process_tree, &process_data);
    if (!p) {
      return (process_t *)0;
    }
  }
  
  // The following operations will not change rank of the tree node.
  p->tsnow = p->tsmax = clamp_i32(priority, 0, 9) / 4 + 2;
  strncpy(p->name, name, sizeof(p->name) - 1);
  p->name[sizeof(p->name) - 1] = '\0';
  p->flags = 0;
  p->queue = 0;
  tree_init(&p->layers, sizeof(void *), alloc_mem2, reclaim_mem2, 
            layer_pointer_cmp);
  tree_init(&p->children, sizeof(pid_t), alloc_mem2, reclaim_mem2, pid_cmp);

  // TSS
  memset(&p->tss, 0, sizeof(p->tss));
  p->tss.eflags = 0x00000202; /* IF = 1; */
  p->tss.iomap = 0x40000000;
  p->sel = p->pid + 4;
  // TODO: set GDT entry

  // Update control data
  last_pid = p->pid;
  return p;
}

// "proc" cannot be in queues
void process_enqueue(process_t *proc) {
  queue_t *q = &g_task_mgr.queues[proc->queue];
  queue_push(q, &proc);
}
