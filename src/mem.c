/*
 *
 * mem.c - memory management
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

vptr(*alloc) DCLPROTO((int));
vptr(*ncalloc) DCLPROTO((int));

/*

	There are two ways to allocate memory in zsh.  The first way is
	to call zalloc/zcalloc, which call malloc/calloc directly.  It
	is legal to call realloc() or free() on memory allocated this way.
	The second way is to call halloc/hcalloc, which allocates memory
	from one of the memory pools on the heap stack.  A pool can be
	created by calling pushheap(), and destroyed by calling popheap().
	To free the memory in the pool without destroying it, call
	freeheap(); this is equivalent to { popheap(); pushheap(); }
	Memory allocated in this way does not have to be freed explicitly;
	it will all be freed when the pool is destroyed.  In fact,
	attempting to free this memory may result in a core dump.
	The pair of pointers ncalloc and alloc may point to either
	zalloc & zcalloc or halloc & hcalloc; permalloc() sets them to the
	former, and heapalloc() sets them to the latter. This can be useful.
	For example, the dupstruct() routine duplicates a syntax tree,
	allocating the new memory for the tree using alloc().  If you want
	to duplicate a structure for a one-time use (i.e. to execute the list
	in a for loop), call heapalloc(), then dupstruct().  If you want
	to duplicate a structure in order to preserve it (i.e. a function
	definition), call permalloc(), then dupstruct().

	If we use zsh's own allocator we use a simple trick to avoid that
	the (*real*) heap fills up with empty zsh-heaps: we allocate a
	large block of memory before allocating a heap pool, this memory
	is freed again immediatly after the pool is allocated. If there
	are only small blocks on the free list this guarentees that the
	memory for the pool is at the end of the memory which means that
	we can give it back to the systems when the pool is freed.
*/

#if defined(MEM_DEBUG) && defined(USE_ZSH_MALLOC)

int h_m[1025], h_push, h_pop, h_free;

#endif

#define H_ISIZE  sizeof(long)

#define HEAPSIZE (8192 - H_ISIZE)
#define HEAPFREE (16384 - H_ISIZE)

/* set default allocation to heap stack */

void heapalloc()
{				/**/
    alloc = hcalloc;
    ncalloc = halloc;
    useheap = 1;
}

static vptr(*lastcalloc) DCLPROTO((int));
static vptr(*lastncalloc) DCLPROTO((int));
static int lastuseheap;

/* set default allocation to malloc() */

void permalloc()
{				/**/
    lastcalloc = alloc;
    lastncalloc = ncalloc;
    lastuseheap = useheap;
    alloc = zcalloc;
    ncalloc = zalloc;
    useheap = 0;
}

/* reset previous default allocation */

void lastalloc()
{				/**/
    alloc = lastcalloc;
    ncalloc = lastncalloc;
    useheap = lastuseheap;
}

struct heappos {
    struct heappos *next;
    char *ptr;
    int free;
};

struct heap {
    struct heap *next;
    struct heappos pos;
    char *arena;
};

Heap heaps;

/* create a memory pool */

void pushheap()
{				/**/
    Heap h;
    Heappos hp;

#if defined(MEM_DEBUG) && defined(USE_ZSH_MALLOC)
    h_push++;
#endif

    for (h = heaps; h; h = h->next) {
	hp = (Heappos) zalloc(sizeof(*hp));
	hp->next = h->pos.next;
	h->pos.next = hp;
	hp->free = h->pos.free;
	hp->ptr = h->pos.ptr;
    }
}

/* reset a memory pool */

void freeheap()
{				/**/
    Heap h;

#if defined(MEM_DEBUG) && defined(USE_ZSH_MALLOC)
    h_free++;
#endif
    for (h = heaps; h; h = h->next) {
	if (h->pos.next) {
	    h->pos.free = h->pos.next->free;
	    h->pos.ptr = h->pos.next->ptr;
	} else {
	    h->pos.free += (int)(h->pos.ptr - h->arena);
	    h->pos.ptr = h->arena;
	}
    }
}

/* destroy a memory pool */

void popheap()
{				/**/
    Heap h, hn, hl = NULL;
    Heappos hp, hpn;

#if defined(MEM_DEBUG) && defined(USE_ZSH_MALLOC)
    h_pop++;
#endif

    for (h = heaps; h; h = hn) {
	hn = h->next;
	if ((hp = h->pos.next)) {
	    h->pos.next = hp->next;
	    h->pos.free = hp->free;
	    h->pos.ptr = hp->ptr;
	    zfree(hp, sizeof(struct heappos));

	    hl = h;
	} else {
	    for (hp = h->pos.next; hp; hp = hpn) {
		hpn = hp->next;
		free(hp);
	    }
	    zfree(h->arena, HEAPSIZE);
	    zfree(h, sizeof(struct heap));
	}
    }
    if (hl)
	hl->next = NULL;
    else
	heaps = NULL;
}

/* allocate memory from the current memory pool */

vptr halloc(size)		/**/
int size;
{
    Heap h, hp;
    char *ret;

    zigunsafe();

    size = (size + H_ISIZE - 1) & ~(H_ISIZE - 1);

#if defined(MEM_DEBUG) && defined(USE_ZSH_MALLOC)
    h_m[size < 1024 ? (size / H_ISIZE) : 1024]++;
#endif

    for (h = heaps; h && h->pos.free < size; h = h->next);

    if (h) {
	ret = h->pos.ptr;
	h->pos.ptr += size;
	h->pos.free -= size;
    } else {
	int n = (size > HEAPSIZE) ? size : HEAPSIZE;

#ifdef USE_ZSH_MALLOC
	static int called = 0;
	vptr foo;

	if (called)
	    foo = (vptr) malloc(HEAPFREE);
#endif

	for (hp = NULL, h = heaps; h; hp = h, h = h->next);

	h = (Heap) zalloc(sizeof *h);
	h->arena = (char *)zalloc(n);

#ifdef USE_ZSH_MALLOC
	if (called)
	    zfree(foo, HEAPFREE);
	called = 1;
#endif

	h->next = NULL;

	if (hp)
	    hp->next = h;
	else
	    heaps = h;

	h->pos.next = NULL;
	h->pos.ptr = h->arena + size;

	h->pos.free = n - size;

	ret = h->arena;
    }

    zigsafe();

    return (vptr) ret;
}

/* allocate memory from the current memory pool and clear it */

vptr hcalloc(size)		/**/
int size;
{
    vptr ptr;

    ptr = halloc(size);
    memset(ptr, 0, size);
    return ptr;
}

vptr hrealloc(p, old, new)	/**/
char *p;
int old;
int new;
{
    char *ptr;

    ptr = (char *)halloc(new);
    memcpy(ptr, p, old);
    return (vptr) ptr;
}

/* allocate permanent memory */

vptr zalloc(l)			/**/
int l;
{
    vptr z;

    zigunsafe();

    if (!l)
	l = 1;
    if (!(z = (vptr) malloc(l))) {
	zerr("fatal error: out of memory", NULL, 0);
	exit(1);
    }
    zigsafe();

    return z;
}

vptr zcalloc(size)		/**/
int size;
{
    vptr ptr;

    ptr = zalloc(size);
    memset(ptr, 0, size);
    return ptr;
}

char *dupstring(s)		/**/
const char *s;
{
    char *t;

    if (!s)
	return NULL;
    t = (char *)ncalloc(strlen((char *)s) + 1);
    strcpy(t, s);
    return t;
}

char *ztrdup(s)			/**/
const char *s;
{
    char *t;

    if (!s)
	return NULL;
    t = (char *)zalloc(strlen((char *)s) + 1);
    strcpy(t, s);
    return t;
}

#ifdef USE_ZSH_MALLOC

/*
   Below is a simple segment oriented memory allocator for systems on
   which it is better than the system's one. Memory id given in blocks
   aligned to an integer multiple of sizeof(long) (4 bytes on most machines,
   but 8 bytes on e.g. a dec alpha). Each block is preceded by a header
   which contains the length of the data part (in bytes). In allocated
   blocks only this field of the structure m_hdr is senseful. In free
   blocks the second field (next) is a pointer to the next free segment
   on the free list.

   On top of this simple allocator there is a second allocator for small
   chunks of data. It should be both faster and less space-consuming than
   using the normal segment mechanism for such blocks.
   For the first M_NSMALL-1 possible sizes memory is allocated in arrays
   that can hold M_SNUM blocks. Each array is stored in one segment of the
   main allocator. In these segments the third field of the header structure
   (free) contains a pointer to the first free block in the array. The
   last field (used) gives the number of already used blocks in the array.

   If the macro name MEM_DEBUG is defined, some information about the memory
   usage is stored. This information can than be viewed by calling the
   builtin `mem' (which is only available if MEM_DEBUG is set).

   If MEM_WARNING is defined, error messages are printed in case of errors.

   If SECURE_FREE is defined, free() checks if the given address is really
   one that was returned by malloc(), it ignores it if it wasn't (printing
   an error message if MEM_WARNING is also defined).
*/
#if !defined(__hpux) && !defined(DGUX)
#if defined(_BSD) && !defined(SYSV)

extern int brk DCLPROTO((caddr_t));
extern caddr_t sbrk DCLPROTO((int));

#else

extern int brk DCLPROTO((void *));
extern void *sbrk DCLPROTO((int));

#endif
#endif

#if defined(_BSD) && !defined(HAS_STDLIB)

#define FREE_RET_T int
#define FREE_ARG_T char *
#define FREE_DO_RET
#define MALLOC_RET_T char *
#define MALLOC_ARG_T size_t

#else

#define FREE_RET_T void
#define FREE_ARG_T void *
#define MALLOC_RET_T void *
#define MALLOC_ARG_T size_t

#endif

struct m_shdr {
    struct m_shdr *next;
};

struct m_hdr {
    long len;
    struct m_hdr *next;
    struct m_shdr *free;
    long used;
};

#define M_ALIGN (sizeof(long))

#define M_HSIZE (sizeof(struct m_hdr))
#define M_ISIZE (sizeof(long))
#define M_MIN   (2 * M_ISIZE)

struct m_hdr *m_lfree, *m_free;
long m_pgsz = 0;
char *m_high, *m_low;

#define M_SIDX(S)  ((S) / M_ISIZE)
#define M_SNUM     50
#define M_SLEN(M)  ((M)->len / M_SNUM)
#define M_SBLEN(S) ((S) * M_SNUM + sizeof(struct m_shdr *) +  \
		    sizeof(long) + sizeof(struct m_hdr *))
#define M_BSLEN(S) (((S) - sizeof(struct m_shdr *) -  \
		     sizeof(long) - sizeof(struct m_hdr *)) / M_SNUM)
#define M_NSMALL 8

struct m_hdr *m_small[M_NSMALL];

#ifdef MEM_DEBUG

int m_s = 0, m_b = 0;
int m_m[1025], m_f[1025];

struct m_hdr *m_l;

#endif

MALLOC_RET_T malloc(size)
MALLOC_ARG_T size;
{
    struct m_hdr *m, *mp, *mt;
    long n, s, os;
    struct heap *h, *hp, *hf = NULL, *hfp;

    if (!size)
	return (MALLOC_RET_T) m_high;

    if (!m_pgsz) {
#ifdef __hpux
	extern long sysconf DCLPROTO((int));

	m_pgsz = sysconf(_SC_PAGE_SIZE);
#else
#ifdef SYSV
	extern long sysconf DCLPROTO((int));

	m_pgsz = sysconf(_SC_PAGESIZE);
#else
	extern int getpagesize DCLPROTO((void));

	m_pgsz = getpagesize();
#endif
#endif
	m_free = m_lfree = NULL;
    }
    size = (size + M_ALIGN - 1) & ~(M_ALIGN - 1);

    if ((s = M_SIDX(size)) && s < M_NSMALL) {
	for (mp = NULL, m = m_small[s]; m && !m->free; mp = m, m = m->next);

	if (m) {
	    struct m_shdr *sh = m->free;

	    m->free = sh->next;
	    m->used++;

	    if (m->used == M_SNUM && m->next) {
		for (mt = m; mt->next; mt = mt->next);

		mt->next = m;
		if (mp)
		    mp->next = m->next;
		else
		    m_small[s] = m->next;
		m->next = NULL;
	    }
#ifdef MEM_DEBUG
	    m_m[size / M_ISIZE]++;
#endif

	    return (MALLOC_RET_T) sh;
	}
	os = size;
	size = M_SBLEN(size);
    } else
	s = 0;

    for (mp = NULL, m = m_free; m && m->len < size; mp = m, m = m->next);

 /* If there is an empty zsh heap at a lower address we steel it and take
       the memory from it, putting the rest on the free list. */

    for (hp = NULL, h = heaps; h; hp = h, h = h->next)
	if (h->pos.ptr == h->arena &&
	    (!hf || h < hf) &&
	    (!m || ((char *)m) > ((char *)h)))
	    hf = h, hfp = hp;

    if (hf) {
	struct heappos *hpo, *hpn;

	for (hpo = hf->pos.next; hpo; hpo = hpn) {
	    hpn = hpo->next;
	    zfree(hpo, sizeof(struct heappos));
	}
	if (hfp)
	    hfp->next = hf->next;
	else
	    heaps = hf->next;
	zfree(hf->arena, HEAPSIZE);
	zfree(hf, sizeof(struct heap));

	for (mp = NULL, m = m_free; m && m->len < size; mp = m, m = m->next);
    }
    if (!m) {
	n = (size + M_HSIZE + m_pgsz - 1) & ~(m_pgsz - 1);

	if (((char *)(m = (struct m_hdr *)sbrk(n))) == ((char *)-1)) {
#ifdef MEM_WARNING
	    zerr("allocation error at sbrk: %e", NULL, 0);
#endif
	    return NULL;
	}
	if (!m_low)
	    m_low = (char *)m;

#ifdef MEM_DEBUG
	m_s += n;

	if (!m_l)
	    m_l = m;
#endif

	m_high = ((char *)m) + n;

	m->len = n - M_ISIZE;
	m->next = NULL;

	if (mp = m_lfree)
	    m_lfree->next = m;
	m_lfree = m;
    }
    if ((n = m->len - size) > M_MIN) {
	struct m_hdr *mtt = (struct m_hdr *)(((char *)m) + M_ISIZE + size);

	mtt->len = n - M_ISIZE;
	mtt->next = m->next;

	m->len = size;

	if (m_lfree == m)
	    m_lfree = mtt;

	if (mp)
	    mp->next = mtt;
	else
	    m_free = mtt;
    } else if (mp) {
	if (m == m_lfree)
	    m_lfree = mp;
	mp->next = m->next;
    } else {
	m_free = m->next;
	if (m == m_lfree)
	    m_lfree = m_free;
    }

    if (s) {
	struct m_shdr *sh, *shn;

	m->free = sh = (struct m_shdr *)(((char *)m) +
					 sizeof(struct m_hdr) + os);

	m->used = 1;

	for (n = M_SNUM - 2; n--; sh = shn)
	    shn = sh->next = sh + s;
	sh->next = NULL;

	m->next = m_small[s];
	m_small[s] = m;

#ifdef MEM_DEBUG
	m_m[os / M_ISIZE]++;
#endif

	return (MALLOC_RET_T) (((char *)m) + sizeof(struct m_hdr));
    }
#ifdef MEM_DEBUG
    m_m[m->len < (1024 * M_ISIZE) ? (m->len / M_ISIZE) : 1024]++;
#endif

    return (MALLOC_RET_T) & m->next;
}

void zfree(p, sz)		/**/
vptr p;
int sz;
{
    struct m_hdr *m = (struct m_hdr *)(((char *)p) - M_ISIZE), *mp, *mt;
    int i;

#ifdef SECURE_FREE
    sz = 0;
#else
    sz = (sz + M_ALIGN - 1) & ~(M_ALIGN - 1);
#endif

    if (!p)
	return;

    if (((char *)p) < m_low || ((char *)p) > m_high ||
	((long)p) & (M_ALIGN - 1)) {

#ifdef MEM_WARNING
	zerr("attempt to free storage at invalid address", NULL, 0);
#endif

	return;
    }
  fr_rec:

    if ((i = sz / M_ISIZE) < M_NSMALL || !sz)
	for (; i < M_NSMALL; i++) {
	    for (mp = NULL, mt = m_small[i];
		 mt && (((char *)mt) > ((char *)p) ||
			(((char *)mt) + mt->len) < ((char *)p));
		 mp = mt, mt = mt->next);

	    if (mt) {
		struct m_shdr *sh = (struct m_shdr *)p, *sh2;

#ifdef SECURE_FREE
		if ((((char *)p) - (((char *)mt) + sizeof(struct m_hdr))) %
		    M_BSLEN(mt->len)) {

#ifdef MEM_WARNING
		    zerr("attempt to free storage at invalid address", NULL, 0);
#endif
		    return;
		}
		for (sh2 = mt->free; sh2; sh2 = sh2->next)
		    if (((char *)p) == ((char *)sh2)) {

#ifdef MEM_WARNING
			zerr("attempt to free already free storage", NULL, 0);
#endif
			return;
		    }
#endif

		sh->next = mt->free;
		mt->free = sh;

#ifdef MEM_DEBUG
		m_f[M_BSLEN(mt->len) / M_ISIZE]++;
#endif

		if (--mt->used) {
		    if (mp) {
			mp->next = mt->next;
			mt->next = m_small[i];
			m_small[i] = mt;
		    }
		    return;
		}
		if (mp)
		    mp->next = mt->next;
		else
		    m_small[i] = mt->next;

		m = mt;
		p = (vptr) & m->next;

		break;
	    } else if (sz) {
		sz = 0;
		goto fr_rec;
	    }
	}
#ifdef MEM_DEBUG
    if (!mt)
	m_f[m->len < (1024 * M_ISIZE) ? (m->len / M_ISIZE) : 1024]++;
#endif

#ifdef SECURE_FREE
    for (mt = (struct m_hdr *)m_low;
	 ((char *)mt) < m_high;
	 mt = (struct m_hdr *)(((char *)mt) + M_ISIZE + mt->len))
	if (((char *)p) == ((char *)&mt->next))
	    break;

    if (((char *)mt) >= m_high) {

#ifdef MEM_WARNING
	zerr("attempt to free storage at invalid address", NULL, 0);
#endif
	return;
    }
#endif

    for (mp = NULL, mt = m_free; mt && mt < m; mp = mt, mt = mt->next);

    if (m == mt) {

#ifdef MEM_WARNING
	zerr("attempt to free already free storage", NULL, 0);
#endif
	return;
    }
    if (mt && ((char *)mt) == (((char *)m) + M_ISIZE + m->len)) {
	m->len += mt->len + M_ISIZE;
	m->next = mt->next;

	if (mt == m_lfree)
	    m_lfree = m;
    } else
	m->next = mt;

    if (mp && ((char *)m) == (((char *)mp) + M_ISIZE + mp->len)) {
	mp->len += m->len + M_ISIZE;
	mp->next = m->next;

	if (m == m_lfree)
	    m_lfree = mp;
    } else if (mp)
	mp->next = m;
    else
	m_free = m;

    if ((((char *)m_lfree) + M_ISIZE + m_lfree->len) == m_high &&
	m_lfree->len >= m_pgsz + M_MIN) {
	long n = (m_lfree->len - M_MIN) & ~(m_pgsz - 1);

	m_lfree->len -= n;
	if (brk(m_high -= n) == -1) {
#ifdef MEM_WARNING
	    zerr("allocation error at brk: %e", NULL, 0);
#endif
	}
#ifdef MEM_DEBUG
	m_b += n;
#endif
    }
}

FREE_RET_T free(p)
FREE_ARG_T p;
{
    zfree(p, 0);

#ifdef FREE_DO_RET
    return 0;
#endif
}

void zsfree(p)			/**/
char *p;
{
    if (p)
	zfree(p, strlen(p) + 1);
}

MALLOC_RET_T realloc(p, size)
MALLOC_RET_T p;
MALLOC_ARG_T size;
{
    struct m_hdr *m = (struct m_hdr *)(((char *)p) - M_ISIZE), *mp, *mt;
    char *r;
    int i, l = 0;

    if (!p && size)
	return (MALLOC_RET_T) malloc(size);
    if (!p || !size)
	return (MALLOC_RET_T) p;

    for (i = 0; i < M_NSMALL; i++) {
	for (mp = NULL, mt = m_small[i];
	     mt && (((char *)mt) > ((char *)p) ||
		    (((char *)mt) + mt->len) < ((char *)p));
	     mp = mt, mt = mt->next);

	if (mt) {
	    l = M_BSLEN(mt->len);
	    break;
	}
    }
    if (!l)
	l = m->len;

    r = malloc(size);
    memcpy(r, (char *)p, (size > l) ? l : size);
    free(p);

    return (MALLOC_RET_T) r;
}

MALLOC_RET_T calloc(n, size)
MALLOC_ARG_T n;
MALLOC_ARG_T size;
{
    long l;
    char *r;

    if (!(l = n * size))
	return (MALLOC_RET_T) m_high;

    r = malloc(l);

    memset(r, 0, l);

    return (MALLOC_RET_T) r;
}

FREE_RET_T cfree(p)
FREE_ARG_T p;
{
    free(p);

#ifdef FREE_DO_RET
    return 0;
#endif
}

#ifdef MEM_DEBUG

int bin_mem(name, argv, ops, func)	/**/
char *name;
char **argv;
char *ops;
int func;
{
    int i, ii, fi, ui, j;
    struct m_hdr *m, *mf, *ms;
    char *b, *c, buf[40];
    long u = 0, f = 0;
    Lknode nd;

    if (ops['v']) {
	printf("The lower and the upper addresses of the heap. Diff gives\n");
	printf("the difference between them, i.e. the size of the heap.\n\n");
    }
    printf("low mem %ld\t high mem %ld\t diff %ld\n",
	   (long)m_l, (long)m_high, (long)(m_high - ((char *)m_l)));

    if (ops['v']) {
	printf("\nThe number of bytes that were allocated using sbrk() and\n");
	printf("the number of bytes that were given back to the system\n");
	printf("via brk().\n");
    }
    printf("\nsbrk %d\tbrk %d\n", m_s, m_b);

    if (ops['v']) {
	printf("\nInformation about the sizes that were allocated or freed.\n");
	printf("For each size that were used the number of mallocs and\n");
	printf("frees is shown. Diff gives the difference between these\n");
	printf("values, i.e. the number of blocks of that size that is\n");
	printf("currently allocated. Total is the product of size and diff,\n");
	printf("i.e. the number of bytes that are allocated for blocks of\n");
	printf("this size.\n");
    }
    printf("\nsize\tmalloc\tfree\tdiff\ttotal\n");
    for (i = 0; i < 1024; i++)
	if (m_m[i] || m_f[i])
	    printf("%d\t%d\t%d\t%d\t%d\n", i * M_ISIZE, m_m[i], m_f[i],
		   m_m[i] - m_f[i], i * M_ISIZE * (m_m[i] - m_f[i]));

    if (m_m[i] || m_f[i])
	printf("big\t%d\t%d\t%d\n", m_m[i], m_f[i], m_m[i] - m_f[i]);

    if (ops['v']) {
	printf("\nThe list of memory blocks. For each block the following\n");
	printf("information is shown:\n\n");
	printf("num\tthe number of this block\n");
	printf("tnum\tlike num but counted separatedly for used and free\n");
	printf("\tblocks\n");
	printf("addr\tthe address of this block\n");
	printf("len\tthe length of the block\n");
	printf("state\tthe state of this block, this can be:\n");
	printf("\t  used\tthis block is used for one big block\n");
	printf("\t  free\tthis block is free\n");
	printf("\t  small\tthis block is used for an array of small blocks\n");
	printf("cum\tthe accumulated sizes of the blocks, counted\n");
	printf("\tseparatedly for used and free blocks\n");
	printf("\nFor blocks holding small blocks the number of free\n");
	printf("blocks, the number of used blocks and the size of the\n");
	printf("blocks is shown. For otherwise used blocks the first few\n");
	printf("bytes are shown as an ASCII dump.\n");
    }
    printf("\nblock list:\nnum\ttnum\taddr\tlen\tstate\tcum\n");
    for (m = m_l, mf = m_free, ii = fi = ui = 1; ((char *)m) < m_high;
	 m = (struct m_hdr *)(((char *)m) + M_ISIZE + m->len), ii++) {
	for (j = 0, ms = NULL; j < M_NSMALL && !ms; j++)
	    for (ms = m_small[j]; ms; ms = ms->next)
		if (ms == m)
		    break;

	if (m == mf)
	    buf[0] = '\0';
	else if (m == ms)
	    sprintf(buf, "%d %d %d", M_SNUM - ms->used, ms->used,
		    (m->len - sizeof(struct m_hdr)) / M_SNUM + 1);

	else {
	    for (i = 0, b = buf, c = (char *)&m->next; i < 20 && i < m->len;
		 i++, c++)
		*b++ = (*c >= ' ' && *c < 127) ? *c : '.';
	    *b = '\0';
	}

	printf("%d\t%d\t%ld\t%ld\t%s\t%ld\t%s\n", ii,
	       (m == mf) ? fi++ : ui++,
	       (long)m, m->len,
	       (m == mf) ? "free" : ((m == ms) ? "small" : "used"),
	       (m == mf) ? (f += m->len) : (u += m->len),
	       buf);

	if (m == mf)
	    mf = mf->next;
    }

    if (ops['v']) {
	printf("\nHere is some information about the small blocks used.\n");
	printf("For each size the arrays with the number of free and the\n");
	printf("number of used blocks are shown.\n");
    }
    printf("\nsmall blocks:\nsize\tblocks (free/used)\n");

    for (i = 0; i < M_NSMALL; i++)
	if (m_small[i]) {
	    printf("%d\t", i * M_ISIZE);

	    for (ii = 0, m = m_small[i]; m; m = m->next) {
		printf("(%d/%d) ", M_SNUM - m->used, m->used);
		if (!((++ii) & 7))
		    printf("\n\t");
	    }
	    putchar('\n');
	}
    if (ops['v']) {
	printf("\n\nBelow is some information about the allocation\n");
	printf("behaviour of the zsh heaps. First the number of times\n");
	printf("pushheap(), popheap(), and freeheap() were called.\n");
    }
    printf("\nzsh heaps:\n\n");

    printf("push %d\tpop %d\tfree %d\n\n", h_push, h_pop, h_free);

    if (ops['v']) {
	printf("\nThe next list shows for several sizes the number of times\n");
	printf("memory of this size were taken from heaps.\n\n");
    }
    printf("size\tmalloc\ttotal\n");
    for (i = 0; i < 1024; i++)
	if (h_m[i])
	    printf("%d\t%d\t%d\n", i * H_ISIZE, h_m[i], i * H_ISIZE * h_m[i]);
    if (h_m[1024])
	printf("big\t%d\n", h_m[1024]);

    return 0;
}

#endif

#else				/* not USE_ZSH_MALLOC */

void zfree(p, sz)		/**/
vptr p;
int sz;
{
    if (p)
	free(p);
}

void zsfree(p)			/**/
char *p;
{
    if (p)
	free(p);
}

#endif
