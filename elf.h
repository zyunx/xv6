// Format of the ELF executable file
#ifndef _ELF_H
#define _ELF_H

#include "types.h"

#define ELF_MAGIC 0x464C457FU

// File header
struct elfhdr {
	uint magic;		// must equal ELF_MAGIC
	uchar elf[12];
	ushort type;
	ushort machine;
	uint version;
	uint entry;
	uint phoff;
	uint shoff;
	uint flags;
	ushort ehsize;
	ushort phentsize;
	ushort phnum;
	ushort shentsize;
	ushort shnum;
	ushort shstrndx;
};


// Program segment header
struct proghdr {
	uint type;
	uint off;
	uint vaddr;
	uint paddr;
	uint filesz;
	uint memsz;
	uint flags;
	uint align;
};

// Values for proghdr type
#define ELF_PROG_LOAD		1

// Flag bits for proghdr flags
#define ELF_PROG_FLAG_EXEC	1
#define ELF_PROG_FLAG_WRITE 2
#define ELC_PROG_FLAG_READ	4

#endif
