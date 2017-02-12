#ifndef _DEFS_H
#define _DEFS_H

#include <types.h>
#include <param.h>
#include <buf.h>
#include <stat.h>
#include <fs.h>
#include <file.h>

#define NELEM(x)	(sizeof(x)/sizeof((x)[0]))

// kalloc.c
char *kalloc();

// string.c
int memcmp(const void *v1, const void *v2, uint n);
void *memmove(void *dst, const void *src, uint n);
void *memset(void *dst, int c, uint n);
int strncmp(const char *p, const char *q, uint n);

// console.c
void consoleinit();
void cprintf(char*, ...);
void panic(char *) __attribute__((noreturn));

// kalloc.c
void alloc(void);
void kfree(char *);
void kinit1(void *, void *);
void kinit2(void *, void *);

// vm.c
void kvmalloc(void);			// kernel page table
pde_t* setupkvm(void);
void inituvm(pde_t*, char *, uint);
pde_t* copyuvm(pde_t *pgdir, uint sz);

// proc.c
void pinit(void);
void userinit(void);
void scheduler(void)	__attribute__((noreturn)); 
void sched();
void yield();
void sleep(void *chan, struct spinlock *lk);

// syscall.c
int			argint(int, int*);
int			argptr(int, char **, int);
int			argstr(int, char**);
int			fetchint(uint, int*);
int			fetchstr(uint, char**);
void		syscall(void);

// picirq.c
void picinit(void);
void picenable(int);

// timer.c
void timerinit(void);

// ide.c
void iderw(struct buf *);
void ideinit(void);

// bio.c
struct buf *bread(uint, uint);
void bwrite(struct buf *);
void brelse(struct buf *);
void binit(void);

// log.c
void log_write(struct buf *);

// fs.c
void readsb(int dev, struct superblock *sb);
int	dirlink(struct inode*, char *, uint);
struct inode*	dirlookup(struct inode*, char *, uint*);
struct inode*	ialloc(uint, short);
struct inode*	idup(struct inode*);
void			iinit(int dev);
void			initloc(int dev);
void			ilock(struct inode*);
void			iput(struct inode*);
void			iunlock(struct inode*);
void			iunlockput(struct inode*);
void			iupdate(struct inode*);
int				namecmp(const char*, const char *);
struct inode*	namei(char*);
struct inode*	nameiparent(char *, char*);
int				readi(struct inode*, char*, uint, uint);
void			stati(struct inode*, struct stat *);
int				writei(struct inode*, char*, uint, uint);


// file.c
struct file * 	filealloc(void);
void 			fileclose(struct file*);
struct file*	filedup(struct file*);
void			fileinit(void);
int				fileread(struct file*, char *, int n);
int				filestat(struct file*, struct stat*);
int				filewrite(struct file*, char *, int n);

// pipe.c
int				pipealloc(struct file**, struct file**);
void			pipeclose(struct pipe*, int);
int				piperead(struct pipe*, char *, int);
int				pipewrite(struct pipe*, char *, int);

// mp.c
extern int		ismp;
void			mpinit(void);

// lapic.c
extern volatile uint *lapic;
extern uchar ioapicid;
void			lapicinit(void);
void			lapiceoi(void);
void			lapicstartap(uchar apicid, uint addr);
#endif
