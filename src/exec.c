/*
 *
 * exec.c - command execution
 *
 * This file is part of zsh, the Z shell.
 *
 * This software is Copyright 1992 by Paul Falstad
 *
 * Permission is hereby granted to copy, reproduce, redistribute or otherwise
 * use this software as long as: there is no monetary profit gained
 * specifically from the use or reproduction of this software, it is not
 * sold, rented, traded or otherwise marketed, and this copyright notice is
 * included prominently in any copy made.
 *
 * The author make no claims as to the fitness or correctness of this software
 * for any use whatsoever, and it is provided as is. Any use of this software
 * is at the user's own risk.
 *
 */

#include "zsh.h"
#include <errno.h>

#define execerr() { if (forked) _exit(1); \
	closemnodes(mfds); lastval = 1; return; }

static Lklist args;

/* parse list in a string */

List parselstring(s)		/**/
char *s;
{
    List l;

    hungets(s);
    strinbeg();
    pushheap();
    if (!(l = parse_list())) {
	strinend();
	hflush();
	popheap();
	return NULL;
    }
    strinend();
    return l;
}

/* execute a string */

void execstring(s)		/**/
char *s;
{
    List l;

    if ((l = parselstring(s))) {
	execlist(l);
	popheap();
    }
}

/* fork and set limits */

int phork()
{				/**/
    int pid;

#ifdef RLIM_INFINITY
    int t0;

#endif

    if (thisjob >= MAXJOB - 1) {
	zerr("job table full", NULL, 0);
	return -1;
    }
    pid = fork();
    if (pid == -1) {
	zerr("fork failed: %e", NULL, errno);
	return -1;
    }
#ifdef RLIM_INFINITY
    if (!pid)
	for (t0 = 0; t0 != RLIM_NLIMITS; t0++)
	    setrlimit(t0, limits + t0);
#endif
    return pid;
}

/* execute a current shell command */

int execcursh(cmd)		/**/
Cmd cmd;
{
    runlist(cmd->u.list);
    cmd->u.list = NULL;
    return lastval;
}

/* execve after handling $_ and #! */

#define POUNDBANGLIMIT 64

int zexecve(pth, argv)		/**/
char *pth;
char **argv;
{
    int eno;
    static char buf[MAXPATHLEN * 2];
    char **eep;

    for (eep = environ; *eep; eep++)
	if (**eep == '_' && (*eep)[1] == '=')
	    break;
    buf[0] = '_';
    buf[1] = '=';
    if (*pth == '/')
	strcpy(buf + 2, pth);
    else
	sprintf(buf + 2, "%s/%s", pwd, pth);
    if (!*eep)
	eep[1] = NULL;
    *eep = buf;
    execve(pth, argv, environ);
    if ((eno = errno) == ENOEXEC) {
	char execvebuf[POUNDBANGLIMIT + 1], *ptr, *ptr2, *argv0;
	int fd, ct, t0;

	if ((fd = open(pth, O_RDONLY)) >= 0) {
	    argv0 = *argv;
	    *argv = pth;
	    ct = read(fd, execvebuf, POUNDBANGLIMIT);
	    close(fd);
	    if (ct > 0) {
		if (execvebuf[0] == '#')
		    if (execvebuf[1] == '!') {
			for (t0 = 0; t0 != ct; t0++)
			    if (execvebuf[t0] == '\n')
				execvebuf[t0] = '\0';
			execvebuf[POUNDBANGLIMIT] = '\0';
			for (ptr = execvebuf + 2; *ptr && *ptr == ' '; ptr++);
			for (ptr2 = ptr; *ptr && *ptr != ' '; ptr++);
			if (*ptr) {
			    *ptr = '\0';
			    argv[-2] = ptr2;
			    argv[-1] = ptr + 1;
			    execve(ptr2, argv - 2, environ);
			} else {
			    argv[-1] = ptr2;
			    execve(ptr2, argv - 1, environ);
			}
		    } else {
			argv[-1] = "sh";
			execve("/bin/sh", argv - 1, environ);
		} else {
		    for (t0 = 0; t0 != ct; t0++)
			if (!execvebuf[t0])
			    break;
		    if (t0 == ct) {
			argv[-1] = "sh";
			execve("/bin/sh", argv - 1, environ);
		    }
		}
	    } else
		eno = errno;
	    *argv = argv0;
	} else
	    eno = errno;
    }
    return eno;
}

#define MAXCMDLEN (MAXPATHLEN*4)

/* execute an external command */

void execute(dash)		/**/
int dash;
{
    static Lklist exargs;
    char **argv, *arg0, **pp;
    char *z, *s, buf[MAXCMDLEN], buf2[MAXCMDLEN];
    int ee, eno = 0;
    Cmdnam cn;

    if (empty(args)) {
	zerr("no command", NULL, 0);
	_exit(1);
    }
    if (!exargs && (s = zgetenv("STTY"))) {
	exargs = args;
	args = NULL;
	zyztem("stty", s);
	args = exargs;
	exargs = NULL;
    }
    arg0 = (char *)peekfirst(args);
    cn = (Cmdnam) gethnode(arg0, cmdnamtab);
    if (cn && (cn->flags & DISABLED))
	cn = NULL;
    if ((z = zgetenv("ARGV0"))) {
	setdata(firstnode(args), (vptr) ztrdup(z));
	delenv(z - 6);
    } else if (dash) {
	sprintf(buf2, "-%s", arg0);
	setdata(firstnode(args), (vptr) ztrdup(buf2));
    }
    argv = makecline(args);
    unblockchld();
    if ((int)strlen(arg0) > MAXPATHLEN) {
	zerr("command too long: %s", arg0, 0);
	_exit(1);
    }
    for (s = arg0; *s; s++)
	if (*s == '/') {
	    errno = zexecve(arg0, argv);
	    if (arg0 == s || unset(PATHDIRS) ||
		(arg0[0] == '.' && (arg0 + 1 == s ||
				    (arg0[1] == '.' && arg0 + 2 == s)))) {
		zerr("%e: %s", arg0, errno);
		_exit(1);
	    }
	    break;
	}
    if (cn && ISEXCMD(cn->flags)) {
	char nn[MAXPATHLEN];

	if (cn->flags & BUILTIN)
	    strcpy(nn, cn->u.cmd);
	else {
	    for (pp = path; pp < cn->u.name; pp++)
		if (**pp == '.' && (*pp)[1] == '\0') {
		    ee = zexecve(arg0, argv);
		    if (ee != ENOENT)
			eno = ee;
		} else if (**pp != '/') {
		    z = buf;
		    strucpy(&z, *pp);
		    *z++ = '/';
		    strcpy(z, arg0);
		    ee = zexecve(buf, argv);
		    if (ee != ENOENT)
			eno = ee;
		}
	    strcpy(nn, cn->u.name ? *(cn->u.name) : "");
	    strcat(nn, "/");
	    strcat(nn, cn->nam);
	}
	ee = zexecve(nn, argv);

	if (ee != ENOENT)
	    eno = ee;
    }
    for (pp = path; *pp; pp++)
	if ((*pp)[0] == '.' && !(*pp)[1]) {
	    ee = zexecve(arg0, argv);
	    if (ee != ENOENT)
		eno = ee;
	} else {
	    z = buf;
	    strucpy(&z, *pp);
	    *z++ = '/';
	    strcpy(z, arg0);
	    ee = zexecve(buf, argv);
	    if (ee != ENOENT)
		eno = ee;
	}
    if (eno)
	zerr("%e: %s", arg0, eno);
    else
	zerr("command not found: %s", arg0, 0);
    _exit(1);
}

#define try(X) { if (iscom(X)) return ztrdup(X); }

/* get the full pathname of an external command */

char *findcmd(arg0)		/**/
char *arg0;
{
    char **pp;
    char *z, *s, buf[MAXCMDLEN];
    Cmdnam cn;

    cn = (Cmdnam) gethnode(arg0, cmdnamtab);
    if (!cn && isset(HASHCMDS))
	cn = hashcmd(arg0, path);
    if (cn && cn->flags & DISABLED)
	cn = NULL;
    if ((int)strlen(arg0) > MAXPATHLEN)
	return NULL;
    for (s = arg0; *s; s++)
	if (*s == '/') {
	    try(arg0);
	    if (arg0 == s || unset(PATHDIRS)) {
		return NULL;
	    }
	    break;
	}
    if (cn && ISEXCMD(cn->flags)) {
	char nn[MAXPATHLEN];

	if (cn->flags & BUILTIN)
	    strcpy(nn, cn->u.cmd);
	else {
	    for (pp = path; pp < cn->u.name; pp++)
		if (**pp != '/') {
		    z = buf;
		    strucpy(&z, *pp);
		    *z++ = '/';
		    strcpy(z, arg0);
		    try(buf);
		}
	    strcpy(nn, cn->u.name ? *(cn->u.name) : "");
	    strcat(nn, "/");
	    strcat(nn, cn->nam);
	}
	try(nn);
    }
    for (pp = path; *pp; pp++) {
	z = buf;
	strucpy(&z, *pp);
	*z++ = '/';
	strcpy(z, arg0);
	try(buf);
    }
    return NULL;
}

int iscom(s)			/**/
char *s;
{
    struct stat statbuf;

    return (access(s, X_OK) == 0 && stat(s, &statbuf) >= 0 &&
	    S_ISREG(statbuf.st_mode));
}

int isrelative(s)		/**/
char *s;
{
    if (*s != '/')
	return 1;
    for (; *s; s++)
	if (*s == '.' && s[-1] == '/' &&
	    (s[1] == '/' || s[1] == '\0' ||
	     (s[1] == '.' && (s[2] == '/' || s[2] == '\0'))))
	    return 1;
    return 0;
}

Cmdnam hashcmd(arg0, pp)	/**/
char *arg0;
char **pp;
{
    char *s, buf[MAXPATHLEN];
    char **pq;
    DIR *dir;
    struct dirent *de;
    Cmdnam cn;

    for (; *pp; pp++)
	if (**pp == '/') {
	    s = buf;
	    strucpy(&s, *pp);
	    *s++ = '/';
	    strcpy(s, arg0);
	    if (iscom(buf))
		break;
	}
    if (!*pp || isrelative(*pp))
	return NULL;
    cn = (Cmdnam) zcalloc(sizeof *cn);
    cn->flags = EXCMD;
    cn->u.name = pp;
    addhnode(ztrdup(arg0), cn, cmdnamtab, freecmdnam);
    if (unset(HASHDIRS))
	return cn;
    for (pq = pathchecked; pq <= pp; pq++) {
	if (isrelative(*pq) || !(dir = opendir(*pq)))
	    continue;
	readdir(dir);
	readdir(dir);
	while ((de = readdir(dir)))
	    addhcmdnode(de->d_name, pq);
	closedir(dir);
    }
    pathchecked = pp + 1;
    return cn;
}

void fullhash()
{				/**/
    char **pq;
    DIR *dir;
    struct dirent *de;

    for (pq = pathchecked; *pq; pq++) {
	if (isrelative(*pq) || !(dir = opendir(*pq)))
	    continue;
	readdir(dir);
	readdir(dir);
	while ((de = readdir(dir)))
	    addhcmdnode(de->d_name, pq);
	closedir(dir);
    }
    pathchecked = pq;
}

void execlist(list)		/**/
List list;
{
    static int donetrap;

    if (breaks)
	return;
    if (!list || list == &dummy_list)
	return;
    donetrap = 0;
    simplifyright(list);
    switch (list->type) {
    case SYNC:
    case ASYNC:
	execlist2(list->left, list->type, !list->right);
	if (sigtrapped[SIGDEBUG])
	    dotrap(SIGDEBUG);
	if (sourcelevel < 32768 && !donetrap) {
	    if (sigtrapped[SIGZERR] && lastval) {
		dotrap(SIGZERR);
		donetrap = 1;
	    }
	    if (lastval && isset(ERREXIT)) {
		if (sigtrapped[SIGEXIT])
		    dotrap(SIGEXIT);
		exit(lastval);
	    }
	}
	if (list->right && !retflag) {
	/* errflag = 0; */
	    execlist(list->right);
	}
	break;
    }
}

void execlist2(list, type, last1)	/**/
Sublist list;
int type;
int last1;
{
    if (!list)
	return;
    switch (list->type) {
    case END:
	execpline(list, type, last1);
	break;
    case ORNEXT:
	if (!execpline(list, SYNC, 0))
	    execlist2(list->right, type, last1);
	else
	    while ((list = list->right))
		if (list->type == ANDNEXT) {
		    execlist2(list->right, type, last1);
		    return;
		}
	break;
    case ANDNEXT:
	if (execpline(list, SYNC, 0))
	    execlist2(list->right, type, last1);
	else
	    while ((list = list->right))
		if (list->type == ORNEXT) {
		    execlist2(list->right, type, last1);
		    return;
		}
	break;
    }
}

static int in_pipe;

int execpline(l, how, last1)	/**/
Sublist l;
int how;
int last1;
{
    int ipipe[2], opipe[2];

    if (!l)
	return 0;
    ipipe[0] = ipipe[1] = opipe[0] = opipe[1] = 0;
    if (in_pipe) {
	execpline2(l->left, how, opipe[0], ipipe[1], last1);
	return lastval;		/* if we're in a pipe don't change lastval; it'll be */
    }				/* done by the first instance of execpline */
    blockchld();
    if ((thisjob = getfreejob()) == -1)
	return 1;
    initjob();
    if (how == TIMED) {
	jobtab[thisjob].stat |= STAT_TIMED;
	how = SYNC;
    }
    if (l->flags & PFLAG_COPROC) {
	how = ASYNC;
	if (coprocin >= 0) {
	    close(coprocin);
	    close(coprocout);
	}
	mpipe(ipipe);
	mpipe(opipe);
	coprocin = ipipe[0];
	coprocout = opipe[1];
    }
    execpline2(l->left, how, opipe[0], ipipe[1], last1);
    if (how == ASYNC) {
	if (l->flags & PFLAG_COPROC)
	    close(ipipe[1]);
	spawnjob();
	unblockchld();
	return 1;
    } else {
	waitjobs();
	unblockchld();
	if (l->flags & PFLAG_NOT)
	    lastval = !lastval;
	return !lastval;
    }
}

void execpline2(pline, how, input, output, last1)	/**/
Pline pline;
int how;
int input;
int output;
int last1;
{
    int pid;
    int pipes[2];
    int oldlineno = lineno;

    if (breaks)
	return;
    if (!pline)
	return;
    lineno = pline->left->lineno;
    if (pline->type == END) {
	in_pipe = 0;
	execcmd(pline->left, input, output, how == ASYNC, last1);
	pline->left = NULL;
    } else {
	int old_pipe = in_pipe;

	in_pipe = 0;
	mpipe(pipes);
	if (pline->left->type >= CURSH && how == SYNC) {

	/* if we are doing "foo | bar" where foo is a current
				shell command, do foo in a subshell and do
				the rest of the pipeline in the current shell. */

	    int synch[2];

	    pipe(synch);
	    if (!(pid = fork())) {
		close(pipes[0]);
		close(synch[0]);
		entersubsh(how == ASYNC);
		close(synch[1]);
		exiting = 1;
		execcmd(pline->left, input, pipes[1], how == ASYNC, 0);
		_exit(lastval);
	    } else if (pid == -1) {
		close(synch[0]);
		close(synch[1]);
		zerr("fork failed: %e", NULL, errno);
	    } else {
		char dummy, *text = getjobtext((vptr) pline->left);

		addproc(pid, text);
		close(synch[1]);
		read(synch[0], &dummy, 1);
		close(synch[0]);
	    }
	} else {
	/* otherwise just do the pipeline normally. */
	    execcmd(pline->left, input, pipes[1], how == ASYNC, 0);
	}
	in_pipe = old_pipe;
	pline->left = NULL;
	close(pipes[1]);
	if (pline->right) {
	/* if another execpline() is invoked because the command is a list it
	   must know that we're already in a pipeline */
	    in_pipe = 1;
	    execpline2(pline->right, how, pipes[0], output, last1);
	    close(pipes[0]);
	}
    }

    lineno = oldlineno;
}

/* make the argv array */

char **makecline(list)		/**/
struct lklist *list;
{
    int ct = 0;
    Lknode node;
    char **argv, **ptr;

    if (isset(XTRACE)) {
	fprintf(stderr, "%s", (prompt4) ? prompt4 : "");
	for (node = firstnode(list); node; incnode(node), ct++);
	ptr = argv = 2 + (char **)ncalloc((ct + 4) * sizeof(char *));

	for (node = firstnode(list); node; incnode(node))
	    if (*(char *)getdata(node)) {
		*ptr++ = (char *)getdata(node);
		untokenize(getdata(node));
		fputs(getdata(node), stderr);
		if (nextnode(node))
		    fputc(' ', stderr);
	    }
	*ptr = NULL;
	fputc('\n', stderr);
	fflush(stderr);
	return (argv);
    } else {
	for (node = firstnode(list); node; incnode(node), ct++);
	ptr = argv = 2 + (char **)ncalloc((ct + 4) * sizeof(char *));

	for (node = firstnode(list); node; incnode(node))
	    if (*(char *)getdata(node)) {
		*ptr++ = (char *)getdata(node);
		untokenize(getdata(node));
	    }
	*ptr = NULL;
	return (argv);
    }
}

/* untokenize the command line and remove null arguments */

void fixcline(l)		/**/
Lklist l;
{
    Lknode node, next;

    for (node = firstnode(l); node; node = next) {
	next = nextnode(node);
	if (!*(char *)getdata(node))
	    uremnode(l, node);
	else
	    untokenize(getdata(node));
    }
}

void untokenize(s)		/**/
char *s;
{
    for (; *s; s++)
	if (itok(*s))
	    if (*s == Nularg)
		chuck(s--);
	    else
		*s = ztokens[*s - Pound];
}

/* nonzero if we shouldn't clobber a file */

int dontclob(f)			/**/
struct redir *f;
{
    struct stat buf;

    if (unset(NOCLOBBER) || f->type & 1)
	return 0;
    if (stat(f->name, &buf) == -1)
	return 1;
    return S_ISREG(buf.st_mode);
}

/* close an multio (success) */

void closemn(mfds, fd)		/**/
struct multio **mfds;
int fd;
{
    if (mfds[fd]) {
	if (mfds[fd]->ct > 1)
	    if (mfds[fd]->rflag == 0)
		catproc(mfds[fd]);
	    else
		teeproc(mfds[fd]);
	mfds[fd] = NULL;
    }
}

/* close all the mnodes (failure) */

void closemnodes(mfds)		/**/
struct multio **mfds;
{
    int t0, t1;

    for (t0 = 0; t0 != 10; t0++)
	if (mfds[t0]) {
	    for (t1 = 0; t1 != mfds[t0]->ct; t1++)
		close(mfds[t0]->fds[t1]);
	    mfds[t0] = NULL;
	}
}

/* add a fd to an multio */
/* an multio is a list of fds associated with a certain fd.
	thus if you do "foo >bar >ble", the multio for fd 1 will have
	two fds, the result of open("bar",...), and the result of
	open("ble",....). */

void addfd(forked, save, mfds, fd1, fd2, rflag)	/**/
int forked;
int *save;
struct multio **mfds;
int fd1;
int fd2;
int rflag;
{
    int pipes[2];

    if (!mfds[fd1]) {		/* starting a new multio */
	mfds[fd1] = (struct multio *)alloc(sizeof(struct multio));

	if (!forked && fd1 != fd2 && fd1 < 10)
	    save[fd1] = movefd(fd1);
	redup(fd2, fd1);
	mfds[fd1]->ct = 1;
	mfds[fd1]->fds[0] = fd1;
	mfds[fd1]->rflag = rflag;
    } else {
	if (mfds[fd1]->rflag != rflag) {
	    zerr("file mode mismatch on fd %d", NULL, fd1);
	    return;
	}
	if (mfds[fd1]->ct == 1) {	/* split the stream */
	    mfds[fd1]->fds[0] = movefd(fd1);
	    mfds[fd1]->fds[1] = movefd(fd2);
	    mpipe(pipes);
	    mfds[fd1]->pipe = pipes[1 - rflag];
	    redup(pipes[rflag], fd1);
	    mfds[fd1]->ct = 2;
	} else			/* add another fd to an already split stream */
	    mfds[fd1]->fds[mfds[fd1]->ct++] = movefd(fd2);
    }
}

void addvars(l, export)		/**/
Lklist l;
int export;
{
    struct varasg *v;
    Lklist vl;

    while (full(l)) {
	char **arr, **ptr;

	v = (struct varasg *)ugetnode(l);
	singsub(&v->name);
	if (errflag)
	    return;
	untokenize(v->name);
	if (v->type == PMFLAG_s) {
	    vl = newlist();
	    addnode(vl, v->str);
	} else
	    vl = v->arr;
	prefork(vl, v->type ? 3 : 013);
	if (errflag)
	    return;
	postfork(vl, v->type ? 1 : 011);
	if (errflag)
	    return;
	if (v->type == PMFLAG_s && (empty(vl) || !nextnode(firstnode(vl)))) {
	    Param pm;
	    char *val;

	    if (empty(vl))
		val = ztrdup("");
	    else {
		untokenize(peekfirst(vl));
		val = ztrdup(ugetnode(vl));
	    }
	    pm = setsparam(v->name, ztrdup(val));
	    if (errflag)
		return;
	    if (export && !(pm->flags & PMFLAG_x))
		addenv(v->name, val);
	    zsfree(val);
	    continue;
	}
	ptr = arr = (char **)zalloc(sizeof(char **) * (countnodes(vl) + 1));

	while (full(vl)) {
	    char *pp;

	    pp = (char *)ugetnode(vl);
	    if (*pp) {
		*ptr = ztrdup(pp);
		untokenize(*ptr++);
	    }
	}
	*ptr = NULL;
	setaparam(v->name, arr);
	if (errflag)
	    return;
    }
}

void execcmd(cmd, input, output, bkg, last1)	/**/
Cmd cmd;
int input;
int output;
int bkg;
int last1;
{
    int type;
    long pid;
    int save[10], t0;
    struct redir *fn;
    struct multio *mfds[10];
    int fil, forked = 0, iscursh, nullexec = 0, assign = 0;
    char *text, dummy;
    Cmdnam cn1 = NULL;

    args = cmd->args;
    for (t0 = 0; t0 != 10; t0++) {
	save[t0] = -1;
	mfds[t0] = NULL;
    }
    if ((type = cmd->type) == SIMPLE && empty(args))
	if (full(cmd->redir))
	    if (cmd->flags & CFLAG_EXEC) {
		nullexec = 1;
	    } else if (!*nullcmd) {
		zerr("redirection with no command", NULL, 0);
		errflag = lastval = 1;
		return;
	    } else if (*readnullcmd &&
		       ((Redir) peekfirst(cmd->redir))->type == READ &&
		       !nextnode(firstnode(cmd->redir))) {
		addnode(args, dupstring(readnullcmd));
	    } else
		addnode(args, dupstring(nullcmd));
	else {
	    addvars(cmd->vars, 0);
	    if (errflag)
		lastval = 1;
	    return;
	}
    if (full(args) && *(char *)peekfirst(args) == '%') {
	insnode(args, (Lknode) args, dupstring((bkg) ? "bg" : "fg"));
	bkg = 0;
    }
    if (isset(AUTORESUME) && !bkg && empty(cmd->redir) && full(args) &&
	!input && type == SIMPLE && !nextnode(firstnode(args))) {
	if (unset(NOTIFY))
	    scanjobs();
	if (findjobnam(peekfirst(args)) != -1)
	    pushnode(args, dupstring("fg"));
    }
    if (jobbing) {		/* get the text associated with this command */
	text = getjobtext((vptr) cmd);
    } else
	text = NULL;
    if (full(args) && !(cmd->flags & CFLAG_COMMAND) && type == SIMPLE) {
	char *cmdarg = (char *)peekfirst(args);
	Cmdnam cn2 = NULL;
	int builtin = 0;

	if (!strcmp(cmdarg, "builtin") && args->first->next &&
	    (cn2 = (Cmdnam) gethnode(cmdarg, cmdnamtab)) &&
	    (cn2->flags & BUILTIN) &&
	    !(cn2->flags & EXCMD)) {
	    builtin = 1;
	    cmdarg = (char *)args->first->next->dat;
	}
	cn1 = (Cmdnam) gethnode(cmdarg, cmdnamtab);
	if (cn1 && (cn1->flags & BUILTIN) && !(cn1->flags & EXCMD))
	    assign = istypeset(cn1, NULL);
	else if (builtin)
	    assign = istypeset(NULL, cmdarg);
	if (builtin)
	    cn1 = cn2;
    }
 /* do prefork substitutions */
    prefork(args, (((type == CCASE) ? 010 : 0)
		   | (assign ? 02 : isset(MAGICEQUALSUBST))));

 /* warn about "rm *" */
    if (unset(RMSTARSILENT) && interact && isset(SHINSTDIN) &&
	type == SIMPLE && full(args) && nextnode(firstnode(args)) &&
	!strcmp(peekfirst(args), "rm") &&
	!(cmd->flags & CFLAG_NOGLOB)) {
	Lknode node, next;

	for (node = nextnode(firstnode(args)); node && !errflag; node = next) {
	    char *s = (char *)getdata(node);
	    int l = strlen(s);

	    next = nextnode(node);
	    if (s[0] == Star && !s[1]) {
		if (!checkrmall(pwd))
		    uremnode(args, node);
	    } else if (l > 2 && s[l - 2] == '/' && s[l - 1] == Star) {
		char t = s[l - 2];

		s[l - 2] = 0;
		if (!checkrmall(s))
		    uremnode(args, node);
		s[l - 2] = t;
	    }
	}
	if (!nextnode(firstnode(args)))
	    errflag = 1;
    }
    if (errflag) {
	lastval = 1;
	return;
    }
    if (!cn1) {
	char *s, *t;

	if (full(args) && ((char *)peekfirst(args))[0] == Inbrack &&
	    ((char *)peekfirst(args))[1] == '\0')
	    ((char *)peekfirst(args))[0] = '[';
	if (type == SIMPLE && full(args) && !(cmd->flags & CFLAG_COMMAND)) {
	    cn1 = (Cmdnam) gethnode(t = s = (char *)peekfirst(args), cmdnamtab);
	    if (!cn1 && isset(HASHCMDS) && strcmp(t, "..")) {
		while (*t && *t != '/')
		    t++;
		if (!*t)
		    cn1 = hashcmd(s, pathchecked);
	    }
	}
	if (type == SIMPLE && !cn1 && isset(AUTOCD) && isset(SHINSTDIN) &&
	    full(args) && empty(cmd->redir) &&
	    !nextnode(firstnode(args)) && (s = cancd(peekfirst(args)))) {
	    peekfirst(args) = (vptr) s;
	    pushnode(args, dupstring("cd"));
	    cn1 = (Cmdnam) gethnode("cd", cmdnamtab);
	}
    }
/* this is nonzero if cmd is a current shell procedure */

    iscursh = (type >= CURSH) || (type == SIMPLE && cn1 &&
				  (cn1->flags & (BUILTIN | SHFUNC)) &&
				  !(cn1->flags & EXCMD));

/* if this command is backgrounded or (this is an external
		command and we are not exec'ing it) or this is a builtin
		with output piped somewhere, then fork.  If this is the
		last stage in a subshell pipeline, don't fork, but make
		the rest of the function think we forked. */

    if (bkg || !(iscursh || (cmd->flags & CFLAG_EXEC)) ||
	(cn1 && (cn1->flags & (BUILTIN | SHFUNC)) &&
	 !(cn1->flags & EXCMD) && output)) {
	int synch[2];

	blockchld();
	pipe(synch);
	pid = (last1 && execok())? 0 : phork();
	if (pid == -1) {
	    close(synch[0]);
	    close(synch[1]);
	    return;
	}
	if (pid) {
	    close(synch[1]);
	    read(synch[0], &dummy, 1);
	    close(synch[0]);
	    if (pid == -1)
		zerr("%e", NULL, errno);
	    else {
		if (bkg)
		    lastpid = pid;
		else if (!jobtab[thisjob].stty_in_env && full(cmd->vars))
		    while (full(cmd->vars))	/* search for STTY=... */
			if (!strcmp(((Varasg) ugetnode(cmd->vars))->name,
				    "STTY")) {
			    jobtab[thisjob].stty_in_env = 1;
			    break;
			}
		addproc(pid, text);
	    }
	    return;
	}
	close(synch[0]);
	entersubsh(bkg);
	close(synch[1]);
	forked = 1;

	if (sigtrapped[SIGINT] == 2)
	    holdintr();
    } else if ((cmd->flags & CFLAG_EXEC) && !nullexec)
	entersubsh(bkg);

    if (bkg && isset(BGNICE))
	nice(5);

/* perform postfork substitutions */
    postfork(args, !(cmd->flags & CFLAG_NOGLOB));
    if (errflag) {
	lastval = 1;
	goto err;
    } else {
	char *s;

	while (full(args) && (s = (char *)peekfirst(args)) && !*s)
	    ugetnode(args);
    }

    if (input)			/* add pipeline input/output to mnodes */
	addfd(forked, save, mfds, 0, input, 0);
    if (output)
	addfd(forked, save, mfds, 1, output, 1);
    spawnpipes(cmd->redir);	/* do process substitutions */
    while (full(cmd->redir))
	if ((fn = (struct redir *)ugetnode(cmd->redir))->type == INPIPE) {
	    if (fn->fd2 == -1) {
		fixfds(save);
		execerr();
	    }
	    addfd(forked, save, mfds, fn->fd1, fn->fd2, 0);
	} else if (fn->type == OUTPIPE) {
	    if (fn->fd2 == -1) {
		fixfds(save);
		execerr();
	    }
	    addfd(forked, save, mfds, fn->fd1, fn->fd2, 1);
	} else {
	    if (!(fn->type == HERESTR || fn->type == CLOSE || fn->type ==
		  MERGE || fn->type == MERGEOUT))
		if (xpandredir(fn, cmd->redir))
		    continue;
	    if (errflag) {
		fixfds(save);
		execerr();
	    }
	    if (fn->type == HERESTR) {
		fil = getherestr(fn);
		if (fil == -1) {
		    fixfds(save);
		    if (errno != EINTR)
			zerr("%e", NULL, errno);
		    execerr();
		}
		addfd(forked, save, mfds, fn->fd1, fil, 0);
	    } else if (fn->type == READ) {
		fil = open(fn->name, O_RDONLY);
		if (fil == -1) {
		    fixfds(save);
		    if (errno != EINTR)
			zerr("%e: %s", fn->name, errno);
		    execerr();
		}
		addfd(forked, save, mfds, fn->fd1, fil, 0);
	    } else if (fn->type == CLOSE) {
		if (!forked && fn->fd1 < 10)
		    save[fn->fd1] = movefd(fn->fd1);
		closemn(mfds, fn->fd1);
		close(fn->fd1);
	    } else if (fn->type == MERGE || fn->type == MERGEOUT) {
		if (fn->fd2 == FD_COPROC)
		    fn->fd2 = (fn->type == MERGEOUT) ? coprocout : coprocin;
		closemn(mfds, fn->fd1);
		fil = dup(fn->fd2);
		if (fil == -1) {
		    char fdstr[4];

		    fixfds(save);
		    sprintf(fdstr, "%d", fn->fd2);
		    zerr("%s: %e", fdstr, errno);
		    execerr();
		}
		addfd(forked, save, mfds, fn->fd1, fil, fn->type == MERGEOUT);
	    } else {
		if (fn->type >= APP)
		    fil = open(fn->name,
			       (isset(NOCLOBBER) && !(fn->type & 1)) ?
			       O_WRONLY | O_APPEND : O_WRONLY | O_APPEND | O_CREAT, 0666);
		else
		    fil = open(fn->name, dontclob(fn) ?
			       O_WRONLY | O_CREAT | O_EXCL : O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (fil == -1) {
		    fixfds(save);
		    if (errno != EINTR)
			zerr("%e: %s", fn->name, errno);
		    execerr();
		}
		addfd(forked, save, mfds, fn->fd1, fil, 1);
	    }
	}

/* we are done with redirection.  close the mnodes, spawning
		tee/cat processes as necessary. */
    for (t0 = 0; t0 != 10; t0++)
	closemn(mfds, t0);

    if (nullexec) {
	for (t0 = 0; t0 != 10; t0++)
	    if (save[t0] != -1)
		close(save[t0]);
	return;
    }
    if (unset(NOEXEC))
	if (type >= CURSH) {
	    static int (*func[]) DCLPROTO((Cmd)) =
	    {
		execcursh, exectime, execfuncdef, execfor, execwhile,
		execrepeat, execif, execcase, execselect, execcond};

	    fixcline(args);
	    lastval = (func[type - CURSH]) (cmd);
	} else if (iscursh) {	/* builtin or shell function */
	    if (!cn1) {
		lastval = 1;
		return;
	    }
	    if (cmd->vars) {
		addvars(cmd->vars, 0);
		if (errflag) {
		    lastval = 1;
		    return;
		}
	    }
	    fixcline(args);
	    if (cn1->flags & SHFUNC)
		execshfunc(cmd, cn1);
	    else {
		if (forked)
		    closem();
		lastval = execbin(args, cn1);
		if (isset(PRINTEXITVALUE) && isset(SHINSTDIN) &&
		    lastval && !subsh) {
		    fprintf(stderr, "zsh: exit %ld\n", (long)lastval);
		}
		fflush(stdout);
		if (save[1] == -1) {
		    if (ferror(stdout)) {
			zerr("write error: %e", NULL, errno);
			clearerr(stdout);
			errflag = 0;
		    }
		} else
		    clearerr(stdout);
	    }
	} else {
	    if (cmd->flags & CFLAG_EXEC)
		setiparam("SHLVL", --shlvl);
	    if (cmd->vars) {
		addvars(cmd->vars, 1);
		if (errflag) {
		    lastval = 1;
		    return;
		}
	    }
	    if (type == SIMPLE) {
		closem();
		execute(cmd->flags & CFLAG_DASH);
	    } else		/* ( ... ) */
		execlist(cmd->u.list);
	}
  err:
    if (forked)
	_exit(lastval);
    fixfds(save);
}

/* restore fds after redirecting a builtin */

void fixfds(save)		/**/
int *save;
{
    int old_errno = errno;
    int t0;

    for (t0 = 0; t0 != 10; t0++)
	if (save[t0] != -1)
	    redup(save[t0], t0);
    errno = old_errno;
}

void entersubsh(bkg)		/**/
int bkg;
{
    if (!jobbing) {
	if (bkg) {
	    sigtrapped[SIGINT] = 2;
	    sig_ignore(SIGINT);
	    sigtrapped[SIGQUIT] = 2;
	    sig_ignore(SIGQUIT);
	    if (isatty(0)) {
		close(0);
		if (open("/dev/null", O_RDWR)) {
		    zerr("can't open /dev/null: %e", NULL, errno);
		    _exit(1);
		}
	    }
	}
    } else if (thisjob != -1) {
	if (!jobtab[thisjob].gleader) {
	    jobtab[thisjob].gleader = getpid();
	    setpgrp(0L, jobtab[thisjob].gleader);
	    if (!bkg)
		attachtty(jobtab[thisjob].gleader);
	} else
	    setpgrp(0L, jobtab[thisjob].gleader);
    }
    subsh = 1;
    if (SHTTY != -1) {
	close(SHTTY);
	SHTTY = -1;
    }
    if (jobbing) {
	sig_default(SIGTTOU);
	sig_default(SIGTTIN);
	sig_default(SIGTSTP);
	sig_default(SIGPIPE);
    }
    if (interact) {
	sig_default(SIGTERM);
	if (sigtrapped[SIGINT] != 2)
	    sig_default(SIGINT);
    }
    if (sigtrapped[SIGQUIT] != 2)
	sig_default(SIGQUIT);
    opts[MONITOR] = OPT_UNSET;
    opts[USEZLE] = OPT_UNSET;
    clearjobtab();
}

/* close all internal shell fds */

void closem()
{				/**/
    int t0;

    for (t0 = 10; t0 != NOFILE; t0++)
	close(t0);
}

/* convert here document into a here string */

char *gethere(str, typ)		/**/
char *str;
int typ;
{
    char pbuf[256];
    int qt = 0, siz = 0, l, strip = 0;
    char *s, *t, *bptr;

    for (s = str; *s; s++)
	if (INULL(*s)) {
	    *s = Nularg;
	    qt = 1;
	}
    untokenize(str);
    if (typ == HEREDOCDASH) {
	strip = 1;
	while (*str == '\t')
	    str++;
    }
    t = ztrdup("");
    for (;;) {
	char *u, *v;

	if (!hgets(pbuf, sizeof(pbuf)))
	    break;
	bptr = pbuf;
	if (strip)
	    while (*bptr == '\t')
		bptr++;
	for (u = bptr, v = str; *u != '\n' && *v; u++, v++)
	    if (*u != *v)
		break;
	if (!(*u == '\n' && !*v)) {
	    l = strlen(bptr);
	    if (!qt && l > 1 && bptr[l - 1] == '\n' && bptr[l - 2] == '\\')
		bptr[l -= 2] = '\0';
	    t = realloc(t, siz + l + 1);
	    strncpy(t + siz, bptr, l);
	    siz += l;
	} else
	    break;
    }
    t[siz] = '\0';
    if (siz && t[siz - 1] == '\n')
	t[siz - 1] = '\0';
    if (!qt)
	for (s = t; *s; s++)
	    if (*s == '$') {
		*s = Qstring;
	    } else if (*s == '`') {
		*s = Qtick;
	    } else if (*s == '(') {
		*s = Inpar;
	    } else if (*s == ')') {
		*s = Outpar;
	    } else if (*s == '\\' &&
		       (s[1] == '$' || s[1] == '`'))
		chuck(s);
    s = dupstring(t);
    zsfree(t);
    return s;
}

/* open here string fd */

int getherestr(fn)		/**/
struct redir *fn;
{
    Lklist fake;
    char *s = gettemp(), *t;
    int fd;

    fake = newlist();
    addnode(fake, fn->name);
    prefork(fake, 010);
    if (!errflag)
	postfork(fake, 011);
    if (errflag)
	return -1;
    if ((fd = open(s, O_CREAT | O_WRONLY, 0600)) == -1)
	return -1;
    while ((t = (char *)ugetnode(fake))) {
	untokenize(t);
	write(fd, t, strlen(t));
	if (full(fake))
	    write(fd, " ", 1);
    }
    write(fd, "\n", 1);
    close(fd);
    fd = open(s, O_RDONLY);
    unlink(s);
    return fd;
}

void catproc(mn)		/**/
struct multio *mn;
{
    int len, t0;
    char *buf;

    if (phork()) {
	for (t0 = 0; t0 != mn->ct; t0++)
	    close(mn->fds[t0]);
	close(mn->pipe);
	return;
    }
    closeallelse(mn);
    buf = (char *)zalloc(4092);
    for (t0 = 0; t0 != mn->ct; t0++)
	while ((len = read(mn->fds[t0], buf, 4092)))
	    write(mn->pipe, buf, len);
    _exit(0);
}

void teeproc(mn)		/**/
struct multio *mn;
{
    int len, t0;
    char *buf;

    if (phork()) {
	for (t0 = 0; t0 != mn->ct; t0++)
	    close(mn->fds[t0]);
	close(mn->pipe);
	return;
    }
    buf = (char *)zalloc(4092);
    closeallelse(mn);
    while ((len = read(mn->pipe, buf, 4092)) > 0)
	for (t0 = 0; t0 != mn->ct; t0++)
	    write(mn->fds[t0], buf, len);
    _exit(0);
}

void closeallelse(mn)		/**/
struct multio *mn;
{
    int t0, t1;

    for (t0 = 0; t0 != NOFILE; t0++)
	if (mn->pipe != t0) {
	    for (t1 = 0; t1 != mn->ct; t1++)
		if (mn->fds[t1] == t0)
		    break;
	    if (t1 == mn->ct)
		close(t0);
	}
}

long int zstrtol(s, t, base)	/**/
char *s;
char **t;
int base;
{
    int ret = 0;

    if (base <= 10)
	for (; *s >= '0' && *s < ('0' + base); s++)
	    ret = ret * base + *s - '0';
    else
	for (; idigit(*s) || (*s >= 'a' && *s < ('a' + base - 10))
	     || (*s >= 'A' && *s < ('A' + base - 10)); s++)
	    ret = ret * base + (idigit(*s) ? (*s - '0') : (*s & 0x1f) + 9);
    if (t)
	*t = (char *)s;
    return ret;
}

/* $(...) */

Lklist getoutput(cmd, qt)	/**/
char *cmd;
int qt;
{
    List list;
    int pipes[2];
    int pid;

    if (*cmd == '<') {
	int stream;
	char *fi, *s, x;

	for (cmd++; *cmd == ' '; cmd++);
	for (s = cmd; *s && *s != ' '; s++)
	    if (*s == '\\')
		s++;
	    else if (*s == '$')
		*s = String;
	x = *s;
	*s = '\0';
	fi = dupstring(cmd);
	*s = x;
	if (*fi == '~')
	    *fi = Tilde;
	else if (*fi == '=')
	    *fi = Equals;
	singsub(&fi);
	if (errflag)
	    return NULL;
	stream = open(fi, O_RDONLY);
	if (stream == -1) {
	    zerr("%e: %s", fi, errno);
	    return NULL;
	}
	return readoutput(stream, qt);
    }
    if (!(list = parselstring(cmd)))
	return NULL;
    mpipe(pipes);
    blockchld();
    if ((cmdoutpid = pid = phork()) > 0) {
	Lklist retval;

	popheap();
	close(pipes[1]);
	retval = readoutput(pipes[0], qt);
	chldsuspend(0);		/* unblocks */
	lastval = cmdoutval;
	cmdoutval = 0;
	return retval;
    } else if (pid == -1) {
	popheap();
	close(pipes[0]);
	close(pipes[1]);
	errflag = 1;
	cmdoutpid = 0;
	unblockchld();
	return NULL;
    }
    unblockchld();
    subsh = 1;
    close(pipes[0]);
    redup(pipes[1], 1);
    entersubsh(0);
    sig_ignore(SIGTSTP);
    exiting = 1;
    execlist(list);
    close(1);
    _exit(lastval);
    zerr("exit returned in child!!", NULL, 0);
    kill(getpid(), SIGKILL);
    return NULL;		/* redundant but shuts up Convex cc */
}

/* read output of command substitution */

Lklist readoutput(in, qt)	/**/
int in;
int qt;
{
    Lklist ret;
    char *buf, *ptr;
    int bsiz, c, cnt = 0;
    FILE *fin;

    fin = fdopen(in, "r");
    ret = newlist();
    ptr = buf = (char *)ncalloc(bsiz = 64);
    if (qt) {
	*ptr++ = Nularg;
	cnt++;
    }
    while ((c = fgetc(fin)) != EOF)
	if (!qt && isep(c)) {
	    if (cnt) {
		*ptr = '\0';
		if (isset(GLOBSUBST))
		    tokenize(buf);
		addnode(ret, buf);
		ptr = buf = (char *)ncalloc(bsiz = 64);
		cnt = 0;
	    }
	} else {
	    *ptr++ = c;
	    if (++cnt == bsiz) {
		char *pp = (char *)ncalloc(bsiz *= 2);

		memcpy(pp, buf, cnt);
		ptr = (buf = pp) + cnt;
	    }
	}
    if (ptr != buf && ptr[-1] == '\n')
	ptr[-1] = '\0';
    else
	*ptr = '\0';
    if (!qt && isset(GLOBSUBST))
	tokenize(buf);
    if (cnt)
	addnode(ret, buf);
    fclose(fin);
    return ret;
}

/* =(...) */

char *getoutputfile(cmd)	/**/
char *cmd;
{
    int pid;
    char *nam = gettemp(), *str;
    List list;

    if (thisjob == -1)
	return NULL;
    for (str = cmd; *str && *str != Outpar; str++);
    if (!*str)
	zerr("oops.", NULL, 0);
    *str = '\0';
    if (!(list = parselstring(cmd)))
	return NULL;
    permalloc();
    if (!jobtab[thisjob].filelist)
	jobtab[thisjob].filelist = newlist();
    addnode(jobtab[thisjob].filelist, ztrdup(nam));
    heapalloc();
    blockchld();
    if ((pid = cmdoutpid = phork())) {
	popheap();
	if (pid < 0)
	    unblockchld();
	else {
	    int os = jobtab[thisjob].stat;

	    waitforpid(pid);
	    cmdoutval = 0;
	    jobtab[thisjob].stat = os;
	}
	return nam;
    }
    subsh = 1;
    close(1);
    entersubsh(0);
    (void)creat(nam, 0666);
    exiting = 1;
    execlist(list);
    close(1);
    _exit(lastval);
    zerr("exit returned in child!!", NULL, 0);
    kill(getpid(), SIGKILL);
    return NULL;		/* redundant but shuts up Convex cc */
}

/* get a temporary named pipe */

char *namedpipe()
{				/**/
#ifdef HAS_FIFOS
    char *tnam = gettemp();

    if (mknod(tnam, 0010666, 0) < 0)
	return NULL;
    return tnam;
#else
         return NULL;
#endif
}

/* <(...) */

char *getoutproc(cmd)		/**/
char *cmd;
{
#ifndef HAS_FIFOS
    zerr("doesn't look like your system supports FIFOs.", NULL, 0);
    return NULL;
#else
    List list;
    int fd;
    char *pnam, *str;

    if (thisjob == -1)
	return NULL;
    for (str = cmd; *str && *str != Outpar; str++);
    if (!*str)
	zerr("oops.", NULL, 0);
    *str = '\0';
    pnam = namedpipe();
    if (!pnam)
	return NULL;
    permalloc();
    if (!jobtab[thisjob].filelist)
	jobtab[thisjob].filelist = newlist();
    addnode(jobtab[thisjob].filelist, ztrdup(pnam));
    heapalloc();
    if (!(list = parselstring(cmd)))
	return NULL;
    if (phork()) {
	popheap();
	return pnam;
    }
    entersubsh(1);
    closem();
    fd = open(pnam, O_WRONLY);
    if (fd == -1) {
	zerr("can't open %s: %e", pnam, errno);
	_exit(1);
    }
    redup(fd, 1);
    fd = open("/dev/null", O_RDONLY);
    redup(fd, 0);
    exiting = 1;
    execlist(list);
    close(1);
    _exit(lastval);
    return NULL;
#endif
}

/* >(...) */

char *getinproc(cmd)		/**/
char *cmd;
{
#ifndef HAS_FIFOS
    zerr("doesn't look like your system supports FIFOs.", NULL, 0);
    return NULL;
#else
    List list;
    int pid, fd;
    char *pnam, *str;

    if (thisjob == -1)
	return NULL;
    for (str = cmd; *str && *str != Outpar; str++);
    if (!*str)
	zerr("oops.", NULL, 0);
    *str = '\0';
    pnam = namedpipe();
    if (!pnam)
	return NULL;
    permalloc();
    if (!jobtab[thisjob].filelist)
	jobtab[thisjob].filelist = newlist();
    addnode(jobtab[thisjob].filelist, ztrdup(pnam));
    heapalloc();
    if (!(list = parselstring(cmd)))
	return NULL;
    if ((pid = phork())) {
	popheap();
	return pnam;
    }
    entersubsh(1);
    closem();
    fd = open(pnam, O_RDONLY);
    redup(fd, 0);
    exiting = 1;
    execlist(list);
    _exit(lastval);
    return NULL;
#endif
}

/* > >(...) (does not use named pipes) */

int getinpipe(cmd)		/**/
char *cmd;
{
    List list;
    int pipes[2];
    char *str;

    for (str = cmd; *str && *str != Outpar; str++);
    if (!*str)
	zerr("oops.", NULL, 0);
    *str = '\0';
    if (!(list = parselstring(cmd + 2)))
	return -1;
    mpipe(pipes);
    if (phork()) {
	popheap();
	close(pipes[1]);
	return pipes[0];
    }
    close(pipes[0]);
    closem();
    entersubsh(1);
    redup(pipes[1], 1);
    exiting = 1;
    execlist(list);
    _exit(lastval);
    return 0;
}

/* < <(...) */

int getoutpipe(cmd)		/**/
char *cmd;
{
    List list;
    int pipes[2];
    char *str;

    for (str = cmd; *str && *str != Outpar; str++);
    if (!*str)
	zerr("oops.", NULL, 0);
    *str = '\0';
    if (!(list = parselstring(cmd + 2)))
	return -1;
    strinend();
    mpipe(pipes);
    if (phork()) {
	popheap();
	close(pipes[0]);
	return pipes[1];
    }
    close(pipes[1]);
    entersubsh(1);
    redup(pipes[0], 0);
    closem();
    exiting = 1;
    execlist(list);
    _exit(lastval);
    return 0;
}

/* run a list, saving the current job num */

void runlist(l)			/**/
List l;
{
    int cj = thisjob;

    execlist(l);
    thisjob = cj;
}

char *gettemp()
{				/**/
    return mktemp(dyncat((tmpprefix ? tmpprefix : DEFTMPPREFIX), "XXXXXX"));
}

/* my getwd */

char *zgetwd()
{				/**/
    static char buf0[MAXPATHLEN];
    char buf3[MAXPATHLEN];

#ifdef apollo
    char *buf2 = buf0 + 2;	/* changed +1 to +2  RBC 17.11.91 */

#else
    char *buf2 = buf0 + 1;

#endif
    struct stat sbuf;
    struct dirent *de;
    DIR *dir;
    ino_t ino, rootino = (ino_t) ~ 0;
    dev_t dev, rootdev = (dev_t) ~ 0;

    holdintr();
    buf2[0] = '\0';
    buf0[0] = '/';
#ifdef apollo
    buf0[1] = '/';		/* added RBC 17.11.91 */
#endif
    if (stat(buf0, &sbuf) >= 0) {
	rootino = sbuf.st_ino;
	rootdev = sbuf.st_dev;
    }
    for (;;) {
	if (stat(".", &sbuf) < 0) {
	    chdir(buf0);
	    noholdintr();
	    return ztrdup(".");
	}
	ino = sbuf.st_ino;
	dev = sbuf.st_dev;
	if (stat("..", &sbuf) < 0) {
	    chdir(buf0);
	    noholdintr();
	    return ztrdup(".");
	}
	if ((sbuf.st_ino == ino && sbuf.st_dev == dev) ||
	    (ino == rootino && dev == rootdev)) {
	    chdir(buf0);
	    noholdintr();
	    return ztrdup(buf0);
	}
	dir = opendir("..");
	if (!dir) {
	    chdir(buf0);
	    noholdintr();
	    return ztrdup(".");
	}
	chdir("..");
	readdir(dir);
	readdir(dir);
	while ((de = readdir(dir)))
	    if (de->d_ino == ino) {
		lstat(de->d_name, &sbuf);
		if (sbuf.st_dev == dev)
		    goto match;
	    }
	closedir(dir);
	dir = opendir(".");
	readdir(dir);
	readdir(dir);
	while ((de = readdir(dir))) {
	    lstat(de->d_name, &sbuf);
	    if (sbuf.st_dev == dev)
		goto match;
	}
	noholdintr();
	closedir(dir);
	return ztrdup(".");
      match:
	strcpy(buf3, de->d_name);
	if (*buf2)
	    strcat(buf3, "/");
	strcat(buf3, buf2);
	strcpy(buf2, buf3);
	closedir(dir);
    }
}

/* open pipes with fds >= 10 */

void mpipe(pp)			/**/
int *pp;
{
    pipe(pp);
    pp[0] = movefd(pp[0]);
    pp[1] = movefd(pp[1]);
}

/* do process substitution with redirection */

void spawnpipes(l)		/**/
Lklist l;
{
    Lknode n = firstnode(l);
    Redir f;

    for (; n; incnode(n)) {
	f = (Redir) getdata(n);
	if (f->type == OUTPIPE) {
	    char *str = f->name;

	    f->fd2 = getoutpipe(str);
	}
	if (f->type == INPIPE) {
	    char *str = f->name;

	    f->fd2 = getinpipe(str);
	}
    }
}

/* perform time ... command */

int exectime(cmd)		/**/
Cmd cmd;
{
    int jb = thisjob;

    if (!cmd->u.pline) {
	shelltime();
	return 0;
    }
    execpline(cmd->u.pline, TIMED, 0);
    thisjob = jb;
    return lastval;
}

/* define a function */

int execfuncdef(cmd)		/**/
Cmd cmd;
{
    Cmdnam cc;
    char *s;

    permalloc();
    while ((s = (char *)ugetnode(cmd->args))) {
	cc = (Cmdnam) zalloc(sizeof *cc);
	cc->flags = SHFUNC;
	if (!cmd->u.list)
	    cc->u.list = NULL;
	else
	    cc->u.list = (List) dupstruct(cmd->u.list);
	addhnode(ztrdup(s), cc, cmdnamtab, freecmdnam);
	if (!strncmp(s, "TRAP", 4)) {
	    int t0 = getsignum(s + 4);

	    if (t0 != -1) {
		settrap(t0, cmd->u.list);
		permalloc();
	    }
	}
    }
    heapalloc();
    return 0;
}

/* evaluate a [[ ... ]] */

int execcond(cmd)		/**/
Cmd cmd;
{
    return !evalcond(cmd->u.cond);
}

void execshfunc(cmd, cn)	/**/
Cmd cmd;
Cmdnam cn;
{
    List l;

    if (errflag)
	return;
    if (!cn->u.list) {
	char *nam;

	if (!(cn->flags & PMFLAG_u))
	    return;
	if (!(l = getfpfunc(nam = (char *)peekfirst(cmd->args)))) {
	    zerr("function not found: %s", nam, 0);
	    lastval = 1;
	    return;
	}
	cn->flags &= ~PMFLAG_u;
	permalloc();
	cn->u.list = (List) dupstruct(l);
	heapalloc();
	popheap();
    }
    doshfunc(cn->u.list, cmd->args, cn->flags);
}

void doshfuncnoval(list, doshargs, flags)	/**/
List list;
Lklist doshargs;
int flags;
{
    int val = lastval;

    doshfunc(list, doshargs, flags);
    lastval = val;
}

void doshfunc(list, doshargs, flags)	/**/
List list;
Lklist doshargs;
int flags;
{
    char **tab, **x, *oargv0;
    int oxtr = opts[XTRACE], opev = opts[PRINTEXITVALUE], xexittr;
    int oldzoptind;
    Lklist olist;
    char *s;
    List xexitfn;

    pushheap();
    xexittr = sigtrapped[SIGEXIT];
    xexitfn = sigfuncs[SIGEXIT];
    tab = pparams;
    oargv0 = argzero;
    oldzoptind = zoptind;
    zoptind = 1;
    if (flags & PMFLAG_t)
	opts[XTRACE] = OPT_SET;
    opts[PRINTEXITVALUE] = OPT_UNSET;
    if (doshargs) {
	pparams = x =
	    (char **)zcalloc(((sizeof *x) * (1 + countnodes(doshargs))));
	argzero = ztrdup(ugetnode(doshargs));
	while ((*x = (char *)ugetnode(doshargs)))
	    *x = ztrdup(*x), x++;
    } else {
	pparams = (char **)zcalloc(sizeof *pparams);
	argzero = ztrdup(argzero);
    }
    permalloc();
    olist = locallist;
    locallist = newlist();
    locallevel++;
    heapalloc();
    runlist(dupstruct(list));
    locallevel--;
    while ((s = (char *)getnode(locallist))) {
	Param pm = (Param) gethnode(s, paramtab);

	if (pm && pm->level > locallevel)
	    unsetparam(s);
	zsfree(s);
    }
    zfree(locallist, sizeof(struct lklist));

    locallist = olist;
    breaks = retflag = 0;
    freearray(pparams);
    zsfree(argzero);
    zoptind = oldzoptind;
    argzero = oargv0;
    pparams = tab;
    if (sigfuncs[SIGEXIT] && sigfuncs[SIGEXIT] != xexitfn) {
	dotrap(SIGEXIT);
	freestruct(sigfuncs[SIGEXIT]);
    }
    sigtrapped[SIGEXIT] = xexittr;
    sigfuncs[SIGEXIT] = xexitfn;
    opts[XTRACE] = oxtr;
    opts[PRINTEXITVALUE] = opev;
    popheap();
}

/* search fpath for an undefined function */

List getfpfunc(s)		/**/
char *s;
{
    char **pp = fpath, buf[MAXPATHLEN];
    int fd;

    for (; *pp; pp++) {
	sprintf(buf, "%s/%s", *pp, s);
	if (!access(buf, R_OK) && (fd = open(buf, O_RDONLY)) != -1) {
	    int len = lseek(fd, 0, 2);

	    if (len == -1)
		close(fd);
	    else {
		char *d;
		List r;

		lseek(fd, 0, 0);
		d = (char *)zcalloc(len + 1);
		if (read(fd, d, len) != len) {
		    zfree(d, len + 1);
		    close(fd);
		} else {
		    close(fd);
		    r = parselstring(d);
		    zfree(d, len + 1);

		    return r;
		}
	    }
	}
    }
    return NULL;
}

/* check to see if AUTOCD applies here */

extern int doprintdir;

char *cancd(s)			/**/
char *s;
{
    int nocdpath = s[0] == '.' &&
    (s[1] == '/' || !s[1] || (s[1] == '.' && (s[2] == '/' || !s[1])));
    char *t;

    if (*s != '/') {
	char sbuf[MAXPATHLEN], **cp;

	if (cancd2(s))
	    return s;
	if (access(s, X_OK) == 0)
	    return NULL;
	if (!nocdpath)
	    for (cp = cdpath; *cp; cp++) {
		sprintf(sbuf, "%s/%s", *cp, s);
		if (cancd2(sbuf)) {
		    doprintdir = -1;
		    return dupstring(sbuf);
		}
	    }
	if ((t = cd_able_vars(s))) {
	    if (cancd2(t)) {
		doprintdir = -1;
		return t;
	    }
	}
	return NULL;
    }
    return cancd2(s) ? s : NULL;
}

int cancd2(s)			/**/
char *s;
{
    struct stat buf;

    return !(access(s, X_OK) || stat(s, &buf) || !S_ISDIR(buf.st_mode));
}
