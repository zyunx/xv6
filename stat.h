#ifndef _STAT_H
#define _STAT_H

#include "types.h"

#define T_DIR	1	// Directory
#define T_FILE	2	// File
#define T_DEV	3   // Device

struct stat {
	short type;		// Type of file
	int dev;		// File system's disk device
	uint ino;		// Inode number
	short nlink;	// number of linkes to file
	uint size;		// size of file in bytes
};
#endif
