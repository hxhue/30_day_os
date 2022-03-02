#include "support/tree.h"
#include "task/task.h"
#include "boot/def.h"
#include "boot/gdt.h"
#include "graphics/layer.h"
#include "memory/memory.h"
#include "support/queue.h"
#include "stddef.h"
#include "support/asm.h"
#include "support/debug.h"
#include "support/list.h"
#include "support/type.h"
#include <string.h>
#include <task/task.h>
#include <support/tree.h>
#include <support/queue.h>
#include <stdlib.h>
#include <boot/def.h>

#define SCHEDULER_QUEUE_NUM     3
#define SCHEDULER_QUEUE_URGENT  0
#define SCHEDULER_QUEUE_NEW     1
#define SCHEDULER_QUEUE_OLD     2
#define SCHEDULER_QUEUE_SIZE 1024

typedef struct task_mgr_t {
  list_t queues[SCHEDULER_QUEUE_NUM]; // linked-lists of process_t *
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

process_node_t *current_proc_node;
process_node_t *kernel_proc_node;

process_t *get_proc_from_node(process_node_t *node) {
  return *(process_t **)list_get_value(node);
}

void init_task_mgr() {
  tree_init(&g_task_mgr.process_tree, sizeof(process_t), alloc_mem2, 
            reclaim_mem2, process_cmp);
  for (int i = 0; i < SCHEDULER_QUEUE_NUM; ++i) {
    list_init(&g_task_mgr.queues[i], sizeof(void *), alloc_mem2, reclaim_mem2);
  }
  // Create kernel task
  process_t *p = process_new(9, "kernel");
  // Since the kernel process is the first process, it should be assigned
  // selector numbered KERNEL_GDT_SEL.
  xassert(p->sel == KERNEL_GDT_SEL);
  xassert(p->pid == 0);
  // Do not enqueue it, because it's running
  p->state = PROCSTATE_RUNNING;
  // Since queues share the same malloc() and free() functions, we can use any
  // of them of make the kernel node.
  kernel_proc_node = list_make_node(&g_task_mgr.queues[0], &p); 
  current_proc_node = kernel_proc_node;
  asm_load_tr(p->sel * 8);
}

// TSS: eflags set to 0x00000202, iomap set to 0x40000000, others set to 0.
// It still needs to be modified.
process_t *process_new(int priority, const char *name) {
  static int last_pid = -1;
  process_t *p = NULL;

  // Search for a usable pid and insert the process_t data into the tree
  {
    process_t pdata;
    pdata.pid = (last_pid + 1 + PID_MAX) % PID_MAX;
    while (tree_find(&g_task_mgr.process_tree, &pdata) != (void *)0) {
      pdata.pid = (pdata.pid + 1 + PID_MAX) % PID_MAX;
      if (pdata.pid == last_pid)
        return NULL;
    }

    p = (process_t *)tree_insert(&g_task_mgr.process_tree, &pdata);
    if (!p)
      return NULL;
  }
  
  // The following operations will not change rank of the tree node.
  p->tsnow = p->tsmax = clamp_i32(priority, 0, 9) * 4 + 2;
  strncpy(p->name, name, sizeof(p->name) - 1);
  p->name[sizeof(p->name) - 1] = '\0';
  p->flags = 0;
  // 0: Urgent, 1: New, 2: Old
  p->priority = SCHEDULER_QUEUE_NEW;
  p->state = PROCSTATE_READY;
  tree_init(&p->layers, sizeof(void *), alloc_mem2, reclaim_mem2, 
            layer_pointer_cmp);
  tree_init(&p->children, sizeof(pid_t), alloc_mem2, reclaim_mem2, pid_cmp);
  memset(&p->tss, 0, sizeof(p->tss));
  p->tss.eflags = 0x00000202;
  p->tss.iomap = 0x40000000;
  p->sel = p->pid + SEL_START;

  // Set GDT entry
  segment_descriptor_t *gdt = (segment_descriptor_t *)0x00270000;
  set_gdt_entry(gdt + p->sel, 103, (u32)&p->tss, ACC_TSS32, FLAG_TSS32);

  // Update control data
  last_pid = p->pid;
  return p;
}

list_node_t *process_start(process_t *proc) {
  list_t *list = &g_task_mgr.queues[proc->priority];
  list_node_t *node = list_make_node(list, &proc);
  return list_push_back(list, node);
}

// #define KERNEL_TASK_MAX_DELAY 10
//static int kernel_task_delay = 1;

void process_enqueue(list_node_t *pnode) {
  process_t *proc = get_proc_from_node(pnode);
  list_t *list = &g_task_mgr.queues[proc->priority];
  list_push_back(list, pnode);
}

// Only increment the priority (make it less prior) of the next process. Current
// process is unmodified. (So any modifications on current process should be
// done before calling process_switch().)
// Returns 1 when process switch happens, 0 when it does not.
static int process_switch() {
  // xprintf("%s\n", __func__);
  process_t *p = NULL;
  for (int i = 0; i < SCHEDULER_QUEUE_NUM; ++i) {
    list_t *list = &g_task_mgr.queues[i];
    if (list_size(list) > 0) {
      list_node_t *node = list_pop_front(list);
      p = get_proc_from_node(node);
      if (p->priority + 1 < SCHEDULER_QUEUE_NUM) {
        p->priority++;
      }
      p->state = PROCSTATE_RUNNING;

      process_t *curp = get_proc_from_node(current_proc_node);
      curp->state = PROCSTATE_READY;
      process_enqueue(current_proc_node);
      
      current_proc_node = node;
      break;
    }
  }
  if (p) {
    asm_farjmp(0, p->sel * 8);
    return 1;
  }
  return 0;
}

void process_count_time_slice() {
  process_t *p = get_proc_from_node(current_proc_node);
  // xprintf("%s\n", __func__);
  if (p->flags & PROCFLAG_URGENT) {
    p->flags &= ~(PROCFLAG_URGENT);
    process_switch();
  } else if (--p->tsnow <= 0) {
    p->tsnow = p->tsmax;
    process_switch();
  }
}

// Voluntarily give up time slices immediately.
static inline void process_yield_immediate() {
  process_t *p = get_proc_from_node(current_proc_node);
  p->tsnow = p->tsmax;
  if (!process_switch()) {
    asm_hlt();
  }
}

// Voluntarily give up time slices. Process switch will happen at the next timer
// interrupt.
static inline void process_yield_aligned() {
  process_t *p = get_proc_from_node(current_proc_node);
  p->tsnow = 0;
  asm_hlt();
}

void process_yield() {
  process_yield_immediate();
  // process_yield_aligned();
}

void process_promote(process_node_t *pnode) {
  process_t *proc = get_proc_from_node(pnode);

  // xprintf("pnode: state: %d, urgent: %d. ", proc->state,
  //         (proc->flags & PROCFLAG_URGENT));

  if (proc->state == PROCSTATE_READY && !(proc->flags & PROCFLAG_URGENT)) {
    xassert(pnode != current_proc_node);
    proc->flags |= PROCFLAG_URGENT;
    list_t *list = &g_task_mgr.queues[proc->priority];
    list_unlink(list, pnode);
    list_t *urgent_list = &g_task_mgr.queues[SCHEDULER_QUEUE_URGENT];
    list_push_back(urgent_list, pnode);
    // Check if current task is an urgent task. If not, interrupt it.
    process_t *current_proc = get_proc_from_node(current_proc_node);
    if (!(current_proc->flags & PROCFLAG_URGENT)) {
      if (process_switch()) {
        // xprintf("Deprived\n");
      } else {
        xprintf("No need to deprive\n");
      }
    } else {
      xprintf("Cannot deprive\n");
    }
  } else {
    // xprintf("Aleady urgent\n");
  }
}
