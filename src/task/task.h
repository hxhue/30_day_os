#ifndef TASK_H
#define TASK_H

#if (defined(__cplusplus))
extern "C" {
#endif

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

typedef struct pid_node_t {
	pid_t pid;
	struct pid_node_t *next;
} pid_node_t;

typedef struct process_t {
	int sel, flags;
	pid_t pid;
	// Linked lists with head
	struct layer_node_t *layers;
	struct pid_node_t *children;
	char name[32];
	TSS32_t tss;
} process_t;

#if (defined(__cplusplus))
}
#endif

#endif
