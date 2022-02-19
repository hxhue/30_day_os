#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#if (defined(__cplusplus))
	extern "C" {
#endif

typedef struct allocator_t allocator_t;

void init_allocator(allocator_t *alloc, void *buf, unsigned size, unsigned ele_size);

#if (defined(__cplusplus))
	}
#endif

#endif
