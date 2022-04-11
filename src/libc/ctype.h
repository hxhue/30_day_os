#ifndef CTYPE_H
#define CTYPE_H

#if (defined(__cplusplus))
	extern "C" {
#endif

int isalnum(int c);
int isalpha(int c);
int isdigit(int c);
int isspace(int c);
int isblank(int c);

#if (defined(__cplusplus))
  }
#endif

#endif