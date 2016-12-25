// init: The inital user-level program
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char *argv[] = {"sh", 0};

int main(void)
{
	int pid, wpid;

	if (open("/console", O_RDWR) < 0) {
		console("init: mknod console 1 1\n");
		mknod("/console", 1, 1);
		open("/console", O_RDWR);
	}
	dup(0);
	dup(0);

	//console("hello init\n");
	//write(1, "hello init\n", 11);
	puts("hello from init\n");

	for (;;) {
		puts("init: starting sh\n");
		pid = fork();
		if (pid < 0) {
			puts("init: fork failed\n");
			exit();
		}

		if (pid == 0) {
			exec("sh", argv);
			puts("init: exec sh failed\n");
			exit();
		}

		while ((wpid = wait()) >= 0 && wpid != pid) {
			puts("zombie!\n");
		}
	}
}
