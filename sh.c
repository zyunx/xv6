#include "user.h"
#include "fcntl.h"

// cmd type
#define EXEC	1

#define MAXARGS	10

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
	char *s;
	int ret;

	s = *ps;
	while (s < es && strchr(whitespace, *s))
		s++;
	if (q)
		*q = s;
	ret = *s;

	switch(*s) {
		case 0:
			break;
		case '|':
		case '(':
		case ')':
		case ';':
		case '&':
		case '<':
			s++;
			break;
		case '>':
			s++;
			if (*s == '>') {
				ret = '+';
				s++;
			}
			break;
		default:
			ret = 'a';
			while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
				s++;
			break;
	}
	if (eq)
		*eq = s;
	
	while (s < es && strchr(whitespace, *s))
		s++;

	*ps = s;
	return ret;
}

int peek(char **ps, char *es, char *toks)
{
	char *s;

	s = *ps;
	while (s < es && strchr(whitespace, *s))
		s++;
	*ps = s;
	return *s && strchr(toks, *s);
}

struct cmd {
	int type;
};

struct execcmd {
	int type;
	char *argv[MAXARGS];
	char *eargv[MAXARGS];
};


int
getcmd(char *buf, int nbuf)
{
	printf(2, "# ");
	memset(buf, 0, nbuf);
	gets(buf, nbuf);
	if (buf[0] == 0) // EOF
		return -1;
	return 0;
}


struct cmd*
parsecmd(char *s)
{
//	parseexec(s	
	return 0;
}

void
runcmd(struct cmd *cmd)
{
	struct execcmd *ecmd;
	exit();
/*
	switch (cmd->type)
	{
		case EXEC:
			ecmd = cmd;
			if (ecmd->argv[0] == 0)
				exit();
			exec(ecmd->argv[0], ecmd->argv);
			printf(2, "exec %s failed\n", ecmd->argv[0]);
			break;
		default:
			cprintf("invalid command\n");
			exit();

	}
	*/
}

int
main(int argc, char *argv[])
{
	char buf[100];

	int fd;
	char line[128];
	struct cmd cmd;
	int pid, wpid;

	// ensure that three file descriptor are open.
	while ((fd = open("console", O_RDWR)) >= 0) {
		if (fd >= 3) {
			close(fd);
			break;
		}
	}

	printf(1, "hello from sh\n");
	while(getcmd(buf, sizeof(buf)) >= 0) {
			
		pid = fork();

		if (pid == 0) {
			runcmd(parsecmd(buf));
		}

		wpid = wait();
	}
	exit();
}
