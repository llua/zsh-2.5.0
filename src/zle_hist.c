/*
 *
 * zle_hist.c - history editing
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

void toggleliteralhistory()
{				/**/
    char *s;

    if (histline == curhist) {
	zsfree(curhistline);
	curhistline = ztrdup(UTOSCP(line));
    }
    lithist ^= 1;
    if (!(s = qgetevent(histline)))
	feep();
    else
	sethistline(STOUCP(s));
}

void uphistory()
{				/**/
    char *s;

    if (mult < 0) {
	mult = -mult;
	downhistory();
	return;
    }
    if (histline == curhist) {
	zsfree(curhistline);
	curhistline = ztrdup(UTOSCP(line));
    }
    histline -= mult;
    if (!(s = qgetevent(histline))) {
	if (unset(NOHISTBEEP))
	    feep();
	histline += mult;
    } else
	sethistline(STOUCP(s));
}

void uplineorhistory()
{				/**/
    int ocs = cs;

    if (mult < 0) {
	mult = -mult;
	downlineorhistory();
	return;
    }
    if ((lastcmd & ZLE_LINEMOVE) != ZLE_LINEMOVE)
	lastcol = cs - findbol();
    cs = findbol();
    while (mult) {
	if (!cs)
	    break;
	cs--;
	cs = findbol();
	mult--;
    }
    if (mult) {
	cs = ocs;
	if (virangeflag) {
	    feep();
	    return;
	}
	uphistory();
    } else {
	int x = findeol();

	if ((cs += lastcol) > x)
	    cs = x;
    }
}

void uplineorsearch()
{				/**/
    int ocs = cs;

    if (mult < 0) {
	mult = -mult;
	downlineorsearch();
	return;
    }
    if ((lastcmd & ZLE_LINEMOVE) != ZLE_LINEMOVE)
	lastcol = cs - findbol();
    cs = findbol();
    while (mult) {
	if (!cs)
	    break;
	cs--;
	cs = findbol();
	mult--;
    }
    if (mult) {
	cs = ocs;
	if (virangeflag) {
	    feep();
	    return;
	}
	historysearchbackward();
    } else {
	int x = findeol();

	if ((cs += lastcol) > x)
	    cs = x;
    }
}

void downlineorhistory()
{				/**/
    int ocs = cs;

    if (mult < 0) {
	mult = -mult;
	uplineorhistory();
	return;
    }
    if ((lastcmd & ZLE_LINEMOVE) != ZLE_LINEMOVE)
	lastcol = cs - findbol();
    while (mult) {
	int x = findeol();

	if (x == ll)
	    break;
	cs = x + 1;
	mult--;
    }
    if (mult) {
	cs = ocs;
	if (virangeflag) {
	    feep();
	    return;
	}
	downhistory();
    } else {
	int x = findeol();

	if ((cs += lastcol) > x)
	    cs = x;
    }
}

void downlineorsearch()
{				/**/
    int ocs = cs;

    if (mult < 0) {
	mult = -mult;
	uplineorsearch();
	return;
    }
    if ((lastcmd & ZLE_LINEMOVE) != ZLE_LINEMOVE)
	lastcol = cs - findbol();
    while (mult) {
	int x = findeol();

	if (x == ll)
	    break;
	cs = x + 1;
	mult--;
    }
    if (mult) {
	cs = ocs;
	if (virangeflag) {
	    feep();
	    return;
	}
	historysearchforward();
    } else {
	int x = findeol();

	if ((cs += lastcol) > x)
	    cs = x;
    }
}

void acceptlineanddownhistory()
{				/**/
    char *s, *t;

    if (!(s = qgetevent(histline + 1))) {
	feep();
	return;
    }
    pushnode(bufstack, t = ztrdup(s));
    for (; *t; t++)
	if (*t == HISTSPACE)
	    *t = ' ';
    done = 1;
    stackhist = histline + 1;
}

void downhistory()
{				/**/
    char *s;

    if (mult < 0) {
	mult = -mult;
	uphistory();
	return;
    }
    histline += mult;
    if (!(s = qgetevent(histline))) {
	if (unset(NOHISTBEEP))
	    feep();
	histline -= mult;
	return;
    }
    sethistline(STOUCP(s));
}

static int histpos;

void historysearchbackward()
{				/**/
    int t0, ohistline = histline;
    char *s;

    if (histline == curhist) {
	zsfree(curhistline);
	curhistline = ztrdup(UTOSCP(line));
    }
    if (lastcmd & ZLE_HISTSEARCH)
	t0 = histpos;
    else
	for (t0 = 0; line[t0] && !iblank(line[t0]); t0++);
    histpos = t0;
    for (;;) {
	histline--;
	if (!(s = qgetevent(histline))) {
	    feep();
	    histline = ohistline;
	    return;
	}
	if (!hstrncmp(s, UTOSCP(line), t0) && hstrcmp(s, UTOSCP(line)))
	    break;
    }
    sethistline(STOUCP(s));
}

void historysearchforward()
{				/**/
    int t0, ohistline = histline;
    char *s;

    if (histline == curhist) {
	zsfree(curhistline);
	curhistline = ztrdup(UTOSCP(line));
    }
    if (lastcmd & ZLE_HISTSEARCH)
	t0 = histpos;
    else
	for (t0 = 0; line[t0] && !iblank(line[t0]); t0++);
    histpos = t0;
    for (;;) {
	histline++;
	if (!(s = qgetevent(histline))) {
	    feep();
	    histline = ohistline;
	    return;
	}
	if (!hstrncmp(s, UTOSCP(line), t0) && hstrcmp(s, UTOSCP(line)))
	    break;
    }
    sethistline(STOUCP(s));
}

void beginningofbufferorhistory()
{				/**/
    if (findbol())
	cs = 0;
    else
	beginningofhistory();
}

void beginningofhistory()
{				/**/
    char *s;

    if (histline == curhist) {
	zsfree(curhistline);
	curhistline = ztrdup(UTOSCP(line));
    }
    if (!(s = qgetevent(firsthist()))) {
	if (unset(NOHISTBEEP))
	    feep();
	return;
    }
    histline = firsthist();
    sethistline(STOUCP(s));
}

void endofbufferorhistory()
{				/**/
    if (findeol() != ll)
	cs = ll;
    else
	endofhistory();
}

void endofhistory()
{				/**/
    if (histline == curhist) {
	if (unset(NOHISTBEEP))
	    feep();
    } else {
	histline = curhist;
	sethistline(STOUCP(curhistline));
    }
}

void insertlastword()
{				/**/
    char *s, *t;
    int len, z = lithist;

/* multiple calls will now search back through the history, pem */
    static char *lastinsert;
    static int lasthist, lastpos;
    int evhist = curhist - 1;

    if (lastinsert) {
	int lastlen = strlen(lastinsert);
	int pos = cs;

	if (lastpos <= pos &&
	    lastlen == pos - lastpos &&
	    strncmp(lastinsert, (char *)&line[lastpos], lastlen) == 0) {
	    evhist = --lasthist;
	    cs = lastpos;
	    foredel(pos - cs);
	}
	zsfree(lastinsert);
	lastinsert = NULL;
    }
    lithist = 0;
    if (!(s = qgetevent(evhist), lithist = z, s)) {
	feep();
	return;
    }
    for (t = s + strlen(s); t > s; t--)
	if (*t == HISTSPACE)
	    break;
    if (t != s)
	t++;
    lasthist = evhist;
    lastpos = cs;
    lastinsert = ztrdup(t);
    spaceinline(len = strlen(t));
    strncpy((char *)line + cs, t, len);
    cs += len;
}

char *qgetevent(ev)		/**/
int ev;
{
    if (ev > curhist)
	return NULL;
    return ((ev == curhist) ? curhistline : quietgetevent(ev));
}

void pushline()
{				/**/
    if (mult < 0)
	return;
    pushnode(bufstack, ztrdup(UTOSCP(line)));
    while (--mult)
	pushnode(bufstack, ztrdup(""));
    stackcs = cs;
    *line = '\0';
    ll = cs = 0;
}

void pushpopinput()
{				/**/
    int ics;
    char *iline;
    Histent curhistent = gethistent(curhist);

    if (mult < 0)
	return;
    if (*(curhistent->lit)) {
	ics = strlen(curhistent->lit);
	iline = (char *)zalloc(strlen((char *)line) + ics + 1);
	strcpy(iline, curhistent->lit);
	strcat(iline, (char *)line);
	free(line);
	line = (unsigned char *)iline;
	ll += ics;
	cs += ics;
	*(curhistent->lit) = '\0';
    }
    pushline();
    if (!isfirstln) {
	*(hptr = chline) = '\0';
	errflag = done = 1;
    }
}

void pushinput()
{				/**/
    if (mult < 0)
	return;
    if (!isfirstln)
	mult++;
    pushpopinput();
}

void getline()
{				/**/
    char *s = (char *)getnode(bufstack);

    if (!s)
	feep();
    else {
	int cc;

	cc = strlen(s);
	spaceinline(cc);
	strncpy((char *)line + cs, s, cc);
	cs += cc;
	zsfree(s);
    }
}

void historyincrementalsearchbackward()
{				/**/
    doisearch(-1);
}

void historyincrementalsearchforward()
{				/**/
    doisearch(1);
}

extern int ungetok;

void doisearch(dir)		/**/
int dir;
{
    char *s, *oldl;
    char ibuf[256], *sbuf = ibuf + 14;
    int sbptr = 0, cmd, ohl = histline, ocs = cs;
    int nomatch, chequiv = 0;

    strcpy(ibuf, (dir == -1) ? "bck-i-search: " : "fwd-i-search: ");
    statusline = ibuf;
    oldl = ztrdup(UTOSCP(line));
    if (histline == curhist) {
	zsfree(curhistline);
	curhistline = ztrdup(UTOSCP(line));
    }
    for (;;) {
	nomatch = 0;
	if (sbptr > 1 || (sbptr == 1 && sbuf[0] != '^')) {
	    int ohistline = histline;

	    for (;;) {
		char *t;

		if (!(s = qgetevent(histline))) {
		    feep();
		    nomatch = 1;
		    histline = ohistline;
		    break;
		}
		if ((sbuf[0] == '^') ?
		    (t = (hstrncmp(s, sbuf + 1, sbptr - 1)) ? NULL : s) :
		    (t = hstrnstr(s, sbuf, sbptr)))
		    if (!(chequiv && !hstrcmp(UTOSCP(line), s))) {
			sethistline(STOUCP(s));
			cs = t - s + sbptr - (sbuf[0] == '^');
			break;
		    }
		histline += dir;
	    }
	    chequiv = 0;
	}
	refresh();
	if ((cmd = getkeycmd()) < 0 || cmd == z_sendbreak) {
	    setline(oldl);
	    cs = ocs;
	    histline = ohl;
	    break;
	}
	switch (cmd) {
	case z_backwarddeletechar:
	    if (sbptr)
		sbuf[--sbptr] = '\0';
	    else
		feep();
	    histline = ohl;
	    continue;
	case z_acceptandhold:
	    acceptandhold();
	    goto brk;
	case z_acceptandinfernexthistory:
	    acceptandinfernexthistory();
	    goto brk;
	case z_acceptlineanddownhistory:
	    acceptlineanddownhistory();
	    goto brk;
	case z_acceptline:
	    acceptline();
	    goto brk;
	case z_historyincrementalsearchbackward:
	    ohl = (histline += (dir = -1));
	    chequiv = 1;
	    memcpy(ibuf, "bck", 3);
	    refresh();
	    continue;
	case z_historyincrementalsearchforward:
	    ohl = (histline += (dir = 1));
	    chequiv = 1;
	    memcpy(ibuf, "fwd", 3);
	    refresh();
	    continue;
	case z_sendstring:
	    sendstring();
	    continue;
	case z_quotedinsert:
	    if ((c = getkey(0)) == EOF)
		goto brk;
	    else
		cmd = z_selfinsert;
	default:
	    if (cmd == z_magicspace)
		c = ' ';
	    else if (cmd != z_selfinsert && cmd != z_selfinsertunmeta) {
		if (ungetok)
		    ungetkey(c);
		else
		    feep();
		goto brk;
	    }
	    if (!nomatch && sbptr != 39) {
		sbuf[sbptr++] = c;
		sbuf[sbptr] = '\0';
	    }
	}
    }
  brk:
    free(oldl);
    statusline = NULL;
}

void acceptandinfernexthistory()
{				/**/
    int t0;
    char *s, *t;

    done = 1;
    for (t0 = histline - 2;; t0--) {
	if (!(s = qgetevent(t0)))
	    return;
	if (!hstrncmp(s, UTOSCP(line), ll))
	    break;
    }
    if (!(s = qgetevent(t0 + 1)))
	return;
    pushnode(bufstack, t = ztrdup(s));
    for (; *t; t++)
	if (*t == HISTSPACE)
	    *t = ' ';
    stackhist = t0 + 1;
}

void infernexthistory()
{				/**/
    int t0;
    char *s, *t;

    if (!(t = qgetevent(histline - 1))) {
	feep();
	return;
    }
    for (t0 = histline - 2;; t0--) {
	if (!(s = qgetevent(t0))) {
	    feep();
	    return;
	}
	if (!strcmp(s, t))
	    break;
    }
    if (!(s = qgetevent(t0 + 1))) {
	feep();
	return;
    }
    histline = t0 + 1;
    sethistline(STOUCP(s));
}

void vifetchhistory()
{				/**/
    char *s;

    if (mult < 0)
	return;
    if (histline == curhist) {
	if (!(lastcmd & ZLE_ARG)) {
	    cs = ll;
	    cs = findbol();
	    return;
	}
	zsfree(curhistline);
	curhistline = ztrdup(UTOSCP(line));
    }
    if (!(lastcmd & ZLE_ARG))
	mult = curhist;
    if (!(s = qgetevent(mult)))
	feep();
    else {
	histline = mult;
	sethistline(STOUCP(s));
    }
}

extern int viins_cur_bindtab[];

int getvisrchstr()
{				/**/
    static char sbuf[80] = "/";
    int sptr = 1, cmd, ret = 0;
    int *obindtab = NULL;

    if (visrchstr) {
	zsfree(visrchstr);
	visrchstr = NULL;
    }
    statusline = sbuf;
    sbuf[1] = '\0';
    if (bindtab == altbindtab) {
	obindtab = bindtab;
	bindtab = viins_cur_bindtab;
    }
    while (sptr) {
	refresh();
	if ((cmd = getkeycmd()) < 0 || cmd == z_sendbreak) {
	    ret = 0;
	    break;
	} else if (cmd == z_acceptline || cmd == z_vicmdmode) {
	    visrchstr = ztrdup(sbuf + 1);
	    ret = 1;
	    break;
	} else if (cmd == z_backwarddeletechar ||
		   cmd == z_vibackwarddeletechar) {
	    sbuf[--sptr] = '\0';
	    continue;
	} else if (cmd == z_sendstring) {
	    sendstring();
	    continue;
	} else if (cmd == z_quotedinsert) {
	    if ((c = getkey(0)) == EOF) {
		feep();
		continue;
	    }
	} else if (cmd != z_selfinsert && cmd != z_selfinsertunmeta) {
	    feep();
	    continue;
	}
	if (sptr != 79) {
	    sbuf[sptr++] = c;
	    sbuf[sptr] = '\0';
	}
    }
    statusline = NULL;
    if (obindtab)
	bindtab = obindtab;
    return ret;
}

void vihistorysearchforward()
{				/**/
    visrchsense = 1;
    if (getvisrchstr())
	virepeatsearch();
}

void vihistorysearchbackward()
{				/**/
    visrchsense = -1;
    if (getvisrchstr())
	virepeatsearch();
}

void virepeatsearch()
{				/**/
    int ohistline = histline, t0;
    char *s;

    if (!visrchstr) {
	feep();
	return;
    }
    t0 = strlen(visrchstr);
    if (histline == curhist) {
	zsfree(curhistline);
	curhistline = ztrdup(UTOSCP(line));
    }
    for (;;) {
	histline += visrchsense;
	if (!(s = qgetevent(histline))) {
	    feep();
	    histline = ohistline;
	    return;
	}
	if (!hstrcmp(UTOSCP(line), s))
	    continue;
	if (*visrchstr == '^') {
	    if (!hstrncmp(s, visrchstr + 1, t0 - 1))
		break;
	} else if (hstrnstr(s, visrchstr, t0))
	    break;
    }
    sethistline(STOUCP(s));
}

void virevrepeatsearch()
{				/**/
    visrchsense = -visrchsense;
    virepeatsearch();
    visrchsense = -visrchsense;
}

/* Extra function added by A.R. Iano-Fletcher.	*/
/*The extern variable "cs" is the position of the cursor. */
/* history-beginning-search-backward */

void historybeginningsearchbackward()
{				/**/
    int cpos = cs;		/* save cursor position */
    int ohistline = histline;
    char *s;

    if (histline == curhist) {
	zsfree(curhistline);
	curhistline = ztrdup((char *)line);
    }
    for (;;) {
	histline--;
	if (!(s = qgetevent(histline))) {
	    feep();
	    histline = ohistline;
	    return;
	}
	if (!hstrncmp(s, (char *)line, cs) && hstrcmp(s, (char *)line))
	    break;
    }

    sethistline((unsigned char *)s);	/* update command line.		*/
    cs = cpos;			/* reset cursor position.	*/
}

/* Extra function added by A.R. Iano-Fletcher.	*/

/* history-beginning-search-forward */
void historybeginningsearchforward()
{				/**/
    int cpos = cs;		/* save cursor position */
    int ohistline = histline;
    char *s;

    if (histline == curhist) {
	zsfree(curhistline);
	curhistline = ztrdup((char *)line);
    }
    for (;;) {
	histline++;
	if (!(s = qgetevent(histline))) {
	    feep();
	    histline = ohistline;
	    return;
	}
	if (!hstrncmp(s, (char *)line, cs) && hstrcmp(s, (char *)line))
	    break;
    }

    sethistline((unsigned char *)s);	/* update command line.		*/
    cs = cpos;			/* reset cursor position.	*/
}
