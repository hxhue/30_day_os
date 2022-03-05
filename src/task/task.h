#ifndef TASK_H
#define TASK_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/queue.h>
#include <support/tree.h>
#include <support/list.h>

typedef struct TSS32_t {
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
} TSS32_t;

struct layer_t;
typedef struct layer_node_t {
	struct layer_t *layer;
	struct layer_node_t *next;
} layer_node_t;

// pid_t is signed, so the difference of two pid_t is safe in range of int.
typedef int pid_t;

typedef struct process_t process_t;

enum ProcessState {
  PROCSTATE_READY,
	PROCSTATE_RUNNING,
  PROCSTATE_BLOCKED,
};

enum ProcessFlags {
	PROCFLAG_URGENT = 0x01, // Related to state "ready".
};

struct process_t {
	int sel, state;
	unsigned flags;
	unsigned short event_mask, events;
	// Schedule-related data.
	int priority, tsmax, tsnow; 
	pid_t pid;
	tree_t layers;           // tree of layer_t *
  tree_t children;         // tree of pid
	char name[32];
	TSS32_t tss;
	queue_t mouse_msg_queue; // queue of decoded_mouse_msg_t
	queue_t layer_msg_queue; // queue of layer_msg_t
};

typedef list_node_t process_node_t;
extern process_node_t *kernel_proc_node;
extern process_node_t *current_proc_node;

void init_task_mgr();

process_node_t *process_start(process_t *proc);
process_t *process_new(int priority, const char *name);
void process_yield();            // Voluntarily give up time slices.
void process_count_time_slice(); // May trigger process switch when current
                                 // process runs out of time slices.

// Process "proc" has received some urgent task, and needs it to be done as soon
// as possible. This process will be moved into a high priority queue
// immediately, but given limited time slices. When work is done, the original
// time slice count is restored instead of being reset. This function only
// adjust the queue the process is in, so process switch won't happen
// immediately. Returns 1 on success, 0 on failure.
int process_set_urgent(process_node_t *pnode);

process_t *get_proc_from_node(process_node_t *node);

// When kernel dicides to send irq-related data to a process, it checks if the
// process wants the data. If so, the data is tranfered, and the process is
// updated with a higher priority (smaller priority field value). For a certain
// irq packet, not every process which has registered IRQs will receive it.
// TODO: This may be merged into signals.
void process_register_event(process_node_t *pnode, int eventno);
void process_unregister_event(process_node_t *pnode, int eventno);

int  process_switch();
void process_try_preempt(); // Calls process_switch() when current process is
                            // not urgent.

#if (defined(__cplusplus))
}
#endif

#endif
