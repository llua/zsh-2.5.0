/*
 *
 * glob.c - filename generation
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
#include <pwd.h>
#include <grp.h>

#define exists(X) (access(X,0) == 0 || readlink(X,NULL,0) == 0)

static int mode;		/* != 0 if we are parsing glob patterns */
static int pathpos;		/* position in pathbuf */
static int matchsz;		/* size of matchbuf */
static int matchct;		/* number of matches found */
static char pathbuf[MAXPATHLEN];/* pathname buffer */
static char **matchbuf;		/* array of matches */
static char **matchptr;		/* &matchbuf[matchct] */
static char *colonmod;		/* colon modifiers in qualifier list */
static ino_t old_ino;
static dev_t old_dev;
static int old_pos;

#ifdef ULTRIX
typedef struct stat *Statptr;	/* This makes the Ultrix compiler happy.  Go figure. */

#endif

#define TT_DAYS 0
#define TT_HOURS 1
#define TT_MINS 2
#define TT_WEEKS 3
#define TT_MONTHS 4

/* max # of qualifiers */

struct qual {
    struct qual *next, *or;
    int (*func) DCLPROTO((struct stat *, long));
    long data;
    int sense;
    int amc;
    int range;
    int timef;
};

static struct qual *quals;

static int qualct, qualorct;
static int range, amc, timef;
static int gf_nullglob, gf_markdirs, gf_noglobdots, gf_listtypes;

char *glob_pre, *glob_suf;

/* pathname component in filename patterns */

struct complist {
    Complist next;
    Comp comp;
    int closure;		/* 1 if this is a (foo/)# */
};
struct comp {
    Comp left, right, next, exclude;
    char *str;
    int stat;
};

#define C_ONEHASH	1
#define C_TWOHASH	2
#define C_CLOSURE	(C_ONEHASH|C_TWOHASH)
#define C_LAST		4
#define C_PATHADD	8

#define CLOSUREP(c)	(c->stat & C_CLOSURE)
#define ONEHASHP(c)	(c->stat & C_ONEHASH)
#define TWOHASHP(c)	(c->stat & C_TWOHASH)
#define LASTP(c)	(c->stat & C_LAST)
#define PATHADDP(c)	(c->stat & C_PATHADD)

void glob(list, np)		/**/
Lklist list;
Lknode *np;
{
    struct qual *qo, *qn, *ql;
    Lknode node = prevnode(*np);
    Lknode next = nextnode(*np);
    int luh = useheap;
    char *str;			/* the pattern */
    int sl;			/* length of the pattern */
    Complist q;			/* pattern after parsing */
    char *ostr = (char *)getdata(*np);	/* the pattern before the parser chops it up */

    heapalloc();
    str = dupstring(ostr);
    if (!luh)
	permalloc();
    sl = strlen(str);
    uremnode(list, *np);
    qo = qn = quals = ql = NULL;
    qualct = qualorct = 0;
    colonmod = NULL;
    gf_nullglob = isset(NULLGLOB);
    gf_markdirs = isset(MARKDIRS);
    gf_listtypes = 0;
    gf_noglobdots = unset(GLOBDOTS);
    if (str[sl - 1] == Outpar) {/* check for qualifiers */
	char *s;
	int sense = 0;
	long data = 0;

#ifdef ULTRIX
	int (*func) DCLPROTO((Statptr, long));

#else
	int (*func) DCLPROTO((struct stat *, long));

#endif

	for (s = str + sl - 2; s != str; s--)
	    if (*s == Bar || *s == Outpar || *s == Inpar
		|| (isset(EXTENDEDGLOB) && *s == Tilde))
		break;
	if (*s == Inpar) {
	    *s++ = '\0';
	    while (*s != Outpar && !colonmod) {
#ifdef ULTRIX
		func = (int (*)DCLPROTO((Statptr, long)))0;
#else
		func = (int (*)DCLPROTO((struct stat *, long)))0;
#endif
		if (idigit(*s)) {
		    func = qualflags;
		    data = 0;
		    while (idigit(*s))
			data = data * 010 + (*s++ - '0');
		} else if (*s == ',' || *s == Comma) {
		    s++;
		    if (qualct) {
			qn = (struct qual *)hcalloc(sizeof *qn);
			qo->or = qn;
			qo = qn;
			qualorct++;
			qualct = 0;
			ql = NULL;
		    }
		} else
		    switch ((int)(unsigned char)(*s++)) {
		    case ':':
			colonmod = s - 1;
			break;
		    case (int)STOUC(Hat):
		    case '^':
			sense ^= 1;
			break;
		    case '-':
			sense ^= 2;
			break;
#ifdef S_IFLNK
		    case '@':
			func = qualmode;
			data = S_IFLNK;
			break;
#endif
#ifdef S_IFSOCK
		    case (int)STOUC(Equals):
		    case '=':
			func = qualmode;
			data = S_IFSOCK;
			break;
#endif
#ifdef S_IFIFO
		    case 'p':
			func = qualmode;
			data = S_IFIFO;
			break;
#endif
		    case '/':
			func = qualmode;
			data = S_IFDIR;
			break;
		    case '.':
			func = qualmode;
			data = S_IFREG;
			break;
		    case '%':
			if (*s == 'b')
			    s++, func = qualisblk;
			else if (*s == 'c')
			    s++, func = qualischar;
			else
			    func = qualisdev;
			break;
		    case (int)STOUC(Star):
			func = qualiscom;
			break;
		    case 'R':
			func = qualflags;
			data = 0004;
			break;
		    case 'W':
			func = qualflags;
			data = 0002;
			break;
		    case 'X':
			func = qualflags;
			data = 0001;
			break;
		    case 'r':
			func = qualflags;
			data = 0400;
			break;
		    case 'w':
			func = qualflags;
			data = 0200;
			break;
		    case 'x':
			func = qualflags;
			data = 0100;
			break;
		    case 's':
			func = qualflags;
			data = 04000;
			break;
		    case 'S':
			func = qualflags;
			data = 02000;
			break;
		    case 'd':
			func = qualdev;
			data = qgetnum(&s);
			break;
		    case 'l':
			func = qualnlink;
			amc = -1;
			goto getrange;
		    case 'U':
			func = qualuid;
			data = geteuid();
			break;
		    case 'G':
			func = qualgid;
			data = getegid();
			break;
		    case 'u':
			func = qualuid;
			if (idigit(*s))
			    data = qgetnum(&s);
			else {
			    struct passwd *pw;
			    char sav, *tt;

			    tt = get_strarg(s);
			    if (!*tt) {
				zerr("missing end of name",
				     NULL, 0);
				data = 0;
			    } else {
				sav = *tt;
				*tt = '\0';

				if ((pw = getpwnam(s + 1)))
				    data = pw->pw_uid;
				else {
				    zerr("unknown user", NULL, 0);
				    data = 0;
				}
				if ((*tt = sav) != Outpar)
				    s = tt + 1;
				else
				    s = tt;
			    }
			}
			break;
		    case 'g':
			func = qualgid;
			if (idigit(*s))
			    data = qgetnum(&s);
			else {
			    struct group *gr;
			    char sav, *tt;

			    tt = get_strarg(s);
			    if (!*tt) {
				zerr("missing end of name",
				     NULL, 0);
				data = 0;
			    } else {
				sav = *tt;
				*tt = '\0';

				if ((gr = getgrnam(s + 1)))
				    data = gr->gr_gid;
				else {
				    zerr("unknown group", NULL, 0);
				    data = 0;
				}
				if ((*tt = sav) != Outpar)
				    s = tt + 1;
				else
				    s = tt;
			    }
			}
			break;
		    case 'o':
			func = qualeqflags;
			data = qgetoctnum(&s);
			break;
		    case 'M':
			gf_markdirs = !(sense & 1);
			break;
		    case 'T':
			gf_listtypes = !(sense & 1);
			break;
		    case 'N':
			gf_nullglob = !(sense & 1);
			break;
		    case 'D':
			gf_noglobdots = sense & 1;
			break;
		    case 'a':
			amc = 0;
			func = qualtime;
			goto getrange;
		    case 'm':
			amc = 1;
			func = qualtime;
			goto getrange;
		    case 'c':
			amc = 2;
			func = qualtime;
			goto getrange;
		    case 'L':
			func = qualsize;
			amc = -1;
		      getrange:
			timef = TT_DAYS;
			if (amc >= 0)
			    if (*s == 'h')
				timef = TT_HOURS, ++s;
			    else if (*s == 'm')
				timef = TT_MINS, ++s;
			    else if (*s == 'w')
				timef = TT_WEEKS, ++s;
			    else if (*s == 'M')
				timef = TT_MONTHS, ++s;
			if ((range = *s == '+' ? 1 : *s == '-' ? -1 : 0))
			    ++s;
			data = qgetnum(&s);
			break;

		    default:
			zerr("unknown file attribute", NULL, 0);
			return;
		    }
		if (func) {
		    if (!qn)
			qn = (struct qual *)hcalloc(sizeof *qn);
		    if (ql)
			ql->next = qn;
		    ql = qn;
		    if (!quals)
			quals = qo = qn;
		    qn->func = func;
		    qn->sense = sense;
		    qn->data = data;
		    qn->range = range;
		    qn->timef = timef;
		    qn->amc = amc;
		    qn = NULL;
		    qualct++;
		}
		if (errflag)
		    return;
	    }
	}
    } else if ((str[sl - 1] == '/') &&
	       !((str[sl - 2] == Star) &&
		 (str[sl - 3] == Star))) {	/* foo/ == foo(/) */
	str[sl - 1] = '\0';
	quals = (struct qual *)hcalloc(sizeof *quals);
	quals->func = qualmode;
	quals->data = S_IFDIR;
	quals->sense = 0;
	qualct = 1;
    }
    if (*str == '/') {		/* pattern has absolute path */
	str++;
	pathbuf[0] = '/';
	pathbuf[pathpos = 1] = '\0';
    } else			/* pattern is relative to pwd */
	pathbuf[pathpos = 0] = '\0';
    q = parsepat(str);
    if (!q || errflag) {	/* if parsing failed */
	if (isset(NOBADPATTERN)) {
	    insnode(list, node, ostr);
	    return;
	}
	errflag = 0;
	zerr("bad pattern: %s", ostr, 0);
	return;
    }
    matchptr = matchbuf = (char **)zalloc((matchsz = 16) * sizeof(char *));

    matchct = 0;
    old_ino = (ino_t) 0;
    old_dev = (dev_t) 0;
    old_pos = -1;
    scanner(q);			/* do the globbing */
    if (matchct)
	badcshglob |= 2;
    else if (!gf_nullglob)
	if (isset(CSHNULLGLOB)) {
	    badcshglob |= 1;
	} else if (unset(NONOMATCH)) {
	    zerr("no matches found: %s", ostr, 0);
	    free(matchbuf);
	    return;
	} else {
	    *matchptr++ = dupstring(ostr);
	    matchct = 1;
	}
    qsort((vptr) & matchbuf[0], matchct, sizeof(char *),
	       (int (*)DCLPROTO((const void *, const void *)))notstrcmp);

    matchptr = matchbuf;
    while (matchct--)		/* insert matches in the arg list */
	insnode(list, node, *matchptr++);
    free(matchbuf);
    *np = (next) ? prevnode(next) : lastnode(list);
}

/* get number after qualifier */

long qgetnum(s)			/**/
char **s;
{
    long v = 0;

    if (!idigit(**s)) {
	zerr("number expected", NULL, 0);
	return 0;
    }
    while (idigit(**s))
	v = v * 10 + *(*s)++ - '0';
    return v;
}

/* get octal number after qualifier */

long qgetoctnum(s)		/**/
char **s;
{
    long v = 0;

    if (!idigit(**s)) {
	zerr("octal number expected", NULL, 0);
	return 0;
    }
    while (**s >= '0' && **s <= '7')
	v = v * 010 + *(*s)++ - '0';
    return v;
}

int notstrcmp(a, b)		/**/
char **a;
char **b;
{
    char *c = *b, *d = *a;
    int x1, x2, cmp;

    for (; *c == *d && *c; c++, d++);
    cmp = (int)(unsigned char)*c - (int)(unsigned char)*d;
    if (isset(NUMERICGLOBSORT)) {
	for (; c > *b && idigit(c[-1]); c--, d--);
	if (idigit(*c) && idigit(*d)) {
	    x1 = atoi(c);
	    x2 = atoi(d);
	    if (x1 != x2)
		return x1 - x2;
	}
    }
    return cmp;
}

int forstrcmp(a, b)		/**/
char **a;
char **b;
{
    char *c = *b, *d = *a;

    for (; *c == *d && *c; c++, d++);
    return ((int)(unsigned char)*d - (int)(unsigned char)*c);
}

/* add a match to the list */

void insert(s)			/**/
char *s;
{
    struct stat buf, buf2, *bp;
    int statted = 0;

    if (gf_listtypes || gf_markdirs) {
	statted = 1;
	if (!lstat(s, &buf) && (gf_listtypes || S_ISDIR(buf.st_mode))) {
	    char *t;
	    int ll = strlen(s);

	    t = (char *)ncalloc(ll + 2);
	    strcpy(t, s);
	    t[ll] = file_type(buf.st_mode);
	    t[ll + 1] = '\0';
	    s = t;
	}
    }
    if (qualct || qualorct) {	/* do the (X) (^X) stuff */
	struct qual *qo, *qn;
	int t = 0;

	if (statted || lstat(s, &buf) >= 0) {
	    statted = 0;
	    for (qo = quals; qo && !t; qo = qo->or) {

		t = 1;
		for (qn = qo; t && qn && qn->func; qn = qn->next) {
		    range = qn->range;
		    amc = qn->amc;
		    timef = qn->timef;
		    if ((qn->sense & 2) && !statted) {
			statted = 1;
			stat(s, &buf2);
		    }
		    bp = (qn->sense & 2) ? &buf2 : &buf;
		    if (!(!!((qn->func) (bp, qn->data)) ^
			  (qn->sense & 1))) {
			t = 0;
			break;
		    }
		}
	    }
	}
	if (!t)
	    return;
    }
    if (colonmod) {
	char *cm2 = colonmod;

	modify(&s, &cm2);
    }
    *matchptr++ = s;
    if (++matchct == matchsz) {
	matchbuf = (char **)realloc((char *)matchbuf,
				    sizeof(char **) * (matchsz *= 2));

	matchptr = matchbuf + matchct;
    }
}

#ifdef __STDC__
char file_type(mode_t filemode)
{
#else
char file_type(filemode)	/**/
mode_t filemode;
{
#endif
    switch (filemode & S_IFMT) {/* screw POSIX */
    case S_IFDIR:
	return '/';
#ifdef S_IFIFO
    case S_IFIFO:
	return '|';
#endif
    case S_IFCHR:
	return '%';
    case S_IFBLK:
	return '#';
#ifdef S_IFLNK
    case S_IFLNK:
	return /* (access(pbuf, F_OK) == -1) ? '&' :*/ '@';
#endif
#ifdef S_IFSOCK
    case S_IFSOCK:
	return '=';
#endif
    default:
	if (filemode & 0111)
	    return '*';
	else
	    return ' ';
    }
}

/* check to see if str is eligible for filename generation */

int haswilds(str)		/**/
char *str;
{
    if ((*str == Inbrack || *str == Outbrack) && !str[1])
	return 0;
    if (str[0] == '%')
	return 0;
    for (; *str; str++)
	if (*str == Pound || *str == Hat || *str == Star ||
	    *str == Bar || *str == Inbrack || *str == Inang ||
	    *str == Quest || (*str == Inpar && str[1] == ':'))
	    return 1;
    return 0;
}

/* check to see if str is eligible for brace expansion */

int hasbraces(str)		/**/
char *str;
{
    int mb, bc, cmct1, cmct2;
    char *lbr = NULL;

    if (str[0] == Inbrace && str[1] == Outbrace)
	return 0;
    if (isset(BRACECCL)) {
	for (mb = bc = 0; *str; ++str)
	    if (*str == Inbrace) {
		if (++bc > mb)
		    mb = bc;
	    } else if (*str == Outbrace)
		if (--bc < 0)
		    return (0);
	return (mb && bc == 0);
    }
    for (mb = bc = cmct1 = cmct2 = 0; *str; str++) {
	if (*str == Inbrace) {
	    if (!bc)
		lbr = str;
	    bc++;
	    if (str[4] == Outbrace && str[2] == '-') {	/* {a-z} */
		cmct1++;
		if (bc == 1)
		    cmct2++;
	    }
	} else if (*str == Outbrace) {
	    bc--;
	    if (!bc) {
		if (!cmct2) {
		    *lbr = '{';
		    *str = '}';
		}
		cmct2 = 0;
	    }
	} else if (*str == Comma && bc) {
	    cmct1++;
	    if (bc == 1)
		cmct2++;
	}
	if (bc > mb)
	    mb = bc;
	if (bc < 0)
	    return 0;
    }
    return (mb && bc == 0 && cmct1);
}

/* expand stuff like >>*.c */

int xpandredir(fn, tab)		/**/
struct redir *fn;
Lklist tab;
{
    Lklist fake;
    char *nam;
    struct redir *ff;
    int ret = 0;

    fake = newlist();
    addnode(fake, fn->name);
    prefork(fake, 0);
    if (!errflag)
	postfork(fake, 1);
    if (errflag)
	return 0;
    if (full(fake) && !nextnode(firstnode(fake))) {
	fn->name = (char *)peekfirst(fake);
	untokenize(fn->name);
    } else
	while ((nam = (char *)ugetnode(fake))) {
	    ff = (struct redir *)alloc(sizeof *ff);
	    *ff = *fn;
	    ff->name = nam;
	    addnode(tab, ff);
	    ret = 1;
	}
    return ret;
}

/* concatenate s1 and s2 in dynamically allocated buffer */

char *dyncat(s1, s2)		/**/
char *s1;
char *s2;
{
    char *ptr;

    ptr = (char *)ncalloc(strlen(s1) + strlen(s2) + 1);
    strcpy(ptr, s1);
    strcat(ptr, s2);
    return ptr;
}

/* concatenate s1, s2, and s3 in dynamically allocated buffer */

char *tricat(s1, s2, s3)	/**/
char *s1;
char *s2;
char *s3;
{
    char *ptr;

    ptr = (char *)zalloc(strlen(s1) + strlen(s2) + strlen(s3) + 1);
    strcpy(ptr, s1);
    strcat(ptr, s2);
    strcat(ptr, s3);
    return ptr;
}

/* brace expansion */

void xpandbraces(list, np)	/**/
Lklist list;
Lknode *np;
{
    Lknode node = (*np), last = prevnode(node);
    char *str = (char *)getdata(node), *str3 = str, *str2;
    int prev, bc, comma;

    for (; *str != Inbrace; str++);
    for (str2 = str, bc = comma = 0; *str2; ++str2)
	if (*str2 == Inbrace)
	    ++bc;
	else if (*str2 == Outbrace) {
	    if (--bc == 0)
		break;
	} else if (bc == 1 && *str2 == Comma)
	    ++comma;
    if (!comma && !bc && isset(BRACECCL)) {	/* {a-mnop} */
	char ccl[256], *p;
	unsigned char c1, c2, lastch;

	uremnode(list, node);
	memset(ccl, 0, sizeof(ccl) / sizeof(ccl[0]));
	for (p = str + 1, lastch = 0; p < str2;) {
	    if (itok(c1 = *p++))
		c1 = ztokens[c1 - STOUC(Pound)];
	    if (itok(c2 = *p))
		c2 = ztokens[c2 - STOUC(Pound)];
	    if (c1 == '-' && lastch && p < str2 && (int)lastch <= (int)c2) {
		while ((int)lastch < (int)c2)
		    ccl[lastch++] = 1;
		lastch = 0;
	    } else
		ccl[lastch = c1] = 1;
	}
	strcpy(str + 1, str2 + 1);
	for (p = ccl + 255; p-- > ccl;)
	    if (*p) {
		*str = p - ccl;
		insnode(list, last, dupstring(str3));
	    }
	*np = nextnode(last);
	return;
    }
    if (str[2] == '-' && str[4] == Outbrace) {	/* {a-z} */
	char c1, c2;

	uremnode(list, node);
	chuck(str);
	c1 = *str;
	chuck(str);
	chuck(str);
	c2 = *str;
	chuck(str);
	if (itok(c1))
	    c1 = ztokens[c1 - Pound];
	if (itok(c2))
	    c2 = ztokens[c2 - Pound];
	if (c1 < c2)
	    for (; c2 >= c1; c2--) {	/* {a-z} */
		*str = c2;
		insnode(list, last, dupstring(str3));
	} else
	    for (; c2 <= c1; c2++) {	/* {z-a} */
		*str = c2;
		insnode(list, last, dupstring(str3));
	    }
	*np = nextnode(last);
	return;
    }
    prev = str - str3;
    str2 = getparen(str++);
    if (!str2) {
	zerr("how did you get this error?", NULL, 0);
	return;
    }
    uremnode(list, node);
    node = last;
    for (;;) {
	char *zz, *str4;
	int cnt;

	for (str4 = str, cnt = 0; cnt || (*str != Comma && *str !=
					  Outbrace); str++)
	    if (*str == Inbrace)
		cnt++;
	    else if (*str == Outbrace)
		cnt--;
	    else if (!*str)
		exit(10);
	zz = (char *)zalloc(prev + (str - str4) + strlen(str2) + 1);
	ztrncpy(zz, str3, prev);
	strncat(zz, str4, str - str4);
	strcat(zz, str2);
	insnode(list, node, zz);
	incnode(node);
	if (*str != Outbrace)
	    str++;
	else
	    break;
    }
    *np = nextnode(last);
}

/* get closing paren, given pointer to opening paren */

char *getparen(str)		/**/
char *str;
{
    int cnt = 1;
    char typein = *str++, typeout = typein + 1;

    for (; *str && cnt; str++)
	if (*str == typein)
	    cnt++;
	else if (*str == typeout)
	    cnt--;
    if (!str && cnt)
	return NULL;
    return str;
}

/* check to see if a matches b (b is not a filename pattern) */

int matchpat(a, b)		/**/
char *a;
char *b;
{
    Comp c;
    int val, len;
    char *b2;

    remnulargs(b);
    len = strlen(b);
    b2 = (char *)alloc(len + 3);
    strcpy(b2 + 1, b);
    b2[0] = Inpar;
    b2[len + 1] = Outpar;
    b2[len + 2] = '\0';
    c = parsereg(b2);
    if (!c) {
	zerr("bad pattern: %s", b, 0);
	return 0;
    }
    val = domatch(a, c, 0);
    return val;
}

/* do the ${foo%%bar}, ${foo#bar} stuff */
/* please do not laugh at this code. */

char *get_match_ret(s, b, e, fl)/**/
char *s;
int b;
int e;
int fl;
{
    char buf[80], *r, *p, *rr;
    int ll = 0, l = strlen(s), bl = 0, t = 0, i;

    if (fl & 8)
	ll += 1 + (e - b);
    if (fl & 16)
	ll += 1 + (l - (e - b));
    if (fl & 32) {
	sprintf(buf, "%d ", b + 1);
	ll += (bl = strlen(buf));
    }
    if (fl & 64) {
	sprintf(buf + bl, "%d ", e + 1);
	ll += (bl = strlen(buf));
    }
    if (fl & 128) {
	sprintf(buf + bl, "%d ", e - b);
	ll += (bl = strlen(buf));
    }
    if (bl)
	buf[bl - 1] = '\0';

    rr = r = (char *)ncalloc(ll);

    if (fl & 8) {
	for (i = b, p = s + b; i < e; i++)
	    *rr++ = *p++;
	t = 1;
    }
    if (fl & 16) {
	if (t)
	    *rr++ = ' ';
	for (i = 0, p = s; i < b; i++)
	    *rr++ = *p++;
	for (i = e, p = s + e; i < l; i++)
	    *rr++ = *p++;
	t = 1;
    }
    *rr = '\0';
    if (bl) {
	if (t)
	    *rr++ = ' ';
	strcpy(rr, buf);
    }
    return r;
}

void getmatch(sp, pat, fl, n)	/**/
char **sp;
char *pat;
int fl;
int n;
{
    Comp c;
    char *s = *sp, *t, sav;
    int i, j, l = strlen(*sp);

    remnulargs(pat);
    c = parsereg(pat);
    if (!c) {
	zerr("bad pattern: %s", pat, 0);
	return;
    }
    switch (fl & 7) {
    case 0:
	for (i = 1, t = s + 1; i <= l; i++, t++) {
	    sav = *t;
	    *t = '\0';
	    if (domatch(s, c, 0) && !--n) {
		*t = sav;
		*sp = get_match_ret(*sp, 0, i, fl);
		return;
	    }
	    *t = sav;
	}
	break;

    case 1:
	for (t = s + l - 1; t >= s; t--) {
	    if (domatch(t, c, 0) && !--n) {
		*sp = get_match_ret(*sp, t - s, l, fl);
		return;
	    }
	}
	break;

    case 2:
	for (t = s + l; t > s; t--) {
	    sav = *t;
	    *t = '\0';
	    if (domatch(s, c, 0) && !--n) {
		*t = sav;
		*sp = get_match_ret(*sp, 0, t - s, fl);
		return;
	    }
	    *t = sav;
	}
	break;

    case 3:
	for (i = 0, t = s; i < l; i++, t++) {
	    if (domatch(t, c, 0) && !--n) {
		*sp = get_match_ret(*sp, i, l, fl);
		return;
	    }
	}
	break;

    case 4:
	for (i = 1; i <= l; i++) {
	    for (t = s + i, j = i; j <= l; j++, t++) {
		sav = *t;
		*t = '\0';
		if (domatch(s + i - 1, c, 0) && !--n) {
		    *t = sav;
		    *sp = get_match_ret(*sp, i - 1, j, fl);
		    return;
		}
		*t = sav;
	    }
	}
	break;

    case 5:
	for (i = 1; i <= l; i++) {
	    for (t = s + l, j = i; j <= l; j++, t--) {
		sav = *t;
		*t = '\0';
		if (domatch(t - i, c, 0) && !--n) {
		    *t = sav;
		    *sp = get_match_ret(*sp, l - j, t - s, fl);
		    return;
		}
		*t = sav;
	    }
	}
	break;

    case 6:
	for (i = l; i; i--) {
	    for (t = s, j = i; j <= l; j++, t++) {
		sav = t[i];
		t[i] = '\0';
		if (domatch(t, c, 0) && !--n) {
		    t[i] = sav;
		    *sp = get_match_ret(*sp, t - s, t - s + i, fl);
		    return;
		}
		t[i] = sav;
	    }
	}
	break;

    case 7:
	for (i = l; i; i--) {
	    for (t = s + l, j = i; j <= l; j++, t--) {
		sav = *t;
		*t = '\0';
		if (domatch(t - i, c, 0) && !--n) {
		    *t = sav;
		    *sp = get_match_ret(*sp, l - j, t - s, fl);
		    return;
		}
		*t = sav;
	    }
	}
	break;
    }
    *sp = get_match_ret(*sp, 0, 0, fl);
}

/* add a component to pathbuf */

SPROTO(int addpath, (char *s));

static int addpath(s)
char *s;
{
    if ((int)strlen(s) + pathpos >= MAXPATHLEN)
	return 0;
    while ((pathbuf[pathpos++] = *s++));
    pathbuf[pathpos - 1] = '/';
    pathbuf[pathpos] = '\0';
    return 1;
}

char *getfullpath(s)		/**/
char *s;
{
    static char buf[MAXPATHLEN];

    strcpy(buf, pathbuf);
    strcat(buf, s);
    return buf;
}

/* do the globbing */

void scanner(q)			/**/
Complist q;
{
    Comp c;
    int closure;
    struct stat st;

    if (!q)
	return;

    if (q->closure && old_pos != pathpos &&
	stat((*pathbuf) ? pathbuf : ".", &st) != -1)
	if (st.st_ino == old_ino && st.st_dev == old_dev)
	    return;
	else {
	    old_pos = pathpos;
	    old_ino = st.st_ino;
	    old_dev = st.st_dev;
	}
    if ((closure = q->closure))	/* (foo/)# */
	if (q->closure == 2)	/* (foo/)## */
	    q->closure = 1;
	else
	    scanner(q->next);
    if ((c = q->comp)) {
	if (!(c->next || c->left) && !haswilds(c->str)) {
	    if (q->next) {
		int oppos = pathpos;

		if (errflag)
		    return;
		if (q->closure && !strcmp(c->str, "."))
		    return;
		if (!addpath(c->str))
		    return;
		if (!closure || exists(pathbuf))
		    scanner((q->closure) ? q : q->next);
		pathbuf[pathpos = oppos] = '\0';
	    } else {
		char *s;

		if (exists(s = getfullpath(c->str)))
		    insert(dupstring(s));
	    }
	} else {
	    char *fn;
	    int dirs = !!q->next;
	    struct dirent *de;
	    DIR *lock = opendir((*pathbuf) ? pathbuf : ".");

	    if (lock == NULL)
		return;
	    while ((de = readdir(lock))) {
		if (errflag)
		    break;
		fn = &de->d_name[0];
		if (fn[0] == '.'
		    && (fn[1] == '\0'
			|| (fn[1] == '.' && fn[2] == '\0')))
		    continue;
		if (!dirs && !colonmod &&
		    ((glob_pre && !strpfx(glob_pre, fn))
		     || (glob_suf && !strsfx(glob_suf, fn))))
		    continue;
		if (domatch(fn, c, gf_noglobdots)) {
		    int oppos = pathpos;

		    if (dirs) {
			if (closure) {
			    int type3;
			    struct stat buf;

			    if (lstat(getfullpath(fn), &buf) == -1) {
				if (errno != ENOENT && errno != EINTR &&
				    errno != ENOTDIR) {
				    zerr("%e: %s", fn, errno);
				    errflag = 0;
				}
				continue;
			    }
			    type3 = buf.st_mode & S_IFMT;
			    if (type3 != S_IFDIR)
				continue;
			}
			if (addpath(fn))
			    scanner((q->closure) ? q : q->next);	/* scan next level */
			pathbuf[pathpos = oppos] = '\0';
		    } else
			insert(dyncat(pathbuf, fn));
		}
	    }
	    closedir(lock);
	}
    } else
	zerr("no idea how you got this error message.", NULL, 0);
}

/* do the [..(foo)..] business */

int minimatch(pat, str)		/**/
char **pat;
char **str;
{
    char *pt = *pat + 1, *s = *str;

    for (; *pt != Outpar; s++, pt++)
	if ((*pt != Quest || !*s) && *pt != *s) {
	    *pat = getparen(*pat) - 1;
	    return 0;
	}
    *str = s - 1;
    return 1;
}

static char *pptr;
static Comp tail = 0;
static int first;

int domatch(str, c, fist)	/**/
char *str;
Comp c;
int fist;
{
    pptr = str;
    first = fist;
    return doesmatch(c);
}

#define untok(C)  (itok(C) ? ztokens[(C) - Pound] : (C))

/* See if pattern has a matching exclusion (~...) part */

int excluded(c, eptr, efst)	/**/
Comp c;
char *eptr;
int efst;
{
    char *saves = pptr;
    int savei = first, ret;

    first = efst;
    pptr = (PATHADDP(c) && pathpos) ? getfullpath(eptr) : eptr;

    ret = doesmatch(c->exclude);

    pptr = saves;
    first = savei;

    return ret;
}

/* see if current pattern matches c */

int doesmatch(c)		/**/
Comp c;
{
    char *pat = c->str;
    int done = 0;

  tailrec:
    if (ONEHASHP(c) || (done && TWOHASHP(c))) {
	char *saves = pptr;

	if (first && *pptr == '.')
	    return 0;
	if (doesmatch(c->next))
	    return 1;
	pptr = saves;
	first = 0;
    }
    done++;
    for (;;) {
	if (!pat || !*pat) {
	    char *saves;
	    int savei;

	    if (errflag)
		return 0;
	    saves = pptr;
	    savei = first;
	    if (c->left || c->right)
		if (!doesmatch(c->left) ||
		    (c->exclude && excluded(c, saves, savei)))
		    if (c->right) {
			pptr = saves;
			first = savei;
			if (!doesmatch(c->right))
			    return 0;
		    } else
			return 0;
	    if (*pptr && CLOSUREP(c)) {
		pat = c->str;
		goto tailrec;
	    }
	    if (!c->next)
		return (!LASTP(c) || !*pptr);
	    c = c->next;
	    done = 0;
	    pat = c->str;
	    goto tailrec;
	}
	if (first && *pptr == '.' && *pat != '.')
	    return 0;
	if (*pat == Star) {	/* final * is not expanded to ?#; returns success */
	    while (*pptr)
		pptr++;
	    return 1;
	}
	first = 0;
	if (*pat == Quest && *pptr) {
	    pptr++;
	    pat++;
	    continue;
	}
	if (*pat == Hat)
	    return 1 - doesmatch(c->next);
	if (*pat == Inbrack) {
	    if (!*pptr)
		break;
	    if (pat[1] == Hat || pat[1] == '^' || pat[1] == '!') {
		pat[1] = Hat;
		for (pat += 2; *pat != Outbrack && *pat; pat++)
		    if (*pat == '-' && pat[-1] != Hat && pat[1] != Outbrack) {
			if (untok(pat[-1]) <= *pptr && untok(pat[1]) >= *pptr)
			    break;
		    } else if (*pptr == untok(*pat))
			break;
		if (!*pat) {
		    zerr("something is very wrong.", NULL, 0);
		    return 0;
		}
		if (*pat != Outbrack)
		    break;
		pat++;
		pptr++;
		continue;
	    } else {
		for (pat++; *pat != Outbrack && *pat; pat++)
		    if (*pat == Inpar) {
			if (minimatch(&pat, &pptr))
			    break;
		    } else if (*pat == '-' && pat[-1] != Inbrack &&
			       pat[1] != Outbrack) {
			if (untok(pat[-1]) <= *pptr && untok(pat[1]) >= *pptr)
			    break;
		    } else if (*pptr == untok(*pat))
			break;
		if (!pat || !*pat) {
		    zerr("oh dear.  that CAN'T be right.", NULL, 0);
		    return 0;
		}
		if (*pat == Outbrack)
		    break;
		for (pptr++; *pat != Outbrack; pat++);
		pat++;
		continue;
	    }
	}
	if (*pat == Inang) {
	    int t1, t2, t3;
	    char *ptr;

	    if (*++pat == Outang) {	/* handle <> case */
		(void)zstrtol(pptr, &ptr, 10);
		if (ptr == pptr)
		    break;
		pptr = ptr;
		pat++;
	    } else {
		t1 = zstrtol(pptr, &ptr, 10);
		if (ptr == pptr)
		    break;
		pptr = ptr;
		t2 = zstrtol(pat, &ptr, 10);
		if (*ptr != '-')
		    t3 = t2, pat = ptr;
		else
		    t3 = zstrtol(ptr + 1, &pat, 10);
		if (!t3)
		    t3 = -1;
		if (*pat++ != Outang)
		    exit(21);
		if (t1 < t2 || (t3 != -1 && t1 > t3))
		    break;
	    }
	    continue;
	}
	if (*pptr == *pat) {
	    pptr++;
	    pat++;
	    continue;
	}
	break;
    }
    return 0;
}

Complist parsepat(str)		/**/
char *str;
{
    mode = 0;
    pptr = str;
    return parsecomplist();
}

Comp parsereg(str)		/**/
char *str;
{
    mode = 1;
    pptr = str;
    return parsecompsw(0);
}

Complist parsecomplist()
{				/**/
    Comp c1;
    Complist p1;

    if (pptr[0] == Star && pptr[1] == Star &&
	(pptr[2] == '/' ||
	 (pptr[2] == Star && pptr[3] == Star && pptr[4] == '/'))) {
	pptr += 3;
	if (pptr[-1] == Star)
	    pptr += 2;
	p1 = (Complist) alloc(sizeof *p1);
	if ((p1->next = parsecomplist()) == NULL) {
	    errflag = 1;
	    return NULL;
	}
	p1->comp = (Comp) alloc(sizeof *p1->comp);
	p1->comp->stat |= C_LAST;
	p1->comp->str = dupstring("*");
	*p1->comp->str = Star;
	p1->closure = 1;
	return p1;
    }
    if (*pptr == Inpar) {
	char *str;
	int pars = 1;

	for (str = pptr + 1; *str && pars; str++)
	    if (*str == Inpar)
		pars++;
	    else if (*str == Outpar)
		pars--;
	if (str[0] != Pound || str[-1] != Outpar || str[-2] != '/')
	    goto kludge;
	pptr++;
	if (!(c1 = parsecompsw(0)))
	    return NULL;
	if (pptr[0] == '/' && pptr[1] == Outpar && pptr[2] == Pound) {
	    int pdflag = 0;

	    pptr += 3;
	    if (*pptr == Pound) {
		pdflag = 1;
		pptr++;
	    }
	    p1 = (Complist) alloc(sizeof *p1);
	    p1->comp = c1;
	    p1->closure = 1 + pdflag;
	    p1->next = parsecomplist();
	    return (p1->comp) ? p1 : NULL;
	}
    } else {
      kludge:
	if (!(c1 = parsecompsw(1)))
	    return NULL;
	if (*pptr == '/' || !*pptr) {
	    int ef = *pptr == '/';

	    p1 = (Complist) alloc(sizeof *p1);
	    p1->comp = c1;
	    p1->closure = 0;
	    p1->next = ef ? (pptr++, parsecomplist()) : NULL;
	    return (ef && !p1->next) ? NULL : p1;
	}
    }
    errflag = 1;
    return NULL;
}

Comp parsecomp()
{				/**/
    Comp c = (Comp) alloc(sizeof *c), c1, c2;
    char *s = c->str = (char *)alloc(MAXPATHLEN * 2), *ls = NULL;

    c->next = tail;

    while (*pptr && (mode || *pptr != '/') && *pptr != Bar &&
	   (!isset(EXTENDEDGLOB) || *pptr != Tilde ||
	    !pptr[1] || pptr[1] == Outpar || pptr[1] == Bar) &&
	   *pptr != Outpar) {
	if (*pptr == Hat) {
	    *s++ = Hat;
	    *s++ = '\0';
	    pptr++;
	    if (!(c->next = parsecomp()))
		return NULL;
	    return c;
	}
	if (*pptr == Star && pptr[1] &&
	    (!isset(EXTENDEDGLOB) || pptr[1] != Tilde || !pptr[2] ||
	     pptr[2] == Bar ||
	     pptr[2] == Outpar) && (mode || pptr[1] != '/')) {
	    *s++ = '\0';
	    pptr++;
	    c1 = (Comp) alloc(sizeof *c1);
	    *(c1->str = dupstring("?")) = Quest;
	    c1->stat |= C_ONEHASH;
	    if (!(c2 = parsecomp()))
		return NULL;
	    c1->next = c2;
	    c->next = c1;
	    return c;
	}
	if (*pptr == Inpar) {
	    int pars = 1;
	    char *startp = pptr, *endp;
	    Comp stail = tail;
	    int dpnd = 0;

	    for (pptr = pptr + 1; *pptr && pars; pptr++)
		if (*pptr == Inpar)
		    pars++;
		else if (*pptr == Outpar)
		    pars--;
	    if (pptr[-1] != Outpar) {
		errflag = 1;
		return NULL;
	    }
	    if (*pptr == Pound) {
		dpnd = 1;
		pptr++;
		if (*pptr == Pound) {
		    pptr++;
		    dpnd = 2;
		}
	    }
	    if (!(c1 = parsecomp()))
		return NULL;
	    tail = dpnd ? NULL : c1;
	    endp = pptr;
	    pptr = startp;
	    pptr++;
	    *s++ = '\0';
	    c->next = (Comp) alloc(sizeof *c);
	    c->next->left = parsecompsw(0);
	    if (dpnd)
		c->next->stat |= (dpnd == 2) ? C_TWOHASH : C_ONEHASH;
	    c->next->next = dpnd ? c1 : (Comp) alloc(sizeof *c);
	    pptr = endp;
	    tail = stail;
	    return c;
	}
	if (*pptr == Pound) {
	    *s = '\0';
	    pptr++;
	    if (!ls)
		return NULL;
	    if (*pptr == Pound) {
		pptr++;
		c->next = c1 = (Comp) alloc(sizeof *c);
		c1->str = dupstring(ls);
	    } else
		c1 = c;
	    c1->next = c2 = (Comp) alloc(sizeof *c);
	    c2->str = dupstring(ls);
	    c2->stat |= C_ONEHASH;
	    c2->next = parsecomp();
	    if (!c2->next)
		return NULL;
	    *ls++ = '\0';
	    return c;
	}
	ls = s;
	if (*pptr == Inang) {
	    int dshct;

	    dshct = (pptr[1] == Outang);
	    *s++ = *pptr++;
	    while (*pptr && (*s++ = *pptr++) != Outang)
		if (s[-1] == '-')
		    dshct++;
		else if (!idigit(s[-1]))
		    break;
	    if (s[-1] != Outang)
		return NULL;
	} else if (*pptr == Inbrack) {
	    while (*pptr && (*s++ = *pptr++) != Outbrack);
	    if (s[-1] != Outbrack)
		return NULL;
	} else if (itok(*pptr) && *pptr != Star && *pptr != Quest)
	    *s++ = ztokens[*pptr++ - Pound];
	else
	    *s++ = *pptr++;
    }
    if (*pptr == '/' || !*pptr)
	c->stat |= C_LAST;
    *s++ = '\0';
    return c;
}

Comp parsecompsw(pathadd)	/**/
int pathadd;
{
    Comp c1, c2, c3, excl = NULL;

    c1 = parsecomp();
    if (!c1)
	return NULL;
    if (isset(EXTENDEDGLOB) && *pptr == Tilde) {
	int oldmode = mode;

	mode = 1;
	pptr++;
	excl = parsecomp();
	mode = oldmode;
	if (!excl)
	    return NULL;
    }
    if (*pptr == Bar || excl) {
	c2 = (Comp) alloc(sizeof *c2);
	if (*pptr == Bar) {
	    pptr++;
	    c3 = parsecompsw(pathadd);
	    if (!c3)
		return NULL;
	} else {
	    if (!*pptr || *pptr == '/')
		c2->stat |= C_LAST;
	    c3 = NULL;
	}
	c2->str = dupstring("");
	c2->left = c1;
	c2->right = c3;
	c2->exclude = excl;
	if (pathadd)
	    c2->stat |= C_PATHADD;
	return c2;
    }
    return c1;
}

void tokenize(s)		/**/
char *s;
{
    char *t;

    for (; *s; s++)
	if (*s == '\\')
	    chuck(s);
	else
	    for (t = ztokens; *t; t++)
		if (*t == *s) {
		    *s = (t - ztokens) + Pound;
		    break;
		}
}

/* remove unnecessary Nulargs */

void remnulargs(s)		/**/
char *s;
{
    int nl = *s;
    char *t = s;

    while (*s)
	if (INULL(*s))
	    chuck(s);
	else
	    s++;
    if (!*t && nl) {
	t[0] = Nularg;
	t[1] = '\0';
    }
}

/* qualifier functions */

int qualdev(buf, dv)		/**/
struct stat *buf;
long dv;
{
    return buf->st_dev == dv;
}

int qualnlink(buf, ct)		/**/
struct stat *buf;
long ct;
{
    return (range < 0 ? buf->st_nlink < ct :
	    range > 0 ? buf->st_nlink > ct :
	    buf->st_nlink == ct);
}

int qualuid(buf, uid)		/**/
struct stat *buf;
long uid;
{
    return buf->st_uid == uid;
}

int qualgid(buf, gid)		/**/
struct stat *buf;
long gid;
{
    return buf->st_gid == gid;
}

int qualisdev(buf, junk)	/**/
struct stat *buf;
long junk;
{
    junk = buf->st_mode & S_IFMT;
    return junk == S_IFBLK || junk == S_IFCHR;
}

int qualisblk(buf, junk)	/**/
struct stat *buf;
long junk;
{
    junk = buf->st_mode & S_IFMT;
    return junk == S_IFBLK;
}

int qualischar(buf, junk)	/**/
struct stat *buf;
long junk;
{
    junk = buf->st_mode & S_IFMT;
    return junk == S_IFCHR;
}

int qualmode(buf, mod)		/**/
struct stat *buf;
long mod;
{
    return (buf->st_mode & S_IFMT) == mod;
}

int qualflags(buf, mod)		/**/
struct stat *buf;
long mod;
{
    return buf->st_mode & mod;
}

int qualeqflags(buf, mod)	/**/
struct stat *buf;
long mod;
{
    return (buf->st_mode & 07777) == mod;
}

int qualiscom(buf, mod)		/**/
struct stat *buf;
long mod;
{
    return (buf->st_mode & (S_IFMT | S_IEXEC)) == (S_IFREG | S_IEXEC);
}

int qualsize(buf, size)		/**/
struct stat *buf;
long size;
{
    return (range < 0 ? buf->st_size < size :
	    range > 0 ? buf->st_size > size :
	    buf->st_size == size);
}

int qualtime(buf, days)		/**/
struct stat *buf;
long days;
{
    time_t now, diff;

    time(&now);
    diff = now - (amc == 0 ? buf->st_atime : amc == 1 ? buf->st_mtime :
		  buf->st_ctime);
    switch (timef) {
    case TT_DAYS:
	diff /= 86400l;
	break;
    case TT_HOURS:
	diff /= 3600l;
	break;
    case TT_MINS:
	diff /= 60l;
	break;
    case TT_WEEKS:
	diff /= 604800l;
	break;
    case TT_MONTHS:
	diff /= 2592000l;
	break;
    }

    return (range < 0 ? diff < days :
	    range > 0 ? diff > days :
	    diff == days);
}
