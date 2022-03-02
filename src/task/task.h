#ifndef TASK_H
#define TASK_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/tree.h>
#include "support/list.h"

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
	int sel, flags, state;
	// Schedule-related data.
	int priority, tsmax, tsnow; 
	pid_t pid;
	tree_t layers;           // tree of layer_t *
  tree_t children;         // tree of pid
	char name[32];
	TSS32_t tss;
};

typedef list_node_t process_node_t;

void init_task_mgr();

list_node_t *process_start(process_t *proc);

process_t *process_new(int priority, const char *name);

void process_enqueue(list_node_t *pnode);

// Voluntarily give up time slices.
void process_yield();

// Returns selector of next process. If process switch will not happen, -1 is 
// returned.
int process_count_time_slice();

extern process_node_t *current_process;

#if (defined(__cplusplus))
}
#endif

#endif
