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
//
// 32-bit numbers:
// llllllll llllllll llllllll llllllll 00000000 00000000 00000000 00000tt0
//
// 64-bit pointers:
// pppppppp pppppppp pppppppp pppppppp pppppppp pppppppp pppppppp ppppptt1

#define TAG_MASK 0x07

intptr_t get_tag(void *x)
{
	return (intptr_t)x & TAG_MASK;
}

void *clear_tag(void *x)
{
	return (void *)((intptr_t)x & ~TAG_MASK);
}

void *set_tag(intptr_t t, void *x)
{
	return (void *)((intptr_t)clear_tag(x) | t);
}

void *set_num_tag(intptr_t t, int32_t n)
{
	return set_tag(t, (void *)((intptr_t)n << 32));
}

int32_t clear_num_tag(void *x)
{
	return (int32_t)((intptr_t)clear_tag(x) >> 32);
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

void push_obj(void *x, STACK **sp)
{
	STACK *s = *sp;

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

void *pop_obj(STACK *s)
{
	if (s->top == s->data) {
		halt("Stack underflow");
	}
	return *(--s->top);
}

///////////////////////////////////////////////////////////////////////////////

#define EMPTY_FRAME     0x00
#define NOT_EMPTY_FRAME 0x01

typedef struct frame {
	struct frame *parent;
	int32_t size;
	void *data[];
} FRAME;

FRAME *alloc_frame(FRAME *p, int32_t s)
{
	FRAME *f;

	if (get_tag(p) != EMPTY_FRAME) {
		haltf("Invalid parent %p", p);
	}
	if (s < 0) {
		haltf("Invalid size %d", s);
	}
	if (!(f = malloc(sizeof(FRAME) + s * sizeof(void *)))) {
		halt("Out of memory");
	}
	f->parent = p;
	f->size = s;
	return f;
}

void set_not_empty(FRAME *f)
{
	f->parent = set_tag(NOT_EMPTY_FRAME, f->parent);
}

int is_empty(FRAME *f)
{
	return get_tag(f->parent) == EMPTY_FRAME;
}

FRAME *get_parent(FRAME *f)
{
	return clear_tag(f->parent);
}

void set_elem(int32_t i, void *x, FRAME *f)
{
	if (i < 0) {
		haltf("Invalid index %d", i);
	}
	if (!is_empty(f)) {
		halt("Not empty frame");
	}
	if (i >= f->size) {
		haltf("Expected %d elements, not %d", i + 1, f->size);
	}
	f->data[i] = x;
}

void *get_elem(int32_t i, FRAME *f)
{
	if (i < 0) {
		haltf("Invalid index %d", i);
	}
	if (is_empty(f)) {
		halt("Empty frame");
	}
	if (i >= f->size) {
		haltf("Expected %d elements, not %d", i + 1, f->size);
	}
	return f->data[i];
}

///////////////////////////////////////////////////////////////////////////////

typedef struct pair {
	void *fst, *snd;
} PAIR;

PAIR *alloc_pair(void *a, void *b)
{
	PAIR *p;

	if (!(p = malloc(sizeof(PAIR)))) {
		halt("Out of memory");
	}
	p->fst = a;
	p->snd = b;
	return p;
}

void *get_fst(PAIR *p)
{
	return p->fst;
}

void *get_snd(PAIR *p)
{
	return p->snd;
}

///////////////////////////////////////////////////////////////////////////////

#define INT_OBJ  0x00
#define PAIR_OBJ 0x01
#define ADDR_OBJ 0x02

void *make_int(int32_t n)
{
	return set_num_tag(INT_OBJ, n);
}

int32_t get_int(void *x)
{
	if (get_tag(x) != INT_OBJ) {
		halt("Expected integer");
	}
	return clear_num_tag(x);
}

void *make_pair(PAIR *p)
{
	return set_tag(PAIR_OBJ, p);
}

PAIR *get_pair(void *x)
{
	if (get_tag(x) != PAIR_OBJ) {
		halt("Expected pair");
	}
	return clear_tag(x);
}

void *make_addr(int32_t a)
{
	return set_num_tag(ADDR_OBJ, a);
}

int32_t get_addr(void *x)
{
	if (get_tag(x) != ADDR_OBJ) {
		halt("Expected address");
	}
	return clear_num_tag(x);
}

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

void *read_int(FILE *in, int32_t sign)
{
	int c;
	int32_t n = 0;

	while (isdigit((c = getc(in)))) {
		n = (n * 10) + digittoint(c);
	}
	n *= sign;
	ungetc(c, in);
	return make_int(n);
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
		return read_int(in, sign);
	}
	haltf("Unexpected '%c'", c);
}

///////////////////////////////////////////////////////////////////////////////

void write(void *x, FILE *out)
{
	switch (get_tag(x)) {
	case INT_OBJ:
		fprintf(out, "%d", get_int(x));
		break;
	case PAIR_OBJ:
		break;
	case ADDR_OBJ:
		fprintf(out, "%d", get_addr(x));
		break;
	default:
		haltf("Unexpected tag %lx", get_tag(x));
	}
}

///////////////////////////////////////////////////////////////////////////////

typedef struct state {
	int32_t ctr;
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
	s->ctr = 0;
	s->data = alloc_stack();
	s->ctrl = alloc_stack();
	s->env = NULL; // TODO
	return s;
}

// LDC - load constant
void do_ldc(int32_t n, STATE *s)
{
	push_obj(make_int(n), &s->data);
	s->ctr++;
}

// LD - load from environment
void do_ld(int32_t fi, int32_t ei, STATE *s)
{
	int32_t fc = 0;
	FRAME *f;

	if (fi < 0) {
		haltf("Invalid frame index %d", fi);
	}
	if (ei < 0) {
		haltf("Invalid element index %d", ei);
	}
	f = s->env;
	for (fc = 0; fc < fi; fc++) {
		if (!f) {
			haltf("Expected %d frames, not %d", fi, fc);
		}
		f = f->parent;
		fc++;
	}
	push_obj(get_elem(ei, f), &s->data);
	s->ctr++;
}

// ADD - integer addition
void do_add(STATE *s)
{
	int32_t n, m;

	m = get_int(pop_obj(s->data));
	n = get_int(pop_obj(s->data));
	push_obj(make_int(n + m), &s->data);
	s->ctr++;
}

// SUB - integer subtraction
void do_sub(STATE *s)
{
	int32_t n, m;

	m = get_int(pop_obj(s->data));
	n = get_int(pop_obj(s->data));
	push_obj(make_int(n - m), &s->data);
	s->ctr++;
}

// MUL - integer multiplication
void do_mul(STATE *s)
{
	int32_t n, m;

	m = get_int(pop_obj(s->data));
	n = get_int(pop_obj(s->data));
	push_obj(make_int(n * m), &s->data);
	s->ctr++;
}

// DIV - integer division
void do_div(STATE *s)
{
	int32_t n, m;

	m = get_int(pop_obj(s->data));
	n = get_int(pop_obj(s->data));
	push_obj(make_int(n / m), &s->data);
	s->ctr++;
}

// CEQ - compare equal
void do_ceq(STATE *s)
{
	int32_t n, m;

	m = get_int(pop_obj(s->data));
	n = get_int(pop_obj(s->data));
	push_obj(make_int(n == m ? 1 : 0), &s->data);
	s->ctr++;
}

// CGT - compare greater than
void do_cgt(STATE *s)
{
	int32_t n, m;

	m = get_int(pop_obj(s->data));
	n = get_int(pop_obj(s->data));
	push_obj(make_int(n > m ? 1 : 0), &s->data);
	s->ctr++;
}

// CGTE - compare greater than or equal
void do_cgte(STATE *s)
{
	int32_t n, m;

	m = get_int(pop_obj(s->data));
	n = get_int(pop_obj(s->data));
	push_obj(make_int(n >= m ? 1 : 0), &s->data);
	s->ctr++;
}

// ATOM - test if value is an integer
void do_atom(STATE *s)
{
	intptr_t t;

	t = get_tag(pop_obj(s->data));
	push_obj(make_int(t == INT_OBJ ? 1 : 0), &s->data);
	s->ctr++;
}

// CONS - allocate a CONS cell
void do_cons(STATE *s)
{
	void *x, *y;

	y = pop_obj(s->data);
	x = pop_obj(s->data);
	push_obj(make_pair(alloc_pair(x, y)), &s->data);
	s->ctr++;
}

// CAR - extract first element from CONS cell
void do_car(STATE *s)
{
	PAIR *p;

	p = get_pair(pop_obj(s->data));
	push_obj(get_fst(p), &s->data);
	s->ctr++;
}

// CDR - extract second element from CONS cell
void do_cdr(STATE *s)
{
	PAIR *p;

	p = get_pair(pop_obj(s->data));
	push_obj(get_snd(p), &s->data);
	s->ctr++;
}

// SEL - conditional branch
void do_sel(int32_t ta, int32_t fa, STATE *s)
{
	int32_t n;

	n = get_int(pop_obj(s->data));
	push_obj(make_addr(s->ctr + 1), &s->ctrl);
	s->ctr = n ? ta : fa;
}

// JOIN - return from branch
void do_join(STATE *s)
{
	s->ctr = get_addr(pop_obj(s->data));
}

// LDF - load function
// AP - call function
// RTN - return from function call
// DUM - create an empty environment frame
// RAP - recursive environment call function
// STOP - terminate co-processor execution
// TAP - tail-call function
// TRAP - recursive environment tail-call functions
// ST - store to environment
// DBUG - printf debugging
// BRK - breakpoint debugging

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
