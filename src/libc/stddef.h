/* copyright(C) 2003 H.Kawai (under KL-01). */

#if (!defined(STDDEF_H))

#define STDDEF_H	1

#if (defined(__cplusplus))
	extern "C" {
#endif

typedef unsigned int size_t;
typedef int ptrdiff_t;

#if (!defined(NULL))
	#define NULL	((void *) 0)
#endif

#if (defined(__cplusplus))
	}
#endif

#endif
