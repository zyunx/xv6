#include "user.h"
#include "fcntl.h"

void cat1(int from, int to)
{
	char buf[256];
	int n;

	while((n = read(from, buf, 256)) > 0) 
			write(to, buf, n);

}
int main(int argc, char *argv[])
{
	int fd;
	char** arg;

	if (argc == 1) {
		cat1(0, 1);

	} else {
		arg = argv + 1;
		while (*arg != 0) {
			fd = open(*(arg++), O_RDONLY);
			cat1(fd, 1);
			close(fd);
		}
	}	
	exit();
}
