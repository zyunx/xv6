#ifndef _USER_H
#define _USER_H

#include "types.h"

// system calls
int exec(char *, char **);
int console(char *);
int mkdir(char *);
int mknod(char *, int, int);
int open(char *, int);
int close(int);
int write(int, void*, int);
int read(int, void*, int);
int dup(int);
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
char* sbrk(int);

// ulib.c
uint strlen(char *);
char* strchr(const char *s, char c);
void *memset(void *dst, int c, int n);

void printf(int, char *fmt, ...);
char * gets(char *buf, int max);

void *malloc(uint);


#endif
