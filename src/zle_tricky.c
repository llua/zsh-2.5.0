/*
 *
 * zle_tricky.c - expansion and completion
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

#define ZLE
#include "zsh.h"
#include	<pwd.h>

#ifdef HAS_NIS
#include	<rpc/types.h>
#include	<rpc/rpc.h>
#include	<rpcsvc/ypclnt.h>
#include	<rpcsvc/yp_prot.h>

#define PASSWD_FILE	"/etc/passwd"
#define PASSWD_MAP	"passwd.byname"

typedef struct {
    int len;
    char *s;
}
dopestring;

#endif

#define inststr(X) inststrlen((X),1,-1)

extern char *glob_pre, *glob_suf;

/*
 * We store the following prefizes/suffizes:
 * lpre/lsuf -- what's on the line
 * rpre/rsuf -- same as lpre/lsuf, but expanded
 *   ... and if we are completing files, too:
 * ppre/psuf -- the path prefix/suffix
 * fpre/fsuf -- prefix/suffix of the pathname component the cursor is in
 * prpre     -- ppre in expanded form usable for opendir
 */

static int we, wb, usemenu, useglob;

static char **menuarr, **menucur;
static int menupos, menulen, menuend, menuwe;
static Lklist matches;
static char **amatches;
static int nmatches;
static int ispattern;
static Comp patcomp, filecomp;
static char *lpre, *lsuf;
static char *rpre, *rsuf;
static char *ppre, *psuf, *prpre;
static char *fpre, *fsuf;
static char *mpre, *msuf;
static char ic;
static int lpl, lsl, rpl, rsl, fpl, fsl, noreal;
static int ab, ae;
static int addwhat;
static char *firstm, *shortest, *qword;
static int shortl, amenu;

int usetab()
{				/**/
    unsigned char *s = line + cs - 1;

    for (; s >= line && *s != '\n'; s--)
	if (*s != '\t' && *s != ' ')
	    return 0;
    return 1;
}

#define COMP_COMPLETE 0
#define COMP_LIST_COMPLETE 1
#define COMP_SPELL 2
#define COMP_EXPAND 3
#define COMP_EXPAND_COMPLETE 4
#define COMP_LIST_EXPAND 5
#define COMP_ISEXPAND(X) ((X) >= COMP_EXPAND)

void completeword()
{				/**/
    usemenu = isset(MENUCOMPLETE);
    useglob = isset(GLOBCOMPLETE);
    if (c == '\t' && usetab())
	selfinsert();
    else
	docomplete(COMP_COMPLETE);
}

void menucompleteword()
{				/**/
    usemenu = 1;
    useglob = isset(GLOBCOMPLETE);
    if (c == '\t' && usetab())
	selfinsert();
    else
	docomplete(COMP_COMPLETE);
}

void listchoices()
{				/**/
    usemenu = isset(MENUCOMPLETE);
    useglob = isset(GLOBCOMPLETE);
    docomplete(COMP_LIST_COMPLETE);
}

void spellword()
{				/**/
    usemenu = useglob = 0;
    docomplete(COMP_SPELL);
}

void deletecharorlist()
{				/**/
    char **mc = menucur;

    usemenu = isset(MENUCOMPLETE);
    useglob = isset(GLOBCOMPLETE);
    if (cs != ll)
	deletechar();
    else
	docomplete(COMP_LIST_COMPLETE);

    menucur = mc;
}

void expandword()
{				/**/
    usemenu = useglob = 0;
    if (c == '\t' && usetab())
	selfinsert();
    else
	docomplete(COMP_EXPAND);
}

void expandorcomplete()
{				/**/
    usemenu = isset(MENUCOMPLETE);
    useglob = isset(GLOBCOMPLETE);
    if (c == '\t' && usetab())
	selfinsert();
    else
	docomplete(COMP_EXPAND_COMPLETE);
}

void menuexpandorcomplete()
{				/**/
    usemenu = 1;
    useglob = isset(GLOBCOMPLETE);
    if (c == '\t' && usetab())
	selfinsert();
    else
	docomplete(COMP_EXPAND_COMPLETE);
}

void listexpand()
{				/**/
    usemenu = isset(MENUCOMPLETE);
    useglob = isset(GLOBCOMPLETE);
    docomplete(COMP_LIST_EXPAND);
}

void reversemenucomplete()
{				/**/
    if (!menucmp) {
	menucompleteword();	/* better than just feep'ing, pem */
	return;
    }
    if (menucur == menuarr)
	menucur = menuarr + arrlen(menuarr) - 1;
    else
	menucur--;
    do_single(*menucur);
}

/*
 * Accepts the current completion and starts a new arg,
 * with the next completions. This gives you a way to accept
 * several selections from the list of matches.
 */
void acceptandmenucomplete()
{				/**/
    if (!menucmp) {
	feep();
	return;
    }
    cs = menuend;
    inststrlen(" ", 1, 1);
    if (mpre)
	inststrlen(mpre, 1, -1);
    if (msuf)
	inststrlen(msuf, 0, -1);
    menupos = cs;
    menuend = cs + (msuf ? strlen(msuf) : 0);
    menulen = 0;
    menuwe = 1;
    menucompleteword();
}

static int lincmd, linredir, lastambig, inwhat, haswhat;
static char *cmdstr;

#define IN_NOTHING 0
#define IN_CMD     1
#define IN_MATH    2
#define IN_COND    3
#define IN_ENV     4

#define HAS_SUFFIX  1
#define HAS_FILES   2
#define HAS_MISC    4
#define HAS_PATHPAT 8

int checkparams(p)		/**/
char *p;
{
    int t0, n, l = strlen(p), e = 0;
    struct hashnode *hn;

    for (t0 = paramtab->hsize - 1, n = 0; n < 2 && t0 >= 0; t0--)
	for (hn = paramtab->nodes[t0]; n < 2 && hn; hn = hn->next)
	    if (pfxlen(p, hn->nam) == l) {
		n++;
		if (strlen(hn->nam) == l)
		    e = 1;
	    }
    return (n == 1) ? (getsparam(p) != NULL) :
	(!menucmp && e && isset(RECEXACT));
}

int cmphaswilds(str)		/**/
char *str;
{
    if ((*str == Inbrack || *str == Outbrack) && !str[1])
	return 0;
    if (str[0] == '%')
	return 0;
    for (; *str;) {
	if (*str == String || *str == Qstring) {
	    if (*++str == Inbrace) {
		for (str++; *str; str++)
		    if (*str == Outbrace)
			break;
	    } else
		for (; *str; str++)
		    if (*str != '^' && *str != Hat &&
			*str != '=' && *str != Equals &&
			*str != '#' && *str != Pound &&
			*str != '~' && *str != Tilde &&
			*str != '+')
			break;
	}
	if (*str == Pound || *str == Hat || *str == Star ||
	    *str == Bar || *str == Inbrack || *str == Inang ||
	    *str == Quest || (*str == Inpar && str[1] == ':'))
	    return 1;
	if (*str)
	    str++;
    }
    return 0;
}

void docomplete(lst)		/**/
int lst;
{
    char *s, *ol;
    int olst = lst, dh, chl = 0;

    if (menucmp) {
	do_menucmp(lst);
	return;
    } else if ((amenu = (isset(AUTOMENU) &&
			 (lastcmd & ZLE_MENUCMP) &&
			 lastambig)))
	usemenu = 1;

    dh = doexpandhist();
    if (dh == 0 || dh == 1)
	return;

    if (!isfirstln && chline != NULL) {
	char *p;

	ol = dupstring((char *) line);
	*hptr = '\0';
	chl = strlen(chline);
	sizeline(ll + chl + 1);
	strcpy((char *) line, chline);
	strcat((char *) line, ol);
	for (p = (char *) line; *p; p++)
	    if (*p == '\n')
		*p = ' ';
	cs += chl;
	ll = strlen((char *) line);
    }
    else
	ol = NULL;
    inwhat = IN_NOTHING;
    qword = NULL;
    s = get_comp_string();
    lexrestore();
    if (ol) {
	strcpy((char *) line, ol);
	ll = strlen((char *) line);
	cs -= chl;
	wb -= chl;
	we -= chl;
	if (wb < 0 || we < 0)
	    return;
    }
    freeheap();
    lexsave();
    if (inwhat == IN_ENV)
	lincmd = 0;
    if (s) {
	if (lst == COMP_EXPAND_COMPLETE) {
	    char *q = s;

	    if (*q == Equals) {
		q = s + 1;
		if (gethnode(q, cmdnamtab) || hashcmd(q, pathchecked))
		    if (isset(RECEXACT))
			lst = COMP_EXPAND;
		    else {
			int t0, n = 0;
			char *fc;
			struct hashnode *hn;

			for (t0 = cmdnamtab->hsize - 1; t0 >= 0; t0--)
			    for (hn = cmdnamtab->nodes[t0]; hn; hn = hn->next) {
				if (strpfx(q, hn->nam) &&
				    ISEXCMD(((Cmdnam) hn)->flags) &&
				    (fc = findcmd(hn->nam))) {
				    zsfree(fc);
				    n++;
				}
				if (n == 2)
				    break;
			    }

			if (n == 1)
			    lst = COMP_EXPAND;
		    }
	    } else if (*q != Tilde) {
		for (; *q && *q != String; q++);
		if (*q == String && q[1] != Inpar && q[1] != Inbrack) {
		    if (*++q == Inbrace) {
			for (++q; *q && *q != Outbrace; q++);
			if (*q && q - s + 1 == cs - wb)
			    lst = COMP_EXPAND;
		    } else {
			char *t, sav;

			for (; *q; q++)
			    if (*q != '^' && *q != Hat &&
				*q != '=' && *q != Equals &&
				*q != '#' && *q != Pound &&
				*q != '~' && *q != Tilde &&
				*q != '+')
				break;
			for (t = q; *q && (ialnum(*q) || *q == '_'); q++);
			sav = *q;
			*q = '\0';
			if (cs - wb == q - s && checkparams(t))
			    lst = COMP_EXPAND;
			*q = sav;
		    }
		    if (lst != COMP_EXPAND)
			lst = COMP_COMPLETE;
		}
		q = s;
	    }
	    if (lst == COMP_EXPAND_COMPLETE) {
		for (q = s; *q; q++)
		    if (*q == Tick || *q == Qtick ||
			*q == String || *q == Qstring)
			break;
		lst = *q ? COMP_EXPAND : COMP_COMPLETE;
	    }
	    if (unset(GLOBCOMPLETE) && cmphaswilds(s))
		lst = COMP_EXPAND;
	}
	if (lincmd && (inwhat == IN_NOTHING))
	    inwhat = IN_CMD;

	if (lst == COMP_SPELL) {
	    char **x = &s;
	    char *q = s;

	    for (; *q; q++)
		if (INULL(*q))
		    *q = Nularg;
	    untokenize(s);
	    cs = wb;
	    foredel(we - wb);
	/* call the real spell checker, ash@aaii.oz.zu */
	    spckword(x, NULL, NULL, !lincmd, 0);
	    inststr(*x);
	} else if (COMP_ISEXPAND(lst)) {
	    char *ol = (olst == COMP_EXPAND_COMPLETE) ?
	    dupstring((char *)line) : (char *)line;
	    int ocs = cs, ne = noerrs;

	    noerrs = 1;

	    doexpansion(s, lst, lincmd);
	    lastambig = 0;

	    noerrs = ne;

	    if (olst == COMP_EXPAND_COMPLETE &&
		!strcmp(ol, (char *)line)) {

		cs = ocs;
		errflag = 0;

		untokenize(s);
		docompletion(s, lst, lincmd);
	    }
	} else
	    docompletion(s, lst, lincmd);
	zsfree(s);
    }
    lexrestore();
    popheap();
    zsfree(qword);
}

void do_menucmp(lst)		/**/
int lst;
{
    if (lst == COMP_LIST_COMPLETE) {
	amatches = menuarr;
	listmatches();
	return;
    }
    if (!*++menucur)
	menucur = menuarr;
    do_single(*menucur);
}

int addedx;			/* 1 if x added to complete in a blank between words */
int instring;			/* 1 if we are completing in a string */

void addx(ptmp)			/**/
char **ptmp;
{
    if (!line[cs] || inblank(line[cs]) || line[cs] == ')') {
	*ptmp = (char *)line;
	line = (unsigned char *)ncalloc(strlen((char *)line) + 3);
	memcpy(line, *ptmp, cs);
	line[cs] = 'x';
	strcpy((char *)line + cs + 1, (*ptmp) + cs);
	addedx = 1;
    } else {
	addedx = 0;
	*ptmp = NULL;
    }
}

char *get_comp_string()
{				/**/
    int t0, tt0, i, j, k, l, cp, rd, inc = 0, opb = -1, sl, im = 0, ocs;
    char *s = NULL, *linptr, *tmp, *p, *tt = NULL, *q = NULL;

    noaliases = isset(COMPLETEALIASES);

    instring = 0;
    for (i = j = k = l = 0, q = p = (char *)line; p < (char *)line + cs; p++)
	if (*p == '`' && !(k & 1))
	    i++, q = p;
	else if (*p == '\"' && !(k & 1))
	    j++;
	else if (*p == '\'' && !(j & 1))
	    k++;
	else if (*p == '(' && p[1] == '(')
	    l++, p++;
	else if (*p == ')' && p[1] == ')')
	    l--, p++;
	else if (*p == '\\' && p[1] && !((k & 1) && p[1] == '\''))
	    p++;
    if ((i & 1) || (j & 1) || (k & 1) || l) {
	instring = (j & 1) ? 2 : (k & 1);
	if (l)
	    im = 2;
	addx(&tmp);
	if (!addedx) {
	    tmp = (char *)line;
	    if (i & 1) {
		line = (unsigned char *)dupstring((char *)line);
		strcpy((char *)line, (char *)tmp);
	    } else
		line = (unsigned char *)dupstring((char *)line);
	}
	for (p = (char *)line; *p; p++)
	    if (*p == '"' || *p == '\'')
		*p = ' ';
	    else if ((*p == '(' && p[1] == '(') || (*p == ')' && p[1] == ')'))
		*p = p[1] = ' ', p++;
    } else
	addx(&tmp);
    if (i & 1) {
	sizeline(ll + 1);
	if (tmp)
	    q = (char *)line + (q - tmp);
	for (p = q + strlen(q); p > q; p--)
	    p[1] = *p;
	*q++ = '$';
	*q = '(';
	ll++;
	cs++;
	inc = 1;
    }
    linptr = (char *)line;
    q = NULL;
  start:
    lincmd = incmdpos;
    linredir = inredir;
    cmdstr = NULL;
    zleparse = 1;
    clwpos = -1;
    lexsave();
    hungets(" ");		/* KLUDGE! */
    hungets(UTOSCP(linptr));
    strinbeg();
    pushheap();
    heapalloc();
    i = tt0 = cp = rd = 0;

    do {
	lincmd = incmdpos;
	linredir = inredir;
	ctxtlex();
	if (tok == DINBRACK)
	    im |= 1;
	else if (tok == DOUTBRACK)
	    im &= ~1;
	else if (tok == DINPAR)
	    im |= 2;
	else if (tok == DOUTPAR)
	    im &= ~2;

	if (tok == ENDINPUT)
	    break;
	if (tok == BAR || tok == AMPER || tok == BARAMP ||
	    tok == DBAR || tok == DAMPER)
	    if (tt)
		break;
	    else
		i = tt0 = cp = rd = 0;
	if (lincmd && tok == STRING)
	    cmdstr = dupstring(tokstr), i = 0;
	if (!zleparse && !tt0) {
	    tt = tokstr ? dupstring(tokstr) : NULL;
	    if (addedx && tt)
		chuck(tt + cs - wb - 1);
	    tt0 = tok;
	    clwpos = i;
	    cp = lincmd;
	    rd = linredir;
	}
	if (!tokstr)
	    continue;
	if (i + 1 == clwsize) {
	    clwords = (char **)realloc(clwords, (clwsize *= 2) * sizeof(char *));
	    memset((vptr) (clwords + i), 0, (clwsize / 2) * sizeof(char *));
	}
	zsfree(clwords[i]);
	clwords[i] = ztrdup(tokstr);
	sl = strlen(tokstr);
	while (sl && clwords[i][sl - 1] == ' ')
	    clwords[i][--sl] = '\0';
	if (clwpos == i++ && addedx)
	    chuck(&clwords[i - 1][((cs - wb - 1) >= sl) ? (sl - 1) : (cs - wb - 1)]);
    }
    while (tok != LEXERR && tok != ENDINPUT &&
	   (tok != SEPER || (zleparse && !tt0)));
    clwnum = (tt || !i) ? i : i - 1;
    zsfree(clwords[clwnum]);
    clwords[clwnum] = NULL;
    t0 = tt0;
    lincmd = cp;
    linredir = rd;

    if (!t0 || t0 == ENDINPUT) {
	s = ztrdup("");
	we = wb = cs;
	clwpos = clwnum;
	t0 = STRING;
    } else if (t0 == STRING) {
	s = ztrdup(clwords[clwpos]);
    } else if (t0 == ENVSTRING) {
	for (s = tt; *s && *s != '='; s++, wb++);
	if (*s) {
	    s++;
	    wb++;
	    t0 = STRING;
	    s = ztrdup(s);
	    inwhat = IN_ENV;
	}
	lincmd = 1;
    }
    hflush();
    strinend();
    errflag = zleparse = 0;
    if (addedx)
	wb++;
    if (we > ll)
	we = ll;
    if (im & 2) {
	if (tokstr) {
	    *tokstr = ' ';
	    if (addedx)
		tokstr[cs - wb + 1] = '\0';
	    s = ztrdup(tokstr);
	}
	if (tmp)
	    line = (unsigned char *)tmp;
	goto check;
    }
    if (t0 == LEXERR && parbegin != -1) {
	if (opb == parbegin) {
	    if (inc)
		cs--, ll--, inc = 0;
	    if (tmp)
		line = (unsigned char *)tmp;
	    feep();
	    noaliases = 0;
	    permalloc();
	    return NULL;
	}
	opb = parbegin;
	linptr += ll + 1 - parbegin;
	popheap();
	permalloc();
	lexrestore();
	if (inc)
	    cs--, ll--, inc = 0;
	goto start;
    }
    if (inc)
	cs--, ll--, inc = 0;
    tt = (char *)line;
    if (tmp)
	line = (unsigned char *)tmp;
    if (t0 != STRING) {
	if (tmp) {
	    tmp = NULL;
	    linptr = (char *)line;
	    popheap();
	    permalloc();
	    lexrestore();
	    goto start;
	}
	feep();
	noaliases = 0;
	permalloc();
	return NULL;
    } else if (clwords[clwpos] && clwords[clwpos][0]) {
	for (p = clwords[clwpos]; *p; p++)
	    if ((*p == String || *p == Equals || *p == Inang || *p == Outang) &&
		p[1] == Inpar)
		break;
	if (*p && cs > p - clwords[clwpos]) {
	    for (q = clwords[clwpos] + we - wb; q > p; q--)
		if (*q == Outpar)
		    break;
	    if (q > p && cs - wb <= q - clwords[clwpos]) {
		if (!tmp) {
		    tmp = (char *)line;
		    tt = dupstring((char *)line);
		}
		line = (unsigned char *)tt;
		line[wb + q - clwords[clwpos] + addedx] = '\0';
		linptr = (char *)line;
		ll = strlen((char *)line) - 1;
		popheap();
		permalloc();
		lexrestore();
		q = NULL;
		goto start;
	    }
	    q = NULL;
	}
    }
  check:

    if (q)
	wb--, we--;

    noaliases = 0;

    inwhat = (im & 1) ? IN_COND : (inwhat == IN_ENV ? IN_ENV : IN_NOTHING);
    im &= ~1;

 /* check if we are in a mathematical expression */

    if (!im) {
	for (tt = s + cs - wb; tt > s && *tt != Inbrack; tt--);
	if (*tt == Inbrack) {
	    if (tt[-1] == String)
		im = 1;
	    else {
		for (p = tt - 1; p > s && (ialnum(*p) || *p == '_'); p--);
		if (*p == String || (*p == Inbrace && p > s && p[-1] == String))
		    im = 1;
		else if (*p == Outpar) {
		    for (i = 1, p--; p >= s && i; p--)
			if (*p == Outpar)
			    i++;
			else if (*p == Inpar)
			    i--;
		    if (!i && ((p >= s && *p == String) ||
			       (p > s && *p == Inbrack && p[-1] == String)))
			im = 1;
		}
	    }
	    if (im) {
		for (i = 1, p = tt + 1; *p && i; p++)
		    if (*p == Inbrack)
			i++;
		    else if (*p == Outbrack)
			i--;
		if (cs - wb >= p - s)
		    im = 0;
	    }
	}
    }
    if (im) {
	inwhat = IN_MATH;
	for (p = s + cs - wb - 1; p >= s && (ialnum(*p) || *p == '_'); p--);
	if (++p > s) {
	    strcpy(s, p);
	    wb += p - s - ((im & 2) ? 1 : 0);
	}
	for (p = s; *p && (ialnum(*p) || *p == '_'); p++);
	*p = '\0';
	we = wb + p - s;
    }
    qword = ztrdup(s);
    for (p = s, tt = qword, i = wb; *p; p++, tt++, i++)
	if (INULL(*p)) {
	    if (p[1] || *p != Bnull) {
		if (*p == Bnull)
		    *tt = '\\';
		else {
		    ocs = cs;
		    cs = i;
		    foredel(1);
		    chuck(tt--);
		    if ((cs = ocs) >= i--)
			cs--;
		    we--;
		}
	    } else {
		ocs = cs;
		*tt = '\0';
		cs = we;
		backdel(1);
		if (ocs == we)
		    cs = we - 1;
		else
		    cs = ocs;
		we--;
	    }
	    chuck(p--);
	}
    if (unset(NOBANGHIST)) {
	q = tt = ncalloc(2 * strlen(qword) + 1);
	for (p = qword; *p; p++) {
	    if (*p == (char)bangchar)
		*q++ = '\\';	/*, wb--, cs--;*/
	    *q++ = *p;
	}
	*q = '\0';
	zsfree(qword);
	qword = ztrdup(tt);
    }
    ll = strlen((char *)line);
    permalloc();

    return (char *)s;
}

void doexpansion(s, lst, explincmd)	/**/
char *s;
int lst;
int explincmd;
{
    Lklist vl = newlist();
    char *ss;
    int ng = opts[NULLGLOB];

    opts[NULLGLOB] = OPT_SET;
    lexsave();
    s = dupstring(s);
    pushheap();
    addnode(vl, s);
    prefork(vl, 0);
    if (errflag)
	goto end;
    postfork(vl, (lst == COMP_LIST_EXPAND) || (lst == COMP_EXPAND));
    if (errflag)
	goto end;
    if (empty(vl) || !*(char *)peekfirst(vl)) {
	if (!noerrs)
	    feep();
	goto end;
    }
    if (lst == COMP_LIST_EXPAND) {
	listlist(vl);
	goto end;
    } else if (peekfirst(vl) == (vptr) s ||
	       (!nextnode(firstnode(vl)) && *s == Tilde &&
		(ss = dupstring(s), filesubstr(&ss, 0)) &&
		!strcmp(ss, (char *)peekfirst(vl)))) {
	if (lst == COMP_EXPAND_COMPLETE)
	    docompletion(s, COMP_COMPLETE, explincmd);
	else
	    feep();
	goto end;
    }
    cs = wb;
    foredel(we - wb);
    while ((ss = (char *)ugetnode(vl))) {
	untokenize(ss);
	ss = quotename(ss, NULL, NULL, NULL);
	inststr(ss);
#if 0
	if (full(vl)) {
	    spaceinline(1);
	    line[cs++] = ' ';
	}
#endif
	spaceinline(1);
	line[cs++] = ' ';
    }
  end:
    opts[NULLGLOB] = ng;
    popheap();
    lexrestore();
}

void gotword(s)			/**/
char *s;
{
    we = ll + 1 - inbufct;
    if (cs <= we) {
	wb = ll - wordbeg;
	zleparse = 0;
    /* major hack ahead */
	if (wb > 0 && line[wb] == '!' && line[wb - 1] == '\\')
	    wb--;
    }
}

void inststrlen(str, move, len)	/**/
char *str;
int move;
int len;
{
    if (!len)
	return;
    if (len == -1)
	len = strlen(str);
    spaceinline(len);
    strncpy((char *)(line + cs), str, len);
    if (move)
	cs += len;
}

char *quotename(s, e, te, pl)	/**/
char *s;
char **e;
char **te;
int *pl;
{
    char *tt, *v, *u, buf[MAXPATHLEN * 2];
    int sf = 0;

    tt = v = buf;
    u = s;
    for (; *u; u++) {
	if (e && *e == u)
	    *e = v, sf |= 1;
	if (te && *te == u)
	    *pl = v - tt, sf |= 2;
	if (ispecial(*u) &&
	    (!instring || (!isset(NOBANGHIST) &&
			   *u == (char)bangchar) ||
	     (instring == 2 &&
	      (*u == '$' || *u == '`' || *u == '\"')) ||
	     (instring == 1 && *u == '\'')))
	    if (*u == '\n' || (instring == 1 && *u == '\'')) {
		*v++ = '\'';
		if (*u == '\'')
		    *v++ = '\\';
		*v++ = *u;
		*v++ = '\'';
		continue;
	    } else
		*v++ = '\\';
	*v++ = *u;
    }
    *v = '\0';
    if (strcmp(buf, s))
	tt = dupstring(buf);
    else
	tt = s;
    v += tt - buf;
    if (e && (sf & 1))
	*e += tt - buf;
    if (te && (sf & 2))
	*te += tt - buf;

    if (e && *e == u)
	*e = v;
    if (te && *te == u)
	*pl = v - tt;

    return tt;
}

void addmatch(s, t)		/**/
char *s;
char *t;
{
    int test = 0, sl = strlen(s), pl = rpl, cc = 0;
    char sav = 0, *e = NULL, *tt, *te, *fc;
    Comp cp = patcomp;
    Param pm;

    if (!addwhat)
	test = 1;
    else if (addwhat == -1 || addwhat == -5 || addwhat == -6 ||
	     addwhat == CC_FILES || addwhat == -7 || addwhat == -8) {
	if (sl < fpl + fsl)
	    return;

	if ((addwhat == CC_FILES ||
	     addwhat == -5) && !*psuf && !*fsuf) {
	    char **pt = fignore;
	    int filell;

	    for (test = 1; test && *pt; pt++)
		if ((filell = strlen(*pt)) < sl
		    && !strcmp(*pt, s + sl - filell))
		    test = 0;

	    if (!test)
		return;
	}
	pl = fpl;
	if (addwhat == -5 || addwhat == -8) {
	    test = 1;
	    cp = filecomp;
	    cc = cp || ispattern;
	    e = s + sl - fsl;
	} else {
	    if ((cp = filecomp)) {
		if ((test = domatch(s, filecomp, 0)))
		    cc = 1;
	    } else {
		e = s + sl - fsl;
		if ((test = !strncmp(s, fpre, fpl)))
		    test = !strcmp(e, fsuf);
		if (ispattern)
		    cc = 1;
	    }
	}
	if (test) {
	    fc = NULL;
	    if (addwhat == -7 && ISEXCMD(((Cmdnam) t)->flags) &&
		!(fc = findcmd(s)))
		return;
	    if (fc)
		zsfree(fc);
	    haswhat |= HAS_FILES;

	    if (addwhat == CC_FILES || addwhat == -6 ||
		addwhat == -5 || addwhat == -8) {
		te = s + pl;
		s = quotename(s, &e, &te, &pl);
		sl = strlen(s);
	    } else if (!cc) {
		s = dupstring(t = s);
		e += s - t;
	    }
	    if (cc) {
		tt = (char *)halloc(strlen(ppre) + strlen(psuf) + sl + 1);
		strcpy(tt, ppre);
		strcat(tt, s);
		strcat(tt, psuf);
		untokenize(s = tt);
	    }
	}
    } else if (addwhat == -2 ||
	       (addwhat == -3 && !(((Cmdnam) t)->flags & DISABLED)) ||
	       (addwhat == -4 && (pm = (Param) t) && (pmtype(pm) == PMFLAG_s) &&
		(tt = pm->gets.cfn(pm)) && *tt == '/') ||
	       (addwhat > 0 &&
		(((addwhat & CC_ARRAYS) && (((Param) t)->flags & PMFLAG_A)) ||
		 ((addwhat & CC_INTVARS) && (((Param) t)->flags & PMFLAG_i)) ||
		 ((addwhat & CC_ENVVARS) && (((Param) t)->flags & PMFLAG_x)) ||
		 ((addwhat & CC_SCALARS) &&
		  !(((Param) t)->flags & (PMFLAG_A | PMFLAG_i))) ||
		 ((addwhat & CC_READONLYS) && (((Param) t)->flags & PMFLAG_r)) ||
		 ((addwhat & CC_SPECIALS) && (((Param) t)->flags & PMFLAG_SPECIAL)) ||
		 ((addwhat & CC_PARAMS) && !(((Param) t)->flags & PMFLAG_x)) ||
		 ((addwhat & CC_FUNCS) && (((Cmdnam) t)->flags & SHFUNC)) ||
		 ((addwhat & CC_BUILTINS) && (((Cmdnam) t)->flags & BUILTIN) &&
		  !(((Cmdnam) t)->flags & EXCMD)) ||
		 ((addwhat & CC_DISCMDS) && (((Cmdnam) t)->flags & DISABLED)) ||
		 ((addwhat & CC_EXCMDS) && (((Cmdnam) t)->flags & EXCMD)) ||
		 ((addwhat & CC_ALREG) && (((Alias) t)->cmd) == 1) ||
		 ((addwhat & CC_ALGLOB) && !(((Alias) t)->cmd))))) {
	if (sl >= rpl + rsl) {
	    if (cp)
		test = domatch(s, patcomp, 0);
	    else {
		e = s + sl - rsl;
		if ((test = !strncmp(s, rpre, rpl)))
		    test = !strcmp(e, rsuf);
	    }
	}
	if (!test && sl < lpl + lsl)
	    return;
	if (!test && !noreal && sl >= lpl + lsl) {
	    e = s + sl - lsl;
	    if ((test = !strncmp(s, lpre, lpl)))
		test = !strcmp(e, lsuf);
	    pl = lpl;
	}
	if (test)
	    haswhat |= HAS_MISC;
    }
    if (!test)
	return;

    t = s += (ispattern ? 0 : pl);
    e += t - s;
    s = t;

    if (ispattern)
	e = NULL, sav = '\0';
    else {
	if ((sav = *e)) {
	    *e = '\0';
	    t = dupstring(t);
	}
    }

    if (!ispattern && firstm) {
	if ((test = pfxlen(firstm, s)) < ab)
	    ab = test;
	if ((test = sfxlen(firstm, s)) < ae)
	    ae = test;
    }
 /* If we are doing a glob completion we store the whole string in
     * the list. Otherwise only the part that fits between the prefix
     * and the suffix is stored. */

    addnode(matches, t);
    if (!firstm)
	firstm = t, ab = ae = shortl = 100000;
    if (!ispattern && (sl = strlen(t)) < shortl)
	shortl = sl, shortest = t;
    if (sav)
	*e = sav;
}

#ifdef HAS_NIS
static int match_username(status, key, keylen, val, vallen, data)
int status;
char *key, *val;
int keylen, vallen;
dopestring *data;
{
    if (errflag || status != YP_TRUE)
	return 1;

    if (vallen > keylen && val[keylen] == ':') {
	val[keylen] = '\0';
	addmatch(dupstring(val), NULL);
    }
    return 0;
}

#endif

void maketildelist()
{				/**/
    int i;

#ifdef HAS_NIS
    char domain[YPMAXDOMAIN];
    struct ypall_callback cb;
    dopestring data;
    FILE *pwf;
    char buf[BUFSIZ], *p;
    int skipping;

    data.s = fpre;
    data.len = fpl;
 /* Get potential matches from NIS and cull those without local accounts */
    if (getdomainname(domain, YPMAXDOMAIN) == 0) {
	cb.foreach = (int ((*)()))match_username;
	cb.data = (char *)&data;
	yp_all(domain, PASSWD_MAP, &cb);
/*	for (n = firstnode(matches); n; incnode(n))
	    if (getpwnam(getdata(n)) == NULL)
		uremnode(matches, n);*/
    }
 /* Don't forget the non-NIS matches from the flat passwd file */
    if ((pwf = fopen(PASSWD_FILE, "r")) != NULL) {
	skipping = 0;
	while (fgets(buf, BUFSIZ, pwf) != NULL) {
	    if (strchr(buf, '\n') != NULL) {
		if (!skipping) {
		    if ((p = strchr(buf, ':')) != NULL) {
			*p = '\0';
			addmatch(dupstring(buf), NULL);
		    }
		} else
		    skipping = 0;
	    } else
		skipping = 1;
	}
	fclose(pwf);
    }
#else
    struct passwd *tmppwd;

#ifdef CACHE_USERNAMES
    static int usernamescached = 0;

    if (!usernamescached) {
	setpwent();
	while ((tmppwd = getpwent()) != NULL && !errflag)
	    adduserdir(dupstring(tmppwd->pw_name), tmppwd->pw_dir, 1, 1);
	endpwent();
	usernamescached = 1;
    }
#else
    setpwent();
    while ((tmppwd = getpwent()) != NULL && !errflag)
	addmatch(dupstring(tmppwd->pw_name), NULL);
    endpwent();

    if (addwhat != -1)
	return;
#endif
#endif

    for (i = 0; i < userdirct; i++)
	if (addwhat == -1 || namdirs[i].homedir)
	    addmatch(namdirs[i].name, NULL);
}

char *rembslash(s)		/**/
char *s;
{
    char *t = s = dupstring(s);

    while (*s)
	if (*s == '\\') {
	    chuck(s);
	    if (*s)
		s++;
	} else
	    s++;

    return t;
}

int getcpat(str, cpatindex, cpat, class)	/**/
char *str;
int cpatindex;
char *cpat;
int class;
{
    char *s, *t, *p;
    int d = 0;

    if (!str || !*str)
	return -1;

    cpat = rembslash(cpat);

    str = ztrdup(str);
    untokenize(str);
    if (!cpatindex)
	cpatindex++, d = 0;
    else if ((d = (cpatindex < 0)))
	cpatindex = -cpatindex;

    for (s = d ? str + strlen(str) - 1 : str;
	 d ? (s >= str) : *s;
	 d ? s-- : s++) {
	for (t = s, p = cpat; *t && *p; p++) {
	    if (class) {
		if (*p == *s && !--cpatindex) {
		    zsfree(str);
		    return (int)(s - str + 1);
		}
	    } else if (*t++ != *p)
		break;
	}
	if (!class && !*p && !--cpatindex) {
	    zsfree(str);
	    return (int)(t - str);
	}
    }
    zsfree(str);
    return -1;
}

Compctl ccmain;

Compctl get_ccompctl(occ, compadd, incmd)	/**/
Compctl occ;
int *compadd;
int incmd;
{
    Compctl compc, ret;
    Compctlp ccp;
    int t, i, a, b, tt, ra = 0, rb = 0, j;
    Compcond or, cc;
    char *s, *ss, *sc = NULL;
    Comp comp;

    *compadd = 0;

    if (!(ret = compc = occ)) {
	if (inwhat == IN_ENV)
	    ret = &cc_default;
	else if (inwhat == IN_MATH) {
	    cc_dummy.mask = CC_PARAMS;
	    ret = &cc_dummy;
	    cc_dummy.refc = 10000;
	} else if (inwhat == IN_COND) {
	    s = clwpos ? clwords[clwpos - 1] : "";
	    cc_dummy.mask = !strcmp("-o", s) ? CC_OPTIONS :
		((*s == '-' && s[1] && !s[2]) ||
		 !strcmp("-nt", s) ||
		 !strcmp("-ot", s) ||
		 !strcmp("-ef", s)) ? CC_FILES :
		(CC_FILES | CC_PARAMS);
	    ret = &cc_dummy;
	    cc_dummy.refc = 10000;
	} else if (incmd)
	    ret = &cc_compos;
	else if (linredir ||
		 !(cmdstr &&
		   (((ccp = (Compctlp) gethnode(cmdstr, compctltab)) &&
		     (compc = ret = ccp->cc)) ||
		    ((s = dupstring(cmdstr)) && remlpaths(&s) &&
		     (ccp = (Compctlp) gethnode(s, compctltab)) &&
		     (compc = ret = ccp->cc)))))
	    ret = &cc_default;

	ccmain = compc = ret;
	ccmain->refc++;
    }
    if (compc && compc->ext) {
	compc = compc->ext;
	for (t = 0; compc && !t; compc = compc->next) {
	    for (cc = compc->cond; cc && !t; cc = or) {
		or = cc->or;
		for (t = 1; cc && t; cc = cc->and) {
		    for (t = i = 0; i < cc->n && !t; i++) {
			s = NULL;
			ra = 0;
			rb = clwnum - 1;
			switch (cc->type) {
			case CCT_POS:
			    tt = clwpos;
			    goto cct_num;
			case CCT_NUMWORDS:
			    tt = clwnum;
			  cct_num:
			    if ((a = cc->u.r.a[i]) < 0)
				a += clwnum;
			    if ((b = cc->u.r.b[i]) < 0)
				b += clwnum;
			    if (cc->type == CCT_POS)
				ra = a, rb = b;
			    t = (tt >= a && tt <= b);
			    break;
			case CCT_CURSUF:
			case CCT_CURPRE:
			    s = ztrdup(clwpos < clwnum ? clwords[clwpos] : "");
			    untokenize(s);
			    sc = rembslash(cc->u.s.s[i]);
			    a = strlen(sc);
			    if (!strncmp(s, sc, a)) {
				*compadd = (cc->type == CCT_CURSUF ? a : 0);
				t = 1;
			    }
			    break;
			case CCT_CURSUB:
			case CCT_CURSUBC:
			    if (clwpos < 0 || clwpos > clwnum)
				t = 0;
			    else {
				if ((a = getcpat(clwords[clwpos],
						 cc->u.s.p[i],
						 cc->u.s.s[i],
						 cc->type == CCT_CURSUBC)) != -1)
				    *compadd = a, t = 1;
			    }
			    break;

			case CCT_CURPAT:
			case CCT_CURSTR:
			    tt = clwpos;
			    goto cct_str;
			case CCT_WORDPAT:
			case CCT_WORDSTR:
			    tt = 0;
			  cct_str:
			    if ((a = tt + cc->u.s.p[i]) < 0)
				a += clwnum;
			    s = ztrdup((a < 0 || a >= clwnum) ? "" : clwords[a]);
			    untokenize(s);

			    if (cc->type == CCT_CURPAT || cc->type == CCT_WORDPAT) {
				tokenize(ss = dupstring(cc->u.s.s[i]));
				t = ((comp = parsereg(ss)) && domatch(s, comp, 0));
			    } else
				t = (!strcmp(s, rembslash(cc->u.s.s[i])));
			    break;
			case CCT_RANGESTR:
			case CCT_RANGEPAT:
			    if (cc->type == CCT_RANGEPAT)
				tokenize(sc = dupstring(cc->u.l.a[i]));
			    for (j = clwpos; j; j--) {
				untokenize(s = ztrdup(clwords[j]));
				if (cc->type == CCT_RANGESTR)
				    sc = rembslash(cc->u.l.a[i]);
				if (cc->type == CCT_RANGESTR ?
				    !strncmp(s, sc, strlen(sc)) :
				    ((comp = parsereg(sc)) &&
				     domatch(s, comp, 0))) {
				    zsfree(s);
				    ra = j + 1;
				    t = 1;
				    break;
				}
				zsfree(s);
			    }
			    if (t) {
				if (cc->type == CCT_RANGEPAT)
				    tokenize(sc = dupstring(cc->u.l.b[i]));
				for (j++; j < clwnum; j++) {
				    untokenize(s = ztrdup(clwords[j]));
				    if (cc->type == CCT_RANGESTR)
					sc = rembslash(cc->u.l.b[i]);
				    if (cc->type == CCT_RANGESTR ?
					!strncmp(s, sc, strlen(sc)) :
					((comp = parsereg(sc)) &&
					 domatch(s, comp, 0))) {
					zsfree(s);
					rb = j - 1;
					t = clwpos <= rb;
					break;
				    }
				    zsfree(s);
				}
			    }
			    s = NULL;
			}
			zsfree(s);
		    }
		}
	    }
	    if (t)
		break;
	}
	if (compc)
	    ret = compc;
    }
    if (ret->subcmd) {
	char **ow = clwords, *os = cmdstr, *ops = NULL;
	int oldn = clwnum, oldp = clwpos;

	if (ra < 1)
	    ra = 1;
	if (ra >= clwnum)
	    ra = clwnum - 1;
	if (rb < 1)
	    rb = 1;
	if (rb >= clwnum)
	    rb = clwnum - 1;

	clwnum = rb - ra + 1;
	clwpos = clwpos - ra;
	if (ret->subcmd[0]) {
	    clwnum++;
	    clwpos++;
	    incmd = 0;
	    ops = clwords[ra - 1];
	    clwords[ra - 1] = cmdstr = ret->subcmd;
	    clwords += ra - 1;
	} else {
	    cmdstr = clwords[ra];
	    incmd = !clwpos;
	    clwords += ra;
	}
	*compadd = 0;
	if (ccmain != &cc_dummy)
	    freecompctl(ccmain);
	ret = get_ccompctl(NULL, compadd, incmd);
	clwords = ow;
	cmdstr = os;
	clwnum = oldn;
	clwpos = oldp;
	if (ops)
	    clwords[ra - 1] = ops;
    }
    return ret;
}

void dumphtable(ht, what)	/**/
Hashtab ht;
int what;
{
    int t0;
    struct hashnode *hn;

    addwhat = what;

    for (t0 = ht->hsize - 1; t0 >= 0; t0--)
	for (hn = ht->nodes[t0]; hn; hn = hn->next)
	    addmatch(hn->nam, (char *)hn);

}

char *getreal(str)		/**/
char *str;
{
    Lklist l = newlist();
    int ne = noerrs;

    noerrs = 1;
    addnode(l, dupstring(str));
    prefork(l, 0);
    if (!errflag) {
	postfork(l, 0);
	if (!errflag && full(l)) {
	    noerrs = ne;
	    return ztrdup(peekfirst(l));
	}
    }
    errflag = 0;
    noerrs = ne;

    return ztrdup(str);
}

void gen_matches_files(dirs, execs, all)	/**/
int dirs;
int execs;
int all;
{
    DIR *d;
    struct dirent *de;
    struct stat buf;
    char *n, p[MAXPATHLEN], *q = NULL, *e;
    Lklist l = NULL;
    int ns = 0, ng = opts[NULLGLOB], test, aw = addwhat;

    addwhat = execs ? -8 : -5;
    opts[NULLGLOB] = OPT_SET;

    if (*psuf) {
	q = psuf + strlen(psuf) - 1;
	ns = !(*q == Star || *q == Outpar);
	l = newlist();
	dirs = 1;
	all = execs = 0;
    }
    if ((d = opendir((prpre && *prpre) ? prpre : "."))) {
	if (!all && prpre) {
	    strcpy(p, prpre);
	    q = p + strlen(prpre);
	}
	while ((de = readdir(d)) && !errflag) {
	    n = de->d_name;
	    if (n[0] == '.' && (n[1] == '\0' || (n[1] == '.' && n[2] == '\0')))
		continue;
	    if (*n != '.' || *fpre == '.' || isset(GLOBDOTS)) {
		if (filecomp)
		    test = domatch(n, filecomp, 0);
		else {
		    e = n + strlen(n) - fsl;
		    if ((test = !strncmp(n, fpre, fpl)))
			test = !strcmp(e, fsuf);
		}
		if (!test)
		    continue;
		if (!all) {
		    strcpy(q, n);
		    if (stat(p, &buf) < 0)
			continue;
		}
		if (all ||
		    (dirs && (buf.st_mode & S_IFMT) == S_IFDIR) ||
		    (execs &&
		     ((buf.st_mode & (S_IFMT | S_IEXEC))
		      == (S_IFREG | S_IEXEC)))) {
		    if (*psuf) {
			int o = strlen(p), tt;

			strcpy(p + o, psuf);

			if (ispattern || (ns && isset(GLOBCOMPLETE))) {
			    if (ns) {
				int tl = strlen(p);

				p[tl] = Star;
				p[tl + 1] = '\0';
			    }
			    addnode(l, p);
			    postfork(l, 1);
			    tt = full(l);
			    while (ugetnode(l));
			} else
			    tt = !access(p, F_OK);

			p[o] = '\0';
			if (tt)
			    addmatch(dupstring(n), NULL);
		    } else
			addmatch(dupstring(n), NULL);
		}
	    }
	}
	closedir(d);
    }
    opts[NULLGLOB] = ng;
    addwhat = aw;
}

char *expl, *ccsuffix;
int remsuffix;

void quotepresuf(ps)		/**/
char **ps;
{
    if (*ps) {
	char *p = quotename(*ps, NULL, NULL, NULL);

	if (p != *ps) {
	    zsfree(*ps);
	    *ps = ztrdup(p);
	}
    }
}

int clearflag;

void docompletion(s, lst, incmd)/**/
char *s;
int lst;
int incmd;
{
    Compctl cc = NULL;
    int offs, t, sf1, sf2, compadd, isp = 0, ooffs;
    char *p, *sd = NULL, sav, *tt, *s1, *s2, *os = NULL;
    int owe = we, owb = wb, ocs = cs, delit;
    unsigned char *ol = NULL;

  xorrec:

    if (unset(COMPLETEINWORD) && cs != we)
	cs = we;
    if ((offs = cs - wb) > (t = strlen(s)))
	offs = t;

    ispattern = haswhat = lastambig = 0;
    patcomp = filecomp = NULL;
    menucur = NULL;
    shortest = NULL;

    zsfree(rpre);
    zsfree(rsuf);
    zsfree(lpre);
    zsfree(lsuf);
    zsfree(ppre);
    zsfree(psuf);
    zsfree(prpre);
    zsfree(fpre);
    zsfree(fsuf);
    zsfree(mpre);
    zsfree(msuf);

    rpre = rsuf = lpre = lsuf = ppre = psuf = prpre =
	fpre = fsuf = mpre = msuf = firstm = NULL;

    if (!cc) {
	heapalloc();
	pushheap();
	os = dupstring(s);
	ol = (unsigned char *)dupstring((char *)line);
    }
    matches = newlist();

    if (!cc || cc->ext)
	cc = get_ccompctl(cc, &compadd, incmd);

    wb += compadd;
    s += compadd;
    if ((offs -= compadd) < 0) {
	feep();
	goto compend;
    }
 /* insert prefix, if any */

    if (cc->prefix) {
	int pl = 0, sl = strlen(cc->prefix);

	if (*s) {
	    sd = dupstring(s);
	    untokenize(sd);
	    pl = pfxlen(cc->prefix, sd);
	    s += pl;
	}
	if (pl < sl) {
	    int savecs = cs;

	    cs = wb + pl;
	    inststrlen(cc->prefix + pl, 0, sl - pl);
	    cs = savecs + sl - pl;
	}
	wb += sl;
	we += sl - pl;
    }
    if ((ccsuffix = cc->suffix) && *ccsuffix) {
	char *sdup = dupstring(ccsuffix);
	int sl = strlen(sdup), suffixll;

	for (p = sdup + sl - 1; p >= sdup && *p == ' '; p--, sl--);
	p[1] = '\0';

	if (!sd) {
	    sd = dupstring(s);
	    untokenize(sd);
	}
	if (*sd && (suffixll = strlen(sd)) >= sl &&
	    !strcmp(sdup, sd + suffixll - sl))
	    ccsuffix = NULL, haswhat |= HAS_SUFFIX, s[suffixll - sl] = '\0';
    }
    if ((ic = *s) != Tilde && ic != Equals)
	ic = '\0';
    offs = cs - wb;
    if ((offs = cs - wb) > (t = strlen(s)))
	offs = t;

 /* Check if we have to complete a parameter name... */

    for (p = s + offs; p > s && *p != String; p--);
    if (*p == String && p[1] != Inpar && p[1] != Inbrack) {
	char *b = p + 1, *e = b;
	int n, br = 1;

	if (*b == Inbrace)
	    b++, br++;

	for (t = 0; *b && !t; b++)
	    if (*b == Inpar) {
		for (n = 1, b++; *b && b < s + offs && n; b++)
		    if (*b == Inpar)
			n++;
		    else if (*b == Outpar)
			n--;
		b--;
	    } else
		t = ialnum(*b) || *b == '_' || *b == Star || *b == Quest ||
		    *b == Inbrack || *b == Outbrace || *b == '/';

	for (e = b; *e && t; e++)
	    if (!(ialnum(*e) || *e == '_' || *e == Star || *e == Quest))
		break;

	if (offs <= e - s) {
	    if (b > s && b[-1] != String)
		b--;
	    if (cs == we)
		complexpect = br;
	    wb += b - s;
	    offs -= b - s;
	    we = wb + e - b;
	    *e = '\0';
	    s = b;
	    isp = 1;
	    cc = ccmain = &cc_dummy;
	    cc_dummy.refc = 10000;
	    cc_dummy.mask = CC_PARAMS | CC_ENVVARS;
	} else
	    complexpect = 0;
    }
    ooffs = offs;
    if (cc->mask & CC_DELETE) {
	delit = 1;
	*s = '\0';
	offs = 0;
    } else
	delit = 0;

 /* compute line prefix/suffix */

    sav = s[offs];
    s[offs] = '\0';
    p = quotename(s, NULL, NULL, NULL);
    if (strcmp(p, s) && !strpfx(p, qword)) {
	int l1, l2;

	backdel(l1 = cs - wb);
	untokenize(p);
	inststrlen(p, 1, l2 = strlen(p));
	we += l2 - 1l;
    }
    lpre = ztrdup(s);
    s[offs] = sav;
    if (s[offs] &&
	(p = quotename(s + offs, NULL, NULL, NULL)) &&
	(strcmp(p, s + offs) && !strsfx(p, qword))) {
	int l1, l2;

	foredel(l1 = strlen(s + offs));
	untokenize(p);
	inststrlen(p, 0, l2 = strlen(p));
	we += l2 - l1;
    }
    lsuf = ztrdup(s + offs);

 /* first check for ~... and =... */

    if (ic) {
	for (p = lpre + strlen(lpre); p > lpre; p--)
	    if (*p == '/')
		break;

	if (*p == '/')
	    ic = 0;
    }
 /* compute real prefix/suffix */

    noreal = !delit;
    for (p = lpre; *p && *p != String && *p != Tick; p++);
    tt = ic ? lpre + 1 : lpre;
    rpre = (*p || *lpre == Tilde || *lpre == Equals) ?
	(noreal = 0, getreal(tt)) :
	ztrdup(tt);

    for (p = lsuf; *p && *p != String && *p != Tick; p++);
    rsuf = *p ? (noreal = 0, getreal(lsuf)) : ztrdup(lsuf);

 /* check if word is a pattern */

    for (s1 = NULL, sf1 = 0, p = rpre + (rpl = strlen(rpre)) - 1;
	 p >= rpre && (ispattern != 3 || !sf1);
	 p--)
	if (itok(*p) && (p > rpre || (*p != Equals && *p != Tilde)))
	    ispattern |= sf1 ? 1 : 2;
	else if (*p == '/') {
	    sf1++;
	    if (!s1)
		s1 = p;
	}
    for (s2 = NULL, sf2 = t = 0, p = rsuf; *p && (!t || !sf2); p++)
	if (itok(*p))
	    t |= sf2 ? 4 : 2;
	else if (*p == '/') {
	    sf2++;
	    if (!s2)
		s2 = p;
	}
    ispattern = ispattern | t;

    if (!useglob)
	ispattern = 0;

    if (ispattern) {
	p = (char *)ncalloc(rpl + rsl + 2);
	strcpy(p, rpre);
	if (rpl && p[rpl - 1] != Star) {
	    p[rpl] = Star;
	    strcpy(p + rpl + 1, rsuf);
	} else
	    strcpy(p + rpl, rsuf);
	patcomp = parsereg(p);
    }
    if (!patcomp) {
	untokenize(rpre);
	untokenize(rsuf);

	rpl = strlen(rpre);
	rsl = strlen(rsuf);
    }
    untokenize(lpre);
    untokenize(lsuf);

    lpl = strlen(lpre);
    lsl = strlen(lsuf);

 /* handle completion of files specially */

    if ((cc->mask & (CC_FILES | CC_COMMPATH)) || cc->glob) {
	if (!s1)
	    s1 = rpre;
	if (!s2)
	    s2 = rsuf + rsl;

	if (*s1 != '/')
	    ppre = ztrdup("");
	else {
	    if ((sav = *s1 ? s1[1] : '\0'))
		s1[1] = '\0';
	    ppre = ztrdup(rpre);
	    if (sav)
		s1[1] = sav;
	}
	psuf = ztrdup(s2);

	fpre = ztrdup(((s1 == s || s1 == rpre || ic) &&
		       (*s != '/' || cs == wb)) ? s1 : s1 + 1);
	sav = *s2;
	*s2 = '\0';
	fsuf = ztrdup(rsuf);
	*s2 = sav;

	if (useglob && (ispattern & 2)) {
	    int t2;

	    p = (char *)ncalloc((t2 = strlen(fpre)) + strlen(fsuf) + 2);
	    strcpy(p, fpre);
	    if ((!t2 || p[t2 - 1] != Star) && *fsuf != Star)
		p[t2++] = Star;
	    strcpy(p + t2, fsuf);
	    filecomp = parsereg(p);
	}
	if (!filecomp) {
	    untokenize(fpre);
	    untokenize(fsuf);

	    fpl = strlen(fpre);
	    fsl = strlen(fsuf);
	}
	addwhat = -1;

	if (ic == Tilde)
	    maketildelist();
	else if (ic == Equals) {
	    if (isset(HASHLISTALL))
		fullhash();
	    dumphtable(cmdnamtab, -7);
	} else {
	    if (ispattern & 1) {
		Lklist l = newlist();
		Lknode n;
		int ng = opts[NULLGLOB];

		opts[NULLGLOB] = OPT_SET;

		addwhat = 0;
		p = (char *)ncalloc(lpl + lsl + 3);
		strcpy(p, lpre);
		if (*lsuf != '*' && *lpre && lpre[lpl - 1] != '*')
		    strcat(p, "*");
		strcat(p, lsuf);
		if (*lsuf && lsuf[lsl - 1] != '*' && lsuf[lsl - 1] != ')')
		    strcat(p, "*");

		tokenize(p);
		addnode(l, p);
		postfork(l, 1);

		if (full(l)) {
		    haswhat |= HAS_PATHPAT;
		    for (n = firstnode(l); n; incnode(n))
			addmatch(getdata(n), NULL);
		}
		opts[NULLGLOB] = ng;
	    } else {
		addwhat = CC_FILES;
		prpre = ztrdup(ppre);

		if (sf2)
		    gen_matches_files(1, 0, 0);
		else {
		    if (cc->mask & CC_FILES)
			gen_matches_files(0, 0, 1);
		    else if (cc->mask & CC_COMMPATH) {
			if (sf1)
			    gen_matches_files(1, 1, 0);
			else {
			    char **pc = path, *pp = prpre;

			    for (; *pc; pc++)
				if (pc[0][0] == '.' && !pc[0][1])
				    break;
			    if (*pc) {
				prpre = "./";
				gen_matches_files(1, 1, 0);
				prpre = pp;
			    }
			}
		    }
		    if (cc->glob) {
			int ns, pl = strlen(prpre), o;
			char *g = dupstring(cc->glob), pa[MAXPATHLEN];
			char *p2, *p3;
			int ne = noerrs, md = opts[MARKDIRS];

			glob_pre = fpre;
			glob_suf = fsuf;

			noerrs = 1;
			addwhat = -6;
			strcpy(pa, prpre);
			o = strlen(pa);
			opts[MARKDIRS] = OPT_UNSET;

			while (*g) {
			    Lklist l = newlist();

			    while (*g && inblank(*g))
				g++;
			    if (!*g)
				break;
			    for (p = g + 1; *p && !inblank(*p); p++)
				if (*p == '\\' && p[1])
				    p++;
			    sav = *p;
			    *p = '\0';
			    tokenize(g = dupstring(g));
			    if (*g == Equals || *g == Tilde) {
				filesub(&g, 0);
				addnode(l, dupstring(g));
			    } else if (*g == '/')
				addnode(l, dupstring(g));
			    else {
				strcpy(pa + o, g);
				addnode(l, dupstring(pa));
			    }
			    postfork(l, 1);
			    if (full(l) && peekfirst(l)) {
				for (p2 = (char *)peekfirst(l); *p2; p2++)
				    if (itok(*p2))
					break;
				if (!*p2) {
				    if (*g == Equals || *g == Tilde || *g == '/') {
					while ((p2 = (char *)ugetnode(l)))
					    if (strpfx(prpre, p2))
						addmatch(p2 + pl, NULL);
				    } else {
					while ((p2 = p3 = (char *)ugetnode(l))) {
					    for (ns = sf1; *p3 && ns; p3++)
						if (*p3 == '/')
						    ns--;

					    addmatch(p3, NULL);
					}
				    }
				}
			    }
			    pa[o] = '\0';
			    *p = sav;
			    g = p;
			}
			glob_pre = glob_suf = NULL;
			noerrs = ne;
			opts[MARKDIRS] = md;
		    }
		}
	    }
	}
    }
 /* Use tricat() instead of dyncat() to get zalloc()'d memory */
    if (ic == Tilde || ic == Equals) {
	char *orpre = rpre;

	rpre = tricat("", (ic == Tilde) ? "~" : "=", rpre);
	rpl++;
	zsfree(orpre);
    }
    if (!ic && (cc->mask & CC_COMMPATH) && !*ppre && !*psuf) {
	dumphtable(aliastab, -2);
	if (isset(HASHLISTALL))
	    fullhash();
	dumphtable(cmdnamtab, -3);
	if (isset(AUTOCD) && isset(CDABLEVARS))
	    dumphtable(paramtab, -4);
    }
    addwhat = -2;

    if (cc->mask & CC_NAMED) {
	int t0;

	for (t0 = 0; t0 < userdirct; t0++)
	    addmatch(namdirs[t0].name, NULL);
    }
    if (cc->mask & CC_OPTIONS) {
	struct option *o;

	for (o = optns; o->name; o++)
	    addmatch(dupstring(o->name), NULL);
    }
    if (cc->mask & CC_VARS)
	dumphtable(paramtab, -2);
    if (cc->mask & CC_BINDINGS) {
	int t0;

	for (t0 = 0; t0 != ZLECMDCOUNT; t0++)
	    if (*zlecmds[t0].name)
		addmatch(dupstring(zlecmds[t0].name), NULL);
    }
    if (cc->keyvar) {
	char **usr = get_user_var(cc->keyvar);

	if (usr)
	    while (*usr)
		addmatch(*usr++, NULL);
    }
    if (cc->mask & CC_USERS)
	maketildelist();
    if (cc->func) {
	List list;
	char **r;
	int lv = lastval;

	if ((list = getshfunc(cc->func))) {
	    Lklist args = newlist();

	    addnode(args, cc->func);

	    if (delit) {
		sav = os[ooffs];
		os[ooffs] = '\0';
		p = dupstring(os);
		untokenize(p);
		addnode(args, p);
		os[ooffs] = sav;
		p = dupstring(os + ooffs);
		untokenize(p);
		addnode(args, p);
	    } else {
		addnode(args, lpre);
		addnode(args, lsuf);
	    }

	    inzlefunc = 1;
	    doshfuncnoval(list, args, 0);
	    inzlefunc = 0;
	    if ((r = get_user_var("reply")))
		while (*r)
		    addmatch(*r++, NULL);
	}
	lastval = lv;
    }
    if (cc->mask & (CC_JOBS | CC_RUNNING | CC_STOPPED)) {
	int i;
	char *j, *jj;

	for (i = 0; i < MAXJOB; i++)
	    if (jobtab[i].stat & STAT_INUSE) {
		int stopped = jobtab[i].stat & STAT_STOPPED;

		j = jj = dupstring(jobtab[i].procs->text);
		for (; *jj; jj++)
		    if (*jj == ' ') {
			*jj = '\0';
			break;
		    }
		if ((cc->mask & CC_JOBS) || (stopped && (cc->mask & CC_STOPPED))
		    || (!stopped && (cc->mask & CC_RUNNING)))
		    addmatch(j, NULL);
	    }
    }
    if (cc->str) {
	Lklist foo = newlist();
	Lknode n;
	int first = 1, ng = opts[NULLGLOB], oowe = we, oowb = wb;

	opts[NULLGLOB] = OPT_SET;

	zleparse = 1;
	lexsave();
	hungets(cc->str);
	hungets("foo ");	/* KLUDGE! */
	strinbeg();
	noaliases = 1;
	do {
	    ctxtlex();
	    if (tok == ENDINPUT)
		break;
	    if (!first && tokstr && *tokstr)
		addnode(foo, ztrdup(tokstr));
	    first = 0;
	}
	while (tok != ENDINPUT && zleparse);
	noaliases = 0;
	hflush();
	strinend();
	errflag = zleparse = 0;
	lexrestore();
	prefork(foo, 0);
	if (!errflag) {
	    postfork(foo, 1);
	    if (!errflag)
		for (n = firstnode(foo); n; incnode(n))
		    addmatch((char *)n->dat, NULL);
	}
	opts[NULLGLOB] = ng;
	we = oowe;
	wb = oowb;
    }
    if (cc->hpat) {
	Comp compc = NULL;
	char *e, *h, hpatsav;
	int i = curhist - 1, n = cc->hnum, l = lithist;

	if (*(cc->hpat)) {
	    char *thpat = dupstring(cc->hpat);

	    tokenize(thpat);
	    compc = parsereg(thpat);
	}
	if (!n)
	    n = -1;
	lithist = 0;

	while (n-- && (h = qgetevent(i--))) {
	    while (*h) {
		for (e = h; *e && *e != HISTSPACE; e++);
		hpatsav = *e;
		*e = '\0';
		if (*h != '\'' && *h != '"' && *h != '`' && *h != '$' &&
		    (!compc || domatch(h, compc, 0)))
		    addmatch(dupstring(h), NULL);
		if (hpatsav) {
		    *e = hpatsav;
		    h = e + 1;
		} else
		    h = e;
	    }
	}
	lithist = l;
    }
    if ((t = cc->mask & (CC_ARRAYS | CC_INTVARS | CC_ENVVARS | CC_SCALARS |
			 CC_READONLYS | CC_SPECIALS | CC_PARAMS)))
	dumphtable(paramtab, t);
    if ((t = cc->mask & (CC_FUNCS | CC_BUILTINS | CC_DISCMDS | CC_EXCMDS)))
	dumphtable(cmdnamtab, t);
    if ((t = cc->mask & (CC_ALREG | CC_ALGLOB)))
	dumphtable(aliastab, t);

    expl = cc->explain;

    remsuffix = (cc->mask & CC_REMOVE);
    ccsuffix = cc->suffix;

    mpre = ztrdup(lpre);
    msuf = ztrdup(lsuf);
    quotepresuf(&lpre);
    quotepresuf(&lsuf);
    quotepresuf(&fpre);
    quotepresuf(&fsuf);
    quotepresuf(&ppre);
    quotepresuf(&psuf);

    if (empty(matches) || errflag) {
	if (cc->xor && !isp) {
	    errflag = 0;
	    cc = cc->xor;
	    wb = owb;
	    we = owe;
	    cs = ocs;
	    s = dupstring(os);
	    strcpy((char *)line, (char *)ol);
	    goto xorrec;
	}
	strcpy((char *)line, (char *)ol);
	feep();
    } else if (lst == COMP_LIST_COMPLETE)
	listtlist(matches);
    else {
	if (delit) {
	    wb -= compadd;
	    strcpy((char *)line + wb, (char *)line + we);
	    we = cs = wb;
	}
	if (nextnode(firstnode(matches)))
	    do_ambiguous();
	else
	    do_single((char *)peekfirst(matches));
    }

    if (expl && (empty(matches) || nextnode(firstnode(matches)))) {
	int up;

	trashzle();

	clearflag = (isset(USEZLE) && termok &&
		     (isset(ALWAYSLASTPROMPT) && mult == 1)) ||
	    (unset(ALWAYSLASTPROMPT) && mult != 1);

	up = printfmt(expl, countnodes(matches), 1);

	if (clearflag)
	    tcmultout(TCUP, TCMULTUP, up + nlnct);
	fflush(stdout);
    }
    ll = strlen((char *)line);
    if (cs > ll)
	cs = ll;
  compend:
    if (ccmain != &cc_dummy)
	freecompctl(ccmain);
    popheap();
    permalloc();
}

char **get_user_var(nam)	/**/
char *nam;
{
    if (!nam)
	return NULL;
    else if (*nam == '(') {
	char *ptr, *s, **uarr, **aptr;
	int count = 0, nonempty = 0, brk = 0;
	Lklist arrlist = newlist();

	ptr = dupstring(nam);
	s = ptr + 1;
	while (*++ptr) {
	    if (*ptr == '\\' && ptr[1])
		chuck(ptr), nonempty++;
	    else if (*ptr == ',' || inblank(*ptr) || *ptr == ')') {
		if (*ptr == ')')
		    brk++;
		if (nonempty) {
		    *ptr = '\0';
		    count++;
		    if (*s == '\n')
			s++;
		    addnode(arrlist, s);
		}
		s = ptr + 1;
		nonempty = 0;
	    } else
		nonempty++;
	    if (brk)
		break;
	}
	if (!brk || !count)
	    return NULL;
	*ptr = '\0';
	aptr = uarr = (char **)ncalloc(sizeof(char *) * (count + 1));

	while ((*aptr++ = (char *)ugetnode(arrlist)));
	uarr[count] = NULL;
	return uarr;
    } else
	return getaparam(nam);
}

int strbpcmp(a, b)		/**/
const void *a;
const void *b;
{
    char *aa = *((char **)a), *bb = *((char **)b);

    while (*aa && *bb) {
	if (*aa == '\\')
	    aa++;
	if (*bb == '\\')
	    bb++;
	if (*aa != *bb)
	    return (int)(*aa - *bb);
	if (*aa)
	    aa++;
	if (*bb)
	    bb++;
    }
    return 0;
}

void do_ambiguous()
{				/**/
    char **ap, **bp, **cp;
    int p, atend = (cs == we);
    Lknode nod;

    lastambig = 1;

    if (menuarr) {
	freearray(menuarr);
	menuarr = NULL;
	menucmp = 0;
    }
    ap = amatches = (char **)ncalloc(((nmatches = countnodes(matches)) + 1) *
				     sizeof(char *));

    for (nod = firstnode(matches); nod; incnode(nod))
	*ap++ = (char *)getdata(nod);
    *ap = NULL;

    qsort((vptr) amatches, nmatches, sizeof(char *),
	       (int (*)DCLPROTO((const void *, const void *)))strbpcmp);

    for (ap = cp = amatches; *ap; ap++) {
	*cp++ = *ap;
	for (bp = ap; bp[1] && !strcmp(*ap, bp[1]); bp++);
	ap = bp;
    }
    *cp = NULL;

    if ((nmatches = arrlen(amatches)) == 1) {
	lastambig = 0;
	do_single(amatches[0]);
	return;
    }
    complexpect = 0;

    if ((p = (usemenu || ispattern)) || isset(AUTOMENU)) {
	permalloc();
	menuarr = arrdup(amatches);
	heapalloc();
    }
    if (shortest && shortl == 0 && isset(RECEXACT) &&
	(usemenu == 0 || unset(AUTOMENU))) {
	do_single(shortest);
    } else {
	if (p)
	    do_ambig_menu();
	else {
	    if (ab)
		inststrlen(firstm, 1, ab);
	    if (ae && !atend)
		inststrlen(firstm + strlen(firstm) - ae, 0, ae);
	    if (isset(LISTAMBIGUOUS) && (ab || (ae && !atend))) {
		lastambig = 0;
		return;
	    }
	}
	if (unset(NOLISTBEEP))
	    feep();
	if (isset(AUTOLIST) && !amenu)
	    listmatches();
    }
}

int ztat(nam, buf, ls)		/**/
char *nam;
struct stat *buf;
int ls;
{
    char b[MAXPATHLEN], *p;

    for (p = b; *nam; nam++)
	if (*nam == '\\' && nam[1])
	    *p++ = *++nam;
	else
	    *p++ = *nam;
    *p = '\0';

    return ls ? lstat(b, buf) : stat(b, buf);
}

void do_single(str)		/**/
char *str;
{
    int ccs, l, insc = 0, inscs = 0;
    char singlec = ' ';

    addedsuffix = 0;

    if (!menucur) {
	if (ispattern) {
	    cs = we;
	    menupos = wb;
	} else
	    menupos = cs;
	menuwe = (cs == we);
	if (ccsuffix && !(haswhat & HAS_SUFFIX)) {
	    if (*ccsuffix) {
		ccs = cs;
		cs = we;
		inststrlen(ccsuffix, menuwe, -1);
		menuend = cs;
		cs = ccs;
		if (remsuffix)
		    addedsuffix = strlen(ccsuffix);
	    } else
		menuend = we;

	    haswhat |= HAS_SUFFIX;
	} else
	    menuend = we;
    }
    ccs = cs;
    cs = menupos;
    if (menucur)
	l = menulen;
    else if (ispattern)
	l = we - wb;
    else
	l = 0;

    if (l) {
	foredel(l);
	if (menuwe)
	    ccs -= l;
	menuend -= l;
    }
    inststrlen(str, 1, menulen = strlen(str));

    if (menuwe)
	cs = ccs + menulen;
    menuend += menulen;

    if (!(haswhat & HAS_SUFFIX)) {
	if (!(haswhat & HAS_MISC)) {
	    char p[MAXPATHLEN], *ss;
	    struct stat buf;

	    if (ispattern || ic) {
		int ne = noerrs;

		noerrs = 1;

		if (ic) {
		    *p = ic;
		    sprintf(p + 1, "%s%s%s%s", fpre, str, fsuf, psuf);
		} else
		    strcpy(p, str);
		ss = dupstring(p);
		singsub(&ss);
		strcpy(p, ss);

		noerrs = ne;
	    } else
		sprintf(p, "%s%s%s%s%s",
			(prpre && *prpre) ? prpre : "./", fpre, str,
			fsuf, psuf);
	    if (!ztat(p, &buf, 0) && (buf.st_mode & S_IFMT) == S_IFDIR) {
		singlec = '/';
		if (menuwe)
		    addedsuffix = isset(AUTOREMOVESLASH) ? 1 : 0;
	    }
	}
	if (menuend > ll)
	    menuend = ll;
	if (menuend && ((char)line[menuend - 1]) != singlec)
	    if (!menucur || !line[menuend]) {
		ccs = cs;
		cs = menuend;
		inststrlen((char *)&singlec, 1, 1);
		insc = 1;
		inscs = cs;
		if (!menuwe)
		    cs = ccs;
	    } else
		line[menuend] = (unsigned char)singlec;
    }
    if (isset(ALWAYSTOEND) || menuwe)
	cs = menuend + !(haswhat & HAS_SUFFIX);
    if (menucmp && singlec == ' ' && !(haswhat & HAS_SUFFIX)) {
	if (insc) {
	    ccs = cs;
	    cs = inscs;
	    backdel(1);
	    if (ccs != inscs)
	      cs = ccs;
	}
	else
	    cs--;
    }
}

void do_ambig_menu()
{				/**/
    menucmp = 1;
    menucur = NULL;
    do_single(*menuarr);
    menucur = menuarr;
}

int strpfx(s, t)		/**/
char *s;
char *t;
{
    while (*s && *s == *t)
	s++, t++;
    return !*s;
}

int strsfx(s, t)		/**/
char *s;
char *t;
{
    int ls = strlen(s), lt = strlen(t);

    if (ls <= lt)
	return !strcmp(t + lt - ls, s);
    return 0;
}

int pfxlen(s, t)		/**/
char *s;
char *t;
{
    int i = 0;

    while (*s && *s == *t)
	s++, t++, i++;
    return i;
}

int sfxlen(s, t)		/**/
char *s;
char *t;
{
    if (*s && *t) {
	int i = 0;
	char *s2 = s + strlen(s) - 1, *t2 = t + strlen(t) - 1;

	while (s2 >= s && t2 >= t && *s2 == *t2)
	    s2--, t2--, i++;

	return i;
    } else
	return 0;
}

int printfmt(fmt, n, dopr)	/**/
char *fmt;
int n;
int dopr;
{
    char *p = fmt, nc[14];
    int l = 1, cc = 0;

    for (; *p; p++) {
	if (*p == '%')
	    if (*++p)
		switch (*p) {
		case '%':
		    if (dopr)
			putchar('%');
		    cc++;
		    break;
		case 'n':
		    sprintf(nc, "%d", n);
		    if (dopr)
			printf(nc);
		    cc += strlen(nc);
	    } else
		break;
	else {
	    cc++;
	    if (*p == '\n') {
		l += 1 + (cc / columns);
		cc = 0;
	    }
	    if (dopr)
		putchar(*p);
	}
    }
    if (dopr)
	putchar('\n');

    return l + (cc / columns);
}

void listmatches()
{				/**/
    int longest = 1, fct, fw = 0, colsz, t0, t1, ct, up, cl;
    int off, boff;
    int of = (isset(LISTTYPES) && !(haswhat & HAS_MISC));
    char **arr, **ap, sav;

    off = ispattern && ppre && *ppre &&
	!(haswhat & (HAS_MISC | HAS_PATHPAT)) ? strlen(ppre) : 0;
    boff = ispattern && psuf && *psuf &&
	!(haswhat & (HAS_MISC | HAS_PATHPAT)) ? strlen(psuf) : 0;

    trashzle();
    ct = nmatches;

    clearflag = (isset(USEZLE) && termok &&
		 (isset(ALWAYSLASTPROMPT) && mult == 1)) ||
	(unset(ALWAYSLASTPROMPT) && mult != 1);

    arr = amatches;

    for (ap = arr; *ap; ap++)
	if ((cl = strlen(*ap + off) - boff +
	     (ispattern ? 0 :
	      (!(haswhat & HAS_MISC) ? fpl + fsl : lpl + lsl))) > longest)
	    longest = cl;
    if (of)
	longest++;

    fct = (columns - 1) / (longest + 2);
    if (fct == 0)
	fct = 1;
    else
	fw = (columns - 1) / fct;
    colsz = (ct + fct - 1) / fct;

    up = colsz + nlnct - clearflag;

    if (expl)
	up += printfmt(expl, ct, 0);

    if ((listmax && ct > listmax) || (!listmax && up >= lines)) {
	fprintf(stdout, "zsh: do you wish to see all %d possibilities? ", ct);
	fflush(stdout);
	if (getquery() != 'y') {
	    if (clearflag) {
		tcmultout(TCUP, TCMULTUP, 1);
		if (tccan(TCCLEAREOD))
		    tcout(TCCLEAREOD);
		tcmultout(TCUP, TCMULTUP, nlnct);
	    }
	    return;
	}
	if (clearflag) {
	    tcout(TCUP);
	    if (tccan(TCCLEAREOD))
		tcout(TCCLEAREOD);
	}
    }
    if (expl)
	printfmt(expl, ct, 1);

    for (t1 = 0; t1 != colsz; t1++) {
	ap = arr + t1;
	if (of) {
	    while (*ap) {
		int t2 = ispattern ? strlen(*ap) :
		strlen(*ap + off) - boff + 1 + fpl + fsl;
		char pbuf[MAXPATHLEN], *pb;
		struct stat buf;

		if (ispattern) {
		    sav = ap[0][t2 - boff];
		    ap[0][t2 - boff] = '\0';
		    printf("%s", *ap + off);
		    ap[0][t2 - boff] = sav;
		    pb = *ap;
		    t2 -= off + boff - 1;
		} else {
		    printf("%s%s%s", fpre, *ap, fsuf);
		    sprintf(pb = pbuf, "%s%s%s%s",
			    (prpre && *prpre) ? prpre : "./", fpre, *ap, fsuf);
		}
		if (ztat(pb, &buf, 1))
		    putchar(' ');
		else
		    putchar(file_type(buf.st_mode));
		for (t0 = colsz; t0 && *ap; t0--, ap++);
		if (*ap)
		    for (; t2 < fw; t2++)
			putchar(' ');
	    }
	} else
	    while (*ap) {
		int t2 = ispattern ? strlen(*ap) :
		strlen(*ap + off) - boff;

		if (ispattern) {
		    sav = ap[0][t2 - boff];
		    ap[0][t2 - boff] = '\0';
		    printf("%s", *ap + off);
		    ap[0][t2 - boff] = sav;
		    t2 -= off + boff;
		} else if (!(haswhat & HAS_MISC)) {
		    printf("%s%s%s", fpre, *ap, fsuf);
		    t2 += fpl + fsl;
		} else {
		    printf("%s%s%s", lpre, *ap, lsuf);
		    t2 += lpl + lsl;
		}
		for (t0 = colsz; t0 && *ap; t0--, ap++);
		if (*ap)
		    for (; t2 < fw; t2++)
			putchar(' ');
	    }
	if (t1 != colsz - 1 || !clearflag)
	    putchar('\n');
    }
    if (clearflag)
	if (up < lines)
	    tcmultout(TCUP, TCMULTUP, up);
	else
	    clearflag = 0, putchar('\n');

    expl = NULL;

    fflush(stdout);
}

void listtlist(l)		/**/
Lklist l;
{
    int n = countnodes(l), nm = nmatches;
    char **am = amatches, **ap, **bp, **cp;
    Lknode nod;

    nmatches = n;

    amatches = ap = (char **)ncalloc((n + 1) * sizeof(char *));

    for (nod = firstnode(l); nod; incnode(nod))
	*ap++ = (char *)getdata(nod);
    *ap = NULL;

    qsort((vptr) amatches, nmatches, sizeof(char *),
	       (int (*)DCLPROTO((const void *, const void *)))strpcmp);

    for (ap = cp = amatches; *ap; ap++) {
	*cp++ = *ap;
	for (bp = ap; bp[1] && !strcmp(*ap, bp[1]); bp++);
	ap = bp;
    }
    *cp = NULL;

    listmatches();

    amatches = am;
    nmatches = nm;
}

void listlist(l)		/**/
Lklist l;
{
    int hw = haswhat, ip = ispattern;
    char *lp = lpre, *ls = lsuf;

    haswhat = HAS_MISC;
    ispattern = 0;
    lpre = lsuf = "";

    listtlist(l);

    lpre = lp;
    lsuf = ls;
    ispattern = ip;
    haswhat = hw;
}

void selectlist(l)		/**/
Lklist l;
{
    int longest = 1, fct, fw = 0, colsz, t0, t1, ct;
    Lknode n;
    char **arr, **ap;

    trashzle();
    ct = countnodes(l);
    ap = arr = (char **)alloc((countnodes(l) + 1) * sizeof(char **));

    for (n = (Lknode) firstnode(l); n; incnode(n))
	*ap++ = (char *)getdata(n);
    *ap = NULL;
    for (ap = arr; *ap; ap++)
	if (strlen(*ap) > longest)
	    longest = strlen(*ap);
    t0 = ct;
    longest++;
    while (t0)
	t0 /= 10, longest++;
    fct = (columns - 1) / (longest + 3);	/* to compensate for added ')' */
    if (fct == 0)
	fct = 1;
    else
	fw = (columns - 1) / fct;
    colsz = (ct + fct - 1) / fct;
    for (t1 = 0; t1 != colsz; t1++) {
	ap = arr + t1;
	do {
	    int t2 = strlen(*ap) + 2, t3;

	    fprintf(stderr, "%d) %s", t3 = ap - arr + 1, *ap);
	    while (t3)
		t2++, t3 /= 10;
	    for (; t2 < fw; t2++)
		fputc(' ', stderr);
	    for (t0 = colsz; t0 && *ap; t0--, ap++);
	}
	while (*ap);
	fputc('\n', stderr);
    }

 /* Below is a simple attempt at doing it the Korn Way..
       ap = arr;
       t0 = 0;
       do
       {
       t0++;
       fprintf(stderr,"%d) %s\n",t0,*ap);
       ap++;
       }
       while (*ap);*/
    fflush(stderr);
}

int doexpandhist()
{				/**/
    unsigned char *cc, *ce, oc = ' ', ooc = ' ';
    int t0, oldcs, oldll;

    for (cc = line, ce = line + ll; cc < ce; ooc = oc, oc = *cc++)
	if (*cc == '\\' && cc[1])
	    cc++;
	else if ((*cc == bangchar && oc != '>' && (oc != '&' || ooc != '>')) ||
		 (*cc == hatchar && *line == hatchar && cc != line))
	    break;
    if (*cc == bangchar && cc[1] == '"')
	return 3;
    if (cc == ce)
	return 2;
    oldcs = cs;
    oldll = ll;
    zleparse = 1;
    lexsave();
    hungets(UTOSCP(line));
    strinbeg();
    pushheap();
    ll = cs = 0;
    for (;;) {
	t0 = hgetc();
	if (lexstop)
	    break;
	spaceinline(1);
	line[cs++] = t0;
    }
    hflush();
    popheap();
    strinend();
    errflag = zleparse = 0;
    t0 = histdone;
    lexrestore();
    line[ll = cs] = '\0';
    if (ll == oldll)
	cs = oldcs;
    return t0;
}

void magicspace()
{				/**/
    c = ' ';
    selfinsert();
    doexpandhist();
}

void expandhistory()
{				/**/
    if (!doexpandhist())
	feep();
}

static int cmdwb, cmdwe;

char *getcurcmd()
{				/**/
    int curlincmd;
    char *s = NULL;

    zleparse = 1;
    lexsave();
    hungets(" ");		/* KLUDGE! */
    hungets(UTOSCP(line));
    strinbeg();
    pushheap();
    do {
	curlincmd = incmdpos;
	ctxtlex();
	if (tok == ENDINPUT)
	    break;
	if (tok == STRING && curlincmd) {
	    zsfree(s);
	    s = ztrdup(tokstr);
	    cmdwb = ll - wordbeg;
	    cmdwe = ll + 1 - inbufct;
	}
    }
    while (tok != ENDINPUT && zleparse);
    hflush();
    popheap();
    strinend();
    errflag = zleparse = 0;
    lexrestore();
    return s;
}

void processcmd()
{				/**/
    char *s, *t;

    s = getcurcmd();
    if (!s) {
	feep();
	return;
    }
    t = zlecmds[bindk].name;
    mult = 1;
    permalloc();
    pushline();
    lastalloc();
    sizeline(strlen(s) + strlen(t) + 1);
    strcpy((char *)line, t);
    strcat((char *)line, " ");
    cs = ll = strlen((char *)line);
    inststr(s);
    zsfree(s);
    done = 1;
}

void expandcmdpath()
{				/**/
    int oldcs = cs, na = noaliases;
    char *s, *str;

    noaliases = 1;
    s = getcurcmd();
    noaliases = na;
    if (!s || cmdwb < 0 || cmdwe < cmdwb) {
	feep();
	return;
    }
    str = findcmd(s);
    zsfree(s);
    if (!str) {
	feep();
	return;
    }
    cs = cmdwb;
    foredel(cmdwe - cmdwb);
    spaceinline(strlen(str));
    strncpy((char *)line + cs, str, strlen(str));
    cs = oldcs;
    if (cs >= cmdwe - 1)
	cs += cmdwe - cmdwb + strlen(str);
    if (cs > ll)
	cs = ll;
    zsfree(str);
}

/* Extra function added by AR Iano-Fletcher. */
/* This is a expand/complete in the vein of wash. */

void expandorcompleteprefix()
{				/**/
 /* global c is the current character typed. */
    int csafe = c;

 /* insert a space and backspace. */
    c = ' ';
    selfinsert();		/* insert the extra character */
    forwardchar();		/* move towards beginning */

 /* do the expansion/completion. */
    c = csafe;
    expandorcomplete();		/* complete. */

 /* remove the inserted space. */
    backwardchar();		/* move towards ends */
    deletechar();		/* delete the added space. */
}
