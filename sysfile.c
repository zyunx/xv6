//
// File-system system calls
// Mostly argumnet checking, since we don't trust
// user code, and calls into file.c and fs.c
//

#include <types.h>
#include <defs.h>
#include <proc.h>
#include <file.h>
#include <fcntl.h>

// Fetch the nth word-size system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file
static int
argfd(int n, int *pfd, struct file **pf)
{
	int fd;
	struct file *f;

	if (argint(n, &fd) < 0)
		return -1;
	if (fd < 0 || fd >= NOFILE || (f = proc->ofile[fd]) == 0)
		return -1;

	if (pfd)
		*pfd = fd;
	if (pf)
		*pf = f;
	return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
	int fd;

	for (fd = 0; fd < NOFILE; fd++) {
		if (proc->ofile[fd] == 0) {
			proc->ofile[fd] = f;
			return fd;
		}
	}
	return -1;
}

static struct inode*
create(char *path, short type, short major, short minor)
{
	uint off;
	struct inode *ip, *dp;
	char name[DIRSIZ];

	if ((dp = nameiparent(path, name)) == 0)
		return 0;
	DBG_P("[create] parent inum %d\n", dp->inum);

	ilock(dp);

	if ((ip = dirlookup(dp, name, &off)) != 0) {
		iunlockput(dp);
		ilock(ip);
		if (type == T_FILE && ip->type == T_FILE)
			return ip;
		iunlockput(ip);
		return 0;
	}

	if ((ip = ialloc(dp->dev, type)) == 0)
		panic("create: ialloc");

	ilock(ip);
	ip->major = major;
	ip->minor = minor;
	ip->nlink = 1;
	iupdate(ip);

	if(type == T_DIR) {	// Create . and .. entries.
		dp->nlink++;		// for ".."
		iupdate(dp);
		// No ip->nlink++ for "." : avoid cyclic ref count.
		if (dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
			panic("create dots");
	}

	if (dirlink(dp, name, ip->inum) < 0)
		panic("create: dirlink");

	iunlockput(dp);

	return ip;
}

int
sys_open(void)
{
	char *path;
	int fd, omode;
	struct file *f;
	struct inode *ip;

	//extern void test_fs();
	//test_fs();
	//return 0;
	DBG_P("[sys_open]\n");

	if (argstr(0, &path) < 0 || argint(1, &omode) < 0)
		return -1;

	begin_op();

	if (omode & O_CREATE) {
		DBG_P("sys_create: create %s\n", path);
		ip = create(path, T_FILE, 0, 0);
		if (ip == 0) {
			end_op();
			return -1;
		}
	} else {
		if ((ip = namei(path)) == 0) {
			DBG_P("[sys_open] not exist\n");
			end_op();
			return -1;
		}
		ilock(ip);
		if (ip->type == T_DIR && omode != O_RDONLY) {
			iunlockput(ip);
			end_op();
			return -1;
		}
	}


	if ((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0) {
		if (f)
			fileclose(f);
		iunlockput(ip);
		end_op();
		return -1;
	}
	iunlock(ip);
	end_op();

	f->type = FD_INODE;
	f->ip = ip;
	f->off = 0;
	f->readable = !(omode & O_WRONLY);
	f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

	DBG_P("[sys_open] fd %d\n", fd);
	return fd;
}

int sys_close(void)
{
	int fd;
	struct file *f;
	DBG_P("[sys_close] \n");
	if (argfd(0, &fd, &f) < 0)
		return -1;
	DBG_P("[sys_close] fd %d\n", fd);
	proc->ofile[fd] = 0;
	fileclose(f);
	return 0;
}

int
sys_write(void)
{
	struct file *f;
	int n;
	char *p;

	if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
		return -1;
	return filewrite(f, p, n);
}

int
sys_read(void)
{
	struct file *f;
	int n;
	char *p;

	DBG_P("[sys_read]\n");
	if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
		return -1;

	n = fileread(f, p, n);
	return n;
}

int
sys_exec(void)
{
	char *path, *argv[MAXARG];
	int i;
	uint uargv, uarg;

	DBG_P("[sys_exec]\n");
	if (argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0) {
		return -1;
	}
	memset(argv, 0, sizeof(argv));
	for (i = 0;; i++) {
		if (i >= NELEM(argv))
			return -1;
		if (fetchint(uargv+4*i, (int*)&uarg) < 0)
			return -1;
		if (uarg == 0) {
			argv[i] = 0;
			break;
		}
		if (fetchstr(uarg, &argv[i]) < 0)
			return -1;
	}
	DBG_P("[sys_exec] path %s\n", path);
	i = 0;
	while (argv[i] != 0) {
		DBG_P("[sys_exec] arg %d: %s\n", i, argv[i]);
		i++;
	}
	return exec(path, argv);
}

int
sys_mkdir(void)
{
	char *path;
	struct inode *ip;

	cprintf("sys_mkdir\n");
	begin_op();
	if (argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0) {
		end_op();
		return -1;
	}
	iunlockput(ip);
	end_op();
	return 0;
}

int
sys_mknod(void)
{
	struct inode *ip;
	char *path;
	int major, minor;

	cprintf("sys_mknod\n");
	begin_op();

	if ((argstr(0, &path)) < 0 ||
			argint(1, &major) < 0 ||
			argint(2, &minor) < 0 ||
			(ip = create(path, T_DEV, major, minor)) == 0) {
		
		cprintf("path %s\n", path);
		cprintf("major %d\n", major);
		cprintf("minor %d\n", minor);
		end_op();

		return -1;
	}
	iunlockput(ip);
	end_op();
	return 0;
}

int
sys_dup(void)
{
	struct file *f;
	int fd;

	if (argfd(0, 0, &f) < 0)
		return -1;
	if ((fd = fdalloc(f)) < 0)
		return -1;
	filedup(f);
	return fd;
}

int
sys_pipe(void)
{
	int *fd;
	struct file *rf, *wf;
	int fd0, fd1;

	if (argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
		return -1;
	if (pipealloc(&rf, &wf) < 0)
		return -1;
	fd0 = -1;
	if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0) {
		if (fd0 >= 0)
			proc->ofile[fd0] = 0;
		fileclose(rf);
		fileclose(wf);
		return -1;
	}
	fd[0] = fd0;
	fd[1] = fd1;
	return 0;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
	int off;
	struct dirent de;

	for (off = 2*sizeof(de); off < dp->size; off+=sizeof(de)) {
		if (readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
			panic("isdirempty: readi");
		if (de.inum != 0)
			return 0;
	}
	return 1;
}


int
sys_unlink(void)
{
	struct inode *ip, *dp;
	struct dirent de;
	char name[DIRSIZ], *path;
	uint off;

	if (argstr(0, &path) < 0)
		return -1;

	begin_op();
	if ((dp = nameiparent(path, name)) == 0) {
		end_op();
		return -1;
	}

	ilock(dp);

	// Canot unlink "." or "..".
	if (namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
		goto bad;

	if ((ip = dirlookup(dp, name, &off)) == 0)
		goto bad;
	ilock(ip);

	if (ip->nlink < 1)
		panic("unlink: nlink < 1");
	if (ip->type == T_DIR & !isdirempty(ip)) {
		iunlockput(ip);
		goto bad;
	}

	memset(&de, 0, sizeof(de));
	if (writei(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
		panic("unlink: writei");
	DBG_P("[sys_unlink] unlink %s\n", name);
	if (ip->type == T_DIR) {
		dp->nlink--;
		iupdate(dp);
	}
	iunlockput(dp);

	ip->nlink--;
	iupdate(ip);
	iunlockput(ip);

	end_op();
	return 0;

bad:
	iunlockput(dp);
	end_op();
	return -1;

}
