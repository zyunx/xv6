#include "user.h"
#include "fcntl.h"

// cmd type
#define EXEC	1
#define REDIR	2
#define PIPE	3

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
struct redircmd {
	int type;
	struct cmd *cmd;
	char *file;
	char *efile;
	int mode;
	int fd;
};
struct pipecmd {
	int type;
	struct cmd *left;
	struct cmd *right;
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
	struct redircmd *rcmd;
	struct pipecmd *pcmd;

	if (cmd == 0)
		return 0;

	switch(cmd->type) {
		case EXEC:
			ecmd = (struct execcmd*)cmd;
			for (i = 0; ecmd->argv[i]; i++)
				*ecmd->eargv[i] = 0;
			break;
		case REDIR:
			rcmd = (struct redircmd*)cmd;
			nulterminate(rcmd->cmd);
			*rcmd->efile = 0;
			break;
		case PIPE:
			pcmd = (struct pipecmd*)cmd;
			nulterminate(pcmd->left);
			nulterminate(pcmd->right);
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
redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
	struct redircmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = REDIR;
	cmd->cmd = subcmd;
	cmd->file = file;
	cmd->efile = efile;
	cmd->mode = mode;
	cmd->fd = fd;
	return (struct cmd*) cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
	struct pipecmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = PIPE;
	cmd->left = left;
	cmd->right = right;
	return (struct cmd*)cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
	int tok;
	char *q, *eq;

	while (peek(ps, es, "<>")) {
		tok = gettoken(ps, es, 0, 0);
		if (gettoken(ps, es, &q, &eq) != 'a') {
			panic("missing file for redirection");
		}
		switch (tok) {
			case '<':
				cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
				break;
			case '>':
				cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
				break;
			case '+':
				cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
				break;
		}
			
	}
	return cmd;
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
	while (!peek(ps, es, "|)&;><")) {
		if ((tok = gettoken(ps, es, &q, &eq)) == 0)
			break;
				
		if (tok != 'a')
			panic("syntax error");
	
		cmd->argv[argc] = q;
		cmd->eargv[argc] = eq;
		argc++;
		if (argc >= MAXARGS)
			panic("too many args");

		ret = parseredirs(ret, ps, es);
	}
	cmd->argv[argc] = 0;
	cmd->eargv[argc] = 0;

	return ret;
}

struct cmd*
parsepipe(char **ps, char *es)
{
	struct cmd *cmd;

	cmd = parseexec(ps, es);
	if (peek(ps, es, "|")) {
		gettoken(ps, es, 0, 0);
		cmd = pipecmd(cmd, parsepipe(ps, es));
	}
	return cmd;
}

struct cmd*
parsecmd(char *s)
{
	int i;
	char *es;
	struct cmd *cmd;
	struct execcmd *ecmd;

	es = s + strlen(s);
	//cmd = parseexec(&s, es); 
	cmd = parsepipe(&s, es);
	nulterminate(cmd);
	return cmd;
}

void printcmd(struct cmd *cmd)
{
	struct execcmd *ecmd;
	struct redircmd *rcmd;
	struct pipecmd *pcmd;

	switch (cmd->type)
	{
		case EXEC:
			ecmd = (struct execcmd*) cmd;
			printf(2, "%s", ecmd->argv[0]);
			break;

		case REDIR:
			rcmd = (struct redircmd*)cmd;
			printcmd(rcmd->cmd);
			printf(2, " +%s", rcmd->file);
			break;

		case PIPE:
			pcmd = (struct pipecmd*)cmd;
			printcmd(pcmd->left);
			printf(2, " | ");
			printcmd(pcmd->right);
			break;
		default:
			printf(2, "errcmd\n");

	}

}

int
fork1(void)
{
	int pid;

	pid = fork();
	if (pid == -1)
		panic("fork");
	return pid;
}

void
runcmd(struct cmd *cmd)
{
	int i;
	int p[2];
	struct execcmd *ecmd;
	struct redircmd *rcmd;
	struct pipecmd *pcmd;

	switch (cmd->type)
	{
		case EXEC:
			ecmd = (struct execcmd*) cmd;
			if (ecmd->argv[0] == 0)
				exit();
		
			exec(ecmd->argv[0], ecmd->argv);
			printf(2, "exec %s failed\n", ecmd->argv[0]);
			exit();
			break;

		case REDIR:
			rcmd = (struct redircmd*)cmd;
			close(rcmd->fd);
			if (open(rcmd->file, rcmd->mode) < 0) {
				printf(2, "open %s failed\n", rcmd->file);
				exit();
			}
			runcmd(rcmd->cmd);
			break;

		case PIPE:
			pcmd = (struct pipecmd*)cmd;
			//printcmd(cmd);
			if (pipe(p) < 0)
				panic("pipe");
			if (fork1() == 0) {
				close(1);
				dup(p[1]);
				close(p[0]);
				close(p[1]);
				runcmd(pcmd->left);
			}	
			if (fork1() == 0) {
				close(0);
				dup(p[0]);
				close(p[0]);
				close(p[1]);
				runcmd(pcmd->right);
			}
			close(p[0]);
			close(p[1]);
			wait();
			wait();
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
