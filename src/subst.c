/*
 *
 * subst.c - various substitutions
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

/* do substitutions before fork */

void prefork(list, flags)	/**/
Lklist list;
int flags;
{
    Lknode node = firstnode(list);
    int qt;

 /* bits 0 and 1 of flags are flags to filesub (treat as assignment)
     * bit 2 is a flag to paramsubst (single word sub)
     */
    while (node) {
	char *str, *str3;
	int keep = 1;

	str = str3 = (char *)getdata(node);
	if ((*str == Inang || *str == Outang || *str == Equals) &&
	    str[1] == Inpar) {
	    if (*str == Inang)
		setdata(node, (vptr) getoutproc(str + 2));	/* <(...) */
	    else if (*str == Equals)
		setdata(node, (vptr) getoutputfile(str + 2));	/* =(...) */
	    else
		setdata(node, (vptr) getinproc(str + 2));	/* >(...) */
	    if (!getdata(node)) {
		zerr("parse error in process substitution", NULL, 0);
		return;
	    }
	} else
	    while (*str) {
		if ((qt = *str == Qstring) || *str == String)
		    if (str[1] != Inpar)
			if (str[1] == Inbrack) {
			    arithsubst((vptr *) & str, &str3);	/* $[...] */
			    setdata(node, (vptr) str3);
			    str = str3;
			    continue;
			} else {
			    if (!paramsubst(list, node, str, str3,
					    qt | (flags & 4), !(flags & 010)))
				keep = 0;
			    if (errflag)
				return;
			    if (!keep)
				break;
			    str3 = str = (char *)getdata(node);
			    continue;
			}
		str++;
		if (errflag)
		    return;
	    }
	if (keep) {
	    if (*(char *)getdata(node))
		remnulargs(getdata(node));
	    if (unset(IGNOREBRACES))
		while (hasbraces(getdata(node)))
		    xpandbraces(list, &node);
	    filesub((char **)getaddrdata(node), flags & 3);
	    if (errflag)
		return;
	    incnode(node);
	} else {
	    Lknode zapnode;

	    zapnode = node;
	    incnode(node);
	    uremnode(list, zapnode);
	}
    }
}

void postfork(list, flags)	/**/
Lklist list;
int flags;
{
    Lknode node = firstnode(list);
    int glb = 1;

 /* bit 0 of flags is to do glob, bit 1 is flag for single substitution */
    badcshglob = 0;
    if (isset(NOGLOBOPT) || !(flags & 1))
	glb = 0;
    while (node) {
	char *str3, *str;

	str = str3 = (char *)getdata(node);
	while (*str) {
	    if (((*str == String || *str == Qstring) && str[1] == Inpar) ||
		*str == Tick || *str == Qtick) {
		Lknode n = prevnode(node);

	    /* `...`,$(...) */
		commsubst(list, node, str, str3,
			  (*str == Qstring || *str == Qtick || flags > 1));
		if (errflag)
		    return;
		if (!(node = nextnode(n)))
		    return;
		str = str3 = (char *)getdata(node);
		continue;
	    }
	    str++;
	}
	if (glb) {
	    if (haswilds(getdata(node)))
		glob(list, &node);
	    if (errflag)
		return;
	}
	incnode(node);
    }
    if (badcshglob == 1)
	zerr("no match", NULL, 0);
}

/* perform substitution on a single word */

void singsub(s)			/**/
char **s;
{
    Lklist foo;

    foo = newlist();
    addnode(foo, *s);
    prefork(foo, 014);
    if (errflag)
	return;
    postfork(foo, 010);
    if (errflag)
	return;
    *s = (char *)ugetnode(foo);
    remnulargs(*s);
    if (firstnode(foo))
	zerr("ambiguous: %s", *s, 0);
}

/* ~, = subs: assign = 2 => typeset; assign = 1 => something that looks
	like an assignment but may not be; assign = 3 => normal assignment */

void filesub(namptr, assign)	/**/
char **namptr;
int assign;
{
    char *sub = (char *)NULL, *str, *ptr;
    int len;

    filesubstr(namptr, assign);

    if (!assign)
	return;

    if (assign < 3)
	if ((*namptr)[1] && (sub = strchr(*namptr + 1, Equals))) {
	    if (assign == 1)
		for (ptr = *namptr; ptr != sub; ptr++)
		    if (!iident(*ptr) && !INULL(*ptr))
			return;
	    *sub = Equals;
	    str = sub + 1;
	    if ((sub[1] == Tilde || sub[1] == Equals) && filesubstr(&str, assign)) {
		sub[1] = '\0';
		*namptr = dyncat(*namptr, str);
	    }
	} else
	    return;

    ptr = *namptr;
    while ((sub = strchr(ptr, ':'))) {
	str = sub + 1;
	len = sub - *namptr;
	if ((sub[1] == Tilde || sub[1] == Equals) && filesubstr(&str, assign)) {
	    sub[1] = '\0';
	    *namptr = dyncat(*namptr, str);
	}
	ptr = *namptr + len + 1;
    }
}

int filesubstr(namptr, assign)	/**/
char **namptr;
int assign;
{
    char *str = *namptr, *cnam;

    if (*str == Tilde && str[1] != '=' && str[1] != Equals) {
	if (str[1] == '+' && (str[2] == '/' || str[2] == Inpar ||
			      (assign && str[2] == ':') || !str[2])) {
	    *namptr = dyncat(pwd, str + 2);	/* ~+ */
	    return 1;
	} else if (str[1] == '-' && (str[2] == '/' || str[2] == Inpar ||
				     (assign && str[2] == ':') || !str[2])) {
	    *namptr = dyncat((cnam = oldpwd) ? cnam : pwd, str + 2);	/* ~- */
	    return 1;
	} else if (ialpha(str[1])) {	/* ~foo */
	    char *ptr, *hom;

	    for (ptr = ++str; *ptr && iuser(*ptr); ptr++);
	    if (*ptr && *ptr != '/' && *ptr != Inpar &&
		(!assign || *ptr != ':'))
		return 0;
	    if (!(hom = getnamedir(str, ptr - str))) {
		if (!isset(NONOMATCH))
		    zerr("user not found: %l", str, ptr - str);
		return 0;
	    }
	    *namptr = dyncat(hom, ptr);
	    return 1;
	} else if (str[1] == '/' || str[1] == Inpar ||
		   (assign && str[1] == ':')) {	/* ~/foo */
	    *namptr = dyncat(home, str + 1);
	    return 1;
	} else if (!str[1]) {	/* ~ by itself */
	    *namptr = dupstring(home);
	    return 1;
	}
    } else if (*str == Equals && unset(NOEQUALS) && str[1]) {
	char *ptr, *ds;
	int val;

	if (str[1] == '-') {	/* =- */
	    val = -1;
	    ptr = str + 2;
	} else if (idigit(str[1]))
	    val = zstrtol(str + 1, &ptr, 10);	/* =# */
	else
	/* =foo */
	{
	    char sav, *pp;

	    for (pp = str + 1; *pp && *pp != Inpar && (!assign || *pp != ':');
		 pp++);
	    sav = *pp;
	    *pp = '\0';
	    if (!(cnam = findcmd(str + 1))) {
		if (!isset(NONOMATCH))
		    zerr("%s not found", str + 1, 0);
		return 0;
	    }
	    *namptr = dupstring(cnam);
	    zsfree(cnam);
	    if (sav) {
		*pp = sav;
		*namptr = dyncat(*namptr, pp);
	    }
	    return 1;
	}
	ds = dstackent(val);
	if (!ds)
	    return 1;
	*namptr = dyncat(ds, ptr);
	return 1;
    }
    return 0;
}

/* get a named directory */

char *getnamedir(user, len)	/**/
char *user;
int len;
{
    char sav, *str, *ret_val = NULL;
    struct passwd *pw;
    int t0;
    struct param *pm;

    if (len == 0)
	return dupstring(home);
    sav = user[len];
    user[len] = '\0';
    if ((t0 = findname(user)) != -1)
	ret_val = dupstring(namdirs[t0].dir);
    else if ((pm = (struct param *)gethnode(user, paramtab)) &&
	     !(pm->flags & (PMFLAG_i | PMFLAG_A)) &&
	     (str = getsparam(user)) && *str == '/') {
	adduserdir(user, str, 0, 1);
	ret_val = str;
    } else if ((pw = getpwnam(user))) {
	str = xsymlink(pw->pw_dir);
	adduserdir(user, str, 1, 1);
	ret_val = dupstring(str);
	zsfree(str);
    }
    user[len] = sav;
    return ret_val;
}

/* `...`, $(...) */

void commsubst(l, n, str3, str, qt)	/**/
Lklist l;
Lknode n;
char *str3;
char *str;
int qt;
{
    char *str2;
    Lknode where = prevnode(n);
    Lklist pl;

    if (*str3 == Tick || *str3 == Qtick) {
	*str3 = '\0';
	for (str2 = ++str3; *str3 && *str3 != Tick && *str3 != Qtick; str3++);
	*str3++ = '\0';
    } else {
	*str3++ = '\0';
	for (str2 = ++str3; *str3 && *str3 != Outpar; str3++);
	*str3++ = '\0';
    }
    uremnode(l, n);
    if (!(pl = getoutput(str2, qt))) {
	if (!errflag)
	    zerr("parse error in command substitution", NULL, 0);
	return;
    }
    if (full(pl)) {
	setdata(firstnode(pl), (vptr) dyncat(str, peekfirst(pl)));
	setdata(lastnode(pl), (vptr) dyncat(getdata(lastnode(pl)), str3));
	inslist(pl, where, l);
    } else
	insnode(l, where, dyncat(str, str3));
}

void strcatsub(dest, src, globsubst)	/**/
char *dest;
char *src;
int globsubst;
{
    strcat(dest, src);
    if (globsubst)
	tokenize(dest);
}

int strpcmp(a, b)		/**/
const void *a;
const void *b;
{
    return strcmp(*(char **)a, *(char **)b);
}

int invstrpcmp(a, b)		/**/
const void *a;
const void *b;
{
    return -strcmp(*(char **)a, *(char **)b);
}

int cstrpcmp(a, b)		/**/
const void *a;
const void *b;
{
    char *c = *(char **)a, *d = *(char **)b;

    for (; *c && tulower(*c) == tulower(*d); c++, d++);

    return (int)(unsigned char)tulower(*c) - (int)(unsigned char)tulower(*d);
}

int invcstrpcmp(a, b)		/**/
const void *a;
const void *b;
{
    char *c = *(char **)a, *d = *(char **)b;

    for (; *c && tulower(*c) == tulower(*d); c++, d++);

    return (int)(unsigned char)tulower(*d) - (int)(unsigned char)tulower(*c);
}

char *dopadding(str, prenum, postnum, preone, postone, premul, postmul)	/**/
char *str;
int prenum;
int postnum;
char *preone;
char *postone;
char *premul;
char *postmul;
{
    char def[2], *ret, *t, *r;
    int ls, ls2, lpreone, lpostone, lpremul, lpostmul, lr, f, m, c, cc;

    def[0] = ifs[0];
    def[1] = '\0';
    if (preone && !*preone)
	preone = def;
    if (postone && !*postone)
	postone = def;
    if (!premul || !*premul)
	premul = def;
    if (!postmul || !*postmul)
	postmul = def;

    ls = strlen(str);
    lpreone = preone ? strlen(preone) : 0;
    lpostone = postone ? strlen(postone) : 0;
    lpremul = strlen(premul);
    lpostmul = strlen(postmul);

    lr = prenum + postnum;

    if (lr == ls)
	return str;

    r = ret = (char *)halloc(lr + 1);

    if (prenum) {
	if (postnum) {
	    ls2 = ls / 2;

	    f = prenum - ls2;
	    if (f <= 0)
		for (str -= f, c = prenum; c--; *r++ = *str++);
	    else {
		if (f <= lpreone)
		    for (c = f, t = preone + lpreone - f; c--; *r++ = *t++);
		else {
		    f -= lpreone;
		    if ((m = f % lpremul))
			for (c = m, t = premul + lpremul - m; c--; *r++ = *t++);
		    for (cc = f / lpremul; cc--;)
			for (c = lpremul, t = premul; c--; *r++ = *t++);
		    for (c = lpreone; c--; *r++ = *preone++);
		}
		for (c = ls2; c--; *r++ = *str++);
	    }
	    ls2 = ls - ls2;
	    f = postnum - ls2;
	    if (f <= 0)
		for (c = postnum; c--; *r++ = *str++);
	    else {
		for (c = ls2; c--; *r++ = *str++);
		if (f <= lpostone)
		    for (c = f; c--; *r++ = *postone++);
		else {
		    f -= lpostone;
		    for (c = lpostone; c--; *r++ = *postone++);
		    for (cc = f / lpostmul; cc--;)
			for (c = lpostmul, t = postmul; c--; *r++ = *t++);
		    if ((m = f % lpostmul))
			for (; m--; *r++ = *postmul++);
		}
	    }
	} else {
	    f = prenum - ls;
	    if (f <= 0)
		for (c = prenum, str -= f; c--; *r++ = *str++);
	    else {
		if (f <= lpreone)
		    for (c = f, t = preone + lpreone - f; c--; *r++ = *t++);
		else {
		    f -= lpreone;
		    if ((m = f % lpremul))
			for (c = m, t = premul + lpremul - m; c--; *r++ = *t++);
		    for (cc = f / lpremul; cc--;)
			for (c = lpremul, t = premul; c--; *r++ = *t++);
		    for (c = lpreone; c--; *r++ = *preone++);
		}
		for (c = ls; c--; *r++ = *str++);
	    }
	}
    } else if (postnum) {
	f = postnum - ls;
	if (f <= 0)
	    for (c = postnum; c--; *r++ = *str++);
	else {
	    for (c = ls; c--; *r++ = *str++);
	    if (f <= lpostone)
		for (c = f; c--; *r++ = *postone++);
	    else {
		f -= lpostone;
		for (c = lpostone; c--; *r++ = *postone++);
		for (cc = f / lpostmul; cc--;)
		    for (c = lpostmul, t = postmul; c--; *r++ = *t++);
		if ((m = f % lpostmul))
		    for (; m--; *r++ = *postmul++);
	    }
	}
    }
    *r = '\0';

    return ret;
}

char *get_strarg(s)		/**/
char *s;
{
    char t = *s++;

    if (!t)
	return s - 1;

    switch (t) {
    case '(':
	t = ')';
	break;
    case '[':
	t = ']';
	break;
    case '{':
	t = '}';
	break;
    case '<':
	t = '>';
	break;
    case Inpar:
	t = Outpar;
	break;
    case Inang:
	t = Outang;
	break;
    case Inbrace:
	t = Outbrace;
	break;
    case Inbrack:
	t = Outbrack;
	break;
    }

    while (*s && *s != t)
	s++;

    return s;
}

/* parameter substitution */

#define	isstring(c)	(c == '$' || c == String || c == Qstring)
#define	isbrace(c)	(c == '{' || c == Inbrace)

int paramsubst(l, n, aptr, bptr, qt, sp)	/**/
Lklist l;
Lknode n;
char *aptr;
char *bptr;
int qt;				/* if bit 0 set, real quote, else single word substitution */
int sp;
{
    char *s = aptr, *u, *idbeg, *idend, *ostr = bptr;
    int brs;			/* != 0 means ${...}, otherwise $... */
    int colf;			/* != 0 means we found a colon after the name */
    int doub = 0;		/* != 0 means we have %%, not %, or ##, not # */
    int isarr = 0;
    int plan9 = isset(RCEXPANDPARAM);
    int globsubst = isset(GLOBSUBST);
    int getlen = 0;
    int whichlen = 0;
    int chkset = 0;
    int vunset = 0;
    int spbreak = isset(SHWORDSPLIT) && sp && !qt;
    char *val = NULL, **aval = NULL;
    int fwidth = 0;
    Value v;
    int flags = 0;
    int flnum = 0;
    int substr = 0;
    int sortit = 0, casind = 0;
    int casmod = 0;
    char *sep = NULL, *spsep = NULL;
    char *premul = NULL, *postmul = NULL, *preone = NULL, *postone = NULL;
    long prenum = 0, postnum = 0;
    int copied = 0;

    *s++ = '\0';
    if (!ialnum(*s) && *s != '#' && *s != Pound && *s != '-' &&
	*s != '!' && *s != '$' && *s != String && *s != Qstring &&
	*s != '?' && *s != Quest && *s != '_' &&
	*s != '*' && *s != Star && *s != '@' && *s != '{' &&
	*s != Inbrace && *s != '=' && *s != Equals && *s != Hat &&
	*s != '^' &&
	*s != '+') {
	s[-1] = '$';
	return 1;
    }
    if ((brs = (*s == '{' || *s == Inbrace))) {
	s++;

	if (*s == '(' || *s == Inpar) {
	    char *t, sav, *d;
	    int tt = 0;
	    long num;

	    for (s++; *s != ')' && *s != Outpar; s++, tt = 0) {
		switch (*s) {
		case ')':
		case Outpar:
		    break;
		case 'M':
		    flags |= 8;
		    break;
		case 'R':
		    flags |= 16;
		    break;
		case 'B':
		    flags |= 32;
		    break;
		case 'E':
		    flags |= 64;
		    break;
		case 'N':
		    flags |= 128;
		    break;
		case 'S':
		    substr = 1;
		    break;
		case 'I':
		    flnum = 0;
		    t = get_strarg(++s);
		    if (*t) {
			sav = *t;
			*t = '\0';
			d = dupstring(s + 1);
			untokenize(d);
			if ((flnum = mathevalarg(s + 1, &d)) < 0)
			    flnum = -flnum;
			*t = sav;
			s = t;
		    } else
			goto flagerr;
		    break;

		case 'L':
		    casmod = 2;
		    break;
		case 'U':
		    casmod = 1;
		    break;
		case 'C':
		    casmod = 3;
		    break;

		case 'o':
		    sortit = 1;
		    break;
		case 'O':
		    sortit = 2;
		    break;
		case 'i':
		    casind = 1;
		    break;

		case 'c':
		    whichlen = 1;
		    break;
		case 'w':
		    whichlen = 2;
		    break;

		case 's':
		    tt = 1;
		/* fall through */
		case 'j':
		    t = get_strarg(++s);
		    if (*t) {
			sav = *t;
			*t = '\0';
			if (tt)
			    spsep = dupstring(s + 1);
			else
			    sep = dupstring(s + 1);
			*t = sav;
			s = t;
		    } else
			goto flagerr;
		    break;

		case 'l':
		    tt = 1;
		/* fall through */
		case 'r':
		    t = get_strarg(++s);
		    if (!*t)
			goto flagerr;
		    sav = *t;
		    *t = '\0';
		    d = dupstring(s + 1);
		    untokenize(d);
		    if ((num = mathevalarg(d, &d)) < 0)
			num = -num;
		    if (tt)
			prenum = num;
		    else
			postnum = num;
		    *t = sav;
		    sav = *s;
		    s = t + 1;
		    if (*s != sav) {
			s--;
			break;
		    }
		    t = get_strarg(s);
		    if (!*t)
			goto flagerr;
		    sav = *t;
		    *t = '\0';
		    if (tt)
			premul = dupstring(s + 1);
		    else
			postmul = dupstring(s + 1);
		    *t = sav;
		    sav = *s;
		    s = t + 1;
		    if (*s != sav) {
			s--;
			break;
		    }
		    t = get_strarg(s);
		    if (!*t)
			goto flagerr;
		    sav = *t;
		    *t = '\0';
		    if (tt)
			preone = dupstring(s + 1);
		    else
			postone = dupstring(s + 1);
		    *t = sav;
		    s = t;
		    break;

		default:
		  flagerr:
		    zerr("error in flags", NULL, 0);
		    return 1;
		}
	    }
	    s++;
	}
    }
    if (sortit && casind)
	sortit |= (casind << 1);

    if (!premul)
	premul = " ";
    if (!postmul)
	postmul = " ";

    for (;;) {
	if (*s == '^' || *s == Hat)
	    plan9 ^= 1, s++;
	else if (*s == '=' || *s == Equals)
	    spbreak ^= 1, s++;
	else if ((*s == '#' || *s == Pound) && (iident(s[1])
						|| s[1] == '*' || s[1] == Star || s[1] == '@'
						|| (isstring(s[1]) && isbrace(s[2]) && iident(s[3]))))
	    getlen = 1 + whichlen, s++;
	else if (*s == '~' || *s == Tilde)
	    globsubst ^= 1, s++;
	else if (*s == '+' && iident(s[1]))
	    chkset = 1, s++;
	else
	    break;
    }
    globsubst = globsubst && !(qt & 1);

    idbeg = s;
    if (isstring(*s) && isbrace(s[1])) {
	int bct, sav;

	val = s;
	for (bct = 1, s += 2; *s && bct; ++s)
	    if (*s == Inbrace || *s == '{')
		++bct;
	    else if (*s == Outbrace || *s == '}')
		--bct;
	sav = *s;
	*s = 0;
	singsub(&val);
	*s = sav;
	isarr = 0;
	v = (Value) NULL;
    } else if (!(v = getvalue(&s, 1))) {
	vunset = 1;
    } else if ((isarr = v->isarr)) {
	aval = getarrvalue(v);
	if (qt && ((qt & 1) || !getlen) && isarr > 0) {
	    val = sepjoin(aval, sep);
	    isarr = 0;
	}
    } else {
	if (v->pm->flags & PMFLAG_A) {
	    int tmplen = arrlen(v->pm->gets.afn(v->pm));

	    if (v->a < 0)
		v->a += tmplen + v->inv;
	    if (!v->inv && (v->a >= tmplen || v->a < 0))
		vunset = 1;
	}
	if (!vunset) {
	    val = getstrvalue(v);
	    fwidth = v->pm->ct ? v->pm->ct : strlen(val);
	    switch (v->pm->flags & (PMFLAG_L | PMFLAG_R | PMFLAG_Z)) {
		char *t;
		int t0;

	    case PMFLAG_L:
	    case PMFLAG_L | PMFLAG_Z:
		t = val;
		if (v->pm->flags & PMFLAG_Z)
		    while (*t == '0')
			t++;
		else
		    while (isep(*t))
			t++;
		val = (char *)ncalloc(fwidth + 1);
		val[fwidth] = '\0';
		if ((t0 = strlen(t)) > fwidth)
		    t0 = fwidth;
		memset(val, ' ', fwidth);
		strncpy(val, t, t0);
		break;
	    case PMFLAG_R:
	    case PMFLAG_Z:
	    case PMFLAG_Z | PMFLAG_R:
		if (strlen(val) < fwidth) {
		    t = (char *)ncalloc(fwidth + 1);
		    memset(t, (v->pm->flags & PMFLAG_R) ? ' ' : '0', fwidth);
		    if ((t0 = strlen(val)) > fwidth)
			t0 = fwidth;
		    strcpy(t + (fwidth - t0), val);
		    val = t;
		} else {
		    t = (char *)ncalloc(fwidth + 1);
		    t[fwidth] = '\0';
		    strncpy(t, val + strlen(val) - fwidth, fwidth);
		    val = t;
		}
		break;
	    }
	    switch (v->pm->flags & (PMFLAG_l | PMFLAG_u)) {
		char *t;

	    case PMFLAG_l:
		t = val;
		for (; *t; t++)
		    *t = tulower(*t);
		break;
	    case PMFLAG_u:
		t = val;
		for (; *t; t++)
		    *t = tuupper(*t);
		break;
	    }
	}
    }
    idend = s;
    if ((colf = *s == ':'))
	s++;

/* check for ${..?...} or ${..=..} or one of those.  Only works
		if the name is in braces. */

    if (brs && (*s == '-' || *s == '=' || *s == Equals || *s == '?' ||
		*s == '+' || *s == '#' || *s == '%' || *s == Quest || *s == Pound)) {

	if (!flnum)
	    flnum++;
	if (*s == '%')
	    flags |= 1;

	if ((*s == '%' || *s == '#' || *s == Pound) && *s == s[1]) {
	    s++;
	    doub = 1;
	}
	u = ++s;

	flags |= (doub | (substr << 1)) << 1;
	if (!(flags & 0xf8))
	    flags |= 16;

	if (brs) {
	    int bct = 1;

	    for (;;) {
		if (*s == '{' || *s == Inbrace)
		    bct++;
		else if (*s == '}' || *s == Outbrace)
		    bct--;
		if (!bct || !*s)
		    break;
		s++;
	    }
	} else {
	    while (*s++);
	    s--;
	}
	if (*s)
	    *s++ = '\0';
	if (colf && !vunset)
	    vunset = (isarr) ? !*aval : !*val;

	switch ((int)(unsigned char)u[-1]) {
	case '-':
	    if (vunset)
		val = dupstring(u), isarr = 0;
	    break;
	case '=':
	case (int)STOUC(Equals):
	    if (vunset) {
		char sav = *idend;

		*idend = '\0';
		val = dupstring(u);
		singsub(&val);
		setsparam(idbeg, ztrdup(val));
		*idend = sav;
		isarr = 0;
	    }
	    break;
	case '?':
	case (int)STOUC(Quest):
	    if (vunset) {
		char *msg;

		*idend = '\0';
		msg = tricat(idbeg, ": ", *u ? u : "parameter not set");
		zerr("%s", msg, 0);
		zsfree(msg);
		if (!interact)
		    exit(1);
		return 1;
	    }
	    break;
	case '+':
	    if (vunset)
		val = dupstring("");
	    else
		val = dupstring(u);
	    isarr = 0;
	    break;
	case '%':
	case '#':
	case (int)STOUC(Pound):
	    if (qt & 1)
		tokenize(u);

	    if (!vunset && v && v->isarr) {
		char **ap = aval;
		char **pp = aval = (char **)ncalloc(sizeof(char *) * (arrlen(aval) + 1));

		singsub(&u);
		while ((*pp = *ap++)) {
		    getmatch(pp, u, flags, flnum);
		    pp++;
		}
		if (!isarr)
		    val = sepjoin(aval, sep);
	    } else {
		if (vunset)
		    val = dupstring("");
		singsub(&u);
		getmatch(&val, u, flags, flnum);
	    }
	    break;
	}
    } else {			/* no ${...=...} or anything, but possible modifiers. */
	if (chkset) {
	    val = dupstring(vunset ? "0" : "1");
	    isarr = 0;
	} else if (vunset) {
	    if (isset(NOUNSET)) {
		*idend = '\0';
		zerr("%s: parameter not set", idbeg, 0);
		return 1;
	    }
	    val = dupstring("");
	}
	if (colf) {
	    s--;
	    if (!isarr)
		modify(&val, &s);
	    else {
		char *ss = s;
		char **ap = aval;
		char **pp = aval = (char **)ncalloc(sizeof(char *) * (arrlen(aval) + 1));

		while ((*pp = *ap++)) {
		    ss = s;
		    modify(pp++, &ss);
		}
		s = ss;
	    }
	}
	if (brs) {
	    if (*s != '}' && *s != Outbrace) {
		zerr("closing brace expected", NULL, 0);
		return 1;
	    }
	    s++;
	}
    }
    if (errflag)
	return 1;
    if (getlen) {
	long len = 0;
	char buf[14];

	if (isarr) {
	    char **ctr;
	    int sl = sep ? strlen(sep) : 1;

	    if (getlen == 1)
		for (ctr = aval; *ctr; ctr++, len++);
	    else if (getlen == 2)
		for (len = -sl, ctr = aval; *ctr; len += sl + strlen(*ctr), ctr++);
	    else
		for (ctr = aval;
		     *ctr;
		     len += wordcount(*ctr, sep, getlen > 3), ctr++);
	} else {
	    if (getlen < 3)
		len = strlen(val);
	    else
		len = wordcount(val, sep, getlen > 3);
	}

	sprintf(buf, "%ld", len);
	val = dupstring(buf);
	isarr = 0;
    }
    if (isarr > 0 && !plan9 && (!aval || !aval[0])) {
	val = dupstring("");
	isarr = 0;
    } else if (isarr && aval && aval[0] && !aval[1]) {
	val = aval[0];
	isarr = 0;
    }
    if (!qt && (spbreak || spsep || sep)) {
	if (isarr)
	    val = sepjoin(aval, sep);
	if (spbreak || spsep) {
	    isarr = 1;
	    aval = sepsplit(val, spsep);
	    if (!aval || !aval[0]) {
		val = dupstring("");
		isarr = 0;
	    } else if (!aval[1]) {
		val = aval[0];
		isarr = 0;
	    }
	} else
	    isarr = 0;
    }
    if (casmod) {
	if (isarr) {
	    char **ap;

	    ap = aval = arrdup(aval);
	    copied = 1;

	    if (casmod == 1)
		for (; *ap; ap++)
		    makeuppercase(ap);
	    else if (casmod == 2)
		for (; *ap; ap++)
		    makelowercase(ap);
	    else
		for (; *ap; ap++)
		    makecapitals(ap);

	} else {
	    val = dupstring(val);
	    copied = 1;
	    if (casmod == 1)
		makeuppercase(&val);
	    else if (casmod == 2)
		makelowercase(&val);
	    else
		makecapitals(&val);
	}
    }
    if (isarr) {
	char *x;
	char *y;
	int xlen;
	int i;

	if (!aval[0]) {
	    if (plan9)
		return 0;
	    y = (char *)ncalloc((aptr - bptr) + strlen(s) + 1);
	    strcpy(y, ostr);
	    strcat(y, s);
	    remnulargs(y);
	    if (INULL(*y))
		return 0;
	    else {
		setdata(n, (vptr) y);
		return 1;
	    }
	}
	if (sortit && !copied)
	    aval = arrdup(aval);
	if (sortit == 1)
	    qsort(aval, arrlen(aval), sizeof(char *), (int (*)())strpcmp);

	else if (sortit == 2)
	    qsort(aval, arrlen(aval), sizeof(char *), (int (*)())invstrpcmp);

	else if (sortit == 3)
	    qsort(aval, arrlen(aval), sizeof(char *), (int (*)())cstrpcmp);

	else if (sortit)
	    qsort(aval, arrlen(aval), sizeof(char *), (int (*)())invcstrpcmp);

	if (plan9) {
	    int dlen;

	    dlen = (aptr - bptr) + strlen(s) + 1;
	    i = 0;
	    while (aval[i]) {
		x = aval[i++];
		if (prenum || postnum)
		    x = dopadding(x, prenum, postnum, preone, postone,
				  premul, postmul);
		if (qt && !*x) {
		    x = nulstring;
		    xlen = nulstrlen;
		} else
		    xlen = strlen(x);
		y = (char *)ncalloc(dlen + xlen);
		strcpy(y, ostr);
		strcatsub(y, x, globsubst);
		strcat(y, s);
		if (i == 1)
		    setdata(n, (vptr) y);
		else
		    insnode(l, n, (vptr) y), incnode(n);
	    }
	} else {
	    x = aval[0];
	    if (prenum || postnum)
		x = dopadding(x, prenum, postnum, preone, postone,
			      premul, postmul);
	    if (qt && !*x) {
		x = nulstring;
		xlen = nulstrlen;
	    } else
		xlen = strlen(x);
	    y = (char *)ncalloc((aptr - bptr) + xlen + 1);
	    strcpy(y, ostr);
	    strcatsub(y, x, globsubst);
	    setdata(n, (vptr) y);

	    i = 1;
	    while (aval[i] && aval[i + 1]) {
		x = aval[i++];
		if (prenum || postnum)
		    x = dopadding(x, prenum, postnum, preone, postone,
				  premul, postmul);
		if (qt && !*x)
		    y = dupstring(nulstring);
		else if (globsubst) {
		    y = (char *)ncalloc(strlen(x) + 1);
		    *y = '\0';
		    strcatsub(y, x, 1);
		} else
		    y = x;
		insnode(l, n, (vptr) y), incnode(n);
	    }

	    x = aval[i];
	    if (prenum || postnum)
		x = dopadding(x, prenum, postnum, preone, postone,
			      premul, postmul);
	    if (qt && !*x) {
		x = nulstring;
		xlen = nulstrlen;
	    } else
		xlen = strlen(x);
	    y = (char *)ncalloc(xlen + strlen(s) + 1);
	    strcpy(y, x);
	    strcat(y, s);
	    insnode(l, n, (vptr) y);
	}
    } else {
	int xlen;
	char *x;
	char *y;

	x = val;
	if (prenum || postnum)
	    x = dopadding(x, prenum, postnum, preone, postone,
			  premul, postmul);
	if (qt && !*x) {
	    x = nulstring;
	    xlen = nulstrlen;
	} else
	    xlen = strlen(x);
	y = (char *)ncalloc((aptr - bptr) + xlen + strlen(s) + 1);
	strcpy(y, ostr);
	strcatsub(y, x, globsubst);
	strcat(y, s);
	setdata(n, (vptr) y);
    }

    return 1;
}

/* arithmetic substitution */

void arithsubst(aptr, bptr)	/**/
vptr *aptr;
char **bptr;
{
    char *s = (char *)*aptr, *t, buf[16];
    long v;

    *s = '\0';
    for (; *s != Outbrack; s++);
    *s++ = '\0';
    v = matheval((char *)*aptr + 2);
    sprintf(buf, "%ld", v);
    t = (char *)ncalloc(strlen(*bptr) + strlen(buf) + strlen(s) + 1);
    strcpy(t, *bptr);
    strcat(t, buf);
    strcat(t, s);
    *bptr = t;
}

void modify(str, ptr)		/**/
char **str;
char **ptr;
{
    char *ptr1, *ptr2, *ptr3, del, *lptr, c, *test, *sep, *t, *tt, tc, *e;
    char *copy, *all, *tmp, sav;
    int gbal, wall, rec, al, nl;

    test = NULL;

    if (**ptr == ':')
	*str = dupstring(*str);

    while (**ptr == ':') {
	lptr = *ptr;
	(*ptr)++;
	wall = gbal = 0;
	rec = 1;
	c = '\0';
	sep = NULL;

	for (; !c && **ptr;) {
	    switch (**ptr) {
	    case 'h':
	    case 'r':
	    case 'e':
	    case 't':
	    case 'l':
	    case 'u':
		c = **ptr;
		break;

	    case 's':
		c = **ptr;
		(*ptr)++;
		zsfree(hsubl);
		zsfree(hsubr);
		ptr1 = *ptr;
		del = *ptr1++;
		for (ptr2 = ptr1; *ptr2 != del && *ptr2; ptr2++);
		if (!*ptr2) {
		    zerr("bad subtitution", NULL, 0);
		    return;
		}
		*ptr2++ = '\0';
		for (ptr3 = ptr2; *ptr3 != del && *ptr3; ptr3++);
		if ((sav = *ptr3))
		    *ptr3++ = '\0';
		for (tt = hsubl = ztrdup(ptr1); *tt; tt++)
		    if (INULL(*tt))
			chuck(tt);
		for (tt = hsubr = ztrdup(ptr2); *tt; tt++)
		    if (INULL(*tt))
			chuck(tt);
		ptr2[-1] = del;
		if (sav)
		    ptr3[-1] = sav;
		*ptr = ptr3 - 1;
		break;

	    case '&':
		c = 's';
		break;

	    case 'g':
		(*ptr)++;
		gbal = 1;
		break;

	    case 'w':
		wall = 1;
		(*ptr)++;
		break;
	    case 'W':
		wall = 1;
		(*ptr)++;
		ptr1 = get_strarg(ptr2 = *ptr);
		if ((sav = *ptr1))
		    *ptr1 = '\0';
		sep = dupstring(ptr2 + 1);
		if (sav)
		    *ptr1 = sav;
		*ptr = ptr1 + 1;
		c = '\0';
		break;

	    case 'f':
		rec = -1;
		(*ptr)++;
		break;
	    case 'F':
		rec = -1;
		(*ptr)++;
		ptr1 = get_strarg(ptr2 = *ptr);
		if ((sav = *ptr1))
		    *ptr1 = '\0';
		ptr2 = dupstring(ptr2 + 1);
		if (sav)
		    *ptr1 = sav;
		untokenize(ptr2);
		rec = mathevalarg(ptr2, &ptr2);
		*ptr = ptr1 + 1;
		c = '\0';
		break;
	    default:
		*ptr = lptr;
		return;
	    }
	}
	(*ptr)++;
	if (!c) {
	    *ptr = lptr;
	    return;
	}
	if (rec < 0)
	    test = dupstring(*str);

	while (rec--) {
	    if (wall) {
		al = 0;
		all = NULL;
		for (t = e = *str; (tt = findword(&e, sep));) {
		    tc = *e;
		    *e = '\0';
		    copy = dupstring(tt);
		    *e = tc;
		    switch (c) {
		    case 'h':
			remtpath(&copy);
			break;
		    case 'r':
			remtext(&copy);
			break;
		    case 'e':
			rembutext(&copy);
			break;
		    case 't':
			remlpaths(&copy);
			break;
		    case 'l':
			downcase(&copy);
			break;
		    case 'u':
			upcase(&copy);
			break;
		    case 's':
			if (hsubl && hsubr)
			    subst(&copy, hsubl, hsubr, gbal);
			break;
		    }
		    tc = *tt;
		    *tt = '\0';
		    nl = al + strlen(t) + strlen(copy);
		    ptr1 = tmp = (char *)halloc(nl + 1);
		    if (all)
			for (ptr2 = all; *ptr2;)
			    *ptr1++ = *ptr2++;
		    for (ptr2 = t; *ptr2;)
			*ptr1++ = *ptr2++;
		    *tt = tc;
		    for (ptr2 = copy; *ptr2;)
			*ptr1++ = *ptr2++;
		    *ptr1 = '\0';
		    al = nl;
		    all = tmp;
		    t = e;
		}
		*str = all;

	    } else {
		switch (c) {
		case 'h':
		    remtpath(str);
		    break;
		case 'r':
		    remtext(str);
		    break;
		case 'e':
		    rembutext(str);
		    break;
		case 't':
		    remlpaths(str);
		    break;
		case 'l':
		    downcase(str);
		    break;
		case 'u':
		    upcase(str);
		    break;
		case 's':
		    if (hsubl && hsubr) {
			char *oldstr = *str;

			subst(str, hsubl, hsubr, gbal);
			if (*str != oldstr) {
			    *str = dupstring(oldstr = *str);
			    zsfree(oldstr);
			}
		    }
		    break;
		}
	    }
	    if (rec < 0) {
		if (!strcmp(test, *str))
		    rec = 0;
		else
		    test = dupstring(*str);
	    }
	}
    }
}

/* get a directory stack entry */

char *dstackent(val)		/**/
int val;
{
    Lknode node;

    if ((val < 0 && !firstnode(dirstack)) || !val--)
	return pwd;
    if (val < 0)
	node = lastnode(dirstack);
    else
	for (node = firstnode(dirstack); node && val; val--, incnode(node));
    if (!node) {
	if (!isset(NONOMATCH))
	    zerr("not enough dir stack entries.", NULL, 0);
	return NULL;
    }
    return (char *)getdata(node);
}

/* make an alias hash table node */

struct alias *mkanode(txt, cmflag)	/**/
char *txt;
int cmflag;
{
    struct alias *ptr = (Alias) zcalloc(sizeof *ptr);

    ptr->text = txt;
    ptr->cmd = cmflag;
    ptr->inuse = 0;
    return ptr;
}
