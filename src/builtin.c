/*
 *
 * builtin.c - builtin commands
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

#define makecond() allocnode(N_COND)

/* builtin flags */

#define BINF_PLUSOPTS	1	/* +xyz legal */
#define BINF_R		2	/* this is r (fc -e -) */
#define BINF_PRINTOPTS	4
#define BINF_SETOPTS	8
#define BINF_FCOPTS	16
#define BINF_TYPEOPT	32
#define BINF_TYPEOPTS	(BINF_TYPEOPT|BINF_PLUSOPTS)
#define BINF_ECHOPTS	64

/* builtin funcs */

#define BIN_TYPESET 0
#define BIN_BG 1
#define BIN_FG 2
#define BIN_JOBS 3
#define BIN_WAIT 4
#define BIN_DISOWN 5
#define BIN_BREAK 6
#define BIN_CONTINUE 7
#define BIN_EXIT 8
#define BIN_RETURN 9
#define BIN_SHIFT 10
#define BIN_CD 11
#define BIN_POPD 12
#define BIN_PUSHD 13
#define BIN_PRINT 14
#define BIN_EVAL 15
#define BIN_SCHED 16
#define BIN_FC 17
#define BIN_PUSHLINE 18
#define BIN_LOGOUT 19
#define BIN_BUILTIN 20
#define BIN_TEST 21
#define BIN_BRACKET 22
#define BIN_EXPORT 23
#define BIN_TRUE 24
#define BIN_FALSE 25
#define BIN_ECHO 26

struct bincmd {
    char *name;
    int (*handlerfunc) DCLPROTO((char *, char **, char *, int));
    int minargs;		/* min # of args */
    int maxargs;		/* max # of args, or -1 for no limit */
    int flags;			/* BINF_flags (see above) */
    int funcid;			/* xbins (see above) for overloaded handlerfuncs */
    char *optstr;		/* string of legal options */
    char *defopts;		/* options set by default for overloaded handlerfuncs */
};

static char *auxdata;
static int auxlen;
static int showflag = 0, showflag2 = 0;

#define NULLBINCMD ((int (*) DCLPROTO((char *,char **,char *,int))) 0)

struct bincmd builtins[] =
{
    {"[", bin_test, 0, -1, 0, BIN_BRACKET, NULL, NULL},
    {".", bin_dot, 1, -1, 0, 0, NULL, NULL},
    {":", bin_colon, 0, -1, 0, BIN_TRUE, NULL, NULL},
    {"alias", bin_alias, 0, -1, 0, 0, "gamr", NULL},
    {"autoload", bin_typeset, 0, -1, BINF_TYPEOPTS, 0, "txv", "fu"},
    {"bg", bin_fg, 0, -1, 0, BIN_BG, NULL, NULL},
    {"bindkey", bin_bindkey, 0, -1, 0, 0, "asvemdr", NULL},
    {"break", bin_break, 0, 1, 0, BIN_BREAK, NULL, NULL},
    {"builtin", NULLBINCMD, 0, 0, 0, BIN_BUILTIN, NULL, NULL},
    {"bye", bin_break, 0, 1, 0, BIN_EXIT, NULL, NULL},
    {"cd", bin_cd, 0, 2, 0, BIN_CD, NULL, NULL},
    {"chdir", bin_cd, 0, 2, 0, BIN_CD, NULL, NULL},
    {"compctl", bin_compctl, 0, -1, 0, 0, NULL, NULL},
    {"continue", bin_break, 0, 1, 0, BIN_CONTINUE, NULL, NULL},
    {"declare", bin_typeset, 0, -1, BINF_TYPEOPTS, 0, "LRZfilrtux", NULL},
    {"dirs", bin_dirs, 0, -1, 0, 0, "v", NULL},
    {"disable", bin_disable, 0, -1, 0, 0, "-m", NULL},
    {"disown", bin_fg, 1, -1, 0, BIN_DISOWN, NULL, NULL},
    {"echo", bin_print, 0, -1, BINF_PRINTOPTS | BINF_ECHOPTS, BIN_ECHO, "n", "-"},
    {"echotc", bin_echotc, 1, -1, 0, 0, NULL, NULL},
    {"enable", bin_enable, 0, -1, 0, 0, "m", NULL},
    {"eval", bin_eval, 0, -1, 0, BIN_EVAL, NULL, NULL},
    {"exit", bin_break, 0, 1, 0, BIN_EXIT, NULL, NULL},
    {"export", bin_typeset, 0, -1, BINF_TYPEOPTS, BIN_EXPORT, "LRZfilrtu", "x"},
    {"false", bin_colon, 0, -1, 0, BIN_FALSE, NULL, NULL},
    {"fc", bin_fc, 0, -1, BINF_FCOPTS, BIN_FC, "nlreIRWAdDfEm", NULL},
    {"fg", bin_fg, 0, -1, 0, BIN_FG, NULL, NULL},
    {"functions", bin_typeset, 0, -1, BINF_TYPEOPTS, 0, "tum", "f"},
    {"getln", bin_read, 0, -1, 0, 0, "ecnAlE", "zr"},
    {"getopts", bin_getopts, 2, -1, 0, 0, NULL, NULL},
    {"hash", bin_hash, 2, 2, 0, 0, NULL, NULL},
    {"history", bin_fc, 0, -1, 0, BIN_FC, "nrdDfEm", "l"},
    {"integer", bin_typeset, 0, -1, BINF_TYPEOPTS, 0, "lrtux", "i"},
    {"jobs", bin_fg, 0, -1, 0, BIN_JOBS, "lpZrs", NULL},
    {"kill", bin_kill, 0, -1, 0, 0, NULL, NULL},
    {"let", bin_let, 1, -1, 0, 0, NULL, NULL},
    {"limit", bin_limit, 0, -1, 0, 0, "sh", NULL},
    {"local", bin_typeset, 0, -1, BINF_TYPEOPTS, 0, "LRZilrtu", NULL},
    {"log", bin_log, 0, 0, 0, 0, NULL, NULL},
    {"logout", bin_break, 0, 1, 0, BIN_LOGOUT, NULL, NULL},

#if defined(MEM_DEBUG) && defined(USE_ZSH_MALLOC)
    {"mem", bin_mem, 0, 0, 0, 0, "v", NULL},
#endif

    {"popd", bin_cd, 0, 2, 0, BIN_POPD, NULL, NULL},
    {"print", bin_print, 0, -1, BINF_PRINTOPTS, BIN_PRINT, "RDPnrslzNu0123456789pioOc-", NULL},
    {"pushd", bin_cd, 0, 2, 0, BIN_PUSHD, NULL, NULL},
    {"pushln", bin_print, 0, -1, BINF_PRINTOPTS, BIN_PRINT, NULL, "-nz"},
    {"pwd", bin_pwd, 0, 0, 0, 0, NULL, NULL},
    {"r", bin_fc, 0, -1, BINF_R, BIN_FC, "nrl", NULL},
    {"read", bin_read, 0, -1, 0, 0, "rzu0123456789pkqecnAlE", NULL},
    {"readonly", bin_typeset, 0, -1, BINF_TYPEOPTS, 0, "LRZfiltux", "r"},
    {"rehash", bin_rehash, 0, 0, 0, 0, "f", NULL},
    {"return", bin_break, 0, 1, 0, BIN_RETURN, NULL, NULL},
    {"sched", bin_sched, 0, -1, 0, 0, NULL, NULL},
    {"set", bin_set, 0, -1, BINF_SETOPTS | BINF_PLUSOPTS, 0, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZaefghjklmnosuvwxy", NULL},
    {"setopt", bin_setopt, 0, -1, BINF_PLUSOPTS, 0, "0123456789BCDEFGHIJKLMNOPQRSTUVWXYZaefghjklmnosuvwxy", NULL},
    {"shift", bin_break, 0, -1, 0, BIN_SHIFT, NULL, NULL},
    {"source", bin_dot, 1, -1, 0, 0, NULL, NULL},
    {"suspend", bin_suspend, 0, 0, 0, 0, "f", NULL},
    {"test", bin_test, 0, -1, 0, BIN_TEST, NULL, NULL},
    {"ttyctl", bin_ttyctl, 0, 0, 0, 0, "fu", NULL},
    {"times", bin_times, 0, 0, 0, 0, NULL, NULL},
    {"trap", bin_trap, 0, -1, 0, 0, NULL, NULL},
    {"true", bin_colon, 0, -1, 0, BIN_TRUE, NULL, NULL},
    {"type", bin_whence, 0, -1, 0, 0, "pfam", "v"},
    {"typeset", bin_typeset, 0, -1, BINF_TYPEOPTS, 0, "LRZfilrtuxm", NULL},
    {"ulimit", bin_ulimit, 0, 1, 0, 0, "Hacdflmnopstv", NULL},
    {"umask", bin_umask, 0, 1, 0, 0, NULL, NULL},
    {"unalias", bin_unalias, 1, -1, 0, 0, "m", NULL},
    {"unfunction", bin_unhash, 1, -1, 0, 0, "m", NULL},
    {"unhash", bin_unhash, 1, -1, 0, 0, "m", NULL},
    {"unlimit", bin_unlimit, 0, -1, 0, 0, "h", NULL},
    {"unset", bin_unset, 1, -1, 0, 0, "m", NULL},
    {"unsetopt", bin_setopt, 0, -1, BINF_PLUSOPTS, 1, "0123456789BCDEFGHIJKLMNOPQRSTUWXYZabefghjklmnosuvwxy", NULL},
    {"vared", bin_vared, 1, 6, 0, 0, NULL, NULL},
    {"wait", bin_fg, 0, -1, 0, BIN_WAIT, NULL, NULL},
    {"whence", bin_whence, 0, -1, 0, 0, "pvcfam", NULL},
    {"which", bin_whence, 0, -1, 0, 0, "pam", "c"},
    {NULL, NULLBINCMD, 0, 0, 0, 0, NULL, NULL}
};

/* print options */

SPROTO(void prtopt, (int set));

static void prtopt(set)
int set;
{
    struct option *opp;

    if (isset(KSHOPTIONPRINT)) {
	printf("Current option settings\n");
	for (opp = optns; opp->name; opp++)
	    printf("%-20s%s\n", opp->name, isset(opp->id) ? "on" : "off");
    } else
	for (opp = optns; opp->name; opp++)
	    if ((!set) == (!isset(opp->id)))
		puts(opp->name);
}

/* add builtins to the command hash table */

void addbuiltins()
{				/**/
    struct cmdnam *c;
    struct bincmd *b;
    int t0;

    for (t0 = 0, b = builtins; b->name; b++, t0++) {
	c = (Cmdnam) zcalloc(sizeof *c);
	c->flags = BUILTIN;
	c->u.binnum = t0;
	addhnode(ztrdup(b->name), c, cmdnamtab, freecmdnam);
    }
}

/* enable */

int bin_enable(name, argv, ops, whocares)	/**/
char *name;
char **argv;
char *ops;
int whocares;
{
    struct cmdnam *c;
    struct bincmd *b;
    int t0, ret = 0;
    Comp com;

    if (!*argv) {
	listhtable(cmdnamtab, (HFunc) penabledcmd);
	return 0;
    }
    for (; *argv; argv++) {
	if (ops['m']) {
	    tokenize(*argv);
	    if (!(com = parsereg(*argv))) {
		ret = 1;
		untokenize(*argv);
		zwarnnam(name, "bad pattern : %s", *argv, 0);
		continue;
	    }
	    for (t0 = 0, b = builtins; b->name; b++, t0++)
		if (domatch(b->name, com, 0)) {
		    c = (Cmdnam) zcalloc(sizeof *c);
		    c->flags = BUILTIN;
		    c->u.binnum = t0;
		    addhnode(ztrdup(b->name), c, cmdnamtab, freecmdnam);
		}
	} else {
	    for (t0 = 0, b = builtins; b->name; b++, t0++)
		if (!strcmp(*argv, b->name))
		    break;
	    if (!b->name) {
		zerrnam(name, "no such builtin: %s", *argv, 0);
		ret = 1;
	    } else {
		c = (Cmdnam) zcalloc(sizeof *c);
		c->flags = BUILTIN;
		c->u.binnum = t0;
		addhnode(ztrdup(b->name), c, cmdnamtab, freecmdnam);
	    }
	}
    }
    return ret;
}

/* :, true, false */

int bin_colon(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    return (func == BIN_FALSE);
}

/* break, bye, continue, exit, logout, return, shift */

int bin_break(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    int num = lastval, nump = 0;

    if (*argv && (**argv == '-' || idigit(**argv))) {
	num = matheval(*argv++);
	nump = 1;
    }
    if ((func == BIN_BREAK || func == BIN_CONTINUE) && !loops) {
	if (func == BIN_CONTINUE)
	    zerrnam(name, "not in loop", NULL, 0);
	return 1;
    }
    switch (func) {
    case BIN_CONTINUE:
	contflag = 1;
    case BIN_BREAK:
	breaks = nump ? num : 1;
	if (breaks > loops)
	    breaks = loops;
	break;
    case BIN_RETURN:
	if (isset(INTERACTIVE) || locallevel || sourcelevel) {
	    retflag = 1;
	    breaks = loops;
	    lastval = num;
	    if (trapreturn)
		trapreturn = lastval;
	    return lastval;
	}			/* else fall through */
    case BIN_LOGOUT:
	if (func == BIN_LOGOUT && !islogin) {
	    zerrnam(name, "not login shell", NULL, 0);
	    return 1;
	}
    case BIN_EXIT:
	zexit(num);
	break;
    case BIN_SHIFT:
	{
	    char **s;
	    int l;

	    if (!nump)
		num = 1;
	    if (num < 0) {
		zerrnam(name, "bad number", NULL, 0);
		return 1;
	    }
	    if (*argv) {
		for (; *argv; argv++)
		    if ((s = getaparam(*argv))) {
			if (num < (l = arrlen(s)))
			    l = num;
			permalloc();
			s = arrdup(s + l);
			heapalloc();
			setaparam(*argv, s);
		    }
	    } else {
		if (num > arrlen(pparams))
		    num = arrlen(pparams);
		permalloc();
		s = arrdup(pparams + num);
		heapalloc();
		freearray(pparams);
		pparams = s;
	    }
	    break;
	}
    }
    return 0;
}

/* bg, disown, fg, jobs, wait */

int bin_fg(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    int job, lng, firstjob = -1, retval = 0;

    if (ops['Z']) {
	if (*argv)
	    strcpy(hackzero, *argv);
	return 0;
    }
    lng = (ops['l']) ? 1 : (ops['p']) ? 2 : 0;
    if ((func == BIN_FG || func == BIN_BG) && !jobbing) {
	zwarnnam(name, "no job control in this shell.", NULL, 0);
	return 1;
    }
    if (unset(NOTIFY))
	scanjobs();
    if (curjob != -1 && !(jobtab[curjob].stat & STAT_INUSE)) {
	curjob = prevjob;
	setprevjob();
	if (curjob != -1 && !(jobtab[curjob].stat & STAT_INUSE))
	    curjob = prevjob;
	setprevjob();
    }
    if (func == BIN_JOBS)
	stopmsg = 2;
    if (!*argv)
	if (func == BIN_FG || func == BIN_BG) {
	    if (curjob == -1 || curjob == thisjob) {
		zwarnnam(name, "no current job", NULL, 0);
		return 1;
	    }
	    firstjob = curjob;
	} else if (func == BIN_JOBS) {
	    for (job = 0; job != MAXJOB; job++)
		if (job != thisjob && jobtab[job].stat) {
		    if ((!ops['r'] && !ops['s']) ||
			(ops['r'] && ops['s']) ||
			(ops['r'] && !(jobtab[job].stat & STAT_STOPPED)) ||
			(ops['s'] && jobtab[job].stat & STAT_STOPPED))
			printjob(job + jobtab, lng);
		}
	    return 0;
	} else {
	    for (job = 0; job != MAXJOB; job++)
		if (job != thisjob && jobtab[job].stat)
		    waitjob(job, SIGINT);
	    return 0;
	}
    for (; (firstjob != -1) || *argv; (void)(*argv && argv++)) {
	int stopped, ocj = thisjob;

	if (func == BIN_WAIT && isanum(*argv)) {
	    waitforpid((long)atoi(*argv));
	    retval = lastval2;
	    thisjob = ocj;
	    continue;
	}
	job = (*argv) ? getjob(*argv, name) : firstjob;
	firstjob = -1;
	if (job == -1)
	    break;
	if (!(jobtab[job].stat & STAT_INUSE)) {
	    zwarnnam(name, "no such job: %d", 0, job);
	    return 1;
	}
	switch (func) {
	case BIN_FG:
	case BIN_BG:
	case BIN_WAIT:
	    if ((stopped = (jobtab[job].stat & STAT_STOPPED)))
		makerunning(jobtab + job);
	    else if (func == BIN_BG) {
		zwarnnam(name, "job already in background", NULL, 0);
		thisjob = ocj;
		return 1;
	    }
	    if (curjob == job) {
		curjob = prevjob;
		prevjob = (func == BIN_BG) ? -1 : job;
	    }
	    if (prevjob == job)
		prevjob = -1;
	    if (prevjob == -1)
		setprevjob();
	    if (curjob == -1) {
		curjob = prevjob;
		setprevjob();
	    }
	    if (func != BIN_WAIT)
		printjob(jobtab + job, (stopped) ? -1 : 0);
	    if (func != BIN_BG) {
		if (strcmp(jobtab[job].pwd, pwd)) {
		    printf("(pwd : ");
		    printdir(jobtab[job].pwd);
		    printf(")\n");
		}
		fflush(stdout);
		if (func != BIN_WAIT) {
		    thisjob = job;
		    attachtty(jobtab[job].gleader);
		}
	    }
	    if (stopped) {
		if (func != BIN_BG && jobtab[job].ty)
		    settyinfo(jobtab[job].ty);
		killpg(jobtab[job].gleader, SIGCONT);
	    }
	    if (func == BIN_WAIT)
	        waitjob(job, SIGINT);
	    if (func != BIN_BG) {
		waitjobs();
		retval = lastval2;
	    }
	    break;
	case BIN_JOBS:
	    printjob(job + jobtab, lng);
	    break;
	case BIN_DISOWN:
	    {
		static struct job zero;

		jobtab[job] = zero;
		break;
	    }
	}
	thisjob = ocj;
    }
    return retval;
}

/* let */

int bin_let(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    long val = 0;

    while (*argv)
	val = matheval(*argv++);
    return !val;
}

/* print the directory stack */

SPROTO(void pdstack, (void));

static void pdstack()
{
    Lknode node;

    printdir(pwd);
    for (node = firstnode(dirstack); node; incnode(node)) {
	putchar(' ');
	printdir(getdata(node));
    }
    putchar('\n');
}

/* exit the shell */

void zexit(val)			/**/
int val;
{
    if (isset(MONITOR))
	if (!stopmsg) {
	    checkjobs();
	    if (stopmsg) {
		stopmsg = 2;
		return;
	    }
	} else
	    killrunjobs();
    if (unset(NORCS) && interact) {
	if (isset(APPENDHISTORY))
	    savehistfile(getsparam("HISTFILE"), 1, 3);
	else
	    savehistfile(getsparam("HISTFILE"), 1, 0);
	if (islogin && !subsh)
	    sourcehome(".zlogout");
    }
    if (sigtrapped[SIGEXIT])
	dotrap(SIGEXIT);
    exit(val);
}

/* identify an option name */

int optlookup(s)		/**/
char *s;
{
    char *t;
    struct option *o;

    t = s = dupstring(s);
    while (*t)
	if (*t == '_')
	    chuck(t);
	else {
	    *t = tulower(*t);
	    t++;
	}
    for (o = optns; o->name; o++)
	if (!strcmp(o->name, s))
	    return (int)o->id;
    return -1;
}

/* setopt, unsetopt */

int bin_setopt(nam, args, ops, isun)	/**/
char *nam;
char **args;
char *ops;
int isun;
{
    struct option *opp;
    int c, match;

    match = ops[(int)'m'];
    ops['m'] = 0;
    if (!ops['@'] && !*args) {
	prtopt(!isun);
	return 0;
    }
    for (opp = optns; opp->name; opp++)
	if (ops[opp->id] == 1 + isun)
	    opts[(int)opp->id] = OPT_SET;
	else if (ops[(int)opp->id] == 2 - isun)
	    opts[(int)opp->id] = OPT_UNSET;
    if (!match)
	while (*args) {
	    c = optlookup(*args++);
	    if (c != -1) {
		if (c == INTERACTIVE)
		    zwarnnam(nam, "can't change option: %s", args[-1], 0);
		else
		    opts[c] = (isun) ? OPT_UNSET : OPT_SET;
	    } else {
		zwarnnam(nam, "no such option: %s", args[-1], 0);
		return 1;
	    }
    } else {
	Comp com;
	struct option *o;

	while (*args) {
	    tokenize(*args);
	    if (!(com = parsereg(*args))) {
		untokenize(*args);
		zwarnnam(nam, "bad pattern : %s", *args, 0);
		continue;
	    }
	    for (o = optns; o->name; o++)
		if (o->id != INTERACTIVE && o->id != MONITOR &&
		    domatch(o->name, com, 0))
		    opts[(int)o->id] = (isun) ? OPT_UNSET : OPT_SET;
	    args++;
	}
    }
    return 0;
}

/* execute func on each member of the hash table ht */

static hnamcmp DCLPROTO((struct hashnode ** a, struct hashnode ** b));
static int hnamcmp(a, b)
struct hashnode **a;
struct hashnode **b;
{
    return forstrcmp(&((*a)->nam), &((*b)->nam));
}

void listhtable(ht, func)	/**/
Hashtab ht;
HFunc func;
{
    int t0;
    struct hashnode *hn;

#ifndef HASHORDER

    int nhash;
    struct hashnode **hnsorttab, **htp;

    hnsorttab = (struct hashnode **)zalloc(ht->ct * sizeof(struct hashnode *));

    for (htp = hnsorttab, t0 = ht->hsize - 1; t0 >= 0; t0--)
	for (hn = ht->nodes[t0]; hn; hn = hn->next)
	    *htp++ = hn;

    qsort((vptr) & hnsorttab[0], ht->ct, sizeof(struct hashnode *),
	           (int (*)DCLPROTO((const void *, const void *)))hnamcmp);

    for (htp = hnsorttab, nhash = 0; nhash < ht->ct; nhash++, htp++)
	func((*htp)->nam, (char *)(*htp));

    free(hnsorttab);

#else

    for (t0 = ht->hsize - 1; t0 >= 0; t0--)
	for (hn = ht->nodes[t0]; hn; hn = hn->next)
	    func(hn->nam, (char *)hn);

#endif
}

/* print a shell function (used with listhtable) */

void pshfunc(s, cc)		/**/
char *s;
Cmdnam cc;
{
    char *t;

    if (!(cc->flags & SHFUNC))
	return;
    if (showflag && (cc->flags & showflag2) != showflag2)
	return;
    if (cc->flags & PMFLAG_u)
	printf("undefined ");
    if (cc->flags & PMFLAG_t)
	printf("traced ");
    if (!cc->u.list || !showflag) {
	printf("%s ()\n", s);
	return;
    }
    t = getpermtext((vptr) dupstruct((vptr) cc->u.list));
    printf("%s () {\n\t%s\n}\n", s, t);
    zsfree(t);
}

void penabledcmd(s, cc)		/**/
char *s;
Cmdnam cc;
{
    if (cc->flags & BUILTIN)
	printf("%s\n", s);
}

void pdisabledcmd(s, cc)	/**/
char *s;
Cmdnam cc;
{
    if (cc->flags & DISABLED)
	printf("%s\n", s);
}

void niceprintf(s, f)		/**/
char *s;
FILE *f;
{
    for (; *s; s++) {
	if (isprint(*s))
	    fputc(*s, f);
	else if (*s == '\n') {
	    putc('\\', f);
	    putc('n', f);
	} else {
	    putc('^', f);
	    fputc(*s | 0x40, f);
	}
    }
}

int bin_umask(nam, args, ops, func)	/**/
char *nam;
char **args;
char *ops;
int func;
{
    int um;
    char *s = *args;

    um = umask(0);
    umask(um);
    if (!s) {
	printf("%03o\n", (unsigned)um);
	return 0;
    }
    if (idigit(*s)) {
	um = zstrtol(s, &s, 8);
	if (*s) {
	    zwarnnam(nam, "bad umask", NULL, 0);
	    return 1;
	}
    } else {
	int whomask, umaskop, mask;

	for (;;) {
	    whomask = 0;
	    while (*s == 'u' || *s == 'g' || *s == 'o')
		if (*s == 'u')
		    s++, whomask |= 0100;
		else if (*s == 'g')
		    s++, whomask |= 0010;
		else if (*s == 'o')
		    s++, whomask |= 0001;
	    if (!whomask)
		whomask = 0111;
	    umaskop = (int)*s;
	    if (!(umaskop == '+' || umaskop == '-' || umaskop == '=')) {
		zwarnnam(nam, "bad symbolic mode operator: %c", NULL, umaskop);
		return 1;
	    }
	    mask = 0;
	    while (*++s && *s != ',')
		if (*s == 'r')
		    mask |= 04 * whomask;
		else if (*s == 'w')
		    mask |= 02 * whomask;
		else if (*s == 'x')
		    mask |= whomask;
		else {
		    zwarnnam(nam, "bad symbolic mode permission: %c",
			     NULL, *s);
		    return 1;
		}
	    if (umaskop == '+')
		um &= ~mask;
	    else if (umaskop == '-')
		um |= mask;
	    else		/* umaskop == '=' */
		um = (um | (whomask * 07)) & ~mask;
	    if (*s == ',')
		s++;
	    else
		break;
	}
	if (*s) {
	    zwarnnam(nam, "bad character in symbolic mode: %c", NULL, *s);
	    return 1;
	}
    }
    umask(um);
    return 0;
}

/* type, whence, which */

int bin_whence(nam, argv, ops, func)	/**/
char *nam;
char **argv;
char *ops;
int func;
{
    struct cmdnam *chn;
    struct alias *a;
    int retval = 0;
    int csh = ops[(int)'c'], all = ops[(int)'a'];
    int v = ops['v'] || csh;
    char *cnam;
    int informed;

    for (; *argv; argv++) {
	if (ops['m']) {
	    int i, n;
	    Comp com;

	    tokenize(*argv);
	    if (!(com = parsereg(*argv))) {
		retval = 1;
		untokenize(*argv);
		zwarnnam(nam, "bad pattern : %s", *argv, 0);
		continue;
	    }
	    if (!ops['p']) {
		n = aliastab->hsize;
		for (i = 0; i < n; i++) {
		    for (a = (struct alias *)aliastab->nodes[i]; a;
			 a = (struct alias *)a->next) {
			if (a->nam && domatch(a->nam, com, 0)) {
			    if (a->cmd < 0)
				printf((csh) ? "%s: shell reserved word\n" :
				       (v) ? "%s is a reserved word\n" : "%s\n", a->nam);
			    else if (!v)
				puts(a->text);
			    else if (a->cmd)
				printf((csh) ? "%s: aliased to %s\n" :
				       "%s is an alias for %s\n", a->nam, a->text);
			    else
				printf((csh) ? "%s: globally aliased to %s\n" :
				       "%s is a global alias for %s\n", a->nam, a->text);
			}
		    }
		}
		n = cmdnamtab->hsize;
		for (i = 0; i < n; i++) {
		    for (chn = (struct cmdnam *)cmdnamtab->nodes[i]; chn;
			 chn = (struct cmdnam *)chn->next) {
			if (chn->nam &&
			    (chn->flags & (SHFUNC | BUILTIN)) &&
			    !(chn->flags & EXCMD) &&
			    domatch(chn->nam, com, 0)) {
			    if (chn->flags & SHFUNC) {
				if (csh || ops['f']) {
				    showflag = 1;
				    showflag2 = 0;
				    pshfunc(chn->nam, chn);
				} else {
				    printf((v) ? "%s is a function\n" : "%s\n", chn->nam);
				}
			    } else
				printf((csh) ? "%s: shell built-in command\n" :
				       (v) ? "%s is a shell builtin\n" : "%s\n", chn->nam);
			}
		    }
		}
	    }
	    fullhash();
	    n = cmdnamtab->hsize;
	    for (i = 0; i < n; i++) {
		for (chn = (struct cmdnam *)cmdnamtab->nodes[i]; chn;
		     chn = (struct cmdnam *)chn->next) {
		    if (chn->nam && (chn->flags & EXCMD) &&
			domatch(chn->nam, com, 0)) {
			if (chn->flags & BUILTIN)
			    printf("%s is hashed to %s\n", chn->nam,
				   chn->u.cmd);
			else if (v && !csh)
			    printf("%s is %s/%s\n", chn->nam,
				   chn->u.name ? *(chn->u.name) : "",
				   chn->nam);
			else
			    printf("%s/%s\n", chn->u.name ? *(chn->u.name) : "",
				   chn->nam);
		    }
		}
	    }

	} else {
	    informed = 0;
	    if (!ops['p'] && (a = (Alias) gethnode(*argv, aliastab))) {
		if (a->cmd < 0)
		    printf((csh) ? "%s: shell reserved word\n" :
			   (v) ? "%s is a reserved word\n" : "%s\n", *argv);
		else if (!v)
		    puts(a->text);
		else if (a->cmd)
		    printf((csh) ? "%s: aliased to %s\n" :
			   "%s is an alias for %s\n", *argv, a->text);
		else
		    printf((csh) ? "%s: globally aliased to %s\n" :
			   "%s is a global alias for %s\n", *argv, a->text);
		if (!all)
		    continue;
		informed = 1;
	    }
	    if (!ops['p'] && (chn = (Cmdnam) gethnode(*argv, cmdnamtab)) &&
		(chn->flags & (SHFUNC | BUILTIN))) {
		if (chn->flags & EXCMD)
		    printf("%s is hashed to %s\n", chn->nam, chn->u.cmd);
		else if (chn->flags & SHFUNC) {
		    if (csh || ops['f']) {
			showflag = 1;
			showflag2 = 0;
			pshfunc(*argv, chn);
		    } else {
			printf((v) ? "%s is a function\n" : "%s\n", *argv);
		    }
		} else
		    printf((csh) ? "%s: shell built-in command\n" :
			   (v) ? "%s is a shell builtin\n" : "%s\n", *argv);
		if (!all)
		    continue;
		informed = 1;
	    }
	    if (all) {
		char **pp, buf[MAXPATHLEN], *z;

		for (pp = path; *pp; pp++) {
		    z = buf;
		    strucpy(&z, *pp);
		    *z++ = '/';
		    strcpy(z, *argv);
		    if (iscom(buf)) {
			if (v && !csh)
			    printf("%s is %s\n", *argv, buf);
			else
			    puts(buf);
			informed = 1;
		    }
		}
		if (!informed && v) {
		    printf("%s not found\n", *argv);
		    retval = 1;
		}
	    } else if (!(cnam = findcmd(*argv))) {
		if (v)
		    printf("%s not found\n", *argv);
		retval = 1;
	    } else {
		if (v && !csh)
		    printf("%s is %s\n", *argv, cnam);
		else
		    puts(cnam);
		zsfree(cnam);
	    }
	}
    }
    return retval;
}

/* cd, chdir, pushd, popd */

int doprintdir = 0;		/* set in exec.c (for autocd) */

int bin_cd(nam, argv, ops, func)/**/
char *nam;
char **argv;
char *ops;
int func;
{
    char *dest, *dir;
    struct stat st1, st2;

    doprintdir = (doprintdir == -1);

    if (func == BIN_CD && isset(AUTOPUSHD))
	func = BIN_PUSHD;
    dir = dest = cd_get_dest(nam, argv, ops, func);
    if (!dest)
	return 1;
    dest = cd_do_chdir(nam, dest);
    if (dest != dir)
	zsfree(dir);
    if (!dest)
	return 1;
    cd_new_pwd(func, dest);
    zsfree(dest);

    if (stat(pwd, &st1) < 0) {
	zsfree(pwd);
	pwd = zgetwd();
    } else if (stat(".", &st2) < 0)
	chdir(pwd);
    else if (st1.st_ino != st2.st_ino || st1.st_dev != st2.st_dev)
	if (isset(CHASELINKS)) {
	    zsfree(pwd);
	    pwd = zgetwd();
	} else {
	    chdir(pwd);
	}
    return 0;
}

char *cd_get_dest(nam, argv, ops, func)	/**/
char *nam;
char **argv;
char *ops;
int func;
{
    char *dest = NULL;

    if (!argv[0])
	if (func == BIN_CD || ((func == BIN_PUSHD && isset(PUSHDTOHOME))
			       || empty(dirstack)))
	    dest = ztrdup(home);
	else
	    dest = ztrdup(getnode(dirstack));
    else if (!argv[1]) {
	Lknode n;
	int dd;
	char *end;

	doprintdir++;
	if (argv[0][1] && argv[0][0] == (isset(PUSHDMINUS) ? '-' : '+')) {
	    dd = zstrtol(argv[0] + 1, &end, 10) - 1;
	    if (dd >= 0 && *end == '\0') {
		for (n = firstnode(dirstack); n && dd; dd--, incnode(n));
		if (!n) {
		    zwarnnam(nam, "no such entry in dir stack", NULL, 0);
		    return NULL;
		}
		dest = (char *)remnode(dirstack, n);
	    }
	} else if (argv[0][1] && argv[0][0] == (isset(PUSHDMINUS) ? '+' : '-')) {
	    dd = zstrtol(argv[0] + 1, &end, 10);
	    if (*end == '\0') {
		for (n = lastnode(dirstack); n != (Lknode) dirstack && dd;
		     dd--, n = prevnode(n));
		if (n == (Lknode) dirstack) {
		    zwarnnam(nam, "no such entry in dir stack", NULL, 0);
		    return NULL;
		}
		dest = (char *)remnode(dirstack, n);
	    }
	}
	if (!dest)
	    dest = ztrdup(strcmp(argv[0], "-") ? (doprintdir--, argv[0]) :
			  oldpwd);
    } else {
	char *u;
	int len1, len2, len3;

	if (!(u = ztrstr(pwd, argv[0]))) {
	    zwarnnam(nam, "string not in pwd: %s", argv[0], 0);
	    return NULL;
	}
	len1 = strlen(argv[0]);
	len2 = strlen(argv[1]);
	len3 = u - pwd;
	dest = (char *)zalloc(len3 + len2 + strlen(u + len1) + 1);
	strncpy(dest, pwd, len3);
	strcpy(dest + len3, argv[1]);
	strcat(dest, u + len1);
	doprintdir++;
    }
    return dest;
}

char *cd_do_chdir(cnam, dest)	/**/
char *cnam;
char *dest;
{
    int hasdot = 0, eno = ENOENT;
    int nocdpath = dest[0] == '.' &&
    (dest[1] == '/' || !dest[1] || (dest[1] == '.' &&
				    (dest[2] == '/' || !dest[1])));
    char **pp, *ret;

    if (*dest == '/') {
	if ((ret = cd_try_chdir(NULL, dest)))
	    return ret;
	zwarnnam(cnam, "%e: %s", dest, errno);
	return NULL;
    }
    if (!nocdpath)
	for (pp = cdpath; *pp; pp++)
	    if ((*pp)[0] == '.' && (*pp)[1] == '\0')
		hasdot = 1;
    if (!hasdot || nocdpath) {
	if ((ret = cd_try_chdir(NULL, dest)))
	    return ret;
	if (errno != ENOENT)
	    eno = errno;
    }
    if (!nocdpath)
	for (pp = cdpath; *pp; pp++) {
	    if ((ret = cd_try_chdir(*pp, dest))) {
		if (strcmp(*pp, ".")) {
		    doprintdir++;
		}
		return ret;
	    }
	    if (errno != ENOENT)
		eno = errno;
	}
    if ((ret = cd_able_vars(dest))) {
	if ((ret = cd_try_chdir(NULL, ret))) {
	    doprintdir++;
	    return ret;
	}
	if (errno != ENOENT)
	    eno = errno;
    }
    zwarnnam(cnam, "%e: %s", dest, eno);
    return NULL;
}

char *cd_able_vars(s)		/**/
char *s;
{
    char *rest;

    if (isset(CDABLEVARS)) {
	for (rest = s; *rest && *rest != '/'; rest++);
	s = getnamedir(s, rest - s);

	if (s && *rest)
	    s = dyncat(s, rest);

	return s;
    }
    return NULL;
}

char *cd_try_chdir(pfix, dest)	/**/
char *pfix;
char *dest;
{
    char buf[MAXPATHLEN], buf2[MAXPATHLEN];
    char *s;
    int dotsct;

    if (pfix)
	sprintf(buf, "%s/%s", (!strcmp("/", pfix)) ? "" : pfix, dest);
    else
	strcpy(buf, dest);
    dotsct = fixdir(buf2, buf);
    if (buf2[0] == '/')
	return (chdir(buf) == -1) ? NULL : ztrdup(buf2);
    if (!dotsct) {
	if (chdir(buf) == -1)
	    return NULL;
	if (*buf2)
	    sprintf(buf, "%s/%s", (!strcmp("/", pwd)) ? "" : pwd, buf2);
	else
	    strcpy(buf, pwd);
	return ztrdup(buf);
    }
    strcpy(buf, pwd);
    s = buf + strlen(buf) - 1;
    while (dotsct--)
	while (s != buf)
	    if (*--s == '/')
		break;
    if (s == buf || *buf2)
	s++;
    strcpy(s, buf2);
    if (chdir(buf) != -1 || chdir(dest) != -1)
	return ztrdup(buf);
    return NULL;
}

int fixdir(d, s)		/**/
char *d;
char *s;
{
    int ct = 0;
    char *d0 = d;

#ifdef HAS_RFS
    while (*s == '/' && s[1] == '.' && s[2] == '.') {
	*d++ = '/';
	*d++ = '.';
	*d++ = '.';
	s += 3;
    }
#endif
#ifdef apollo
    if (*s == '/')
	*d++ = *s++;		/*added RBC 18/05/92 */
#endif
    for (;;) {
	if (*s == '/') {
	    *d++ = *s++;
	    while (*s == '/')
		s++;
	}
	if (!*s) {
	    while (d > d0 + 1 && d[-1] == '/')
		d--;
	    *d = '\0';
	    return ct;
	}
	if (s[0] == '.' && s[1] == '.' && (s[2] == '\0' || s[2] == '/')) {
	    if (d > d0 + 1) {
		for (d--; d > d0 + 1 && d[-1] != '/'; d--);
		if (d[-1] != '/')
		    d--;
	    } else
		ct++;
	    s++;
	    while (*++s == '/');
	} else if (s[0] == '.' && (s[1] == '/' || s[1] == '\0')) {
	    while (*++s == '/');
	} else {
	    while (*s != '/' && *s != '\0')
		*d++ = *s++;
	}
    }
}

void cd_new_pwd(func, s)	/**/
int func;
char *s;
{
    Param pm;
    List l;
    char *new_pwd;

    if (isset(CHASELINKS))
	new_pwd = findpwd(s);
    else
	new_pwd = ztrdup(s);
    if (!strcmp(new_pwd, pwd) &&
	(func != BIN_PUSHD || isset(PUSHDIGNOREDUPS))) {
	zsfree(new_pwd);
#ifdef ALWAYS_DO_CD_PROCESSING
	if (unset(PUSHDSILENT) && func != BIN_CD && isset(INTERACTIVE))
	    pdstack();
	if (l = getshfunc("chpwd")) {
	    fflush(stdout);
	    fflush(stderr);
	    doshfuncnoval(dupstruct(l), NULL, 0);
	}
#endif
	return;
    }
    zsfree(oldpwd);
    oldpwd = pwd;
    pwd = new_pwd;
    if ((pm = (Param) gethnode("PWD", paramtab)) &&
	(pm->flags & PMFLAG_x) && pm->env)
	pm->env = replenv(pm->env, pwd);
    if ((pm = (Param) gethnode("OLDPWD", paramtab)) &&
	(pm->flags & PMFLAG_x) && pm->env)
	pm->env = replenv(pm->env, oldpwd);
    if (func == BIN_PUSHD) {
	permalloc();
	if (isset(PUSHDIGNOREDUPS)) {
	    Lknode n;
	    char *nodedata;

	    for (n = firstnode(dirstack); n; incnode(n)) {
		nodedata = (char *)getdata(n);
		if (!strcmp(oldpwd, nodedata) || !strcmp(pwd, nodedata)) {
		    free(remnode(dirstack, n));
		    break;
		}
	    }
	}
	pushnode(dirstack, ztrdup(oldpwd));
	heapalloc();
    }
    if (unset(PUSHDSILENT) && func != BIN_CD && isset(INTERACTIVE))
	pdstack();
    else if (doprintdir)
	printdircr(pwd);
    if ((l = getshfunc("chpwd"))) {
	fflush(stdout);
	fflush(stderr);
	doshfuncnoval(dupstruct(l), NULL, 0);
    }
    if (dirstacksize != -1 && countnodes(dirstack) >= dirstacksize) {
	if (dirstacksize < 2)
	    dirstacksize = 2;
	else
	    free(remnode(dirstack, lastnode(dirstack)));
    }
}

int bin_rehash(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    newcmdnamtab();
    if (ops['f'])
	fullhash();
    return 0;
}

int bin_hash(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    struct cmdnam *chn;

    chn = (Cmdnam) zcalloc(sizeof *chn);
    chn->flags = HASHCMD;
    chn->u.cmd = ztrdup(argv[1]);
    addhnode(ztrdup(argv[0]), chn, cmdnamtab, freecmdnam);
    return 0;
}

/* convert %%, %1, %foo, %?bar? to a job number */

int getjob(s, prog)		/**/
char *s;
char *prog;
{
    int t0, retval;

    if (*s != '%')
	goto jump;
    s++;
    if (*s == '%' || *s == '+' || !*s) {
	if (curjob == -1) {
	    zwarnnam(prog, "no current job", NULL, 0);
	    retval = -1;
	    goto done;
	}
	retval = curjob;
	goto done;
    }
    if (*s == '-') {
	if (prevjob == -1) {
	    zwarnnam(prog, "no previous job", NULL, 0);
	    retval = -1;
	    goto done;
	}
	retval = prevjob;
	goto done;
    }
    if (idigit(*s)) {
	t0 = atoi(s);
	if (t0 && t0 < MAXJOB && jobtab[t0].stat && t0 != thisjob) {
	    retval = t0;
	    goto done;
	}
	zwarnnam(prog, "no such job", NULL, 0);
	retval = -1;
	goto done;
    }
    if (*s == '?') {
	struct process *pn;

	for (t0 = MAXJOB - 1; t0 >= 0; t0--)
	    if (jobtab[t0].stat && t0 != thisjob)
		for (pn = jobtab[t0].procs; pn; pn = pn->next)
		    if (ztrstr(pn->text, s + 1)) {
			retval = t0;
			goto done;
		    }
	zwarnnam(prog, "job not found: %s", s, 0);
	retval = -1;
	goto done;
    }
  jump:
    if ((t0 = findjobnam(s)) != -1) {
	retval = t0;
	goto done;
    }
    zwarnnam(prog, "job not found: %s", s, 0);
    retval = -1;
  done:
    return retval;
}

/* find a job named s */

int findjobnam(s)		/**/
char *s;
{
    int t0;

    for (t0 = MAXJOB - 1; t0 >= 0; t0--)
	if (jobtab[t0].stat && jobtab[t0].procs && t0 != thisjob &&
	    jobtab[t0].procs->text && strpfx(s, jobtab[t0].procs->text))
	    return t0;
    return -1;
}

int isanum(s)			/**/
char *s;
{
    while (*s == '-' || idigit(*s))
	s++;
    return *s == '\0';
}

int bin_kill(nam, argv, ops, func)	/**/
char *nam;
char **argv;
char *ops;
int func;
{
    int sig = SIGTERM;
    int retval = 0;

    if (*argv && **argv == '-') {
	if (idigit((*argv)[1]))
	    sig = atoi(*argv + 1);
	else {
	    if ((*argv)[1] == 'l' && (*argv)[2] == '\0') {
		printf("%s", sigs[1]);
		for (sig = 2; sig <= SIGCOUNT; sig++)
		    printf(" %s", sigs[sig]);
		putchar('\n');
		return 0;
	    }
	    for (sig = 1; sig <= SIGCOUNT; sig++)
		if (!strcmp(sigs[sig], *argv + 1))
		    break;
	    if (sig > SIGCOUNT) {
		zwarnnam(nam, "unknown signal: SIG%s", *argv + 1, 0);
		zwarnnam(nam, "type kill -l for a List of signals", NULL, 0);
		return 1;
	    }
	}
	argv++;
    }
    for (; *argv; argv++) {
	if (**argv == '%') {
	    int p = getjob(*argv, "kill");

	    if (p == -1) {
		retval = 1;
		continue;
	    }
	    if (killjb(jobtab + p, sig) == -1) {
		zwarnnam("kill", "kill failed: %e", NULL, errno);
		retval = 1;
		continue;
	    }
	    if (jobtab[p].stat & STAT_STOPPED) {
		if (sig == SIGCONT)
		    jobtab[p].stat &= ~STAT_STOPPED;
		if (sig != SIGKILL && sig != SIGCONT && sig != SIGTSTP
		    && sig != SIGTTOU && sig != SIGTTIN && sig != SIGSTOP)
		    killjb(jobtab + p, SIGCONT);
	    }
	} else if (!isanum(*argv)) {
	    zwarnnam("kill", "illegal pid: %s", *argv, 0);
	    retval = 1;
	} else if (kill(atoi(*argv), sig) == -1) {
	    zwarnnam("kill", "kill failed: %e", NULL, errno);
	    retval = 1;
	}
    }
    return retval;
}

#ifdef RLIM_INFINITY
static char *recs[] =
{
    "cputime", "filesize", "datasize", "stacksize", "coredumpsize",
#ifdef RLIMIT_RSS
#ifdef RLIMIT_MEMLOCK
    "memoryuse",
#else
    "resident",
#endif				/* RLIMIT_MEMLOCK */
#endif				/* RLIMIT_RSS */
#ifdef RLIMIT_MEMLOCK
    "memorylocked",
#endif
#ifdef RLIMIT_NPROC
    "maxproc",
#endif
#ifdef RLIMIT_OFILE
    "openfiles",
#endif
#ifdef RLIMIT_NOFILE
    "descriptors",
#endif
#ifdef RLIMIT_VMEM
    "vmemorysize"
#endif
};
#endif

int bin_limit(nam, argv, ops, func)	/**/
char *nam;
char **argv;
char *ops;
int func;
{
#ifndef RLIM_INFINITY
    zwarnnam(nam, "not available on this system", NULL, 0);
    return 1;
#else
    char *s;
    int hard = ops['h'], t0, lim;
    long val;

    if (ops['s']) {
	if (*argv)
	    zwarnnam(nam, "arguments after -s ignored", NULL, 0);
	for (t0 = 0; t0 != RLIM_NLIMITS; t0++)
	    if (setrlimit(t0, limits + t0) < 0)
		zwarnnam(nam, "setrlimit failed: %e", NULL, errno);
	return 0;
    }
    if (!*argv) {
	showlimits(hard, -1);
	return 0;
    }
    while ((s = *argv++)) {
	for (lim = -1, t0 = 0; t0 != RLIM_NLIMITS; t0++)
	    if (!strncmp(recs[t0], s, strlen(s))) {
		if (lim != -1)
		    lim = -2;
		else
		    lim = t0;
	    }
	if (lim < 0) {
	    zwarnnam("limit",
		     (lim == -2) ? "ambiguous resource specification: %s"
		     : "no such resource: %s", s, 0);
	    return 1;
	}
	if (!(s = *argv++)) {
	    showlimits(hard, lim);
	    return 0;
	}
	if (!lim) {
	    val = zstrtol(s, &s, 10);
	    if (*s)
		if ((*s == 'h' || *s == 'H') && !s[1])
		    val *= 3600L;
		else if ((*s == 'm' || *s == 'M') && !s[1])
		    val *= 60L;
		else if (*s == ':')
		    val = val * 60 + zstrtol(s + 1, &s, 10);
		else {
		    zwarnnam("limit", "unknown scaling factor: %s", s, 0);
		    return 1;
		}
	}
#ifdef RLIMIT_NPROC
	else if (lim == RLIMIT_NPROC)
	    val = zstrtol(s, &s, 10);
#endif
#ifdef RLIMIT_OFILE
	else if (lim == RLIMIT_OFILE)
	    val = zstrtol(s, &s, 10);
#endif
#ifdef RLIMIT_NOFILE
	else if (lim == RLIMIT_NOFILE)
	    val = zstrtol(s, &s, 10);
#endif
	else {
	    val = zstrtol(s, &s, 10);
	    if (!*s || ((*s == 'k' || *s == 'K') && !s[1]))
		val *= 1024L;
	    else if ((*s == 'M' || *s == 'm') && !s[1])
		val *= 1024L * 1024;
	    else {
		zwarnnam("limit", "unknown scaling factor: %s", s, 0);
		return 1;
	    }
	}
	if (hard)
	    if (val > limits[lim].rlim_max && geteuid()) {
		zwarnnam("limit", "can't raise hard limits", NULL, 0);
		return 1;
	    } else {
		limits[lim].rlim_max = val;
		if (limits[lim].rlim_max < limits[lim].rlim_cur)
		    limits[lim].rlim_cur = limits[lim].rlim_max;
	} else if (val > limits[lim].rlim_max) {
	    zwarnnam("limit", "limit exceeds hard limit", NULL, 0);
	    return 1;
	} else
	    limits[lim].rlim_cur = val;
    }
    return 0;
#endif
}

int bin_unlimit(nam, argv, ops, func)	/**/
char *nam;
char **argv;
char *ops;
int func;
{
#ifndef RLIM_INFINITY
    zwarnnam(nam, "not available on this system", NULL, 0);
    return 1;
#else
    int hard = ops['h'], t0, lim;

    if (hard && geteuid()) {
	zwarnnam(nam, "can't remove hard limits", NULL, 0);
	return 1;
    }
    if (!*argv) {
	for (t0 = 0; t0 != RLIM_NLIMITS; t0++) {
	    if (hard)
		limits[t0].rlim_max = RLIM_INFINITY;
	    else
		limits[t0].rlim_cur = limits[t0].rlim_max;
	}
	return 0;
    }
    for (; *argv; argv++) {
	for (lim = -1, t0 = 0; t0 != RLIM_NLIMITS; t0++)
	    if (!strncmp(recs[t0], *argv, strlen(*argv))) {
		if (lim != -1)
		    lim = -2;
		else
		    lim = t0;
	    }
	if (lim < 0) {
	    zwarnnam(nam,
		     (lim == -2) ? "ambiguous resource specification: %s"
		     : "no such resource: %s", *argv, 0);
	    return 1;
	}
	if (hard)
	    limits[lim].rlim_max = RLIM_INFINITY;
	else
	    limits[lim].rlim_cur = limits[lim].rlim_max;
    }
    return 0;
#endif
}

#ifdef RLIM_INFINITY
void showlimits(hard, lim)	/**/
int hard;
int lim;
{
    int t0;
    RLIM_TYPE val;

    for (t0 = 0; t0 != RLIM_NLIMITS; t0++)
	if (t0 == lim || lim == -1) {
	    printf("%-16s", recs[t0]);
	    val = (hard) ? limits[t0].rlim_max : limits[t0].rlim_cur;
	    if (val == RLIM_INFINITY)
		printf("unlimited\n");
	    else if (!t0)
		printf("%d:%02d:%02d\n", (int)(val / 3600),
		       (int)(val / 60) % 60, (int)(val % 60));
#ifdef RLIMIT_NPROC
	    else if (t0 == RLIMIT_NPROC)
		printf("%d\n", (int)val);
#endif
#ifdef RLIMIT_OFILE
	    else if (t0 == RLIMIT_OFILE)
		printf("%d\n", (int)val);
#endif
#ifdef RLIMIT_NOFILE
	    else if (t0 == RLIMIT_NOFILE)
		printf("%d\n", (int)val);
#endif
	    else if (val >= 1024L * 1024L)
		printf("%ldMb\n", val / (1024L * 1024L));
	    else
		printf("%ldKb\n", val / 1024L);
	}
}
#endif

int bin_sched(nam, argv, ops, func)	/**/
char *nam;
char **argv;
char *ops;
int func;
{
    char *s = *argv++;
    time_t t;
    long h, m;
    struct tm *tm;
    struct schedcmd *sch, *sch2, *schl;
    int t0;

    if (s && *s == '-') {
	t0 = atoi(s + 1);

	if (!t0) {
	    zwarnnam("sched", "usage for delete: sched -<item#>.", NULL, 0);
	    return 1;
	}
	for (schl = (struct schedcmd *)&schedcmds, sch = schedcmds, t0--;
	     sch && t0; sch = (schl = sch)->next, t0--);
	if (!sch) {
	    zwarnnam("sched", "not that many entries", NULL, 0);
	    return 1;
	}
	schl->next = sch->next;
	zsfree(sch->cmd);
	zfree(sch, sizeof(struct schedcmd));

	return 0;
    }
    if (!s) {
	char tbuf[40];

	for (t0 = 1, sch = schedcmds; sch; sch = sch->next, t0++) {
	    t = sch->time;
	    tm = localtime(&t);
	    ztrftime(tbuf, 20, "%a %b %e %k:%M:%S", tm);
	    printf("%3d %s %s\n", t0, tbuf, sch->cmd);
	}
	return 0;
    } else if (!*argv) {
	zwarnnam("sched", "not enough arguments", NULL, 0);
	return 1;
    }
    if (*s == '+') {
	h = zstrtol(s + 1, &s, 10);
	if (*s != ':') {
	    zwarnnam("sched", "bad time specifier", NULL, 0);
	    return 1;
	}
	m = zstrtol(s + 1, &s, 10);
	if (*s) {
	    zwarnnam("sched", "bad time specifier", NULL, 0);
	    return 1;
	}
	t = time(NULL) + h * 3600 + m * 60;
    } else {
	h = zstrtol(s, &s, 10);
	if (*s != ':') {
	    zwarnnam("sched", "bad time specifier", NULL, 0);
	    return 1;
	}
	m = zstrtol(s + 1, &s, 10);
	if (*s && *s != 'a' && *s != 'p') {
	    zwarnnam("sched", "bad time specifier", NULL, 0);
	    return 1;
	}
	t = time(NULL);
	tm = localtime(&t);
	t -= tm->tm_sec + tm->tm_min * 60 + tm->tm_hour * 3600;
	if (*s == 'p')
	    h += 12;
	t += h * 3600 + m * 60;
	if (t < time(NULL))
	    t += 3600 * 24;
    }
    sch = (struct schedcmd *)zcalloc(sizeof *sch);
    sch->time = t;
    sch->cmd = ztrdup(spacejoin(argv));
    sch->next = NULL;
    for (sch2 = (struct schedcmd *)&schedcmds; sch2->next; sch2 = sch2->next);
    sch2->next = sch;
    return 0;
}

int bin_eval(nam, argv, ops, func)	/**/
char *nam;
char **argv;
char *ops;
int func;
{
    char *s = ztrdup(spacejoin(argv));
    List list;

    hungets(s);
    zsfree(s);
    strinbeg();
    if (!(list = parse_list())) {
	hflush();
	strinend();
	return 1;
    }
    strinend();
    runlist(list);
    return lastval;
}

/* get the history event associated with s */

int fcgetcomm(s)		/**/
char *s;
{
    int cmd;

    if ((cmd = atoi(s))) {
	if (cmd < 0)
	    cmd = curhist + cmd;
	if (cmd >= curhist) {
	    zwarnnam("fc", "bad history number: %d", 0, cmd);
	    return -1;
	}
	return cmd;
    }
    cmd = hcomsearch(s);
    if (cmd == -1)
	zwarnnam("fc", "event not found: %s", s, 0);
    return cmd;
}

/* perform old=new substituion */

int fcsubs(sp, sub)		/**/
char **sp;
struct asgment *sub;
{
    char *s1, *s2, *s3, *s4, *s = *sp, *s5;
    int subbed = 0;

    while (sub) {
	s1 = sub->name;
	s2 = sub->value;
	sub = sub->next;
	s5 = s;
	while ((s3 = (char *)ztrstr(s5, s1))) {
	    s4 = (char *)
		alloc(1 + (s3 - s) + strlen(s2) + strlen(s3 + strlen(s1)));
	    ztrncpy(s4, s, s3 - s);
	    strcat(s4, s2);
	    s5 = s4 + strlen(s4);
	    strcat(s4, s3 + strlen(s1));
	    s = s4;
	    subbed = 1;
	}
    }
    *sp = s;
    return subbed;
}

/* print a series of history events to a file */

int fclist(f, n, r, D, d, first, last, subs, com)	/**/
FILE *f;
int n;
int r;
int D;
int d;
int first;
int last;
struct asgment *subs;
Comp com;
{
    int fclistdone = 0;
    char *s, *hs;
    Histent ent;

    if (r) {
	r = last;
	last = first;
	first = r;
    }
    if (!subs)
	fclistdone = 1;
    for (;;) {
	hs = quietgetevent(first);
	if (!hs) {
	    zwarnnam("fc", "no such event: %d", NULL, first);
	    return 1;
	}
	s = makehstr(hs);
	if (!com || domatch(s, com, 0)) {
	    fclistdone |= fcsubs(&s, subs);
	    if (n)
		fprintf(f, "%5d  ", first);
	    ent = NULL;
	    if (d) {
		struct tm *ltm;

		if (!ent)
		    ent = gethistent(first);
		ltm = localtime(&ent->stim);
		if (d >= 2) {
		    if (d >= 4) {
			fprintf(f, "%d.%d.%d ",
				ltm->tm_mday, ltm->tm_mon + 1,
				ltm->tm_year + 1900);
		    } else {
			fprintf(f, "%d/%d/%d ",
				ltm->tm_mon + 1,
				ltm->tm_mday,
				ltm->tm_year + 1900);
		    }
		}
		fprintf(f, "%02d:%02d  ", ltm->tm_hour, ltm->tm_min);
	    }
	    if (D) {
		long diff;

		if (!ent)
		    ent = gethistent(first);
		diff = (ent->ftim) ? ent->ftim - ent->stim : 0;
		fprintf(f, "%ld:%02ld  ", diff / 60, diff % 60);
	    }
	    if (f == stdout) {
		niceprintf(s, f);
		putc('\n', f);
	    } else
		fprintf(f, "%s\n", s);
	}
	if (first == last)
	    break;
	else if (first > last)
	    first--;
	else
	    first++;
    }
    if (f != stdout)
	fclose(f);
    if (!fclistdone) {
	zwarnnam("fc", "no substitutions performed", NULL, 0);
	return 1;
    }
    return 0;
}

int fcedit(ename, fn)		/**/
char *ename;
char *fn;
{
    if (!strcmp(ename, "-"))
	return 1;
    return !zyztem(ename, fn);
}

/* fc, history, r */

int bin_fc(nam, argv, ops, func)/**/
char *nam;
char **argv;
char *ops;
int func;
{
    int first = -1, last = -1, retval, delayrem, minflag = 0;
    char *s;
    struct asgment *asgf = NULL, *asgl = NULL;
    Comp com = NULL;

    if (!interact) {
	zwarnnam(nam, "not interactive shell", NULL, 0);
	return 1;
    }
    if (*argv && ops['m']) {
	tokenize(*argv);
	if (!(com = parsereg(*argv++))) {
	    zwarnnam(nam, "invalid match pattern", NULL, 0);
	    return 1;
	}
    }
    delayrem = !strcmp(nam, "r");
    if (!delayrem && !(ops['l'] && unset(HISTNOSTORE)) &&
	(ops['R'] || ops['W'] || ops['A']))
	remhist();
    if (ops['R']) {
	readhistfile(*argv ? *argv : getsparam("HISTFILE"), 1);
	return 0;
    }
    if (ops['W']) {
	savehistfile(*argv ? *argv : getsparam("HISTFILE"), 1,
		     (ops['I'] ? 2 : 0));
	return 0;
    }
    if (ops['A']) {
	savehistfile(*argv ? *argv : getsparam("HISTFILE"), 1,
		     (ops['I'] ? 3 : 1));
	return 0;
    }
    while (*argv && equalsplit(*argv, &s)) {
	struct asgment *a = (struct asgment *)alloc(sizeof *a);

	if (!asgf)
	    asgf = asgl = a;
	else {
	    asgl->next = a;
	    asgl = a;
	}
	a->name = *argv;
	a->value = s;
	argv++;
    }
    if (*argv) {
	minflag = **argv == '-';
	first = fcgetcomm(*argv);
	if (first == -1)
	    return 1;
	argv++;
    }
    if (*argv) {
	last = fcgetcomm(*argv);
	if (last == -1)
	    return 1;
	argv++;
    }
    if (*argv) {
	zwarnnam("fc", "too many arguments", NULL, 0);
	return 1;
    }
    if (first == -1)
	first = (ops['l']) ? curhist - 16 : curhist - 1;
    if (last == -1)
	last = (ops['l']) ? curhist : first;
    if (first < firsthist())
	first = firsthist();
    if (last == -1)
	last = (minflag) ? curhist : first;
    if (ops['l'])
	retval = fclist(stdout, !ops['n'], ops['r'], ops['D'],
			ops['d'] + ops['f'] * 2 + ops['E'] * 4,
			first, last, asgf, com);
    else {
	FILE *out;
	char *fil = gettemp();

	retval = 1;
	out = fopen(fil, "w");
	if (!out)
	    zwarnnam("fc", "can't open temp file: %e", NULL, errno);
	else {
	    if (!fclist(out, 0, ops['r'], 0, 0, first, last, asgf, com))
		if (fcedit(auxdata ? auxdata : fceditparam, fil))
		    if (stuff(fil))
			zwarnnam("fc", "%e: %s", s, errno);
		    else
			retval = 0;
	}
	unlink(fil);
    }
    if (delayrem)
	remhist();
    return retval;
}

int bin_suspend(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    if (islogin && !ops['f']) {
	zerrnam(name, "can't suspend login shell", NULL, 0);
	return 1;
    }
    if (jobbing) {
	sig_default(SIGPIPE);
	sig_default(SIGTTIN);
	sig_default(SIGTSTP);
	sig_default(SIGTTOU);
    }
    kill(0, SIGTSTP);
    if (jobbing) {
	while (gettygrp() != mypgrp) {
	    sleep(1);
	    if (gettygrp() != mypgrp)
		kill(0, SIGTTIN);
	}
	sig_ignore(SIGTTOU);
	sig_ignore(SIGTSTP);
	sig_ignore(SIGTTIN);
	sig_ignore(SIGPIPE);
    }
    return 0;
}

int bin_alias(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    struct alias *an;
    struct asgment *asg;
    int incm = !(ops['a'] || ops['g']), ret = 0;

    if (ops['r'])
	showflag = 2;
    else
	showflag = !incm;
    if (!*argv)
	listhtable(aliastab, (HFunc) printalias);
    else
	while ((asg = getasg(*argv++))) {
	    if (asg->value)
		addhnode(ztrdup(asg->name), mkanode(ztrdup(asg->value), incm),
			 aliastab, freeanode);
	    else if (ops['m']) {
		int n, i;
		struct alias *a;
		Comp com;

		tokenize(argv[-1]);
		if (!(com = parsereg(argv[-1]))) {
		    ret = 1;
		    untokenize(argv[-1]);
		    zerrnam(name, "bad pattern : %s", argv[-1], 0);
		    continue;
		}
		n = aliastab->hsize;
		for (i = 0; i < n; i++) {
		    for (a = (struct alias *)aliastab->nodes[i]; a;
			 a = (struct alias *)a->next) {
			if (a->nam && domatch(a->nam, com, 0))
			    printalias(a->nam, a), ret = 0;
		    }
		}
	    } else if ((an = (Alias) gethnode(asg->name, aliastab))) {
		if ((!ops['r'] || an->cmd == 1) &&
		    (!ops['g'] || !an->cmd))
		    printalias(asg->name, an);
	    } else
		ret = 1;
	}
    return ret;
}

/* print an alias; used with listhtable */

void printalias(s, a)		/**/
char *s;
struct alias *a;
{
    char *ptr;
    int special = 0;

    if (a->cmd >= 0 && (!showflag ||
			(showflag == 1 && !a->cmd) ||
			(showflag == 2 && a->cmd)))
	/*!(showflag && a->cmd))*/  {
	for (ptr = a->text; *ptr; ptr++)
	    if (ispecial(*ptr))
		special++;
	if (special) {
	    printf("%s=\'", s);
	    for (ptr = a->text; *ptr; ptr++)
		if (*ptr == '\'')
		    printf("\'\\\'\'");
		else
		    putchar(*ptr);
	    printf("\'\n");
	} else
	    printf("%s=%s\n", s, a->text);
	}
}

/* print a param; used with listhtable */

void printparam(s, p)		/**/
char *s;
Param p;
{
    if ((showflag > 0 && !(p->flags & showflag)) || (p->flags & PMFLAG_UNSET))
	return;
    if (!showflag) {
	int fgs = p->flags;

	if (fgs & PMFLAG_i)
	    printf("integer ");
	if (fgs & PMFLAG_A)
	    printf("array ");
	if (fgs & PMFLAG_L)
	    printf("left justified %d ", p->ct);
	if (fgs & PMFLAG_R)
	    printf("right justified %d ", p->ct);
	if (fgs & PMFLAG_Z)
	    printf("zero filled %d ", p->ct);
	if (fgs & PMFLAG_l)
	    printf("lowercase ");
	if (fgs & PMFLAG_u)
	    printf("uppercase ");
	if (fgs & PMFLAG_r)
	    printf("readonly ");
	if (fgs & PMFLAG_t)
	    printf("tagged ");
	if (fgs & PMFLAG_x)
	    printf("exported ");
    }
    if (showflag2)
	printf("%s\n", s);
    else {
	char *t, **u;

	printf("%s=", s);
	switch (p->flags & PMTYPE) {
	case PMFLAG_s:
	    if (p->gets.cfn && (t = p->gets.cfn(p)))
		puts(t);
	    else
		putchar('\n');
	    break;
	case PMFLAG_i:
	    printf("%ld\n", p->gets.ifn(p));
	    break;
	case PMFLAG_A:
	    putchar('(');
	    u = p->gets.afn(p);
	    if (!*u)
		printf(")\n");
	    else {
		while (u[1])
		    printf("%s ", *u++);
		printf("%s)\n", *u);
	    }
	    break;
	}
    }
}

/* autoload, declare, export, functions, integer, local, readonly, typeset */

int bin_typeset(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    int on = 0, off = 0, roff, bit = 1, retcode = 0, initon, initoff;
    char *optstr = "LRZilurtx";
    struct param *pm;
    struct asgment *asg;

    for (; *optstr; optstr++, bit <<= 1)
	if (ops[*optstr] == 1)
	    on |= bit;
	else if (ops[*optstr] == 2)
	    off |= bit;
    roff = off;
    if (ops['f']) {
	on &= PMFLAG_t | PMFLAG_u;
	off &= PMFLAG_t | PMFLAG_u;
	showflag = (ops['f'] == 1);
	if (ops['@'] && ((off & ~PMFLAG_t) || (on & ~(PMFLAG_u | PMFLAG_t)))) {
	    zerrnam(name, "invalid option(s)", NULL, 0);
	    return 1;
	}
	showflag2 = 0;
	if (!*argv) {
	    showflag2 = off | on;
	    listhtable(cmdnamtab, (HFunc) pshfunc);
	} else if (ops['m']) {
	    Comp com;
	    int i, n;
	    struct cmdnam *chn;

	    on &= ~512;
	    for (; *argv; argv++) {
		tokenize(*argv);
		if (!(com = parsereg(*argv))) {
		    retcode = 1;
		    untokenize(*argv);
		    zerrnam(name, "bad pattern : %s", *argv, 0);
		    continue;
		}
		n = cmdnamtab->hsize;
		for (i = 0; i < n; i++)
		    for (chn = (struct cmdnam *)cmdnamtab->nodes[i]; chn;
			 chn = (struct cmdnam *)chn->next)
			if ((chn->flags & SHFUNC) &&
			    domatch(chn->nam, com, 0)) {
			    if (on | off)
				chn->flags = (chn->flags | on) & (~off);
			    else
				pshfunc(chn->nam, chn);
			}
	    }
	} else
	    for (; *argv; argv++) {
		Cmdnam cc;

		if ((cc = (Cmdnam) gethnode(*argv, cmdnamtab)) &&
		    (cc->flags & SHFUNC))
		    if (on | off)
			cc->flags = (cc->flags | on) & (~off);
		    else
			pshfunc(*argv, cc);
		else if (on & PMFLAG_u) {
		    cc = (Cmdnam) zcalloc(sizeof *cc);
		    cc->flags = SHFUNC | on;
		    addhnode(ztrdup(*argv), cc, cmdnamtab, freecmdnam);
		} else
		    retcode = 1;
	    }
	return retcode;
    }
    if ((on | off) & PMFLAG_x)
	func = BIN_EXPORT;
    if (on & PMFLAG_i)
	off |= PMFLAG_R | PMFLAG_L | PMFLAG_Z | PMFLAG_u | PMFLAG_A;
    if (on & PMFLAG_L)
	off |= PMFLAG_R | PMFLAG_i;
    if (on & PMFLAG_R)
	off |= PMFLAG_L | PMFLAG_i;
    if (on & PMFLAG_Z)
	off |= PMFLAG_i;
    if (on & PMFLAG_u)
	off |= PMFLAG_l;
    if (on & PMFLAG_l)
	off |= PMFLAG_u;
    on &= ~off;
    showflag = showflag2 = 0;
    initon = on;
    initoff = off;
    if (!*argv) {
	showflag = on | roff;
	showflag2 = roff || ops['+'];
	listhtable(paramtab, (HFunc) printparam);
    } else
	while ((asg = getasg(*argv++))) {
	    on = initon;
	    off = initoff;
	    if (ops['m']) {
		Comp com;
		int i, n;

		on &= ~512;
		tokenize(asg->name);
		if (!(com = parsereg(asg->name))) {
		    untokenize(asg->name);
		    zerrnam(name, "bad pattern : %s", argv[-1], 0);
		    continue;
		}
		n = paramtab->hsize;
		for (i = 0; i < n; i++)
		    for (pm = (struct param *)paramtab->nodes[i]; pm;
			 pm = (struct param *)pm->next)
			if (domatch(pm->nam, com, 0)) {
			    if (!on && !roff && !asg->value) {
				printparam(pm->nam, pm);
				continue;
			    }
			    pm->flags = (pm->flags | on) & ~off;
			    if ((on & (PMFLAG_L | PMFLAG_R | PMFLAG_Z | PMFLAG_i))
				&& (pmtype(pm) != PMFLAG_A))
				pm->ct = auxlen;
			    if (pmtype(pm) != PMFLAG_A) {
				if (pm->flags & PMFLAG_x) {
				    if (!pm->env)
					pm->env = addenv(pm->nam,
							 (asg->value) ? asg->value : getsparam(pm->nam));
				} else if (pm->env) {
				    delenv(pm->env);
				    zsfree(pm->env);
				    pm->env = NULL;
				}
				if (asg->value)
				    setsparam(pm->nam, ztrdup(asg->value));
			    }
			}
	    } else {
		if (!isident(asg->name)) {
		    zerr("not an identifier: %s", asg->name, 0);
		    continue;
		}
		pm = (Param) gethnode(asg->name, paramtab);
		if (pm && (pm->flags & PMFLAG_SPECIAL)) {
		    func = 0;
		    on = (pmtype(pm) == PMFLAG_i) ?
			(on &= ~(PMFLAG_L | PMFLAG_R | PMFLAG_Z | PMFLAG_u)) :
			(on & ~PMFLAG_i);
		    off &= ~PMFLAG_i;
		}
		if (pm && pm->level)
		    on &= ~PMFLAG_x;
		bit = 0;	/* flag for switching int<->not-int */
		if (pm && !(pm->flags & PMFLAG_UNSET) &&
		    ((((locallevel == pm->level) || func == BIN_EXPORT)
		      && !(bit = ((off & pm->flags) | (on & ~pm->flags)) & PMFLAG_i)) ||
		     (pm->flags & PMFLAG_SPECIAL))) {
		    if (!on && !roff && !asg->value) {
			printparam(asg->name, pm);
			continue;
		    }
		    pm->flags = (pm->flags | on) & ~off;
		    if (on & (PMFLAG_L | PMFLAG_R | PMFLAG_Z | PMFLAG_i))
			pm->ct = auxlen;
		    if (pmtype(pm) != PMFLAG_A) {
			if (pm->flags & PMFLAG_x) {
			    if (!pm->env)
				pm->env = addenv(asg->name,
						 (asg->value) ? asg->value : getsparam(asg->name));
			} else if (pm->env) {
			    delenv(pm->env);
			    zsfree(pm->env);
			    pm->env = NULL;
			}
			if (asg->value)
			    setsparam(asg->name, ztrdup(asg->value));
		    }
		} else {
		    int readonly = on & PMFLAG_r;

		    if (bit) {
			if (!asg->value)
			    asg->value = dupstring(getsparam(asg->name));
			unsetparam(asg->name);
		    } else if (locallist && func != BIN_EXPORT) {
			permalloc();
			addnode(locallist, ztrdup(asg->name));
			heapalloc();
		    }
		    pm = createparam(ztrdup(asg->name), on & ~PMFLAG_r);
		    pm->ct = auxlen;
		    if (func != BIN_EXPORT)
			pm->level = locallevel;
		    if (asg->value)
			setsparam(asg->name, ztrdup(asg->value));
		    pm->flags |= readonly;
		}
	    }
	}
    return 0;
}

/* Check whether a command is a bin_typeset. */

int istypeset(c, nam)		/**/
Cmdnam c;
char *nam;
{
    struct bincmd *b;

    if (c)
	if ((c->flags & BUILTIN) && !(c->flags & EXCMD))
	    return (builtins[c->u.binnum].handlerfunc == bin_typeset);
	else
	    nam = c->nam;
    if (nam)
	for (b = builtins; b->name; b++)
	    if (!strcmp(nam, b->name) && b->handlerfunc == bin_typeset)
		return 1;
    return 0;
}

/* echo, print, pushln */

int bin_print(name, args, ops, func)	/**/
char *name;
char **args;
char *ops;
int func;
{
    int nnl = 0, fd;
    Histent ent;
    FILE *fout = stdout;

    if (ops['z']) {
	permalloc();
	pushnode(bufstack, ztrdup(spacejoin(args)));
	heapalloc();
	return 0;
    }
    if (ops['s']) {
	permalloc();
	ent = gethistent(++curhist);
	zsfree(ent->lex);
	zsfree(ent->lit);
	ent->lex = ztrdup(join(args, HISTSPACE));
	ent->lit = ztrdup(join(args, ' '));
	ent->stim = ent->ftim = time(NULL);
	ent->flags = 0;
	heapalloc();
	return 0;
    }
    if (ops['R'])
	ops['r'] = 1;
    if (ops['u'] || ops['p']) {
	if (ops['u']) {
	    for (fd = 0; fd < 10; fd++)
		if (ops[fd + '0'])
		    break;
	    if (fd == 10)
		fd = 0;
	} else
	    fd = coprocout;
	if ((fd = dup(fd)) < 0) {
	    zwarnnam(name, "bad file number", NULL, 0);
	    return 1;
	}
	if ((fout = fdopen(fd, "w")) == 0) {
	    zwarnnam(name, "bad mode on fd", NULL, 0);
	    return 1;
	}
    }
    if (ops['o']) {
	if (ops['i'])
	    qsort(args, arrlen(args), sizeof(char *), cstrpcmp);

	else
	    qsort(args, arrlen(args), sizeof(char *), strpcmp);
    } else if (ops['O']) {
	if (ops['i'])
	    qsort(args, arrlen(args), sizeof(char *), invcstrpcmp);

	else
	    qsort(args, arrlen(args), sizeof(char *), invstrpcmp);
    }
    if (ops['c']) {
	int l, nc, nr, sc, n, t, i;
	char **ap;

	for (n = l = 0, ap = args; *ap; ap++, n++)
	    if (l < (t = strlen(*ap)))
		l = t;

	nc = (columns - 1) / (l + 2);
	sc = 0;
	if (nc)
	    sc = (columns - 1) / nc;
	else
	    nc = 1;
	nr = (n + nc - 1) / nc;

	for (i = 0; i < nr; i++) {
	    ap = args + i;
	    do {
		l = strlen(*ap);
		fprintf(fout, "%s", *ap);
		for (; l < sc; l++)
		    fputc(' ', fout);
		for (t = nr; t && *ap; t--, ap++);
	    }
	    while (*ap);
	    fputc(ops['N'] ? '\0' : '\n', fout);
	}
	if (fout != stdout)
	    fclose(fout);
	return 0;
    }
    for (; *args; args++) {
	char *arg = *args;
	int len = -1;

	if (!ops['r'])
	    arg = getkeystring(arg, &len, func != BIN_ECHO, &nnl);
	if (ops['D'])
	    fprintdir(arg, fout);
	else {
	    if (ops['P'])
		arg = putprompt(arg, &len, -1);
	    fwrite(arg, 1, len == -1 ? strlen(arg) : len, fout);
	}

	if (args[1])
	    fputc(ops['l'] ? '\n' : ops['0'] ? '\0' : ' ', fout);
    }
    if (!(ops['n'] || nnl))
	fputc(ops['N'] ? '\0' : '\n', fout);
    if (fout != stdout)
	fclose(fout);
    return 0;
}

int bin_dirs(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    Lklist l;

    if (ops['v']) {
	Lknode node;
	int t0 = 1;

	printf("0\t");
	printdir(pwd);
	for (node = firstnode(dirstack); node; incnode(node)) {
	    printf("\n%d\t", t0++);
	    printdir(getdata(node));
	}
	putchar('\n');
	return 0;
    }
    if (!*argv) {
	pdstack();
	return 0;
    }
    permalloc();
    l = newlist();
    if (!*argv) {
	heapalloc();
	return 0;
    }
    while (*argv)
	addnode(l, ztrdup(*argv++));
    freetable(dirstack, freestr);
    dirstack = l;
    heapalloc();
    return 0;
}

int bin_unalias(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    int ret = 0;
    Alias dat;

    while (*argv)
	if (ops['m']) {
	    int n, i, match = 0;
	    Alias a;
	    Comp com;

	    tokenize(*argv++);
	    if (!(com = parsereg(argv[-1]))) {
		ret = 1;
		untokenize(argv[-1]);
		zwarnnam(name, "bad pattern : %s", argv[-1], 0);
		continue;
	    }
	    n = aliastab->hsize;
	    for (i = 0; i < n; i++) {
		for (a = (Alias) aliastab->nodes[i]; a; a = dat) {
		    dat = (Alias) a->next;
		    if (a->nam && a->cmd >= 0 && domatch(a->nam, com, 0))
			freeanode(remhnode(a->nam, aliastab)), match++;
		}
	    }
	    if (!ret)
		ret = !match;
	} else {
	    if ((dat = (Alias) gethnode(*argv++, aliastab)) && dat->cmd >= 0)
		freeanode(remhnode(dat->nam, aliastab));
	    else
		ret = 1;
	}
    return ret;
}

int bin_disable(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    Cmdnam chn, chn2, nchn;
    Comp com;

    if (!*argv) {
	listhtable(cmdnamtab, (HFunc) pdisabledcmd);
	return 0;
    }
    if (ops['m']) {
	for (; *argv; argv++) {
	    tokenize(*argv);
	    if (!(com = parsereg(*argv))) {
		untokenize(*argv);
		zwarnnam(name, "bad pattern : %s", *argv, 0);
		continue;
	    }
	    if (!strncmp(*argv, "TRAP", 4)) {
		char trapname[20];
		int t;

		strncpy(trapname, "TRAP", sizeof(trapname));
		for (t = 0; t < VSIGCOUNT; t++) {
		    strncpy(trapname + 4, sigs[t], sizeof(trapname) - 5);
		    if (domatch(trapname, com, 0)) {
			unsettrap(t);
			chn = (Cmdnam) zcalloc(sizeof *chn);
			chn->flags |= DISABLED;
			addhnode(ztrdup(trapname), chn, cmdnamtab, freecmdnam);
		    }
		}
	    } else {
		int t, n;

		n = cmdnamtab->hsize;
		for (t = 0; t < n; t++)
		    for (chn = (Cmdnam) cmdnamtab->nodes[t]; chn; chn = nchn) {
			nchn = (Cmdnam) chn->next;
			if (domatch(chn->nam, com, 0)) {
			    chn2 = (Cmdnam) zcalloc(sizeof *chn2);
			    chn2->flags |= DISABLED;
			    addhnode(ztrdup(chn->nam), chn2, cmdnamtab,
				     freecmdnam);
			}
		    }
	    }
	}
    } else {
	char **p, buf[MAXPATHLEN];

	while (*argv) {
	    if (!strncmp(*argv, "TRAP", 4))
		unsettrap(getsignum(*argv + 4));
	    chn = (Cmdnam) zcalloc(sizeof *chn);
	    for (p = path; *p; p++) {
		strcpy(buf, *path);
		strcat(buf, "/");
		strcat(buf, *argv);
		if (iscom(buf)) {
		    chn->u.name = path;
		    break;
		}
	    }
	    chn->flags |= *p ? EXCMD : DISABLED;
	    addhnode(ztrdup(*argv++), chn, cmdnamtab, freecmdnam);
	}
    }
    return 0;
}

int bin_unhash(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    vptr dat;
    Comp com;

    if (ops['m']) {
	for (; *argv; argv++) {
	    tokenize(*argv);
	    if (!(com = parsereg(*argv))) {
		untokenize(*argv);
		zwarnnam(name, "bad pattern : %s", *argv, 0);
		continue;
	    }
	    if (!strncmp(*argv, "TRAP", 4)) {
		char trapname[20];
		int t;

		strncpy(trapname, "TRAP", sizeof(trapname));
		for (t = 0; t < VSIGCOUNT; t++) {
		    strncpy(trapname + 4, sigs[t], sizeof(trapname) - 5);
		    if (domatch(trapname, com, 0))
			unsettrap(t);
		}
	    } else {
		Cmdnam chn, nchn;
		int t, n;

		n = cmdnamtab->hsize;
		for (t = 0; t < n; t++)
		    for (chn = (Cmdnam) cmdnamtab->nodes[t]; chn; chn = nchn) {
			nchn = (Cmdnam) chn->next;
			if (domatch(chn->nam, com, 0))
			    freecmdnam(remhnode(chn->nam, cmdnamtab));
		    }
	    }
	}
    } else {
	while (*argv) {
	    if (!strncmp(*argv, "TRAP", 4))
		unsettrap(getsignum(*argv + 4));
	    if ((dat = remhnode(*argv++, cmdnamtab)))
		freecmdnam(dat);
	}
    }
    return 0;
}

int bin_unset(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    int retval = 0;
    char *s;

    while ((s = *argv++))
	if (ops['m']) {
	    int i, n;
	    Comp com;
	    struct param *par, *next;

	    tokenize(s);
	    if (!(com = parsereg(s))) {
		retval = 1;
		untokenize(s);
		zwarnnam(name, "bad pattern : %s", s, 0);
		continue;
	    }
	    n = paramtab->hsize;
	    for (i = 0; i < n; i++)
		for (par = (struct param *)paramtab->nodes[i]; par;
		     par = next) {
		    next = (struct param *)par->next;
		    if (domatch(par->nam, com, 0))
			unsetparam(par->nam);
		}
	} else {
	    if (gethnode(s, paramtab))
		unsetparam(s);
	    else
		retval = 1;
	}
    return retval;
}

static char *zbuf;
static int readfd;

int zread()
{				/**/
    char cc;

    if (zbuf)
	return (*zbuf) ? *zbuf++ : EOF;
    if (read(readfd, &cc, 1) != 1)
	return EOF;
    return (int)cc;
}

extern int cs;

int bin_read(name, args, ops, func)	/**/
char *name;
char **args;
char *ops;
int func;
{
    char *reply = "REPLY", *readpmpt;
    int bsiz, c = 0, gotnl = 0, al = 0;
    char *buf, *bptr, *firstarg = *args, *zbuforig;
    Lklist readll = newlist();

    if (ops['k']) {
	int nchars, val;
	char cc, d;
	int haso = 0, isem = !strcmp(term, "emacs");

	if (SHTTY == -1) {
	    SHTTY = open("/dev/tty", O_RDWR);
	    haso = 1;
	}
	if (SHTTY == -1) {
	    fprintf(stderr, "not interactive and can't open terminal\n");
	    fflush(stderr);
	    return 1;
	}
	if (*args && idigit(**args)) {
	    if (!(nchars = atoi(*args)))
		nchars = 1;
	    args++;
	} else
	    nchars = 1;

	if (*args && **args == '/') {
	    fprintf(stderr, "%s", putprompt(*args + 1, &c, 0));
	    fflush(stderr);
	    args++;
	}
	if (*args)
	    reply = *args++;

	bptr = buf = (char *)zalloc(nchars + 1);
	buf[nchars] = '\0';

	attachtty(mypgrp);
	if (!isem)
	    setcbreak();

	for (bptr = buf; nchars;) {
#ifdef FIONREAD
	    ioctl(SHTTY, FIONREAD, (char *)&val);
	    if (val) {
		if (!isem)
		    settyinfo(&shttyinfo);
		if (ops['e'] || ops['E']) {
		    printf("%s\n", buf);
		    if (ops['e'])
			zsfree(buf);
		}
		if (!ops['e'])
		    setsparam(reply, buf);

		if (haso) {
		    close(SHTTY);
		    SHTTY = -1;
		}
		return 1;
	    }
#endif
	    if (read(SHTTY, &cc, 1) == 1)
		nchars--, *bptr++ = cc;
	}
	if (isem)
	    while (read(SHTTY, &d, 1) == 1 && d != '\n');
	else
	    settyinfo(&shttyinfo);

	if (haso) {
	    close(SHTTY);
	    SHTTY = -1;
	}
	if (ops['e'] || ops['E']) {
	    printf("%s\n", buf);
	    if (ops['e'])
		zsfree(buf);
	}
	if (!ops['e'])
	    setsparam(reply, buf);
	return 0;
    }
    if (ops['l']) {
	if (!inzlefunc) {
	    zwarnnam(name, "option valid only in functions called from zle",
		     NULL, 0);
	    errflag = 0;
	    return 1;
	}
	if (ops['n']) {
	    char nbuf[14];

	    if (ops['e'] || ops['E'])
		printf("%d\n", cs + 1);
	    if (!ops['e']) {
		sprintf(nbuf, "%d", cs + 1);
		setsparam(*args ? *args : "REPLY", ztrdup(nbuf));
	    }
	    return 0;
	}
	if (ops['e'] || ops['E'])
	    printf("%s\n", (char *)line);
	if (!ops['e'])
	    setsparam(*args ? *args : "REPLY", ztrdup((char *)line));
	return 0;
    }
    if (ops['c']) {
	if (!inzlefunc) {
	    zwarnnam(name, "option valid only in functions called from zle",
		     NULL, 0);
	    errflag = 0;
	    return 1;
	}
	if (ops['n']) {
	    char nbuf[14];

	    if (ops['e'] || ops['E'])
		printf("%d\n", clwpos + 1);
	    if (!ops['e']) {
		sprintf(nbuf, "%d", clwpos + 1);
		setsparam(*args ? *args : "REPLY", ztrdup(nbuf));
	    }
	    return 0;
	}
	if (ops['A'] && !ops['e']) {
	    char **p, **b = (char **)zcalloc((clwnum + 1) * sizeof(char *));
	    int i;

	    for (i = 0, p = b; i < clwnum; p++, i++)
		*p = ztrdup(clwords[i]);

	    setaparam(*args ? *args : "reply", b);
	    return 0;
	}
	if (ops['e'] || ops['E']) {
	    int i;

	    for (i = 0; i < clwnum; i++)
		printf("%s\n", clwords[i]);

	    if (ops['e'])
		return 0;
	}
	if (*args) {
	    int i = 0;

	    for (; i < clwnum && *args; args++, i++)
		setsparam(*args, ztrdup(clwords[i]));
	} else
	    setsparam("REPLY", ztrdup(clwords[clwpos]));

	return 0;
    }
    if (ops['q']) {
	char *readbuf;
	int haso = 0;

	if (SHTTY == -1)
	    SHTTY = open("/dev/tty", O_RDWR), haso = 1;

	if (SHTTY == -1) {
	    fprintf(stderr, "not interactive and can't open terminal\n");
	    fflush(stderr);
	    return 1;
	}
	readbuf = (char *)zalloc(2);
	readbuf[1] = '\0';

	if (*args && **args == '/') {
	    fprintf(stderr, "%s", putprompt(*args + 1, &c, 0));
	    fflush(stderr);
	    args++;
	}
	reply = (*args) ? *args++ : "REPLY";

	readbuf[0] = ((char)getquery()) == 'y' ? 'y' : 'n';

	if (haso) {
	    close(SHTTY);
	    SHTTY = -1;
	}
	if (ops['e'] || ops['E']) {
	    printf("%s\n", readbuf);
	    if (ops['e'])
		free(readbuf);
	}
	if (!ops['e'])
	    setsparam(reply, readbuf);

	return readbuf[0] == 'n';
    }
    if (*args && **args == '?')
	args++;
    reply = *args ? *args++ : ops['A'] ? "reply" : "REPLY";
    if (ops['A'] && *args) {
	zwarnnam(name, "only one array argument allowed", NULL, 0);
	return 1;
    }
    if (ops['u'] && !ops['p']) {
	for (readfd = 0; readfd < 10; ++readfd)
	    if (ops[readfd + '0'])
		break;
	if (readfd == 10)
	    readfd = 0;
    } else if (ops['p'])
	readfd = coprocin;
    else {
	attachtty((jobtab[thisjob].gleader) ? jobtab[thisjob].gleader : mypgrp);
	readfd = 0;
	if (firstarg) {
	    for (readpmpt = firstarg;
		 *readpmpt && *readpmpt != '?'; readpmpt++);
	    if (*readpmpt++) {
		if (isatty(0))
		    write(2, readpmpt, strlen(readpmpt));
		readpmpt[-1] = '\0';
	    }
	}
#if 0
	else if (isset(SHINSTDIN) && unset(INTERACTIVE)) {
	    if (isatty(1))
		readfd = 1;
	    else if (isatty(2))
		readfd = 2;
	}
#endif
    }

    zbuforig = zbuf = (!ops['z']) ? NULL :
	(full(bufstack)) ? (char *)getnode(bufstack) : ztrdup("");
    while (*args || (ops['A'] && !gotnl)) {
	buf = bptr = (char *)zalloc(bsiz = 64);
	for (;;) {
	    if (gotnl)
		break;
	    c = zread();
	    if (!ops['r'] && c == '\n' && bptr != buf && bptr[-1] == '\\') {
		bptr--;
		continue;
	    }
	    if (c == EOF || (isep(c) && bptr != buf) || c == '\n')
		break;
	    if (isep(c))
		continue;
	    *bptr++ = c;
	    if (bptr == buf + bsiz) {
		buf = realloc(buf, bsiz *= 2);
		bptr = buf + (bsiz / 2);
	    }
	}
	if (c == EOF) {
	    if (readfd == coprocin) {
		close(coprocin);
		close(coprocout);
		coprocin = coprocout = -1;
	    }
	    return 1;
	}
	if (c == '\n')
	    gotnl = 1;
	*bptr = '\0';
	if (ops['e'] || ops['E']) {
	    printf("%s\n", buf);
	    if (ops['e'])
		free(buf);
	}
	if (!ops['e']) {
	    if (ops['A']) {
		addnode(readll, buf);
		al++;
	    } else
		setsparam(reply, buf);
	}
	if (!ops['A'])
	    reply = *args++;
    }
    if (ops['A']) {
	char **pp, **p = NULL;
	Lknode n;

	p = (ops['e'] ? (char **)NULL
	     : (char **)zcalloc((al + 1) * sizeof(char *)));

	for (pp = p, n = firstnode(readll); n; incnode(n)) {
	    if (ops['e'] || ops['E']) {
		printf("%s\n", (char *)getdata(n));
		if (p)
		    zsfree(getdata(n));
	    } else
		*pp++ = (char *)getdata(n);
	}
	if (p)
	    setaparam(reply, p);
	return 0;
    }
    buf = bptr = (char *)zalloc(bsiz = 64);
    if (!gotnl)
	for (;;) {
	    c = zread();
	    if (!ops['r'] && c == '\n' && bptr != buf && bptr[-1] == '\\') {
		bptr--;
		continue;
	    }
	    if (c == EOF || (c == '\n' && !zbuf))
		break;
	    if (isep(c) && bptr == buf)
		continue;
	    *bptr++ = c;
	    if (bptr == buf + bsiz) {
		buf = realloc(buf, bsiz *= 2);
		bptr = buf + (bsiz / 2);
	    }
	}
    while (bptr > buf && isep(bptr[-1]))
	bptr--;
    *bptr = '\0';
    if (ops['e'] || ops['E']) {
	printf("%s\n", buf);
	if (ops['e'])
	    zsfree(buf);
    }
    if (!ops['e'])
	setsparam(reply, buf);
    if (zbuforig) {
	char first = *zbuforig;

	zsfree(zbuforig);
	if (!first)
	    return 1;
    } else if (c == EOF) {
	if (readfd == coprocin) {
	    close(coprocin);
	    close(coprocout);
	    coprocin = coprocout = -1;
	}
	return 1;
    }
    return 0;
}

int bin_vared(name, args, ops, func)	/**/
char *name;
char **args;
char *ops;
int func;
{
    char *s;
    char *t;
    struct param *pm;
    int create = 0, pl1, pl2;
    char *p1 = NULL, *p2 = NULL;

    while (*args && **args == '-') {
	while (*++(*args))
	    switch (**args) {
	    case 'c':
		create = 1;
		break;
	    case 'p':
		if ((*args)[1])
		    p1 = *args + 1, *args = "" - 1;
		else if (args[1])
		    p1 = *(++args), *args = "" - 1;
		else {
		    zwarnnam(name, "prompt string expected after -p", NULL, 0);
		    return 1;
		}
		break;
	    case 'r':
		if ((*args)[1])
		    p2 = *args + 1, *args = "" - 1;
		else if (args[1])
		    p2 = *(++args), *args = "" - 1;
		else {
		    zwarnnam(name, "prompt string expected after -r", NULL, 0);
		    return 1;
		}
		break;
	    default:
		zwarnnam(name, "unknown option: %s", *args, 0);
		return 1;
	    }
	args++;
    }

    if (!*args) {
	zwarnnam(name, "missing variable", NULL, 0);
	return 1;
    }
    if (!(s = getsparam(args[0]))) {
	if (create)
	    createparam(args[0], PMFLAG_s);
	else {
	    zwarnnam(name, "no such variable: %s", args[0], 0);
	    return 1;
	}
    }
    permalloc();
    pushnode(bufstack, ztrdup(s));
    heapalloc();
    if (p1)
	p1 = putprompt(p1, &pl1, 0);
    else
	pl1 = 0;
    if (p2)
	p2 = putprompt(p2, &pl2, 0);
    else
	pl2 = 0;
    t = (char *)zleread((unsigned char *)p1, (unsigned char *)p2, pl1, pl2);
    if (!t || errflag)
	return 1;
    if (t[strlen(t) - 1] == '\n')
	t[strlen(t) - 1] = '\0';
    pm = (struct param *)gethnode(args[0], paramtab);
    if (pmtype(pm) == PMFLAG_A)
	setaparam(args[0], spacesplit(t));
    else
	setsparam(args[0], t);
    return 0;
}

#define fset(X) (flags & X)

/* execute a builtin handler function after parsing the arguments */

int execbin(args, cnode)	/**/
Lklist args;
Cmdnam cnode;
{
    struct bincmd *b;
    char ops[128], *arg, *pp, *name, **argv, **oargv, *optstr;
    int t0, flags, sense, argc = 0, execop;
    Lknode n;
    char *oxarg, *xarg = NULL;

    auxdata = NULL;
    auxlen = 0;
    for (t0 = 0; t0 != 128; t0++)
	ops[t0] = 0;
    name = (char *)ugetnode(args);
    b = builtins + cnode->u.binnum;

/* the 'builtin' builtin is handled specially */

    if (b->funcid == BIN_BUILTIN) {
	Cmdnam cname;

	if (!(name = (char *)ugetnode(args))) {
	    zerrnam("builtin", "command name expected", NULL, 0);
	    return 1;
	}
	if ((cname = (Cmdnam) gethnode(name, cmdnamtab)) &&
	    (cname->flags & BUILTIN) &&
	    !(cname->flags & EXCMD))
	    b = builtins + cname->u.binnum;
	else
	    for (b = builtins; b->name; b++)
		if (!strcmp(name, b->name))
		    break;
	if (!b->name) {
	    zerrnam("builtin", "no such builtin: %s", name, 0);
	    return 1;
	}
    }
    flags = b->flags;
    arg = (char *)ugetnode(args);
    optstr = b->optstr;
    if (flags & BINF_ECHOPTS && arg && strcmp(arg, "-n"))
	optstr = NULL;
    if (optstr)
	while (arg &&
	       ((sense = *arg == '-') || (fset(BINF_PLUSOPTS) && *arg == '+')) &&
	       (fset(BINF_PLUSOPTS) || !atoi(arg))) {
	    if (xarg) {
		oxarg = tricat(xarg, " ", arg);
		zsfree(xarg);
		xarg = oxarg;
	    } else
		xarg = ztrdup(arg);
	    if (arg[1] == '-')
		arg++;
	    if (!arg[1]) {
		ops['-'] = 1;
		if (!sense)
		    ops['+'] = 1;
	    } else
		ops['@'] = 1;
	    execop = -1;
	    while (*++arg)
		if (strchr(b->optstr, execop = (int)*arg))
		    ops[(int)*arg] = (sense) ? 1 : 2;
		else
		    break;
	    if (*arg) {
		zerr("bad option: %c", NULL, *arg);
		zsfree(xarg);
		return 1;
	    }
	    arg = (char *)ugetnode(args);
	    if (fset(BINF_SETOPTS) && execop == 'o') {
		int c;

		if (!arg)
		    prtopt(sense);
		else {
		    c = optlookup(arg);
		    if (c == -1) {
			zerr("bad option: %s", arg, 0);
			zsfree(xarg);
			return 1;
		    } else {
			if (c == INTERACTIVE)
			    zerr("can't change option: %s", arg, 0);
			else
			    ops[c] = ops['o'];
			arg = (char *)ugetnode(args);
		    }
		}
	    }
	    if ((fset(BINF_PRINTOPTS) && ops['R']) || ops['-'])
		break;
	    if (fset(BINF_SETOPTS) && ops['A']) {
		auxdata = arg;
		arg = (char *)ugetnode(args);
		break;
	    }
	    if (fset(BINF_FCOPTS) && execop == 'e') {
		auxdata = arg;
		arg = (char *)ugetnode(args);
	    }
	    if (fset(BINF_TYPEOPT) && (execop == 'L' || execop == 'R' ||
				       execop == 'Z' || execop == 'i') && arg && idigit(*arg)) {
		auxlen = atoi(arg);
		arg = (char *)ugetnode(args);
	    }
	}
    if (fset(BINF_R))
	auxdata = "-";
    if ((pp = b->defopts))
	while (*pp)
	    ops[(int)*pp++] = 1;
    if (arg) {
	argc = 1;
	n = firstnode(args);
	while (n)
	    argc++, incnode(n);
    }
    oargv = argv = (char **)ncalloc(sizeof(char **) * (argc + 1));

    if ((*argv++ = arg))
	while ((*argv++ = (char *)ugetnode(args)));
    argv = oargv;
    if (errflag) {
	zsfree(xarg);
	return 1;
    }
    if (argc < b->minargs || (argc > b->maxargs && b->maxargs != -1)) {
	zerrnam(name, (argc < b->minargs)
		? "not enough arguments" : "too many arguments", NULL, 0);
	zsfree(xarg);
	return 1;
    }
    if (isset(XTRACE)) {
	char **execpp = argv;

	fprintf(stderr, "%s%s", (prompt4) ? prompt4 : "", name);
	if (xarg)
	    fprintf(stderr, " %s", xarg);
	while (*execpp)
	    fprintf(stderr, " %s", *execpp++);
	fputc('\n', stderr);
	fflush(stderr);
    }
    zsfree(xarg);
    return (*(b->handlerfunc)) (name, argv, ops, b->funcid);
}

struct asgment *getasg(s)	/**/
char *s;
{
    static struct asgment asg;

    if (!s)
	return NULL;
    if (*s == '=') {
	zerr("bad assignment", NULL, 0);
	return NULL;
    }
    asg.name = s;
    for (; *s && *s != '='; s++);
    if (*s) {
	*s = '\0';
	asg.value = s + 1;
    } else
	asg.value = NULL;
    return &asg;
}

/* ., source */

int bin_dot(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    char **old, *old0;
    int ret, diddot = 0, dotdot = 0;
    char buf[MAXPATHLEN];
    char *s, **t, *enam;

    if (!*argv)
	return 0;
    old = pparams;
    old0 = argzero;
    if (argv[1]) {
	permalloc();
	pparams = arrdup(argv + 1);
	heapalloc();
    }
    enam = argzero = ztrdup(*argv);
    errno = ENOENT;
    ret = 1;
    if (*name != '.' && access(argzero, F_OK) == 0) {
	diddot = 1;
	ret = source(enam = argzero);
    }
    if (ret) {
	for (s = argzero; *s; s++)
	    if (*s == '/') {
		if (*argzero == '.') {
		    if (argzero + 1 == s)
			++diddot;
		    else if (argzero[1] == '.' && argzero + 2 == s)
			++dotdot;
		}
		ret = source(argzero);
		break;
	    }
	if (!*s || (ret && isset(PATHDIRS) && diddot < 2 && dotdot == 0)) {
	    for (t = path; *t; t++) {
		if ((*t)[0] == '.' && !(*t)[1]) {
		    if (diddot)
			continue;
		    diddot = 1;
		    strcpy(buf, argzero);
		} else
		    sprintf(buf, "%s/%s", *t, argzero);
		if (access(buf, F_OK) == 0) {
		    ret = source(enam = buf);
		    break;
		}
	    }
	}
    }
    if (argv[1]) {
	freearray(pparams);
	pparams = old;
    }
    if (ret)
	zerrnam(name, "%e: %s", enam, errno);
    zsfree(argzero);
    argzero = old0;
    return ret ? ret : lastval;
}

int bin_set(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    struct option *opp;
    char **x;

    if (((ops['+'] && ops['-']) || !ops['-']) && !ops['@'] && !*argv) {
	showflag = ~0;
	showflag2 = ops[(int)'+'];
	listhtable(paramtab, (HFunc) printparam);
    }
    for (opp = optns; opp->name; opp++)
	if (ops[(int)opp->id] == 1)
	    opts[(int)opp->id] = OPT_SET;
	else if (ops[(int)opp->id] == 2)
	    opts[(int)opp->id] = OPT_UNSET;
    if (ops['A'] && !auxdata) {
	showflag = PMFLAG_A;
	showflag2 = ops[(int)'+'];
	listhtable(paramtab, (HFunc) printparam);
    }
    if (!*argv && !ops['-'])
	return 0;
    permalloc();
    x = arrdup(argv);
    heapalloc();
    if (ops['A'])
	setaparam(auxdata, x);
    else {
	freearray(pparams);
	pparams = x;
    }
    return 0;
}

#define pttime(X) printf("%ldm%lds",((long) (X))/3600,((long) (X))/60%60)

int bin_times(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    struct tms buf;

    if (times(&buf) == -1)
	return 1;
    pttime(buf.tms_utime);
    putchar(' ');
    pttime(buf.tms_stime);
    putchar('\n');
    pttime(buf.tms_cutime);
    putchar(' ');
    pttime(buf.tms_cstime);
    putchar('\n');
    return 0;
}

int bin_getopts(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    char *optstr = *argv++, *var = *argv++;
    char **args = (*argv) ? argv : pparams;
    static int optcind = 1, quiet;
    char *str, optbuf[3], *opch = optbuf + 1;
    int oldzoptind = zoptind;

    if (zoptind < 1)
	zoptind = 1;
    if (zoptind == 1)
	quiet = 0;
    optbuf[0] = '+';
    optbuf[1] = optbuf[2] = '\0';
    zsfree(zoptarg);
    zoptarg = ztrdup("");
    setsparam(var, ztrdup(""));
    if (*optstr == ':') {
	quiet = 1;
	optstr++;
    }
    if (zoptind > arrlen(args))
	return 1;
    str = args[zoptind - 1];
    if ((*str != '+' && *str != '-') || optcind >= (int)strlen(str) ||
	!strcmp("--", str)) {
	if (*str == '+' || *str == '-')
	    zoptind++;
	optcind = 0;
	return 1;
    }
    if (!optcind)
	optcind = 1;
    *opch = str[optcind++];
    if (!args[zoptind - 1][optcind]) {
	zoptind++;
	optcind = 0;
    }
    for (; *optstr; optstr++)
	if (*opch == *optstr)
	    break;
    if (!*optstr) {
	setsparam(var, ztrdup("?"));
	zoptind = oldzoptind;
	if (quiet) {
	    zsfree(zoptarg);
	    zoptarg = ztrdup(opch);
	    return 0;
	}
	zerr("bad option: %c", NULL, *opch);
	errflag = 0;
	return 0;
    }
    setsparam(var, ztrdup(opch - (*str == '+')));
    if (optstr[1] == ':') {
	if (!args[zoptind - 1]) {
	    if (quiet) {
		zsfree(zoptarg);
		zoptarg = ztrdup(opch);
		setsparam(var, ztrdup(":"));
		return 0;
	    }
	    setsparam(var, ztrdup("?"));
	    zerr("argument expected after %c option", NULL, *opch);
	    errflag = 0;
	    return 0;
	}
	zsfree(zoptarg);
	zoptarg = ztrdup(args[zoptind - 1] + optcind);
	zoptind++;
	optcind = 0;
    }
    return 0;
}

/* get a signal number from a string */

int getsignum(s)		/**/
char *s;
{
    int x = atoi(s), t0;

    if (idigit(*s) && x >= 0 && x < VSIGCOUNT)
	return x;
    for (t0 = 0; t0 != VSIGCOUNT; t0++)
	if (!strcmp(s, sigs[t0]))
	    return t0;
    return -1;
}

int bin_trap(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    List l;
    char *arg;

    if (!*argv) {
	int t0;

	for (t0 = 0; t0 != VSIGCOUNT; t0++)
	    if (sigtrapped[t0])
		if (!sigfuncs[t0])
		    printf("TRAP%s () {}\n", sigs[t0]);
		else {
		    char *s =
		    getpermtext((vptr) dupstruct((vptr) sigfuncs[t0]));

		    printf("TRAP%s () {\n\t%s\n}\n", sigs[t0], s);
		    zsfree(s);
		}
	return 0;
    }
    if ((getsignum(*argv) != -1) || (!strcmp(*argv, "-") && argv++)) {
	int t0;

	if (!*argv)
	    for (t0 = 0; t0 != VSIGCOUNT; t0++)
		unsettrap(t0);
	else
	    while (*argv)
		unsettrap(getsignum(*argv++));
	return 0;
    }
    arg = *argv++;
    if (!*arg)
	l = NULL;
    else if (!(l = parselstring(arg))) {
	zerrnam(name, "couldn't parse trap command", NULL, 0);
	return 1;
    }
    for (; *argv; argv++) {
	int sg = getsignum(*argv);

	if (sg == -1) {
	    zerrnam(name, "undefined signal: %s", *argv, 0);
	    break;
	}
	settrap(sg, l);
    }
    if (l)
	popheap();
    return errflag;
}

#ifdef RLIM_INFINITY
void printulimit(lim, hard)	/**/
int lim;
int hard;
{
    RLIM_TYPE t0;

    t0 = (hard) ? limits[lim].rlim_max : limits[lim].rlim_cur;
    switch (lim) {
    case RLIMIT_CPU:
	printf("cpu time (seconds)         ");
	break;
    case RLIMIT_FSIZE:
	printf("file size (blocks)         ");
	t0 /= 512;
	break;
    case RLIMIT_DATA:
	printf("data seg size (kbytes)     ");
	t0 /= 1024;
	break;
    case RLIMIT_STACK:
	printf("stack size (kbytes)        ");
	t0 /= 1024;
	break;
    case RLIMIT_CORE:
	printf("core file size (blocks)    ");
	t0 /= 512;
	break;
#ifdef RLIMIT_RSS
    case RLIMIT_RSS:
	printf("resident set size (kbytes) ");
	t0 /= 1024;
	break;
#endif
#ifdef RLIMIT_MEMLOCK
    case RLIMIT_MEMLOCK:
	printf("locked-in-memory size (kb) ");
	t0 /= 1024;
	break;
#endif
#ifdef RLIMIT_NPROC
    case RLIMIT_NPROC:
	printf("processes                  ");
	break;
#endif
#ifdef RLIMIT_OFILE
    case RLIMIT_OFILE:
	printf("open files                 ");
	break;
#endif
#ifdef RLIMIT_NOFILE
    case RLIMIT_NOFILE:
	printf("file descriptors           ");
	break;
#endif
#ifdef RLIMIT_VMEM
    case RLIMIT_VMEM:
	printf("virtual memory size (kb)   ");
	t0 /= 1024;
	break;
#endif
    }
    if (t0 == RLIM_INFINITY)
	printf("unlimited\n");
    else
	printf("%ld\n", (long)t0);
}
#endif

int bin_ulimit(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
#ifndef RLIM_INFINITY
    zwarnnam(name, "not available on this system", NULL, 0);
    return 1;
#else
    int res, hard;

    hard = ops['H'];
    if (ops['a'] || !ops['@'])
	res = -1;
    else if (ops['t'])
	res = RLIMIT_CPU;
    else if (ops['f'])
	res = RLIMIT_FSIZE;
    else if (ops['d'])
	res = RLIMIT_DATA;
    else if (ops['s'])
	res = RLIMIT_STACK;
    else if (ops['c'])
	res = RLIMIT_CORE;
#ifdef RLIMIT_MEMLOCK
    else if (ops['l'])
	res = RLIMIT_MEMLOCK;
#endif
#ifdef RLIMIT_RSS
    else if (ops['m'])
	res = RLIMIT_RSS;
#endif
#ifdef RLIMIT_NOFILE
    else if (ops['n'])
	res = RLIMIT_NOFILE;
#endif
#ifdef RLIMIT_OFILE
    else if (ops['o'])
	res = RLIMIT_OFILE;
#endif
#ifdef RLIMIT_NPROC
    else if (ops['p'])
	res = RLIMIT_NPROC;
#endif
#ifdef RLIMIT_VMEM
    else if (ops['v'])
	res = RLIMIT_VMEM;
#endif
    else {
	zwarnnam(name, "no such limit", NULL, 0);
	return 1;
    }
    if (res == -1)
	if (*argv) {
	    zwarnnam(name, "no arguments required after -a", NULL, 0);
	    return 1;
	} else {
	    int t0;

	    for (t0 = 0; t0 != RLIM_NLIMITS; t0++)
		printulimit(t0, hard);
	    return 0;
	}
    if (!*argv)
	printulimit(res, hard);
    else if (strcmp(*argv, "unlimited")) {
	RLIM_TYPE t0;

	t0 = (RLIM_TYPE) atol(*argv);
	switch (res) {
	case RLIMIT_FSIZE:
	case RLIMIT_CORE:
	    t0 *= 512;
	    break;
	case RLIMIT_DATA:
	case RLIMIT_STACK:
#ifdef RLIMIT_RSS
	case RLIMIT_RSS:
#endif
#ifdef RLIMIT_MEMLOCK
	case RLIMIT_MEMLOCK:
#endif
#ifdef RLIMIT_VMEM
	case RLIMIT_VMEM:
#endif
	    t0 *= 1024;
	    break;
	}
	if (hard) {
	    if (t0 > limits[res].rlim_max && geteuid()) {
		zwarnnam(name, "can't raise hard limits", NULL, 0);
		return 1;
	    }
	    limits[res].rlim_max = t0;
	} else {
	    if (t0 > limits[res].rlim_max) {
		if (geteuid()) {
		    zwarnnam(name, "value exceeds hard limit", NULL, 0);
		    return 1;
		}
		limits[res].rlim_max = limits[res].rlim_cur = t0;
	    } else
		limits[res].rlim_cur = t0;
	}
    } else {
	if (hard) {
	    if (geteuid()) {
		zwarnnam(name, "can't remove hard limits", NULL, 0);
		return 1;
	    }
	    limits[res].rlim_max = RLIM_INFINITY;
	} else
	    limits[res].rlim_cur = limits[res].rlim_max;
    }
    return 0;
#endif
}

int putraw(c)			/**/
int c;
{
    putchar(c);
    return 0;
}

int bin_echotc(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    char *s, buf[2048], *t, *u;
    int num, argct, t0;

    s = *argv++;
    if (!termok)
	return 1;
    if ((num = tgetnum(s)) != -1) {
	printf("%d\n", num);
	return 0;
    }
    if (tgetflag(s)) {
	puts("yes");
	return (0);
    }
    u = buf;
    t = tgetstr(s, &u);
    if (!t || !*t) {
	zwarnnam(name, "no such capability: %s", s, 0);
	return 1;
    }
    for (argct = 0, u = t; *u; u++)
	if (*u == '%') {
	    if (u++, (*u == 'd' || *u == '2' || *u == '3' || *u == '.' ||
		      *u == '+'))
		argct++;
	}
    if (arrlen(argv) != argct) {
	zwarnnam(name, (arrlen(argv) < argct) ? "not enough arguments" :
		 "too many arguments", NULL, 0);
	return 1;
    }
    if (!argct)
	tputs(t, 1, putraw);
    else {
	t0 = (argv[1]) ? atoi(argv[1]) : atoi(*argv);
	tputs(tgoto(t, atoi(*argv), t0), t0, putraw);
    }
    return 0;
}

int bin_pwd(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    printf("%s\n", pwd);
    return 0;
}

#define TEST_END 0
#define TEST_INPAR 1
#define TEST_OUTPAR 2
#define TEST_STR 3
#define TEST_AND 4
#define TEST_OR 5
#define TEST_NOT 6

static char **tsp;
static int *tip;

int bin_test(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    char **s;
    int cnt, *arr, *ap, last_expr = 0;
    Cond c;

    if (func == BIN_BRACKET) {
	for (s = argv; *s; s++);
	if (s == argv || strcmp(s[-1], "]")) {
	    zerrnam(name, "']' expected", NULL, 0);
	    return 1;
	}
	s[-1] = NULL;
    }
    if (!*argv)
	return 1;
    for (s = argv, cnt = 0; *s; s++, cnt++);
    ap = arr = (int *)alloc((cnt + 1) * sizeof *arr);
    for (s = argv; *s; s++, ap++)
	if (!strcmp(*s, "(")) {
	    *ap = TEST_INPAR;
	    last_expr = 0;
	} else if (!strcmp(*s, ")")) {
	    *ap = TEST_OUTPAR;
	    last_expr = 1;
	} else if (!strcmp(*s, "=") || !strcmp(*s, "!=")) {
	    zerrnam(name, "argument expected", NULL, 0);
	    return 1;
	} else if (s[1] && (!strcmp(s[1], "=") || !strcmp(s[1], "!=")
			    || (s[1][0] == '-' &&
				get_cond_num(s[1] + 1) > -1))) {
	    if (!s[2] || (!strcmp(s[2], "(") || !strcmp(s[2], ")"))) {
		zerrnam(name, "argument expected", NULL, 0);
		return 1;
	    }
	    ap[0] = ap[1] = ap[2] = TEST_STR;
	    ap += 2;
	    s += 2;
	    last_expr = 1;
	} else if (!strcmp(*s, "-a") && last_expr) {
	    *ap = TEST_AND;
	    last_expr = 0;
	} else if (!strcmp(*s, "-o") && last_expr) {
	    *ap = TEST_OR;
	    last_expr = 0;
	} else if (!strcmp(*s, "!") && !last_expr) {
	    *ap = TEST_NOT;
	    last_expr = 0;
	} else {
	    *ap = TEST_STR;
	    last_expr = 1;
	    if (s[1] && strcmp(s[1], "(") && strcmp(s[1], ")") &&
		**s == '-') {
		*++ap = TEST_STR;
		++s;
	    }
	}
    *ap = TEST_END;
    tsp = argv;
    tip = arr;
    c = partest(0);
    if (*tip != TEST_END || errflag) {
	zerrnam(name, "parse error", NULL, 0);
	return 1;
    }
    return (c) ? !evalcond(c) : 1;
}

Cond partest(level)		/**/
int level;
{
    Cond a, b;

    switch (level) {
    case 0:
	a = partest(1);
	if (*tip == TEST_OR) {
	    tip++, tsp++;
	    b = (Cond) makecond();
	    b->left = (vptr) a;
	    b->right = (vptr) partest(0);
	    b->type = COND_OR;
	    return b;
	}
	return a;
    case 1:
	a = partest(2);
	if (*tip == TEST_AND) {
	    tip++, tsp++;
	    b = (Cond) makecond();
	    b->left = (vptr) a;
	    b->right = (vptr) partest(1);
	    b->type = COND_AND;
	    return b;
	}
	return a;
    case 2:
	if (*tip == TEST_NOT) {
	    tip++, tsp++;
	    b = (Cond) makecond();
	    b->left = (vptr) partest(2);
	    b->type = COND_NOT;
	    return b;
	}
    case 3:
	if (*tip == TEST_INPAR) {
	    tip++, tsp++;
	    b = partest(0);
	    if (*tip != TEST_OUTPAR) {
		zerrnam("test", "parse error", NULL, 0);
		return NULL;
	    }
	    tip++, tsp++;
	    return b;
	}
	if (tip[0] != TEST_STR) {
	    zerrnam("test", "parse error", NULL, 0);
	    return NULL;
	} else if (tip[1] != TEST_STR) {
	    b = (Cond) makecond();
	    if (!strcmp(*tsp, "-t")) {
		b->left = (vptr) dupstring("1");
		b->type = 't';
	    } else {
		b->left = (vptr) tsp[0];
		b->type = 'n';
	    }
	    tip++, tsp++;
	    return b;
	} else if (tip[2] != TEST_STR) {
	    b = par_cond_double(tsp[0], tsp[1]);
	    tip += 2, tsp += 2;
	    return b;
	} else {
	    b = par_cond_triple(tsp[0], tsp[1], tsp[2]);
	    tip += 3, tsp += 3;
	    return b;
	}
    }
    return NULL;
}

int get_xcompctl(name, av, cc, isdef)	/**/
char *name;
char ***av;
Compctl cc;
int isdef;
{
    char **argv = *av, *t, *tt, sav;
    int n, l = 0, ready = 0, dummy;
    Compcond m, c, o;
    Compctl *next = &(cc->ext);

    while (!ready) {
	o = m = c = (Compcond) zcalloc(sizeof(*c));
	for (t = *argv; *t;) {
	    while (*t == ' ')
		t++;
	    switch (*t) {
	    case 's':
		c->type = CCT_CURSUF;
		break;
	    case 'S':
		c->type = CCT_CURPRE;
		break;
	    case 'p':
		c->type = CCT_POS;
		break;
	    case 'c':
		c->type = CCT_CURSTR;
		break;
	    case 'C':
		c->type = CCT_CURPAT;
		break;
	    case 'w':
		c->type = CCT_WORDSTR;
		break;
	    case 'W':
		c->type = CCT_WORDPAT;
		break;
	    case 'n':
		c->type = CCT_CURSUB;
		break;
	    case 'N':
		c->type = CCT_CURSUBC;
		break;
	    case 'm':
		c->type = CCT_NUMWORDS;
		break;
	    case 'r':
		c->type = CCT_RANGESTR;
		break;
	    case 'R':
		c->type = CCT_RANGEPAT;
		break;
	    default:
		t[1] = '\0';
		zerrnam(name, "unknown condition code: %s", t, 0);
		freecompctl(cc);
		zfree(m, sizeof(struct compcond));

		return 1;
	    }
	    if (t[1] != '[') {
		t[1] = '\0';
		zerrnam(name, "expected condition after condition code: %s", t, 0);
		freecompctl(cc);
		zfree(m, sizeof(struct compcond));

		return 1;
	    }
	    t++;
	    for (n = 0, tt = t; *tt == '['; n++) {
		for (l = 1, tt++; *tt && l; tt++)
		    if (*tt == '\\' && tt[1])
			tt++;
		    else if (*tt == '[')
			l++;
		    else if (*tt == ']')
			l--;
		    else if (l == 1 && *tt == ',')
			*tt = '\201';
		if (tt[-1] == ']')
		    tt[-1] = '\200';
	    }

	    if (l) {
		t[1] = '\0';
		zerrnam(name, "error after condition code: %s", t, 0);
		freecompctl(cc);
		zfree(m, sizeof(struct compcond));

		return 1;
	    }
	    c->n = n;

	    if (c->type == CCT_POS ||
		c->type == CCT_NUMWORDS) {
		c->u.r.a = (int *)zcalloc(n * sizeof(int));
		c->u.r.b = (int *)zcalloc(n * sizeof(int));
	    } else if (c->type == CCT_CURSUF ||
		       c->type == CCT_CURPRE)
		c->u.s.s = (char **)zcalloc(n * sizeof(char *));

	    else if (c->type == CCT_RANGESTR ||
		     c->type == CCT_RANGEPAT) {
		c->u.l.a = (char **)zcalloc(n * sizeof(char *));
		c->u.l.b = (char **)zcalloc(n * sizeof(char *));
	    } else {
		c->u.s.p = (int *)zcalloc(n * sizeof(int));
		c->u.s.s = (char **)zcalloc(n * sizeof(char *));
	    }
	    for (l = 0; *t == '['; l++, t++) {
		for (t++; *t && *t == ' '; t++);
		tt = t;
		if (c->type == CCT_POS ||
		    c->type == CCT_NUMWORDS) {
		    for (; *t && *t != '\201' && *t != '\200'; t++);
		    if (!(sav = *t)) {
			zerrnam(name, "error in condition", NULL, 0);
			freecompctl(cc);
			freecompcond(m);
			return 1;
		    }
		    *t = '\0';
		    c->u.r.a[l] = atoi(tt);
		    if (sav == '\200')
			c->u.r.b[l] = c->u.r.a[l];
		    else {
			tt = ++t;
			for (; *t && *t != '\200'; t++);
			if (!*t) {
			    zerrnam(name, "error in condition", NULL, 0);
			    freecompctl(cc);
			    freecompcond(m);
			    return 1;
			}
			*t = '\0';
			c->u.r.b[l] = atoi(tt);
		    }
		} else if (c->type == CCT_CURSUF ||
			   c->type == CCT_CURPRE) {
		    for (; *t && *t != '\200'; t++)
			if (*t == '\201')
			    *t = ',';
		    if (!*t) {
			zerrnam(name, "error in condition", NULL, 0);
			freecompctl(cc);
			freecompcond(m);
			return 1;
		    }
		    *t = '\0';
		    c->u.s.s[l] = ztrdup(tt);
		} else if (c->type == CCT_RANGESTR ||
			   c->type == CCT_RANGEPAT) {
		    for (; *t && *t != '\201'; t++);
		    if (!*t) {
			zerrnam(name, "error in condition", NULL, 0);
			freecompctl(cc);
			freecompcond(m);
			return 1;
		    }
		    *t = '\0';
		    c->u.l.a[l] = ztrdup(tt);
		    tt = ++t;
		    for (; *t && *t != '\200'; t++)
			if (*t == '\201')
			    *t = ',';
		    if (!*t) {
			zerrnam(name, "error in condition", NULL, 0);
			freecompctl(cc);
			freecompcond(m);
			return 1;
		    }
		    *t = '\0';
		    c->u.l.b[l] = ztrdup(tt);
		} else {
		    for (; *t && *t != '\201'; t++);
		    if (!*t) {
			zerrnam(name, "error in condition", NULL, 0);
			freecompctl(cc);
			freecompcond(m);
			return 1;
		    }
		    *t = '\0';
		    c->u.s.p[l] = atoi(tt);
		    tt = ++t;
		    for (; *t && *t != '\200'; t++)
			if (*t == '\201')
			    *t = ',';
		    if (!*t) {
			zerrnam(name, "error in condition", NULL, 0);
			freecompctl(cc);
			freecompcond(m);
			return 1;
		    }
		    *t = '\0';
		    c->u.s.s[l] = ztrdup(tt);
		}
	    }
	    while (*t == ' ')
		t++;
	    if (*t == ',') {
		o->or = c = (Compcond) zcalloc(sizeof(*c));
		o = c;
		t++;
	    } else if (*t) {
		c->and = (Compcond) zcalloc(sizeof(*c));
		c = c->and;
	    }
	}
	*next = (Compctl) zcalloc(sizeof(*cc));
	(*next)->cond = m;
	argv++;
	if (get_compctl(name, &argv, *next, &dummy, 0, isdef)) {
	    freecompctl(cc);
	    return 1;
	}
	if ((!argv || !*argv) &&
	    (cc == &cc_default || cc == &cc_compos))
	    ready = 1;
	else {
	    if (!argv || !*argv || **argv != '-' ||
		((!argv[0][1] || argv[0][1] == '+') && !argv[1])) {
		zerrnam(name, "missing command names", NULL, 0);
		freecompctl(cc);
		return 1;
	    }
	    if (!strcmp(*argv, "--"))
		ready = 1;
	    else if (!strcmp(*argv, "-+") && argv[1] && !strcmp(argv[1], "--")) {
		ready = 1;
		argv++;
	    }
	    argv++;
	    next = &((*next)->next);
	}
    }
    *av = argv - 1;
    return 0;
}

int get_compctl(name, av, cc, t, first, isdef)	/**/
char *name;
char ***av;
Compctl cc;
int *t;
int first;
int isdef;
{
    unsigned long flags = 0;
    Compctl cc2 = NULL;
    char **argv = *av, *usrkeys = NULL, *compglob = NULL, *str = NULL;
    char *funcn = NULL, *explain = NULL, *compprefix = NULL, *suffix = NULL;
    char *subcmd = NULL, *hpat = NULL;
    int ready = 0, hnum = 0, hx = 0;

    for (; !ready && *argv && **argv == '-';) {
	if (**argv == '-' && !(*argv)[1])
	    *argv = "-+";
	while (!ready && *++(*argv))
	    switch (**argv) {
	    case 'f':
		flags |= CC_FILES;
		break;
	    case 'c':
		flags |= CC_COMMPATH;
		break;
	    case 'o':
		flags |= CC_OPTIONS;
		break;
	    case 'v':
		flags |= CC_VARS;
		break;
	    case 'b':
		flags |= CC_BINDINGS;
		break;
	    case 'A':
		flags |= CC_ARRAYS;
		break;
	    case 'I':
		flags |= CC_INTVARS;
		break;
	    case 'F':
		flags |= CC_FUNCS;
		break;
	    case 'p':
		flags |= CC_PARAMS;
		break;
	    case 'E':
		flags |= CC_ENVVARS;
		break;
	    case 'j':
		flags |= CC_JOBS;
		break;
	    case 'r':
		flags |= CC_RUNNING;
		break;
	    case 'z':
		flags |= CC_STOPPED;
		break;
	    case 'B':
		flags |= CC_BUILTINS;
		break;
	    case 'a':
		flags |= CC_ALREG | CC_ALGLOB;
		break;
	    case 'R':
		flags |= CC_ALREG;
		break;
	    case 'G':
		flags |= CC_ALGLOB;
		break;
	    case 'u':
		flags |= CC_USERS;
		break;
	    case 'd':
		flags |= CC_DISCMDS;
		break;
	    case 'e':
		flags |= CC_EXCMDS;
		break;
	    case 'N':
		flags |= CC_SCALARS;
		break;
	    case 'O':
		flags |= CC_READONLYS;
		break;
	    case 'Z':
		flags |= CC_SPECIALS;
		break;
	    case 'q':
		flags |= CC_REMOVE;
		break;
	    case 'U':
		flags |= CC_DELETE;
		break;
	    case 'n':
		flags |= CC_NAMED;
		break;
	    case 'k':
		if ((*argv)[1]) {
		    usrkeys = (*argv) + 1;
		    *argv = "" - 1;
		} else if (!argv[1]) {
		    zerrnam(name, "variable name expected after -k", NULL, 0);
		    return 1;
		} else {
		    usrkeys = *++argv;
		    *argv = "" - 1;
		}
		break;
	    case 'K':
		if ((*argv)[1]) {
		    funcn = (*argv) + 1;
		    *argv = "" - 1;
		} else if (!argv[1]) {
		    zerrnam(name, "function name expected after -K", NULL, 0);
		    return 1;
		} else {
		    funcn = *++argv;
		    *argv = "" - 1;
		}
		break;
	    case 'X':
		if ((*argv)[1]) {
		    explain = (*argv) + 1;
		    *argv = "" - 1;
		} else if (!argv[1]) {
		    zerrnam(name, "string expected after -X", NULL, 0);
		    return 1;
		} else {
		    explain = *++argv;
		    *argv = "" - 1;
		}
		break;
	    case 'P':
		if (hx) {
		    zerrnam(name, "prefix definition in xor'd completion not allowed",
			    NULL, 0);
		    return 1;
		}
		if ((*argv)[1]) {
		    compprefix = (*argv) + 1;
		    *argv = "" - 1;
		} else if (!argv[1]) {
		    zerrnam(name, "string expected after -P", NULL, 0);
		    return 1;
		} else {
		    compprefix = *++argv;
		    *argv = "" - 1;
		}
		break;
	    case 'S':
		if (hx) {
		    zerrnam(name, "suffix definition in xor'd completion not allowed",
			    NULL, 0);
		    return 1;
		}
		if ((*argv)[1]) {
		    suffix = (*argv) + 1;
		    *argv = "" - 1;
		} else if (!argv[1]) {
		    zerrnam(name, "string expected after -S", NULL, 0);
		    return 1;
		} else {
		    suffix = *++argv;
		    *argv = "" - 1;
		}
		break;
	    case 'g':
		if ((*argv)[1]) {
		    compglob = (*argv) + 1;
		    *argv = "" - 1;
		} else if (!argv[1]) {
		    zerrnam(name, "glob pattern expected after -g", NULL, 0);
		    return 1;
		} else {
		    compglob = *++argv;
		    *argv = "" - 1;
		}
		break;
	    case 's':
		if ((*argv)[1]) {
		    str = (*argv) + 1;
		    *argv = "" - 1;
		} else if (!argv[1]) {
		    zerrnam(name, "command string expected after -s", NULL, 0);
		    return 1;
		} else {
		    str = *++argv;
		    *argv = "" - 1;
		}
		break;
	    case 'l':
		if ((*argv)[1]) {
		    subcmd = (*argv) + 1;
		    *argv = "" - 1;
		} else if (!argv[1]) {
		    zerrnam(name, "command name expected after -s", NULL, 0);
		    return 1;
		} else {
		    subcmd = *++argv;
		    *argv = "" - 1;
		}
		break;
	    case 'H':
		if ((*argv)[1])
		    hnum = atoi((*argv) + 1);
		else if (argv[1])
		    hnum = atoi(*++argv);
		else {
		    zerrnam(name, "number expected after -H", NULL, 0);
		    return 1;
		}
		if (!argv[1]) {
		    zerrnam(name, "missing pattern after -H", NULL, 0);
		    return 1;
		}
		hpat = *++argv;
		if (hnum < 1)
		    hnum = 0;
		if (*hpat == '*' && !hpat[1])
		    hpat = "";
		*argv = "" - 1;
		break;
	    case 'C':
		if (first && !hx) {
		    Compctl c2;

		    cc = cc2 = &cc_compos;

		    c2 = (Compctl) zcalloc(sizeof *cc2);
		    c2->xor = cc2->xor;
		    c2->ext = cc2->ext;
		    c2->refc = 1;

		    freecompctl(c2);

		    cc2->ext = cc2->xor = NULL;
		} else {
		    zerrnam(name, "misplaced command completion (-C) flag", NULL, 0);
		    return 1;
		}
		break;
	    case 'D':
		if (first && !hx) {
		    Compctl c2;

		    cc = cc2 = &cc_default, isdef = 1;
		    c2 = (Compctl) zcalloc(sizeof *cc2);
		    c2->xor = cc2->xor;
		    c2->ext = cc2->ext;
		    c2->refc = 1;

		    freecompctl(c2);

		    cc2->ext = cc2->xor = NULL;
		} else {
		    zerrnam(name, "misplaced default completion (-D) flag", NULL, 0);
		    return 1;
		}
		break;
	    case 'x':
		if (!argv[1]) {
		    zerrnam(name, "condition expected after -x", NULL, 0);
		    return 1;
		}
		if (first) {
		    argv++;
		    if (get_xcompctl(name, &argv, cc, isdef))
			return 2;
		    ready = 2;
		} else {
		    zerrnam(name, "recursive extended completion not allowed",
			    NULL, 0);
		    return 1;
		}
		break;
	    default:
		if (!first && (**argv == '-' || **argv == '+'))
		    (*argv)--, argv--, ready = 1;
		else {
		    zerrnam(name, "bad option: %c", NULL, **argv);
		    return 1;
		}
	    }

	if (*++argv && (!ready || ready == 2) && **argv == '+' && !argv[0][1]) {
	    hx = 1;
	    ready = 0;

	    if (subcmd &&
		(usrkeys || compglob || str || funcn || explain || compprefix ||
		 suffix || hpat || flags)) {
		zerrnam(name, "illegal combination of options", NULL, 0);
		return 1;
	    }
	    cc->mask = flags;
	    zsfree(cc->keyvar);
	    zsfree(cc->glob);
	    zsfree(cc->str);
	    zsfree(cc->func);
	    zsfree(cc->explain);
	    zsfree(cc->prefix);
	    zsfree(cc->suffix);
	    zsfree(cc->subcmd);
	    zsfree(cc->hpat);

	    cc->mask = flags;
	    cc->keyvar = ztrdup(usrkeys);
	    cc->glob = ztrdup(compglob);
	    cc->str = ztrdup(str);
	    cc->func = ztrdup(funcn);
	    cc->explain = ztrdup(explain);
	    cc->prefix = ztrdup(compprefix);
	    cc->suffix = ztrdup(suffix);
	    cc->subcmd = ztrdup(subcmd);
	    cc->hpat = ztrdup(hpat);
	    cc->hnum = hnum;

	    if (!*++argv || **argv != '-' ||
		(**argv == '-' && (!argv[0][1] ||
				   (argv[0][1] == '-' && !argv[0][2])))) {
		if (isdef) {
		    zerrnam(name, "recursive xor'd default completions not allowed",
			    NULL, 0);
		    return 1;
		}
		cc->xor = &cc_default;
		if (!*argv || **argv == '-') {
		    if (*argv)
			(*argv)--;
		    argv--;
		    ready = 1;
		}
	    } else {
		cc->xor = (Compctl) zcalloc(sizeof(*cc));
		cc = cc->xor;
		flags = 0;
		usrkeys = NULL;
		compglob = NULL;
		str = NULL;
		funcn = NULL;
		explain = NULL;
		compprefix = NULL;
		suffix = NULL;
		subcmd = NULL;
		hpat = NULL;
		hnum = 0;
	    }
	}
    }

    if (subcmd &&
	(usrkeys || compglob || str || funcn || explain || compprefix || suffix ||
	 hpat || flags)) {
	zerrnam(name, "illegal combination of options", NULL, 0);
	return 1;
    }
    if (cc2)
	*t = 1;

    cc->mask = flags;
    zsfree(cc->keyvar);
    zsfree(cc->glob);
    zsfree(cc->str);
    zsfree(cc->func);
    zsfree(cc->explain);
    zsfree(cc->prefix);
    zsfree(cc->suffix);
    zsfree(cc->subcmd);
    zsfree(cc->hpat);

    cc->mask = flags;
    cc->keyvar = ztrdup(usrkeys);
    cc->glob = ztrdup(compglob);
    cc->str = ztrdup(str);
    cc->func = ztrdup(funcn);
    cc->explain = ztrdup(explain);
    cc->prefix = ztrdup(compprefix);
    cc->suffix = ztrdup(suffix);
    cc->subcmd = ztrdup(subcmd);
    cc->hpat = ztrdup(hpat);
    cc->hnum = hnum;
    *av = argv;

    return 0;
}

int bin_compctl(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    Compctl cc = NULL;
    int t = 0, t2;
    unsigned long flags = 0;

    if (*argv) {
	cc = (Compctl) zcalloc(sizeof(*cc));

	if ((t2 = get_compctl(name, &argv, cc, &t, 1, 0))) {
	    if (t2 == 1)
		freecompctl(cc);
	    return 1;
	}
	if (*argv)
	    compctl_process_cc(argv, cc);

	flags = cc->mask;

	if (!*argv)
	    freecompctl(cc);
    }
    if (!*argv && !t) {
	showflag = flags;
	listhtable(compctltab, (HFunc) printcompctlp);
	printcompctl("COMMAND", &cc_compos);
	printcompctl("DEFAULT", &cc_default);
    }
    return 0;
}

void printif(str, c)		/**/
char *str;
int c;
{
    if (str)
	printf("-%c \"%s\" ", c, str);
}

void printcompctlp(s, ccp)	/**/
char *s;
Compctlp ccp;
{
    printcompctl(s, ccp->cc);
}

void printcompctl(s, cc)	/**/
char *s;
Compctl cc;
{
    char *css = "fcqovbAIFpEjrzBRGudeNOZUn";
    char *mss = " pcCwWsSnNmrR";
    unsigned long t = 0x7fffffff;

    if (cc->mask & showflag) {
	if (s)
	    puts(s);
    } else if (!showflag) {
	unsigned long flags = cc->mask;

	if (s)
	    printf("%s ", s);
	if (flags & t) {
	    putchar('-');
	    if ((flags & (CC_ALREG | CC_ALGLOB)) == (CC_ALREG | CC_ALGLOB))
		putchar('a'), flags &= ~(CC_ALREG | CC_ALGLOB);
	    while (*css) {
		if ((flags & 1) && (t & 1))
		    putchar(*css);
		css++;
		flags >>= 1;
		t >>= 1;
	    }
	    putchar(' ');
	}
	flags = cc->mask;
	if (cc->keyvar)
	    printf(*cc->keyvar == '(' ? "-k \"%s\" " : "-k %s ", cc->keyvar);
	printif(cc->func, 'K');
	printif(cc->explain, 'X');
	printif(cc->prefix, 'P');
	printif(cc->suffix, 'S');
	printif(cc->glob, 'g');
	printif(cc->str, 's');
	printif(cc->subcmd, 'l');
	if (cc->hpat)
	    printf("-H %d \"%s\" ", cc->hnum, cc->hpat);
	if (cc->ext) {
	    Compcond c, o;
	    int i;

	    cc = cc->ext;
	    printf("-x ");

	    while (cc) {
		c = cc->cond;

		putchar('\"');
		for (c = cc->cond; c;) {
		    o = c->or;
		    while (c) {
			putchar(mss[c->type]);

			for (i = 0; i < c->n; i++)
			    switch (c->type) {
			    case CCT_POS:
			    case CCT_NUMWORDS:
				printf("[%d,%d]", c->u.r.a[i], c->u.r.b[i]);
				break;
			    case CCT_CURSUF:
			    case CCT_CURPRE:
				printf("[%s]", c->u.s.s[i]);
				break;
			    case CCT_RANGESTR:
			    case CCT_RANGEPAT:
				printf("[%s,%s]", c->u.l.a[i], c->u.l.b[i]);
				break;
			    default:
				printf("[%d,%s]", c->u.s.p[i], c->u.s.s[i]);
			    }
			if ((c = c->and))
			    putchar(' ');
		    }
		    if ((c = o))
			printf(" , ");
		}
		printf("\" ");
		c = cc->cond;
		cc->cond = NULL;
		printcompctl(NULL, cc);
		cc->cond = c;
		if ((cc = (Compctl) (cc->next)))
		    printf("- ");
	    }
	}
	if (cc && cc->xor) {
	    printf("+ ");
	    if (cc->xor != &cc_default)
		printcompctl(NULL, cc->xor);
	}
	if (s)
	    putchar('\n');
    }
}

void compctl_process(s, mask, uk, gl, st, fu, ex, pr, su, sc, hp, hn)	/**/
char **s;
int mask;
char *uk;
char *gl;
char *st;
char *fu;
char *ex;
char *pr;
char *su;
char *sc;
char *hp;
int hn;
{
    Compctl cc;

    cc = (Compctl) zcalloc(sizeof *cc);
    cc->mask = mask;
    cc->keyvar = ztrdup(uk);
    cc->glob = ztrdup(gl);
    cc->str = ztrdup(st);
    cc->func = ztrdup(fu);
    cc->explain = ztrdup(ex);
    cc->prefix = ztrdup(pr);
    cc->suffix = ztrdup(su);
    cc->subcmd = ztrdup(sc);
    cc->hpat = ztrdup(hp);
    cc->hnum = hn;

    compctl_process_cc(s, cc);
}

void compctl_process_cc(s, cc)	/**/
char **s;
Compctl cc;
{
    Compctlp ccp;

    cc->refc = 0;
    for (; *s; s++) {
	cc->refc++;
	ccp = (Compctlp) zalloc(sizeof *ccp);
	ccp->cc = cc;
	addhnode(ztrdup(*s), ccp, compctltab, freecompctlp);
    }
}

int bin_ttyctl(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    if (!ops['@'])
	printf("tty is %sfrozen\n", ttyfrozen ? "" : "not ");
    else if (ops['f'])
	ttyfrozen = 1;
    else
	ttyfrozen = 0;
    return 0;
}
