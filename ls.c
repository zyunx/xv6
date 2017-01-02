#include "user.h"
#include "fcntl.h"
#include "fs.h"

int
readdirent(int fd, struct dirent *de)
{
	int n, r;

	n = 0;
	while ((r = read(fd, ((char*)de) + n, sizeof(*de) - n)) > 0) {
		n += r;
		//printf(2, "r %d n %d\n", r, n);
		if (sizeof(*de) == n)
			return 0;
	}

	if (n == 0)
		return -2;

	return -1;
}

int
main(int argc, char *argv[])
{
	int fd;
	int ret;
	struct dirent de;

	fd = open(".", O_RDONLY);
//	memset(de.name, 0, sizeof(de.name));
//	cnt = read(fd, &de, sizeof(de));
//	printf(2, "%d %s\n", cnt, de.name);
//	while(1);
	while ((ret = readdirent(fd, &de)) == 0) {
		if (de.inum == 0)
			continue;
		printf(1, "%d %s\n", de.inum, de.name); 
		//memset(de.name, 0, sizeof(de.name));
	}
//	while(1);
	if (ret == -1)
		printf(2, "ls: error\n");

	close(fd);

	exit();
}
