/*
 *
 * table.c - linked lists and hash tables
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

#define TABLE_C
#include "zsh.h"

/* get an empty linked list header */

Lklist newlist()
{				/**/
    Lklist list;

    list = (Lklist) alloc(sizeof *list);
    list->first = 0;
    list->last = (Lknode) list;
    return list;
}

/* get an empty hash table */

Hashtab newhtable(size)		/**/
int size;
{
    Hashtab ret;

    ret = (Hashtab) zcalloc(sizeof *ret);
    ret->hsize = size;
    ret->nodes = (Hashnode *) zcalloc(size * sizeof(Hashnode));
    return ret;
}

/* the hash function used by Chris Torek */
/* The Peter J. Weinberger hash function from the Dragon Book
 * used to be here but it
 * a) was slower than this
 * b) took 32-bit integers for granted
 *    b1) there are other integer widths
 *    b2) integer constant like 0xf0000000 is unsigned in ANSI C,
 *        signed with pcc
 * c) I _believe_ after some testing that this hashes better
 *    Jarkko Hietaniemi <Jarkko.Hietaniemi@hut.fi>
 */

unsigned hasher(s)		/**/
char *s;
{
    unsigned hash;

    for (hash = 0; *s; s++)
	hash = hash + (hash << 5) + *s;
 /* if hashing counted strings: (size_t, char *) pairs,
     * "+ 1" should be appended to the above line */

    return hash;
}

/* add a node to a hash table */

void addhnode(nam, dat, ht, freefunc)	/**/
char *nam;
vptr dat;
Hashtab ht;
FFunc freefunc;
{
    int hval = hasher(nam) % ht->hsize;
    struct hashnode **hp = ht->nodes + hval, *hn;

    for (; *hp; hp = &(*hp)->next)
	if (!strcmp((*hp)->nam, nam)) {
	    zsfree((*hp)->nam);
	    hn = (struct hashnode *)dat;
	    hn->next = (*hp)->next;
	    if (!freefunc)
		zerr("attempt to call NULL freefunc", NULL, 0);
	    else
		freefunc(*hp);
	    *hp = hn;
	    hn->nam = nam;
	    return;
	}
    hn = (Hashnode) dat;
    hn->nam = nam;
    hn->next = ht->nodes[hval];
    ht->nodes[hval] = hn;
    if (++ht->ct == ht->hsize * 4)
	expandhtab(ht);
}

/* add a node to command hash table */

void addhcmdnode(nam, pnam)	/**/
char *nam;
char **pnam;
{
    int hval = hasher(nam) % cmdnamtab->hsize;
    struct hashnode *hp = cmdnamtab->nodes[hval], *hn;
    Cmdnam cc;

    for (; hp; hp = hp->next)
	if (!strcmp(hp->nam, nam))
	    return;
    cc = (Cmdnam) zcalloc(sizeof *cc);
    cc->flags = EXCMD;
    cc->u.name = pnam;
    hn = (Hashnode) cc;
    hn->nam = ztrdup(nam);
    hn->next = cmdnamtab->nodes[hval];
    cmdnamtab->nodes[hval] = hn;
    if (++cmdnamtab->ct == cmdnamtab->hsize * 4)
	expandhtab(cmdnamtab);
}

/* expand hash tables when they get too many entries */

void expandhtab(ht)		/**/
Hashtab ht;
{
    struct hashnode **arr, **ha, *hn, *hp;
    int osize = ht->hsize, nsize = osize * 8, os = osize;

    ht->hsize = nsize;
    arr = ht->nodes;
    ht->nodes = (Hashnode *) zcalloc(nsize * sizeof(struct hashnode *));

    ht->ct = 0;

    for (ha = arr; osize; osize--, ha++)
	for (hn = *ha; hn;) {
	    hp = hn->next;
	    addhnode(hn->nam, (vptr) hn, ht, (FFunc) 0);
	    hn = hp;
	}
    zfree(arr, os * sizeof(struct hashnode *));
}

/* get an entry in a hash table */

vptr gethnode(nam, ht)		/**/
char *nam;
Hashtab ht;
{
    int hval = hasher(nam) % ht->hsize;
    struct hashnode *hn = ht->nodes[hval];

    for (; hn; hn = hn->next)
	if (!strcmp(hn->nam, nam))
	    return (vptr) hn;
    return NULL;
}

void freehtab(ht, freefunc)	/**/
Hashtab ht;
FFunc freefunc;
{
    int val;
    struct hashnode *hn, **hp = &ht->nodes[0], *next;

    for (val = ht->hsize; val; val--, hp++)
	for (hn = *hp; hn;) {
	    next = hn->next;
	    zsfree(hn->nam);
	    freefunc(hn);
	    hn = next;
	}
    zfree(ht->nodes, ht->hsize * sizeof(struct hashnode *));
    zfree(ht, sizeof(struct hashtab));
}

/* remove a hash table entry and return a pointer to it */

vptr remhnode(nam, ht)		/**/
char *nam;
Hashtab ht;
{
    int hval = hasher(nam) % ht->hsize;
    struct hashnode *hn = ht->nodes[hval], *hp;

    if (!hn)
	return NULL;
    if (!strcmp(hn->nam, nam)) {
	ht->nodes[hval] = hn->next;
	zsfree(hn->nam);
	ht->ct--;
	return (vptr) hn;
    }
    for (hp = hn, hn = hn->next; hn; hn = (hp = hn)->next)
	if (!strcmp(hn->nam, nam)) {
	    hp->next = hn->next;
	    zsfree(hn->nam);
	    ht->ct--;
	    return (vptr) hn;
	}
    return NULL;
}

/* insert a node in a linked list after 'llast' */

void insnode(list, llast, dat)	/**/
Lklist list;
Lknode llast;
vptr dat;
{
    Lknode tmp;

    tmp = llast->next;
    llast->next = (Lknode) alloc(sizeof *tmp);
    llast->next->last = llast;
    llast->next->dat = dat;
    llast->next->next = tmp;
    if (tmp)
	tmp->last = llast->next;
    else
	list->last = llast->next;
}

/* remove a node from a linked list */

vptr remnode(list, nd)		/**/
Lklist list;
Lknode nd;
{
    vptr dat;

    nd->last->next = nd->next;
    if (nd->next)
	nd->next->last = nd->last;
    else
	list->last = nd->last;
    dat = nd->dat;
    zfree(nd, sizeof(struct lknode));

    return dat;
}

/* remove a node from a linked list */

vptr uremnode(list, nd)		/**/
Lklist list;
Lknode nd;
{
    vptr dat;

    nd->last->next = nd->next;
    if (nd->next)
	nd->next->last = nd->last;
    else
	list->last = nd->last;
    dat = nd->dat;
    return dat;
}

/* delete a character in a string */

void chuck(str)			/**/
char *str;
{
    while ((str[0] = str[1]))
	str++;
}

/* get top node in a linked list */

vptr getnode(list)		/**/
Lklist list;
{
    vptr dat;
    Lknode node = list->first;

    if (!node)
	return NULL;
    dat = node->dat;
    list->first = node->next;
    if (node->next)
	node->next->last = (Lknode) list;
    else
	list->last = (Lknode) list;
    zfree(node, sizeof(struct lknode));

    return dat;
}

/* get top node in a linked list without freeing */

vptr ugetnode(list)		/**/
Lklist list;
{
    vptr dat;
    Lknode node = list->first;

    if (!node)
	return NULL;
    dat = node->dat;
    list->first = node->next;
    if (node->next)
	node->next->last = (Lknode) list;
    else
	list->last = (Lknode) list;
    return dat;
}

void freetable(tab, freefunc)	/**/
Lklist tab;
FFunc freefunc;
{
    Lknode node = tab->first, next;

    while (node) {
	next = node->next;
	if (freefunc)
	    freefunc(node->dat);
	zfree(node, sizeof(struct lknode));

	node = next;
    }
    zfree(tab, sizeof(struct lklist));
}

char *ztrstr(s, t)		/**/
char *s;
char *t;
{
    char *p1, *p2;

    for (; *s; s++) {
	for (p1 = s, p2 = t; *p2; p1++, p2++)
	    if (*p1 != *p2)
		break;
	if (!*p2)
	    return (char *)s;
    }
    return NULL;
}

/* insert a list in another list */

void inslist(l, where, x)	/**/
Lklist l;
Lknode where;
Lklist x;
{
    Lknode nx = where->next;

    if (!l->first)
	return;
    where->next = l->first;
    l->last->next = nx;
    l->first->last = where;
    if (nx)
	nx->last = l->last;
    else
	x->last = l->last;
}

int countnodes(x)		/**/
Lklist x;
{
    Lknode y;
    int ct = 0;

    for (y = firstnode(x); y; incnode(y), ct++);
    return ct;
}
