/*
 *
 * lex.c - lexical analysis
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

/* lexical state */

static int dbparens, in_brace_param = 0;
int len = 0, bsiz = 256;
char *bptr;

struct lexstack {
    struct lexstack *next;

    int incmdpos;
    int incond;
    int incasepat;
    int dbparens;
    int in_brace_param;
    int alstat;
    char *hlastw;
    int isfirstln;
    int isfirstch;
    int histremmed;
    int histdone;
    int spaceflag;
    int stophist;
    int lithist;
    int alstackind;
    int hlinesz;
    char *hline;
    char *hptr;
    char *tokstr;
    char *bptr;
    int bsiz;

    unsigned char *cstack;
    int csp;
};

static struct lexstack *lstack = NULL;

/* save the lexical state */

/* is this a hack or what? */

void lexsave()
{				/**/
    struct lexstack *ls;

    ls = (struct lexstack *)malloc(sizeof(struct lexstack));

    ls->incmdpos = incmdpos;
    ls->incond = incond;
    ls->incasepat = incasepat;
    ls->dbparens = dbparens;
    ls->in_brace_param = in_brace_param;
    ls->alstat = alstat;
    ls->alstackind = alstackind;
    ls->isfirstln = isfirstln;
    ls->isfirstch = isfirstch;
    ls->histremmed = histremmed;
    ls->histdone = histdone;
    ls->spaceflag = spaceflag;
    ls->stophist = stophist;
    ls->lithist = lithist;
    ls->hline = chline;
    ls->hptr = hptr;
    ls->hlastw = hlastw;
    ls->hlinesz = hlinesz;
    ls->cstack = cmdstack;
    ls->csp = cmdsp;
    cmdstack = (unsigned char *)zalloc(256);
    ls->tokstr = tokstr;
    ls->bptr = bptr;
    ls->bsiz = bsiz;
    in_brace_param = 0;
    cmdsp = 0;
    inredir = 0;

    ls->next = lstack;
    lstack = ls;
}

/* restore lexical state */

void lexrestore()
{				/**/
    struct lexstack *ln;

    if (!lstack) {
	zerr("lexrestore without lexsave", NULL, 0);
	return;
    }
    incmdpos = lstack->incmdpos;
    incond = lstack->incond;
    incasepat = lstack->incasepat;
    dbparens = lstack->dbparens;
    in_brace_param = lstack->in_brace_param;
    alstat = lstack->alstat;
    isfirstln = lstack->isfirstln;
    isfirstch = lstack->isfirstch;
    histremmed = lstack->histremmed;
    histdone = lstack->histdone;
    spaceflag = lstack->spaceflag;
    stophist = lstack->stophist;
    lithist = lstack->lithist;
    chline = lstack->hline;
    hptr = lstack->hptr;
    hlastw = lstack->hlastw;
    if (cmdstack)
	free(cmdstack);
    cmdstack = lstack->cstack;
    cmdsp = lstack->csp;
    tokstr = lstack->tokstr;
    bptr = lstack->bptr;
    bsiz = lstack->bsiz;
    clearalstack();
    alstackind = lstack->alstackind;
    hlinesz = lstack->hlinesz;
    lexstop = errflag = 0;

    ln = lstack->next;
    free(lstack);
    lstack = ln;
}

void yylex()
{				/**/
    if (tok == LEXERR)
	return;
    do
	tok = gettok();
    while (tok != ENDINPUT && exalias());
    if (tok != NEWLIN)
	isnewlin = 0;
    else
	isnewlin = (inbufct) ? -1 : 1;
    if (tok == SEMI || tok == NEWLIN)
	tok = SEPER;
}

void ctxtlex()
{				/**/
    static int oldpos;

    yylex();
    switch (tok) {
    case SEPER:
    case NEWLIN:
    case SEMI:
    case DSEMI:
    case AMPER:
    case INPAR:
    case INBRACE:
    case DBAR:
    case DAMPER:
    case BAR:
    case BARAMP:
    case INOUTPAR:
    case DO:
    case THEN:
    case ELIF:
    case ELSE:
	incmdpos = 1;
	break;
    case STRING:		/* case ENVSTRING: */
    case ENVARRAY:
    case OUTPAR:
    case CASE:
	incmdpos = 0;
	break;
    }
    if (IS_REDIROP(tok) || tok == FOR || tok == FOREACH || tok == SELECT) {
	inredir = 1;
	oldpos = incmdpos;
	incmdpos = 0;
    } else if (inredir) {
	incmdpos = oldpos;
	inredir = 0;
    }
}

#define LX1_BKSLASH 0
#define LX1_COMMENT 1
#define LX1_NEWLIN 2
#define LX1_SEMI 3
#define LX1_BANG 4
#define LX1_AMPER 5
#define LX1_BAR 6
#define LX1_INPAR 7
#define LX1_OUTPAR 8
#define LX1_INBRACE 9
#define LX1_OUTBRACE 10
#define LX1_INBRACK 11
#define LX1_OUTBRACK 12
#define LX1_INANG 13
#define LX1_OUTANG 14
#define LX1_OTHER 15

#define LX2_BREAK 0
#define LX2_OUTPAR 1
#define LX2_BAR 2
#define LX2_STRING 3
#define LX2_INBRACK 4
#define LX2_OUTBRACK 5
#define LX2_TILDE 6
#define LX2_INPAR 7
#define LX2_INBRACE 8
#define LX2_OUTBRACE 9
#define LX2_OUTANG 10
#define LX2_INANG 11
#define LX2_EQUALS 12
#define LX2_BKSLASH 13
#define LX2_QUOTE 14
#define LX2_DQUOTE 15
#define LX2_BQUOTE 16
#define LX2_OTHER 17

unsigned char lexact1[256], lexact2[256], lextok2[256];

void initlextabs()
{				/**/
    int t0;
    static char *lx1 = "\\q\n;!&|(){}[]<>xx";
    static char *lx2 = "x)|$[]~({}><=\\\'\"`x";

    for (t0 = 0; t0 != 256; t0++) {
	lexact1[t0] = LX1_OTHER;
	lexact2[t0] = LX2_OTHER;
	lextok2[t0] = t0;
    }
    for (t0 = 0; lx1[t0]; t0++)
	if (lx1[t0] != 'x')
	    lexact1[(int)lx1[t0]] = t0;
    for (t0 = 0; lx2[t0]; t0++)
	if (lx2[t0] != 'x')
	    lexact2[(int)lx2[t0]] = t0;
    lexact2[';'] = LX2_BREAK;
    lexact2['&'] = LX2_BREAK;
    lextok2[','] = Comma;
    lextok2['*'] = Star;
    lextok2['?'] = Quest;
    lextok2['{'] = Inbrace;
    lextok2['['] = Inbrack;
    lextok2['$'] = String;
}

/* initialize lexical state */

void lexinit()
{				/**/
    incond = incasepat = nocorrect =
    dbparens = alstat = lexstop = in_brace_param = 0;
    incmdpos = 1;
    tok = ENDINPUT;
    if (isset(EXTENDEDGLOB)) {
	lextok2['#'] = Pound;
	lextok2['^'] = Hat;
    } else {
	lextok2['#'] = '#';
	lextok2['^'] = '^';
    }
}

/* add a char to the string buffer */

void add(c)			/**/
int c;
{
    *bptr++ = c;
    if (bsiz == ++len) {
	int newbsiz;

	newbsiz = bsiz * 8;
	while (newbsiz < inbufct)
	    newbsiz *= 2;
	bptr = len + (tokstr = (char *)hrealloc(tokstr, bsiz, newbsiz));
	bsiz = newbsiz;
    }
}

SPROTO(void unadd, (void));

static void unadd()
{
    bptr--;
    len--;
}

int gettok()
{				/**/
    int bct = 0, pct = 0, brct = 0;
    int c, d, intpos = 1;
    int peekfd = -1, peek, ninbracks, intick;

  beginning:
    hlastw = NULL;
    tokstr = NULL;
    parbegin = -1;
    while (iblank(c = hgetc()) && !lexstop);
    isfirstln = 0;
    wordbeg = inbufct;
    hwbegin();
    hwaddc(c);
    if (dbparens) {
	hungetc(c);
	c = '\"';
    } else if (idigit(c)) {	/* handle 1< foo */
	d = hgetc();
	hungetc(d);
	lexstop = 0;
	if (d == '>' || d == '<') {
	    peekfd = c - '0';
	    c = hgetc();
	}
    }
/* chars in initial position in word */

    if (c == hashchar &&
	(isset(INTERACTIVECOMMENTS) ||
	 (!zleparse && (!interact || unset(SHINSTDIN) || strin)))) {
    /* History is handled here to prevent extra newlines
		 * being inserted into the history.
		 *
		 * Also ignore trailing spaces to prevent history from
		 * changing. If trailing spaces are not ignored then
		 * each time a comment inside a command (a 'for' loop
		 * is a good example) is seen an extra space is appended
		 * to the end of the comment causing a new history entry
		 * even if HISTIGNOREDUPS is set.
		 */
	int nsp = 0;		/* number of contiguos spaces */

	while ((c = hgetch()) != '\n' && !lexstop) {
	    if (c == ' ')
		++nsp;
	    else {
		while (nsp) {
		    hwaddc(' ');
		    --nsp;
		}
		hwaddc(c);
	    }
	}
	if (errflag)
	    peek = LEXERR;
	else {
	    hwadd();
	    hwbegin();
	    hwaddc('\n');
	    peek = NEWLIN;
	}
	return peek;
    }
    if (lexstop)
	return (errflag) ? LEXERR : ENDINPUT;
    switch (lexact1[(unsigned char)c]) {
    case LX1_BKSLASH:
	d = hgetc();
	if (d == '\n')
	    goto beginning;
	hungetc(d);
	break;
    case LX1_NEWLIN:
	return NEWLIN;
    case LX1_SEMI:
	d = hgetc();
	if (d != ';') {
	    hungetc(d);
	    return SEMI;
	}
	return DSEMI;
    case LX1_BANG:
	d = hgetc();
	hungetc(d);
	if (!inblank(d))
	    break;
	if (incmdpos || incond)
	    return BANG;
	break;
    case LX1_AMPER:
	d = hgetc();
	if (d != '&') {
	    hungetc(d);
	    return AMPER;
	}
	return DAMPER;
    case LX1_BAR:
	d = hgetc();
	if (d == '|')
	    return DBAR;
	else if (d == '&')
	    return BARAMP;
	hungetc(d);
	return BAR;
    case LX1_INPAR:
	d = hgetc();
	if (d == '(' && incmdpos) {
	    dbparens = 1;
	    return DINPAR;
	} else if (d == ')')
	    return INOUTPAR;
	hungetc(d);
	if (!(incond == 1 || incmdpos))
	    break;
	return INPAR;
    case LX1_OUTPAR:
	return OUTPAR;
    case LX1_INBRACE:
	if (!incmdpos)
	    break;
	return INBRACE;
    case LX1_OUTBRACE:
	return OUTBRACE;
    case LX1_INBRACK:
	if (!incmdpos)
	    break;
	d = hgetc();
	if (d == '[')
	    return DINBRACK;
	hungetc(d);
	break;
    case LX1_OUTBRACK:
	if (!incond)
	    break;
	d = hgetc();
	if (d == ']')
	    return DOUTBRACK;
	hungetc(d);
	break;
    case LX1_INANG:
	d = hgetc();
	if ((!incmdpos && d == '(') || incasepat) {
	    hungetc(d);
	    break;
	} else if (idigit(d) || d == '-' || d == '>') {
	    int tbs = 256, n = 0, nc;
	    char *tbuf, *tbp, *ntb;

	    tbuf = tbp = (char *)zalloc(tbs);
	    hungetc(d);

	    while ((nc = hgetc()) && !lexstop) {
		if (!idigit(nc) && nc != '-')
		    break;
		*tbp++ = (char)nc;
		if (++n == tbs) {
		    ntb = (char *)realloc(tbuf, tbs *= 2);
		    tbp += ntb - tbuf;
		    tbuf = ntb;
		}
	    }
	    if (nc == '>' && !lexstop) {
		lexstop = 0;
		hungetc(nc);
		while (n--)
		    hungetc(*--tbp);
		zfree(tbuf, tbs);
		break;
	    }
	    lexstop = 0;
	    if (nc)
		hungetc(nc);
	    while (n--)
		hungetc(*--tbp);
	    zfree(tbuf, tbs);
	    peek = INANG;
	} else if (d == '<') {
	    int e = hgetc();

	    if (e == '(') {
		hungetc(e);
		hungetc(d);
		peek = INANG;
	    } else if (e == '<')
		peek = TRINANG;
	    else if (e == '-')
		peek = DINANGDASH;
	    else {
		hungetc(e);
		peek = DINANG;
	    }
	} else if (d == '&')
	    peek = INANGAMP;
	else {
	    peek = INANG;
	    hungetc(d);
	}
	tokfd = peekfd;
	return peek;
    case LX1_OUTANG:
	d = hgetc();
	if (d == '(') {
	    hungetc(d);
	    break;
	} else if (d == '&') {
	    d = hgetc();
	    if (d == '!')
		peek = OUTANGAMPBANG;
	    else {
		hungetc(d);
		peek = OUTANGAMP;
	    }
	} else if (d == '!')
	    peek = OUTANGBANG;
	else if (d == '>') {
	    d = hgetc();
	    if (d == '&') {
		d = hgetc();
		if (d == '!')
		    peek = DOUTANGAMPBANG;
		else {
		    hungetc(d);
		    peek = DOUTANGAMP;
		}
	    } else if (d == '!')
		peek = DOUTANGBANG;
	    else if (d == '(') {
		hungetc(d);
		hungetc('>');
		peek = OUTANG;
	    } else {
		hungetc(d);
		peek = DOUTANG;
		if (isset(NOCLOBBER))
		    hwaddc('!');
	    }
	} else {
	    hungetc(d);
	    peek = OUTANG;
	    if (!incond && isset(NOCLOBBER))
		hwaddc('!');
	}
	tokfd = peekfd;
	return peek;
    }

/* we've started a string, now get the rest of it, performing
		tokenization */

    peek = STRING;
    len = 0;
    bptr = tokstr = (char *)ncalloc(bsiz = 256);
    for (;;) {
	int act;
	int e;
	int endchar;

	if (inblank(c) && !bct && !pct)
	    act = LX2_BREAK;
	else {
	    act = lexact2[(unsigned char)c];
	    c = lextok2[(unsigned char)c];
	}
	switch (act) {
	case LX2_BREAK:
	    if (!in_brace_param)
		goto brk;
	    break;
	case LX2_OUTPAR:
	    if (!pct)
		goto brk;
	    c = Outpar;
	    pct--;
	    break;
	case LX2_BAR:
	    if (!pct && !incasepat)
		goto brk;
	    c = Bar;
	    break;
	case LX2_STRING:
	    e = hgetc();
	    if (e == '[') {
		cmdpush(CS_MATHSUBST);
		add(String);
		add(Inbrack);
		ninbracks = 1;
		while (ninbracks && (c = hgetc()) && !lexstop) {
		    if (c == '[')
			ninbracks++;
		    else if (c == ']')
			ninbracks--;
		    if (ninbracks)
			add(c);
		}
		c = Outbrack;
		cmdpop();
	    } else if (e == '(') {
		add(String);
		if (skipcomm()) {
		    peek = LEXERR;
		    goto brk;
		}
		c = Outpar;
	    } else {
		if (e == '{')
		    in_brace_param = 1;
		hungetc(e);
	    }
	    break;
	case LX2_INBRACK:
	    brct++;
	    break;
	case LX2_OUTBRACK:
	    if (incond && !brct)
		goto brk;
	    brct--;
	    c = Outbrack;
	    break;
	case LX2_TILDE:	/* if (intpos) */
	    c = Tilde;
	    break;
	case LX2_INPAR:
	    e = hgetc();
	    hungetc(e);
	    if (e == ')' ||
		(incmdpos && !brct && peek != ENVSTRING))
		goto brk;
	    pct++;
	    c = Inpar;
	    break;
	case LX2_INBRACE:
	    bct++;
	    break;
	case LX2_OUTBRACE:
	    if (!bct)
		goto brk;
	    if (!--bct && in_brace_param)
		in_brace_param = 0;
	    c = Outbrace;
	    break;
	case LX2_OUTANG:
	    e = hgetc();
	    if (e != '(') {
		hungetc(e);
		goto brk;
	    }
	    add(Outang);
	    if (skipcomm()) {
		peek = LEXERR;
		goto brk;
	    }
	    c = Outpar;
	    break;
	case LX2_INANG:
	    e = hgetc();
	    if (!(idigit(e) || e == '-' || e == '>' || e == '(' || e == ')')) {
		hungetc(e);
		goto brk;
	    }
	    c = Inang;
	    if (e == '(') {
		add(c);
		if (skipcomm()) {
		    peek = LEXERR;
		    goto brk;
		}
		c = Outpar;
	    } else if (e == ')')
		hungetc(e);
	    else {
		add(c);
		c = e;
		while (c != '>' && !lexstop)
		    add(c), c = hgetc();
		c = Outang;
	    }
	    break;
	case LX2_EQUALS:
	    if (intpos) {
		e = hgetc();
		if (e != '(') {
		    hungetc(e);
		    c = Equals;
		} else {
		    add(Equals);
		    if (skipcomm()) {
			peek = LEXERR;
			goto brk;
		    }
		    c = Outpar;
		}
	    } else if (peek != ENVSTRING && incmdpos && !bct) {
		e = hgetc();
		if (e == '(' && incmdpos) {
		    *bptr = '\0';
		    return ENVARRAY;
		}
		hungetc(e);
		peek = ENVSTRING;
		intpos = 2;
	    } else
		c = Equals;
	    break;
	case LX2_BKSLASH:
	    c = hgetc();
	    if (c == '\n') {
		c = hgetc();
		continue;
	    }
	    add(Bnull);
	    add(c);
	    c = hgetc();
	    continue;
	case LX2_QUOTE:
	    add(Snull);

	/* we add the Nularg to prevent this:

				echo $PA'TH'

				from printing the path. */

	    cmdpush(CS_QUOTE);
	    for (;;) {
		while ((c = hgetc()) != '\'' && !lexstop) {
		    if (isset(CSHJUNKIEQUOTES) && c == '\n') {
			if (bptr[-1] == '\\')
			    unadd();
			else
			    break;
		    }
		    add(c);
		}
		if (c != '\'') {
		    zerr("unmatched \'", NULL, 0);
		    peek = LEXERR;
		    cmdpop();
		    goto brk;
		}
		e = hgetc();
		if (e != '\'' || unset(RCQUOTES))
		    break;
		add(c);
	    }
	    cmdpop();
	    hungetc(e);
	    c = Snull;
	    break;
	case LX2_DQUOTE:
	    add(Dnull);
	    cmdpush(dbparens ? CS_MATH : CS_DQUOTE);	/* math or dquote */
	    endchar = dbparens ? ')' : '"';
	    intick = 0;
	    while (((c = hgetc()) != endchar || (dbparens && pct > 0))
		   && !lexstop)
		if (c == '\\') {
		    c = hgetc();
		    if (c != '\n') {
			add(c == '$' || c == '\\' ||
			    c == '\"' || c == '`' ? Bnull : '\\');
			add(c);
		    }
		} else {
		    if (isset(CSHJUNKIEQUOTES) && c == '\n') {
			if (bptr[-1] == '\\')
			    unadd();
			else
			    break;
		    }
		    if (c == '$' && !intick) {
			e = hgetc();
			if (e == '(') {
			    add(Qstring);
			    if (skipcomm()) {
				peek = LEXERR;
				cmdpop();
				goto brk;
			    }
			    c = Outpar;
			} else if (e == '[') {
			    add(String);
			    add(Inbrack);
			    while ((c = hgetc()) != ']' && !lexstop)
				add(c);
			    c = Outbrack;
			} else {
			    c = Qstring;
			    hungetc(e);
			}
		    } else if (c == '`') {
			c = Qtick;
			intick = !intick;
		    } else if (dbparens) {
			if (c == '(')
			    pct++;
			else if (c == ')')
			    pct--;
		    }
		    add(c);
		}
	    cmdpop();
	    if (c != endchar || (dbparens && (pct || (c = hgetc()) != ')'))) {
		if (!dbparens)
		    zerr("unmatched \"", NULL, 0);
		peek = LEXERR;
		goto brk;
	    }
	    if (dbparens) {
		dbparens = 0;
		add(Dnull);
		*bptr = '\0';
		return DOUTPAR;
	    }
	    c = Dnull;
	    break;
	case LX2_BQUOTE:
	    add(Tick);
	    cmdpush(CS_BQUOTE);
	    parbegin = inbufct;
	    while ((c = hgetc()) != '`' && !lexstop)
		if (c == '\\') {
		    c = hgetc();
		    if (c != '\n') {
			add(c == '`' || c == '\\' || c == '$' ? Bnull : '\\');
			add(c);
		    }
		} else {
		    if (isset(CSHJUNKIEQUOTES) && c == '\n') {
			if (bptr[-1] == '\\')
			    unadd();
			else
			    break;
		    }
		    add(c);
		}
	    cmdpop();
	    if (c != '`') {
		if (!zleparse)
		    zerr("unmatched `", NULL, 0);
		peek = LEXERR;
		goto brk;
	    }
	    c = Tick;
	    parbegin = -1;
	    break;
	}
	add(c);
	c = hgetc();
	if (intpos)
	    intpos--;
	if (lexstop)
	    break;
    }
  brk:
    hungetc(c);
    *bptr = '\0';
    return peek;
}

/* expand aliases, perhaps */

int exalias()
{				/**/
    struct alias *an;
    char *s, *t;

    s = yytext = hwadd();
    for (t = s; *t && *t != HISTSPACE; t++);
    if (!*t)
	t = NULL;
    else
	*t = '\0';
    if (interact && isset(SHINSTDIN) && !strin && !incasepat && tok == STRING &&
	(isset(CORRECTALL) || (isset(CORRECT) && incmdpos)) && !nocorrect)
	spckword(&tokstr, &s, &t, !incmdpos, 1);
    if (zleparse && !alstackind) {
	int zp = zleparse;

	gotword(s);
	if (zp && !zleparse) {
	    if (t)
		*t = HISTSPACE;
	    return 0;
	}
    }
    an = noaliases ? NULL : (struct alias *)gethnode(s, aliastab);
    if (t)
	*t = HISTSPACE;
    if (alstackind != MAXAL && an && !an->inuse)
	if (!(an->cmd && !incmdpos && alstat != ALSTAT_MORE)) {
	    if (an->cmd < 0) {
		tok = DO - an->cmd - 1;
		return 0;
	    } else {
		an->inuse = 1;
		hungets(ALPOPS);
		hungets((alstack[alstackind++] = an)->text);
		alstat = 0;
	    /* remove from history if it begins with space */
		if (isset(HISTIGNORESPACE) && an->text[0] == ' ')
		    remhist();
		lexstop = 0;
		return 1;
	    }
	}
    return 0;
}

/* skip (...) */

int skipcomm()
{				/**/
    int pct = 1, c;

    cmdpush(CS_CMDSUBST);
    parbegin = inbufct;
    c = Inpar;
    do {
	add(c);
	c = hgetc();
	if (itok(c) || lexstop)
	    break;
	else if (c == '(')
	    pct++;
	else if (c == ')')
	    pct--;
	else if (c == '\\') {
	    add(c);
	    c = hgetc();
	} else if (c == '\'') {
	    add(c);
	    while ((c = hgetc()) != '\'' && !lexstop)
		add(c);
	} else if (c == '\"') {
	    add(c);
	    while ((c = hgetc()) != '\"' && !lexstop)
		if (c == '\\') {
		    add(c);
		    add(hgetc());
		} else
		    add(c);
	} else if (c == '`') {
	    add(c);
	    while ((c = hgetc()) != '`' && !lexstop)
		if (c == '\\')
		    add(c), add(hgetc());
		else
		    add(c);
	}
    }
    while (pct);
    if (!lexstop)
	parbegin = -1;
    cmdpop();
    return lexstop;
}
