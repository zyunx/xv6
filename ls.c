#include "user.h"
#include "fcntl.h"
#include "fs.h"

int
main(int argc, char *argv[])
{
	int fd;
	int cnt;
	struct dirent de;

	fd = open(".", O_RDONLY);
//	memset(de.name, 0, sizeof(de.name));
//	cnt = read(fd, &de, sizeof(de));
//	printf(2, "%d %s\n", cnt, de.name);
//	while(1);
	while ((cnt = read(fd, &de, sizeof(de))) == sizeof(de)) {
		if (de.inum == 0)
			break;
		printf(1, "%s\t%d\n", de.name, de.inum); 
		//memset(de.name, 0, sizeof(de.name));
	}
//	while(1);
	if (cnt != sizeof(de))
		printf(2, "ls: error\n");

	close(fd);

	exit();
}
