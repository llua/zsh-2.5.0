/*
 *
 * params.c - parameters
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
#include "version.h"
#include <pwd.h>

static Param argvparam;

struct iparam {
    struct hashnode *next;
    char *nam;			/* hash data */
    vptr value;
    int (*func1) ();		/* set func */
    int (*func2) ();		/* get func */
    int ct;			/* output base or field width */
    int flags;
    vptr data;			/* used by getfns */
    char *env;			/* location in environment, if exported */
    char *ename;		/* name of corresponding environment var */
    Param old;			/* old struct for use with local */
    int level;			/* if (old != NULL), level of localness */
};

#define IFN(X) ((int (*)())(X))

/* put predefined params in hash table */

void setupparams()
{				/**/
    static struct iparam pinit[] =
    {
#define IPDEF1(A,B,C,D) {NULL,A,NULL,IFN(C),IFN(B),10,\
		PMFLAG_i|PMFLAG_SPECIAL|D,NULL,NULL,NULL,NULL,0}
	IPDEF1("#", poundgetfn, IFN(nullsetfn), PMFLAG_r),
	IPDEF1("ARGC", poundgetfn, IFN(nullsetfn), PMFLAG_r),
	IPDEF1("ERRNO", errnogetfn, IFN(nullsetfn), PMFLAG_r),
	IPDEF1("GID", gidgetfn, IFN(nullsetfn), PMFLAG_r),
	IPDEF1("EGID", egidgetfn, IFN(nullsetfn), PMFLAG_r),
	IPDEF1("HISTSIZE", histsizegetfn, histsizesetfn, 0),
	IPDEF1("LITHISTSIZE", lithistsizegetfn, lithistsizesetfn, 0),
	IPDEF1("RANDOM", randomgetfn, randomsetfn, 0),
	IPDEF1("SECONDS", secondsgetfn, secondssetfn, 0),
	IPDEF1("UID", uidgetfn, IFN(nullsetfn), PMFLAG_r),
	IPDEF1("EUID", euidgetfn, IFN(nullsetfn), PMFLAG_r),

#define IPDEF2(A,B,C,D) {NULL,A,NULL,IFN(C),IFN(B),0,\
		PMFLAG_SPECIAL|D,NULL,NULL,NULL,NULL,0}
	IPDEF2("-", dashgetfn, IFN(nullsetfn), PMFLAG_r),
	IPDEF2("HISTCHARS", histcharsgetfn, histcharssetfn, 0),
	IPDEF2("HOME", homegetfn, homesetfn, 0),
	IPDEF2("TERM", termgetfn, termsetfn, 0),
	IPDEF2("WORDCHARS", wordcharsgetfn, wordcharssetfn, 0),
	IPDEF2("IFS", ifsgetfn, ifssetfn, 0),
	IPDEF2("_", underscoregetfn, IFN(nullsetfn), PMFLAG_r),

#define IPDEF3(A) {NULL,A,NULL,IFN(nullsetfn),IFN(strconstgetfn),0,PMFLAG_r|\
		PMFLAG_SPECIAL,NULL,NULL,NULL,NULL,0}
	IPDEF3("HOSTTYPE"),
	IPDEF3("VERSION"),

#define IPDEF4(A,B) {NULL,A,NULL,IFN(nullsetfn),IFN(intvargetfn),10,\
		PMFLAG_r|PMFLAG_i|PMFLAG_SPECIAL,(vptr)B,NULL,NULL,NULL,0}
	IPDEF4("!", &lastpid),
	IPDEF4("$", &mypid),
	IPDEF4("?", &lastval),
	IPDEF4("status", &lastval),
	IPDEF4("LINENO", &lineno),
	IPDEF4("PPID", &ppid),

#define IPDEF5(A,B) {NULL,A,NULL,IFN(intvarsetfn),IFN(intvargetfn),10,\
		PMFLAG_i|PMFLAG_SPECIAL,(vptr)B,NULL,NULL,NULL,0}
	IPDEF5("BAUD", &baud),
	IPDEF5("COLUMNS", &columns),
	IPDEF5("DIRSTACKSIZE", &dirstacksize),
	IPDEF5("KEYTIMEOUT", &keytimeout),
	IPDEF5("LINES", &lines),
	IPDEF5("LISTMAX", &listmax),
	IPDEF5("LOGCHECK", &logcheck),
	IPDEF5("MAILCHECK", &mailcheck),
	IPDEF5("OPTIND", &zoptind),
	IPDEF5("PERIOD", &period),
	IPDEF5("REPORTTIME", &reporttime),
	IPDEF5("SAVEHIST", &savehist),
	IPDEF5("SHLVL", &shlvl),
	IPDEF5("TMOUT", &tmout),

#define IPDEF6(A,B) {NULL,A,NULL,IFN(nullsetfn),IFN(strvargetfn),0,\
		PMFLAG_r|PMFLAG_SPECIAL,(vptr)B,NULL,NULL,NULL,0}
	IPDEF6("LOGNAME", &zlogname),
	IPDEF6("PWD", &pwd),
	IPDEF6("TTY", &ttystrname),
	IPDEF6("USERNAME", &username),

#define IPDEF7(A,B) {NULL,A,NULL,IFN(strvarsetfn),IFN(strvargetfn),0,\
		PMFLAG_SPECIAL,(vptr)B,NULL,NULL,NULL,0}
	IPDEF7("FCEDIT", &fceditparam),
	IPDEF7("HOST", &hostnam),
	IPDEF7("OLDPWD", &oldpwd),
	IPDEF7("OPTARG", &zoptarg),
	IPDEF7("MAIL", &mailfile),
	IPDEF7("NULLCMD", &nullcmd),
	IPDEF7("POSTEDIT", &postedit),
	IPDEF7("prompt", &prompt),
	IPDEF7("PROMPT", &prompt),
	IPDEF7("PROMPT2", &prompt2),
	IPDEF7("PROMPT3", &prompt3),
	IPDEF7("PROMPT4", &prompt4),
	IPDEF7("READNULLCMD", &readnullcmd),
	IPDEF7("RPROMPT", &rprompt),
	IPDEF7("PS1", &prompt),
	IPDEF7("PS2", &prompt2),
	IPDEF7("PS3", &prompt3),
	IPDEF7("PS4", &prompt4),
	IPDEF7("RPS1", &rprompt),
	IPDEF7("SPROMPT", &sprompt),
	IPDEF7("TIMEFMT", &timefmt),
	IPDEF7("TMPPREFIX", &tmpprefix),
	IPDEF7("WATCHFMT", &watchfmt),
	IPDEF7("0", &argzero),

#define IPDEF8(A,B,C) {NULL,A,NULL,IFN(colonarrsetfn),IFN(colonarrgetfn),0,\
		PMFLAG_SPECIAL,(vptr)C,NULL,B,NULL,0}
	IPDEF8("CDPATH", "cdpath", &cdpath),
	IPDEF8("FIGNORE", "fignore", &fignore),
	IPDEF8("FPATH", "fpath", &fpath),
	IPDEF8("MAILPATH", "mailpath", &mailpath),
	IPDEF8("MANPATH", "manpath", &manpath),
	IPDEF8("WATCH", "watch", &watch),
	IPDEF8("PSVAR", "psvar", &psvar),
	{NULL, "PATH", NULL, IFN(colonpathsetfn), IFN(colonpathgetfn), 0,
	 PMFLAG_SPECIAL, (vptr) NULL, NULL, "path", NULL, 0},

#define IPDEF9(A,B,C,D) {NULL,A,NULL,IFN(arrvarsetfn),IFN(arrvargetfn),0,\
		PMFLAG_A|PMFLAG_SPECIAL|C,(vptr)B,NULL,D,NULL,0}
	IPDEF9("cdpath", &cdpath, 0, "CDPATH"),
	IPDEF9("fignore", &fignore, 0, "FIGNORE"),
	IPDEF9("fpath", &fpath, 0, "FPATH"),
	IPDEF9("mailpath", &mailpath, 0, "MAILPATH"),
	IPDEF9("manpath", &manpath, 0, "MANPATH"),
	IPDEF9("watch", &watch, 0, "WATCH"),
	IPDEF9("psvar", &psvar, 0, "PSVAR"),
	IPDEF9("signals", &sigptr, PMFLAG_r, NULL),
	IPDEF9("argv", &pparams, 0, NULL),
	IPDEF9("*", &pparams, 0, NULL),
	IPDEF9("@", &pparams, 0, NULL),

	{NULL, "path", NULL, IFN(pathsetfn), IFN(pathgetfn), 0,
	 PMFLAG_A | PMFLAG_SPECIAL, NULL, NULL, "PATH", NULL, 0},

	{NULL,}
    };
    struct iparam *ip;

    for (ip = pinit; ip->nam; ip++)
	addhnode(ztrdup(ip->nam), ip, paramtab, (FFunc) 0);
    argvparam = (Param) gethnode("argv", paramtab);

    ((struct iparam *)gethnode("HOSTTYPE", paramtab))->data = (vptr) ztrdup(HOSTTYPE);
    ((struct iparam *)gethnode("VERSION", paramtab))->data = (vptr) ztrdup(VERSIONSTR);
}

struct param *createparam(name, flags)	/**/
char *name;
int flags;
{
    struct param *pm, *oldpm = (Param) gethnode(name, paramtab);
    int spec;

    spec = oldpm && (oldpm->flags & PMFLAG_SPECIAL);
    if ((oldpm && oldpm->level == locallevel) || spec) {
	pm = oldpm;
	pm->ct = 0;
	oldpm = pm->old;
	pm->flags = (flags & (PMFLAG_x | PMFLAG_L | PMFLAG_R | PMFLAG_Z |
			      PMFLAG_l | PMFLAG_u | PMFLAG_r | PMFLAG_t)) |
	    (pm->flags & (PMFLAG_s | PMFLAG_i | PMFLAG_A | PMFLAG_SPECIAL));
	if (pm->ename) {
	    Param altpm = (Param) gethnode(pm->ename, paramtab);

	    if (altpm)
		altpm->flags &= ~(PMFLAG_UNSET | PMFLAG_x | PMFLAG_L |
				  PMFLAG_R | PMFLAG_Z | PMFLAG_l |
				  PMFLAG_u | PMFLAG_r | PMFLAG_t);
	}
    } else {
	pm = (struct param *)zcalloc(sizeof *pm);
	if ((pm->old = oldpm))
	    remhnode(name, paramtab);	/* needed to avoid freeing oldpm */
	addhnode(ztrdup(name), pm, paramtab, freepm);
    }
    if (isset(ALLEXPORT) && !oldpm)
	pm->flags |= PMFLAG_x;
    if (!spec) {
	pm->flags = flags;
	if ((flags & PMTYPE) == PMFLAG_s) {
	    pm->sets.cfn = strsetfn;
	    pm->gets.cfn = strgetfn;
	} else if ((flags & PMTYPE) == PMFLAG_A) {
	    pm->sets.afn = arrsetfn;
	    pm->gets.afn = arrgetfn;
	} else {
	    pm->sets.ifn = intsetfn;
	    pm->gets.ifn = intgetfn;
	}
    }
    return pm;
}

int isident(s)			/**/
char *s;
{
    char *ss;
    int ne = noeval;

    if (!*s)
	return 0;

    for (ss = s; *ss; ss++)
	if (!iident(*ss))
	    break;
    if (!*ss || (*ss == '[' && ss[1] == '('))
	return 1;
    if (*ss != '[')
	return 0;
    noeval = 1;
    (void)mathevalarg(++ss, &ss);
    if (*ss == ',' || *ss == Comma)
	(void)mathevalarg(++ss, &ss);
    noeval = ne;
    if (*ss != ']' || ss[1])
	return 0;
    return 1;
}

char **garr;

long getarg(str, inv, v, a2, w)	/**/
char **str;
int *inv;
Value v;
int a2;
long *w;
{
    int num = 1, word = 0, rev = 0, ind = 0, down = 0, exp = 0, l, i;
    char *s = *str, *sep = NULL, *t, sav, *d, **ta, **p, *tt;
    long r = 0, rr, rrr;
    Comp c;

    if (*s == '(' || *s == Inpar) {
	for (s++; *s != ')' && *s != Outpar && s != *str; s++) {
	    switch (*s) {
	    case 'r':
		rev = 1;
		down = ind = 0;
		break;
	    case 'R':
		rev = down = 1;
		ind = 0;
		break;
	    case 'i':
		rev = ind = 1;
		down = 0;
		break;
	    case 'I':
		rev = ind = down = 1;
		break;
	    case 'w':
		word = 1;
		break;
	    case 'e':
		exp = 1;
		break;
	    case 'n':
		t = get_strarg(++s);
		if (!*t)
		    goto flagerr;
		sav = *t;
		*t = '\0';
		num = mathevalarg(s + 1, &d);
		if (!num)
		    num = 1;
		*t = sav;
		s = t;
		break;
	    case 's':
		t = get_strarg(++s);
		if (!*t)
		    goto flagerr;
		sav = *t;
		*t = '\0';
		sep = dupstring(s + 1);
		*t = sav;
		s = t;
		break;
	    default:
	      flagerr:
		num = 1;
		word = rev = ind = down = exp = 0;
		sep = NULL;
		s = *str - 1;
	    }
	}
	if (s != *str)
	    s++;
    }
    if (num < 0) {
	down = !down;
	num = -num;
    }
    *inv = ind;

    for (t = s, i = 0; *t && (*t != ']' || i) &&
	 (*t != Outbrack || i) && *t != ','; t++)
	if (*t == '[' || *t == Inbrack)
	    i++;
	else if (*t == ']' || *t == Outbrack)
	    i--;

    if (!*t)
	return 0;
    sav = *t;
    *t = '\0';
    s = dupstring(s);
    *t = sav;
    *str = tt = t;

    if (exp) {
	tokenize(s);
	lexsave();
	strinbeg();
	singsub(&s);
	strinend();
	lexrestore();
	untokenize(s);
    }
    if (!rev) {
	if (!(r = mathevalarg(s, &s)))
	    r = 1;
	if (word && !v->isarr) {
	    s = t = getstrvalue(v);
	    i = wordcount(s, sep, 0);
	    if (r < 0)
		r += i + 1;
	    if (r < 1)
		r = 1;
	    if (r > i)
		r = i;
	    if (!s || !*s)
		return 0;
	    rrr = r;
	    while ((d = findword(&s, sep)) && --r);
	    if (!d)
		return 0;

	    if (a2) {
		if ((d = findword(&s, sep))) {
		    r = (long)(d - t) - (sep ? strlen(sep) + (rrr == i ? 0 : 1) : 1);
		} else
		    r = -1;
	    } else {
		rr = (long)(d - t);
		if (rrr > 1)
		    rr++;
		r = rr;
	    }

	    if (!a2 && *tt != ',' && *tt != Comma)
		*w = (long)(s - t) - 1;
	}
    } else {
	if (!v->isarr && !word) {
	    l = strlen(s);
	    if (a2) {
		if (!l || *s != '*') {
		    d = (char *)ncalloc(l + 2);
		    *d = '*';
		    strcpy(d + 1, s);
		    s = d;
		}
	    } else {
		if (!l || s[l - 1] != '*') {
		    d = (char *)ncalloc(l + 2);
		    strcpy(d, s);
		    strcat(d, "*");
		    s = d;
		}
	    }
	}
	tokenize(s);

	if ((c = parsereg(s))) {
	    if (v->isarr) {
		ta = getarrvalue(v);
		if (!ta || !*ta)
		    return 0;
		if (down)
		    for (r = -1, p = ta + arrlen(ta) - 1; p >= ta; r--, p--) {
			if (domatch(*p, c, 0) && !--num)
			    return r;
		} else
		    for (r = 1, p = ta; *p; r++, p++)
			if (domatch(*p, c, 0) && !--num)
			    return r;
	    } else if (word) {
		ta = sepsplit(d = s = getstrvalue(v), sep);
		if (down) {
		    for (p = ta + (r = arrlen(ta)) - 1; p >= ta; p--, r--)
			if (domatch(*p, c, 0) && !--num)
			    break;
		    if (p < ta)
			return 0;
		} else {
		    for (r = 1, p = ta; *p; r++, p++)
			if (domatch(*p, c, 0) && !--num)
			    break;
		    if (!*p)
			return 0;
		}
		if (a2)
		    r++;
		for (i = 0, t = findword(&d, sep); t && *t;
		     i++, t = findword(&d, sep))
		    if (!--r) {
			r = (long)(t - s + (a2 ? -1 : 1));
			if (!a2 && *tt != ',' && *tt != Comma)
			    *w = r + strlen(ta[i]) - 2;
			return r;
		    }
		return a2 ? -1 : 0;
	    } else {
		d = getstrvalue(v);
		if (!d || !*d)
		    return 0;
		if (a2) {
		    if (down)
			for (r = -2, t = d + strlen(d) - 1; t >= d; r--, t--) {
			    sav = *t;
			    *t = '\0';
			    if (domatch(d, c, 0) && !--num) {
				*t = sav;
				return r;
			    }
			    *t = sav;
		    } else
			for (r = 0, t = d; *t; r++, t++) {
			    sav = *t;
			    *t = '\0';
			    if (domatch(d, c, 0) && !--num) {
				*t = sav;
				return r;
			    }
			    *t = sav;
			}
		} else {
		    if (down)
			for (r = -1, t = d + strlen(d) - 1; t >= d; r--, t--) {
			    if (domatch(t, c, 0) && !--num)
				return r;
		    } else
			for (r = 1, t = d; *t; r++, t++)
			    if (domatch(t, c, 0) && !--num)
				return r;
		}
	    }
	}
    }
    return r;
}

Value getvalue(pptr, bracks)	/**/
char **pptr;
int bracks;
{
    char *s = *pptr, *t = *pptr;
    char sav;
    Value v;
    int inv = 0;

    garr = NULL;

    if (idigit(*s))
	while (idigit(*s))
	    s++;
    else if (iident(*s))
	while (iident(*s))
	    s++;
    else if (*s == Quest)
	*s++ = '?';
    else if (*s == Pound)
	*s++ = '#';
    else if (*s == String)
	*s++ = '$';
    else if (*s == Qstring)
	*s++ = '$';
    else if (*s == Star)
	*s++ = '*';
    else if (*s == '#' || *s == '-' || *s == '?' || *s == '$' ||
	     *s == '_' || *s == '!' || *s == '@' || *s == '*')
	s++;
    else
	return NULL;
    if ((sav = *s))
	*s = '\0';
    if (idigit(*t) && *t != '0') {
	v = (Value) hcalloc(sizeof *v);
	v->pm = argvparam;
	v->inv = 0;
	v->a = v->b = atoi(t) - 1;
	if (sav)
	    *s = sav;
    } else {
	struct param *pm;
	int isvarat = !strcmp(t, "@");

	pm = (struct param *)gethnode(t, paramtab);
	if (sav)
	    *s = sav;
	*pptr = s;
	if (!pm || (pm->flags & PMFLAG_UNSET))
	    return NULL;
	v = (Value) hcalloc(sizeof *v);
	if (pmtype(pm) == PMFLAG_A)
	    v->isarr = isvarat ? -1 : 1;
	v->pm = pm;
	v->inv = 0;
	v->a = 0;
	v->b = -1;
	if (bracks && (*s == '[' || *s == Inbrack)) {
	    int a, b;
	    char *olds = s, *tbrack;

	    *s++ = '[';
	    for (tbrack = s; *tbrack && *tbrack != ']' && *tbrack != Outbrack; tbrack++)
		if (itok(*tbrack))
		    *tbrack = ztokens[*tbrack - Pound];
	    if (*tbrack == Outbrack)
		*tbrack = ']';
	    if ((s[0] == '*' || s[0] == '@') && s[1] == ']') {
		if (v->isarr)
		    v->isarr = (s[0] == '*') ? 1 : -1;
		v->a = 0;
		v->b = -1;
		s += 2;
	    } else {
		long we = 0, dummy;

		a = getarg(&s, &inv, v, 0, &we);
		if (a > 0 && !inv)
		    a--;

		if (inv) {
		    v->inv = 1;
		    if (*s == ']' || *s == Outbrack)
			s++;
		    v->isarr = 0;
		    v->a = v->b = a;
		    if (*s == ',' || *s == Comma) {
			zerr("invalid subscript", NULL, 0);
			while (*s != ']' && *s != Outbrack)
			    s++;
			*pptr = s;
			return v;
		    }
		} else {
		    if (*s == ',' || *s == Comma) {
			s++;
			b = getarg(&s, &inv, v, 1, &dummy);
			if (b > 0)
			    b--;
		    } else {
			b = we ? we : a;
		    }
		    if (*s == ']' || *s == Outbrack) {
			s++;
			if (v->isarr && a == b)
			    v->isarr = 0;
			v->a = a;
			v->b = b;
		    } else
			s = olds;
		}
	    }
	}
    }
    if (!bracks && *s)
	return NULL;
    *pptr = s;
    return v;
}

char *getstrvalue(v)		/**/
Value v;
{
    char *s, **ss;
    static char buf[20];

    if (!v)
	return "";
    if (v->inv) {
	sprintf(buf, "%d", v->a);
	return dupstring(buf);
    }
    if (pmtype(v->pm) != PMFLAG_A) {
	if ((pmtype(v->pm) == PMFLAG_i))
	    convbase(s = buf, v->pm->gets.ifn(v->pm), v->pm->ct);
	else
	    s = v->pm->gets.cfn(v->pm);
	if (v->a == 0 && v->b == -1)
	    return s;
	if (v->a < 0)
	    v->a += strlen(s);
	if (v->b < 0)
	    v->b += strlen(s);
	s = (v->a > (int)strlen(s)) ? dupstring("") : dupstring(s + v->a);
	if (v->b < v->a)
	    s[0] = '\0';
	else if (v->b - v->a < (int)strlen(s))
	    s[v->b - v->a + 1] = '\0';
	return s;
    }
    if (v->isarr)
	return spacejoin(v->pm->gets.afn(v->pm));

    ss = v->pm->gets.afn(v->pm);
    if (v->a < 0)
	v->a += arrlen(ss);
    s = (v->a >= arrlen(ss) || v->a < 0) ? "" : ss[v->a];
    return s;
}

static char *nular[] =
{"", NULL};

char **getarrvalue(v)		/**/
Value v;
{
    char **s;

    if (!v)
	return arrdup(nular);
    if (v->inv) {
	char buf[20];

	s = arrdup(nular);
	sprintf(buf, "%d", v->a);
	s[0] = dupstring(buf);
	return s;
    }
    s = v->pm->gets.afn(v->pm);
    if (v->a == 0 && v->b == -1)
	return s;
    if (v->a < 0)
	v->a += arrlen(s);
    if (v->b < 0)
	v->b += arrlen(s);
    if (v->a > arrlen(s) || v->a < 0)
	s = arrdup(nular);
    else
	s = arrdup(s) + v->a;
    if (v->b < v->a)
	s[0] = NULL;
    else if (v->b - v->a < arrlen(s))
	s[v->b - v->a + 1] = NULL;
    return s;
}

long getintvalue(v)		/**/
Value v;
{
    char **ss;

    if (!v || v->isarr)
	return 0;
    if (v->inv)
	return v->a;
    if (pmtype(v->pm) != PMFLAG_A) {
	if (pmtype(v->pm) == PMFLAG_i)
	    return v->pm->gets.ifn(v->pm);
	return atol(v->pm->gets.cfn(v->pm));
    }
    ss = v->pm->gets.afn(v->pm);
    if (v->a < 0)
	v->a += arrlen(ss);
    if (v->a < 0 || v->a >= arrlen(ss))
	return 0;
    return atol(ss[v->a]);
}

void setstrvalue(v, val)	/**/
Value v;
char *val;
{
    char buf[20];

    if (v->pm->flags & PMFLAG_r) {
	zsfree(val);
	return;
    }
    switch (pmtype(v->pm)) {
    case PMFLAG_s:
	if (v->a == 0 && v->b == -1) {
	    (v->pm->sets.cfn) (v->pm, val);
	    if (v->pm->flags & (PMFLAG_L | PMFLAG_R | PMFLAG_Z) && !v->pm->ct)
		v->pm->ct = strlen(val);
	} else {
	    char *z, *y, *x;
	    int zlen;

	    z = dupstring((v->pm->gets.cfn) (v->pm));
	    zlen = strlen(z);
	    if (v->inv)
		v->a--, v->b--;
	    if (v->a < 0) {
		v->a += zlen;
		if (v->a < 0)
		    v->a = 0;
	    }
	    if (v->a > zlen)
		v->a = zlen;
	    if (v->b < 0)
		v->b += zlen;
	    if (v->b < v->a)
		v->b = v->a;
	    if (v->b > zlen - 1)
		v->b = zlen - 1;
	    z[v->a] = '\0';
	    y = z + v->b + 1;
	    x = (char *)zalloc(strlen(z) + strlen(y) + strlen(val) + 1);
	    strcpy(x, z);
	    strcat(x, val);
	    strcat(x, y);
	    (v->pm->sets.cfn) (v->pm, x);
	    zsfree(val);
	}
	break;
    case PMFLAG_i:
	(v->pm->sets.ifn) (v->pm, matheval(val));
	if (!v->pm->ct && lastbase != 1)
	    v->pm->ct = lastbase;
	zsfree(val);
	break;
    case PMFLAG_A:
	if (v->a != v->b) {
	    char **ss = (char **)zalloc(2 * sizeof(char *));

	    ss[0] = val;
	    ss[1] = NULL;
	} else {
	    char **ss = (v->pm->gets.afn) (v->pm);
	    int ac, ad, t0;

	    if (v->inv)
		v->a--, v->b--;
	    ac = arrlen(ss);
	    if (v->a < 0) {
		v->a += ac;
		if (v->a < 0)
		    v->a = 0;
	    }
	    if (v->a >= ac) {
		char **st = ss;

		ad = v->a + 1;
		ss = (char **)zalloc((ad + 1) * sizeof *ss);
		for (t0 = 0; t0 != ac; t0++)
		    ss[t0] = ztrdup(st[t0]);
		while (ac < ad)
		    ss[ac++] = ztrdup("");
		ss[ac] = NULL;
	    }
	    zsfree(ss[v->a]);
	    ss[v->a] = val;
	    (v->pm->sets.afn) (v->pm, ss);
	}
	break;
    }
    if ((!v->pm->env && !(v->pm->flags & PMFLAG_x) &&
	 !(isset(ALLEXPORT) && !v->pm->old)) ||
	(v->pm->flags & PMFLAG_A) || v->pm->ename)
	return;
    if (pmtype(v->pm) == PMFLAG_i)
	convbase(val = buf, v->pm->gets.ifn(v->pm), v->pm->ct);
    else
	val = v->pm->gets.cfn(v->pm);
    if (v->pm->env)
	v->pm->env = replenv(v->pm->env, val);
    else {
	v->pm->flags |= PMFLAG_x;
	v->pm->env = addenv(v->pm->nam, val);
    }
}

void setintvalue(v, val)	/**/
Value v;
long val;
{
    char buf[20];

    if (v->pm->flags & PMFLAG_r)
	return;
    sprintf(buf, "%ld", val);
    if (v->pm->env) {
	v->pm->env = replenv(v->pm->env, buf);
    } else if ((v->pm->flags & PMFLAG_x) ||
	       (isset(ALLEXPORT) && !v->pm->old)) {
	v->pm->flags |= PMFLAG_x;
	v->pm->env = addenv(v->pm->nam, buf);
    }
    switch (pmtype(v->pm)) {
    case PMFLAG_s:
	(v->pm->sets.cfn) (v->pm, ztrdup(buf));
	break;
    case PMFLAG_i:
	(v->pm->sets.ifn) (v->pm, val);
	if (!v->pm->ct && lastbase != -1)
	    v->pm->ct = lastbase;
	break;
    case PMFLAG_A:
	zerr("attempt to assign integer to array", NULL, 0);
	break;
    }
}

void setintenv(s, val)		/**/
char *s;
long val;
{
    Param pm;
    char buf[20];

    if ((pm = (Param) gethnode(s, paramtab)) && pm->env) {
	sprintf(buf, "%ld", val);
	pm->env = replenv(pm->env, buf);
    }
}

void setarrvalue(v, val)	/**/
Value v;
char **val;
{
    if (v->pm->flags & PMFLAG_r) {
	freearray(val);
	return;
    }
    if (pmtype(v->pm) != PMFLAG_A) {
	freearray(val);
	zerr("attempt to assign array value to non-array", NULL, 0);
	return;
    }
    if (v->a == 0 && v->b == -1)
	(v->pm->sets.afn) (v->pm, val);
    else {
	char **old, **new, **p, **q, **r;
	int n, nn, l, ll, i;

	if (v->inv)
	    v->a--, v->b--;
	q = old = v->pm->gets.afn(v->pm);
	n = arrlen(old);
	if (v->a < 0)
	    v->a += n;
	if (v->b < 0)
	    v->b += n;
	if (v->a < 0)
	    v->a = 0;
	if (v->b < 0)
	    v->b = 0;
	if (v->a >= n)
	    v->a = n;
	if (v->b >= n)
	    v->b = n;

	if (v->a > v->b)
	    (v->pm->sets.afn) (v->pm, arrdup(nular));
	else {
	    l = v->b - v->a + 1;

	    nn = arrlen(val);

	    ll = n - l + nn;

	    p = new = (char **)zcalloc(sizeof(char *) * (ll + 1));

	    for (i = 0; i < v->a; i++)
		*p++ = ztrdup(*q++);
	    for (r = val; *r;)
		*p++ = ztrdup(*r++);
	    if (*q)
		for (q += l; *q;)
		    *p++ = ztrdup(*q++);

	    (v->pm->sets.afn) (v->pm, new);
	}
	freearray(val);
	freearray(old);
    }
}

long getiparam(s)		/**/
char *s;
{
    Value v;

    if (!(v = getvalue(&s, 0)))
	return 0;
    return getintvalue(v);
}

char *getsparam(s)		/**/
char *s;
{
    Value v;

    if (!(v = getvalue(&s, 0)))
	return NULL;
    return getstrvalue(v);
}

char **getaparam(s)		/**/
char *s;
{
    Value v;

    if (!((v = getvalue(&s, 0)) && v->isarr))
	return NULL;
    return getarrvalue(v);
}

Param setsparam(s, val)		/**/
char *s;
char *val;
{
    Value v;
    char *t = s;
    char *ss;

    if (!isident(s)) {
	zerr("not an identifier: %s", s, 0);
	zsfree(val);
	errflag = 1;
	return NULL;
    }
    if ((ss = strchr(s, '['))) {
	*ss = '\0';
	if (!(v = getvalue(&s, 1)))
	    createparam(t, PMFLAG_A);
	*ss = '[';
	v = NULL;
    } else {
	if (!(v = getvalue(&s, 1)))
	    createparam(t, PMFLAG_s);
	else if ((v->pm->flags & PMTYPE) == PMFLAG_A &&
		 !(v->pm->flags & PMFLAG_SPECIAL)) {
	    unsetparam(t);
	    createparam(t, PMFLAG_s);
	    v = NULL;
	}
    }
    if (!v)
	v = getvalue(&t, 1);
    setstrvalue(v, val);
    return v->pm;
}

Param setaparam(s, val)		/**/
char *s;
char **val;
{
    Value v;
    char *t = s;
    char *ss;

    if (!isident(s)) {
	zerr("not an identifier: %s", s, 0);
	freearray(val);
	errflag = 1;
	return NULL;
    }
    if ((ss = strchr(s, '['))) {
	*ss = '\0';
	if (!(v = getvalue(&s, 1)))
	    createparam(t, PMFLAG_A);
	*ss = '[';
	v = NULL;
    } else {
	if (!(v = getvalue(&s, 1)))
	    createparam(t, PMFLAG_A);
	else if ((v->pm->flags & PMTYPE) != PMFLAG_A &&
		 !(v->pm->flags & PMFLAG_SPECIAL)) {
	    unsetparam(t);
	    createparam(t, PMFLAG_A);
	    v = NULL;
	}
    }
    if (!v)
	v = getvalue(&t, 1);
    setarrvalue(v, val);
    return v->pm;
}

Param setiparam(s, val)		/**/
char *s;
long val;
{
    Value v;
    char *t = s;
    Param pm;

    if (!isident(s)) {
	zerr("not an identifier: %s", s, 0);
	errflag = 1;
	return NULL;
    }
    if (!(v = getvalue(&s, 0))) {
	pm = createparam(t, PMFLAG_i);
	pm->u.val = val;
	return pm;
    }
    setintvalue(v, val);
    return v->pm;
}

void unsetparam(s)		/**/
char *s;
{
    Param pm, oldpm;
    int spec;
    static int altflag = 0;

    if (!(pm = (Param) gethnode(s, paramtab)))
	return;
    if ((pm->flags & PMFLAG_r) && pm->level <= locallevel)
	return;
    spec = (pm->flags & PMFLAG_SPECIAL) && !pm->level;
    switch (pmtype(pm)) {
    case 0:
	(pm->sets.cfn) (pm, NULL);
	break;
    case PMFLAG_i:
	(pm->sets.ifn) (pm, 0);
	break;
    case PMFLAG_A:
	(pm->sets.afn) (pm, NULL);
	break;
    }
    if ((pm->flags & PMFLAG_x) && pm->env) {
	delenv(pm->env);
	zsfree(pm->env);
	pm->env = NULL;
    }
    if (pm->ename && !altflag) {
	altflag = 1;
	unsetparam(pm->ename);
	altflag = 0;
    }
    if ((locallevel && locallevel == pm->level) || spec) {
	pm->flags |= PMFLAG_UNSET;
    } else {
	oldpm = pm->old;
	freepm(remhnode(s, paramtab));
	if (oldpm) {
	    addhnode(ztrdup(s), oldpm, paramtab, freepm);
	    if (!(oldpm->flags & PMTYPE) && oldpm->sets.cfn == strsetfn)
		adduserdir(oldpm->nam, oldpm->u.str, 0, 1);
	}
    }
}

void intsetfn(pm, x)		/**/
Param pm;
long x;
{
    pm->u.val = x;
}

long intgetfn(pm)		/**/
Param pm;
{
    return pm->u.val;
}

void strsetfn(pm, x)		/**/
Param pm;
char *x;
{
    zsfree(pm->u.str);
    pm->u.str = x;
    adduserdir(pm->nam, x, 0, 2);
}

char *strgetfn(pm)		/**/
Param pm;
{
    return pm->u.str ? pm->u.str : "";
}

void nullsetfn(pm, x)		/**/
Param pm;
char *x;
{
    zsfree(x);
}

void arrsetfn(pm, x)		/**/
Param pm;
char **x;
{
    if (pm->u.arr && pm->u.arr != x)
	freearray(pm->u.arr);
    pm->u.arr = x;
}

char **arrgetfn(pm)		/**/
Param pm;
{
    char *nullarray = NULL;

    return pm->u.arr ? pm->u.arr : &nullarray;
}

void intvarsetfn(pm, x)		/**/
Param pm;
long x;
{
    *((long *)pm->data) = x;
}

long intvargetfn(pm)		/**/
Param pm;
{
    return *((long *)pm->data);
}

void strvarsetfn(pm, x)		/**/
Param pm;
char *x;
{
    char **q = ((char **)pm->data);

    zsfree(*q);
    *q = x;
}

char *strvargetfn(pm)		/**/
Param pm;
{
    char *s = *((char **)pm->data);

    if (!s)
	return "";
    return s;
}

char *strconstgetfn(pm)		/**/
Param pm;
{
    return (char *)pm->data;
}

char **colonfix(x, ename)	/**/
char *x;
char *ename;
{
    char **s, **t;

    s = colonsplit(x);
    if (ename)
	arrfixenv(ename, s);
    zsfree(x);
    for (t = s; *t; t++)
	if (!**t) {
	    free(*t);
	    *t = ztrdup(".");
	}
    return s;
}

void colonarrsetfn(pm, x)	/**/
Param pm;
char *x;
{
    char ***dptr = (char ***)pm->data;

    freearray(*dptr);
    if (pm->data == (vptr) & mailpath && x && !*x) {
	zsfree(x);
	*dptr = mkarray(NULL);
    } else
	*dptr = x ? colonfix(x, pm->ename ? pm->nam : NULL) : mkarray(NULL);
}

void colonpathsetfn(pm, x)	/**/
Param pm;
char *x;
{
    freearray(path);
    path = x ? colonfix(x, pm->nam) : mkarray(NULL);
    newcmdnamtab();
}

char *colonarrgetfn(pm)		/**/
Param pm;
{
    return join(*(char ***)pm->data, ':');
}

char *colonpathgetfn(pm)	/**/
Param pm;
{
    return join(path, ':');
}

char **arrvargetfn(pm)		/**/
Param pm;
{
    return *((char ***)pm->data);
}

void arrvarsetfn(pm, x)		/**/
Param pm;
char **x;
{
    char ***dptr = (char ***)pm->data;

    if (*dptr != x)
	freearray(*dptr);
    *dptr = x ? x : mkarray(NULL);
    if (pm->ename && x)
	arrfixenv(pm->ename, x);
}

char **pathgetfn(pm)		/**/
Param pm;
{
    return path;
}

void pathsetfn(pm, x)		/**/
Param pm;
char **x;
{
    if (path != x)
	freearray(path);
    path = x ? x : mkarray(NULL);
    newcmdnamtab();
    if (x)
	arrfixenv("PATH", x);
}

long poundgetfn(pm)		/**/
Param pm;
{
    return arrlen(pparams);
}

long randomgetfn(pm)		/**/
Param pm;
{
    return rand() & 0x7fff;
}

void randomsetfn(pm, v)		/**/
Param pm;
long v;
{
    srand((unsigned int)v);
}

long secondsgetfn(pm)		/**/
Param pm;
{
    return time(NULL) - shtimer.tv_sec;
}

void secondssetfn(pm, x)	/**/
Param pm;
long x;
{
    shtimer.tv_sec = time(NULL) - x;
    shtimer.tv_usec = 0;
}

long uidgetfn(pm)		/**/
Param pm;
{
    return getuid();
}

long euidgetfn(pm)		/**/
Param pm;
{
    return geteuid();
}

long gidgetfn(pm)		/**/
Param pm;
{
    return getgid();
}

long egidgetfn(pm)		/**/
Param pm;
{
    return getegid();
}

char *ifsgetfn(pm)		/**/
Param pm;
{
    return ifs;
}

void ifssetfn(pm, x)		/**/
Param pm;
char *x;
{
    if (x) {
	zsfree(ifs);
	ifs = x;
    }
    inittyptab();
}

void histsizesetfn(pm, v)	/**/
Param pm;
long v;
{
    if ((histsiz = v) <= 2)
	histsiz = 2;
    resizehistents();
}

long histsizegetfn(pm)		/**/
Param pm;
{
    return histsiz;
}

void lithistsizesetfn(pm, v)	/**/
Param pm;
long v;
{
    if ((lithistsiz = v) <= 2)
	lithistsiz = 2;
    resizehistents();
}

long lithistsizegetfn(pm)	/**/
Param pm;
{
    return lithistsiz;
}

long errnogetfn(pm)		/**/
Param pm;
{
    return errno;
}

char *dashgetfn(pm)		/**/
Param pm;
{
    static char buf[100];
    char *val;
    int t0;

    for (val = buf, t0 = ' '; t0 <= 'z'; t0++)
	if (isset(t0))
	    *val++ = t0;
    *val = '\0';
    return buf;
}

void histcharssetfn(pm, x)	/**/
Param pm;
char *x;
{
    if (x) {
	bangchar = x[0];
	hatchar = (bangchar) ? x[1] : '\0';
	hashchar = (hatchar) ? x[2] : '\0';
	zsfree(x);
    }
}

char *histcharsgetfn(pm)	/**/
Param pm;
{
    static char buf[4];

    buf[0] = bangchar;
    buf[1] = hatchar;
    buf[2] = hashchar;
    buf[3] = '\0';
    return buf;
}

char *homegetfn(pm)		/**/
Param pm;
{
    return home;
}

void homesetfn(pm, x)		/**/
Param pm;
char *x;
{
    zsfree(home);
    if (x && isset(CHASELINKS) && (home = xsymlink(x)))
	zsfree(x);
    else
	home = x ? x : ztrdup("");
    adduserdir("", home, 0, 1);
}

char *wordcharsgetfn(pm)	/**/
Param pm;
{
    return wordchars;
}

void wordcharssetfn(pm, x)	/**/
Param pm;
char *x;
{
    zsfree(wordchars);
    if (x)
	wordchars = x;
    else
	wordchars = ztrdup(DEFWORDCHARS);
    inittyptab();
}

char *underscoregetfn(pm)	/**/
Param pm;
{
    char *s, *t;

    if (!(s = qgetevent(curhist - 1)))
	return "";
    for (t = s + strlen(s); t > s; t--)
	if (*t == HISTSPACE)
	    break;
    if (t != s)
	t++;
    return t;
}

char *termgetfn(pm)		/**/
Param pm;
{
    return term;
}

extern hasam;

void termsetfn(pm, x)		/**/
Param pm;
char *x;
{
    zsfree(term);
    term = x ? x : ztrdup("");
    if (!interact || unset(USEZLE))
	return;
    if (!*term) {
	termok = 0;
    } else if (tgetent(termbuf, term) != 1) {
	zerr("can't find termcap info for %s", term, 0);
	errflag = 0;
	termok = 0;
    } else {
	char tbuf[1024], *pp;
	int t0;

	termok = 1;
	for (t0 = 0; t0 != TC_COUNT; t0++) {
	    pp = tbuf;
	    zsfree(tcstr[t0]);
	/* AIX tgetstr() ignores second argument */
	    if (!(pp = tgetstr(tccapnams[t0], &pp)))
		tcstr[t0] = NULL, tclen[t0] = 0;
	    else {
		tcstr[t0] = (char *)zalloc(tclen[t0] = strlen(pp) + 1);
		memcpy(tcstr[t0], pp, tclen[t0]);
	    }
	}

    /* if there's no termcap entry for cursor up, forget it.
	Use single line mode. */

	if (!tccan(TCUP)) {
	    termok = 0;
	    return;
	}
	hasam = tgetflag("am");
    /* if there's no termcap entry for cursor left, use \b. */

	if (!tccan(TCLEFT)) {
	    tcstr[TCLEFT] = ztrdup("\b");
	    tclen[TCLEFT] = 1;
	}
    /* if there's no termcap entry for clear, use ^L. */

	if (!tccan(TCCLEARSCREEN)) {
	    tcstr[TCCLEARSCREEN] = ztrdup("\14");
	    tclen[TCCLEARSCREEN] = 1;
	}
    /* if the termcap entry for down is \n, don't use it. */

	if (tccan(TCDOWN) && tcstr[TCDOWN][0] == '\n') {
	    tclen[TCDOWN] = 0;
	    zsfree(tcstr[TCDOWN]);
	    tcstr[TCDOWN] = NULL;
	}
    }
}

void setparams()
{				/**/
    char **envp, **envp2, **envp3, *str;
    char buf[50];
    struct param *pm;
    int ct;

    noerrs = 1;
    for (envp = environ, ct = 2; *envp; envp++, ct++);
    envp = environ;
    envp2 = envp3 = (char **)zalloc(sizeof(char *) * ct);

    for (; *envp; envp++)
	*envp2++ = ztrdup(*envp);
    *envp2 = NULL;
    envp = environ;
    environ = envp2 = envp3;
    for (; *envp; envp++, envp2++) {
	for (str = *envp; *str && *str != '='; str++);
	if (*str == '=') {
	    char *iname = NULL;

	    *str = '\0';
	    pm = (!idigit(**envp) && isident(*envp) && !strchr(*envp, '[')) ?
		setsparam(iname = *envp, ztrdup(str + 1)) : NULL;
	    if (pm) {
		pm->flags |= PMFLAG_x;
		pm->env = *envp2;
		if (pm->flags & PMFLAG_SPECIAL)
		    pm->env = replenv(pm->env, getsparam(iname));
	    }
	    *str = '=';
	}
    }
    pm = (struct param *)gethnode("HOME", paramtab);
    if (!(pm->flags & PMFLAG_x)) {
	pm->flags |= PMFLAG_x;
	pm->env = addenv("HOME", home);
    }
    pm = (struct param *)gethnode("PWD", paramtab);
    if (!(pm->flags & PMFLAG_x)) {
	pm->flags |= PMFLAG_x;
	pm->env = addenv("PWD", pwd);
    }
    pm = (struct param *)gethnode("LOGNAME", paramtab);
    if (!(pm->flags & PMFLAG_x)) {
	pm->flags |= PMFLAG_x;
	pm->env = addenv("LOGNAME", zlogname);
    }
    pm = (struct param *)gethnode("SHLVL", paramtab);
    if (!(pm->flags & PMFLAG_x))
	pm->flags |= PMFLAG_x;
    sprintf(buf, "%d", (int)++shlvl);
    pm->env = addenv("SHLVL", buf);
    noerrs = 0;
}

char *mkenvstr(x, y)		/**/
char *x;
char *y;
{
    char *z;
    int xl = strlen(x), yl = strlen(y);

    z = (char *)zalloc(xl + yl + 2);
    strcpy(z, x);
    z[xl] = '=';
    strcpy(z + xl + 1, y);
    z[xl + yl + 1] = '\0';
    return z;
}

void arrfixenv(s, t)		/**/
char *s;
char **t;
{
    char **ep, *u = join(t, ':');
    int sl = strlen(s);
    Param pm = (Param) gethnode(s, paramtab);

    for (ep = environ; *ep; ep++)
	if (!strncmp(*ep, s, sl) && (*ep)[sl] == '=') {
	    pm->env = replenv(*ep, u);
	    return;
	}
    if (isset(ALLEXPORT))
	pm->flags |= PMFLAG_x;
    if (pm->flags & PMFLAG_x)
	pm->env = addenv(s, u);
}

char *replenv(e, value)		/**/
char *e;
char *value;
{
    char **ep;

    for (ep = environ; *ep; ep++)
	if (*ep == e) {
	    char *s = e;

	    while (*s++ != '=');
	    *s = '\0';
	    *ep = (char *)zalloc(strlen(e) + strlen(value) + 2);
	    strcpy(*ep, e);
	    strcat(*ep, value);
	    zsfree(e);
	    return *ep;
	}
    return NULL;
}

char *addenv(name, value)	/**/
char *name;
char *value;
{
    char **ep, **ep2, **ep3;
    int envct;

    for (ep = environ; *ep; ep++) {
	char *s = *ep, *t = name;

	while (*s && *s == *t)
	    s++, t++;
	if (*s == '=' && !*t) {
	    zsfree(*ep);
	    return *ep = mkenvstr(name, value);
	}
    }
    envct = arrlen(environ);
    ep = ep2 = (char **)zalloc((sizeof(char *)) * (envct + 3));

    for (ep3 = environ; (*ep2 = *ep3); ep3++, ep2++);
    *ep2 = mkenvstr(name, value);
    ep2[1] = NULL;
    free(environ);
    environ = ep;
    return *ep2;
}

void delenv(x)			/**/
char *x;
{
    char **ep;

    ep = environ;
    for (; *ep; ep++)
	if (*ep == x)
	    break;
    if (*ep)
	for (; (ep[0] = ep[1]); ep++);
}

void convbase(s, v, base)	/**/
char *s;
long v;
int base;
{
    int digs = 0;
    long x;

    if (base <= 1)
	base = 10;
    x = v;
    if (x < 0) {
	x = -x;
	digs++;
    }
    for (; x; digs++)
	x /= base;
    if (!digs)
	digs = 1;
    s[digs--] = '\0';
    x = (v < 0) ? -v : v;
    while (digs >= 0) {
	int dig = x % base;

	s[digs--] = (dig < 10) ? '0' + dig : dig - 10 + 'A';
	x /= base;
    }
    if (v < 0)
	s[0] = '-';
}
