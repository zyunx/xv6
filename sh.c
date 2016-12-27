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
nulterminate(struct cmd *cmd)
{
	int i;
	struct execcmd *ecmd;

	if (cmd == 0)
		return 0;

	switch(cmd->type) {
		case EXEC:
			ecmd = (struct execcmd*)cmd;
			for (i = 0; ecmd->argv[i]; i++)
				*ecmd->eargv[i] = 0;
			/*
			for (i = 0; ecmd->argv[i]; i++)
				printf(1, "%s ", ecmd->argv[i]);
			exit();
			*/
			break;
	}

	return cmd;
}
// Constructor
struct cmd*
execcmd(void)
{
	struct execcmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = EXEC;
	return (struct cmd*)cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
	char *q, *eq;
	int tok, argc;
	struct execcmd *cmd;
	struct cmd *ret;

	ret = execcmd();	
	cmd = (struct execcmd*) ret;

	argc = 0;
	while ((tok = gettoken(ps, es, &q, &eq)) != 0)
	{
		if (tok != 'a')
			return 0;
	//	printf(1, "%c\n", *q);
	//	printf(1, "%c\n", *eq);
		cmd->argv[argc] = q;
		cmd->eargv[argc] = eq;
		argc++;
		if (argc >= MAXARGS)
			return 0;
	}

	cmd->argv[argc] = 0;
	cmd->eargv[argc] = 0;
	return ret;
}

struct cmd*
parsecmd(char *s)
{
	char *es;
	struct cmd *cmd;

	es = s + strlen(s);
	cmd = parseexec(&s, es); 
	nulterminate(cmd);
	return cmd;
}

void
runcmd(struct cmd *cmd)
{
	int i;
	struct execcmd *ecmd;

	switch (cmd->type)
	{
		case EXEC:
			ecmd = (struct execcmd*) cmd;
			if (ecmd->argv[0] == 0)
				exit();
		
			exec(ecmd->argv[0], ecmd->argv);
			printf(2, "exec %s failed\n", ecmd->argv[0]);
			break;
		default:
			printf(2, "invalid command\n");
			exit();

	}
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
//	printf(1, "sbrk %x\n", sbrk(10));
//	printf(1, "sbrd %x\n", sbrk(1000));
	while(getcmd(buf, sizeof(buf)) >= 0) {
			
		pid = fork();

		if (pid == 0) {
			runcmd(parsecmd(buf));
		}

		wpid = wait();
	}
	exit();
}
