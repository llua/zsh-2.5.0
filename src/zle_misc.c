/*
 *
 * zle_misc.c - miscellaneous editor routines
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

void selfinsert()
{				/**/
    int ncs = cs + mult;

    if (complexpect && isset(AUTOPARAMKEYS)) {
	if (complexpect == 2 && c == '}') {
	    spaceinline(1);
	    line[cs - 1] = c;
	    line[cs++] = ' ';
	    return;
	} else if (complexpect == 1 &&
		   (c == '[' || c == ':' || c == '#' || c == '%' ||
		    c == '-' || c == '?' || c == '+')) {
	    line[cs - 1] = c;
	    return;
	}
    }
    if (mult < 0) {
	mult = -mult;
	ncs = cs;
    }
    if (insmode || ll == cs)
	spaceinline(mult);
    else if (mult + cs > ll)
	spaceinline(ll - (mult + cs));
    while (mult--)
	line[cs++] = c;
    cs = ncs;
}

void selfinsertunmeta()
{				/**/
    c &= 0x7f;
    if (c == '\r')
	c = '\n';
    selfinsert();
}

void deletechar()
{				/**/
    if (mult < 0) {
	mult = -mult;
	backwarddeletechar();
	return;
    }
    if (!(cs + mult > ll || line[cs] == '\n')) {
	cs += mult;
	backdel(mult);
    } else
	feep();
}

void backwarddeletechar()
{				/**/
    if (mult < 0) {
	mult = -mult;
	deletechar();
	return;
    }
    if (mult > cs)
	mult = cs;
    backdel(mult);
}

void vibackwarddeletechar()
{				/**/
    if (mult < 0) {
	mult = -mult;
	videletechar();
	return;
    }
    if (mult > cs)
	mult = cs;
    if (cs - mult < viinsbegin) {
	feep();
	return;
    }
    backkill(mult, 1);
}

void vikillline()
{				/**/
    if (viinsbegin > cs) {
	feep();
	return;
    }
    backdel(cs - viinsbegin);
}

void killwholeline()
{				/**/
    int i, fg;

    if (mult < 0)
	return;
    while (mult--) {
	if ((fg = (cs && cs == ll)))
	    cs--;
	while (cs && line[cs - 1] != '\n')
	    cs--;
	for (i = cs; i != ll && line[i] != '\n'; i++);
	forekill(i - cs + (i != ll), fg);
    }
}

void killbuffer()
{				/**/
    cs = 0;
    forekill(ll, 0);
}

void backwardkillline()
{				/**/
    int i = 0;

    if (mult < 0) {
	mult = -mult;
	killline();
	return;
    }
    while (mult--) {
	if (cs && line[cs - 1] == '\n')
	    cs--, i++;
	else
	    while (cs && line[cs - 1] != '\n')
		cs--, i++;
    }
    forekill(i, 1);
}

void gosmacstransposechars()
{				/**/
    int cc;

    if (cs < 2 || line[cs - 1] == '\n' || line[cs - 2] == '\n') {
	if (line[cs] == '\n' || line[cs + 1] == '\n') {
	    feep();
	    return;
	}
	cs += (cs == 0 || line[cs - 1] == '\n') ? 2 : 1;
    }
    cc = line[cs - 2];
    line[cs - 2] = line[cs - 1];
    line[cs - 1] = cc;
}

void transposechars()
{				/**/
    int cc, ct;
    int neg = mult < 0;

    if (neg)
	mult = -mult;
    while (mult--) {
	if (!(ct = cs) || line[cs - 1] == '\n') {
	    if (ll == cs || line[cs] == '\n') {
		feep();
		return;
	    }
	    if (!neg)
		cs++;
	    ct++;
	}
	if (neg) {
	    if (cs && line[cs - 1] != '\n') {
		cs--;
		if (ct > 1 && line[ct - 2] != '\n')
		    ct--;
	    }
	} else {
	    if (cs != ll && line[cs] != '\n')
		cs++;
	}
	if (ct == ll || line[ct] == '\n')
	    ct--;
	if (ct < 1 || line[ct - 1] == '\n') {
	    feep();
	    return;
	}
	cc = line[ct - 1];
	line[ct - 1] = line[ct];
	line[ct] = cc;
    }
}

void poundinsert()
{				/**/
    if (*line != '#') {
	cs = 0;
	spaceinline(1);
	*line = '#';
    } else {
	cs = 0;
	foredel(1);
    }
    done = 1;
}

void acceptline()
{				/**/
    done = 1;
}

void acceptandhold()
{				/**/
    pushnode(bufstack, ztrdup((char *)line));
    stackcs = cs;
    done = 1;
}

void killline()
{				/**/
    int i = 0;

    if (mult < 0) {
	mult = -mult;
	backwardkillline();
	return;
    }
    while (mult--) {
	if (line[cs] == '\n')
	    cs++, i++;
	else
	    while (cs != ll && line[cs] != '\n')
		cs++, i++;
    }
    backkill(i, 0);
}

void killregion()
{				/**/
    if (mark > ll)
	mark = ll;
    if (mark > cs)
	forekill(mark - cs, 0);
    else
	backkill(cs - mark, 1);
}

void copyregionaskill()
{				/**/
    if (mark > ll)
	mark = ll;
    if (mark > cs)
	cut(cs, mark - cs, 0);
    else
	cut(mark, cs - mark, 1);
}

static int kct, yankb, yanke;

void yank()
{				/**/
    int cc;
    char *buf = cutbuf;

    if (!cutbuf) {
	feep();
	return;
    }
    if (mult < 0)
	return;
    if (vibufspec) {
	if (!(buf = vibuf[vibufspec])) {
	    feep();
	    vibufspec = 0;
	    return;
	}
	vibufspec = 0;
    }
    yankb = cs;
    while (mult--) {
	kct = kringnum;
	cc = strlen(buf);
	spaceinline(cc);
	strncpy((char *)line + cs, buf, cc);
	cs += cc;
	yanke = cs;
    }
}

void viputafter()
{				/**/
    int cc;
    char *buf = cutbuf;

    if (!cutbuf) {
	feep();
	return;
    }
    if (mult < 0)
	return;
    if (vibufspec) {
	if (!(buf = vibuf[vibufspec])) {
	    feep();
	    vibufspec = 0;
	    return;
	}
	vibufspec = 0;
    }
    if (strchr(buf, '\n')) {
	cs = findeol();
	if (cs == ll) {
	    spaceinline(1);
	    line[cs] = '\n';
	}
    }
    if (cs != ll)
	cs++;
    yankb = cs;
    while (mult--) {
	kct = kringnum;
	cc = strlen(buf);
	spaceinline(cc);
	strncpy((char *)line + cs, buf, cc);
	cs += cc;
	yanke = cs;
    }
    cs = yankb;
}

void yankpop()
{				/**/
    int cc;

    if (!(lastcmd & ZLE_YANK) || !kring[kct]) {
	feep();
	return;
    }
    cs = yankb;
    foredel(yanke - yankb);
    cc = strlen(kring[kct]);
    spaceinline(cc);
    strncpy((char *)line + cs, kring[kct], cc);
    cs += cc;
    yanke = cs;
    kct = (kct - 1) & (KRINGCT - 1);
}

void overwritemode()
{				/**/
    insmode ^= 1;
}

void undefinedkey()
{				/**/
    feep();
}

void quotedinsert()
{				/**/
#ifndef HAS_TIO
    struct sgttyb sob;

    sob = shttyinfo.sgttyb;
    sob.sg_flags = (sob.sg_flags | RAW) & ~ECHO;
    ioctl(SHTTY, TIOCSETN, &sob);
#endif
    c = getkey(0);
#ifndef HAS_TIO
    setterm();
#endif
    if (c > 0)
	selfinsert();
    else
	feep();
}

void digitargument()
{				/**/
    int sign = (mult < 0 || (lastcmd & ZLE_NEGARG)) ? -1 : 1;

    if ((lastcmd & (ZLE_ARG | ZLE_NEGARG)) != ZLE_ARG)
	mult = 0;
    mult = mult * 10 + sign * (c & 0xf);
}

void negargument()
{				/**/
    if (lastcmd & ZLE_ARG)
	feep();
    mult = -1;
}

void universalargument()
{				/**/
    if (!(lastcmd & ZLE_ARG))
	mult = 4;
    else
	mult *= 4;
}

void copyprevword()
{				/**/
    int len, t0;

    for (t0 = cs - 1; t0 >= 0; t0--)
	if (iword(line[t0]))
	    break;
    for (; t0 >= 0; t0--)
	if (!iword(line[t0]))
	    break;
    if (t0)
	t0++;
    len = cs - t0;
    spaceinline(len);
    strncpy((char *)line + cs, (char *)line + t0, len);
    cs += len;
}

void sendbreak()
{				/**/
    errflag = 1;
}

void undo()
{				/**/
    char *s;
    struct undoent *ue;

    ue = undos + undoct;
    if (!ue->change) {
	feep();
	return;
    }
    line[ll] = '\0';
    s = ztrdup((char *)line + ll - ue->suff);
    sizeline((ll = ue->pref + ue->suff + ue->len) + 1);
    strncpy((char *)line + ue->pref, ue->change, ue->len);
    strcpy((char *)line + ue->pref + ue->len, s);
    zsfree(s);
    ue->change = NULL;
    undoct = (undoct - 1) & (UNDOCT - 1);
    cs = ue->cs;
}

void quoteregion()
{				/**/
    char *s, *t;
    int x, y;

    if (mark > ll)
	mark = ll;
    if (mark < cs) {
	x = mark;
	mark = cs;
	cs = x;
    }
    s = (char *)hcalloc((y = mark - cs) + 1);
    strncpy(s, (char *)line + cs, y);
    s[y] = '\0';
    foredel(mark - cs);
    t = makequote(s);
    spaceinline(x = strlen(t));
    strncpy((char *)line + cs, t, x);
    mark = cs;
    cs += x;
}

void quoteline()
{				/**/
    char *s;

    line[ll] = '\0';
    s = makequote((char *)line);
    setline(s);
}

char *makequote(s)		/**/
char *s;
{
    int qtct = 0;
    char *l, *ol;

    for (l = s; *l; l++)
	if (*l == '\'')
	    qtct++;
    l = ol = (char *)halloc((qtct * 3) + 3 + strlen(s));
    *l++ = '\'';
    for (; *s; s++)
	if (*s == '\'') {
	    *l++ = '\'';
	    *l++ = '\\';
	    *l++ = '\'';
	    *l++ = '\'';
	} else
	    *l++ = *s;
    *l++ = '\'';
    *l = '\0';
    return ol;
}

#define NAMLEN 70

int executenamedcommand()
{				/**/
    char buf[NAMLEN], *ptr;
    int len, cmd, t0;

    strcpy(buf, "execute: ");
    ptr = buf + 9;
    len = 0;
    statusline = buf;
    refresh();
    for (;; refresh()) {
	if ((cmd = getkeycmd()) < 0 || cmd == z_sendbreak) {
	    statusline = NULL;
	    return z_undefinedkey;
	}
	switch (cmd) {
	case z_backwarddeletechar:
	case z_vibackwarddeletechar:
	    if (len) {
		len--;
		*--ptr = '\0';
	    }
	    break;
	case z_killregion:
	case z_backwardkillword:
	case z_vibackwardkillword:
	    while (len && (len--, *--ptr != '-'))
		*ptr = '\0';
	    break;
	case z_killwholeline:
	case z_vikillline:
	case z_backwardkillline:
	    len = 0;
	    ptr = buf + 9;
	    *ptr = '\0';
	    break;
	case z_acceptline:
	    for (t0 = 0; t0 != ZLECMDCOUNT; t0++)
		if (!strcmp(buf + 9, zlecmds[t0].name))
		    break;
	    if (t0 != ZLECMDCOUNT) {
		lastnamed = t0;
		statusline = NULL;
		return t0;
	    } else {
		feep();
		break;
	    }
	default:
	    if (cmd == z_listchoices || cmd == z_deletecharorlist ||
		c == ' ' || c == '\t') {
		Lklist cmdll;
		int ambig = 100;

		heapalloc();
		cmdll = newlist();
		for (t0 = 0; t0 != ZLECMDCOUNT; t0++)
		    if (strpfx(buf + 9, zlecmds[t0].name)) {
			int xx;

			addnode(cmdll, zlecmds[t0].name);
			xx = pfxlen(peekfirst(cmdll), zlecmds[t0].name);
			if (xx < ambig)
			    ambig = xx;
		    }
		permalloc();
		if (empty(cmdll))
		    feep();
		else if (cmd == z_listchoices ||
			 cmd == z_deletecharorlist)
		    listlist(cmdll);
		else if (!nextnode(firstnode(cmdll))) {
		    strcpy(buf + 9, peekfirst(cmdll));
		    ptr = buf + (len = strlen(buf));
		} else {
		    strcpy(buf + 9, peekfirst(cmdll));
		    len = ambig;
		    ptr = buf + 9 + len;
		    *ptr = '\0';
		    feep();
		    if (isset(AUTOLIST))
			listlist(cmdll);
		}
	    } else {
		if (len == NAMLEN - 10 || icntrl(c))
		    feep();
		else
		    *ptr++ = c, *ptr = '\0', len++;
	    }
	}
    }
}

void vijoin()
{				/**/
    int x;

    if ((x = findeol()) == ll) {
	feep();
	return;
    }
    cs = x + 1;
    for (x = 1; cs != ll && iblank(line[cs]); cs++, x++);
    backdel(x);
    spaceinline(1);
    line[cs] = ' ';
}

void viswapcase()
{				/**/
    if (cs < ll) {
	int ch = line[cs];

	if (islower(ch))
	    ch = tuupper(ch);
	else if (isupper(ch))
	    ch = tulower(ch);
	line[cs] = ch;
	if (cs != ll - 1)
	    cs++;
    }
}

void vicapslockpanic()
{				/**/
    char ch;

    statusline = "press a lowercase key to continue";
    refresh();
    do
	ch = getkey(0);
    while (!islower(ch));
}

int owrite;

void visetbuffer()
{				/**/
    int ch;

    ch = getkey(0);
    if (!isalnum(ch)) {
	feep();
	return;
    }
    if (ch >= 'A' && ch <= 'Z')	/* needed in cut() */
	owrite = 0;
    else
	owrite = 1;
    vibufspec = tolower(ch) + (idigit(ch)) ? -'1' + 26 : -'a';
}

static char *bp;
static int lensb, countp;

void stradd(d)			/**/
char *d;
{
    while ((*bp++ = *d++));
    bp--;
}

int putstr(d)			/**/
int d;
{
    *bp++ = d;
    if (countp)
	lensb++;
    return 0;
}

void tstradd(X)			/**/
char *X;
{
    int t0;

    if (termok && unset(SINGLELINEZLE)) {
#ifdef _IBMR2
    /* AIX tgetstr() ignores second argument */
	char *tbuf;

	if (tbuf = tgetstr(X, &tbuf))
#else
	char tbuf[2048], *tptr = tbuf;

	if (tgetstr(X, &tptr))
#endif
	    tputs(tbuf, 1, putstr);
	if (*X == 's' && (X[1] == 'o' || X[1] == 'e') &&
	    (t0 = tgetnum("sg")) > -1)
	    lensb -= t0;
    }
}

/* get a prompt string */

static char *buf, *bl0, *fm;
static int bracepos;

char *putprompt(fmin, lenp, isspell)	/**/
char *fmin;
int *lenp;
int isspell;
{
    static char buf0[256], buf1[256], buf2[256];

    bracepos = 0;
    lensb = 0;
    countp = 1;
    fm = fmin;
    if (!fm) {
	*lenp = 0;
	return "";
    }
/* KLUDGE ALERT!  What we have here are three buffers:
	 *  buf1 and buf2 alternate between PS1 and PS2, though which is
	 *   which is indeterminate depending on spellchecking, "select",
	 *   etc. -- those operations also share these two buffers.
	 *  buf0 is used for any prompting that manages to happen while
	 *   zleread() is in progress (signal traps, etc.), because
	 *   zleread() re-uses the pointers returned to buf1 and buf2
	 *   and will be confused if either of those is overwritten.
	 */
    buf = zleactive ? buf0 : ((buf == buf1) ? buf2 : buf1);
    bp = bl0 = buf;
    if (!columns)
	columns = 80;
    clearerr(stdin);

    putpromptchar(isspell == -1 ? 0 : isspell, 1, '\0');

    if (isspell != -1) {
	*lenp = (bp - bl0) - lensb;
	*lenp %= columns;
	if (*lenp == columns - 1) {
	    *lenp = 0;
	    *bp++ = ' ';
	}
    } else
	*lenp = (bp - bl0);
    *bp = '\0';

    *bp = '\0';
    return buf;
}

int putpromptchar(isspell, doprint, endchar)	/**/
int isspell;
int doprint;
int endchar;
{
    char buf3[MAXPATHLEN], *ss;
    int t0, arg, test, sep;
    struct tm *tm;
    time_t timet;

    if (isset(PROMPTSUBST)) {
	char *sss;

	fm = dupstring(fm);
	for (ss = fm; *ss; ss++)
	    if (*ss == '$' && ss[1] && (ss == fm || ss[-1] != '%')) {
		*ss = String;
		if (ss[1] == '[') {
		    ss[1] = Inbrack;
		    for (t0 = 0, sss = ss + 2; *sss && (t0 || *sss != ']'); sss++) {
			if (*sss == '[')
			    t0++;
			if (*sss == ']')
			    t0--;
			if (*sss == '\\' && sss[1])
			    sss++;
		    }
		    if (*sss == ']')
			*sss = Outbrack, ss = sss;
		} else if (ss[1] == '(') {
		    ss[1] = Inpar;
		    for (t0 = 0, sss = ss + 2; *sss && (t0 || *sss != ')'); sss++) {
			if (*sss == '(')
			    t0++;
			if (*sss == ')')
			    t0--;
			if (*sss == '\\' && sss[1])
			    sss++;
		    }
		    if (*sss == ')')
			*sss = Outpar, ss = sss;
		}
	    }
	lexsave();
	singsub(&fm);
	lexrestore();
    }
    for (; *fm && *fm != endchar; fm++) {
	if (bp - buf >= 220)
	    break;
	arg = 0;
	if (*fm == '%') {
	    if (idigit(*++fm)) {
		arg = zstrtol(fm, &fm, 10);
	    }
	    if (*fm == '(') {
		int tc;

		if (idigit(*++fm)) {
		    arg = zstrtol(fm, &fm, 10);
		}
		test = 0;
		ss = pwd;
		switch (tc = *fm) {
		case 'c':
		case '.':
		case '~':
		    t0 = finddir(ss);
		    if (t0 != -1) {
			arg--;
			ss += namdirs[t0].len;
		    }
		case '/':
		case 'C':
		    for (; *ss; ss++)
			if (*ss == '/')
			    arg--;
		    if (arg <= 0)
			test = 1;
		    break;
		case 't':
		case 'T':
		case 'd':
		case 'D':
		case 'w':
		    timet = time(NULL);
		    tm = localtime(&timet);
		    switch (tc) {
		    case 't':
			test = (arg == tm->tm_min);
			break;
		    case 'T':
			test = (arg == tm->tm_hour);
			break;
		    case 'd':
			test = (arg == tm->tm_mday);
			break;
		    case 'D':
			test = (arg == tm->tm_mon);
			break;
		    case 'w':
			test = (arg == tm->tm_wday);
			break;
		    }
		    break;
		case '?':
		    if (lastval == arg)
			test = 1;
		    break;
		case '#':
		    if (geteuid() == arg)
			test = 1;
		    break;
		case 'g':
		    if (getegid() == arg)
			test = 1;
		    break;
		case 'L':
		    if (shlvl >= arg)
			test = 1;
		    break;
		case 'S':
		    if (time(NULL) - shtimer.tv_sec >= arg)
			test = 1;
		    break;
		case 'v':
		    if (arrlen(psvar) >= arg)
			test = 1;
		    break;
		case '_':
		    test = (cmdsp >= arg);
		    break;
		default:
		    test = -1;
		    break;
		}
		if (!*fm || !(sep = *++fm))
		    return 0;
		fm++;
		if (!putpromptchar(isspell, test == 1 && doprint, sep) || !*++fm ||
		    !putpromptchar(isspell, test == 0 && doprint, ')')) {
		    return 0;
		}
		continue;
	    }
	    if (!doprint)
		continue;
	    switch (*fm) {
	    case '~':
		t0 = finddir(pwd);
		if (t0 != -1) {
		    *bp++ = '~';
		    stradd(namdirs[t0].name);
		    stradd(pwd + namdirs[t0].len);
		    break;
		}
	    case 'd':
	    case '/':
		stradd(pwd);
		break;
	    case 'c':
	    case '.':
		t0 = finddir(pwd);
		if (t0 != -1) {
		    sprintf(buf3, "~%s%s", namdirs[t0].name,
			    pwd + namdirs[t0].len);
		} else {
		    strcpy(buf3, pwd);
		}
		if (!arg)
		    arg++;
		for (ss = buf3 + strlen(buf3); ss > buf3; ss--)
		    if (*ss == '/' && !--arg) {
			ss++;
			break;
		    }
		if (*ss == '/' && ss[1] && (ss != buf3))
		    ss++;
		stradd(ss);
		break;
	    case 'C':
		strcpy(buf3, pwd);
		if (!arg)
		    arg++;
		for (ss = buf3 + strlen(buf3); ss > buf3; ss--)
		    if (*ss == '/' && !--arg) {
			ss++;
			break;
		    }
		if (*ss == '/' && ss[1] && (ss != buf3))
		    ss++;
		stradd(ss);
		break;
	    case 'h':
	    case '!':
		sprintf(bp, "%d", curhist);
		bp += strlen(bp);
		break;
	    case 'M':
		stradd(hostnam);
		break;
	    case 'm':
		if (!arg)
		    arg++;
		for (ss = hostnam; *ss; ss++)
		    if (*ss == '.' && !--arg)
			break;
		t0 = *ss;
		*ss = '\0';
		stradd(hostnam);
		*ss = t0;
		break;
	    case 'S':
		tstradd("so");
		break;
	    case 's':
		tstradd("se");
		break;
	    case 'B':
		tstradd("md");
		break;
	    case 'b':
		tstradd("me");
		break;
	    case 'U':
		tstradd("us");
		break;
	    case 'u':
		tstradd("ue");
		break;
	    case '{':
		bracepos = bp - buf;
		countp = 0;
		break;
	    case '}':
		lensb += (bp - buf) - bracepos;
		countp = 1;
		break;
	    case 't':
	    case '@':
	    case 'T':
	    case '*':
	    case 'w':
	    case 'W':
	    case 'D':
		{
		    char *tmfmt, *dd;

		    switch (*fm) {
		    case 'T':
			tmfmt = "%k:%M";
			break;
		    case '*':
			tmfmt = "%k:%M:%S";
			break;
		    case 'w':
			tmfmt = "%a %e";
			break;
		    case 'W':
			tmfmt = "%m/%d/%y";
			break;
		    case 'D':
			tmfmt = "%y-%m-%d";
			if (fm[1] == '{') {
			    for (ss = fm + 2, dd = buf3; *ss && *ss != '}'; ++ss, ++dd)
				*dd = *((*ss == '\\' && ss[1]) ? ++ss : ss);
			    if (*ss == '}') {
				*dd = '\0';
				fm = ss;
				tmfmt = buf3;
			    }
			}
			break;
		    default:
			tmfmt = "%l:%M%p";
			break;
		    }
		    timet = time(NULL);
		    tm = localtime(&timet);
		    ztrftime(bp, buf + 220 - bp, tmfmt, tm);
		    if (*bp == ' ')
			chuck(bp);
		    bp += strlen(bp);
		    break;
		}
	    case 'n':
		stradd(username);
		break;
	    case 'l':
		if (*ttystrname)
		    stradd((strncmp(ttystrname, "/dev/tty", 8) ?
			    ttystrname + 5 : ttystrname + 8));
		else
		    stradd("()");
		break;
	    case '?':
		sprintf(bp, "%ld", (long)lastval);
		bp += strlen(bp);
		break;
	    case '%':
		*bp++ = '%';
		break;
	    case '#':
		*bp++ = (geteuid())? '%' : '#';
		break;
	    case 'v':
		if (!arg)
		    arg++;
	    /* The number 35 here comes from 256-220-1, where 256 is
	       sizeof(buf), 220 is from the overflow test made at the
	       top of the loop, and 1 is for the \0 byte at the end. */

		if (arrlen(psvar) >= arg && (int)strlen(psvar[arg - 1]) < 35)
		    stradd(psvar[arg - 1]);
		else
		    stradd("");
		break;
	    case 'E':
		tstradd("ce");
		break;
	    case '_':
		if (cmdsp) {
		    if (arg <= 0)
			arg = 1;
		    if (arg > cmdsp)
			arg = cmdsp;
		    for (t0 = cmdsp - arg; arg--; t0++) {
			stradd(cmdnames[cmdstack[t0]]);
			if (arg)
			    stradd(" ");
		    }
		}
		break;
	    case 'r':
		if (isspell) {
		    stradd(rstring);
		    break;
		}
	    case 'R':
		if (isspell) {
		    stradd(Rstring);
		    break;
		}
	    default:
		*bp++ = '%';
		*bp++ = *fm;
		break;
	    }
	} else if (*fm == '!' && doprint) {
	    sprintf(bp, "%d", curhist);
	    bp += strlen(bp);
	} else {
	    if (fm[0] == '\\' && fm[1])
		fm++;
	    if (doprint && (*bp++ = *fm) == '\n')
		bl0 = bp, lensb = 0;
	}
    }

    return *fm;
}
