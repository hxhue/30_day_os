#ifndef SKIPLIST_H
#define SKIPLIST_H

#if (defined(__cplusplus))
	extern "C" {
#endif

typedef struct sl_entry sl_entry;

sl_entry * sl_init(); // Allocates a new list and return its head
void sl_destroy(sl_entry * head); // Frees the resources used by a list
char * sl_get(sl_entry * head, char * key); // Returns a key's value
void sl_set(sl_entry * head, char * key, char * value); // Sets a key's value
void sl_unset(sl_entry * head, char * key); // Removes a key, value pair

#if (defined(__cplusplus))
	}
#endif

#endif
