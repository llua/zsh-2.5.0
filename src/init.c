/*
 *
 * init.c - main loop and initialization routines
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

#define GLOBALS
#include "zsh.h"
#include <pwd.h>

static int noexitct = 0;

void main(argc, argv, envp)	/**/
int argc;
char **argv;
char **envp;
{
    char *zshname;

#ifdef LC_ALL
    setlocale(LC_ALL, "");
#endif
    environ = envp;
    permalloc();
    if (!(zshname = strrchr(argv[0], '/')))
	zshname = argv[0];
    else
	zshname++;
    if (*zshname == '-')
	zshname++;
    setflags(zshname);
    parseargs(argv);
    setmoreflags();
    setupvals(zshname);
    initialize();
    heapalloc();
    runscripts(zshname);
    for (;;) {
	do
	    loop(1);
	while (tok != ENDINPUT);
	if (!(isset(IGNOREEOF) && interact)) {
#if 0
	    if (interact)
		fputs(islogin ? "logout\n" : "exit\n", stderr);
#endif
	    zexit(lastval);
	    continue;
	}
	noexitct++;
	if (noexitct >= 10) {
	    stopmsg = 1;
	    zexit(lastval);
	}
	zerrnam("zsh", (!islogin) ? "use 'exit' to exit."
		: "use 'logout' to logout.", NULL, 0);
    }
}

/* keep executing lists until EOF found */

void loop(toplevel)		/**/
int toplevel;
{
    List list;

    pushheap();
    for (;;) {
	freeheap();
	errflag = 0;
	if (interact && isset(SHINSTDIN))
	    preprompt();
	hbegin();		/* init history mech */
	intr();			/* interrupts on */
	ainit();		/* init alias mech */
	lexinit();
	if (!(list = parse_event())) {	/* if we couldn't parse a list */
	    hend();
	    if (tok == ENDINPUT && !errflag)
		break;
	    continue;
	}
	if (hend()) {
	    if (stopmsg)	/* unset 'you have stopped jobs' flag */
		stopmsg--;
	    execlist(list);
	    if (toplevel)
		noexitct = 0;
	}
	if (ferror(stderr)) {
	    zerr("write error", NULL, 0);
	    clearerr(stderr);
	}
	if (subsh)		/* how'd we get this far in a subshell? */
	    exit(lastval);
	if (((!interact || sourcelevel) && errflag) || retflag)
	    break;
	if (trapreturn) {
	    lastval = trapreturn;
	    trapreturn = 0;
	}
	if (isset('t') && toplevel) {
	    if (sigtrapped[SIGEXIT])
		dotrap(SIGEXIT);
	    exit(lastval);
	}
    }
    popheap();
}

void setflags(zshname)		/**/
const char *zshname;
{
    int c;

    for (c = 0; c != 43; c++)
	opts[c] = OPT_UNSET;
    for (c = 43; c != 128; c++)
	opts[c] = OPT_INVALID;
    for (c = 'a'; c <= 'z'; c++)
	opts[c] = opts[c - 'a' + 'A'] = OPT_UNSET;
    for (c = '0'; c <= '9'; c++)
	opts[c] = OPT_UNSET;
    opts['A'] = OPT_INVALID;
    opts['i'] = (isatty(0)) ? OPT_SET : OPT_UNSET;
    opts[BGNICE] = opts[NOTIFY] = OPT_SET;
    opts[USEZLE] = (interact && SHTTY != -1) ? OPT_SET : OPT_UNSET;
    opts[HASHCMDS] = opts[HASHLISTALL] = opts[HASHDIRS] = OPT_SET;

/* Bourne/Korn mode:
		The following seven options cause zsh to behave more like
		Bourne and Korn shells when invoked as "sh" or "ksh".
		K - don't recognize csh-style history subst
		k - allow interactive comments
		I - don't perform brace expansion
		3 - don't print error for unmatched wildcards
		H - don't query 'rm *'
		y - split parameters using IFS
		KSHOPTIONPRINT - print options ksh-like
		 -- wnp@rcvie.co.at, 1992-05-14
	*/

    if (strcmp(zshname, "sh") == 0 || strcmp(zshname, "ksh") == 0) {
	opts['K'] = opts['k'] = opts['I'] = opts['3'] = OPT_SET;
	opts['H'] = opts['y'] = opts[KSHOPTIONPRINT] = OPT_SET;
	opts[GLOBSUBST] = OPT_SET;
    }
}

static char *cmd;
static int opti = OPT_UNSET;

void parseargs(argv)		/**/
char **argv;
{
    char **x;
    int bk = 0, action;
    Lklist paramlist;

    hackzero = argzero = *argv;
    opts[LOGINSHELL] = (**(argv++) == '-') ? OPT_SET : OPT_UNSET;
    SHIN = 0;
    while (!bk && *argv && (**argv == '-' || **argv == '+')) {
	action = (**argv == '-') ? OPT_SET : OPT_UNSET;
	while (*++*argv) {
	    if ((bk = (**argv == 'b' || **argv == '-')))
		break;
	    if (opts[(int)**argv] == OPT_INVALID) {
		zerr("bad option: -%c", NULL, **argv);
		exit(1);
	    }
	    if (**argv == 'c') {/* -c command */
		argv++;
		if (!*argv) {
		    zerr("string expected after -c", NULL, 0);
		    exit(1);
		}
		cmd = *argv;
		opts[INTERACTIVE] = opti;
		opts['c'] = OPT_SET;
		bk = 1;
		break;
	    } else if (**argv == 'o') {
		int c;

		if (!*++*argv)
		    argv++;
		if (!*argv) {
		    zerr("string expected after -o", NULL, 0);
		    exit(1);
		}
		c = optlookup(*argv);
		if (c == -1)
		    zerr("no such option: %s", *argv, 0);
		else {
		    if (c == 'i')
			opti = action;
		    opts[c] = action;
		}
		break;
	    } else {
		if (**argv == 'i')
		    opti = action;
		opts[(int)**argv] = action;
	    }
	}
	argv++;
    }
    paramlist = newlist();
    if (*argv) {
	if (opts[SHINSTDIN] == OPT_UNSET) {
	    argzero = *argv;
	    if (opts['c'] == OPT_UNSET)
		SHIN = movefd(open(argzero, O_RDONLY));
	    if (SHIN == -1) {
		zerr("can't open input file: %s", argzero, 0);
		exit(1);
	    }
	    opts[INTERACTIVE] = opti;
	    argv++;
	}
	while (*argv)
	    addnode(paramlist, ztrdup(*argv++));
    } else
	opts[SHINSTDIN] = OPT_SET;
    pparams = x = (char **)zcalloc((countnodes(paramlist) + 1) * sizeof(char *));

    while ((*x++ = (char *)getnode(paramlist)));
    free(paramlist);
    argzero = ztrdup(argzero);
}

void setmoreflags()
{				/**/
#ifndef NOCLOSEFUNNYFDS
    int t0;

#endif
    long ttpgrp;

/* stdout,stderr fully buffered */
#ifdef _IOFBF
    setvbuf(stdout, malloc(BUFSIZ), _IOFBF, BUFSIZ);
    setvbuf(stderr, malloc(BUFSIZ), _IOFBF, BUFSIZ);
#else
    setbuffer(stdout, malloc(BUFSIZ), BUFSIZ);
    setbuffer(stderr, malloc(BUFSIZ), BUFSIZ);
#endif
    subsh = 0;
#ifndef NOCLOSEFUNNYFDS
/* this works around a bug in some versions of in.rshd */
    if (isset('c'))
	for (t0 = 3; t0 != 10; t0++)
	    close(t0);
#endif
#ifdef JOB_CONTROL
    opts[MONITOR] = (interact) ? OPT_SET : OPT_UNSET;
    if (jobbing) {
	SHTTY = movefd((isatty(0)) ? dup(0) : open("/dev/tty", O_RDWR));
	if (SHTTY == -1)
	    opts[MONITOR] = OPT_UNSET;
	else {
#if defined(TIOCSETD) && defined(NTTYDISC)
	    int ldisc = NTTYDISC;

	    ioctl(SHTTY, TIOCSETD, (char *)&ldisc);
#endif
	    gettyinfo(&shttyinfo);	/* get tty state */
#if defined(__sgi)
	    if (shttyinfo.tio.c_cc[VSWTCH] <= 0)	/* hack for irises */
		shttyinfo.tio.c_cc[VSWTCH] = CSWTCH;
#endif
	}
#if defined(__sgi) || defined(__386BSD__) || defined(__NetBSD__) || defined(__FreeBSD__)
	attachtty(GETPGRP());
#endif
	if ((mypgrp = GETPGRP()) <= 0)
	    opts[MONITOR] = OPT_UNSET;
	else
	    while ((ttpgrp = gettygrp()) != -1 && ttpgrp != mypgrp) {
		sleep(1);
		mypgrp = GETPGRP();
		if (mypgrp == gettygrp())
		    break;
		killpg(mypgrp, SIGTTIN);
		mypgrp = GETPGRP();
	    }
    } else
	SHTTY = -1;
#else
    opts[MONITOR] = OPT_UNSET;
    SHTTY = movefd((isatty(0)) ? dup(0) : open("/dev/tty", O_RDWR));
    if (SHTTY != -1)
	gettyinfo(&shttyinfo);
#endif
}

SPROTO(long get_baudrate, (int speedcode));

static long get_baudrate(speedcode)
int speedcode;
{
    switch (speedcode) {
    case B0:
	return (0L);
    case B50:
	return (50L);
    case B75:
	return (75L);
    case B110:
	return (110L);
    case B134:
	return (134L);
    case B150:
	return (150L);
    case B200:
	return (200L);
    case B300:
	return (300L);
    case B600:
	return (600L);
#ifdef _B900
    case _B900:
	return (900L);
#endif
    case B1200:
	return (1200L);
    case B1800:
	return (1800L);
    case B2400:
	return (2400L);
#ifdef _B3600
    case _B3600:
	return (3600L);
#endif
    case B4800:
	return (4800L);
#ifdef _B7200
    case _B7200:
	return (7200L);
#endif
    case B9600:
	return (9600L);
#ifdef B19200
    case B19200:
	return (19200L);
#else
#ifdef EXTA
    case EXTA:
	return (19200L);
#endif
#endif
#ifdef B38400
    case B38400:
	return (38400L);
#else
#ifdef EXTB
    case EXTB:
	return (38400L);
#endif
#endif
    default:
	break;
    }
    return (0L);
}

void setupvals(zshname)		/**/
const char *zshname;
{
    struct passwd *pswd;
    char *ptr;
    struct timezone dummy_tz;

    noeval = 0;
    curhist = 0;
    histsiz = DEFAULT_HISTSIZE;
    lithistsiz = 5;
    inithist();
    mailcheck = logcheck = 60;
    keytimeout = 40;
    dirstacksize = -1;
    listmax = 100;
    clwords = (char **)zcalloc((clwsize = 16) * sizeof(char *));

    cmdstack = (unsigned char *)zalloc(256);
    cmdsp = 0;

    reporttime = -1;
    bangchar = '!';
    hashchar = '#';
    hatchar = '^';
    termok = 0;
    curjob = prevjob = coprocin = coprocout = -1;
    gettimeofday(&shtimer, &dummy_tz);	/* init $SECONDS */
    srand((unsigned int)shtimer.tv_sec);
/* build various hash tables; argument to newhtable is table size */
    aliastab = newhtable(37);
    addreswords();
    addhnode(ztrdup("run-help"), mkanode(ztrdup("man"), 1), aliastab, NULL);
    addhnode(ztrdup("which-command"),
	     mkanode(ztrdup("whence"), 1), aliastab, NULL);
    paramtab = newhtable(151);
    cmdnamtab = newhtable(37);
    compctltab = newhtable(13);
    initxbindtab();

/* Bourne/Korn mode:
		The following variable assignments cause zsh to behave more
		like Bourne and Korn shells when invoked as "sh" or "ksh".
		NULLCMD=":"
		READNULLCMD=":"
		 -- chip@fin.uucp, 1992-06-15
	*/
    if (strcmp(zshname, "sh") == 0 || strcmp(zshname, "ksh") == 0) {
	nullcmd = ztrdup(":");
	readnullcmd = ztrdup(":");
    } else {
	nullcmd = ztrdup("cat");
	readnullcmd = ztrdup("more");
    }

    prompt = ztrdup("%m%# ");
    prompt2 = ztrdup("> ");
    prompt3 = ztrdup("?# ");
    prompt4 = ztrdup("+ ");
    sprompt = ztrdup("zsh: correct `%R' to `%r' [nyae]? ");
    term = ztrdup("");
    ppid = getppid();
#ifdef HAS_TIO
#if defined(HAS_TCCRAP) && defined(HAS_TERMIOS)
    baud = cfgetospeed(&shttyinfo.tio);
    if (baud < 100)
	baud = get_baudrate((int)baud);	/* aren't "standards" great?? */
#else
    baud = get_baudrate(shttyinfo.tio.c_cflag & CBAUD);
#endif
#else
    baud = get_baudrate(shttyinfo.sgttyb.sg_ospeed);
#endif
#ifdef TIOCGWINSZ
    if (!(columns = shttyinfo.winsize.ws_col))
	columns = 80;
    if (!(lines = shttyinfo.winsize.ws_row))
	lines = 24;
#else
    columns = 80;
    lines = 24;
#endif
    ifs = ztrdup(" \t\n");
    timefmt = ztrdup(DEFTIMEFMT);
    watchfmt = ztrdup(DEFWATCHFMT);
    if (!(ttystrname = ztrdup(ttyname(SHTTY))))
	ttystrname = ztrdup("");
    wordchars = ztrdup(DEFWORDCHARS);
    fceditparam = ztrdup(DEFFCEDIT);
    tmpprefix = ztrdup(DEFTMPPREFIX);
    postedit = ztrdup("");
    hostnam = (char *)zalloc(256);
    underscore = ztrdup("");
    gethostname(hostnam, 256);
    mypid = getpid();
    cdpath = mkarray(NULL);
    manpath = mkarray(NULL);
    fignore = mkarray(NULL);
    fpath = mkarray(NULL);
    mailpath = mkarray(NULL);
    watch = mkarray(NULL);
    psvar = mkarray(NULL);
    compctlsetup();
    namdirs = (Nameddirs) zcalloc(sizeof(*namdirs) * 2);
    userdirsz = 2;
    userdirct = 0;
    zoptarg = ztrdup("");
    zoptind = 1;
    schedcmds = NULL;
    path = (char **)zalloc(4 * sizeof *path);
    path[0] = ztrdup("/bin");
    path[1] = ztrdup("/usr/bin");
    path[2] = ztrdup("/usr/ucb");
    path[3] = NULL;
    inittyptab();
    initlextabs();
    setupparams();

    if (!home ||
#ifdef SYSV
	!(username = cuserid(NULL))
#else
	!(username = getlogin())
#endif
	) {
	if ((pswd = getpwuid(getuid()))) {
	    username = ztrdup(pswd->pw_name);
	    if (!home)
		home = ztrdup(pswd->pw_dir);
	} else {
	    username = ztrdup("");
	    if (!home)
		home = ztrdup("/");
	}
    }
    if ((ptr = zgetenv("LOGNAME")))
	zlogname = ztrdup(ptr);
    else
	zlogname = ztrdup(username);

    if (ispwd(home))
	pwd = ztrdup(home);
    else if ((ptr = zgetenv("PWD")) && ispwd(ptr))
	pwd = ztrdup(ptr);
    else
	pwd = zgetwd();
    oldpwd = ztrdup(pwd);

    setparams();

    inittyptab();
    if (!strcmp(term, "emacs"))
	opts[USEZLE] = OPT_UNSET;
#ifndef HAS_RUSAGE
    times(&shtms);
#endif
}

void compctlsetup()
{				/**/
    static char
        *os[] =
    {"setopt", "unsetopt", NULL}, *vs[] =
    {"export", "typeset", "vared", "unset", NULL}, *cs[] =
    {"which", "builtin", NULL}, *bs[] =
    {"bindkey", NULL};

    compctl_process(os, CC_OPTIONS, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		    NULL, NULL, 0);
    compctl_process(vs, CC_VARS, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		    NULL, NULL, 0);
    compctl_process(bs, CC_BINDINGS, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		    NULL, NULL, 0);
    compctl_process(cs, CC_COMMPATH, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		    NULL, NULL, 0);
    cc_compos.mask = CC_COMMPATH;
    cc_default.refc = 10000;
    cc_default.mask = CC_FILES;
}

void initialize()
{				/**/
#ifdef RLIM_INFINITY
    int t0;

#endif

    breaks = loops = 0;
    lastmailcheck = time(NULL);
    locallist = NULL;
    locallevel = sourcelevel = 0;
    trapreturn = 0;
    dirstack = newlist();
    bufstack = newlist();
    newcmdnamtab();
    inbuf = (char *)zalloc(inbufsz = 256);
    inbufptr = inbuf + inbufsz;
    inbufct = 0;
#ifdef RLIM_INFINITY
    for (t0 = 0; t0 != RLIM_NLIMITS; t0++)
	getrlimit(t0, limits + t0);
#endif
    hsubl = hsubr = NULL;
    lastpid = 0;
    bshin = fdopen(SHIN, "r");
    intr();
#ifndef QDEBUG
    sig_ignore(SIGQUIT);
#endif
    sig_handle(SIGHUP);
    sig_handle(SIGCHLD);
    if (interact) {
	sig_handle(SIGALRM);
#ifdef SIGWINCH
	sig_handle(SIGWINCH);
#endif
	sig_ignore(SIGTERM);
    }
    if (jobbing) {
	long ttypgrp;

	while ((ttypgrp = gettygrp()) != -1 && ttypgrp != mypgrp)
	    kill(0, SIGTTIN);
	if (ttypgrp == -1) {
	    opts[MONITOR] = OPT_UNSET;
	} else {
	    sig_ignore(SIGTTOU);
	    sig_ignore(SIGTSTP);
	    sig_ignore(SIGTTIN);
	    sig_ignore(SIGPIPE);
	    attachtty(mypgrp);
	}
    }
#ifdef SIGNAL_MASKS
    if (islogin) {
	sig_setmask(sig_mask(0));
    } else if (interact) {
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	sig_unblock(set);
    }
#endif				/* SIGNAL_MASKS */
}

void addreswords()
{				/**/
    static char *reswds[] =
    {
	"do", "done", "esac", "then", "elif", "else", "fi", "for", "case",
	"if", "while", "function", "repeat", "time", "until", "exec", "command",
	"select", "coproc", "noglob", "-", "nocorrect", "foreach", "end", NULL
    };
    int t0;

    for (t0 = 0; reswds[t0]; t0++)
	addhnode(ztrdup(reswds[t0]), mkanode(NULL, -1 - t0), aliastab, NULL);
}

void runscripts(zshname)	/**/
char *zshname;
{
/*
	   KSH Mode:
	   if called as "ksh", we source the standard
	   sh/ksh scripts:
	   wnp@rcvie.co.at 1992/05/14
	 */

    sourcelevel = 32768;	/* hack to avoid errexit in init scripts */

    if (strcmp(zshname, "ksh") == 0) {
	if (islogin)
	    source("/etc/profile");
	if (islogin)
	    sourcehome(".profile");
	if (!isset(KSHPRIV))
	    source(getsparam("ENV"));
	else
	    source("/etc/suid_profile");
    } else {
#ifdef GLOBALZSHENV
	source(GLOBALZSHENV);
#endif
	if (!isset(NORCS)) {
	    sourcehome(".zshenv");
	    if (islogin) {
#ifdef GLOBALZPROFILE
		source(GLOBALZPROFILE);
#endif
		sourcehome(".zprofile");
	    }
	    if (interact) {
#ifdef GLOBALZSHRC
		source(GLOBALZSHRC);
#endif
		sourcehome(".zshrc");
	    }
	    if (islogin) {
#ifdef GLOBALZLOGIN
		source(GLOBALZLOGIN);
#endif
		sourcehome(".zlogin");
	    }
	}
    }
    sourcelevel = 0;

    if (isset('c')) {
	if (SHIN >= 10)
	    close(SHIN);
	SHIN = movefd(open("/dev/null", O_RDONLY));
	execstring(cmd);
	stopmsg = 1;
	zexit(lastval);
    }
    if (interact && !isset(NORCS))
	readhistfile(getsparam("HISTFILE"), 0);
#ifdef TIOCGWINSZ
    adjustwinsize();
#endif
    if (isset('t') && opti == OPT_UNSET)
	prompt = ztrdup("");
}

void ainit()
{				/**/
    alstackind = 0;		/* reset alias stack */
    alstat = 0;
    isfirstln = 1;
}
