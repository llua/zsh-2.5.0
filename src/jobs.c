/*
 * jobs.c - job control
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
#include <setjmp.h>

#if defined(POSIX) && !defined(__386BSD__) && !defined(__NetBSD__) && !defined(__FreeBSD__)
#define JMP_BUF       sigjmp_buf
#define SETJMP(b)     sigsetjmp((b), 1)
#define LONGJMP(b,n)  siglongjmp((b), (n))
#else
#define JMP_BUF       jmp_buf
#define SETJMP(b)     setjmp(b)
#define LONGJMP(b,n)  longjmp((b), (n))
#endif

#if defined(RESETHANDNEEDED) && !defined(POSIX)
#define SIGPROCESS(sig)  sig_ignore(sig)
#define SIGRESET(sig)    sig_handle(sig)
#else
#define SIGPROCESS(sig)  ;
#define SIGRESET(sig)    ;
#endif

/* empty job structure for quick clearing of jobtab entries */

static struct job zero;		/* static variables are initialized to zero */

struct timeval dtimeval, now;
struct timezone dummy_tz;

/* Diff two timevals for elapsed-time computations */

struct timeval *dtime(dt, t1, t2)	/**/
struct timeval *dt;
struct timeval *t1;
struct timeval *t2;
{
    dt->tv_sec = t2->tv_sec - t1->tv_sec;
    dt->tv_usec = t2->tv_usec - t1->tv_usec;
    if (dt->tv_usec < 0) {
	dt->tv_usec += 1000000;
	dt->tv_sec -= 1;
    }
    return dt;
}

/* do a safe, race-free sigpause() to wait for SIGCHLD */

static int chld_longjmp = 0;
static struct z_jmp_buf {
    JMP_BUF jbuf;
} foil_race;

void chldhandler DCLPROTO((struct z_jmp_buf * jump));

void chldsuspend(sig)		/**/
int sig;
{
/* assumes blockchld() is in effect */
    if (SETJMP(foil_race.jbuf) == 0) {
	chld_longjmp = 1;
	unblockchld();
	chldpause(sig);
    }
    chld_longjmp = 0;
}

#ifdef INTHANDTYPE
#define RETURN   { SIGRESET(sig); return 0; }
#else
#define RETURN   { SIGRESET(sig); return; }
#endif

static int from_sig = 0;

/* the signal handler */

HANDTYPE handler(sig)		/**/
int sig;
{
    sigset_t heldsigs;
    int do_jump;
    struct z_jmp_buf jump_to;

    SIGPROCESS(sig);

    fast_block(&heldsigs);	/* Prevent signal traps temporarily */

    do_jump = chld_longjmp;
    chld_longjmp = 0;		/* In case a SIGCHLD somehow arrives */

    if (zigheld) {
	zighold(sig, heldsigs);	/* zigsafe() will fast_unblock(&heldsigs) */
	RETURN;
    }
    if (sig == SIGCHLD) {	/* Traps can cause nested chldsuspend() */
	if (do_jump)
	    jump_to = foil_race;/* copy foil_race */
    }
    fast_unblock(&heldsigs);	/* Signal traps OK again (foil_race copied) */

    switch (sig) {
    case SIGHUP:
	if (sigtrapped[SIGHUP])
	    dotrap(SIGHUP);
	else {
	    stopmsg = 1;
	    from_sig = 1;
	    zexit(SIGHUP);
	}
	break;

    case SIGINT:
	if (sigtrapped[SIGINT])
	    dotrap(SIGINT);
	else {
	    breaks = loops;
	    errflag = 1;
	}
	break;

#if defined(SIGWINCH) && defined(TIOCGWINSZ)
    case SIGWINCH:
	adjustwinsize();
	break;
#endif

    case SIGCHLD:
	chldhandler(do_jump ? &jump_to : (struct z_jmp_buf *)0);
	RETURN;

    default:
	dotrap(sig);
	if (sig == SIGALRM) {
	    if (!sigtrapped[SIGALRM]) {
		zerr("timeout", NULL, 0);
		exit(1);
	    } else if (tmout) {
		alarm(tmout);
	    }
	}
	break;
    }

    RETURN;
}

#undef RETURN

#define RETURN \
    if (jump) { SIGRESET(SIGCHLD); LONGJMP(jump->jbuf, 1); } else return

void chldhandler(jump)
struct z_jmp_buf *jump;
{
    long pid;
    int statusp;
    Job jn;
    struct process *pn;

#ifdef HAS_RUSAGE
    struct rusage ru;

#else
    long chlds, chldu;

#endif

    for (;;) {
	int old_errno = errno;

#ifdef HAS_RUSAGE
	pid = wait3((vptr) & statusp, WNOHANG | WUNTRACED, &ru);
#else
#ifndef WNOHANG
	pid = wait(&statusp);
#else
#ifdef HAS_WAITPID
	pid = waitpid(-1, (vptr) & statusp, WNOHANG | WUNTRACED);
#else
	pid = wait3((vptr) & statusp, WNOHANG | WUNTRACED, NULL);
#endif
#endif
	chlds = shtms.tms_cstime;
	chldu = shtms.tms_cutime;
	times(&shtms);
#endif
	if (pid == -1) {
	    if (errno != ECHILD)
		zerr("wait failed: %e", NULL, errno);
	    errno = old_errno;
	    RETURN;
	}
	errno = old_errno;
	if (!pid)
	    RETURN;
	if (pid == cmdoutpid) {
	    cmdoutdone(statusp);
	    continue;
	}
	findproc(pid, &jn, &pn);/* find the process of this pid */
	if (jn) {
	    pn->statusp = statusp;
#ifdef HAS_RUSAGE
	    pn->ti.ru = ru;
#else
	    pn->ti.st = shtms.tms_cstime - chlds;
	    pn->ti.ut = shtms.tms_cutime - chldu;
#endif
	    gettimeofday(&pn->endtime, &dummy_tz);
	    updatestatus(jn);
	}
#if 0
	else if (WIFSTOPPED(statusp))
	    kill(pid, SIGKILL);	/* kill stopped untraced children */
#endif
    }
}

#undef RETURN

/* clean up after a $() or `` substitution */

void cmdoutdone(statusp)	/**/
int statusp;
{
    cmdoutpid = 0;
    if (WIFSIGNALED(statusp)) {
	cmdoutval = (0200 | WTERMSIG(statusp));
	if (WTERMSIG(statusp) == SIGINT)
	    (void)kill(getpid(), SIGINT);
	else if (sigtrapped[WTERMSIG(statusp)])
	    dotrap(WTERMSIG(statusp));
    } else
	cmdoutval = WEXITSTATUS(statusp);
}

/* change job table entry from stopped to running */

void makerunning(jn)		/**/
Job jn;
{
    struct process *pn;

    jn->stat &= ~STAT_STOPPED;
    for (pn = jn->procs; pn; pn = pn->next)
	if (WIFSTOPPED(pn->statusp))
	    pn->statusp = SP_RUNNING;
}

/* update status of job, possibly printing it */

void updatestatus(jn)		/**/
Job jn;
{
    struct process *pn;
    int notrunning = 1, alldone = 1, val = 0, job = jn - jobtab;
    int statusp = 0, somestopped = 0, inforeground = 0;
    int pgrp;

    pgrp = gettygrp();
    for (pn = jn->procs; pn; pn = pn->next) {
	if (pn->statusp == SP_RUNNING)
	    notrunning = 0;
	if (pn->statusp == SP_RUNNING || WIFSTOPPED(pn->statusp))
	    alldone = 0;
	if (WIFSTOPPED(pn->statusp))
	    somestopped = 1;
	if (!pn->next && jn)
	    val = (WIFSIGNALED(pn->statusp)) ?
		0200 | WTERMSIG(pn->statusp) : WEXITSTATUS(pn->statusp);
	if (pn->pid == jn->gleader) {
	    statusp = pn->statusp;
	    if (pgrp == 0 || pn->pid == pgrp || 
		(pgrp > 1 && kill(-pgrp, 0) == -1))
		inforeground = 1;
	}
    }
    if (!notrunning)
	return;
    if (somestopped) {
	if (jn->stty_in_env && !jn->ty) {
	    jn->ty = (struct ttyinfo *)zalloc(sizeof(struct ttyinfo));

	    gettyinfo(jn->ty);
	}
	if (jn->stat & STAT_STOPPED)
	    return;
    }
    if (alldone && job == thisjob)
	lastval = val;
    if (alldone)
	lastval2 = val;
    if (inforeground && !ttyfrozen && !val && !jn->stty_in_env)
	gettyinfo(&shttyinfo);
#ifdef TIOCGWINSZ
    adjustwinsize();
#endif
    jn->stat |= (alldone) ? STAT_CHANGED | STAT_DONE :
	STAT_CHANGED | STAT_STOPPED;
    if ((jn->stat & (STAT_DONE | STAT_STOPPED)) == STAT_STOPPED) {
	prevjob = curjob;
	curjob = job;
    }
    if ((isset(NOTIFY) || job == thisjob) && (jn->stat & STAT_LOCKED)) {
	printjob(jn, !!isset(LONGLISTJOBS));
	if (zleactive)
	    refresh();
    }
    if (sigtrapped[SIGCHLD] && job != thisjob)
	dotrap(SIGCHLD);

 /* If the foreground job got a signal, pretend we got it, too. */
    if (inforeground && WIFSIGNALED(statusp)) {
	if (sigtrapped[WTERMSIG(statusp)]) {
	    dotrap(WTERMSIG(statusp));
	} else if (WTERMSIG(statusp) == SIGINT ||
		   WTERMSIG(statusp) == SIGQUIT) {
	    breaks = loops;
	    errflag = 1;
	}
    }
}

/* find process and job associated with pid */

void findproc(pid, jptr, pptr)	/**/
int pid;
Job *jptr;
struct process **pptr;
{
    struct process *pn;
    int jn;

    for (jn = 1; jn != MAXJOB; jn++)
	for (pn = jobtab[jn].procs; pn; pn = pn->next)
	    if (pn->pid == pid) {
		*pptr = pn;
		*jptr = jobtab + jn;
		return;
	    }
    *pptr = NULL;
    *jptr = NULL;
}

/*
	lng = 0 means jobs
	lng = 1 means jobs -l
	lng = 2 means jobs -p
*/

void printjob(jn, lng)		/**/
Job jn;
int lng;
{
    int job = jn - jobtab, len = 9, sig, sflag = 0, llen;
    int conted = 0, lineleng = columns, skip = 0, doputnl = 0;
    struct process *pn;

    if (lng < 0) {
	conted = 1;
	lng = 0;
    }
/* find length of longest signame, check to see if we
		really need to print this job */

    for (pn = jn->procs; pn; pn = pn->next) {
	if (pn->statusp != SP_RUNNING)
	    if (WIFSIGNALED(pn->statusp)) {
		sig = WTERMSIG(pn->statusp);
		llen = strlen(sigmsg[sig]);
		if (WCOREDUMP(pn->statusp))
		    llen += 14;
		if (llen > len)
		    len = llen;
		if (sig != SIGINT && sig != SIGPIPE)
		    sflag = 1;
		else if (sig == SIGINT)
		    errflag = 1;
		if (job == thisjob && sig == SIGINT)
		    doputnl = 1;
	    } else if (WIFSTOPPED(pn->statusp)) {
		sig = WSTOPSIG(pn->statusp);
		if ((int)strlen(sigmsg[sig]) > len)
		    len = strlen(sigmsg[sig]);
		if (job == thisjob && sig == SIGTSTP)
		    doputnl = 1;
	    } else if (isset(PRINTEXITVALUE) && isset(SHINSTDIN) &&
		       WEXITSTATUS(pn->statusp))
		sflag = 1;
    }

/* print if necessary */

    if (interact && jobbing && ((jn->stat & STAT_STOPPED) || sflag ||
				job != thisjob)) {
	int len2, fline = 1;
	struct process *qn;

	trashzle();
	if (doputnl)
	    putc('\n', stderr);
	for (pn = jn->procs; pn;) {
	    len2 = ((job == thisjob) ? 5 : 10) + len;	/* 2 spaces */
	    if (lng)
		qn = pn->next;
	    else
		for (qn = pn->next; qn; qn = qn->next) {
		    if (qn->statusp != pn->statusp)
			break;
		    if ((int)strlen(qn->text) + len2 + ((qn->next) ? 3 : 0) > lineleng)
			break;
		    len2 += strlen(qn->text) + 2;
		}
	    if (job != thisjob)
		if (fline)
		    fprintf(stderr, "[%ld]  %c ",
			    (long)(jn - jobtab),
			    (job == curjob) ? '+'
			    : (job == prevjob) ? '-' : ' ');
		else
		    fprintf(stderr, (job > 9) ? "        " : "       ");
	    else
		fprintf(stderr, "zsh: ");
	    if (lng)
		if (lng == 1)
		    fprintf(stderr, "%ld ", pn->pid);
		else {
		    int x = jn->gleader;

		    fprintf(stderr, "%d ", x);
		    do
			skip++;
		    while ((x /= 10));
		    skip++;
		    lng = 0;
	    } else
		fprintf(stderr, "%*s", skip, "");
	    if (pn->statusp == SP_RUNNING)
		if (!conted)
		    fprintf(stderr, "running%*s", len - 7 + 2, "");
		else
		    fprintf(stderr, "continued%*s", len - 9 + 2, "");
	    else if (WIFEXITED(pn->statusp))
		if (WEXITSTATUS(pn->statusp))
		    fprintf(stderr, "exit %-4d%*s", WEXITSTATUS(pn->statusp),
			    len - 9 + 2, "");
		else
		    fprintf(stderr, "done%*s", len - 4 + 2, "");
	    else if (WIFSTOPPED(pn->statusp))
		fprintf(stderr, "%-*s", len + 2, sigmsg[WSTOPSIG(pn->statusp)]);
	    else if (WCOREDUMP(pn->statusp))
		fprintf(stderr, "%s (core dumped)%*s",
			sigmsg[WTERMSIG(pn->statusp)],
			(int)(len - 14 + 2 - strlen(sigmsg[WTERMSIG(pn->statusp)])), "");
	    else
		fprintf(stderr, "%-*s", len + 2, sigmsg[WTERMSIG(pn->statusp)]);
	    for (; pn != qn; pn = pn->next)
		fprintf(stderr, (pn->next) ? "%s | " : "%s", pn->text);
	    putc('\n', stderr);
	    fline = 0;
	}
    } else if (doputnl && interact)
	putc('\n', stderr);
    fflush(stderr);

/* print "(pwd now: foo)" messages */

    if (interact && job == thisjob && strcmp(jn->pwd, pwd)) {
	printf("(pwd now: ");
	printdir(pwd);
	printf(")\n");
	fflush(stdout);
    }
/* delete job if done */

    if (jn->stat & STAT_DONE) {
	if ((jn->stat & STAT_TIMED) || (reporttime != -1 && report(jn))) {
	    dumptime(jn);
	}
	deletejob(jn);
	if (job == curjob) {
	    curjob = prevjob;
	    prevjob = job;
	}
	if (job == prevjob)
	    setprevjob();
    } else
	jn->stat &= ~STAT_CHANGED;
}

void deletejob(jn)		/**/
Job jn;
{
    struct process *pn, *nx;
    char *s;

    for (pn = jn->procs; pn; pn = nx) {
	nx = pn->next;
	zfree(pn, sizeof(struct process));
    }
    zsfree(jn->pwd);
    if (jn->filelist) {
	while ((s = (char *)getnode(jn->filelist))) {
	    unlink(s);
	    zsfree(s);
	}
	zfree(jn->filelist, sizeof(struct lklist));
    }
    if (jn->ty)
	zfree(jn->ty, sizeof(struct ttyinfo));

    *jn = zero;
}

/* set the previous job to something reasonable */

void setprevjob()
{				/**/
    int t0;

    for (t0 = MAXJOB - 1; t0; t0--)
	if ((jobtab[t0].stat & STAT_INUSE) && (jobtab[t0].stat & STAT_STOPPED) &&
	    t0 != curjob && t0 != thisjob)
	    break;
    if (!t0)
	for (t0 = MAXJOB - 1; t0; t0--)
	    if ((jobtab[t0].stat & STAT_INUSE) && t0 != curjob && t0 != thisjob)
		break;
    prevjob = (t0) ? t0 : -1;
}

/* initialize a job table entry */

void initjob()
{				/**/
    jobtab[thisjob].pwd = ztrdup(pwd);
    jobtab[thisjob].stat = STAT_INUSE;
    jobtab[thisjob].gleader = 0;
}

/* add a process to the current job */

void addproc(pid, text)		/**/
long pid;
char *text;
{
    struct process *process;

    if (!jobtab[thisjob].gleader)
	jobtab[thisjob].gleader = pid;
    process = (struct process *)zcalloc(sizeof *process);
    process->pid = pid;
    if (text)
	strcpy(process->text, text);
    else
	*process->text = '\0';
    process->next = NULL;
    process->statusp = SP_RUNNING;
    gettimeofday(&process->bgtime, &dummy_tz);
    if (jobtab[thisjob].procs) {
	struct process *n;

	for (n = jobtab[thisjob].procs; n->next; n = n->next);
	process->next = NULL;
	n->next = process;
    } else
	jobtab[thisjob].procs = process;
}

/* determine if it's all right to exec a command without
	forking in last component of subshells; it's not ok if we have files
	to delete */

int execok()
{				/**/
    Job jn;

    if (!exiting)
	return 0;
    for (jn = jobtab + 1; jn != jobtab + MAXJOB; jn++)
	if (jn->stat && jn->filelist)
	    return 0;
    return 1;

}

void waitforpid(pid)		/**/
long pid;
{
/* blockchld() around this loop in case #ifndef WNOHANG */
    blockchld();		/* unblocked in chldsuspend() */
    while (!errflag && (kill(pid, 0) >= 0 || errno != ESRCH)) {
	chldsuspend(SIGINT);
	blockchld();
    }
    unblockchld();
}

/* wait for a job to finish */

void waitjob(job, sig)		/**/
int job;
int sig;
{
    Job jn = jobtab + job;

    blockchld();		/* unblocked during chldsuspend() */
    if (jn->procs) {		/* if any forks were done */
	jn->stat |= STAT_LOCKED;
	if (jn->stat & STAT_CHANGED)
	    printjob(jobtab + job, !!isset(LONGLISTJOBS));
	while (!errflag && jn->stat &&
	       !(jn->stat & STAT_DONE) &&
	       !(interact && (jn->stat & STAT_STOPPED))) {
	    chldsuspend(sig);
	    blockchld();
	}
    } else
	deletejob(jobtab + job);
    unblockchld();
}

/* wait for running job to finish */

void waitjobs()
{				/**/
    waitjob(thisjob, 0);
    thisjob = -1;
}

/* clear job table when entering subshells */

void clearjobtab()
{				/**/
    int t0;

    for (t0 = 1; t0 != MAXJOB; t0++) {
	if (jobtab[t0].pwd)
	    zsfree(jobtab[t0].pwd);
	if (jobtab[t0].ty)
	    zfree(jobtab[t0].ty, sizeof(struct ttyinfo));

	jobtab[t0] = zero;
    }
}

/* get a free entry in the job table to use */

int getfreejob()
{				/**/
    int t0;

    for (t0 = 1; t0 != MAXJOB; t0++)
	if (!jobtab[t0].stat) {
	    jobtab[t0].stat |= STAT_INUSE;
	    return t0;
	}
    zerr("job table full or recursion limit exceeded", NULL, 0);
    return -1;
}

/* print pids for & */

void spawnjob()
{				/**/
    struct process *pn;

    if (!subsh) {
	if (curjob == -1 || !(jobtab[curjob].stat & STAT_STOPPED)) {
	    curjob = thisjob;
	    setprevjob();
	} else if (prevjob == -1 || !(jobtab[prevjob].stat & STAT_STOPPED))
	    prevjob = thisjob;
	if (interact && jobbing && jobtab[thisjob].procs) {
	    fprintf(stderr, "[%d]", thisjob);
	    for (pn = jobtab[thisjob].procs; pn; pn = pn->next)
		fprintf(stderr, " %ld", pn->pid);
	    fprintf(stderr, "\n");
	    fflush(stderr);
	}
    }
    if (!jobtab[thisjob].procs)
	deletejob(jobtab + thisjob);
    else
	jobtab[thisjob].stat |= STAT_LOCKED;
    thisjob = -1;
}

int report(j)			/**/
Job j;
{
    if (!j->procs)
	return 0;
#ifdef HAS_RUSAGE
    return (j->procs->ti.ru.ru_utime.tv_sec + j->procs->ti.ru.ru_stime.tv_sec)
	>= reporttime;
#else
    return (j->procs->ti.ut + j->procs->ti.st) / HZ >= reporttime;
#endif
}

void printtime(real, ti, desc)	/**/
struct timeval *real;
struct timeinfo *ti;
char *desc;
{
    char *s;
    long real100;

#ifdef HAS_RUSAGE
#ifdef sun
    long ticks = 1;
    int pk = getpagesize() / 1024;

#else
    long sec;

#endif
    struct rusage *ru = &ti->ru;

#endif

    if (!desc)
	desc = "";
#ifdef HAS_RUSAGE
#ifdef sun
    ticks = (ru->ru_utime.tv_sec + ru->ru_stime.tv_sec) * HZ +
	(ru->ru_utime.tv_usec + ru->ru_stime.tv_usec) * HZ / 1000000;
    if (!ticks)
	ticks = 1;
#else
    sec = ru->ru_utime.tv_sec + ru->ru_stime.tv_sec;
    if (!sec)
	sec = 1;
#endif
#endif
    for (s = timefmt; *s; s++)
	if (*s == '%')
	    switch (s++, *s) {
	    case 'E':
		fprintf(stderr, "%ld.%03lds",
			(long)real->tv_sec, (long)real->tv_usec / 1000);
		break;
#ifndef HAS_RUSAGE
	    case 'U':
		fprintf(stderr, "%ld.%03lds",
			ti->ut / HZ, ti->ut * 1000 / HZ % 1000);
		break;
	    case 'S':
		fprintf(stderr, "%ld.%03lds",
			ti->st / HZ, ti->st * 1000 / HZ % 1000);
		break;
	    case 'P':
		if (real->tv_sec > 21000) {
		    real100 = (real->tv_sec + 99) / 100;
		    fprintf(stderr, "%d%%",
			    (int)((ti->ut + ti->st) / HZ) / real100);
		} else {
		    if ((real100 = real->tv_sec * 1000 + real->tv_usec / 1000))
			fprintf(stderr, "%d%%",
				(int)(100000 * ((ti->ut + ti->st) / HZ)) / real100);
		}
		break;
#else
	    case 'U':
		fprintf(stderr, "%ld.%03lds",
			(long)ru->ru_utime.tv_sec,
			(long)ru->ru_utime.tv_usec / 1000);
		break;
	    case 'S':
		fprintf(stderr, "%ld.%03lds",
			(long)ru->ru_stime.tv_sec,
			(long)ru->ru_stime.tv_usec / 1000);
		break;
	    case 'P':
		if (real->tv_sec > 21000) {
		    real100 = (real->tv_sec + 99) / 100;
		    fprintf(stderr, "%ld%%",
			    (ru->ru_utime.tv_sec + ru->ru_stime.tv_sec) / real100);
		} else {
		    if ((real100 = real->tv_sec * 1000 + real->tv_usec / 1000))
			fprintf(stderr, "%ld%%",
				(100000 * (ru->ru_utime.tv_sec + ru->ru_stime.tv_sec)
				 +
				 (ru->ru_utime.tv_usec + ru->ru_stime.tv_usec) / 10)
				/ real100);
		}
		break;
	    case 'W':
		fprintf(stderr, "%ld", ru->ru_nswap);
		break;
#ifdef sun
	    case 'K':
	    case 'D':
		fprintf(stderr, "%ld", ru->ru_idrss / ticks * pk);
		break;
	    case 'M':
		fprintf(stderr, "%ld", ru->ru_maxrss * pk);
		break;
#else
	    case 'X':
		fprintf(stderr, "%ld", ru->ru_ixrss / sec);
		break;
	    case 'D':
		fprintf(stderr, "%ld",
			(ru->ru_idrss + ru->ru_isrss) / sec);
		break;
	    case 'K':
		fprintf(stderr, "%ld",
			(ru->ru_ixrss + ru->ru_idrss + ru->ru_isrss) / sec);
		break;
	    case 'M':
		fprintf(stderr, "%ld", ru->ru_maxrss / 1024);
		break;
#endif
	    case 'F':
		fprintf(stderr, "%ld", ru->ru_majflt);
		break;
	    case 'R':
		fprintf(stderr, "%ld", ru->ru_minflt);
		break;
	    case 'I':
		fprintf(stderr, "%ld", ru->ru_inblock);
		break;
	    case 'O':
		fprintf(stderr, "%ld", ru->ru_oublock);
		break;
	    case 'r':
		fprintf(stderr, "%ld", ru->ru_msgrcv);
		break;
	    case 's':
		fprintf(stderr, "%ld", ru->ru_msgsnd);
		break;
	    case 'k':
		fprintf(stderr, "%ld", ru->ru_nsignals);
		break;
	    case 'w':
		fprintf(stderr, "%ld", ru->ru_nvcsw);
		break;
	    case 'c':
		fprintf(stderr, "%ld", ru->ru_nivcsw);
		break;
#endif
	    case 'J':
		fprintf(stderr, "%s", desc);
		break;
	    default:
		fprintf(stderr, "%%%c", *s);
		break;
	} else
	    putc(*s, stderr);
    putc('\n', stderr);
    fflush(stderr);
}

void dumptime(jn)		/**/
Job jn;
{
    struct process *pn;

    if (!jn->procs)
	return;
    for (pn = jn->procs; pn; pn = pn->next)
	printtime(dtime(&dtimeval, &pn->bgtime, &pn->endtime), &pn->ti, pn->text);
}

void shelltime()
{				/**/
    struct timeinfo ti;

#ifdef HAS_RUSAGE
    struct rusage ru;

    getrusage(RUSAGE_SELF, &ru);
    memcpy(&ti.ru, &ru, sizeof(ru));
    gettimeofday(&now, &dummy_tz);
    printtime(dtime(&dtimeval, &shtimer, &now), &ti, "shell");

    getrusage(RUSAGE_CHILDREN, &ru);
    memcpy(&ti.ru, &ru, sizeof(ru));
    printtime(dtime(&dtimeval, &shtimer, &now), &ti, "children");
#else
    struct tms buf;

    times(&buf);
    ti.ut = buf.tms_utime;
    ti.st = buf.tms_stime;
    gettimeofday(&now, &dummy_tz);
    printtime(dtime(&dtimeval, &shtimer, &now), &ti, "shell");
    ti.ut = buf.tms_cutime;
    ti.st = buf.tms_cstime;
    printtime(dtime(&dtimeval, &shtimer, &now), &ti, "children");
#endif
}

/* SIGHUP any jobs left running */

void killrunjobs()
{				/**/
    int t0, killed = 0;

    if (isset(NOHUP))
	return;
    for (t0 = 1; t0 != MAXJOB; t0++)
	if ((from_sig || t0 != thisjob) && (jobtab[t0].stat & STAT_LOCKED) &&
	    !(jobtab[t0].stat & STAT_STOPPED)) {
	    if (killpg(jobtab[t0].gleader, SIGHUP) != -1)
		killed++;
	}
    if (killed)
	zerr("warning: %d jobs SIGHUPed", NULL, killed);
}

/* check to see if user has jobs running/stopped */

void checkjobs()
{				/**/
    int t0;

    scanjobs();
    for (t0 = 1; t0 != MAXJOB; t0++)
	if (t0 != thisjob && jobtab[t0].stat & STAT_LOCKED)
	    break;
    if (t0 != MAXJOB) {
	if (jobtab[t0].stat & STAT_STOPPED) {
#ifdef USE_SUSPENDED
	    zerr("you have suspended jobs.", NULL, 0);
#else
	    zerr("you have stopped jobs.", NULL, 0);
#endif
	} else
	    zerr("you have running jobs.", NULL, 0);
	stopmsg = 1;
    }
}

/* send a signal to a job (simply involves kill if monitoring is on) */

int killjb(jn, sig)		/**/
Job jn;
int sig;
{
    struct process *pn;
    int err = 0;

    if (jobbing)
	return (killpg(jn->gleader, sig));
    for (pn = jn->procs; pn; pn = pn->next)
	if ((err = kill(pn->pid, sig)) == -1 && errno != ESRCH)
	    return -1;
    return err;
}
