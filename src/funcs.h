/* structure for foo=bar assignments */
struct asgment {
    struct asgment *next;
    char *name, *value;
};

#include "builtin.pro"
#include "cond.pro"
#include "exec.pro"
#include "glob.pro"
#include "hist.pro"
#include "init.pro"
#include "jobs.pro"
#include "lex.pro"
#include "loop.pro"
#include "math.pro"
#include "mem.pro"
#include "params.pro"
#include "parse.pro"
#include "subst.pro"
#include "table.pro"
#include "text.pro"
#include "utils.pro"
#include "watch.pro"
#include "zle_hist.pro"
#include "zle_main.pro"
#include "zle_misc.pro"
#include "zle_move.pro"
#include "zle_refresh.pro"
#include "zle_tricky.pro"
#include "zle_utils.pro"
#include "zle_vi.pro"
#include "zle_word.pro"

char *mktemp DCLPROTO((char *));

#ifndef HAS_STDLIB
char *malloc DCLPROTO((int));
char *realloc DCLPROTO((char *, int));
char *calloc DCLPROTO((int, int));

#endif
char *ttyname DCLPROTO((int));

extern char PC, *BC, *UP;
extern short ospeed;
extern int tgetent DCLPROTO((char *bp, char *name));
extern int tgetnum DCLPROTO((char *id));
extern int tgetflag DCLPROTO((char *id));
extern char *tgetstr DCLPROTO((char *id, char **area));
extern char *tgoto DCLPROTO((char *cm, int destcol, int destline));
extern int tputs DCLPROTO((char *cp, int affcnt, int (*outc) (int)));

/* missing prototypes for various C compilers */

#if defined(SOLARIS)
#include <setjmp.h>
/* Solaris does not seem to have prototype for these under /usr/include */
extern char *getdomainname DCLPROTO((char *name, int namelen));
extern FILE *fdopen(int, const char *);
extern int kill(pid_t, int);
extern int sigaction(int, const struct sigaction *, struct sigaction *);
extern int sighold(int);
extern int sigrelse(int);
extern int sigpause(int);
extern int gettimeofday(struct timeval *tp, struct timezone *tzp);

#endif

#if defined(__convexc__)
/* ConvexOS does not seem to have prototypes for these under /usr/include */
extern int bzero(char *b, int length);
extern int gethostname(char *name, int namelen);
extern int getrlimit(int resource, struct rlimit *rlp);
extern int getrusage(int, struct rusage *);
extern int gettimeofday(struct timeval *tp, struct timezone *tzp);
extern int mknod(char *path, int mode, int dev);
extern int nice(int incr);
extern int readlink(char *path, char *buf, int bufsize);
extern int select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * expectfds, struct timeval *timeout);
extern int setrlimit(int resource, struct rlimit *rlp);
extern int sigblock(int mask);
extern int sigpause(int sigmask);
extern int sigsetmask(int mask);
extern int sigvec(int sig, struct sigvec *vec, struct sigvec *ovec);
extern int wait3(int *status, int options, struct rusage *rusage);

#endif

/* HP/UX 9 c89 */
#if defined(__hpux) && defined(_XPG3) && !defined(_POSIX1_1988)
#define WRITE_ARG_2_T void *
#else
#define WRITE_ARG_2_T char *
#endif

#if defined(__hpux) && defined(_HPUX_SOURCE)
#define SELECT_ARG_2_T int *
#else
#define SELECT_ARG_2_T fd_set *
#endif

#if defined(__sgi)
#if defined(_POSIX_SOURCE)
/* cc -ansiposix pretends not to see these, maybe too BSDish? */
extern void setpwent(void);
extern void endpwent(void);
extern struct passwd *getpwent(void);

#endif
/* cc -ansi/-ansiposix pretends not to see these, maybe too BSDish/SYSVish? */
#if defined(__STDC__)
#ifdef IRIX5
#ifdef _POSIX_SOURCE
extern int BSDgettimeofday(struct timeval *tp, struct timezone *tzp);
extern int getrusage(int rwho, struct rusage *rusage);
extern int lstat(const char *path, struct stat *buf);
extern int readlink(const char *path, void *buf, size_t bufsiz);

#endif
extern int wait3(union wait *statptr, int options, struct rusage *rusage);

#else
extern int (*BSDsignal(int, int (*)())) ();
extern int kill(pid_t pid, int sig);
extern int readlink(const char *pathname, char *buf, int bufsiz);

#endif
extern void bzero(void *b, int length);
extern int gethostname(char *name, int namelen);
extern int ioctl(int fildes, int request,...);
extern int mknod(const char *pathname, mode_t mode, dev_t dev);
extern int nice(int incr);
extern void seekdir(DIR * dirp, long loc);
extern int select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval *timeout);
extern int sighold(int sig);
extern int sigrelse(int sig);
extern int sigpause(int sig);

#endif
#endif

#if defined(_CRAY) && defined(__STDC__)
extern FILE *fdopen(int fildes, char *type);

#include <setjmp.h>
typedef int sigjmp_buf[_SJBLEN];
extern int _Sigsetjmp __((sigjmp_buf _Env, int _Savemask));

#define sigsetjmp(_Env, _Savemask)  _Sigsetjmp(_Env, _Savemask)
extern void siglongjmp __((sigjmp_buf _Env, int _Val));

#endif

#if defined(__NeXT__)
#if defined(__STRICT_ANSI__)	/* -ansi does not see these */
#define bzero(b,len) memset(b,0,len)
extern FILE *fdopen(int filedes, const char *mode);

#endif
/* NeXT has almost everything in
 * /usr/include/bsd/libc.h == <libc.h>
 * (sort of <unistd.h>), except the following */
extern int getppid(void);

#endif

#if defined(__osf__) && defined(__alpha)
/* Digital cc does not need these prototypes, gcc does need them */
void bzero(char *string, int length);
int gethostname(char *address, int address_len);
int ioctl(int d, unsigned long request, char *argp);
int mknod(const char *pathname, int mode, dev_t device);
int nice(int increment);
int select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval *timeout);

#endif

#if defined(DGUX) && defined(__STDC__)
/* Just plain missing. */
extern int getrlimit(int resource, struct rlimit *rlp);
extern int setrlimit(int resource, const struct rlimit *rlp);
extern int getrusage(int who, struct rusage *rusage);
extern int gettimeofday(struct timeval *time_value,
			struct timezone *time_zone);
extern int wait3(union wait *wait_status, int options, struct rusage *rusage);
extern int gethostname(char *nameptr, int maxlength);
extern int getdomainname(char *name, int maxlength);
extern int bzero(char *ptr, int length);
extern int select(int nfds, fd_set * readfds, fd_set * writefds,
		  fd_set * exceptfds, struct timeval *timeout);

#endif

#if defined(SCO)
struct timezone {
    short minutes;
};
extern void gettimeofday(struct timeval *tv, struct timezone *tz);
extern int nice(int incr);
extern int mknod(char *path, int mode, int dev);
extern int ioctl(int fildes, int request,...);

#endif
