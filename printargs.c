#include "user.h"

int main(int argc, char *argv[])
{
	
	char **a;
	int i;

	printf(1, "%d ", argc);
	a = argv;
	while (*a != 0) {
		printf(1, "%s ", *(a++));
	}

/*
	for (i = 0; i < argc; i++)
	{
		if (argv[i] != 0)
			printf(1, "%s ", argv[i]);
	}
	*/
	printf(1, "\n");
	exit();
}
