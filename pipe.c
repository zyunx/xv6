#include <types.h>
#include <defs.h>
#include <param.h>
#include <spinlock.h>

struct pipe {
	struct spinlock lock;

};


int
pipealloc(struct file **f0, struct file **f1)
{
	return -1;
}

void
pipeclose(struct pipe *p, int writable)
{
}


int
pipewrite(struct pipe *p, char *addr, int n)
{
	return -1;
}

int piperead(struct pipe *p, char *addr, int n)
{
	return -1;
}
