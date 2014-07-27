///////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

#define halt(s) \
	do { \
		fprintf(stderr, "%s: " s "\n", __func__); \
		exit(1); \
	} while (0)

#define haltf(s, ...) \
	do { \
		fprintf(stderr, "%s: " s "\n", __func__, __VA_ARGS__); \
		exit(1); \
	} while (0)

///////////////////////////////////////////////////////////////////////////////

// This assumes a 64-bit architecture, with memory allocated aligned to an
// 8-byte boundary, leaving the lowest 3 bits clear to use as a tag.

#define EMPTY_TAG  0

#define NUM_TAG    1
#define ADDR_TAG   2
#define BRANCH_RET_ADDR_TAG 3
#define FUNC_RET_ADDR_TAG 4
#define PAIR_TAG   5
#define CPAIR_TAG  6
#define FRAME_TAG  7

#define TAG_MASK   7

intptr_t tag(void *x)
{
	return (intptr_t)x & TAG_MASK;
}

void *ptr(void *x)
{
	return (void *)((intptr_t)x & ~TAG_MASK);
}

void *tag_ptr(intptr_t t, void *x)
{
	return (void *)((intptr_t)ptr(x) | t);
}

void *lshift_num(int32_t n)
{
	return (void *)((intptr_t)n << 32);
}

int32_t rshift_num(void *x)
{
	return (int32_t)((intptr_t)ptr(x) >> 32);
}

///////////////////////////////////////////////////////////////////////////////

#define MIN_STACK_SIZE 1024
#define MAX_STACK_SIZE (10 * 1024 * 1024)

typedef struct stack {
	int32_t size;
	void **top;
	void *data[];
} STACK;

STACK *alloc_stack()
{
	STACK *s;

	if (!(s = malloc(sizeof(STACK) + MIN_STACK_SIZE * sizeof(void *)))) {
		halt("Out of memory");
	}
	s->size = MIN_STACK_SIZE;
	s->top = s->data;
	return s;
}

void push(void *x, STACK **sp)
{
	STACK *s = *sp;

	if (tag(x) == EMPTY_TAG) {
		halt("Empty tag");
	}
	if (s->top == s->data + s->size) {
		if ((s->size *= 2) > MAX_STACK_SIZE) {
			halt("Stack overflow");
		}
		if (!(*sp = realloc(s, sizeof(STACK) + s->size * sizeof(void *)))) {
			halt("Out of memory");
		}
		s = *sp;
		s->top = s->data + s->size / 2;
	}
	*(s->top++) = x;
}

void *pop(STACK *s)
{
	if (s->top == s->data) {
		halt("Stack underflow");
	}
	return *(--s->top);
}

///////////////////////////////////////////////////////////////////////////////

typedef struct frame {
	struct frame *parent;
	int32_t size;
	void *data[];
} FRAME;

FRAME *alloc_frame(FRAME *p, int32_t n)
{
	FRAME *f;

	if (n < 0) {
		halt("Bad size");
	}
	if (!(f = malloc(sizeof(FRAME) + n * sizeof(void *)))) {
		halt("Out of memory");
	}
	f->parent = p;
	f->size = n;
	return f;
}

FRAME *parent(FRAME *f)
{
	return f->parent;
}

void store(int32_t i, void *x, FRAME *f)
{
	if (i < 0) {
		halt("Frame underflow");
	}
	if (tag(x) == EMPTY_TAG) {
		halt("Empty tag");
	}
	if (f->size < 0) {
		halt("Filled frame");
	}
	if (i >= f->size) {
		halt("Frame overflow");
	}
	f->data[i] = x;
}

void *load(int32_t i, FRAME *f)
{
	if (i < 0) {
		halt("Frame underflow");
	}
	if (f->size > 0) {
		halt("Not filled frame");
	}
	if (i >= f->size * -1) {
		halt("Frame overflow");
	}
	return f->data[i];
}

void tag_filled(FRAME *f)
{
	if (f->size < 0) {
		halt("Filled frame");
	}
	f->size *= -1;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct pair {
	void *fst, *snd;
} PAIR;

PAIR *alloc_pair(void *x, void *y)
{
	PAIR *p;

	if (tag(x) == EMPTY_TAG) {
		halt("Empty first tag");
	}
	if (tag(y) == EMPTY_TAG) {
		halt("Empty second tag");
	}
	if (!(p = malloc(sizeof(PAIR)))) {
		halt("Out of memory");
	}
	p->fst = x;
	p->snd = y;
	return p;
}

void *fst(PAIR *p)
{
	return p->fst;
}

void *snd(PAIR *p)
{
	return p->snd;
}

///////////////////////////////////////////////////////////////////////////////

void *tag_num(int32_t n)
{
	return tag_ptr(NUM_TAG, lshift_num(n));
}

int32_t num(void *x)
{
	if (tag(x) != NUM_TAG) {
		halt("Type mismatch");
	}
	return rshift_num(x);
}

void *tag_addr(int32_t n)
{
	return tag_ptr(ADDR_TAG, lshift_num(n));
}

int32_t addr(void *x)
{
	if (tag(x) != ADDR_TAG) {
		halt("Type mismatch");
	}
	return rshift_num(x);
}

void *tag_branch_ret_addr(int32_t n)
{
	return tag_ptr(BRANCH_RET_ADDR_TAG, lshift_num(n));
}

int32_t branch_ret_addr(void *x)
{
	if (tag(x) != BRANCH_RET_ADDR_TAG) {
		halt("Type mismatch");
	}
	return rshift_num(x);
}

void *tag_func_ret_addr(int32_t n)
{
	return tag_ptr(FUNC_RET_ADDR_TAG, lshift_num(n));
}

int32_t func_ret_addr(void *x)
{
	if (tag(x) != FUNC_RET_ADDR_TAG) {
		halt("Type mismatch");
	}
	return rshift_num(x);
}

void *tag_pair(PAIR *p)
{
	return tag_ptr(PAIR_TAG, p);
}

PAIR *pair(void *x)
{
	if (tag(x) != PAIR_TAG) {
		halt("Type mismatch");
	}
	return ptr(x);
}

void *tag_cpair(PAIR *p)
{
	return tag_ptr(CPAIR_TAG, p);
}

PAIR *cpair(void *x)
{
	if (tag(x) != CPAIR_TAG) {
		halt("Type mismatch");
	}
	return ptr(x);
}

void *tag_frame(FRAME *f)
{
	return tag_ptr(FRAME_TAG, f);
}

FRAME *frame(void *x)
{
	if (tag(x) != FRAME_TAG) {
		halt("Type mismatch");
	}
	return ptr(x);
}

///////////////////////////////////////////////////////////////////////////////

#define STOPPED_ADDR (-1)

typedef struct state {
	int32_t addr;
	STACK *data;
	STACK *ctrl;
	FRAME *env;
} STATE;

STATE *alloc_state()
{
	STATE *s;

	if (!(s = malloc(sizeof(STATE)))) {
		halt("Out of memory");
	}
	s->addr = 0;
	s->data = alloc_stack();
	s->ctrl = alloc_stack();
	s->env = NULL; // TODO
	return s;
}

// LDC - load constant
void do_ldc(int32_t n, STATE *s)
{
	push(tag_num(n), &s->data);
	s->addr++;
}

// LD - load from environment
void do_ld(int32_t fi, int32_t ei, STATE *s)
{
	int32_t i = 0;
	FRAME *f;

	if (fi < 0) {
		halt("Chain underflow");
	}
	if (ei < 0) {
		halt("Frame underflow");
	}
	f = s->env;
	for (i = 0; i < fi; i++) {
		if (!f) {
			halt("Chain overflow");
		}
		f = parent(f);
	}
	push(load(ei, f), &s->data);
	s->addr++;
}

// ADD - integer addition
void do_add(STATE *s)
{
	int32_t n, m;

	m = num(pop(s->data));
	n = num(pop(s->data));
	push(tag_num(n + m), &s->data);
	s->addr++;
}

// SUB - integer subtraction
void do_sub(STATE *s)
{
	int32_t n, m;

	m = num(pop(s->data));
	n = num(pop(s->data));
	push(tag_num(n - m), &s->data);
	s->addr++;
}

// MUL - integer multiplication
void do_mul(STATE *s)
{
	int32_t n, m;

	m = num(pop(s->data));
	n = num(pop(s->data));
	push(tag_num(n * m), &s->data);
	s->addr++;
}

// DIV - integer division
void do_div(STATE *s)
{
	int32_t n, m;

	m = num(pop(s->data));
	n = num(pop(s->data));
	push(tag_num(n / m), &s->data);
	s->addr++;
}

// CEQ - compare equal
void do_ceq(STATE *s)
{
	int32_t n, m;

	m = num(pop(s->data));
	n = num(pop(s->data));
	push(tag_num(n == m ? 1 : 0), &s->data);
	s->addr++;
}

// CGT - compare greater than
void do_cgt(STATE *s)
{
	int32_t n, m;

	m = num(pop(s->data));
	n = num(pop(s->data));
	push(tag_num(n > m ? 1 : 0), &s->data);
	s->addr++;
}

// CGTE - compare greater than or equal
void do_cgte(STATE *s)
{
	int32_t n, m;

	m = num(pop(s->data));
	n = num(pop(s->data));
	push(tag_num(n >= m ? 1 : 0), &s->data);
	s->addr++;
}

// ATOM - test if value is an integer
void do_atom(STATE *s)
{
	intptr_t t;

	t = tag(pop(s->data));
	push(tag_num(t == NUM_TAG ? 1 : 0), &s->data);
	s->addr++;
}

// CONS - allocate a CONS cell
void do_cons(STATE *s)
{
	void *x, *y;
	PAIR *p;

	y = pop(s->data);
	x = pop(s->data);
	p = alloc_pair(x, y);
	push(tag_pair(p), &s->data);
	s->addr++;
}

// CAR - extract first element from CONS cell
void do_car(STATE *s)
{
	PAIR *p;

	p = pair(pop(s->data));
	push(fst(p), &s->data);
	s->addr++;
}

// CDR - extract second element from CONS cell
void do_cdr(STATE *s)
{
	PAIR *p;

	p = pair(pop(s->data));
	push(snd(p), &s->data);
	s->addr++;
}

// SEL - conditional branch
void do_sel(int32_t taddr, int32_t faddr, STATE *s)
{
	int32_t n;

	if (taddr < 0) {
		halt("Bad true address");
	}
	if (faddr < 0) {
		halt("Bad false address");
	}
	n = num(pop(s->data));
	push(tag_branch_ret_addr(s->addr + 1), &s->ctrl);
	s->addr = n ? taddr : faddr;
}

// JOIN - return from branch
void do_join(STATE *s)
{
	s->addr = branch_ret_addr(pop(s->ctrl));
}

// LDF - load function
void do_ldf(int32_t faddr, STATE *s)
{
	PAIR *c;

	if (faddr < 0) {
		halt("Bad function address");
	}
	c = alloc_pair(tag_addr(faddr), tag_frame(s->env));
	push(tag_cpair(c), &s->data);
	s->addr++;
}

// AP - call function
void do_ap(int32_t n, STATE *s)
{
	PAIR *c;
	FRAME *cenv, *fenv;
	int32_t faddr, i;

	c = cpair(pop(s->data));
	faddr = addr(fst(c));
	cenv = frame(snd(c));
	fenv = alloc_frame(cenv, n);
	for (i = n - 1; i >= 0; i--) {
		store(i, pop(s->data), fenv);
	}
	tag_filled(fenv);
	push(tag_frame(s->env), &s->ctrl);
	push(tag_func_ret_addr(s->addr + 1), &s->ctrl);
	s->env = fenv;
	s->addr = faddr;
}

// RTN - return from function call
// TODO: Free frame
void do_rtn(STATE *s)
{
	int32_t fraddr;
	FRAME *env;

	fraddr = func_ret_addr(pop(s->ctrl));
	env = frame(pop(s->ctrl));
	s->env = env;
	s->addr = fraddr;
}

// DUM - create an empty environment frame
void do_dum(int32_t n, STATE *s)
{
	s->env = alloc_frame(s->env, n);
	s->addr++;
}

// RAP - recursive environment call function
void do_rap(int32_t n, STATE *s)
{
	PAIR *c;
	FRAME *cenv;
	int32_t faddr, i;

	c = cpair(pop(s->data));
	faddr = addr(fst(c));
	cenv = frame(snd(c));
	if (cenv != s->env) {
		halt("Frame mismatch");
	}
	for (i = n - 1; i >= 0; i--) {
		store(i, pop(s->data), s->env);
	}
	tag_filled(s->env);
	push(tag_frame(parent(s->env)), &s->ctrl);
	push(tag_func_ret_addr(s->addr + 1), &s->ctrl);
	s->addr = faddr;
}

// STOP - terminate co-processor execution
void do_stop(STATE *s)
{
	s->addr = STOPPED_ADDR;
}

// TSEL - tail-call conditional branch
void do_tsel(int32_t taddr, int32_t faddr, STATE *s)
{
	int32_t n;

	if (taddr < 0) {
		halt("Bad true address");
	}
	if (faddr < 0) {
		halt("Bad false address");
	}
	n = num(pop(s->data));
	s->addr = n ? taddr : faddr;
}

// TAP - tail-call function
// TODO: Overwrite frame
void do_tap(int32_t n, STATE *s)
{
	PAIR *c;
	FRAME *cenv, *fenv;
	int32_t faddr, i;

	c = cpair(pop(s->data));
	faddr = addr(fst(c));
	cenv = frame(snd(c));
	fenv = alloc_frame(cenv, n);
	for (i = n - 1; i >= 0; i--) {
		store(i, pop(s->data), fenv);
	}
	tag_filled(fenv);
	s->env = fenv;
	s->addr = faddr;
}

// TRAP - recursive environment tail-call functions
void do_trap(int32_t n, STATE *s)
{
	PAIR *c;
	FRAME *cenv;
	int32_t faddr, i;

	c = cpair(pop(s->data));
	faddr = addr(fst(c));
	cenv = frame(snd(c));
	if (cenv != s->env) {
		halt("Frame mismatch");
	}
	for (i = n - 1; i >= 0; i--) {
		store(i, pop(s->data), s->env);
	}
	tag_filled(s->env);
	s->addr = faddr;
}

// ST - store to environment
// TODO: Modifies filled frames!

// DBUG - printf debugging
// TODO

// BRK - breakpoint debugging
// TODO

///////////////////////////////////////////////////////////////////////////////

#define COMMENT_CHAR     ';'
#define NEGATE_SIGN_CHAR '-'

int peekc(FILE *in)
{
	int c;

	c = getc(in);
	ungetc(c, in);
	return c;
}

void skip_spaces(FILE *in)
{
	int c;

	while ((c = getc(in)) != EOF) {
		if (isspace(c)) {
			continue;
		}
		if (c == COMMENT_CHAR) {
			while ((c = getc(in)) != EOF && c != '\n') {
				;
			}
			continue;
		}
		ungetc(c, in);
		break;
	}
}

void *read_lit(FILE *in, int32_t sign)
{
	int c;
	int32_t n = 0;

	while (isdigit((c = getc(in)))) {
		n = (n * 10) + digittoint(c);
	}
	n *= sign;
	ungetc(c, in);
	return tag_num(n);
}

void *read(FILE *in)
{
	int c;
	int32_t sign = 1;

	skip_spaces(in);
	if ((c = getc(in)) == EOF) {
		printf("\n");
		exit(0);
	}
	if (isdigit(c) || (c == NEGATE_SIGN_CHAR && isdigit(peekc(in)))) {
		if (c == NEGATE_SIGN_CHAR) {
			sign = -1;
		} else {
			ungetc(c, in);
		}
		return read_lit(in, sign);
	}
	haltf("Bad '%c'", c);
}

///////////////////////////////////////////////////////////////////////////////

void write(void *x, FILE *out)
{
	switch (tag(x)) {
	case NUM_TAG:
		fprintf(out, "%d", num(x));
		break;
	case ADDR_TAG:
		fprintf(out, "@%d", addr(x));
		break;
	case BRANCH_RET_ADDR_TAG:
		fprintf(out, "br@%d", branch_ret_addr(x));
		break;
	case FUNC_RET_ADDR_TAG:
		fprintf(out, "fr@%d", func_ret_addr(x));
		break;
	case PAIR_TAG:
		// TODO
		break;
	case CPAIR_TAG:
		// TODO
		break;
	case FRAME_TAG:
		// TODO
		break;
	default:
		halt("Bad tag");
	}
}

///////////////////////////////////////////////////////////////////////////////

void *eval(void *x, STATE *s)
{
	return x;
}

///////////////////////////////////////////////////////////////////////////////

int main()
{
	STATE *s;

	s = alloc_state();
	printf("Hello, world!\n");
	while (1) {
		printf("> ");
		write(eval(read(stdin), s), stdout);
		printf("\n");
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
