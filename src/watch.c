
/*
 *
 * watch.c - login/logout watching
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

static int wtabsz;
struct utmp *wtab;
static time_t lastutmpcheck;

/* get the time of login/logout for WATCH */

time_t getlogtime(u, inout)	/**/
struct utmp *u;
int inout;
{
    FILE *in;
    struct utmp uu;
    int first = 1;
    int srchlimit = 50;		/* max number of wtmp records to search */

    if (inout)
	return u->ut_time;
    if (!(in = fopen(WTMP_FILE, "r")))
	return time(NULL);
    fseek(in, 0, 2);
    do {
	if (fseek(in, ((first) ? -1 : -2) * sizeof(struct utmp), 1)) {
	    fclose(in);
	    return time(NULL);
	}
	first = 0;
	if (!fread(&uu, sizeof(struct utmp), 1, in)) {
	    fclose(in);
	    return time(NULL);
	}
	if (uu.ut_time < lastwatch || !srchlimit--) {
	    fclose(in);
	    return time(NULL);
	}
    }
    while (memcmp(&uu, u, sizeof(uu)));

    do
	if (!fread(&uu, sizeof(struct utmp), 1, in)) {
	    fclose(in);
	    return time(NULL);
	}
    while (strncmp(uu.ut_line, u->ut_line, sizeof(u->ut_line)));
    fclose(in);
    return uu.ut_time;
}

/* Mutually recursive call to handle ternaries in watchfmt */

#define BEGIN3	'('
#define END3	')'

char *watch3ary(inout, u, fmt, prnt)	/**/
int inout;
struct utmp *u;
char *fmt;
int prnt;
{
    int truth = 1, sep;

    switch (*fmt++) {
    case 'n':
	truth = (u->ut_name[0] != 0);
	break;
    case 'a':
	truth = inout;
	break;
    case 'l':
	if (!strncmp(u->ut_line, "tty", 3))
	    truth = (u->ut_line[3] != 0);
	else
	    truth = (u->ut_line[0] != 0);
	break;
#ifdef UTMP_HOST
    case 'm':
    case 'M':
	truth = (u->ut_host[0] != 0);
	break;
#endif
    default:
	prnt = 0;		/* Skip unknown conditionals entirely */
	break;
    }
    sep = *fmt++;
    fmt = watchlog2(inout, u, fmt, (truth && prnt), sep);
    return watchlog2(inout, u, fmt, (!truth && prnt), END3);
}

/* print a login/logout event */

char *watchlog2(inout, u, fmt, prnt, fini)	/**/
int inout;
struct utmp *u;
char *fmt;
int prnt;
int fini;
{
    char *p, buf[40], *bf;
    int i;
    time_t timet;
    struct tm *tm;
    char *fm2;

    while (*fmt)
	if (*fmt == '\\')
	    if (*++fmt) {
		if (prnt)
		    putchar(*fmt);
		++fmt;
	    } else if (fini)
		return fmt;
	    else
		break;
	else if (*fmt == fini)
	    return ++fmt;
	else if (*fmt != '%') {
	    if (prnt)
		putchar(*fmt);
	    ++fmt;
	} else {
	    if (*++fmt == BEGIN3)
		fmt = watch3ary(inout, u, ++fmt, prnt);
	    else if (!prnt)
		++fmt;
	    else
		switch (*(fm2 = fmt++)) {
		case 'n':
		    printf("%.*s", (int)sizeof(u->ut_name), u->ut_name);
		    break;
		case 'a':
		    printf("%s", (!inout) ? "logged off" : "logged on");
		    break;
		case 'l':
		    if (!strncmp(u->ut_line, "tty", 3))
			printf("%.*s", (int)sizeof(u->ut_line) - 3, u->ut_line + 3);
		    else
			printf("%.*s", (int)sizeof(u->ut_line), u->ut_line);
		    break;
#ifdef UTMP_HOST
		case 'm':
		    for (p = u->ut_host, i = sizeof(u->ut_host); i && *p; i--, p++) {
			if (*p == '.' && !idigit(p[1]))
			    break;
			putchar(*p);
		    }
		    break;
		case 'M':
		    printf("%.*s", (int)sizeof(u->ut_host), u->ut_host);
		    break;
#endif
		case 'T':
		case 't':
		case '@':
		case 'W':
		case 'w':
		case 'D':
		    switch (*fm2) {
		    case '@':
		    case 't':
			fm2 = "%l:%M%p";
			break;
		    case 'T':
			fm2 = "%k:%M";
			break;
		    case 'w':
			fm2 = "%a %e";
			break;
		    case 'W':
			fm2 = "%m/%d/%y";
			break;
		    case 'D':
			fm2 = "%y-%m-%d";
			break;
		    }
		    timet = getlogtime(u, inout);
		    tm = localtime(&timet);
		    ztrftime(buf, 40, fm2, tm);
		    printf("%s", (*buf == ' ') ? buf + 1 : buf);
		    break;
		case '%':
		    putchar('%');
		    break;
		case 'S':
		case 's':
		case 'B':
		case 'b':
		case 'U':
		case 'u':
		    switch (*fm2) {
		    case 'S':
			fm2 = "so";
			break;
		    case 's':
			fm2 = "se";
			break;
		    case 'B':
			fm2 = "md";
			break;
		    case 'b':
			fm2 = "me";
			break;
		    case 'U':
			fm2 = "us";
			break;
		    case 'u':
			fm2 = "ue";
			break;
		    }
		    bf = buf;
		    if (tgetstr(fm2, &bf))
			tputs(buf, 1, putraw);
		    break;
		default:
		    putchar('%');
		    putchar(*fm2);
		    break;
		}
	}
    if (prnt)
	putchar('\n');

    return fmt;
}

/* check the List for login/logouts */

void watchlog(inout, u, w, fmt)	/**/
int inout;
struct utmp *u;
char **w;
char *fmt;
{
    char *v, *vv, sav;
    int bad;

    if (*w && !strcmp(*w, "all")) {
	(void)watchlog2(inout, u, fmt, 1, 0);
	return;
    }
    if (*w && !strcmp(*w, "notme") &&
	strncmp(u->ut_name, username, sizeof(u->ut_name))) {
	(void)watchlog2(inout, u, fmt, 1, 0);
	return;
    }
    for (; *w; w++) {
	bad = 0;
	v = *w;
	if (*v != '@' && *v != '%') {
	    for (vv = v; *vv && *vv != '@' && *vv != '%'; vv++);
	    sav = *vv;
	    *vv = '\0';
	    if (strncmp(u->ut_name, v, sizeof(u->ut_name)))
		bad = 1;
	    *vv = sav;
	    v = vv;
	}
	for (;;)
	    if (*v == '%') {
		for (vv = ++v; *vv && *vv != '@'; vv++);
		sav = *vv;
		*vv = '\0';
		if (strncmp(u->ut_line, v, sizeof(u->ut_line)))
		    bad = 1;
		*vv = sav;
		v = vv;
	    }
#ifdef UTMP_HOST
	    else if (*v == '@') {
		for (vv = ++v; *vv && *vv != '%'; vv++);
		sav = *vv;
		*vv = '\0';
		if (strncmp(u->ut_host, v, strlen(v)))
		    bad = 1;
		*vv = sav;
		v = vv;
	    }
#endif
	    else
		break;
	if (!bad) {
	    (void)watchlog2(inout, u, fmt, 1, 0);
	    return;
	}
    }
}

/* compare 2 utmp entries */

int ucmp(u, v)			/**/
struct utmp *u;
struct utmp *v;
{
    if (u->ut_time == v->ut_time)
	return strncmp(u->ut_line, v->ut_line, sizeof(u->ut_line));
    return u->ut_time - v->ut_time;
}

/* initialize the user List */

void readwtab()
{				/**/
    struct utmp *uptr;
    int wtabmax = 32;
    FILE *in;

    wtabsz = 0;
    if (!(in = fopen(UTMP_FILE, "r")))
	return;
    uptr = wtab = (struct utmp *)zalloc(wtabmax * sizeof(struct utmp));
    while (fread(uptr, sizeof(struct utmp), 1, in))
#ifdef USER_PROCESS
	if   (uptr->ut_type == USER_PROCESS)
#else
	if   (uptr->ut_name[0])
#endif
	{
	    uptr++;
	    if (++wtabsz == wtabmax)
		uptr = (wtab = (struct utmp *)realloc((vptr) wtab, (wtabmax *= 2) *
						      sizeof(struct utmp))) + wtabsz;
	}
    fclose(in);

    if (wtabsz)
	qsort((vptr) wtab, wtabsz, sizeof(struct utmp),
	           (int (*)DCLPROTO((const void *, const void *)))ucmp);
}

/* check for login/logout events; executed before each prompt
	if WATCH is set */

void dowatch()
{				/**/
    char **s = watch;
    char *fmt = (watchfmt) ? watchfmt : DEFWATCHFMT;
    FILE *in;
    int utabsz = 0, utabmax = wtabsz + 4, uct, wct;
    struct utmp *utab, *uptr, *wptr;
    struct stat st;

    holdintr();
    if (!fmt)
	fmt = "%n has %a %l from %m.";
    if (!wtab) {
	readwtab();
	noholdintr();
	return;
    }
    if ((stat(UTMP_FILE, &st) == -1) || (st.st_mtime <= lastutmpcheck)) {
	noholdintr();
	return;
    }
    lastutmpcheck = st.st_mtime;
    uptr = utab = (struct utmp *)zalloc(utabmax * sizeof(struct utmp));

    if (!(in = fopen(UTMP_FILE, "r"))) {
	free(utab);
	noholdintr();
	return;
    }
    while (fread(uptr, sizeof *uptr, 1, in))
#ifdef USER_PROCESS
	if (uptr->ut_type == USER_PROCESS)
#else
	if (uptr->ut_name[0])
#endif
	{
	    uptr++;
	    if (++utabsz == utabmax)
		uptr = (utab = (struct utmp *)realloc((vptr) utab, (utabmax *= 2) *
						      sizeof(struct utmp))) + utabsz;
	}
    fclose(in);
    noholdintr();
    if (errflag) {
	free(utab);
	return;
    }
    if (utabsz)
	qsort((vptr) utab, utabsz, sizeof(struct utmp),
	           (int (*)DCLPROTO((const void *, const void *)))ucmp);

    wct = wtabsz;
    uct = utabsz;
    uptr = utab;
    wptr = wtab;
    if (errflag) {
	free(utab);
	return;
    }
    while ((uct || wct) && !errflag)
	if (!uct || (wct && ucmp(uptr, wptr) > 0))
	    wct--, watchlog(0, wptr++, s, fmt);
	else if (!wct || (uct && ucmp(uptr, wptr) < 0))
	    uct--, watchlog(1, uptr++, s, fmt);
	else
	    uptr++, wptr++, wct--, uct--;
    free(wtab);
    wtab = utab;
    wtabsz = utabsz;
    fflush(stdout);
}

int bin_log(nam, argv, ops, func)	/**/
char *nam;
char **argv;
char *ops;
int func;
{
    if (!watch)
	return 1;
    if (wtab)
	free(wtab);
    wtab = (struct utmp *)zalloc(1);
    wtabsz = 0;
    lastutmpcheck = 0;
    dowatch();
    return 0;
}
