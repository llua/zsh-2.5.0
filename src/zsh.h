/*
 *
 * zsh.h - standard header file
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

#include "config.h"

#ifdef HAS_UNISTD
#include <unistd.h>
#endif
#ifdef _POSIX_VERSION
#define POSIX 1
#endif

#ifdef __NeXT__
#include <bsd/libc.h>
#endif

#include <stdio.h>
#include <ctype.h>

#ifdef HAS_STRING
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef HAS_MEMORY
#include <memory.h>
#endif

#ifdef HAS_LOCALE
#include <locale.h>
#endif

#ifdef HAS_STDLIB
#include <stdlib.h>
#endif

#ifdef SYSV

#ifndef SYSVR4
#if defined(SCO) || defined(_SEQUENT_)
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef SCO
#define MAXPATHLEN 255
#endif
#else
#include <sys/bsdtypes.h>
#include <sys/limits.h>
#include <sys/sioctl.h>
#define MAXPATHLEN PATH_MAX
#define lstat stat
#endif
#endif

#if defined(IRIX5) || defined(SCO)
#include <sys/signal.h>
#endif

int gethostname();

#else				/* not SYSV */

#include <sys/types.h>		/* this is the key to the whole thing */

#endif				/* not SYSV */

#ifdef _IBMR2
#undef _BSD			/* union wait SUCKS! */
#include <sys/wait.h>
#define _BSD
#else
#ifdef HAS_WAIT
#include <wait.h>
#else
#include <sys/wait.h>
#endif
#endif

#if defined(IRIX5) && defined(_POSIX_SOURCE)
/* this stuff is in <sys/wait.h> but with _POSIX_SOURCE
 * it will not be seen -- this is correct but not nice
 * considering using "union wait" */
typedef union wait {
    int w_status;		/* used in syscall */
    struct {
#ifdef _MIPSEL
	unsigned int w_Termsig:7,	/* termination signal */
	    w_Coredump:1,	/* core dump indicator */
	    w_Retcode:8,	/* exit code if w_termsig==0 */
	    w_Filler:16;	/* upper bits filler */
#endif
#ifdef _MIPSEB
	unsigned int w_Filler:16,	/* upper bits filler */
	    w_Retcode:8,	/* exit code if w_termsig==0 */
	    w_Coredump:1,	/* core dump indicator */
	    w_Termsig:7;	/* termination signal */
#endif
    }
    w_T;
 /*
	 * Stopped process status.  Returned
	 * only for traced children unless requested
	 * with the WUNTRACED option bit.
	 */
    struct {
#ifdef _MIPSEL
	unsigned int w_Stopval:8,	/* == W_STOPPED if stopped */
	    w_Stopsig:8,	/* signal that stopped us */
	    w_Filler:16;	/* upper bits filler */
#endif
#ifdef _MIPSEB
	unsigned int w_Filler:16,	/* upper bits filler */
	    w_Stopsig:8,	/* signal that stopped us */
	    w_Stopval:8;	/* == W_STOPPED if stopped */
#endif
    }
    w_S;
}
wait_t;

#endif

#ifdef _CRAY
#define MAXPATHLEN PATH_MAX
#include <sys/machd.h>		/* HZ definition here */
#endif

#if defined(HAS_TIME) || defined(_CRAY) || defined(IRIX5) || defined(_SEQUENT_)
#include <time.h>
/* Sequent DYNIX/ptx gettimeofday() is in the X11 library.  Define timezone
   here so we don't have to load all of X11/Xos.h.
   billb@progress.com 4-Jan-93 */
#ifdef _SEQUENT_
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

#endif
#if (defined(_CRAY) && defined(__STDC__)) || defined(IRIX5)
#include <sys/time.h>
#endif
#else
#include <sys/time.h>
#endif

#if defined(IRIX5) || defined(SCO)
#include <sys/select.h>		/* needs timeval */
#endif

#ifdef SYSV
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#if !defined(SYSV) || defined(SYSVR4)
#include <sys/resource.h>
#endif

#if defined(RLIMIT_OFILE) && defined(RLIMIT_NOFILE)
#undef RLIMIT_NOFILE
#endif

#include <signal.h>
#ifdef USE_SIGSET
#undef signal
#define signal sigset
#endif

#if defined(SCO)
#include <sys/stream.h>
#include <sys/ptem.h>
#endif

#ifdef HAS_TERMIO
#define VDISABLEVAL -1
#define HAS_TIO 1
#include <sys/termio.h>
#else
#ifdef HAS_TERMIOS
#define VDISABLEVAL 0
#define HAS_TIO 1
#include <termios.h>
#ifdef __sgi
#ifndef VSWTCH
#define VSWTCH 7		/* VSWTCH not defined if _POSIX_SOURCE */
#endif
#ifndef CSWTCH
#ifndef CTRL
#define	CTRL(c)	((c)&037)
#endif
#define CSWTCH CTRL('z')	/* CSWTCH not defined if _POSIX_SOURCE */
#endif
#ifndef XTABS
#define XTABS 0014000		/* XTABS not defined if _POSIX_SOURCE */
#endif
#ifndef ONLCR
#define	ONLCR 0000004		/* ONLCR not defined if _POSIX_SOURCE */
#endif
#endif				/* __sgi */
#else
#include <sgtty.h>
#endif
#endif

#if defined(SYSV) && !defined(SYSVR4)
#define readlink(s,t,z)	(-1)
#undef TIOCGWINSZ
#endif

#include <sys/param.h>
#if defined(_CRAY) && defined(__STDC__)
#define OPEN_MAX 64
#define NOFILE OPEN_MAX
#endif

#if defined(SCO)
#define NOFILE _POSIX_OPEN_MAX
#endif

#if defined(SCO)
#include <utime.h>
#endif
#include <sys/times.h>

#ifdef HAS_DIRENT
#if defined(ardent)		/* ardent = Titan */
#include <sys/dirent.h>
#endif
#include <dirent.h>
#else
#include <sys/dir.h>
#if defined(sony) || defined(MACH)
#define dirent direct
#endif
#endif
#if defined(__NeXT__)
#define dirent direct		/* if you have absolutely no struct dirent */
#endif				/* anywhere in system headers, add your */
 /* system here and pray */

#ifdef __hp9000s800
#include <sys/bsdtty.h>
#endif

#if !defined(sun) && (!defined(SYSVR4) || defined(DGUX))
#if defined(_IBMR2) && defined(NOFLSH)
#undef NOFLSH
#endif
#ifndef ardent
#include <sys/ioctl.h>
#endif
#else
#include <sys/filio.h>		/* for FIONREAD */
#endif

#ifdef __STDC__
#define DCLPROTO(X) X
/* prototype template for static functions */
/* Note that the argument list "a" must have its own ()s around it */
#define SPROTO(f,a) static f a
#include <fcntl.h>
#include <sys/stat.h>
#ifndef NULL
#define NULL ((void *)0)
#endif
#else				/* K&R C */
#define DCLPROTO(X) ()
#define SPROTO(f,a) static f()
/* SGI cc digs "const" even when K&R */
#if !(defined(__sgi) && defined(__mips))
#define const
#endif
#include <sys/stat.h>
#ifndef NULL
#define NULL 0
#endif
#endif

#ifdef HAS_UTMPX
#include <utmpx.h>
#define utmp utmpx
#define ut_time ut_xtime
#undef UTMP_FILE
#define UTMP_FILE UTMPX_FILE
#undef WTMP_FILE
#define WTMP_FILE WTMPX_FILE
#else
#include <utmp.h>
#endif

#if !defined(UTMP_FILE) && defined(_PATH_UTMP)
#define UTMP_FILE _PATH_UTMP
#endif
#if !defined(WTMP_FILE) && defined(_PATH_WTMP)
#define WTMP_FILE _PATH_WTMP
#endif

#define DEFWORDCHARS "*?_-.[]~=/&;!#$%^(){}<>"
#define DEFTIMEFMT "%E real  %U user  %S system  %P %J"
#ifdef UTMP_HOST
#define DEFWATCHFMT "%n has %a %l from %m."
#else
#define DEFWATCHFMT "%n has %a %l."
#endif

#ifdef GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

#ifdef HAS_STRING
#define killpg(pgrp,sig) kill(-(pgrp),sig)
#endif

/* SYSV or POSIX compatible BSD */
/* ARGH, this is currently broken as the 'BSD' test had to be taken stripped
 * of an additional test BSD >= 199301 as Solaris cpp is broken ++jhi; */
#if (defined(SYSV) || defined(_POSIX_SOURCE) || defined(BSD) || defined(__convex__) || defined(_CRAY) || defined(__osf__) || defined(__linux__)) && !defined(NeXT)
#if !defined(__sgi) || !defined(_BSD_COMPAT)	/* IRIX */
#ifdef __hpux
#define GETPGRP()  getpgrp2(0)
#else
#define GETPGRP()  getpgrp()
#endif
#else
#define GETPGRP()  getpgrp(0)
#endif
#else
#define GETPGRP()  getpgrp(0)
#endif

#if defined(__sgi) && defined(__STDC__) && (defined(_BSD_COMPAT) || defined(_BSD_SIGNALS))
#ifdef _POSIX_SOURCE
#ifdef IRIX5
#define gettimeofday(tp, tzp) BSDgettimeofday(tp, tzp)
#else
#undef signal
#define signal(s, h) BSDsignal(s, (HANDTYPE (*)()) h)
#endif
#endif
#endif

#ifndef F_OK
#define F_OK 00
#define R_OK 04
#define W_OK 02
#define X_OK 01
#endif

/* sigvec sv_handler needs special casting */
#if defined(__convexc__)
#define SIGVEC_HANDTYPE _SigFunc_Ptr_t
#else
#if defined(IRIX5)
#define SIGVEC_HANDTYPE __sigret_t (*)()
#else
#if defined(ultrix)
/* Ultrix has void here but HANDTYPE is int? */
#define SIGVEC_HANDTYPE void (*)()
#else
#define SIGVEC_HANDTYPE HANDTYPE (*)()
#endif
#endif
#endif

#if 0
/* The quad_t should be used for BSD 4.4.  There was an ifdef BSD4_4
   here, but it broke FreeBSD compilation.  */
#define RLIM_TYPE quad_t
#else
#define RLIM_TYPE long
#endif

/* math.c */
typedef int LV;

#include "zle.h"

/* size of job list */

#define MAXJOB 80

/* memory allocation routines - changed with permalloc()/heapalloc() */

/* real things in mem.c */
extern vptr(*alloc) DCLPROTO((int));
extern vptr(*ncalloc) DCLPROTO((int));

/* character tokens */

#define ALPOP			((char) 0x81)
#define HISTSPACE		((char) 0x83)
#define Pound			((char) 0x84)
#define String			((char) 0x85)
#define Hat			((char) 0x86)
#define Star			((char) 0x87)
#define Inpar			((char) 0x88)
#define Outpar			((char) 0x89)
#define Qstring		        ((char) 0x8a)
#define Equals			((char) 0x8b)
#define Bar		      	((char) 0x8c)
#define Inbrace		        ((char) 0x8d)
#define Outbrace		((char) 0x8e)
#define Inbrack		        ((char) 0x8f)
#define Outbrack		((char) 0x90)
#define Tick			((char) 0x91)
#define Inang			((char) 0x92)
#define Outang			((char) 0x93)
#define Quest			((char) 0x94)
#define Tilde			((char) 0x95)
#define Qtick			((char) 0x96)
#define Comma			((char) 0x97)
#define Snull			((char) 0x98)
#define Dnull			((char) 0x99)
#define Bnull			((char) 0x9a)
#define Nularg			((char) 0x9b)

#define INULL(x)		(((x) & 0xf8) == 0x98)

/* Character tokens are sometimes casted to (unsigned char)'s. Unfortunately,
   SVR4's deceiving compiler botches non-terminal, same size, signed to
   unsigned promotions; i.e. (int) (unsigned char) ((char) -1) evaluates to
   -1, 	not 255 as it should!
   We circumvent the troubles of such shameful delinquency by casting to a
   larger unsigned type then back down to unsigned char.  		--
   Marc Boucher <marc@cam.org>
   Dec Alpha OSF compilers have the same property
   <daniel@ug.eds.com> 25-Jan-1993 */

#if (defined(SYSVR4) || defined (__osf__)) && !defined(__GNUC__)
#define STOUC(X)	((unsigned char)(unsigned short)(X))
#else
#define STOUC(X)	((unsigned char)(X))
#endif

/*
 * The source was full of implicit casts between signed and unsigned
 * character pointers.  To get a clean compile, I've made these casts
 * explicit, but the potential for error is still there.  If your machine
 * needs special treatment, just define your own macros here.
 * 	--jim <jmattson@ucsd.edu>
 */

#define STOUCP(X)	((unsigned char *)(X))
#define UTOSCP(X)	((char *)(X))

/* chars that need to be quoted if meant literally */

#define SPECCHARS "#$^*()$=|{}[]`<>?~;&!\n\t \\\'\""

/* ALPOP in the form of a string */

#define ALPOPS " \201"
#define HISTMARK "\201"

#define SEPER 1
#define NEWLIN 2
#define LEXERR 3
#define SEMI 4
#define DSEMI 5
#define AMPER 6
#define INPAR 7
#define INBRACE 8
#define OUTPAR 9
#define DBAR 10
#define DAMPER 11
#define BANG 12
#define OUTBRACE 13
#define OUTANG 14
#define OUTANGBANG 15
#define DOUTANG 16
#define DOUTANGBANG 17
#define INANG 18
#define DINANG 19
#define DINANGDASH 20
#define INANGAMP 21
#define OUTANGAMP 22
#define OUTANGAMPBANG 23
#define DOUTANGAMP 24
#define DOUTANGAMPBANG 25
#define TRINANG 26
#define BAR 27
#define BARAMP 28
#define DINBRACK 29
#define DOUTBRACK 30
#define STRING 31
#define ENVSTRING 32
#define ENVARRAY 33
#define ENDINPUT 34
#define INOUTPAR 35
#define DO 36
#define DONE 37
#define ESAC 38
#define THEN 39
#define ELIF 40
#define ELSE 41
#define FI 42
#define FOR 43
#define CASE 44
#define IF 45
#define WHILE 46
#define FUNC 47
#define REPEAT 48
#define TIME 49
#define UNTIL 50
#define EXEC 51
#define COMMAND 52
#define SELECT 53
#define COPROC 54
#define NOGLOB 55
#define DASH 56
#define NOCORRECT 57
#define FOREACH 58
#define ZEND 59
#define DINPAR 60
#define DOUTPAR 61

#define WRITE 0
#define WRITENOW 1
#define APP 2
#define APPNOW 3
#define MERGEOUT 4
#define MERGEOUTNOW 5
#define ERRAPP 6
#define ERRAPPNOW 7
#define READ 8
#define HEREDOC 9
#define HEREDOCDASH 10
#define HERESTR 11
#define MERGE 12
#define CLOSE 13
#define INPIPE 14
#define OUTPIPE 15
#define NONE 16

#ifdef GLOBALS
int redirtab[TRINANG - OUTANG + 1] =
{
    WRITE,
    WRITENOW,
    APP,
    APPNOW,
    READ,
    HEREDOC,
    HEREDOCDASH,
    MERGE,
    MERGEOUT,
    MERGEOUTNOW,
    ERRAPP,
    ERRAPPNOW,
    HERESTR,
};

#else
extern int redirtab[TRINANG - OUTANG + 1];

#endif

#ifdef GLOBALS
char nulstring[] =
{Nularg, '\0'};
int nulstrlen = sizeof(nulstring) - 1;

#else
extern char nulstring[];
extern int nulstrlen;

#endif

#define IS_READFD(X) ((X)>=READ && (X)<=MERGE)
#define IS_REDIROP(X) ((X)>=OUTANG && (X)<=TRINANG)
#define IS_ERROR_REDIR(X) ((X)>=MERGEOUT && (X)<=ERRAPPNOW)
#define UN_ERROR_REDIR(X) ((X)-MERGEOUT+WRITE)

#define FD_WORD   -1
#define FD_COPROC -2
#define FD_CLOSE  -3

extern char **environ;

/* linked list abstract data type */

struct lknode {
    struct lknode *next, *last;
    vptr dat;
};
struct lklist {
    struct lknode *first, *last;
};

typedef struct hashtab *Hashtab;
typedef struct hashnode *Hashnode;
typedef struct schedcmd *Schedcmd;
typedef struct alias *Alias;
typedef struct process *Process;
typedef struct job *Job;
typedef struct value *Value;
typedef struct arrind *Arrind;
typedef struct varasg *Varasg;
typedef struct param *Param;
typedef struct cmdnam *Cmdnam;
typedef struct cond *Cond;
typedef struct cmd *Cmd;
typedef struct pline *Pline;
typedef struct sublist *Sublist;
typedef struct list *List;
typedef struct lklist *Lklist;
typedef struct lknode *Lknode;
typedef struct comp *Comp;
typedef struct redir *Redir;
typedef struct complist *Complist;
typedef struct heap *Heap;
typedef struct heappos *Heappos;
typedef struct histent *Histent;
typedef struct compctlp *Compctlp;
typedef struct compctl *Compctl;
typedef struct compcond *Compcond;
typedef void (*FFunc) DCLPROTO((vptr));
typedef vptr(*VFunc) DCLPROTO((vptr));
typedef void (*HFunc) DCLPROTO((char *, char *));

#define addnode(X,Y) insnode(X,(X)->last,Y)
#define full(X) ((X)->first != NULL)
#define empty(X) ((X)->first == NULL)
#define firstnode(X) ((X)->first)
#define getaddrdata(X) (&((X)->dat))
#define getdata(X) ((X)->dat)
#define setdata(X,Y) ((X)->dat = (Y))
#define lastnode(X) ((X)->last)
#define nextnode(X) ((X)->next)
#define prevnode(X) ((X)->last)
#define peekfirst(X) ((X)->first->dat)
#define pushnode(X,Y) insnode(X,(Lknode) X,Y)
#define incnode(X) (X = nextnode(X))
#define gethistent(X) (histentarr+((X)%histentct))

/* node structure for syntax trees */

/* struct list, struct sublist, struct pline, etc.  all fit the form
	of this structure and are used interchangably.
	The ptrs may hold integers or pointers, depending on the type of
	the node.
*/

struct node {
    int type;			/* node type */
};

#define N_LIST 0
#define N_SUBLIST 1
#define N_PLINE 2
#define N_CMD 3
#define N_REDIR 4
#define N_COND 5
#define N_FOR 6
#define N_CASE 7
#define N_IF 8
#define N_WHILE 9
#define N_VARASG 10
#define N_COUNT 11

/* values for types[4] */

#define NT_EMPTY 0
#define NT_NODE  1
#define NT_STR   2
#define NT_LIST  4
#define NT_ARR   8

#define NT_TYPE(T) ((T) & 0xff)
#define NT_N(T, N) (((T) >> (8 + (N) * 4)) & 0xf)
#define NT_SET(T0, N, T1, T2, T3, T4) \
    ((T0) | ((N) << 24) | \
     ((T1) << 8) | ((T2) << 12) | ((T3) << 16) | ((T4) << 20))
#define NT_NUM(T) (((T) >> 24) & 7)
#define NT_HEAP   (1 << 30)

/* tree element for lists */

struct list {
    int ntype;
    int type;
    Sublist left;
    List right;
};

#define SYNC 0			/* ; */
#define ASYNC 1			/* & */
#define TIMED 2

/* tree element for sublists */

struct sublist {
    int ntype;
    int type;
    int flags;			/* see PFLAGs below */
    Pline left;
    Sublist right;
};

#define ORNEXT 10		/* || */
#define ANDNEXT 11		/* && */

#define PFLAG_NOT 1		/* ! ... */
#define PFLAG_COPROC 32		/* coproc ... */

/* tree element for pipes */

struct pline {
    int ntype;
    int type;
    Cmd left;
    Pline right;
};

#define END	0		/* pnode *right is null */
#define PIPE	1		/* pnode *right is the rest of the pipeline */

/* tree element for commands */

struct cmd {
    int ntype;
    int type;
    int flags;			/* see CFLAGs below */
    int lineno;			/* lineno of script for command */
    Lklist args;		/* command & argmument List (char *'s) */
    union {
	List list;		/* for SUBSH/CURSH/SHFUNC */
	struct forcmd *forcmd;
	struct casecmd *casecmd;
	struct ifcmd *ifcmd;
	struct whilecmd *whilecmd;
	Sublist pline;
	Cond cond;
	vptr generic;
    } u;
    Lklist redir;		/* i/o redirections (struct redir *'s) */
    Lklist vars;		/* param assignments (struct varasg *'s) */
};

#define SIMPLE 0
#define SUBSH 1
#define CURSH 2
#define ZCTIME 3
#define FUNCDEF 4
#define CFOR 5
#define CWHILE 6
#define CREPEAT 7
#define CIF 8
#define CCASE 9
#define CSELECT 10
#define COND 11

#define CFLAG_EXEC 1		/* exec ... */
#define CFLAG_COMMAND 2		/* command ... */
#define CFLAG_NOGLOB 4		/* noglob ... */
#define CFLAG_DASH 8		/* - ... */

/* tree element for redirection lists */

struct redir {
    int ntype;
    int type, fd1, fd2;
    char *name;
};

/* tree element for conditionals */

struct cond {
    int ntype;
    int type;			/* can be cond_type, or a single letter (-a, -b, ...) */
    vptr left, right;
};

#define COND_NOT 0
#define COND_AND 1
#define COND_OR 2
#define COND_STREQ 3
#define COND_STRNEQ 4
#define COND_STRLT 5
#define COND_STRGTR 6
#define COND_NT 7
#define COND_OT 8
#define COND_EF 9
#define COND_EQ 10
#define COND_NE 11
#define COND_LT 12
#define COND_GT 13
#define COND_LE 14
#define COND_GE 15

struct forcmd {			/* for/select */
/* Cmd->args contains list of words to loop thru */
    int ntype;
    int inflag;			/* if there is an in ... clause */
    char *name;			/* parameter to assign values to */
    List list;			/* list to look through for each name */
};
struct casecmd {
/* Cmd->args contains word to test */
    int ntype;
    char **pats;
    List *lists;		/* list to execute */
};

/*

	a command like "if foo then bar elif baz then fubar else fooble"
	generates a tree like:

	struct ifcmd a = { next =  &b,  ifl = "foo", thenl = "bar" }
	struct ifcmd b = { next =  &c,  ifl = "baz", thenl = "fubar" }
	struct ifcmd c = { next = NULL, ifl = NULL, thenl = "fooble" }

*/

struct ifcmd {
    int ntype;
    List *ifls;
    List *thenls;
};

struct whilecmd {
    int ntype;
    int cond;			/* 0 for while, 1 for until */
    List cont;			/* condition */
    List loop;			/* list to execute until condition met */
};

/* structure used for multiple i/o redirection */
/* one for each fd open */

struct multio {
    int ct;			/* # of redirections on this fd */
    int rflag;			/* 0 if open for reading, 1 if open for writing */
    int pipe;			/* fd of pipe if ct > 1 */
    int fds[NOFILE];		/* list of src/dests redirected to/from this fd */
};

/* node used in command path hash table (cmdnamtab) */

struct cmdnam {
    struct hashnode *next;
    char *nam;			/* hash data */
    int flags;
    union {
	char **name;		/* full pathname if !(flags & BUILTIN) */
	char *cmd;		/* file name for hashed commands */
	int binnum;		/* func to exec if type & BUILTIN */
	List list;		/* list to exec if type & SHFUNC */
    }
    u;
};

#define EXCMD        0x10000
#define BUILTIN      0x20000
#define SHFUNC       0x40000
#define DISABLED     0x80000
#define HASHCMD      (EXCMD | BUILTIN)

#define ISEXCMD(X) ((X) & EXCMD)

/* node used in parameter hash table (paramtab) */

struct param {
    struct hashnode *next;
    char *nam;			/* hash data */
    union {
	char **arr;		/* value if declared array */
	char *str;		/* value if declared string (scalar) */
	long val;		/* value if declared integer */
    }
    u;
    union {			/* functions to call to set value */
	void (*cfn) DCLPROTO((Param, char *));
	void (*ifn) DCLPROTO((Param, long));
	void (*afn) DCLPROTO((Param, char **));
    }
    sets;
    union {			/* functions to call to get value */
	char *(*cfn) DCLPROTO((Param));
	long (*ifn) DCLPROTO((Param));
	char **(*afn) DCLPROTO((Param));
    }
    gets;
    int ct;			/* output base or field width */
    int flags;
    vptr data;			/* used by getfns */
    char *env;			/* location in environment, if exported */
    char *ename;		/* name of corresponding environment var */
    Param old;			/* old struct for use with local */
    int level;			/* if (old != NULL), level of localness */
};

#define PMFLAG_s 0		/* scalar */
#define PMFLAG_L 1		/* left justify and remove leading blanks */
#define PMFLAG_R 2		/* right justify and fill with leading blanks */
#define PMFLAG_Z 4		/* right justify and fill with leading zeros */
#define PMFLAG_i 8		/* integer */
#define PMFLAG_l 16		/* all lower case */
#define PMFLAG_u 32		/* all upper case */
#define PMFLAG_r 64		/* readonly */
#define PMFLAG_t 128		/* tagged */
#define PMFLAG_x 256		/* exported */
#define PMFLAG_A 512		/* array */
#define PMFLAG_SPECIAL	1024
#define PMFLAG_UNSET	2048
#define PMTYPE (PMFLAG_i|PMFLAG_A)
#define pmtype(X) ((X)->flags & PMTYPE)

/* variable assignment tree element */

struct varasg {
    int ntype;
    int type;			/* nonzero means array */
    char *name;
    char *str;			/* should've been a union here.  oh well */
    Lklist arr;
};

/* lvalue for variable assignment/expansion */

struct value {
    int isarr;
    struct param *pm;		/* parameter node */
    int inv;			/* should we return the index ? */
    int a;			/* first element of array slice, or -1 */
    int b;			/* last element of array slice, or -1 */
};

struct fdpair {
    int fd1, fd2;
};

/* tty state structure */

struct ttyinfo {
#ifdef HAS_TERMIOS
    struct termios tio;
#else
#ifdef HAS_TERMIO
    struct termio tio;
#else
    struct sgttyb sgttyb;
    int lmodes;
    struct tchars tchars;
    struct ltchars ltchars;
#endif
#endif
#ifdef TIOCGWINSZ
    struct winsize winsize;
#endif
};

/* entry in job table */

struct job {
    long gleader;		/* process group leader of this job */
    int stat;
    char *pwd;			/* current working dir of shell when
				   this job was spawned */
    struct process *procs;	/* list of processes */
    Lklist filelist;		/* list of files to delete when done */
    int stty_in_env;		/* if STTY=... is present */
    struct ttyinfo *ty;		/* the modes specified by STTY */
};

#define STAT_CHANGED 1		/* status changed and not reported */
#define STAT_STOPPED 2		/* all procs stopped or exited */
#define STAT_TIMED 4		/* job is being timed */
#define STAT_DONE 8
#define STAT_LOCKED 16		/* shell is finished creating this job,	may be
				   deleted from job table */
#define STAT_INUSE 64		/* this job entry is in use */

#define SP_RUNNING -1		/* fake statusp for running jobs */

#ifndef RUSAGE_CHILDREN
#undef HAS_RUSAGE
#endif

struct timeinfo {
#ifdef HAS_RUSAGE
    struct rusage ru;
#else
    long ut, st;
#endif
};

/* node in job process lists */

#define JOBTEXTSIZE 80

struct process {
    struct process *next;
    long pid;
    char text[JOBTEXTSIZE];	/* text to print when 'jobs' is run */
    int statusp;		/* return code from wait3() */
    struct timeinfo ti;
    struct timeval bgtime;	/* time job was spawned */
    struct timeval endtime;	/* time job exited */
};

/* node in alias hash table */

struct alias {
    struct hashnode *next;
    char *nam;			/* hash data */
    char *text;			/* expansion of alias */
    int cmd;			/* one for regular aliases, zero for global
				   aliases, negative for reserved words */
    int inuse;			/* alias is being expanded */
};

/* node in sched list */

struct schedcmd {
    struct schedcmd *next;
    char *cmd;			/* command to run */
    time_t time;		/* when to run it */
};

#define MAXAL 20		/* maximum number of aliases expanded at once */

/* hash table node */

struct hashnode {
    struct hashnode *next;
    char *nam;
};

/* hash table */

struct hashtab {
    int hsize;			/* size of nodes[] */
    int ct;			/* # of elements */
    struct hashnode **nodes;	/* array of size hsize */
};

/* history entry */

struct histent {
    char *lex;			/* lexical history line */
    char *lit;			/* literal history line */
    time_t stim;		/* command started time (datestamp) */
    time_t ftim;		/* command finished time */
    int flags;			/* Misc flags */
};

#define HIST_OLD	0x00000001	/* Command is already written to disk*/

/* completion control */

struct compcond {
    struct compcond *and, *or;	/* the next or'ed/and'ed conditions */
    int type, n;		/* the type (CCT_*) and the array length */
    union {			/* these structs hold the data used to */
	struct {		/* test this condition */
	    int *a, *b;		/* CCT_POS, CCT_NUMWORDS */
	}
	r;
	struct {		/* CCT_CURSTR, CCT_CURPAT,... */
	    int *p;
	    char **s;
	}
	s;
	struct {		/* CCT_RANGESTR,... */
	    char **a, **b;
	}
	l;
    }
    u;
};

#define CCT_UNUSED     0
#define CCT_POS        1
#define CCT_CURSTR     2
#define CCT_CURPAT     3
#define CCT_WORDSTR    4
#define CCT_WORDPAT    5
#define CCT_CURSUF     6
#define CCT_CURPRE     7
#define CCT_CURSUB     8
#define CCT_CURSUBC    9
#define CCT_NUMWORDS  10
#define CCT_RANGESTR  11
#define CCT_RANGEPAT  12

struct compctlp {		/* the hash table node for compctls */
    struct hashnode *next;
    char *nam;			/* command name */
    Compctl cc;			/* pointer to the compctl desc. */
};

struct compctl {		/* the real desc. for compctls */
    int refc;			/* reference count */
    struct compctl *next;	/* next compctl for -x */
    unsigned long mask;		/* mask of things to complete (CC_*) */
    char *keyvar;		/* for -k (variable) */
    char *glob;			/* for -g (globbing) */
    char *str;			/* for -s (expansion) */
    char *func;			/* for -f (function) */
    char *explain;		/* for -X (explanation) */
    char *prefix, *suffix;	/* for -P and -S (prefix, suffix) */
    char *subcmd;		/* for -l (command name to use) */
    char *hpat;			/* for -H (history pattern) */
    int hnum;			/* for -H (number of events to search) */
    struct compctl *ext;	/* for -x (first of the compctls after -x) */
    struct compcond *cond;	/* for -x (condition for this compctl) */
    struct compctl *xor;	/* for + (next of the xor'ed compctls) */
};

#define CC_FILES	(1<<0)
#define CC_COMMPATH	(1<<1)
#define CC_REMOVE       (1<<2)
#define CC_OPTIONS	(1<<3)
#define CC_VARS		(1<<4)
#define CC_BINDINGS	(1<<5)
#define CC_ARRAYS	(1<<6)
#define CC_INTVARS	(1<<7)
#define CC_FUNCS	(1<<8)
#define CC_PARAMS	(1<<9)
#define CC_ENVVARS	(1<<10)
#define CC_JOBS		(1<<11)
#define CC_RUNNING	(1<<12)
#define CC_STOPPED	(1<<13)
#define CC_BUILTINS	(1<<14)
#define CC_ALREG	(1<<15)
#define CC_ALGLOB	(1<<16)
#define CC_USERS	(1<<17)
#define CC_DISCMDS	(1<<18)
#define CC_EXCMDS	(1<<19)
#define CC_SCALARS	(1<<20)
#define CC_READONLYS    (1<<21)
#define CC_SPECIALS	(1<<22)
#define CC_DELETE       (1<<23)
#define CC_NAMED        (1<<24)

#define CC_RESERVED     (1<<31)

#include <errno.h>
#ifdef __NetBSD__
extern const char *const sys_errlist[];

#else
extern char *sys_errlist[];

#endif
extern int sys_nerr;
extern int errno;

/* values in opts[] array */

#define OPT_INVALID 1		/* opt is invalid, like -$ */
#define OPT_UNSET 0
#define OPT_SET 2

/* the options */

struct option {
    char *name;
    char id;			/* corresponding letter */
};

#define CORRECT '0'
#define NOCLOBBER '1'
#define NOBADPATTERN '2'
#define NONOMATCH '3'
#define GLOBDOTS '4'
#define NOTIFY '5'
#define BGNICE '6'
#define IGNOREEOF '7'
#define MARKDIRS '8'
#define AUTOLIST '9'
#define NOBEEP 'B'
#define PRINTEXITVALUE 'C'
#define PUSHDTOHOME 'D'
#define PUSHDSILENT 'E'
#define NOGLOBOPT 'F'
#define NULLGLOB 'G'
#define RMSTARSILENT 'H'
#define IGNOREBRACES 'I'
#define AUTOCD 'J'
#define NOBANGHIST 'K'
#define SUNKEYBOARDHACK 'L'
#define SINGLELINEZLE 'M'
#define AUTOPUSHD 'N'
#define CORRECTALL 'O'
#define RCEXPANDPARAM 'P'
#define PATHDIRS 'Q'
#define LONGLISTJOBS 'R'
#define RECEXACT 'S'
#define CDABLEVARS 'T'
#define MAILWARNING 'U'
#define NOPROMPTCR 'V'
#define AUTORESUME 'W'
#define LISTTYPES 'X'
#define MENUCOMPLETE 'Y'
#define USEZLE 'Z'
#define ALLEXPORT 'a'
#define ERREXIT 'e'
#define NORCS 'f'
#define HISTIGNORESPACE 'g'
#define HISTIGNOREDUPS 'h'
#define INTERACTIVE 'i'
#define HISTLIT 'j'
#define INTERACTIVECOMMENTS 'k'
#define LOGINSHELL 'l'
#define MONITOR 'm'
#define NOEXEC 'n'
#define KSHPRIV 'p'
#define SHINSTDIN 's'
#define NOUNSET 'u'
#define VERBOSE 'v'
#define CHASELINKS 'w'
#define XTRACE 'x'
#define SHWORDSPLIT 'y'
#define HISTNOSTORE '\3'
#define EXTENDEDGLOB '\5'
#define GLOBCOMPLETE '\6'
#define CSHJUNKIEQUOTES '\7'
#define PUSHDMINUS '\10'
#define CSHJUNKIELOOPS '\11'
#define RCQUOTES '\12'
#define KSHOPTIONPRINT '\13'
#define NOSHORTLOOPS '\14'
#define COMPLETEINWORD '\15'
#define AUTOMENU '\16'
#define HISTVERIFY '\17'
#define NOLISTBEEP '\20'
#define NOHUP '\21'
#define NOEQUALS '\22'
#define CSHNULLGLOB '\23'
#define HASHCMDS '\24'
#define HASHDIRS '\25'
#define NUMERICGLOBSORT '\26'
#define BRACECCL '\27'
#define HASHLISTALL '\30'
#define OVERSTRIKE '\31'
#define NOHISTBEEP '\32'
#define PUSHDIGNOREDUPS '\33'
#define AUTOREMOVESLASH '\34'
#define EXTENDEDHISTORY '\35'
#define APPENDHISTORY '\36'
#define CSHJUNKIEHISTORY '\037'
#define MAGICEQUALSUBST '\040'
#define GLOBSUBST '\041'
#define PROMPTSUBST '\043'
#define ALWAYSLASTPROMPT '\044'
#define COMPLETEALIASES '\045'
#define AUTOPARAMKEYS '\046'
#define ALWAYSTOEND '\047'
#define NOFLOWCONTROL '\050'
#define LISTAMBIGUOUS '\051'
#define AUTONAMEDIRS '\052'
#define CSHJUNKIEPAREN '\053'

#ifndef GLOBALS
extern struct option optns[];

#else
struct option optns[] =
{
    {"correct", CORRECT},
    {"noclobber", NOCLOBBER},
    {"nobadpattern", NOBADPATTERN},
    {"nonomatch", NONOMATCH},
    {"globdots", GLOBDOTS},
    {"notify", NOTIFY},
    {"bgnice", BGNICE},
    {"ignoreeof", IGNOREEOF},
    {"markdirs", MARKDIRS},
    {"autolist", AUTOLIST},
    {"nobeep", NOBEEP},
    {"printexitvalue", PRINTEXITVALUE},
    {"pushdtohome", PUSHDTOHOME},
    {"pushdsilent", PUSHDSILENT},
    {"noglob", NOGLOBOPT},
    {"nullglob", NULLGLOB},
    {"rmstarsilent", RMSTARSILENT},
    {"ignorebraces", IGNOREBRACES},
    {"braceccl", BRACECCL},
    {"autocd", AUTOCD},
    {"nobanghist", NOBANGHIST},
    {"sunkeyboardhack", SUNKEYBOARDHACK},
    {"singlelinezle", SINGLELINEZLE},
    {"autopushd", AUTOPUSHD},
    {"correctall", CORRECTALL},
    {"rcexpandparam", RCEXPANDPARAM},
    {"pathdirs", PATHDIRS},
    {"longlistjobs", LONGLISTJOBS},
    {"recexact", RECEXACT},
    {"cdablevars", CDABLEVARS},
    {"mailwarning", MAILWARNING},
    {"nopromptcr", NOPROMPTCR},
    {"autoresume", AUTORESUME},
    {"listtypes", LISTTYPES},
    {"menucomplete", MENUCOMPLETE},
    {"zle", USEZLE},
    {"allexport", ALLEXPORT},
    {"errexit", ERREXIT},
    {"norcs", NORCS},
    {"histignorespace", HISTIGNORESPACE},
    {"histignoredups", HISTIGNOREDUPS},
    {"interactive", INTERACTIVE},
    {"histlit", HISTLIT},
    {"interactivecomments", INTERACTIVECOMMENTS},
    {"login", LOGINSHELL},
    {"monitor", MONITOR},
    {"noexec", NOEXEC},
    {"shinstdin", SHINSTDIN},
    {"nounset", NOUNSET},
    {"verbose", VERBOSE},
    {"chaselinks", CHASELINKS},
    {"xtrace", XTRACE},
    {"shwordsplit", SHWORDSPLIT},
    {"histnostore", HISTNOSTORE},
    {"extendedglob", EXTENDEDGLOB},
    {"globcomplete", GLOBCOMPLETE},
    {"cshjunkiequotes", CSHJUNKIEQUOTES},
    {"pushdminus", PUSHDMINUS},
    {"cshjunkieloops", CSHJUNKIELOOPS},
    {"rcquotes", RCQUOTES},
    {"noshortloops", NOSHORTLOOPS},
    {"completeinword", COMPLETEINWORD},
    {"automenu", AUTOMENU},
    {"histverify", HISTVERIFY},
    {"nolistbeep", NOLISTBEEP},
    {"nohup", NOHUP},
    {"noequals", NOEQUALS},
    {"kshoptionprint", KSHOPTIONPRINT},
    {"cshnullglob", CSHNULLGLOB},
    {"hashcmds", HASHCMDS},
    {"hashdirs", HASHDIRS},
    {"numericglobsort", NUMERICGLOBSORT},
    {"hashlistall", HASHLISTALL},
    {"overstrike", OVERSTRIKE},
    {"nohistbeep", NOHISTBEEP},
    {"pushdignoredups", PUSHDIGNOREDUPS},
    {"autoremoveslash", AUTOREMOVESLASH},
    {"extendedhistory", EXTENDEDHISTORY},
    {"appendhistory", APPENDHISTORY},
    {"cshjunkiehistory", CSHJUNKIEHISTORY},
    {"magicequalsubst", MAGICEQUALSUBST},
    {"globsubst", GLOBSUBST},
    {"promptsubst", PROMPTSUBST},
    {"alwayslastprompt", ALWAYSLASTPROMPT},
    {"completealiases", COMPLETEALIASES},
    {"autoparamkeys", AUTOPARAMKEYS},
    {"alwaystoend", ALWAYSTOEND},
    {"noflowcontrol", NOFLOWCONTROL},
    {"listambiguous", LISTAMBIGUOUS},
    {"autonamedirs", AUTONAMEDIRS},
    {"cshjunkieparen", CSHJUNKIEPAREN},
    {NULL, 0}
};

#endif

#define ALSTAT_MORE 1		/* last alias ended with ' ' */
#define ALSTAT_JUNK 2		/* don't put word in history List */

#undef isset
#define isset(X) (opts[(int)X] == OPT_SET)
#define unset(X) (opts[(int)X] == OPT_UNSET)
#define interact (isset(INTERACTIVE))
#define jobbing (isset(MONITOR))
#define jobbingv opts[MONITOR]
#define islogin (isset(LOGINSHELL))

#ifndef SYSVR4
#ifndef _IBMR2
#undef WIFSTOPPED
#undef WIFSIGNALED
#undef WIFEXITED
#undef WEXITSTATUS
#undef WTERMSIG
#undef WSTOPSIG
#undef WCOREDUMP

#define WIFSTOPPED(X) (((X)&0377)==0177)
#define WIFSIGNALED(X) (((X)&0377)!=0&&((X)&0377)!=0177)
#define WIFEXITED(X) (((X)&0377)==0)
#define WEXITSTATUS(X) (((X)>>8)&0377)
#define WTERMSIG(X) ((X)&0177)
#define WSTOPSIG(X) (((X)>>8)&0377)
#endif
#if !defined(IRIX5) || !defined(_POSIX_SOURCE)
#define WCOREDUMP(X) ((X)&0200)
#endif
#endif

#if defined(IRIX5) && defined(_POSIX_SOURCE)
#define WCOREDUMP(X) ((*(int *)&(X)) & 0200)
#endif

#ifndef S_ISBLK
#define	_IFMT		0170000
#define	_IFDIR	0040000
#define	_IFCHR	0020000
#define	_IFBLK	0060000
#define	_IFREG	0100000
#define	_IFIFO	0010000
#define	S_ISBLK(m)	(((m)&_IFMT) == _IFBLK)
#define	S_ISCHR(m)	(((m)&_IFMT) == _IFCHR)
#define	S_ISDIR(m)	(((m)&_IFMT) == _IFDIR)
#define	S_ISFIFO(m)	(((m)&_IFMT) == _IFIFO)
#define	S_ISREG(m)	(((m)&_IFMT) == _IFREG)
#endif

#ifndef _IFMT
#define _IFMT 0170000
#endif

#ifndef S_ISSOCK
#define	_IFSOCK	0140000
#define	S_ISSOCK(m)	(((m)&_IFMT) == _IFSOCK)
#endif

#ifndef S_ISLNK
#define	_IFLNK	0120000
#define	S_ISLNK(m)	(((m)&_IFMT) == _IFLNK)
#endif

#if S_IFIFO == S_IFSOCK
#undef S_IFIFO
#endif

#ifndef S_IFIFO
#undef HAS_FIFOS
#endif

#if !defined(S_ISFIFO) && !defined(HAS_FIFOS)
#define S_ISFIFO(m) 0
#endif

/* buffered shell input for non-interactive shells */

EXTERN FILE *bshin;

/* NULL-terminated arrays containing path, cdpath, etc. */

EXTERN char **path, **cdpath, **fpath, **watch, **mailpath;
EXTERN char **manpath, **tildedirs, **fignore;
EXTERN char **psvar;

/* named directories */

typedef struct nameddirs *Nameddirs;

struct nameddirs {
    int len;			/* length of path namdirs[t0].dir */
    int namelen;		/* length of name */
    char *name;
    char *dir;
    int homedir;		/* is this name a home directory? */
};

EXTERN struct nameddirs *namdirs;

/* size of userdirs[], # of userdirs */

EXTERN int userdirsz, userdirct;

EXTERN char *mailfile;

EXTERN char *yytext;

/* error/break flag */

EXTERN int errflag;

/* Status of return from a trap */

EXTERN int trapreturn;

EXTERN char *tokstr;
EXTERN int tok, tokfd;

/* lexical analyzer error flag */

EXTERN int lexstop;

/* suppress error messages */

EXTERN int noerrs;

/* nonzero means we are not evaluating, just parsing (in math.c) */

EXTERN int noeval;

/* current history event number */

EXTERN int curhist;

/* if != 0, this is the first line of the command */

EXTERN int isfirstln;

/* if != 0, this is the first char of the command (not including
	white space) */

EXTERN int isfirstch;

/* number of history entries */

EXTERN int histentct;

/* array of history entries */

EXTERN Histent histentarr;

/* capacity of history lists */

EXTERN int histsiz, lithistsiz;

/* if = 1, we have performed history substitution on the current line
 	if = 2, we have used the 'p' modifier */

EXTERN int histdone;

/* default event (usually curhist-1, that is, "!!") */

EXTERN int defev;

/* != 0 if we are about to read a command word */

EXTERN int incmdpos;

/* != 0 if we are in the middle of a [[ ... ]] */

EXTERN int incond;

/* != 0 if we are after a redirection (for ctxtlex only) */

EXTERN int inredir;

/* != 0 if we are about to read a case pattern */

EXTERN int incasepat;

/* != 0 if we just read FUNCTION */

EXTERN int infunc;

/* != 0 if we just read a newline */

EXTERN int isnewlin;

/* the lists of history events */

EXTERN Lklist histlist, lithistlist;

/* the directory stack */

EXTERN Lklist dirstack;

/* the zle buffer stack */

EXTERN Lklist bufstack;

/* the input queue (stack?)

	inbuf    = start of buffer
	inbufptr = location in buffer	(= inbuf for a FULL buffer)
					(= inbuf+inbufsz for an EMPTY buffer)
	inbufct  = # of chars in buffer (inbufptr+inbufct == inbuf+inbufsz)
	inbufsz  = max size of buffer
*/

EXTERN char *inbuf, *inbufptr;
EXTERN int inbufct, inbufsz;

EXTERN char *ifs;		/* $IFS */

EXTERN char *oldpwd;		/* $OLDPWD */

EXTERN char *underscore;	/* $_ */

/* != 0 if this is a subshell */

EXTERN int subsh;

/* # of break levels */

EXTERN int breaks;

/* != 0 if we have a return pending */

EXTERN int retflag;

/* how far we've hashed the PATH so far */

EXTERN char **pathchecked;

/* # of nested loops we are in */

EXTERN int loops;

/* # of continue levels */

EXTERN int contflag;

/* the job we are working on */

EXTERN int thisjob;

/* the current job (+) */

EXTERN int curjob;

/* the previous job (-) */

EXTERN int prevjob;

/* hash table containing the aliases and reserved words */

EXTERN Hashtab aliastab;

/* hash table containing the parameters */

EXTERN Hashtab paramtab;

/* hash table containing the builtins/shfuncs/hashed commands */

EXTERN Hashtab cmdnamtab;

/* hash table for completion info for commands */

EXTERN Hashtab compctltab;

/* default completion infos */

EXTERN struct compctl cc_compos, cc_default, cc_dummy;

/* the job table */

EXTERN struct job jobtab[MAXJOB];

/* shell timings */

#ifndef HAS_RUSAGE
EXTERN struct tms shtms;

#endif

/* the list of sched jobs pending */

EXTERN struct schedcmd *schedcmds;

/* the last l for s/l/r/ history substitution */

EXTERN char *hsubl;

/* the last r for s/l/r/ history substitution */

EXTERN char *hsubr;

EXTERN char *username;		/* $USERNAME */
EXTERN char *zlogname;		/* $LOGNAME */
EXTERN long lastval;		/* $? */
EXTERN long baud;		/* $BAUD */
EXTERN long columns;		/* $COLUMNS */
EXTERN long lines;		/* $LINES */
EXTERN long reporttime;		/* $REPORTTIME */
EXTERN long lastval2;

/* input fd from the coprocess */

EXTERN int coprocin;

/* output fd from the coprocess */

EXTERN int coprocout;

EXTERN long mailcheck;		/* $MAILCHECK */
EXTERN long logcheck;		/* $LOGCHECK */

/* the last time we checked mail */

EXTERN time_t lastmailcheck;

/* the last time we checked the people in the WATCH variable */

EXTERN time_t lastwatch;

/* the last time we did the periodic() shell function */

EXTERN time_t lastperiod;

/* $SECONDS = time(NULL) - shtimer.tv_sec */

EXTERN struct timeval shtimer;

EXTERN long mypid;		/* $$ */
EXTERN long lastpid;		/* $! */
EXTERN long ppid;		/* $PPID */

/* the process group of the shell */

EXTERN long mypgrp;

EXTERN char *pwd;		/* $PWD */
EXTERN char *zoptarg;		/* $OPTARG */
EXTERN long zoptind;		/* $OPTIND */
EXTERN char *prompt;		/* $PROMPT */
EXTERN char *rprompt;		/* $RPROMPT */
EXTERN char *prompt2;		/* etc. */
EXTERN char *prompt3;
EXTERN char *prompt4;
EXTERN char *sprompt;
EXTERN char *timefmt;
EXTERN char *watchfmt;
EXTERN char *wordchars;
EXTERN char *fceditparam;
EXTERN char *tmpprefix;
EXTERN char *rstring, *Rstring;
EXTERN char *postedit;

EXTERN char *argzero;		/* $0 */

EXTERN char *hackzero;

/* the hostname */

EXTERN char *hostnam;

EXTERN char *home;		/* $HOME */
EXTERN char **pparams;		/* $argv */

/* the default command for null commands */

EXTERN char *nullcmd;
EXTERN char *readnullcmd;

/* the List of local variables we have to destroy */

EXTERN Lklist locallist;

/* what level of localness we are at */

EXTERN int locallevel;

/* what level of sourcing we are at */

EXTERN int sourcelevel;

/* the shell input fd */

EXTERN int SHIN;

/* the shell tty fd */

EXTERN int SHTTY;

/* the stack of aliases we are expanding */

EXTERN struct alias *alstack[MAXAL];

/* the alias stack pointer; also, the number of aliases currently
 	being expanded */

EXTERN int alstackind;

/* != 0 means we are reading input from a string */

EXTERN int strin;

/* period between periodic() commands, in seconds */

EXTERN long period;

/* != 0 means history substitution is turned off */

EXTERN int stophist;

EXTERN int lithist;

/* this line began with a space, so junk it if HISTIGNORESPACE is on */

EXTERN int spaceflag;

/* don't do spelling correction */

EXTERN int nocorrect;

/* != 0 means we have removed the current event from the history List */

EXTERN int histremmed;

/* the options; e.g. if opts['a'] == OPT_SET, -a is turned on */

EXTERN int opts[128];

EXTERN long keytimeout;		/* KEYTIMEOUT */
EXTERN long lineno;		/* LINENO */
EXTERN long listmax;		/* LISTMAX */
EXTERN long savehist;		/* SAVEHIST */
EXTERN long shlvl;		/* SHLVL */
EXTERN long tmout;		/* TMOUT */
EXTERN long dirstacksize;	/* DIRSTACKSIZE */

/* != 0 means we have called execlist() and then intend to exit(),
 	so don't fork if not necessary */

EXTERN int exiting;

EXTERN int lastbase;		/* last input base we used */

/* the limits for child processes */

#ifdef RLIM_INFINITY
EXTERN struct rlimit limits[RLIM_NLIMITS];

#endif

/* the current word in the history List */

EXTERN char *hlastw;

/* pointer into the history line */

EXTERN char *hptr;

/* the current history line */

EXTERN char *chline;

/* the termcap buffer */

EXTERN char termbuf[1024];

/* $TERM */

EXTERN char *term;

/* != 0 if this $TERM setup is usable */

EXTERN int termok;

/* flag for CSHNULLGLOB */

EXTERN int badcshglob;

/* max size of histline */

EXTERN int hlinesz;

/* the alias expansion status - if == ALSTAT_MORE, we just finished
	expanding an alias ending with a space */

EXTERN int alstat;

/* we have printed a 'you have stopped (running) jobs.' message */

EXTERN int stopmsg;

/* the default tty state */

EXTERN struct ttyinfo shttyinfo;

/* $TTY */

EXTERN char *ttystrname;

/* 1 if ttyctl -f has been executed */

EXTERN int ttyfrozen;

/* != 0 if we are allocating in the heaplist */

EXTERN int useheap;

/* Words on the command line, for use in completion */

EXTERN int clwsize, clwnum, clwpos;
EXTERN char **clwords;

/* pid of process undergoing 'process substitution' */

EXTERN int cmdoutpid;

/* exit status of process undergoing 'process substitution' */

EXTERN int cmdoutval;

/* 1 if aliases should not be expanded */

EXTERN int noaliases;

#include "signals.h"

#ifdef GLOBALS

/* signal names */
char **sigptr = sigs;

/* tokens */
char *ztokens = "#$^*()$=|{}[]`<>?~`,'\"\\";

#else
extern char *ztokens, **sigptr;

#endif

#define SIGZERR (SIGCOUNT+1)
#define SIGDEBUG (SIGCOUNT+2)
#define VSIGCOUNT (SIGCOUNT+3)
#define SIGEXIT 0

/* signals that are trapped = 1, signals ignored =2 */

EXTERN int sigtrapped[VSIGCOUNT];

/* trap functions for each signal */

EXTERN List sigfuncs[VSIGCOUNT];

/* $HISTCHARS */

EXTERN unsigned char bangchar, hatchar, hashchar;

EXTERN int eofseen;

/* we are parsing a line sent to use by the editor */

EXTERN int zleparse;

EXTERN int wordbeg;

EXTERN int parbegin;

/* used in arrays of lists instead of NULL pointers */

EXTERN struct list dummy_list;

/* interesting termcap strings */

#define TCCLEARSCREEN 0
#define TCLEFT 1
#define TCMULTLEFT 2
#define TCRIGHT 3
#define TCMULTRIGHT 4
#define TCUP 5
#define TCMULTUP 6
#define TCDOWN 7
#define TCMULTDOWN 8
#define TCDEL 9
#define TCMULTDEL 10
#define TCINS 11
#define TCMULTINS 12
#define TCCLEAREOD 13
#define TCCLEAREOL 14
#define TCINSLINE 15
#define TCDELLINE 16
#define TCNEXTTAB 17
#define TC_COUNT 18

/* lengths of each string */

EXTERN int tclen[TC_COUNT];

EXTERN char *tcstr[TC_COUNT];

#ifdef GLOBALS

/* names of the strings we want */

char *tccapnams[TC_COUNT] =
{
    "cl", "le", "LE", "nd", "RI", "up", "UP", "do",
    "DO", "dc", "DC", "ic", "IC", "cd", "ce", "al", "dl", "ta"
};

#else
extern char *tccapnams[TC_COUNT];

#endif

#define tccan(X) (tclen[X])

#define HISTFLAG_DONE   1
#define HISTFLAG_NOEXEC 2
#define HISTFLAG_RECALL 4

#ifdef HAS_SETPGID
#define setpgrp setpgid
#endif

#define _INCLUDE_POSIX_SOURCE
#define _INCLUDE_XOPEN_SOURCE
#define _INCLUDE_HPUX_SOURCE

#ifdef SV_BSDSIG
#define SV_INTERRUPT SV_BSDSIG
#endif

#if defined(POSIX) || !defined(SYSV)
#define SIGNAL_MASKS
#endif

#ifndef POSIX
typedef unsigned int sigset_t;
typedef unsigned int mode_t;

#define sigemptyset(s)    (*(s) = 0)
#if NSIG == 32
#define sigfillset(s)     ((*(s) = 0xffffffff), 0)
#else
#define sigfillset(s)     ((*(s) = (1 << NSIG) - 1), 0)
#endif
#define z_sigmask(n)      (1 << ((n) - 1))
#define sigaddset(s,n)    ((*(s) |= z_sigmask(n)), 0)
#define sigdelset(s,n)    ((*(s) &= ~z_sigmask(n)), 0)
#define sigismember(s,n)  ((*(s) & z_sigmask(n)) ? 1 : 0)
#endif

#define blockchld()        sig_block(sig_mask(SIGCHLD))
#define unblockchld()      sig_unblock(sig_mask(SIGCHLD))
#define chldpause(S)       sig_suspend(SIGCHLD, (S))

#ifdef SIGNAL_MASKS
#define fast_block(s)      (void)(*(s) = sig_block(sig_notmask(0)))
#define fast_unblock(s)    (void)sig_setmask(*(s))
#else
#define fast_block(s)      (void)sigemptyset(s)
#define fast_unblock(s)    (void)(s)
#endif

EXTERN int zigsig, zigblock;
EXTERN sigset_t zigmask;

#define zigunsafe()	(zigblock++ ? 0 : (zigsig = 0))
#define zighold(z,m)	(zigsig ? 0 : (zigsig = (z), zigmask = (m), 0))
#define zigheld		(zigblock > 0)
#define zigsafe()	if (zigheld && zigsig) { \
			    if (zigblock > 0) \
				zigblock--; \
			    if (zigblock == 0) { \
				int zs = zigsig; \
				zigsig = 0; \
				fast_unblock(&zigmask); \
				handler(zs); \
			    } \
			} else { \
			    if (zigblock > 0) \
				zigblock--; \
			}

#include "ztype.h"
#include "funcs.h"

/* the command stack for use with %_ in prompts */

EXTERN unsigned char *cmdstack;
EXTERN int cmdsp;

#define cmdpush(X) if (!(cmdsp >= 0 && cmdsp < 256)) {;} else cmdstack[cmdsp++]=(X)
#define cmdpop() if (cmdsp <= 0) {;} else cmdsp--

#define CS_FOR          0
#define CS_WHILE        1
#define CS_REPEAT       2
#define CS_SELECT       3
#define CS_UNTIL        4
#define CS_IF           5
#define CS_IFTHEN       6
#define CS_ELSE         7
#define CS_ELIF         8
#define CS_MATH         9
#define CS_COND        10
#define CS_CMDOR       11
#define CS_CMDAND      12
#define CS_PIPE        13
#define CS_ERRPIPE     14
#define CS_FOREACH     15
#define CS_CASE        16
#define CS_FUNCDEF     17
#define CS_SUBSH       18
#define CS_CURSH       19
#define CS_ARRAY       20
#define CS_QUOTE       21
#define CS_DQUOTE      22
#define CS_BQUOTE      23
#define CS_CMDSUBST    24
#define CS_MATHSUBST   25
#define CS_ELIFTHEN    26
#define CS_HEREDOC     27
#define CS_HEREDOCD    28

#ifndef GLOBALS
extern char *cmdnames[];

#else
char *cmdnames[] =
{
    "for",
    "while",
    "repeat",
    "select",
    "until",
    "if",
    "then",
    "else",
    "elif",
    "math",
    "cond",
    "cmdor",
    "cmdand",
    "pipe",
    "errpipe",
    "foreach",
    "case",
    "function",
    "subsh",
    "cursh",
    "array",
    "quote",
    "dquote",
    "bquote",
    "cmdsubst",
    "mathsubst",
    "elif-then",
    "heredoc",
    "heredocd",
};

#endif
