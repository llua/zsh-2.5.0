/*
 *
 * utils.c - miscellaneous utilities
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
#include <pwd.h>
#include <errno.h>
#include <fcntl.h>

/* source a file */

int source(s)			/**/
char *s;
{
    int fd, cj = thisjob;
    int oldlineno = lineno, oldshst;
    FILE *obshin = bshin;
    int osubsh = subsh;

    fd = SHIN;
    lineno = 0;
    oldshst = opts[SHINSTDIN];
    opts[SHINSTDIN] = OPT_UNSET;
    if ((SHIN = movefd(open(s, O_RDONLY))) == -1) {
	SHIN = fd;
	thisjob = cj;
	opts[SHINSTDIN] = oldshst;
	return 1;
    }
    bshin = fdopen(SHIN, "r");
    subsh = 0;
    sourcelevel++;
    loop(0);
    sourcelevel--;
    fclose(bshin);
    bshin = obshin;
    subsh = osubsh;
    opts[SHINSTDIN] = oldshst;
    SHIN = fd;
    thisjob = cj;
    errflag = 0;
    retflag = 0;
    lineno = oldlineno;
    return 0;
}

/* try to source a file in the home directory */

void sourcehome(s)		/**/
char *s;
{
    char buf[MAXPATHLEN];
    char *h;

    if (!(h = getsparam("ZDOTDIR")))
	h = home;
    sprintf(buf, "%s/%s", h, s);
    (void)source(buf);
}

/* print an error */

void zwarnnam(cmd, fmt, str, num)	/**/
char *cmd;
char *fmt;
char *str;
int num;
{
    int waserr = errflag;

    zerrnam(cmd, fmt, str, num);
    errflag = waserr;
}

void zerrnam(cmd, fmt, str, num)/**/
char *cmd;
char *fmt;
char *str;
int num;
{
    if (cmd) {
	if (errflag || noerrs)
	    return;
	errflag = 1;
	trashzle();
	if (isset(SHINSTDIN))
	    fprintf(stderr, "%s: ", cmd);
	else
	    fprintf(stderr, "%s: %s: ", argzero, cmd);
    }
    while (*fmt)
	if (*fmt == '%') {
	    fmt++;
	    switch (*fmt++) {
	    case 's':
		while (*str)
		    niceputc(*str++, stderr);
		break;
	    case 'l':
		while (num--)
		    niceputc(*str++, stderr);
		break;
	    case 'd':
		fprintf(stderr, "%d", num);
		break;
	    case '%':
		putc('%', stderr);
		break;
	    case 'c':
		niceputc(num, stderr);
		break;
	    case 'e':
		if (num == EINTR) {
		    fputs("interrupt\n", stderr);
		    errflag = 1;
		    return;
		}
		if (num == EIO)
		    fputs(sys_errlist[num], stderr);
		else {
		    fputc(tulower(sys_errlist[num][0]), stderr);
		    fputs(sys_errlist[num] + 1, stderr);
		}
		break;
	    }
	} else
	    putc(*fmt++, stderr);
    if (unset(SHINSTDIN) && lineno)
	fprintf(stderr, " [%ld]\n", lineno);
    else
	putc('\n', stderr);
    fflush(stderr);
}

void zerr(fmt, str, num)	/**/
char *fmt;
char *str;
int num;
{
    if (errflag || noerrs)
	return;
    errflag = 1;
    trashzle();
    fprintf(stderr, "%s: ", (isset(SHINSTDIN)) ? "zsh" : argzero);
    zerrnam(NULL, fmt, str, num);
}

void niceputc(c, f)		/**/
int c;
FILE *f;
{
    if (itok(c)) {
	if (c >= Pound && c <= Comma)
	    putc(ztokens[c - Pound], f);
	return;
    }
    c &= 0xff;
    if (isprint(c))
	putc(c, f);
    else if (c == '\n') {
	putc('\\', f);
	putc('n', f);
    } else {
	putc('^', f);
	putc(c | '@', f);
    }
}

void sig_handle(sig)		/**/
int sig;
{
#ifdef POSIX

    struct sigaction act;

    act.sa_handler = (SIGVEC_HANDTYPE) handler;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, sig);
#ifdef SA_INTERRUPT
    if (interact)
	act.sa_flags = SA_INTERRUPT;
    else
#endif
	act.sa_flags = 0;
    sigaction(sig, &act, (struct sigaction *)NULL);

#else
#ifdef SV_INTERRUPT

    struct sigvec vec;

    vec.sv_handler = (SIGVEC_HANDTYPE) handler;
    vec.sv_mask = sigmask(sig);
    vec.sv_flags = SV_INTERRUPT;
    sigvec(sig, &vec, (struct sigvec *)NULL);

#else

    signal(sig, handler);

#endif
#endif
}

void sig_ignore(sig)		/**/
int sig;
{
    signal(sig, SIG_IGN);
}

void sig_default(sig)		/**/
int sig;
{
    signal(sig, SIG_DFL);
}

/* enable ^C interrupts */

void intr()
{				/**/
    if (interact)
	sig_handle(SIGINT);
}

/* disable ^C interrupts */

void nointr()
{				/**/
    if (interact)
	sig_ignore(SIGINT);
}

/* temporary hold ^C interrupts */

void holdintr()
{				/**/
    if (interact) {
#ifdef SIGNAL_MASKS
	sig_block(sig_mask(SIGINT));
#else
	sig_ignore(SIGINT);
#endif
    }
}

/* release ^C interrupts */

void noholdintr()
{				/**/
    if (interact) {
#ifdef SIGNAL_MASKS
	sig_unblock(sig_mask(SIGINT));
#else
	sig_handle(SIGINT);
#endif
    }
}

/* block or unblock a signal */

sigset_t sig_mask(sig)		/**/
int sig;
{
    sigset_t set;

    sigemptyset(&set);
    if (sig)
	sigaddset(&set, sig);
    return set;
}

sigset_t sig_notmask(sig)	/**/
int sig;
{
    sigset_t set;

    sigfillset(&set);
    if (sig)
	sigdelset(&set, sig);
    return set;
}

#if !defined(POSIX) && !defined(SIGNAL_MASKS)
static sigset_t heldset;

#endif

sigset_t sig_block(set)		/**/
sigset_t set;
{
    sigset_t oset;

#ifdef POSIX
    sigprocmask(SIG_BLOCK, &set, &oset);
#else
#ifdef SIGNAL_MASKS
    oset = sigblock(set);
#else
    int i;

    oset = heldset;
    for (i = 1; i <= NSIG; ++i) {
	if (sigismember(&set, i) && !sigismember(&heldset, i)) {
	    sigaddset(&heldset, i);
	    sighold(i);
	}
    }
#endif
#endif

    return oset;
}

sigset_t sig_unblock(set)	/**/
sigset_t set;
{
    sigset_t oset;

#ifdef POSIX
    sigprocmask(SIG_UNBLOCK, &set, &oset);
#else
#ifdef SIGNAL_MASKS
    sigfillset(&oset);
    oset = sigsetmask(oset);
    sigsetmask(oset & ~set);
#else
    int i;

    oset = heldset;
    for (i = 1; i <= NSIG; ++i) {
	if (sigismember(&set, i) && sigismember(&heldset, i)) {
	    sigdelset(&heldset, i);
	    sigrelse(i);
	}
    }
#endif
#endif

    return oset;
}

sigset_t sig_setmask(set)	/**/
sigset_t set;
{
    sigset_t oset;

#ifdef POSIX
    sigprocmask(SIG_SETMASK, &set, &oset);
#else
#ifdef SIGNAL_MASKS
    oset = sigsetmask(set);
#else
    int i;

    oset = heldset;
    for (i = 1; i <= NSIG; ++i) {
	if (sigismember(&set, i) && !sigismember(&heldset, i)) {
	    sigaddset(&heldset, i);
	    sighold(i);
	} else if (!sigismember(&set, i) && sigismember(&heldset, i)) {
	    sigdelset(&heldset, i);
	    sigrelse(i);
	}
    }
#endif
#endif

    return oset;
}

int sig_suspend(sig, sig2)		/**/
int sig;
int sig2;
{
    int ret;

#ifdef POSIX
    sigset_t set;

    if (sig) {
	set = sig_notmask(sig);
	sigdelset(&set, SIGHUP);
	sigdelset(&set, sig2);
    } else
	sigemptyset(&set);
    ret = sigsuspend(&set);
#else
#ifdef HAS_SIGRELSE
    /* Use System V version of sigpause.  */
    ret = sigpause(sig);
#else
    /* Use BSD version of sigpause.  */
    sigset_t set = sig_notmask(sig);
    if (sig2)
      sigdelset(&set, sig2);
    ret = sigpause(set);
#endif
#endif

    return ret;
}

/* get a symlink-free pathname for s relative to PWD */

char *findpwd(s)		/**/
char *s;
{
    char *t;

    if (*s == '/')
	return xsymlink(s);
    s = tricat((pwd[1]) ? pwd : "", "/", s);
    t = xsymlink(s);
    zsfree(s);
    return t;
}

static char xbuf[MAXPATHLEN];

int ispwd(s)			/**/
char *s;
{
    struct stat sbuf, tbuf;

    if (stat(s, &sbuf) == 0 && stat(".", &tbuf) == 0)
	if (sbuf.st_dev == tbuf.st_dev && sbuf.st_ino == tbuf.st_ino)
	    return 1;
    return 0;
}

/* expand symlinks in s, and remove other weird things */

char *xsymlink(s)		/**/
char *s;
{
    if (unset(CHASELINKS))
	return ztrdup(s);
    if (*s != '/')
	return NULL;
    strcpy(xbuf, "");
    if (xsymlinks(s + 1, 1))
	return ztrdup(s);
    if (!*xbuf)
	return ztrdup("/");
    return ztrdup(xbuf);
}

char **slashsplit(s)		/**/
char *s;
{
    char *t, **r, **q;
    int t0;

    if (!*s)
	return (char **)zcalloc(sizeof(char **));

    for (t = s, t0 = 0; *t; t++)
	if (*t == '/')
	    t0++;
    q = r = (char **)zalloc(sizeof(char **) * (t0 + 2));

    while ((t = strchr(s, '/'))) {
	*t = '\0';
	*q++ = ztrdup(s);
	*t = '/';
	while (*t == '/')
	    t++;
	if (!*t) {
	    *q = NULL;
	    return r;
	}
	s = t;
    }
    *q++ = ztrdup(s);
    *q = NULL;
    return r;
}

/* expands symlinks and .. or . expressions */
/* if flag = 0, only expand .. and . expressions */

int xsymlinks(s, flag)		/**/
char *s;
int flag;
{
    char **pp, **opp;
    char xbuf2[MAXPATHLEN], xbuf3[MAXPATHLEN];
    int t0;

    opp = pp = slashsplit(s);
    for (; *pp; pp++) {
	if (!strcmp(*pp, ".")) {
	    zsfree(*pp);
	    continue;
	}
	if (!strcmp(*pp, "..")) {
	    char *p;

	    zsfree(*pp);
	    if (!strcmp(xbuf, "/"))
		continue;
	    p = xbuf + strlen(xbuf);
	    while (*--p != '/');
	    *p = '\0';
	    continue;
	}
	if (unset(CHASELINKS)) {
	    strcat(xbuf, "/");
	    strcat(xbuf, *pp);
	    zsfree(*pp);
	    continue;
	}
	sprintf(xbuf2, "%s/%s", xbuf, *pp);
	t0 = readlink(xbuf2, xbuf3, MAXPATHLEN);
	if (t0 == -1 || !flag) {
	    strcat(xbuf, "/");
	    strcat(xbuf, *pp);
	    zsfree(*pp);
	} else {
	    xbuf3[t0] = '\0';	/* STUPID */
	    if (*xbuf3 == '/') {
		strcpy(xbuf, "");
		if (xsymlinks(xbuf3 + 1, flag))
		    return 1;
	    } else if (xsymlinks(xbuf3, flag))
		return 1;
	    zsfree(*pp);
	}
    }
    free(opp);
    return 0;
}

/* print a directory */

void fprintdir(s, f)		/**/
char *s;
FILE *f;
{
    int t0;

    t0 = finddir(s);
    if (t0 == -1) {
	fputs(s, f);
    } else {
	putc('~', f);
	fputs(namdirs[t0].name, f);
	fputs(s + namdirs[t0].len, f);
    }
}

void printdir(s)		/**/
char *s;
{
    fprintdir(s, stdout);
}

void printdircr(s)		/**/
char *s;
{
    fprintdir(s, stdout);
    putchar('\n');
}

int findname(s)			/**/
char *s;
{
    int t0;

    for (t0 = 0; t0 < userdirct; t0++)
	if (!strcmp(namdirs[t0].name, s))
	    return t0;
    return -1;
}

/* see if a path has a named directory as its prefix */

int finddir(s)			/**/
char *s;
{
    int t0, slen, min, max;
    static int last = -1;
    static char previous[MAXPATHLEN] = "\0";

    if (!s) {			/* Invalidate directory cache */
	*previous = '\0';
	return last = -1;
    }
    if (!strcmp(s, previous))
	return last;

/* The named directories are sorted in increasing length of the path. For the
   same path length, it is sorted in DECREASING order of name length, unless
   HIDE_NAMES is defined, in which case the last included name comes last. */

/* This binary search doesn't seem to make much difference but... */

    slen = strlen(s);
    min = 0;
    max = userdirct;
    while ((t0 = (min + max) >> 1) != min)
	if (slen < namdirs[t0].len)
	    max = t0;
	else
	    min = t0;

/* Binary search alone doesn't work because we want the longest match, not
   necessarily an exact one */

    for (t0 = min; t0 >= 0; t0--)
	if (namdirs[t0].len <= slen && !dircmp(namdirs[t0].dir, s)) {
	    strcpy(previous, s);
	    return last = t0;
	}
    return -1;
}

/* add a named directory */

void adduserdir(s, t, ishomedir, always)	/**/
char *s;
char *t;
int ishomedir;
int always;
{
    int t0 = -1, t1, t2;

    if (!interact)
	return;

    if (ishomedir) {
	if (!strcmp(t, "/") || (t0 = findname(s) != -1))
	    return;
    } else if (!t || *t != '/' || !strcmp(t, "/")) {
	if ((t0 = findname(s)) != -1) {	/* remove the name */
	    zsfree(namdirs[t0].name);
	    zsfree(namdirs[t0].dir);
	    for (; t0 < userdirct - 1; t0++)
		memcpy((vptr) & namdirs[t0], (vptr) & namdirs[t0 + 1],
		       sizeof *namdirs);
	    userdirct--;
	    finddir(0);
	}
	return;
    }
    if (unset(AUTONAMEDIRS) && findname(s) == -1 && always == 2)
	return;

    t2 = strlen(t);
    if (!ishomedir && t2 < MAXPATHLEN && (t0 = findname(s)) != -1) {

    /* update value */

	zsfree(namdirs[t0].dir);
	namdirs[t0].dir = ztrdup(t);
	t1 = namdirs[t0].len;
	namdirs[t0].len = t2;
	if (t2 < t1)
	    qsort((vptr) namdirs, t0 + 1, sizeof *namdirs,
		  (int (*)DCLPROTO((const void *, const void *)))lencmp);
	else
	    qsort((vptr) & namdirs[t0], userdirct - t0, sizeof *namdirs,
		  (int (*)DCLPROTO((const void *, const void *)))lencmp);

	finddir(0);		/* Invalidate directory cache */
	return;
    }
 /* add the name */

    if (userdirsz == userdirct) {
	userdirsz *= 2;
	namdirs = (Nameddirs) realloc((vptr) namdirs,
				      sizeof *namdirs * userdirsz);
	if (!namdirs)
	    return;
    }
    for (t0 = 0; t0 < userdirct; t0++)
	if (namdirs[t0].len > t2)
	    break;
    for (t1 = userdirct; t1 > t0; t1--)
	memcpy((vptr) & namdirs[t1], (vptr) & namdirs[t1 - 1],
	       sizeof *namdirs);
    namdirs[t0].len = t2;
    namdirs[t0].namelen = strlen(s);
    namdirs[t0].name = ztrdup(s);
    namdirs[t0].dir = ztrdup(t);
    namdirs[t0].homedir = ishomedir;
    userdirct++;
    if (t0 && namdirs[t0 - 1].len == t2)
	qsort((vptr) namdirs, t0 + 1, sizeof *namdirs,
	      (int (*)DCLPROTO((const void *, const void *)))lencmp);
    finddir(0);
}

int dircmp(s, t)		/**/
char *s;
char *t;
{
    if (s) {
	for (; *s == *t; s++, t++)
	    if (!*s)
		return 0;
	if (!*s && *t == '/')
	    return 0;
    }
    return 1;
}

int lencmp(first, sec)		/**/
vptr first;
vptr sec;
{
    int i;

#ifndef HIDE_NAMES
    if ((i = ((Nameddirs) first)->len - ((Nameddirs) sec)->len))
	return i;
    else
	return			/* for paths with the same lenght put the shortest name AFTER */
	    ((Nameddirs) sec)->namelen - ((Nameddirs) first)->namelen;
#else
    return ((Nameddirs) first)->len - ((Nameddirs) sec)->len;
#endif
}

int ddifftime(t1, t2)		/**/
time_t t1;
time_t t2;
{
    return ((long)t2 - (long)t1);
}

/* see if jobs need printing */

void scanjobs()
{				/**/
    int t0;

    for (t0 = 1; t0 != MAXJOB; t0++)
	if (jobtab[t0].stat & STAT_CHANGED)
	    printjob(jobtab + t0, 0);
}

/* do pre-prompt stuff */

void preprompt()
{				/**/
    int diff;
    List list;
    struct schedcmd *sch, *schl;

    if (unset(NOTIFY))
	scanjobs();
    if (errflag)
	return;
    if ((list = getshfunc("precmd")))
	doshfuncnoval(list, NULL, 0);
    if (errflag)
	return;
    if (period && (time(NULL) > lastperiod + period) &&
	(list = getshfunc("periodic"))) {
	doshfuncnoval(list, NULL, 0);
	lastperiod = time(NULL);
    }
    if (errflag)
	return;
    if (watch) {
	diff = (int)ddifftime(lastwatch, time(NULL));
	if (diff > logcheck) {
	    dowatch();
	    lastwatch = time(NULL);
	}
    }
    if (errflag)
	return;
    diff = (int)ddifftime(lastmailcheck, time(NULL));
    if (diff > mailcheck) {
	if (mailpath && *mailpath && **mailpath)
	    checkmailpath(mailpath);
	else if (mailfile && *mailfile) {
	    char *x[2];

	    x[0] = mailfile;
	    x[1] = NULL;
	    checkmailpath(x);
	}
	lastmailcheck = time(NULL);
    }
    for (schl = (struct schedcmd *)&schedcmds, sch = schedcmds; sch;
	 sch = (schl = sch)->next) {
	if (sch->time < time(NULL)) {
	    execstring(sch->cmd);
	    schl->next = sch->next;
	    zsfree(sch->cmd);
	    zfree(sch, sizeof(struct schedcmd));

	    sch = schl;
	}
	if (errflag)
	    return;
    }
}

int arrlen(s)			/**/
char **s;
{
    int t0;

    for (t0 = 0; *s; s++, t0++);
    return t0;
}

void checkmailpath(s)		/**/
char **s;
{
    struct stat st;
    char *v, *u, c;

    while (*s) {
	for (v = *s; *v && *v != '?'; v++);
	c = *v;
	*v = '\0';
	if (c != '?')
	    u = NULL;
	else
	    u = v + 1;
	if (**s == 0) {
	    *v = c;
	    zerr("empty MAILPATH component: %s", *s, 0);
	} else if (stat(*s, &st) == -1) {
	    if (errno != ENOENT)
		zerr("%e: %s", *s, errno);
	} else if (S_ISDIR(st.st_mode)) {
	    Lklist l;
	    DIR *lock = opendir(*s);
	    char buf[MAXPATHLEN * 2], **arr, **ap;
	    struct dirent *de;
	    int ct = 1;

	    if (lock) {
		pushheap();
		heapalloc();
		l = newlist();
		readdir(lock);
		readdir(lock);
		while ((de = readdir(lock))) {
		    if (errflag)
			break;
		    if (u)
			sprintf(buf, "%s/%s?%s", *s, de->d_name, u);
		    else
			sprintf(buf, "%s/%s", *s, de->d_name);
		    addnode(l, dupstring(buf));
		    ct++;
		}
		closedir(lock);
		ap = arr = (char **)alloc(ct * sizeof(char *));

		while ((*ap++ = (char *)ugetnode(l)));
		checkmailpath(arr);
		popheap();
	    }
	} else {
	    if (st.st_size && st.st_atime <= st.st_mtime &&
		st.st_mtime > lastmailcheck)
		if (!u) {
		    fprintf(stderr, "You have new mail.\n");
		    fflush(stderr);
		} else {
		    char *z = u;

		    while (*z)
			if (*z == '$' && z[1] == '_') {
			    fprintf(stderr, "%s", *s);
			    z += 2;
			} else
			    fputc(*z++, stderr);
		    fputc('\n', stderr);
		    fflush(stderr);
		}
	    if (isset(MAILWARNING) && st.st_atime > st.st_mtime &&
		st.st_atime > lastmailcheck && st.st_size) {
		fprintf(stderr, "The mail in %s has been read.\n", *s);
		fflush(stderr);
	    }
	}
	*v = c;
	s++;
    }
}

void saveoldfuncs(x, y)		/**/
char *x;
Cmdnam y;
{
    Cmdnam cc;

    if (y->flags & (SHFUNC | DISABLED)) {
	cc = (Cmdnam) zcalloc(sizeof *cc);
	*cc = *y;
	y->u.list = NULL;
	addhnode(ztrdup(x), cc, cmdnamtab, freecmdnam);
    }
}

/* create command hashtable */

void newcmdnamtab()
{				/**/
    Hashtab oldcnt;

    oldcnt = cmdnamtab;
    permalloc();
    cmdnamtab = newhtable(101);
    addbuiltins();
    if (oldcnt) {
	listhtable(oldcnt, (HFunc) saveoldfuncs);
	freehtab(oldcnt, freecmdnam);
    }
    lastalloc();
    pathchecked = path;
}

void freecmdnam(a)		/**/
vptr a;
{
    struct cmdnam *c = (struct cmdnam *)a;

    if (c->flags & SHFUNC) {
	if (c->u.list)
	    freestruct(c->u.list);
    } else if ((c->flags & HASHCMD) == HASHCMD)
	zsfree(c->u.cmd);

    zfree(c, sizeof(struct cmdnam));
}

void freecompcond(a)		/**/
vptr a;
{
    Compcond cc = (Compcond) a;
    Compcond and, or, c;
    int n;

    for (c = cc; c; c = or) {
	or = c->or;
	for (; c; c = and) {
	    and = c->and;
	    if (c->type == CCT_POS ||
		c->type == CCT_NUMWORDS) {
		free(c->u.r.a);
		free(c->u.r.b);
	    } else if (c->type == CCT_CURSUF ||
		       c->type == CCT_CURPRE) {
		for (n = 0; n < c->n; n++)
		    if (c->u.s.s[n])
			zsfree(c->u.s.s[n]);
		free(c->u.s.s);
	    } else if (c->type == CCT_RANGESTR ||
		       c->type == CCT_RANGEPAT) {
		for (n = 0; n < c->n; n++)
		    if (c->u.l.a[n])
			zsfree(c->u.l.a[n]);
		free(c->u.l.a);
		for (n = 0; n < c->n; n++)
		    if (c->u.l.b[n])
			zsfree(c->u.l.b[n]);
		free(c->u.l.b);
	    } else {
		for (n = 0; n < c->n; n++)
		    if (c->u.s.s[n])
			zsfree(c->u.s.s[n]);
		free(c->u.s.p);
		free(c->u.s.s);
	    }
	    zfree(c, sizeof(struct compcond));
	}
    }
}

void freecompctl(a)		/**/
vptr a;
{
    Compctl cc = (Compctl) a;

    if (cc == &cc_default ||
	cc == &cc_compos ||
	--cc->refc > 0)
	return;

    zsfree(cc->keyvar);
    zsfree(cc->glob);
    zsfree(cc->str);
    zsfree(cc->func);
    zsfree(cc->explain);
    zsfree(cc->prefix);
    zsfree(cc->suffix);
    zsfree(cc->hpat);
    zsfree(cc->subcmd);
    if (cc->cond)
	freecompcond(cc->cond);
    if (cc->ext) {
	Compctl n, m;

	n = cc->ext;
	do {
	    m = (Compctl) (n->next);
	    freecompctl(n);
	    n = m;
	}
	while (n);
    }
    if (cc->xor && cc->xor != &cc_default)
	freecompctl(cc->xor);
    zfree(cc, sizeof(struct compctl));
}

void freecompctlp(a)		/**/
vptr a;
{
    Compctlp ccp = (Compctlp) a;

    freecompctl(ccp->cc);
}

void freestr(a)			/**/
vptr a;
{
    zsfree(a);
}

void freeanode(a)		/**/
vptr a;
{
    struct alias *c = (struct alias *)a;

    zsfree(c->text);
    zfree(c, sizeof(struct alias));
}

void freepm(a)			/**/
vptr a;
{
    struct param *pm = (Param) a;

    zfree(pm, sizeof(struct param));
}

void gettyinfo(ti)		/**/
struct ttyinfo *ti;
{
    if (SHTTY != -1) {
#ifdef HAS_TERMIOS
#ifdef HAS_TCCRAP
	if (tcgetattr(SHTTY, &ti->tio) == -1)
#else
	if (ioctl(SHTTY, TCGETS, &ti->tio) == -1)
#endif
	    zerr("bad tcgets: %e", NULL, errno);
#else
#ifdef HAS_TERMIO
	ioctl(SHTTY, TCGETA, &ti->tio);
#else
	ioctl(SHTTY, TIOCGETP, &ti->sgttyb);
	ioctl(SHTTY, TIOCLGET, &ti->lmodes);
	ioctl(SHTTY, TIOCGETC, &ti->tchars);
	ioctl(SHTTY, TIOCGLTC, &ti->ltchars);
#endif
#endif
#ifdef TIOCGWINSZ
/*	if (ioctl(SHTTY, TIOCGWINSZ, &ti->winsize) == -1)
	    	zerr("bad tiocgwinsz: %e",NULL,errno);*/
	ioctl(SHTTY, TIOCGWINSZ, (char *)&ti->winsize);
#endif
    }
}

void settyinfo(ti)		/**/
struct ttyinfo *ti;
{
    if (SHTTY != -1) {
#ifdef HAS_TERMIOS
#ifdef HAS_TCCRAP
#ifndef TCSADRAIN
#define TCSADRAIN 1		/* XXX Princeton's include files are screwed up */
#endif
	tcsetattr(SHTTY, TCSADRAIN, &ti->tio);
    /* if (tcsetattr(SHTTY, TCSADRAIN, &ti->tio) == -1) */
#else
	ioctl(SHTTY, TCSETS, &ti->tio);
    /* if (ioctl(SHTTY, TCSETS, &ti->tio) == -1) */
#endif
	/*	zerr("settyinfo: %e",NULL,errno)*/ ;
#else
#ifdef HAS_TERMIO
	ioctl(SHTTY, TCSETA, &ti->tio);
#else
	ioctl(SHTTY, TIOCSETN, &ti->sgttyb);
	ioctl(SHTTY, TIOCLSET, &ti->lmodes);
	ioctl(SHTTY, TIOCSETC, &ti->tchars);
	ioctl(SHTTY, TIOCSLTC, &ti->ltchars);
#endif
#endif
    }
}

#ifdef TIOCGWINSZ
extern winchanged;

void adjustwinsize()
{				/**/
    int oldcols = columns, oldrows = lines;

    if (SHTTY == -1)
	return;

    ioctl(SHTTY, TIOCGWINSZ, (char *)&shttyinfo.winsize);
    if (shttyinfo.winsize.ws_col)
	columns = shttyinfo.winsize.ws_col;
    if (shttyinfo.winsize.ws_row)
	lines = shttyinfo.winsize.ws_row;
    if (oldcols != columns) {
	if (zleactive) {
	    resetneeded = winchanged = 1;
	    refresh();
	}
	setintenv("COLUMNS", columns);
    }
    if (oldrows != lines)
	setintenv("LINES", lines);
}
#endif

int zyztem(s, t)		/**/
char *s;
char *t;
{
    int cj = thisjob;

    s = tricat(s, " ", t);
    execstring(s);		/* Depends on recursion condom in execute() */
    zsfree(s);
    thisjob = cj;
    return lastval;
}

/* move a fd to a place >= 10 */

int movefd(fd)			/**/
int fd;
{
    int fe;

    if (fd == -1)
	return fd;
#ifdef F_DUPFD
    fe = fcntl(fd, F_DUPFD, 10);
#else
    if ((fe = dup(fd)) < 10)
	fe = movefd(fe);
#endif
    close(fd);
    return fe;
}

/* move fd x to y */

void redup(x, y)		/**/
int x;
int y;
{
    if (x != y) {
	dup2(x, y);
	close(x);
    }
}

void settrap(t0, l)		/**/
int t0;
List l;
{
    Cmd c;

    if (l) {
	c = l->left->left->left;
	if (c->type == SIMPLE && empty(c->args) && empty(c->redir)
	    && empty(c->vars) && !c->flags)
	    l = NULL;
    }
    if (t0 == -1)
	return;
    if (jobbing && (t0 == SIGTTOU || t0 == SIGTSTP || t0 == SIGTTIN
		    || t0 == SIGPIPE)) {
	zerr("can't trap SIG%s in interactive shells", sigs[t0], 0);
	return;
    }
    if (sigfuncs[t0])
	freestruct(sigfuncs[t0]);
    if (!l) {
	sigtrapped[t0] = 2;
	sigfuncs[t0] = NULL;
	if (t0 && t0 <= SIGCOUNT &&
#ifdef SIGWINCH
	    t0 != SIGWINCH &&
#endif
	    t0 != SIGCHLD)
	    sig_ignore(t0);
    } else {
	if (t0 && t0 <= SIGCOUNT &&
#ifdef SIGWINCH
	    t0 != SIGWINCH &&
#endif
	    t0 != SIGCHLD)
	    sig_handle(t0);
	sigtrapped[t0] = 1;
	permalloc();
	sigfuncs[t0] = (List) dupstruct(l);
	heapalloc();
    }
}

void unsettrap(t0)		/**/
int t0;
{
    if (t0 == -1)
	return;
    if (jobbing && (t0 == SIGTTOU || t0 == SIGTSTP || t0 == SIGTTIN
		    || t0 == SIGPIPE)) {
	return;
    }
    sigtrapped[t0] = 0;
    if (t0 == SIGINT)
	intr();
    else if (t0 == SIGHUP)
	sig_handle(t0);
    else if (t0 && t0 <= SIGCOUNT &&
#ifdef SIGWINCH
	     t0 != SIGWINCH &&
#endif
	     t0 != SIGCHLD)
	sig_default(t0);
    if (sigfuncs[t0]) {
	freestruct(sigfuncs[t0]);
	sigfuncs[t0] = NULL;
    }
}

void dotrap(sig)		/**/
int sig;
{
    int sav, savval;

    sav = sigtrapped[sig];
    savval = lastval;
    if (sav == 2)
	return;
    sigtrapped[sig] = 2;
    if (sigfuncs[sig]) {
	Lklist args;
	char *name, num[4];

	lexsave();
	permalloc();
	args = newlist();
	name = (char *)zalloc(5 + strlen(sigs[sig]));
	sprintf(name, "TRAP%s", sigs[sig]);
	addnode(args, name);
	sprintf(num, "%d", sig);
	addnode(args, num);
	trapreturn = -1;
	doshfuncnoval(sigfuncs[sig], args, 0);
	lexrestore();
	freetable(args, (FFunc) NULL);
	zsfree(name);
	if (trapreturn > 0) {
	    breaks = loops;
	    errflag = 1;
	} else
	    trapreturn = 0;
    }
    if (sigtrapped[sig])
	sigtrapped[sig] = sav;
    lastval = savval;
}

/* copy len chars from t into s, and null terminate */

void ztrncpy(s, t, len)		/**/
char *s;
char *t;
int len;
{
    while (len--)
	*s++ = *t++;
    *s = '\0';
}

/* copy t into *s and update s */

void strucpy(s, t)		/**/
char **s;
char *t;
{
    char *u = *s;

    while ((*u++ = *t++));
    *s = u - 1;
}

void struncpy(s, t, n)		/**/
char **s;
char *t;
int n;
{
    char *u = *s;

    while (n--)
	*u++ = *t++;
    *s = u;
    *u = '\0';
}

int checkrmall(s)		/**/
char *s;
{
    fflush(stdin);
    if (*s == '/')
	fprintf(stderr, "zsh: sure you want to delete all the files in %s? ", s);
    else
	fprintf(stderr, "zsh: sure you want to delete all the files in %s/%s? ",
		(pwd[1]) ? pwd : "", s);
    fflush(stderr);
    feep();
    return (getquery() == 'y');
}

int getquery()
{				/**/
    char c, d;
    int val, isem = !strcmp(term, "emacs");

    attachtty(mypgrp);
    if (!isem)
	setcbreak();
#ifdef FIONREAD
    ioctl(SHTTY, FIONREAD, (char *)&val);
    if (val) {
	if (!isem)
	    settyinfo(&shttyinfo);
	write(2, "n\n", 2);
	return 'n';
    }
#endif
    if (read(SHTTY, &c, 1) == 1)
	if (c == 'y' || c == 'Y' || c == '\t')
	    c = 'y';
    if (isem) {
	if (c != '\n')
	    while (read(SHTTY, &d, 1) == 1 && d != '\n');
    } else {
	settyinfo(&shttyinfo);
	if (c != '\n')
	    write(2, "\n", 1);
    }
    return (int)c;
}

static int d;
static char *guess, *best;

void spscan(s, junk)		/**/
char *s;
char *junk;
{
    int nd;

    nd = spdist(s, guess, (int)strlen(guess) / 4 + 1);
    if (nd <= d) {
	best = s;
	d = nd;
    }
}

/* spellcheck a word */
/* fix s and s2 ; if s2 is non-null, fix the history list too */

void spckword(s, s2, tptr, cmd, ask)	/**/
char **s;
char **s2;
char **tptr;
int cmd;
int ask;
{
    char *t, *u;
    char firstchar;
    int x;
    int pram = 0;

    if (**s == '-' || **s == '%')
	return;
    if (!strcmp(*s, "in"))
	return;
    if (!(*s)[0] || !(*s)[1])
	return;
    if (gethnode(*s, cmdnamtab) || gethnode(*s, aliastab))
	return;
    else if (isset(HASHLISTALL)) {
	fullhash();
	if (gethnode(*s, cmdnamtab))
	    return;
    }
    t = *s;
    if (*t == Tilde || *t == Equals || *t == String)
	t++;
    for (; *t; t++)
	if (itok(*t))
	    return;
    best = NULL;
    for (t = *s; *t; t++)
	if (*t == '/')
	    break;
    if (**s == String) {
	if (*t)
	    return;
	pram = 1;
	guess = *s + 1;
	while (*guess == '+' || *guess == '^' ||
	       *guess == '#' || *guess == '~' ||
	       *guess == '=')
	    guess++;
	d = 100;
	listhtable(paramtab, spscan);
    } else {
	if ((u = spname(guess = *s)) != *s)
	    best = u;
	if (!*t && !cmd) {
	    if (access(*s, F_OK) == 0)
		return;
	    if (hashcmd(*s, pathchecked))
		return;
	    guess = *s;
	    d = 100;
	    listhtable(aliastab, spscan);
	    listhtable(cmdnamtab, spscan);
	}
    }
    if (errflag)
	return;
    if (best && (int)strlen(best) > 1 && strcmp(best, guess)) {
	if (ask) {
	    char *pp;
	    int junk;

	    rstring = best;
	    Rstring = guess;
	    firstchar = *guess;
	    if (*guess == Tilde)
		*guess = '~';
	    else if (*guess == String)
		*guess = '$';
	    else if (*guess == Equals)
		*guess = '=';
	    pp = putprompt(sprompt, &junk, 1);
	    *guess = firstchar;
	    fprintf(stderr, "%s", pp);
	    fflush(stderr);
	    feep();
	    x = getquery();
	} else
	    x = 'y';
	if (x == 'y' || x == ' ') {
	    if (!pram) {
		*s = dupstring(best);
	    } else {
		*s = (char *)alloc(strlen(best) + 2);
		strcpy(*s + 1, best);
		**s = String;
	    }
	    if (s2) {
		if (*tptr && !strcmp(hlastw, *s2) && hlastw < hptr) {
		    char *z;

		    hptr = hlastw;
		    if (pram)
			hwaddc('$');
		    for (z = best; *z; z++)
			hwaddc(*z);
		    hwaddc(HISTSPACE);
		    *tptr = hptr - 1;
		    **tptr = '\0';
		}
		*s2 = dupstring(best);
	    }
	} else if (x == 'a') {
	    histdone |= HISTFLAG_NOEXEC;
	} else if (x == 'e') {
	    histdone |= HISTFLAG_NOEXEC | HISTFLAG_RECALL;
	}
    }
}

int ztrftime(buf, bufsize, fmt, tm)	/**/
char *buf;
int bufsize;
char *fmt;
struct tm *tm;
{
    static char *astr[] =
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static char *estr[] =
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
     "Aug", "Sep", "Oct", "Nov", "Dec"};
    static char *lstr[] =
    {"12", " 1", " 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9",
     "10", "11"};
    char tmp[3];

#ifdef HAS_STRFTIME
    char *origbuf = buf;

#endif

    tmp[0] = '%';
    tmp[2] = '\0';
    while (*fmt)
	if (*fmt == '%') {
	    fmt++;
	    switch (*fmt++) {
	    case 'a':
		strucpy(&buf, astr[tm->tm_wday]);
		break;
	    case 'b':
		strucpy(&buf, estr[tm->tm_mon]);
		break;
	    case 'd':
		*buf++ = '0' + tm->tm_mday / 10;
		*buf++ = '0' + tm->tm_mday % 10;
		break;
	    case 'e':
		if (tm->tm_mday > 9)
		    *buf++ = '0' + tm->tm_mday / 10;
		*buf++ = '0' + tm->tm_mday % 10;
		break;
	    case 'k':
		if (tm->tm_hour > 9)
		    *buf++ = '0' + tm->tm_hour / 10;
		*buf++ = '0' + tm->tm_hour % 10;
		break;
	    case 'l':
		strucpy(&buf, lstr[tm->tm_hour % 12]);
		break;
	    case 'm':
		*buf++ = '0' + (tm->tm_mon + 1) / 10;
		*buf++ = '0' + (tm->tm_mon + 1) % 10;
		break;
	    case 'M':
		*buf++ = '0' + tm->tm_min / 10;
		*buf++ = '0' + tm->tm_min % 10;
		break;
	    case 'p':
		*buf++ = (tm->tm_hour > 11) ? 'p' : 'a';
		*buf++ = 'm';
		break;
	    case 'S':
		*buf++ = '0' + tm->tm_sec / 10;
		*buf++ = '0' + tm->tm_sec % 10;
		break;
	    case 'y':
		*buf++ = '0' + tm->tm_year / 10;
		*buf++ = '0' + tm->tm_year % 10;
		break;
	    default:
#ifdef HAS_STRFTIME
		*buf = '\0';
		tmp[1] = fmt[-1];
		strftime(buf, bufsize - strlen(origbuf), tmp, tm);
		buf += strlen(buf);
#else
		*buf++ = '%';
		*buf++ = fmt[-1];
#endif
		break;
	    }
	} else
	    *buf++ = *fmt++;
    *buf = '\0';
    return 0;
}

char *join(arr, delim)		/**/
char **arr;
int delim;
{
    int len = 0;
    char **s, *ret, *ptr;
    static char *lastmem = NULL;

    for (s = arr; *s; s++)
	len += strlen(*s) + 1;
    if (!len)
	return "";
    zsfree(lastmem);
    lastmem = ptr = ret = (char *)zalloc(len);
    for (s = arr; *s; s++) {
	strucpy(&ptr, *s);
	*ptr++ = delim;
    }
    ptr[-1] = '\0';
    return ret;
}

char *spacejoin(s)		/**/
char **s;
{
    return join(s, *ifs);
}

char **colonsplit(s)		/**/
char *s;
{
    int ct;
    char *t, **ret, **ptr;

    for (t = s, ct = 0; *t; t++)
	if (*t == ':')
	    ct++;
    ptr = ret = (char **)zalloc(sizeof(char **) * (ct + 2));

    t = s;
    do {
	for (s = t; *t && *t != ':'; t++);
	*ptr = (char *)zalloc((t - s) + 1);
	ztrncpy(*ptr++, s, t - s);
    }
    while (*t++);
    *ptr = NULL;
    return ret;
}

char **spacesplit(s)		/**/
char *s;
{
    int ct;
    char *t, **ret, **ptr;

    for (t = s, ct = 0; *t; t++)
	if (isep(*t))
	    ct++;
    ptr = ret = (char **)zalloc(sizeof(char **) * (ct + 2));

    t = s;
    do {
	for (s = t; *t && !isep(*t); t++);
	*ptr = (char *)zalloc((t - s) + 1);
	ztrncpy(*ptr++, s, t - s);
    }
    while (*t++);
    *ptr = NULL;
    return ret;
}

int findsep(s, sep)		/**/
char **s;
char *sep;
{
    int i;
    char *t, *tt;

    if (!sep) {
	for (t = *s, i = 1; i && *t;) {
	    for (tt = ifs, i = 1; i && *tt; tt++)
		if (*tt == *t)
		    i = 0;
	    if (i)
		t++;
	}
	i = t - *s;
	*s = t;
	return i;
    }
    if (!sep[0]) {
	i = strlen(*s);
	*s += i;
	return i;
    }
    for (i = 0; **s; (*s)++, i++) {
	for (t = sep, tt = *s; *t && *tt && *t == *tt; t++, tt++);
	if (!*t)
	    return i;
    }
    return -1;
}

char *findword(s, sep)		/**/
char **s;
char *sep;
{
    char *r, *t, *tt;
    int f, sl;

    if (!**s)
	return NULL;

    if (sep) {
	sl = strlen(sep);
	r = *s;
	while (!(f = findsep(s, sep))) {
	    r = *s += sl;
	}
	return r;
    }
    for (t = *s, f = 1; f && *t;) {
	for (tt = ifs, f = 0; !f && *tt; tt++)
	    if (*tt == *t)
		f = 1;
	if (f)
	    t++;
    }
    *s = t;
    findsep(s, sep);
    return t;
}

int wordcount(s, sep, mul)	/**/
char *s;
char *sep;
int mul;
{
    int r = 1, sl, c, cc;
    char *t = s, *ti;

    if (sep) {
	sl = strlen(sep);
	for (; (c = findsep(&t, sep)) >= 0; t += sl)
	    if ((c && *(t + sl)) || mul)
		r++;
    } else {
	if (!mul)
	    for (c = 1; c && *t;) {
		for (c = 0, ti = ifs; !c && *ti; ti++)
		    if (*ti == *t)
			c = 1;
		if (c)
		    t++;
	    }
	if (!*t)
	    return 0;
	for (; *t; t++) {
	    for (c = 0, ti = ifs; !c && *ti; ti++)
		if (*ti == *t)
		    c = 1;
	    if (c && !mul) {
		for (cc = 1, t++; cc && *t;) {
		    for (cc = 0, ti = ifs; !cc && *ti; ti++)
			if (*ti == *t)
			    cc = 1;
		    if (cc)
			t++;
		}
		if (*t)
		    r++;
	    } else if (c)
		r++;
	}
    }
    return r;
}

char *sepjoin(s, sep)		/**/
char **s;
char *sep;
{
    char *r, *p, **t;
    int l, sl, elide = 0;
    static char *lastmem = NULL;
    char sepbuf[2];

    if (!*s)
	return "";
    if (!sep) {
	elide = 1;
	sep = sepbuf;
	sepbuf[0] = *ifs;
	sepbuf[1] = '\0';
    }
    sl = strlen(sep);
    for (t = s, l = 1 - sl; *t; l += strlen(*t) + sl, t++);
    if (l == 1)
	return "";
    zsfree(lastmem);
    lastmem = r = p = (char *)zalloc(l);
    t = s;
    if (elide)
	while (*t && !**t)
	    t++;
    for (; *t; t++) {
	strucpy(&p, *t);
	if (t[1] && (!elide || t[1][0]))
	    strucpy(&p, sep);
    }
    return r;
}

char **sepsplit(s, sep)		/**/
char *s;
char *sep;
{
    int n, sl, f;
    char *t, *tt, **r, **p;

    if (!sep)
	return spacesplit(s);

    sl = strlen(sep);
    n = wordcount(s, sep, 1);
    r = p = (char **)zalloc((n + 1) * sizeof(char *));

    for (t = s; n--;) {
	tt = t;
	f = findsep(&t, sep);	/* f set not but not used? ++jhi; */
	*p = (char *)zalloc(t - tt + 1);
	strncpy(*p, tt, t - tt);
	(*p)[t - tt] = '\0';
	p++;
	t += sl;
    }
    *p = NULL;

    return r;
}

List getshfunc(nam)		/**/
char *nam;
{
    Cmdnam x = (Cmdnam) gethnode(nam, cmdnamtab);

    if (x && (x->flags & SHFUNC)) {
	if (x->flags & PMFLAG_u) {
	    List l;

	    if (!(l = getfpfunc(nam))) {
		zerr("function not found: %s", nam, 0);
		return NULL;
	    }
	    x->flags &= ~PMFLAG_u;
	    permalloc();
	    x->u.list = (List) dupstruct(l);
	    lastalloc();
	}
	return x->u.list;
    }
    return NULL;
}

/* allocate a tree element */

static int sizetab[N_COUNT] =
{
    sizeof(struct list),
    sizeof(struct sublist),
    sizeof(struct pline),
    sizeof(struct cmd),
    sizeof(struct redir),
    sizeof(struct cond),
    sizeof(struct forcmd),
    sizeof(struct casecmd),
    sizeof(struct ifcmd),
    sizeof(struct whilecmd),
    sizeof(struct varasg)};

static int flagtab[N_COUNT] =
{
    NT_SET(N_LIST, 1, NT_NODE, NT_NODE, 0, 0),
    NT_SET(N_SUBLIST, 2, NT_NODE, NT_NODE, 0, 0),
    NT_SET(N_PLINE, 1, NT_NODE, NT_NODE, 0, 0),
    NT_SET(N_CMD, 2, NT_STR | NT_LIST, NT_NODE, NT_NODE | NT_LIST, NT_NODE | NT_LIST),
    NT_SET(N_REDIR, 3, NT_STR, 0, 0, 0),
    NT_SET(N_COND, 1, NT_NODE, NT_NODE, 0, 0),
    NT_SET(N_FOR, 1, NT_STR, NT_NODE, 0, 0),
    NT_SET(N_CASE, 0, NT_STR | NT_ARR, NT_NODE | NT_ARR, 0, 0),
    NT_SET(N_IF, 0, NT_NODE | NT_ARR, NT_NODE | NT_ARR, 0, 0),
    NT_SET(N_WHILE, 1, NT_NODE, NT_NODE, 0, 0),
    NT_SET(N_VARASG, 1, NT_STR, NT_STR, NT_STR | NT_LIST, 0)};

vptr allocnode(type)		/**/
int type;
{
    struct node *n = (struct node *)alloc(sizetab[type]);

    memset((char *)n, 0, sizetab[type]);
    n->type = flagtab[type];
    if (useheap)
	n->type |= NT_HEAP;

    return (vptr) n;
}

vptr dupstruct(a)		/**/
vptr a;
{
    struct node *n = (struct node *)a, *r;

    if (!a || ((List) a) == &dummy_list)
	return (vptr) a;

    if ((n->type & NT_HEAP) && !useheap) {
	heapalloc();
	n = (struct node *)dupstruct2((vptr) n);
	permalloc();
	n = simplifystruct(n);
    }
    r = (struct node *)dupstruct2((vptr) n);

    if (!(n->type & NT_HEAP) && useheap)
	r = expandstruct(r, N_LIST);

    return (vptr) r;
}

struct node *simplifystruct(n)	/**/
struct node *n;
{
    if (!n || ((List) n) == &dummy_list)
	return n;

    switch (NT_TYPE(n->type)) {
    case N_LIST:
	{
	    List l = (List) n;

	    l->left = (Sublist) simplifystruct((struct node *)l->left);
	    if (l->type == SYNC && !l->right)
		return (struct node *)l->left;
	}
	break;
    case N_SUBLIST:
	{
	    Sublist sl = (Sublist) n;

	    sl->left = (Pline) simplifystruct((struct node *)sl->left);
	    if (sl->type == END && !sl->flags && !sl->right)
		return (struct node *)sl->left;
	}
	break;
    case N_PLINE:
	{
	    Pline pl = (Pline) n;

	    pl->left = (Cmd) simplifystruct((struct node *)pl->left);
	    if (pl->type == END && !pl->right)
		return (struct node *)pl->left;
	}
	break;
    case N_CMD:
	{
	    Cmd c = (Cmd) n;
	    int i = 0;

	    if (empty(c->args))
		c->args = NULL, i++;
	    if (empty(c->redir))
		c->redir = NULL, i++;
	    if (empty(c->vars))
		c->vars = NULL, i++;

	    c->u.list = (List) simplifystruct((struct node *)c->u.list);
	    if (i == 3 && !c->flags &&
		(c->type == CWHILE || c->type == CIF ||
		 c->type == COND))
		return (struct node *)c->u.list;
	}
	break;
    case N_FOR:
	{
	    struct forcmd *f = (struct forcmd *)n;

	    f->list = (List) simplifystruct((struct node *)f->list);
	}
	break;
    case N_CASE:
	{
	    struct casecmd *c = (struct casecmd *)n;
	    List *l;

	    for (l = c->lists; *l; l++)
		*l = (List) simplifystruct((struct node *)*l);
	}
	break;
    case N_IF:
	{
	    struct ifcmd *i = (struct ifcmd *)n;
	    List *l;

	    for (l = i->ifls; *l; l++)
		*l = (List) simplifystruct((struct node *)*l);
	    for (l = i->thenls; *l; l++)
		*l = (List) simplifystruct((struct node *)*l);
	}
	break;
    case N_WHILE:
	{
	    struct whilecmd *w = (struct whilecmd *)n;

	    w->cont = (List) simplifystruct((struct node *)w->cont);
	    w->loop = (List) simplifystruct((struct node *)w->loop);
	}
    }

    return n;
}

struct node *expandstruct(n, exp)	/**/
struct node *n;
int exp;
{
    struct node *m;

    if (!n || ((List) n) == &dummy_list)
	return n;

    if (exp != N_COUNT && exp != NT_TYPE(n->type)) {
	switch (exp) {
	case N_LIST:
	    {
		List l;

		m = (struct node *)allocnode(N_LIST);
		l = (List) m;
		l->type = SYNC;
		l->left = (Sublist) expandstruct(n, N_SUBLIST);

		return (struct node *)l;
	    }
	case N_SUBLIST:
	    {
		Sublist sl;

		m = (struct node *)allocnode(N_SUBLIST);
		sl = (Sublist) m;
		sl->type = END;
		sl->left = (Pline) expandstruct(n, N_PLINE);

		return (struct node *)sl;
	    }
	case N_PLINE:
	    {
		Pline pl;

		m = (struct node *)allocnode(N_PLINE);
		pl = (Pline) m;
		pl->type = END;
		pl->left = (Cmd) expandstruct(n, N_CMD);

		return (struct node *)pl;
	    }
	case N_CMD:
	    {
		Cmd c;

		m = (struct node *)allocnode(N_CMD);
		c = (Cmd) m;
		switch (NT_TYPE(n->type)) {
		case N_WHILE:
		    c->type = CWHILE;
		    break;
		case N_IF:
		    c->type = CIF;
		    break;
		case N_COND:
		    c->type = COND;
		}
		c->u.list = (List) expandstruct(n, NT_TYPE(n->type));
		c->args = newlist();
		c->vars = newlist();
		c->redir = newlist();

		return (struct node *)c;
	    }
	}
    } else
	switch (NT_TYPE(n->type)) {
	case N_LIST:
	    {
		List l = (List) n;

		l->left = (Sublist) expandstruct((struct node *)l->left,
						 N_SUBLIST);
		l->right = (List) expandstruct((struct node *)l->right,
					       N_LIST);
	    }
	    break;
	case N_SUBLIST:
	    {
		Sublist sl = (Sublist) n;

		sl->left = (Pline) expandstruct((struct node *)sl->left,
						N_PLINE);
		sl->right = (Sublist) expandstruct((struct node *)sl->right,
						   N_SUBLIST);
	    }
	    break;
	case N_PLINE:
	    {
		Pline pl = (Pline) n;

		pl->left = (Cmd) expandstruct((struct node *)pl->left,
					      N_CMD);
		pl->right = (Pline) expandstruct((struct node *)pl->right,
						 N_PLINE);
	    }
	    break;
	case N_CMD:
	    {
		Cmd c = (Cmd) n;

		if (!c->args)
		    c->args = newlist();
		if (!c->vars)
		    c->vars = newlist();
		if (!c->redir)
		    c->redir = newlist();

		switch (c->type) {
		case CFOR:
		case CSELECT:
		    c->u.list = (List) expandstruct((struct node *)c->u.list,
						    N_FOR);
		    break;
		case CWHILE:
		    c->u.list = (List) expandstruct((struct node *)c->u.list,
						    N_WHILE);
		    break;
		case CIF:
		    c->u.list = (List) expandstruct((struct node *)c->u.list,
						    N_IF);
		    break;
		case CCASE:
		    c->u.list = (List) expandstruct((struct node *)c->u.list,
						    N_CASE);
		    break;
		case COND:
		    c->u.list = (List) expandstruct((struct node *)c->u.list,
						    N_COND);
		    break;
		case ZCTIME:
		    c->u.list = (List) expandstruct((struct node *)c->u.list,
						    N_SUBLIST);
		    break;
		default:
		    c->u.list = (List) expandstruct((struct node *)c->u.list,
						    N_LIST);
		}
	    }
	    break;
	case N_FOR:
	    {
		struct forcmd *f = (struct forcmd *)n;

		f->list = (List) expandstruct((struct node *)f->list,
					      N_LIST);
	    }
	    break;
	case N_CASE:
	    {
		struct casecmd *c = (struct casecmd *)n;
		List *l;

		for (l = c->lists; *l; l++)
		    *l = (List) expandstruct((struct node *)*l, N_LIST);
	    }
	    break;
	case N_IF:
	    {
		struct ifcmd *i = (struct ifcmd *)n;
		List *l;

		for (l = i->ifls; *l; l++)
		    *l = (List) expandstruct((struct node *)*l, N_LIST);
		for (l = i->thenls; *l; l++)
		    *l = (List) expandstruct((struct node *)*l, N_LIST);
	    }
	    break;
	case N_WHILE:
	    {
		struct whilecmd *w = (struct whilecmd *)n;

		w->cont = (List) expandstruct((struct node *)w->cont,
					      N_LIST);
		w->loop = (List) expandstruct((struct node *)w->loop,
					      N_LIST);
	    }
	}

    return n;
}

/* duplicate a syntax tree node of given type, argument number */

vptr dupnode(type, a, argnum)	/**/
int type;
vptr a;
int argnum;
{
    if (!a)
	return NULL;
    switch (NT_N(type, argnum)) {
    case NT_NODE:
	return (vptr) dupstruct2(a);
    case NT_STR:
	return (useheap) ? ((vptr) dupstring(a)) :
	    ((vptr) ztrdup(a));
    case NT_LIST | NT_NODE:
	if (type & NT_HEAP)
	    if (useheap)
		return (vptr) duplist(a, (VFunc) dupstruct2);
	    else
		return (vptr) list2arr(a, (VFunc) dupstruct2);
	else if (useheap)
	    return (vptr) arr2list(a, (VFunc) dupstruct2);
	else
	    return (vptr) duparray(a, (VFunc) dupstruct2);
    case NT_LIST | NT_STR:
	if (type & NT_HEAP)
	    if (useheap)
		return (vptr) duplist(a, (VFunc) dupstring);
	    else
		return (vptr) list2arr(a, (VFunc) ztrdup);
	else if (useheap)
	    return (vptr) arr2list(a, (VFunc) dupstring);
	else
	    return (vptr) duparray(a, (VFunc) ztrdup);
    case NT_NODE | NT_ARR:
	return (vptr) duparray(a, (VFunc) dupstruct2);
    case NT_STR | NT_ARR:
	return (vptr) duparray(a, (VFunc) (useheap ? dupstring : ztrdup));
    default:
	abort();
    }
}

/* Free a syntax tree node of given type, argument number */

void freenode(type, a, argnum)	/**/
int type;
vptr a;
int argnum;
{
    if (!a)
	return;
    switch (NT_N(type, argnum)) {
    case NT_NODE:
	freestruct(a);
	break;
    case NT_STR:
	zsfree(a);
	break;
    case NT_LIST | NT_NODE:
    case NT_NODE | NT_ARR:
	{
	    char **p = (char **)a;

	    while (*p)
		freestruct(*p++);
	    free(a);
	}
	break;
    case NT_LIST | NT_STR:
    case NT_STR | NT_ARR:
	freearray(a);
	break;
    default:
	abort();
    }
}

/* duplicate a syntax tree */

vptr *dupstruct2(a)		/**/
vptr a;
{
    struct node *n = (struct node *)a, *m;
    int type;

    if (!n || ((List) n) == &dummy_list)
	return a;
    type = n->type;
    m = (struct node *)alloc(sizetab[NT_TYPE(type)]);
    m->type = (type & ~NT_HEAP);
    if (useheap)
	m->type |= NT_HEAP;
    switch (NT_TYPE(type)) {
    case N_LIST:
	{
	    List nl = (List) n;
	    List ml = (List) m;

	    ml->type = nl->type;
	    ml->left = (Sublist) dupnode(type, nl->left, 0);
	    ml->right = (List) dupnode(type, nl->right, 1);
	}
	break;
    case N_SUBLIST:
	{
	    Sublist nsl = (Sublist) n;
	    Sublist msl = (Sublist) m;

	    msl->type = nsl->type;
	    msl->flags = nsl->flags;
	    msl->left = (Pline) dupnode(type, nsl->left, 0);
	    msl->right = (Sublist) dupnode(type, nsl->right, 1);
	}
	break;
    case N_PLINE:
	{
	    Pline npl = (Pline) n;
	    Pline mpl = (Pline) m;

	    mpl->type = npl->type;
	    mpl->left = (Cmd) dupnode(type, npl->left, 0);
	    mpl->right = (Pline) dupnode(type, npl->right, 1);
	}
	break;
    case N_CMD:
	{
	    Cmd nc = (Cmd) n;
	    Cmd mc = (Cmd) m;

	    mc->type = nc->type;
	    mc->flags = nc->flags;
	    mc->lineno = nc->lineno;
	    mc->args = (Lklist) dupnode(type, nc->args, 0);
	    mc->u.generic = (vptr) dupnode(type, nc->u.generic, 1);
	    mc->redir = (Lklist) dupnode(type, nc->redir, 2);
	    mc->vars = (Lklist) dupnode(type, nc->vars, 3);
	}
	break;
    case N_REDIR:
	{
	    Redir nr = (Redir) n;
	    Redir mr = (Redir) m;

	    mr->type = nr->type;
	    mr->fd1 = nr->fd1;
	    mr->fd2 = nr->fd2;
	    mr->name = (char *)dupnode(type, nr->name, 0);
	}
	break;
    case N_COND:
	{
	    Cond nco = (Cond) n;
	    Cond mco = (Cond) m;

	    mco->type = nco->type;
	    mco->left = (vptr) dupnode(type, nco->left, 0);
	    mco->right = (vptr) dupnode(type, nco->right, 1);
	}
	break;
    case N_FOR:
	{
	    struct forcmd *nf = (struct forcmd *)n;
	    struct forcmd *mf = (struct forcmd *)m;

	    mf->inflag = nf->inflag;
	    mf->name = (char *)dupnode(type, nf->name, 0);
	    mf->list = (List) dupnode(type, nf->list, 1);
	}
	break;
    case N_CASE:
	{
	    struct casecmd *ncc = (struct casecmd *)n;
	    struct casecmd *mcc = (struct casecmd *)m;

	    mcc->pats = (char **)dupnode(type, ncc->pats, 0);
	    mcc->lists = (List *) dupnode(type, ncc->lists, 1);
	}
	break;
    case N_IF:
	{
	    struct ifcmd *nic = (struct ifcmd *)n;
	    struct ifcmd *mic = (struct ifcmd *)m;

	    mic->ifls = (List *) dupnode(type, nic->ifls, 0);
	    mic->thenls = (List *) dupnode(type, nic->thenls, 1);

	}
	break;
    case N_WHILE:
	{
	    struct whilecmd *nwc = (struct whilecmd *)n;
	    struct whilecmd *mwc = (struct whilecmd *)m;

	    mwc->cond = nwc->cond;
	    mwc->cont = (List) dupnode(type, nwc->cont, 0);
	    mwc->loop = (List) dupnode(type, nwc->loop, 1);
	}
	break;
    case N_VARASG:
	{
	    Varasg nva = (Varasg) n;
	    Varasg mva = (Varasg) m;

	    mva->type = nva->type;
	    mva->name = (char *)dupnode(type, nva->name, 0);
	    mva->str = (char *)dupnode(type, nva->str, 1);
	    mva->arr = (Lklist) dupnode(type, nva->arr, 2);
	}
	break;
    }
    return (vptr *) m;
}

/* free a syntax tree */

void freestruct(a)		/**/
vptr a;
{
    struct node *n = (struct node *)a;
    int type;

    if (!n || ((List) n) == &dummy_list)
	return;

    type = n->type;
    switch (NT_TYPE(type)) {
    case N_LIST:
	{
	    List nl = (List) n;

	    freenode(type, nl->left, 0);
	    freenode(type, nl->right, 1);
	}
	break;
    case N_SUBLIST:
	{
	    Sublist nsl = (Sublist) n;

	    freenode(type, nsl->left, 0);
	    freenode(type, nsl->right, 1);
	}
	break;
    case N_PLINE:
	{
	    Pline npl = (Pline) n;

	    freenode(type, npl->left, 0);
	    freenode(type, npl->right, 1);
	}
	break;
    case N_CMD:
	{
	    Cmd nc = (Cmd) n;

	    freenode(type, nc->args, 0);
	    freenode(type, nc->u.generic, 1);
	    freenode(type, nc->redir, 2);
	    freenode(type, nc->vars, 3);
	}
	break;
    case N_REDIR:
	{
	    Redir nr = (Redir) n;

	    freenode(type, nr->name, 0);
	}
	break;
    case N_COND:
	{
	    Cond nco = (Cond) n;

	    freenode(type, nco->left, 0);
	    freenode(type, nco->right, 1);
	}
	break;
    case N_FOR:
	{
	    struct forcmd *nf = (struct forcmd *)n;

	    freenode(type, nf->name, 0);
	    freenode(type, nf->list, 1);
	}
	break;
    case N_CASE:
	{
	    struct casecmd *ncc = (struct casecmd *)n;

	    freenode(type, ncc->pats, 0);
	    freenode(type, ncc->lists, 1);
	}
	break;
    case N_IF:
	{
	    struct ifcmd *nic = (struct ifcmd *)n;

	    freenode(type, nic->ifls, 0);
	    freenode(type, nic->thenls, 1);

	}
	break;
    case N_WHILE:
	{
	    struct whilecmd *nwc = (struct whilecmd *)n;

	    freenode(type, nwc->cont, 0);
	    freenode(type, nwc->loop, 1);
	}
	break;
    case N_VARASG:
	{
	    Varasg nva = (Varasg) n;

	    freenode(type, nva->name, 0);
	    freenode(type, nva->str, 1);
	    freenode(type, nva->arr, 2);
	}
	break;
    }
    zfree(n, sizetab[NT_TYPE(type)]);
}

Lklist duplist(l, func)		/**/
Lklist l;
VFunc func;
{
    Lklist ret;
    Lknode node;

    ret = newlist();
    for (node = firstnode(l); node; incnode(node))
	addnode(ret, func(getdata(node)));
    return ret;
}

char **duparray(arr, func)	/**/
char **arr;
VFunc func;
{
    char **ret = (char **)alloc((arrlen(arr) + 1) * sizeof(char *)), **rr;

    for (rr = ret; *arr;)
	*rr++ = (char *)func(*arr++);
    *rr = NULL;

    return ret;
}

char **list2arr(l, func)	/**/
Lklist l;
VFunc func;
{
    char **arr, **r;
    Lknode n;

    arr = r = (char **)alloc((countnodes(l) + 1) * sizeof(char *));

    for (n = firstnode(l); n; incnode(n))
	*r++ = (char *)func(getdata(n));
    *r = NULL;

    return arr;
}

Lklist arr2list(arr, func)	/**/
char **arr;
VFunc func;
{
    Lklist l = newlist();

    while (*arr)
	addnode(l, func(*arr++));

    return l;
}

char **mkarray(s)		/**/
char *s;
{
    char **t = (char **)zalloc((s) ? (2 * sizeof s) : (sizeof s));

    if ((*t = s))
	t[1] = NULL;
    return t;
}

void feep()
{				/**/
    if (unset(NOBEEP))
	write(2, "\07", 1);
}

void freearray(s)		/**/
char **s;
{
    char **t = s;

    while (*s)
	zsfree(*s++);
    free(t);
}

int equalsplit(s, t)		/**/
char *s;
char **t;
{
    for (; *s && *s != '='; s++);
    if (*s == '=') {
	*s++ = '\0';
	*t = s;
	return 1;
    }
    return 0;
}

/* see if the right side of a list is trivial */

void simplifyright(l)		/**/
List l;
{
    Cmd c;

    if (l == &dummy_list || !l->right)
	return;
    if (l->right->right || l->right->left->right ||
	l->right->left->flags || l->right->left->left->right ||
	l->left->flags)
	return;
    c = l->left->left->left;
    if (c->type != SIMPLE || full(c->args) || full(c->redir)
	|| full(c->vars))
	return;
    l->right = NULL;
    return;
}

/* initialize the ztypes table */

void inittyptab()
{				/**/
    int t0;
    char *s;

    for (t0 = 0; t0 != 256; t0++)
	typtab[t0] = 0;
    for (t0 = 0; t0 != 32; t0++)
	typtab[t0] = typtab[t0 + 128] = ICNTRL;
    typtab[127] = ICNTRL;
    for (t0 = '0'; t0 <= '9'; t0++)
	typtab[t0] = IDIGIT | IALNUM | IWORD | IIDENT | IUSER;
    for (t0 = 'a'; t0 <= 'z'; t0++)
	typtab[t0] = typtab[t0 - 'a' + 'A'] = IALPHA | IALNUM | IIDENT | IUSER | IWORD;
    for (t0 = 0240; t0 != 0400; t0++)
	typtab[t0] = IALPHA | IALNUM | IIDENT | IUSER | IWORD;
    typtab['_'] = IIDENT | IUSER;
    typtab['-'] = IUSER;
    typtab[' '] |= IBLANK | INBLANK;
    typtab['\t'] |= IBLANK | INBLANK;
    typtab['\n'] |= INBLANK;
    for (t0 = (int)STOUC(ALPOP); t0 <= (int)STOUC(Nularg);
	 t0++)
	typtab[t0] |= ITOK;
    for (s = ifs; *s; s++)
	typtab[(int)(unsigned char)*s] |= ISEP;
    for (s = wordchars; *s; s++)
	typtab[(int)(unsigned char)*s] |= IWORD;
    for (s = SPECCHARS; *s; s++)
	typtab[(int)(unsigned char)*s] |= ISPECIAL;
}

char **arrdup(s)		/**/
char **s;
{
    char **x, **y;

    y = x = (char **)ncalloc(sizeof(char *) * (arrlen(s) + 1));

    while ((*x++ = dupstring(*s++)));
    return y;
}

/* next few functions stolen (with changes) from Kernighan & Pike */
/* "The UNIX Programming Environment" (w/o permission) */

char *spname(oldname)		/**/
char *oldname;
{
    char *p, spnameguess[MAXPATHLEN + 1], spnamebest[MAXPATHLEN + 1];
    static char newname[MAXPATHLEN + 1];
    char *new = newname, *old;

    if (itok(*oldname)) {
	singsub(&oldname);
	if (!oldname)
	    return NULL;
    }
    if (access(oldname, F_OK) == 0)
	return NULL;
    old = oldname;
    for (;;) {
	while (*old == '/')
	    *new++ = *old++;
	*new = '\0';
	if (*old == '\0')
	    return newname;
	p = spnameguess;
	for (; *old != '/' && *old != '\0'; old++)
	    if (p < spnameguess + MAXPATHLEN)
		*p++ = *old;
	*p = '\0';
	if (mindist(newname, spnameguess, spnamebest) >= 3)
	    return NULL;
	for (p = spnamebest; (*new = *p++);)
	    new++;
    }
}

int mindist(dir, mindistguess, mindistbest)	/**/
char *dir;
char *mindistguess;
char *mindistbest;
{
    int mindistd, nd;
    DIR *dd;
    struct dirent *de;
    char buf[MAXPATHLEN];

    if (dir[0] == '\0')
	dir = ".";
    mindistd = 100;
    sprintf(buf, "%s/%s", dir, mindistguess);
    if (access(buf, F_OK) == 0) {
	strcpy(mindistbest, mindistguess);
	return 0;
    }
    if (!(dd = opendir(dir)))
	return mindistd;
    while ((de = readdir(dd))) {
	nd = spdist(de->d_name, mindistguess,
		    (int)strlen(mindistguess) / 4 + 1);
	if (nd <= mindistd) {
	    strcpy(mindistbest, de->d_name);
	    mindistd = nd;
	    if (mindistd == 0)
		break;
	}
    }
    closedir(dd);
    return mindistd;
}

int spdist(s, t, thresh)	/**/
char *s;
char *t;
int thresh;
{
    char *p, *q;
    char *keymap =
    "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\
\t1234567890-=\t\
\tqwertyuiop[]\t\
\tasdfghjkl;'\n\t\
\tzxcvbnm,./\t\t\t\
\n\n\n\n\n\n\n\n\n\n\n\n\n\n\
\t!@#$%^&*()_+\t\
\tQWERTYUIOP{}\t\
\tASDFGHJKL:\"\n\t\
\tZXCVBNM<>?\n\n\t\
\n\n\n\n\n\n\n\n\n\n\n\n\n\n";

    if (!strcmp(s, t))
	return 0;
/* any number of upper/lower mistakes allowed (dist = 1) */
    for (p = s, q = t; *p && tulower(*p) == tulower(*q); p++, q++);
    if (!*p && !*q)
	return 1;
    if (!thresh)
	return 200;
    for (p = s, q = t; *p && *q; p++, q++)
	if (*p == *q)
	    continue;		/* don't consider "aa" transposed, ash */
	else if (p[1] == q[0] && q[1] == p[0])	/* transpositions */
	    return spdist(p + 2, q + 2, thresh - 1) + 1;
	else if (p[1] == q[0])	/* missing letter */
	    return spdist(p + 1, q + 0, thresh - 1) + 2;
	else if (p[0] == q[1])	/* missing letter */
	    return spdist(p + 0, q + 1, thresh - 1) + 2;
	else if (*p != *q)
	    break;
    if ((!*p && strlen(q) == 1) || (!*q && strlen(p) == 1))
	return 2;
    for (p = s, q = t; *p && *q; p++, q++)
	if (p[0] != q[0] && p[1] == q[1]) {
	    int t0;
	    char *z;

	/* mistyped letter */

	    if (!(z = strchr(keymap, p[0])) || *z == '\n' || *z == '\t')
		return spdist(p + 1, q + 1, thresh - 1) + 1;
	    t0 = z - keymap;
	    if (*q == keymap[t0 - 15] || *q == keymap[t0 - 14] ||
		*q == keymap[t0 - 13] ||
		*q == keymap[t0 - 1] || *q == keymap[t0 + 1] ||
		*q == keymap[t0 + 13] || *q == keymap[t0 + 14] ||
		*q == keymap[t0 + 15])
		return spdist(p + 1, q + 1, thresh - 1) + 2;
	    return 200;
	} else if (*p != *q)
	    break;
    return 200;
}

char *zgetenv(s)		/**/
char *s;
{
    char **av, *p, *q;

    for (av = environ; *av; av++) {
	for (p = *av, q = s; *p && *p != '=' && *q && *p == *q; p++, q++);
	if (*p == '=' && !*q)
	    return p + 1;
    }
    return NULL;
}

int tulower(c)			/**/
int c;
{
    c &= 0xff;
    return (isupper(c) ? tolower(c) : c);
}

int tuupper(c)			/**/
int c;
{
    c &= 0xff;
    return (islower(c) ? toupper(c) : c);
}

#ifdef SYSV
#include <sys/utsname.h>

int gethostname(nameptr, maxlength)
char *nameptr;
int maxlength;
{
    struct utsname name;
    int result;

    result = uname(&name);
    if (result >= 0) {
	strncpy(nameptr, name.nodename, maxlength);
	return 0;
    } else
	return -1;
}

#endif

/* set cbreak mode, or the equivalent */

void setcbreak()
{				/**/
    struct ttyinfo ti;

    ti = shttyinfo;
#ifdef HAS_TIO
    ti.tio.c_lflag &= ~ICANON;
    ti.tio.c_cc[VMIN] = 1;
    ti.tio.c_cc[VTIME] = 0;
#else
    ti.sgttyb.sg_flags |= CBREAK;
#endif
    settyinfo(&ti);
}

/* give the tty to some process */

void attachtty(pgrp)		/**/
long pgrp;
{
    static int ep = 0;

    if (jobbing) {
#ifdef HAS_TCSETPGRP
	if (SHTTY != -1 && tcsetpgrp(SHTTY, pgrp) == -1 && !ep)
#else
#if ardent
	if (SHTTY != -1 && setpgrp() == -1 && !ep)
#else
	int arg = pgrp;

	if (SHTTY != -1 && ioctl(SHTTY, TIOCSPGRP, &arg) == -1 && !ep)
#endif
#endif
	{
	    zerr("can't set tty pgrp: %e", NULL, errno);
	    fflush(stderr);
	    opts[MONITOR] = OPT_UNSET;
	    ep = 1;
	    errflag = 0;
	}
    }
}

/* get the tty pgrp */

long gettygrp()
{				/**/
    int arg;

    if (SHTTY == -1)
	return -1;
#ifdef HAS_TCSETPGRP
    arg = tcgetpgrp(SHTTY);
#else
#if ardent
    arg = getpgrp();
#else
    ioctl(SHTTY, TIOCGPGRP, &arg);
#endif
#endif
    return arg;
}

#if defined(SCO)
void gettimeofday(struct timeval *tv, struct timezone *tz)
{
    tv->tv_usec = 0;
    tv->tv_sec = (long)time((time_t) 0);
}
#endif
