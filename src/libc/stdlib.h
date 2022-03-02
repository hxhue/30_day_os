#if (!defined(STDLIB_H))

#define STDLIB_H	1

#if (defined(__cplusplus))
	extern "C" {
#endif

#define RAND_MAX 32767

int  rand();
void srand(unsigned seed);

#if (defined(__cplusplus))
	}
#endif

#endif