/*
 *
 * zle_refresh.c - screen update
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

char **obuf = NULL, **nbuf = NULL;
static int olnct, vcs, vln, vmaxln, winw, winh, winpos, ovln;

void resetvideo()
{				/**/
    int ln;
    static int lwinw = -1, lwinh = -1;

    winw = columns;
    if (isset(SINGLELINEZLE) || !termok)
	winh = 1;
    else
	winh = (lines < 2) ? 24 : lines;
    winpos = vln = vmaxln = 0;
    if (lwinw != winw || lwinh != winh) {
	if (nbuf) {
	    for (ln = 0; ln != lwinh; ln++) {
		zfree(nbuf[ln], winw + 1);
		zfree(obuf[ln], winw + 1);
	    }
	    free(nbuf);
	    free(obuf);
	}
	nbuf = (char **)zcalloc((winh + 1) * sizeof(char *));
	obuf = (char **)zcalloc((winh + 1) * sizeof(char *));

	nbuf[0] = (char *)zalloc(winw + 1);
	obuf[0] = (char *)zalloc(winw + 1);

	lwinw = winw;
	lwinh = winh;
    }
    for (ln = 0; ln != winh + 1; ln++) {
	if (nbuf[ln])
	    *nbuf[ln] = '\0';
	if (obuf[ln])
	    *obuf[ln] = '\0';
    }

    if (pptlen) {
	for (ln = 0; ln != pptlen; ln++)
	    nbuf[0][ln] = obuf[0][ln] = ' ';
	nbuf[0][pptlen] = obuf[0][pptlen] = '\0';
    }
    vcs = pptlen;
    olnct = nlnct = 1;
}

int scrollwindow()
{				/**/
    int t0, hwinh = winh / 2;

    for (t0 = 0; t0 != winh - hwinh; t0++) {
	char *s;

	s = nbuf[t0];
	nbuf[t0] = nbuf[t0 + hwinh];
	nbuf[t0 + hwinh] = s;
    }
    for (t0 = 0; t0 < pptlen - 1; t0++)
	nbuf[0][t0] = ' ';
    strcpy(nbuf[0] + t0, "> ...");
    return winh - hwinh;
}

/* this is the messy part. */
/* this define belongs where it's used!!! */

#define nextline { *s = (unsigned char)'\0'; \
	if (winh == ln+1) if (nvln != -1) break; else ln = scrollwindow()-1; \
	if (!nbuf[++ln]) nbuf[ln] = (char *)zalloc(winw + 1); \
	s = (unsigned char *)nbuf[ln]; sen = s+winw; \
	}

#ifdef TIOCGWINSZ
int winchanged;

#endif

int hasam;
static oput_rprompt;
extern int clearflag;

void refresh()
{				/**/
    unsigned char *s, *t, *sen, *scs = line + cs;
    char **qbuf;
    int ln = 0, nvcs = 0, nvln = -1, t0 = -1, put_rprompt, res = 0;

#ifdef HAS_SELECT
    cost = 0;
#endif
    if (resetneeded) {
	setterm();
#ifdef TIOCGWINSZ
	if (winchanged) {
	    moveto(0, 0);
	    t0 = olnct;		/* this is to clear extra lines even when */
	    winchanged = 0;	/* the terminal cannot TCCLEAREOD */
	}
#endif
	resetvideo();
	resetneeded = 0;
	oput_rprompt = 0;
	if (!clearflag)
	    if (tccan(TCCLEAREOD))
		tcout(TCCLEAREOD);
	    else
		res = 1;
	if (t0 > -1)
	    olnct = t0;
	if (isset(SINGLELINEZLE) || !termok)
	    vcs = 0;
	else if (pmpt && !clearflag)
	    fputs(pmpt, stdout), fflush(stdout);
	if (clearflag)
	    putchar('\r'), vcs = 0, moveto(0, pptlen);
    }
    if (isset(SINGLELINEZLE) || !termok) {
	singlerefresh();
	return;
    }
/* first, we generate the video line buffers so we know what to
	put on the screen.

	s = ptr into the video buffer.
	t = ptr into the real buffer.
	sen = end of the video buffer (eol)
*/

    s = (unsigned char *)(nbuf[ln = 0] + pptlen);
    t = line;
    sen = (unsigned char *)(*nbuf + winw);
    for (; *t; t++) {
	if (icntrl(*t))
	    if (*t == '\n') {
		if (t == scs) {
		    if ((nvcs = (char *)s - nbuf[nvln = ln]) == columns)
			nvcs = 0, nvln++;
		    scs = NULL;
		}
		if (s == sen)
		    nextline;
		nextline;
	    } else if ((char)*t == '\t') {
		t0 = (char *)s - nbuf[ln];
		if (t == scs) {
		    nvln = ln;
		    scs = NULL;
		    if ((nvcs = t0) == columns)
			nvcs = 0, nvln++;
		}
		if ((t0 | 7) + 1 >= winw) {
		    nextline;
		    if (t0 == columns)
			for (t0 = 8; t0; t0--)
			    *s++ = ' ';	/* make tab in first column visible */
		} else {
		    if (t0 == winw) {
			t0 = 0;
			nextline;
		    }
		    do
			*s++ = ' ';
		    while ((++t0) & 7);
		}
	    } else {
		if (s == sen)
		    nextline;
		*s++ = '^';
		if (t == scs)
		    nvcs = s - (unsigned char *)(nbuf[nvln = ln]) - 1,
			scs = NULL;
		if (s == sen)
		    nextline;
		*s++ = (*t == 127) ? '?' : (*t | '@');
	} else {
	    if (s == sen)
		nextline;
	    *s++ = *t;
	}
    /* if the cursor is here, remember it */

	if (t == scs)
	    nvcs = s - (unsigned char *)(nbuf[nvln = ln]) - 1;
    }
    if (scs == t && (nvcs = s - (unsigned char *)(nbuf[nvln = ln])) == columns)
	nvcs = 0, nvln++;
    *s = '\0';
    nlnct = ln + 1;

    if (statusline) {
	if (!nbuf[(nlnct == winh) ? winh - 1 : nlnct++])
	    nbuf[nlnct - 1] = (char *)zalloc(winw + 1);
	s = (unsigned char *)nbuf[nlnct - 1];
	t = (unsigned char *)statusline;
	sen = (unsigned char *)(*nbuf + winw);
	for (; *t; t++) {
	    if (icntrl(*t)) {	/* simplified processing in the status line */
		if (s == sen)
		    nextline;
		*s++ = '^';
		if (s == sen)
		    nextline;
		*s++ = (*t == 127) ? '?' : (*t | '@');
	    } else {
		if (s == sen)
		    nextline;
		*s++ = *t;
	    }
	}
	*s = '\0';
    }
    for (ln = nlnct; ln < winh; ln++)
	zfree(nbuf[ln], winw + 1), nbuf[ln] = NULL;

/* do RPROMPT */

    put_rprompt = pmpt2 && (int)strlen(nbuf[0]) + ppt2len < winw - 1;
    if (put_rprompt) {
	for (t0 = strlen(nbuf[0]); t0 != winw - 1 - ppt2len; t0++)
	    nbuf[0][t0] = ' ';
	nbuf[0][t0] = '\0';
    }
    for (ln = 0; ln < nlnct; ln++) {

    /* if old line and new line are different,
       see if we can insert/delete a line */

	if (ln < olnct - 1 && !(hasam && vcs == columns) &&
	    nbuf[ln] && obuf[ln] &&
	    strncmp(nbuf[ln], obuf[ln], 16)) {
	    if (tccan(TCDELLINE) && obuf[ln + 1] && obuf[ln + 1][0] &&
		nbuf[ln] && !strncmp(nbuf[ln], obuf[ln + 1], 16)) {
		moveto(ln, 0);
		tcout(TCDELLINE);
		zfree(obuf[ln], winw + 1);
		for (t0 = ln; t0 != olnct; t0++)
		    obuf[t0] = obuf[t0 + 1];
		obuf[--olnct] = NULL;
	    }
	/* don't try to insert a line if olnct = vmaxln (vmaxln is the number
	   of lines that have been displayed by this routine) so that we don't
	   go off the end of the screen. */

	    else if (tccan(TCINSLINE) && olnct < vmaxln && nbuf[ln + 1] &&
		     obuf[ln] && !strncmp(nbuf[ln + 1], obuf[ln], 16)) {
		moveto(ln, 0);
		tcout(TCINSLINE);
		for (t0 = olnct; t0 != ln; t0--)
		    obuf[t0] = obuf[t0 - 1];
		obuf[ln] = NULL;
		olnct++;
	    }
	}
	if (res && tccan(TCCLEAREOL)) {
	    moveto(ln, 0);
	    tcout(TCCLEAREOL);
	    refreshline(ln, put_rprompt, 0);
	} else
	    refreshline(ln, put_rprompt, res);
	if (!ln && put_rprompt && !oput_rprompt) {
	    moveto(0, winw - 1 - ppt2len);
	    fputs(pmpt2, stdout);
	    vcs = winw - 1;
	}
    }

/* if old buffer had extra lines, do a clear-end-of-display if we can,
   otherwise, just fill new buffer with blank lines and refresh them */

    if (olnct > nlnct)
	if (tccan(TCCLEAREOD)) {
	    moveto(nlnct, 0);
	    tcout(TCCLEAREOD);
	} else
	    for (ln = nlnct; ln < olnct; ln++)
		if (res && tccan(TCCLEAREOL)) {
		    moveto(ln, 0);
		    tcout(TCCLEAREOL);
		    refreshline(ln, put_rprompt, 0);
		} else
		    refreshline(ln, put_rprompt, res);

/* move to the new cursor position */

    moveto(nvln, nvcs);

    if (isset(ALWAYSLASTPROMPT) &&
	(!nvcs || (nvln == ovln + 1)) &&
	cs == ll &&
	tccan(TCCLEAREOL))
	tcout(TCCLEAREOL);

    ovln = nvln;
    qbuf = nbuf;
    nbuf = obuf;
    obuf = qbuf;
    olnct = nlnct;
    oput_rprompt = put_rprompt;
    if (nlnct > vmaxln)
	vmaxln = nlnct;
    fflush(stdout);
}

#define tcinscost(X) (tccan(TCMULTINS) ? tclen[TCMULTINS] : (X)*tclen[TCINS])
#define tcdelcost(X) (tccan(TCMULTDEL) ? tclen[TCMULTDEL] : (X)*tclen[TCDEL])
#define tc_delchars(X) tcmultout(TCDEL,TCMULTDEL,(X))
#define tc_inschars(X) tcmultout(TCINS,TCMULTINS,(X))
#define tc_upcurs(X) tcmultout(TCUP,TCMULTUP,(X))
#define tc_leftcurs(X) tcmultout(TCLEFT,TCMULTLEFT,(X))

void refreshline(ln, put_rprompt, res)	/**/
int ln;
int put_rprompt;
int res;
{
/* the test in nl below is to prevent a segv if the terminal cannot clear
   either to end of line or display, and nlnct < olnct :-( */

    char *nl = nbuf[ln] ? nbuf[ln] : obuf[0], *ol = obuf[ln] ? obuf[ln] : "", *p1;
    int ccs = 0;

    if (res) {
	char *p = hcalloc(winw + 1);

	memset(p, ' ', winw);
	strcpy(p, nl);
	p[strlen(p)] = ' ';
	nl = p;
    }
    if (hasam && vcs == columns) {	/* must always write another char */
	if (*nl) {		/* after writing in last column */
	    putchar(*nl);
	    nl++, vcs = ccs = 1;
	    if (*ol)
		ol++;
	} else
	    putchar('\r'), putchar('\n'), vcs = 0;
	vln++;
    }
    for (;;) {
	while (*nl && *nl == *ol) {
	    nl++, ol++, ccs++;
	}
	if (!*nl && !*ol)
	    if (!ln && !put_rprompt && oput_rprompt)
		if (tccan(TCCLEAREOL)) {
		    if (ccs < columns) {
			moveto(0, ccs);
			tcout(TCCLEAREOL);
		    }
		    return;
		} else {
		    int x = winw - 1 - ccs;

		    p1 = nl;
		    while (x--)
			*p1++ = ' ';
		    *p1 = '\0';
	    } else {
		if (hasam && ccs == columns && ln < nlnct - 1 &&
		    ln < olnct - 1 && *nbuf[ln + 1] &&
		    !*obuf[ln + 1]) {	/* force join of lines */
		    moveto(ln, ccs - 1);
		    putchar(nl[-1]);
#ifdef HAS_SELECT
		    cost++;
#endif
		    vcs++;
		}
		return;
	    }
    /* if this is the end of the new buffer but the old buffer has stuff
       here, clear to end of line if we can, otherwise fill the new buffer
       with blanks and continue. */

	if (!*nl) {
	    int x = strlen(ol);

	    if (tccan(TCCLEAREOL) &&
		(x > tclen[TCCLEAREOL] || (hasam && ccs + x == columns))) {
		moveto(ln, ccs);
		tcout(TCCLEAREOL);
		*ol = '\0';
		return;
	    } else {
		p1 = nl;
		while (x--)
		    *p1++ = ' ';
		*p1 = '\0';
		continue;
	    }
	}
    /* if this is the end of the old buffer, just dump the rest of the
       new buffer. */

	if (!*ol) {
	    while (!res && *nl == ' ')
		nl++, ccs++;
	    if (*nl) {
		moveto(ln, ccs);
		fwrite(nl, strlen(nl), 1, stdout);
#ifdef HAS_SELECT
		cost += strlen(nl);
#endif
		vcs += strlen(nl);

		if (oput_rprompt && !put_rprompt) {
		    ccs += strlen(nl);
		    *nl = '\0';
		    continue;
		}
	    } else if (hasam && ccs == columns) {	/* must always write */
		moveto(ln, ccs - 1);	/* last column */
		putchar(' '), vcs++;
#ifdef HAS_SELECT
		cost++;
#endif
	    }
	    return;
	}
    /* try to insert/delete characters unless there is an rprompt and the old
       line also had it; in this case the length is not changed so that we
       don't have to redraw the rprompt */

	moveto(ln, ccs);
	if (!ln && put_rprompt && oput_rprompt)
	    goto jump;

	if (ol[1] != nl[1] && tccan(TCDEL)) {
	    int ct = 0;

	    for (p1 = ol; *p1; p1++, ct++)
		if (tcdelcost(ct) < pfxlen(p1, nl)) {
		    tc_delchars(ct);
		    ol = p1;
		    break;
		}
	    if (*p1)
		continue;
	}
	if (ol[1] != nl[1] && tccan(TCINS)) {
	    int ct = 0;

	    for (p1 = nl; *p1; p1++, ct++)
		if (tcinscost(ct) < pfxlen(p1, ol) + ct) {
		/* make sure we aren't inserting characters off the end of the
		   screen */
#if 0
		/* if we are, jump to the end and truncate the line, if we can
		   do it quickly (gee, clever idea, Paul!) */
		    if (ct + ccs + strlen(ol) >= winw - 1) {
			if (!tccan(TCMULTRIGHT) || ccs > winw - tclen[TCMULTRIGHT])
			    continue;
			moveto(ln, winw - 1 - ct);
			if (!tccan(TCCLEAREOL) || ct < tclen[TCCLEAREOL]) {
			    int x = ct;

			    while (vcs++, x--)
				putchar(' ');
			} else
			    tcout(TCCLEAREOL);
			moveto(ln, ccs);
		    }
#endif
		    if (ct + ccs + (int)strlen(ol) < winw - 1) {
			tc_inschars(ct = p1 - nl);
			ccs = (vcs += ct);
#ifdef HAS_SELECT
			cost += ct;
#endif
			fwrite(nl, ct, 1, stdout);
			nl += ct;
			break;
		    }
		}
	    if (*p1)
		continue;
	}
    /* if we can't do anything fancy, just write the new character and
       keep going. */

      jump:
	putchar(*nl);
#ifdef HAS_SELECT
	cost++;
#endif
	nl++, ol++, ccs = ++vcs;
    }
}

void moveto(ln, cl)		/**/
int ln;
int cl;
{
    if (ln == vln && cl == vcs)
	return;

    if (hasam && vcs == columns && vln != lines - 1) {
	putchar(' '), tcout(TCLEFT);
	vln++, vcs = 0;
#ifdef HAS_SELECT
	cost++;
#endif
    }
/* move up */

    if (ln < vln) {
	tc_upcurs(vln - ln);
	vln = ln;
    }
/* move down; if we might go off the end of the screen, use newlines
	instead of TCDOWN */

    while (ln > vln) {
	if (vln < vmaxln - 1)
	    if (ln > vmaxln - 1) {
		if (tc_downcurs(vmaxln - 1 - vln))
		    vcs = 0;
		vln = vmaxln - 1;
	    } else {
		if (tc_downcurs(ln - vln))
		    vcs = 0;
		vln = ln;
		continue;
	    }
	putchar('\r'), vcs = 0;	/* safety precaution */
#ifdef HAS_SELECT
	cost++;
#endif
	while (ln > vln) {
	    putchar('\n');
#ifdef HAS_SELECT
	    cost++;
#endif
	    vln++;
	}
    }
    if (cl < vcs / 2) {
	putchar('\r');
#ifdef HAS_SELECT
	cost++;
#endif
	vcs = 0;
    }
    if (vcs < cl)
	tc_rightcurs(cl);
    else if (vcs > cl)
	tc_leftcurs(vcs - cl);
    vcs = cl;
}

void tcmultout(cap, multcap, ct)/**/
int cap;
int multcap;
int ct;
{
    if (tccan(multcap) && (!tccan(cap) || tclen[multcap] < tclen[cap] * ct))
	tcoutarg(multcap, ct);
    else
	while (ct--)
	    tcout(cap);
}

void tc_rightcurs(cl)		/**/
int cl;
{
    int ct = cl - vcs;

/* do a multright if it's cheaper or if we're walking over the prompt.  */

    if (tccan(TCMULTRIGHT) &&
	(ct > tclen[TCMULTRIGHT] || (vln == 0 && vcs < pptlen))) {
	tcoutarg(TCMULTRIGHT, ct);
	return;
    }
/* try to go with tabs if a multright is not feasible/convenient;
   tabs are assumed to be 8 spaces */

    if (tccan(TCNEXTTAB)) {
	if ((vcs | 7) + 1 <= cl) {
	    tcout(TCNEXTTAB);
	    vcs = (vcs | 7) + 1;
	}
	for (; vcs + 8 <= cl; vcs += 8)
	    tcout(TCNEXTTAB);
	if (vcs == cl)
	    return;
    }
/* if we're walking over the prompt and we can do a bunch of cursor rights,
   do them, even though they're more expensive.  (We can't redraw the
   prompt very easily in general.)  */

    if (vln == 0 && tccan(TCRIGHT))
	for (; vcs < pptlen; vcs++)
	    tcout(TCRIGHT);

/* otherwise write the contents of the video buffer. */

    if ((ct = cl - vcs)) {
	fwrite(nbuf[vln] + vcs, ct, 1, stdout);
#ifdef HAS_SELECT
	cost += ct;
#endif
    }
}

int tc_downcurs(ct)		/**/
int ct;
{
    int ret = 0;

    if (ct) {
	if (tccan(TCMULTDOWN) &&
	    (!tccan(TCDOWN) || tclen[TCMULTDOWN] < tclen[TCDOWN] * ct))
	    tcoutarg(TCMULTDOWN, ct);
	else if (tccan(TCDOWN)) {
	    while (ct--)
		tcout(TCDOWN);
	} else {
	    while (ct--)
		putchar('\n');
	    putchar('\r'), ret = -1;
	}
    }
    return ret;
}

/* I'm NOT going to worry about padding unless anyone complains. */

void tcout(cap)			/**/
int cap;
{
    tputs(tcstr[cap], 1, putraw);
}

void tcoutarg(cap, arg)		/**/
int cap;
int arg;
{
    tputs(tgoto(tcstr[cap], arg, arg), 1, putraw);
}

void clearscreen()
{				/**/
    tcout(TCCLEARSCREEN);
    resetneeded = 1;
    clearflag = 0;
}

void redisplay()
{				/**/
    moveto(0, pptlen);
    if (tccan(TCCLEAREOD))
	tcout(TCCLEAREOD);
    resetneeded = clearflag = 1;
}

void singlerefresh()
{				/**/
    char *vbuf, *vp, **qbuf, *refreshop;
    int t0, vsiz, nvcs = 0;

    for (vsiz = 1 + pptlen, t0 = 0; t0 != ll; t0++, vsiz++)
	if (line[t0] == '\t')
	    vsiz += 7;
	else if (icntrl(line[t0]))
	    vsiz++;
    vbuf = (char *)zalloc(vsiz);
    strcpy(vbuf, pmpt);
    vp = vbuf + pptlen;
    for (t0 = 0; t0 != ll; t0++) {
	if (line[t0] == '\t')
	    do
		*vp++ = ' ';
	    while ((vp - vbuf) & 7);
	else if (line[t0] == '\n') {
	    *vp++ = '\\';
	    *vp++ = 'n';
	} else if (line[t0] == 0x7f) {
	    *vp++ = '^';
	    *vp++ = '?';
	} else if (icntrl(line[t0])) {
	    *vp++ = '^';
	    *vp++ = line[t0] | '@';
	} else
	    *vp++ = line[t0];
	if (t0 == cs)
	    nvcs = vp - vbuf - 1;
    }
    if (t0 == cs)
	nvcs = vp - vbuf;
    *vp = '\0';
    if ((winpos && nvcs < winpos + 1) || (nvcs > winpos + winw - 1)) {
	if ((winpos = nvcs - (winw / 2)) < 0)
	    winpos = 0;
    }
    if (winpos)
	vbuf[winpos] = '<';
    if ((int)strlen(vbuf + winpos) > winw) {
	vbuf[winpos + winw - 1] = '>';
	vbuf[winpos + winw] = '\0';
    }
    strcpy(nbuf[0], vbuf + winpos);
    zfree(vbuf, vsiz);
    nvcs -= winpos;
    for (t0 = 0, vp = *nbuf, refreshop = *obuf; *vp; t0++, vp++) {
	if (*vp != *refreshop && !(*vp == ' ' && !*refreshop)) {
	    singmoveto(t0);
	    putchar(*vp);
	    vcs++;
	}
	if (*refreshop)
	    refreshop++;
    }
    if (*refreshop) {
	singmoveto(t0);
	for (; *refreshop; refreshop++) {
	    putchar(' ');
	    vcs++;
	}
    }
    singmoveto(nvcs);
    qbuf = nbuf;
    nbuf = obuf;
    obuf = qbuf;
    fflush(stdout);
}

void singmoveto(pos)		/**/
int pos;
{
    while (pos < vcs) {
	vcs--;
	putchar('\b');
    }
    while (pos > vcs) {
	putchar(nbuf[0][vcs]);
	vcs++;
    }
}
