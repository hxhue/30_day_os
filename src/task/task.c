#include <boot/def.h>
#include <boot/gdt.h>
#include <event/event.h>
#include <event/mouse.h>
#include <event/timer.h>
#include <graphics/draw.h>
#include <graphics/layer.h>
#include <memory/memory.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <support/asm.h>
#include <support/debug.h>
#include <support/list.h>
#include <support/queue.h>
#include <support/tree.h>
#include <support/type.h>
#include <task/task.h>

#define SCHEDULER_QUEUE_NUM     3
#define SCHEDULER_QUEUE_URGENT  0
#define SCHEDULER_QUEUE_NEW     1
#define SCHEDULER_QUEUE_OLD     2
#define SCHEDULER_QUEUE_SIZE 1024

#define DRAW_COUNT_MAX         30
static int ts_count_stop_flag = 0;
static int draw_count_down = DRAW_COUNT_MAX;

void reset_draw_count_down() {
  draw_count_down = DRAW_COUNT_MAX;
}

typedef struct task_mgr_t {
  list_t queues[SCHEDULER_QUEUE_NUM]; // linked-lists of process_t *
  tree_t process_tree;                // tree of process_t
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
process_node_t *draw_proc_node;

process_t *get_proc_from_node(process_node_t *node) {
  return *(process_t **)list_get_value(node);
}

static void init_kernel_proc() {
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

static void init_draw_proc() {
  // Create kernel task
  process_t *p = process_new(9, "draw");
  p->tss.eip = (int)&draw_main;
  p->tss.esp = (int)((char *)alloc_mem_4k(64 * 1024) + 64 * 1024);
  p->tss.es = 1 * 8;
  p->tss.cs = 2 * 8;
  p->tss.ss = 1 * 8;
  p->tss.ds = 1 * 8;
  p->tss.fs = 1 * 8;
  p->tss.gs = 1 * 8;
  draw_proc_node = process_start(p);
}

void init_task_mgr() {
  tree_init(&g_task_mgr.process_tree, sizeof(process_t), alloc_mem2, 
            reclaim_mem2, process_cmp);
  for (int i = 0; i < SCHEDULER_QUEUE_NUM; ++i) {
    list_init(&g_task_mgr.queues[i], sizeof(void *), alloc_mem2, reclaim_mem2);
  }
  init_kernel_proc();
  init_draw_proc();
}

// TSS: eflags set to 0x00000202, iomap set to 0x40000000, others set to 0.
// It still needs to be modified. State is set to READY.
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
  p->tsnow = p->tsmax = clamp_i32(priority, 0, 9) * 2 + 4;
  strncpy(p->name, name, sizeof(p->name) - 1);
  p->name[sizeof(p->name) - 1] = '\0';
  p->flags = 0;
  p->event_mask = 0;
  // p->events = 0;
  queue_init(&p->mouse_msg_queue, sizeof(decoded_mouse_msg_t), 256, alloc_mem2,
             reclaim_mem2);
  // queue_init(&p->layer_msg_queue, sizeof(layer_msg_t), 256, alloc_mem2,
  //            reclaim_mem2);
  queue_init(&p->timer_msg_queue, sizeof(int), 64, alloc_mem2, reclaim_mem2);
  // Priority: 0: Urgent, 1: New, 2: Old.
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


// void process_enqueue(list_node_t *pnode) {
//   process_t *proc = get_proc_from_node(pnode);
//   list_t *list = &g_task_mgr.queues[proc->priority];
//   list_push_back(list, pnode);
// }

// Only increment the priority (make it less prior) of the next process. Current
// process is unmodified. (So any modifications on current process should be
// done before calling process_switch().) If preempt is 1, then this is called
// in process_try_preempt() in which case the current process is pushed to the
// front (instead of the back) of the queue. Returns 1 when process switch
// happens, 0 when it does not.
int process_switch(int preempt) {
  // xprintf("%s\n", __func__);
  process_t *p = NULL;

  // u32 eflags = asm_load_eflags();
  // asm_cli();

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
      
      process_t *proc = get_proc_from_node(current_proc_node);
      list_t *list = &g_task_mgr.queues[proc->priority];
      if (preempt)
        list_push_front(list, current_proc_node);
      else
        list_push_back(list, current_proc_node);
      
      // if (current_proc_node == draw_proc_node) {
      //   reset_draw_count_down();
      // }

      current_proc_node = node;
      break;
    }
  }

  // asm_store_eflags(eflags);

  if (p) {
    asm_farjmp(0, p->sel * 8);
    return 1;
  }
  return 0;
}

void process_try_preempt() {
  // Check if current task is an urgent task. If not, interrupt it.
  process_t *current_proc = get_proc_from_node(current_proc_node);
  if (!(current_proc->flags & PROCFLAG_URGENT)) {
    process_switch(1);
  }
}


void stop_ts_count() {
  ts_count_stop_flag = 1;
}

void resume_ts_count() {
  ts_count_stop_flag = 0;
}

void process_count_time_slice() {
  --draw_count_down;
  
  if (ts_count_stop_flag) {
    return;
  }
  process_t *p = get_proc_from_node(current_proc_node);
  // xprintf("%s\n", __func__);
  if (draw_count_down <= 0) {
    process_set_urgent(draw_proc_node);
    process_try_preempt();
    reset_draw_count_down();
  } else if (p->flags & PROCFLAG_URGENT) {
    p->flags &= ~(PROCFLAG_URGENT);
    process_switch(0);
  } else if (--p->tsnow <= 0) {
    p->tsnow = p->tsmax;
    process_switch(0);
  }
}

// Voluntarily give up time slices immediately.
void process_yield() {
  process_t *p = get_proc_from_node(current_proc_node);
  p->tsnow = p->tsmax;
  if (!process_switch(0)) {
    asm_hlt();
  }
}

int process_set_urgent(process_node_t *pnode) {
  process_t *proc = get_proc_from_node(pnode);

  // xprintf("pnode: state: %d, urgent: %d. ", proc->state,
  //         (proc->flags & PROCFLAG_URGENT));

  if (proc->state == PROCSTATE_READY && !(proc->flags & PROCFLAG_URGENT)) {
    xassert(pnode != current_proc_node);

    // u32 eflags = asm_load_eflags();
    asm_cli();

    proc->flags |= PROCFLAG_URGENT;
    list_t *list = &g_task_mgr.queues[proc->priority];
    list_unlink(list, pnode);
    list_t *urgent_list = &g_task_mgr.queues[SCHEDULER_QUEUE_URGENT];
    list_push_back(urgent_list, pnode);

    // asm_store_eflags(eflags);
    asm_sti();

    return 1;
  }
  return 0;
}

void process_event_listen(process_node_t *pnode, int eventno) {
  if (eventno < 0 || eventno >= 16) {
    return;
  }
  process_t *proc = get_proc_from_node(pnode);
  u16 bit = 1 << eventno;
  if (bit & proc->event_mask) { // Already registered.
    return;
  }
  proc->event_mask |= bit;
}

void process_event_stop_listening(process_node_t *pnode, int eventno) {
  if (eventno < 0 || eventno >= 16) {
    return;
  }
  process_t *proc = get_proc_from_node(pnode);
  u16 bit = 1 << eventno;
  if (!(bit & proc->event_mask)) { // Not registered.
    return;
  }
  proc->event_mask &= ~bit;
}
