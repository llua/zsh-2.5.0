/*
 *
 * zle_main.c - main routines for line editor
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

#define ZLEGLOBALS
#define ZLE
#include "zsh.h"
#include <sys/types.h>
#include <errno.h>
#ifdef HAS_SYS_SELECT
#include <sys/select.h>
#endif

static int emacs_cur_bindtab[256], eofchar, eofsent;
int viins_cur_bindtab[256], ungetok;	/* needed in zle_hist */

/* hash table containing the zle multi-character bindings */

static Hashtab xbindtab, vi_xbindtab, em_xbindtab;

static Key cky;

/* set up terminal */

void setterm()
{				/**/
    struct ttyinfo ti;

#if defined(CLOBBERS_TYPEAHEAD) && defined(FIONREAD)
    int val;

    ioctl(SHTTY, FIONREAD, (char *)&val);
    if (val)
	return;
#endif

/* sanitize the tty */
#ifdef HAS_TIO
    shttyinfo.tio.c_lflag |= ICANON | ECHO;
#ifdef FLUSHO
    shttyinfo.tio.c_lflag &= ~FLUSHO;
#endif
#else				/* not HAS_TIO */
    shttyinfo.sgttyb.sg_flags = (shttyinfo.sgttyb.sg_flags & ~CBREAK) | ECHO;
    shttyinfo.lmodes &= ~LFLUSHO;
#endif

    attachtty(mypgrp);
    ti = shttyinfo;
#ifdef HAS_TIO
    if (isset(NOFLOWCONTROL))
	ti.tio.c_iflag &= ~IXON;
    ti.tio.c_lflag &= ~(ICANON | ECHO
#ifdef FLUSHO
			| FLUSHO
#endif
	);
#ifdef TAB3
    ti.tio.c_oflag &= ~TAB3;
#else
#ifdef OXTABS
    ti.tio.c_oflag &= ~OXTABS;
#else
    ti.tio.c_oflag &= ~XTABS;
#endif
#endif
    ti.tio.c_oflag |= ONLCR;
    ti.tio.c_cc[VQUIT] =
#ifdef VDISCARD
	ti.tio.c_cc[VDISCARD] =
#endif
#ifdef VSUSP
	ti.tio.c_cc[VSUSP] =
#endif
#ifdef VDSUSP
	ti.tio.c_cc[VDSUSP] =
#endif
#ifdef VSWTCH
	ti.tio.c_cc[VSWTCH] =
#endif
#ifdef VLNEXT
	ti.tio.c_cc[VLNEXT] =
#endif
	VDISABLEVAL;
#if defined(VSTART) && defined(VSTOP)
    if (isset(NOFLOWCONTROL))
	ti.tio.c_cc[VSTART] = ti.tio.c_cc[VSTOP] = VDISABLEVAL;
#endif
    eofchar = ti.tio.c_cc[VEOF];
    ti.tio.c_cc[VMIN] = 1;
    ti.tio.c_cc[VTIME] = 0;
    ti.tio.c_iflag |= (INLCR | ICRNL);
 /* this line exchanges \n and \r; it's changed back in getkey
	so that the net effect is no change at all inside the shell.
	This double swap is to allow typeahead in common cases, eg.

	% bindkey -s '^J' 'echo foo^M'
	% sleep 10
	echo foo<return>  <--- typed before sleep returns

	The shell sees \n instead of \r, since it was changed by the kernel
	while zsh wasn't looking. Then in getkey() \n is changed back to \r,
	and it sees "echo foo<accept line>", as expected. Without the double
	swap the shell would see "echo foo\n", which is translated to
	"echo fooecho foo<accept line>" because of the binding.
	Note that if you type <line-feed> during the sleep the shell just sees
	\n, which is translated to \r in getkey(), and you just get another
	prompt. For type-ahead to work in ALL cases you have to use
	stty inlcr.
	This workaround is due to Sven Wischnowsky <oberon@cs.tu-berlin.de>.

	Unfortunately it's IMPOSSIBLE to have a general solution if both
	<return> and <line-feed> are mapped to the same character. The shell
	could check if there is input and read it before setting it's own
	terminal modes but if we get a \n we don't know whether to keep it or
	change to \r :-(
	*/

#else				/* not HAS_TIO */
    ti.sgttyb.sg_flags = (ti.sgttyb.sg_flags | CBREAK) & ~ECHO & ~XTABS;
    ti.lmodes &= ~LFLUSHO;
    eofchar = ti.tchars.t_eofc;
    ti.tchars.t_quitc =
	ti.ltchars.t_suspc =
	ti.ltchars.t_flushc =
	ti.ltchars.t_dsuspc = ti.ltchars.t_lnextc = -1;
#endif

#if defined(TTY_NEEDS_DRAINING) && defined(TIOCOUTQ) && defined(HAS_SELECT)
/* this is mostly stolen from bash's draino() */
    if (baud) {			/**/
	int n = 0;

	while ((ioctl(SHTTY, TIOCOUTQ, (char *)&n) >= 0) && n) {
	    struct timeval tv;

	    tv.tv_sec = n / baud;
	    tv.tv_usec = ((n % baud) * 1000000) / baud;
	    select(0, NULL, NULL, NULL, &tv);
	}
    }
#endif

    settyinfo(&ti);
}

static char *kungetbuf;
static int kungetct, kungetsz;

void ungetkey(ch)		/**/
int ch;
{
    if (kungetct == kungetsz)
	kungetbuf = realloc(kungetbuf, kungetsz *= 2);
    kungetbuf[kungetct++] = ch;
}

void ungetkeys(s, len)		/**/
char *s;
int len;
{
    s += len;
    while (len--)
	ungetkey(*--s);
}

#if defined(pyr) && defined(HAS_SELECT)
static int breakread(fd, buf, n)
int fd, n;
char *buf;
{
    fd_set f;

    FD_ZERO(&f);
    FD_SET(fd, &f);
    return (select(fd + 1, (SELECT_ARG_2_T) & f, NULL, NULL, NULL) == -1 ?
	    EOF : read(fd, buf, n));
}

#define read    breakread
#endif

int getkey(keytmout)		/**/
int keytmout;
{
    char cc;
    unsigned int ret;
    long exp100ths;
    int die = 0, r;
    int old_errno = errno;

#ifdef HAS_SELECT
    fd_set foofd;

#else
#ifdef HAS_TIO
    struct ttyinfo ti;

#endif
#endif

    if (kungetct)
	ret = (int)(unsigned char)kungetbuf[--kungetct];
    else {
	if (keytmout) {
	    if (keytimeout > 500)
		exp100ths = 500;
	    else if (keytimeout > 0)
		exp100ths = keytimeout;
	    else
		exp100ths = 0;
#ifdef HAS_SELECT
	    if (exp100ths) {
		struct timeval expire_tv;

		expire_tv.tv_sec = exp100ths / 100;
		expire_tv.tv_usec = (exp100ths % 100) * 10000L;
		FD_ZERO(&foofd);
		FD_SET(0, &foofd);
		if (select(1, (SELECT_ARG_2_T) & foofd, NULL, NULL, &expire_tv) <= 0)
		    return EOF;
	    }
#else
#ifdef HAS_TIO
	    ti = shttyinfo;
	    ti.tio.c_lflag &= ~ICANON;
	    ti.tio.c_cc[VMIN] = 0;
	    ti.tio.c_cc[VTIME] = exp100ths / 10;
	    ioctl(SHTTY, TCSETA, &ti.tio);
	    r = read(0, &cc, 1);
	    ioctl(SHTTY, TCSETA, &shttyinfo.tio);
	    return (r <= 0) ? -1 : cc;
#endif
#endif
	}
	while ((r = read(0, &cc, 1)) != 1) {
	    if (r == 0) {
		if (isset(IGNOREEOF))
		    continue;
		stopmsg = 1;
		zexit(1);
	    }
	    if (errno == EINTR) {
		die = 0;
		if (!errflag)
		    continue;
		errflag = 0;
		errno = old_errno;
		return EOF;
	    } else if (errno == EWOULDBLOCK) {
		fcntl(0, F_SETFL, 0);
	    } else if (errno == EIO && !die) {
		ret = jobbingv;
		jobbingv = OPT_SET;
		attachtty(mypgrp);
		refresh();	/* kludge! */
		jobbingv = ret;
		die = 1;
	    } else if (errno != 0) {
		zerr("error on TTY read: %e", NULL, errno);
		stopmsg = 1;
		zexit(1);
	    }
	}
	if (cc == '\r')		/* undo the exchange of \n and \r determined by */
	    cc = '\n';		/* setterm() */
	else if (cc == '\n')
	    cc = '\r';

	ret = (int)(unsigned char)cc;
    }
    if (vichgflag) {
	if (vichgbufptr == vichgbufsz)
	    vichgbuf = realloc(vichgbuf, vichgbufsz *= 2);
	vichgbuf[vichgbufptr++] = ret;
    }
    errno = old_errno;
    return ret;
}

/* read a line */

unsigned char *zleread(ppt, ppt2, plen, p2len)	/**/
unsigned char *ppt;
unsigned char *ppt2;
int plen;
int p2len;
{
    int z;
    unsigned char *s;
    int old_errno = errno;

#ifdef HAS_SELECT
    long costmult = (baud) ? 3840000L / baud : 0;
    struct timeval tv;
    fd_set foofd;

    tv.tv_sec = 0;
#endif

    fflush(stdout);
    fflush(stderr);
    intr();
    insmode = unset(OVERSTRIKE);
    eofsent = 0;
    resetneeded = 0;
    pmpt = (char *)ppt;
    pmpt2 = (char *)ppt2;
    pptlen = plen;
    ppt2len = p2len;
    permalloc();
    histline = curhist;
#ifdef HAS_SELECT
    FD_ZERO(&foofd);
#endif
    undoing = 1;
    line = (unsigned char *)zalloc((linesz = 256) + 1);
    *line = '\0';
    virangeflag = lastcmd = done = cs = ll = mark = 0;
    curhistline = NULL;
    mult = 1;
    vibufspec = 0;
    bindtab = mainbindtab;
    addedsuffix = complexpect = vichgflag = 0;
    viinsbegin = 0;
    statusline = NULL;
    if ((s = (unsigned char *)getnode(bufstack))) {
	setline((char *)s);
	zsfree((char *)s);
	if (stackcs != -1) {
	    cs = stackcs;
	    stackcs = -1;
	    if (cs > ll)
		cs = ll;
	}
	if (stackhist != -1) {
	    histline = stackhist;
	    stackhist = -1;
	}
    }
    initundo();
    if (unset(NOPROMPTCR))
	putchar('\r');
    if (tmout)
	alarm(tmout);
    zleactive = 1;
    resetneeded = 1;
    refresh();
    errflag = retflag = 0;
    while (!done && !errflag) {
	struct zlecmd *zc;

	statusline = NULL;
	bindk = getkeycmd();
	if (!ll && c == eofchar) {
	    eofsent = 1;
	    break;
	}
	if (bindk != -1) {
	    int ce = complexpect;

	    zc = zlecmds + bindk;
	    if (!(lastcmd & ZLE_ARG))
		mult = 1;
	    if ((lastcmd & ZLE_UNDO) != (zc->flags & ZLE_UNDO) && undoing)
		addundo();
	    if (bindk != z_sendstring) {
		if (!(zc->flags & ZLE_MENUCMP) && menucmp)
		    menucmp = 0;
		if (!(zc->flags & ZLE_MENUCMP) &&
		    addedsuffix && !(zc->flags & ZLE_DELETE) &&
		    !((zc->flags & ZLE_INSERT) && c != ' ')) {
		    backdel(addedsuffix);
		}
		if (!menucmp)
		    addedsuffix = 0;
	    }
	    if (zc->func)
		(*zc->func) ();
	    if (ce == complexpect && ce)
		complexpect = 0;
	    if (bindk != z_sendstring)
		lastcmd = zc->flags;
	    if (!(lastcmd & ZLE_UNDO) && undoing)
		addundo();
	} else {
	    errflag = 1;
	    break;
	}
#ifdef HAS_SELECT
	if (baud && (!lastcmd & ZLE_MENUCMP)) {
	    FD_SET(0, &foofd);
	    if ((tv.tv_usec = cost * costmult) > 500000)
		tv.tv_usec = 500000;
	    if (!kungetct && select(1, (SELECT_ARG_2_T) & foofd, NULL, NULL, &tv) <= 0)
		refresh();
	} else
#endif
	if (!kungetct)
	    refresh();
    }
    statusline = NULL;
    trashzle();
    zleactive = 0;
    alarm(0);
    z = strlen(UTOSCP(line));
    line[z] = '\n';
    line[z + 1] = 0;
    heapalloc();
    zsfree(curhistline);
    if (eofsent) {
	free(line);
	line = NULL;
    }
    free(lastline);		/* freeundo */
    errno = old_errno;
    return line;
}

int getkeycmd()
{				/**/
    int ret;
    static int hops = 0;

    cky = NULL;

    if ((c = getkey(0)) < 0)
	return -1;
    if ((ret = bindtab[c]) == z_sequenceleadin) {
	int lastlen = 0, t0 = 1, buflen = 50;
	Key ky;
	char *buf;

	buf = (char *)zalloc(buflen);
	ungetok = 0;
	buf[0] = c, buf[1] = '\0';
	if ((cky = (Key) gethnode(buf, xbindtab))->func == z_undefinedkey)
	    cky = NULL;
	else
	    lastlen = 1;
	if (!c)
	    buf[0] = (char)0x80;
	for (;;) {
	    if ((c = getkey(cky ? 1 : 0)) >= 0) {
		if (t0 == buflen - 1)
		    buf = (char *)realloc(buf, buflen *= 2);
		buf[t0++] = (c) ? c : 0x80;
		buf[t0] = '\0';
		ky = (Key) gethnode(buf, xbindtab);
	    } else
		ky = NULL;
	    if (ky) {
		if (ky->func == z_undefinedkey)
		    continue;
		cky = ky;
		if (!ky->prefixct) {
		    ret = ky->func;
		    break;
		}
		lastlen = t0;
	    } else if (cky) {
		ungetkeys(&buf[lastlen], t0 - lastlen);
		ret = cky->func;
		c = buf[lastlen - 1];
		break;
	    } else
		return z_undefinedkey;
	}
	zfree(buf, buflen);
    } else
	ungetok = 1;		/* for doisearch() */
    if (ret == z_vidigitorbeginningofline)
	ret = (lastcmd & ZLE_ARG) ? z_digitargument : z_beginningofline;
    else if (ret == z_executenamedcmd && !statusline)
	ret = executenamedcommand();
    else if (ret == z_executelastnamedcmd)
	ret = lastnamed;
    else if (ret == z_sendstring) {
#define MAXHOPS 20
	if (++hops == MAXHOPS) {
	    zerr("string inserting another one too many times", NULL, 0);
	    hops = 0;
	    return -1;
	}
    } else
	hops = 0;
    return ret;
}

void sendstring()
{				/**/
    if (!cky) {
	char buf[2];

	buf[0] = c;
	buf[1] = '\0';
	cky = (Key) gethnode(buf, xbindtab);
    }
    ungetkeys(cky->str, cky->len);
}

Key makefunckey(fun)		/**/
int fun;
{
    Key ky = (Key) zcalloc(sizeof *ky);

    ky->func = fun;
    return ky;
}

/* initialize the bindings */

void initxbindtab()
{				/**/
    int t0, vi = 0;
    char buf[3], *s;
    Key ky;

    lastnamed = z_undefinedkey;
    for (t0 = 0; t0 != 32; t0++)
	viins_cur_bindtab[t0] = viinsbind[t0];
    for (t0 = 32; t0 != 256; t0++)
	viins_cur_bindtab[t0] = z_selfinsert;
    viins_cur_bindtab[127] = z_backwarddeletechar;
    for (t0 = 0; t0 != 128; t0++)
	emacs_cur_bindtab[t0] = emacsbind[t0];
    for (t0 = 128; t0 != 256; t0++)
	emacs_cur_bindtab[t0] = z_selfinsert;
    em_xbindtab = newhtable(67);
    vi_xbindtab = newhtable(20);
    if ((s = zgetenv("VISUAL"))) {
	if (ztrstr(s, "vi"))
	    vi = 1;
    } else if ((s = zgetenv("EDITOR")) && ztrstr(s, "vi"))
	vi = 1;
    if (vi) {
	mainbindtab = viins_cur_bindtab;
	xbindtab = vi_xbindtab;
    } else {
	mainbindtab = emacs_cur_bindtab;
	xbindtab = em_xbindtab;
    }
    for (t0 = 0200; t0 != 0240; t0++)
	emacs_cur_bindtab[t0] = viins_cur_bindtab[t0] = z_undefinedkey;
    for (t0 = 0; t0 != 128; t0++)
	altbindtab[t0] = vicmdbind[t0];
    for (t0 = 128; t0 != 256; t0++)
	altbindtab[t0] = emacsbind[t0];
    bindtab = mainbindtab;
    if (!kungetbuf)
	kungetbuf = (char *)zalloc(kungetsz = 32);

    addhnode(ztrdup("\33\133"), ky = makefunckey(z_undefinedkey), em_xbindtab, 0);
    ky->prefixct = 4;
    addhnode(ztrdup("\33\133C"), makefunckey(z_forwardchar), em_xbindtab, 0);
    addhnode(ztrdup("\33\133D"), makefunckey(z_backwardchar), em_xbindtab, 0);
    addhnode(ztrdup("\33\133A"), makefunckey(z_uplineorhistory), em_xbindtab, 0);
    addhnode(ztrdup("\33\133B"), makefunckey(z_downlineorhistory), em_xbindtab, 0);
    addhnode(ztrdup("\33"), ky = makefunckey(z_vicmdmode), vi_xbindtab, 0);
    ky->prefixct = 4;
    addhnode(ztrdup("\33\133"), ky = makefunckey(z_undefinedkey), vi_xbindtab, 0);
    ky->prefixct = 4;
    addhnode(ztrdup("\33\133C"), makefunckey(z_forwardchar), vi_xbindtab, 0);
    addhnode(ztrdup("\33\133D"), makefunckey(z_backwardchar), vi_xbindtab, 0);
    addhnode(ztrdup("\33\133A"), makefunckey(z_uplineorhistory), vi_xbindtab, 0);
    addhnode(ztrdup("\33\133B"), makefunckey(z_downlineorhistory), vi_xbindtab, 0);
    addhnode(ztrdup("\30"), ky = makefunckey(z_undefinedkey), em_xbindtab, 0);
    ky->prefixct = 15;
    addhnode(ztrdup("\30*"), makefunckey(z_expandword), em_xbindtab, (FFunc) 0);
    addhnode(ztrdup("\30g"), makefunckey(z_listexpand), em_xbindtab, (FFunc) 0);
    addhnode(ztrdup("\30G"), makefunckey(z_listexpand), em_xbindtab, (FFunc) 0);
    addhnode(ztrdup("\30\16"), makefunckey(z_infernexthistory), em_xbindtab, 0);
    addhnode(ztrdup("\30\13"), makefunckey(z_killbuffer), em_xbindtab, (FFunc) 0);
    addhnode(ztrdup("\30\6"), makefunckey(z_vifindnextchar), em_xbindtab, (FFunc) 0);
    addhnode(ztrdup("\30\17"), makefunckey(z_overwritemode), em_xbindtab, (FFunc) 0);
    addhnode(ztrdup("\30\25"), makefunckey(z_undo), em_xbindtab, (FFunc) 0);
    addhnode(ztrdup("\30\26"), makefunckey(z_vicmdmode), em_xbindtab, (FFunc) 0);
    addhnode(ztrdup("\30\12"), makefunckey(z_vijoin), em_xbindtab, (FFunc) 0);
    addhnode(ztrdup("\30\2"), makefunckey(z_vimatchbracket), em_xbindtab, (FFunc) 0);
    addhnode(ztrdup("\30s"), makefunckey(z_historyincrementalsearchforward),
	     em_xbindtab, (FFunc) 0);
    addhnode(ztrdup("\30r"), makefunckey(z_historyincrementalsearchbackward),
	     em_xbindtab, (FFunc) 0);
    addhnode(ztrdup("\30u"), makefunckey(z_undo), em_xbindtab, (FFunc) 0);
    addhnode(ztrdup("\30\30"), makefunckey(z_exchangepointandmark), em_xbindtab, 0);
    addhnode(ztrdup("\33"), ky = makefunckey(z_undefinedkey), em_xbindtab, 0);
    ky->prefixct = 4;

    strcpy(buf, "\33q");
    for (t0 = 128; t0 != 256; t0++)
	if (emacsbind[t0] != z_undefinedkey) {
	    buf[1] = t0 & 0x7f;
	    addhnode(ztrdup(buf), makefunckey(emacsbind[t0]), em_xbindtab, 0);
	    ky->prefixct++;
	}
    stackhist = stackcs = -1;
}

char *getkeystring(s, len, fromwhere, misc)	/**/
char *s;
int *len;
int fromwhere;
int *misc;
{
    char *buf = ((fromwhere == 2)
		 ? zalloc(strlen(s) + 1) : alloc(strlen(s) + 1));
    char *t = buf, *u = NULL;
    char svchar = '\0';
    int meta = 0, control = 0;

    for (; *s; s++) {
	if (*s == '\\' && s[1])
	    switch (*++s) {
	    case 'a':
#ifdef __STDC__
		*t++ = '\a';
#else
		*t++ = '\07';
#endif
		break;
	    case 'n':
		*t++ = '\n';
		break;
	    case 'b':
		*t++ = '\010';
		break;
	    case 't':
		*t++ = '\t';
		break;
	    case 'v':
		*t++ = '\v';
		break;
	    case 'f':
		*t++ = '\f';
		break;
	    case 'r':
		*t++ = '\r';
		break;
	    case 'E':
		if (!fromwhere) {
		    *t++ = '\\', s--;
		    continue;
		}
	    case 'e':
		*t++ = '\033';
		break;
	    case 'M':
		if (fromwhere) {
		    if (s[1] == '-')
			s++;
		    meta = 1 + control;	/* preserve the order of ^ and meta */
		} else
		    *t++ = '\\', s--;
		continue;
	    case 'C':
		if (fromwhere) {
		    if (s[1] == '-')
			s++;
		    control = 1;
		} else
		    *t++ = '\\', s--;
		continue;
	    case 'c':
		if (fromwhere < 2) {
		    *misc = 1;
		    break;
		}
	    default:
		if ((idigit(*s) && *s < '8') || *s == 'x') {
		    if (!fromwhere)
			if (*s == '0')
			    s++;
			else if (*s != 'x') {
			    *t++ = '\\', s--;
			    continue;
			}
		    if (s[1] && s[2] && s[3]) {
			svchar = s[3];
			s[3] = '\0';
			u = s;
		    }
		    *t++ = zstrtol(s + (*s == 'x'), &s,
				   (*s == 'x') ? 16 : 8);
		    if (svchar) {
			u[3] = svchar;
			svchar = '\0';
		    }
		    s--;
		} else {
		    if (!fromwhere && *s != '\\')
			*t++ = '\\';
		    *t++ = *s;
		}
		break;
	} else if (*s == '^' && fromwhere == 2) {
	    control = 1;
	    continue;
	} else
	    *t++ = *s;
	if (meta == 2) {
	    t[-1] |= 0x80;
	    meta = 0;
	}
	if (control) {
	    if (t[-1] == '?')
		t[-1] = 0x7f;
	    else
		t[-1] &= 0x9f;
	    control = 0;
	}
	if (meta) {
	    t[-1] |= 0x80;
	    meta = 0;
	}
    }
    *t = '\0';
    *len = t - buf;
    return buf;
}

void printbind(s, len)		/**/
char *s;
int len;
{
    int ch;

    while (len--) {
	ch = (unsigned char)*s++;
	if (ch & 0x80) {
	    printf("\\M-");
	    ch &= 0x7f;
	}
	if (icntrl(ch))
	    switch (ch) {
	    case 0x7f:
		printf("^?");
		break;
	    default:
		printf("^%c", (ch | 0x40));
		break;
	} else
	    putchar(ch);
    }
}

void printbinding(str, k)	/**/
char *str;
Key k;
{
    int len;

    if (k->func == z_undefinedkey)
	return;
    putchar('\"');
    printbind(str, (len = strlen(str)) ? len : 1);
    printf("\"\t");
    if (k->func == z_sendstring) {
	putchar('\"');
	printbind(k->str, k->len);
	printf("\"\n");
    } else
	printf("%s\n", zlecmds[k->func].name);
}

int bin_bindkey(name, argv, ops, junc)	/**/
char *name;
char **argv;
char *ops;
int junc;
{
    int i, *tab;

    if (ops['v'] && ops['e']) {
	zerrnam(name, "incompatible options", NULL, 0);
	return 1;
    }
    if (ops['v'] || ops['e'] || ops['d'] || ops['m']) {
	if (*argv) {
	    zerrnam(name, "too many arguments", NULL, 0);
	    return 1;
	}
	if (ops['d']) {
	    freehtab(em_xbindtab, freekey);
	    freehtab(vi_xbindtab, freekey);
	    initxbindtab();
	}
	if (ops['e']) {
	    mainbindtab = emacs_cur_bindtab;
	    xbindtab = em_xbindtab;
	} else if (ops['v']) {
	    mainbindtab = viins_cur_bindtab;
	    xbindtab = vi_xbindtab;
	}
	if (ops['m'])
	    for (i = 128; i != 256; i++)
		if (mainbindtab[i] == z_selfinsert)
		    mainbindtab[i] = emacsbind[i];
	return 0;
    }
    tab = (ops['a']) ? altbindtab : mainbindtab;
    if (!*argv) {
	char buf[2];

	buf[1] = '\0';
	for (i = 0; i != 256; i++) {
	    buf[0] = i;
	    putchar('\"');
	    printbind(buf, 1);
	    if (i < 254 && tab[i] == tab[i + 1] && tab[i] == tab[i + 2]) {
		printf("\" to \"");
		while (tab[i] == tab[i + 1])
		    i++;
		buf[0] = i;
		printbind(buf, 1);
	    }
	    printf("\"\t%s\n", zlecmds[tab[i]].name);
	}
	listhtable(xbindtab, (HFunc) printbinding);
	return 0;
    }
    while (*argv) {
	Key ky = NULL, cur = NULL;
	char *s;
	int func, len, firstzero = 0;

	s = getkeystring(*argv++, &len, 2, NULL);
	if (len > 1) {
	    if (s[0])
		firstzero = 0;
	    else
		firstzero = 1;
	    for (i = 0; i < len; i++)
		if (!s[i])
		    s[i] = (char)0x80;
	}
	if (!*argv || ops['r']) {
	    if (len == 1)
		func = tab[(unsigned char)*s];
	    else
		func = (ky = (Key) gethnode(s, xbindtab)) ? ky->func
		    : z_undefinedkey;
	    if (func == z_undefinedkey) {
		zerrnam(name, "in-string is not bound", NULL, 0);
		zfree(s, len);
		return 1;
	    }
	    if (ops['r']) {
		if (len == 1 && func != z_sequenceleadin) {
		    tab[(unsigned char)*s] = z_undefinedkey;
		    if (func == z_sendstring)
			free(remhnode(s, xbindtab));
		} else {
		    if (ky->prefixct) {
			if (ky->func == z_sendstring)
			    zfree(ky->str, ky->len);
			ky->func = z_undefinedkey;
		    } else
			free(remhnode(s, xbindtab));
		    if (len > 1) {
			s[--len] = '\0';
			while (len > 1) {
			    (ky = (Key) gethnode(s, xbindtab))->prefixct--;
			    if (!ky->prefixct && ky->func == z_undefinedkey)
				free(remhnode(s, xbindtab));
			    s[--len] = '\0';
			}
			(ky = (Key) gethnode(s, xbindtab))->prefixct--;
			if (!ky->prefixct) {
			    tab[(unsigned char)*s] = ky->func;
			    if (ky->func != z_sendstring)
				free(remhnode(s, xbindtab));
			}
		    }
		}
		zfree(s, len);
		continue;
	    }
	    if (func == z_sendstring) {
		if (len == 1)
		    ky = (Key) gethnode(s, xbindtab);
		printbind(ky->str, ky->len);
		putchar('\n');
	    } else
		printf("%s\n", zlecmds[func].name);
	    zfree(s, len);
	    return 0;
	}
	if (!ops['s']) {
	    for (i = 0; i != ZLECMDCOUNT; i++)
		if (!strcmp(*argv, zlecmds[i].name))
		    break;
	    if (i == ZLECMDCOUNT) {
		zerr("undefined function: %s", *argv, 0);
		zfree(s, len);
		return 1;
	    }
	    func = i;
	} else
	    func = z_sendstring;

	if (len == 1 && tab[(unsigned char)*s] != z_sequenceleadin) {
	    if (ops['s']) {
		addhnode(ztrdup(s), cur = makefunckey(z_sendstring),
			 xbindtab, freekey);
	    } else if (tab[(unsigned char)*s] == z_sendstring)
		free(remhnode(s, xbindtab));
	    tab[(unsigned char)*s] = func;
	} else {
	    if (!(cur = (Key) gethnode(s, xbindtab))
		|| (cur->func == z_undefinedkey))
		for (i = len - 1; i > 0; i--) {
		    char sav;

		    sav = s[i];
		    s[i] = '\0';
		    if (i == 1 && firstzero)
			*s = '\0';
		    if (!(ky = (Key) gethnode(s, xbindtab)))
			addhnode(ztrdup(s), ky = makefunckey(z_undefinedkey),
				 xbindtab, freekey);
		    ky->prefixct++;
		    s[i] = sav;
		    if (i == 1 && firstzero)
			*s = (char)0x80;
		}
	    if (cur) {
		cur->func = func;
		zfree(cur->str, cur->len);
	    } else
		addhnode(ztrdup(s), cur = makefunckey(func), xbindtab, freekey);
	    if (firstzero)
		*s = 0;
	    if (tab[(unsigned char)*s] != z_sequenceleadin) {
		cur->func = tab[(unsigned char)*s];
		tab[(unsigned char)*s] = z_sequenceleadin;
	    }
	}
	if (ops['s']) {
	    cur->str = getkeystring(*argv, &cur->len, 2, NULL);
	    cur->str = (char *)realloc(cur->str, cur->len);
	}
	argv++;
	zfree(s, len);
    }
    return 0;
}

void freekey(x)			/**/
vptr x;
{
    Key k = (Key) x;

    if (k->str)
	zsfree(k->str);
    zfree(k, sizeof(struct key));
}

extern int clearflag;

void trashzle()
{				/**/
    if (zleactive) {
	refresh();
	moveto(nlnct, 0);
	if (clearflag && tccan(TCCLEAREOD)) {
	    tcout(TCCLEAREOD);
	    clearflag = 0;
	}
	printf("%s", postedit);
	fflush(stdout);
	resetneeded = 1;
	settyinfo(&shttyinfo);
    }
    if (errflag)
	kungetct = 0;
}
