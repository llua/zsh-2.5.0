/*
 *
 * math.c - mathematical expression evaluation
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

static char *ptr;

static long yyval;
static LV yylval;

static int mlevel = 0;

/* != 0 means recognize unary plus, minus, etc. */

static int unary = 1;

void mathparse DCLPROTO((int));

/* LR = left-to-right associativity
	RL = right-to-left associativity
	BOO = short-circuiting boolean */

#define LR 0
#define RL 1
#define BOOL 2

#define M_INPAR 0
#define M_OUTPAR 1
#define NOT 2
#define COMP 3
#define POSTPLUS 4
#define POSTMINUS 5
#define UPLUS 6
#define UMINUS 7
#define AND 8
#define XOR 9
#define OR 10
#define MUL 11
#define DIV 12
#define MOD 13
#define PLUS 14
#define MINUS 15
#define SHLEFT 16
#define SHRIGHT 17
#define LES 18
#define LEQ 19
#define GRE 20
#define GEQ 21
#define DEQ 22
#define NEQ 23
#define DAND 24
#define DOR 25
#define DXOR 26
#define QUEST 27
#define COLON 28
#define EQ 29
#define PLUSEQ 30
#define MINUSEQ 31
#define MULEQ 32
#define DIVEQ 33
#define MODEQ 34
#define ANDEQ 35
#define XOREQ 36
#define OREQ 37
#define SHLEFTEQ 38
#define SHRIGHTEQ 39
#define DANDEQ 40
#define DOREQ 41
#define DXOREQ 42
#define COMMA 43
#define EOI 44
#define PREPLUS 45
#define PREMINUS 46
#define NUM 47
#define ID 48
#define PARAM 49
#define POWER 50
#define CID 51
#define CPARAM 52
#define POWEREQ 53
#define TOKCOUNT 54

/* precedences */

static int prec[TOKCOUNT] =
{
    1, 137, 2, 2, 2,
    2, 2, 2, 4, 5,
    6, 7, 7, 7, 8,
    8, 3, 3, 9, 9,
    9, 9, 10, 10, 11,
    12, 12, 13, 13, 14,
    14, 14, 14, 14, 14,
    14, 14, 14, 14, 14,
    14, 14, 14, 15, 200,
    2, 2, 0, 0, 0,
    8, 0, 0, 14
};

#define TOPPREC 15
#define ARGPREC (15-1)

static int type[TOKCOUNT] =
{
    LR, LR, RL, RL, RL,
    RL, RL, RL, LR, LR,
    LR, LR, LR, LR, LR,
    LR, LR, LR, LR, LR,
    LR, LR, LR, LR, BOOL,
    BOOL, LR, RL, RL, RL,
    RL, RL, RL, RL, RL,
    RL, RL, RL, RL, RL,
    BOOL, BOOL, RL, RL, RL,
    RL, RL, LR, LR, LR,
    RL, LR, LR, RL
};

#define LVCOUNT 32

/* list of lvalues (variables) */

static int lvc;
static char *lvals[LVCOUNT];

int zzlex()
{				/**/
    int cct = 0;

    for (;; cct = 0)
	switch (*ptr++) {
	case '+':
	    if (*ptr == '+' && (unary || !ialnum(*ptr))) {
		ptr++;
		return (unary) ? PREPLUS : POSTPLUS;
	    }
	    if (*ptr == '=') {
		unary = 1;
		ptr++;
		return PLUSEQ;
	    }
	    return (unary) ? UPLUS : PLUS;
	case '-':
	    if (*ptr == '-' && (unary || !ialnum(*ptr))) {
		ptr++;
		return (unary) ? PREMINUS : POSTMINUS;
	    }
	    if (*ptr == '=') {
		unary = 1;
		ptr++;
		return MINUSEQ;
	    }
	    return (unary) ? UMINUS : MINUS;
	case '(':
	    unary = 1;
	    return M_INPAR;
	case ')':
	    return M_OUTPAR;
	case '!':
	    if (*ptr == '=') {
		unary = 1;
		ptr++;
		return NEQ;
	    }
	    return NOT;
	case '~':
	    return COMP;
	case '&':
	    unary = 1;
	    if (*ptr == '&') {
		if (*++ptr == '=') {
		    ptr++;
		    return DANDEQ;
		}
		return DAND;
	    } else if (*ptr == '=') {
		ptr++;
		return ANDEQ;
	    }
	    return AND;
	case '|':
	    unary = 1;
	    if (*ptr == '|') {
		if (*++ptr == '=') {
		    ptr++;
		    return DOREQ;
		}
		return DOR;
	    } else if (*ptr == '=') {
		ptr++;
		return OREQ;
	    }
	    return OR;
	case '^':
	    unary = 1;
	    if (*ptr == '^') {
		if (*++ptr == '=') {
		    ptr++;
		    return DXOREQ;
		}
		return DXOR;
	    } else if (*ptr == '=') {
		ptr++;
		return XOREQ;
	    }
	    return XOR;
	case '*':
	    unary = 1;
	    if (*ptr == '*') {
		if (*++ptr == '=') {
		    ptr++;
		    return POWEREQ;
		}
		return POWER;
	    }
	    if (*ptr == '=') {
		ptr++;
		return MULEQ;
	    }
	    return MUL;
	case '/':
	    unary = 1;
	    if (*ptr == '=') {
		ptr++;
		return DIVEQ;
	    }
	    return DIV;
	case '%':
	    unary = 1;
	    if (*ptr == '=') {
		ptr++;
		return MODEQ;
	    }
	    return MOD;
	case '<':
	    unary = 1;
	    if (*ptr == '<') {
		if (*++ptr == '=') {
		    ptr++;
		    return SHLEFTEQ;
		}
		return SHLEFT;
	    } else if (*ptr == '=') {
		ptr++;
		return LEQ;
	    }
	    return LES;
	case '>':
	    unary = 1;
	    if (*ptr == '>') {
		if (*++ptr == '=') {
		    ptr++;
		    return SHRIGHTEQ;
		}
		return SHRIGHT;
	    } else if (*ptr == '=') {
		ptr++;
		return GEQ;
	    }
	    return GRE;
	case '=':
	    unary = 1;
	    if (*ptr == '=') {
		ptr++;
		return DEQ;
	    }
	    return EQ;
	case '?':
	    unary = 1;
	    return QUEST;
	case ':':
	    unary = 1;
	    return COLON;
	case ',':
	    unary = 1;
	    return COMMA;
	case '\0':
	    unary = 1;
	    ptr--;
	    return EOI;
	case '[':
	    unary = 0;
	    {
		int base = zstrtol(ptr, &ptr, 10);

		if (*ptr == ']')
		    ptr++;
		yyval = zstrtol(ptr, &ptr, lastbase = base);
		return NUM;
	    }
	case ' ':
	case '\t':
	case '\n':
	    break;
	case '#':
	    if (*ptr == '\\') {
		ptr++;
		yyval = (long)*ptr++;
		return NUM;
	    } else
		cct = 1, *--ptr = '$', ptr++;
	/* fall through */
	default:
	    if (idigit(*--ptr)) {
		unary = 0;
		yyval = zstrtol(ptr, &ptr, 10);

		if (*ptr == '#') {
		    ptr++;
		    yyval = zstrtol(ptr, &ptr, lastbase = yyval);
		}
		return NUM;
	    }
	    if (iident(*ptr)) {
		char *p, q;

		p = ptr;
		if (lvc == LVCOUNT) {
		    zerr("too many identifiers (complain to author)", NULL, 0);
		    return EOI;
		}
		unary = 0;
		while (iident(*++ptr));
		q = *ptr;
		*ptr = '\0';
		lvals[yylval = lvc++] = ztrdup(p);
		*ptr = q;
		return cct ? CID : ID;
	    } else if (*ptr == '$') {
		char *p, t;
		int l;

		unary = 0;

		if (lvc == LVCOUNT) {
		    zerr("too many identifiers (complain to author)", NULL, 0);
		    return EOI;
		}
		p = ptr++;
		*p = String;

		if (p[1] == '{') {
		    for (ptr++, l = 0; *ptr && (*ptr != '}' || l); ptr++) {
			if (*ptr == '{')
			    l++;
			if (*ptr == '}')
			    l--;
			if (*ptr == '\\' && ptr[1])
			    ptr++;
		    }
		    if (*ptr) {
			ptr++;
			t = *ptr;
			*ptr = '\0';
			lvals[yylval = lvc++] = ztrdup(p);
			*ptr = t;
			*p = '$';
			return cct ? CPARAM : PARAM;
		    }
		    yyval = 0;
		    *p = '$';
		    return NUM;
		} else if (p[1] == '(') {
		    for (ptr++, l = 0; *ptr && (*ptr != ')' || l); ptr++) {
			if (*ptr == '(')
			    l++;
			if (*ptr == ')')
			    l--;
			if (*ptr == '\\' && ptr[1])
			    ptr++;
		    }
		    if (*ptr) {
			ptr++;
			t = *ptr;
			*ptr = '\0';
			p[1] = Inpar;
			ptr[-1] = Outpar;
			lvals[yylval = lvc++] = ztrdup(p);
			p[1] = '(';
			ptr[-1] = ')';
			*ptr = t;
			*p = '$';
			return cct ? CPARAM : PARAM;
		    }
		    yyval = 0;
		    *p = '$';
		    return NUM;
		}
		ptr++;
		while (iident(*ptr) && *ptr != '[' &&
		       *ptr != ']' && *ptr != ',')
		    ptr++;
		while (*ptr == '[') {
		    for (ptr++, l = 0; *ptr && (*ptr != ']' || l); ptr++) {
			if (*ptr == '[')
			    l++;
			if (*ptr == ']')
			    l--;
			if (*ptr == '\\' && ptr[1])
			    ptr++;
		    }
		    if (*ptr)
			ptr++;
		}
		if (p < ptr) {
		    t = *ptr;
		    *ptr = '\0';
		    lvals[yylval = lvc++] = ztrdup(p);
		    *ptr = t;
		    *p = '$';
		    return cct ? CPARAM : PARAM;
		}
		yyval = 0;
		*p = '$';
		return NUM;
	    }
	    return EOI;
	}
}

/* the value stack */

#define STACKSZ 100
int mtok;			/* last token */
int sp = -1;			/* stack pointer */
struct mathvalue {
    LV lval;
    long val;
}

stack[STACKSZ];

SPROTO(void push, (long val, LV lval));

static void push(val, lval)
long val;
LV lval;
{
    if (sp == STACKSZ - 1)
	zerr("stack overflow", NULL, 0);
    else
	sp++;
    stack[sp].val = val;
    stack[sp].lval = lval;
}

long getvar(s)			/**/
LV s;
{
    long t;

    if (!(t = getiparam(lvals[s])))
	return 0;
    return t;
}

long getcvar(s)			/**/
LV s;
{
    char *t;

    if (!(t = getsparam(lvals[s])))
	return 0;
    return (long)*t;
}

long setvar(s, v)		/**/
LV s;
long v;
{
    if (s == -1 || s >= lvc) {
	zerr("lvalue required", NULL, 0);
	return 0;
    }
    if (noeval)
	return v;
    setiparam(lvals[s], v);
    return v;
}

int notzero(a)			/**/
int a;
{
    if (a == 0) {
	zerr("division by zero", NULL, 0);
	return 0;
    }
    return 1;
}

#define pop2() { b = stack[sp--].val; a = stack[sp--].val; }
#define pop3() {c=stack[sp--].val;b=stack[sp--].val;a=stack[sp--].val;}
#define nolval() {stack[sp].lval= -1;}
#define pushv(X) { push(X,-1); }
#define pop2lv() { pop2() lv = stack[sp+1].lval; }
#define set(X) { push(setvar(lv,X),lv); }

void op(what)			/**/
int what;
{
    long a, b, c;
    LV lv;

    if (sp < 0) {
	zerr("bad math expression: stack empty", NULL, 0);
	return;
    }
    switch (what) {
    case NOT:
	stack[sp].val = !stack[sp].val;
	nolval();
	break;
    case COMP:
	stack[sp].val = ~stack[sp].val;
	nolval();
	break;
    case POSTPLUS:
	(void)setvar(stack[sp].lval, stack[sp].val + 1);
	break;
    case POSTMINUS:
	(void)setvar(stack[sp].lval, stack[sp].val - 1);
	break;
    case UPLUS:
	nolval();
	break;
    case UMINUS:
	stack[sp].val = -stack[sp].val;
	nolval();
	break;
    case AND:
	pop2();
	pushv(a & b);
	break;
    case XOR:
	pop2();
	pushv(a ^ b);
	break;
    case OR:
	pop2();
	pushv(a | b);
	break;
    case MUL:
	pop2();
	pushv(a * b);
	break;
    case DIV:
	pop2();
	if (notzero(b))
	    pushv(a / b);
	break;
    case MOD:
	pop2();
	if (notzero(b))
	    pushv(a % b);
	break;
    case PLUS:
	pop2();
	pushv(a + b);
	break;
    case MINUS:
	pop2();
	pushv(a - b);
	break;
    case SHLEFT:
	pop2();
	pushv(a << b);
	break;
    case SHRIGHT:
	pop2();
	pushv(a >> b);
	break;
    case LES:
	pop2();
	pushv((long)(a < b));
	break;
    case LEQ:
	pop2();
	pushv((long)(a <= b));
	break;
    case GRE:
	pop2();
	pushv((long)(a > b));
	break;
    case GEQ:
	pop2();
	pushv((long)(a >= b));
	break;
    case DEQ:
	pop2();
	pushv((long)(a == b));
	break;
    case NEQ:
	pop2();
	pushv((long)(a != b));
	break;
    case DAND:
	pop2();
	pushv((long)(a && b));
	break;
    case DOR:
	pop2();
	pushv((long)(a || b));
	break;
    case DXOR:
	pop2();
	pushv((long)((a && !b) || (!a && b)));
	break;
    case QUEST:
	pop3();
	pushv((a) ? b : c);
	break;
    case COLON:
	break;
    case EQ:
	b = stack[sp].val;
	sp -= 2;
	lv = stack[sp + 1].lval;
	set(b);
	break;
    case PLUSEQ:
	pop2lv();
	set(a + b);
	break;
    case MINUSEQ:
	pop2lv();
	set(a - b);
	break;
    case MULEQ:
	pop2lv();
	set(a * b);
	break;
    case DIVEQ:
	pop2lv();
	if (notzero(b))
	    set(a / b);
	break;
    case MODEQ:
	pop2lv();
	if (notzero(b))
	    set(a % b);
	break;
    case ANDEQ:
	pop2lv();
	set(a & b);
	break;
    case XOREQ:
	pop2lv();
	set(a ^ b);
	break;
    case OREQ:
	pop2lv();
	set(a | b);
	break;
    case SHLEFTEQ:
	pop2lv();
	set(a << b);
	break;
    case SHRIGHTEQ:
	pop2lv();
	set(a >> b);
	break;
    case DANDEQ:
	pop2lv();
	set((long)(a && b));
	break;
    case DOREQ:
	pop2lv();
	set((long)(a || b));
	break;
    case DXOREQ:
	pop2lv();
	set((long)((a && !b) || (!a && b)));
	break;
    case COMMA:
	b = stack[sp].val;
	sp -= 2;
	pushv(b);
	break;
    case PREPLUS:
	stack[sp].val = setvar(stack[sp].lval,
			       stack[sp].val + 1);
	break;
    case PREMINUS:
	stack[sp].val = setvar(stack[sp].lval,
			       stack[sp].val - 1);
	break;
    case POWER:
	pop2();
	if (b < 0) {
	    zerr("can't handle negative exponents", NULL, 0);
	    return;
	}
	for (c = 1; b--; c *= a);
	pushv(c);
	break;
    case POWEREQ:
	pop2lv();
	if (b < 0) {
	    zerr("can't handle negative exponents", NULL, 0);
	    return;
	}
	for (c = 1; b--; c *= a);
	set(c);
	break;
    default:
	zerr("out of integers", NULL, 0);
	return;
    }
}

void bop(tk)			/**/
int tk;
{
    switch (tk) {
    case DAND:
    case DANDEQ:
	if (!stack[sp].val)
	    noeval++;
	break;
    case DOR:
    case DOREQ:
	if (stack[sp].val)
	    noeval++;
	break;
    };
}

long mathevall(s, prek, ep)	/**/
char *s;
int prek;
char **ep;
{
    int t0;
    int xlastbase, xnoeval, xunary, xlvc;
    char *xptr;
    long xyyval;
    LV xyylval;
    char *xlvals[LVCOUNT];
    int xmtok, xsp;
    struct mathvalue xstack[STACKSZ];
    long ret;

    xlastbase = xnoeval = xunary = xlvc = xyyval = xyylval = xsp = xmtok = 0;
    xptr = NULL;
    if (mlevel++) {
	xlastbase = lastbase;
	xnoeval = noeval;
	xunary = unary;
	xlvc = lvc;
	xptr = ptr;
	xyyval = yyval;
	xyylval = yylval;
	memcpy(xlvals, lvals, LVCOUNT * sizeof(char *));

	xmtok = mtok;
	xsp = sp;
	memcpy(xstack, stack, STACKSZ * sizeof(struct mathvalue));
    }
    lastbase = -1;
    for (t0 = 0; t0 != LVCOUNT; t0++)
	lvals[t0] = NULL;
    lvc = 0;
    ptr = s;
    sp = -1;
    unary = 1;
    mathparse(prek);
    *ep = ptr;
    if (sp)
	zerr("bad math expression: unbalanced stack", NULL, 0);
    for (t0 = 0; t0 != lvc; t0++)
	zsfree(lvals[t0]);

    ret = stack[0].val;

    if (--mlevel) {
	lastbase = xlastbase;
	noeval = xnoeval;
	unary = xunary;
	lvc = xlvc;
	ptr = xptr;
	yyval = xyyval;
	yylval = xyylval;
	memcpy(lvals, xlvals, LVCOUNT * sizeof(char *));

	sp = xsp;
	mtok = xmtok;
	memcpy(stack, xstack, STACKSZ * sizeof(struct mathvalue));
    }
    return ret;
}

long matheval(s)		/**/
char *s;
{
    char *junk;
    long x;

    if (!*s)
	return 0;
    x = mathevall(s, TOPPREC, &junk);
    if (*junk)
	zerr("bad math expression: illegal character: %c", NULL, *junk);
    return x;
}

long mathevalarg(s, ss)		/**/
char *s;
char **ss;
{
    long x;

    x = mathevall(s, ARGPREC, ss);
    if (mtok == COMMA)
	(*ss)--;
    return x;
}

/* operator-precedence parse the string and execute */

void mathparse(pc)		/**/
int pc;
{
    if (errflag)
	return;
    mtok = zzlex();
    while (prec[mtok] <= pc) {
	if (errflag)
	    return;
	if (mtok == NUM)
	    push(yyval, -1);
	else if (mtok == ID)
	    push(getvar(yylval), yylval);
	else if (mtok == CID)
	    push(getcvar(yylval), yylval);
	else if (mtok == PARAM) {
	    char *p = lvals[yylval];

	    singsub(&p);
	    untokenize(p);
	    push(atol(p), -1);
	} else if (mtok == CPARAM) {
	    char *p = lvals[yylval];

	    singsub(&p);
	    untokenize(p);
	    push((long)*p, -1);
	} else if (mtok == M_INPAR) {
	    mathparse(TOPPREC);
	    if (mtok != M_OUTPAR) {
		if (!errflag)
		    zerr("')' expected", NULL, 0);
		return;
	    }
	} else if (mtok == QUEST) {
	    int q = stack[sp].val;

	    if (!q)
		noeval++;
	    mathparse(prec[QUEST] - 1);
	    if (!q)
		noeval--;
	    else
		noeval++;
	    mathparse(prec[QUEST]);
	    if (q)
		noeval--;
	    op(QUEST);
	    continue;
	} else {
	    int otok = mtok, onoeval = noeval;

	    if (type[otok] == BOOL)
		bop(otok);
	    mathparse(prec[otok] - (type[otok] != RL));
	    noeval = onoeval;
	    op(otok);
	    continue;
	}
	mtok = zzlex();
    }
}
