#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"

// Memory allocator by Kernighan and Ritchie,
// The C programming lanuage, 2nd ed. Section 8.7

typedef long Align;

union header {
	struct {
		union header *ptr;
		uint size;
	} s;
	Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

void
free(void *ap)
{
	Header *bp, *p;

	bp = (header*)ap - 1;
	
}
