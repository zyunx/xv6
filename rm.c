#include "user.h"

int
main(int argc, char *argv[])
{
	char **arg;

	if (argc < 2) {
		printf(2, "usage:\n    rm <pathname>\n");
		exit();
	}

	arg = argv + 1;
	while (*arg != 0)
	{
		if (unlink(*arg) != 0)
			printf(2, "%s is not unlinked\n", *arg);

		arg++;
	}	
	exit();
}
