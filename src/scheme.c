///////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

#define halt(s) \
	do { \
		fprintf(stderr, s); \
		exit(1); \
	} while (0)

#define haltf(s, ...) \
	do { \
		fprintf(stderr, s, __VA_ARGS__); \
		exit(0); \
	} while (0)

///////////////////////////////////////////////////////////////////////////////

typedef void *value;

///////////////////////////////////////////////////////////////////////////////

#define INIT_STACK_SIZE 1024

typedef struct {
	size_t size;
	value *base, *top, *limit;
} stack;

stack *make_stack()
{
	stack *stk;

	if (!(stk = malloc(sizeof(stack)))) {
		halt("make_stack: Out of memory\n");
	}
	stk->size = INIT_STACK_SIZE;
	if (!(stk->base = malloc(stk->size * sizeof(value)))) {
		halt("make_stack: Out of memory\n");
	}
	stk->top = stk->base;
	stk->limit = stk->base + stk->size;
	return stk;
}

void push_value(value val, stack *stk)
{
	if (stk->top == stk->limit) {
		stk->size *= 2;
		if (!(stk->base = realloc(stk->base, stk->size * sizeof(value)))) {
			halt("push_value: Out of memory\n");
		}
		stk->top = stk->base + stk->size / 2;
		stk->limit = stk->base + stk->size;
	}
	*(stk->top)++ = val;
}

value pop_value(stack *stk)
{
	if (stk->top == stk->base) {
		halt("pop_value: Stack underflow\n");
	}
	return *(stk->top)--;
}

///////////////////////////////////////////////////////////////////////////////

typedef enum {
	NO_TAG   = 0x00,
	INT_TAG  = 0x01,
	CONS_TAG = 0x02
} tag;

#define TAG_MASK 0x07

value set_tag(tag tag, void *val)
{
	assert((((uint64_t)val) & TAG_MASK) == 0);
	return (value)((((uint64_t)val) & ~TAG_MASK) | tag);
}

void *clear_tag(value val)
{
	return (void *)((uint64_t)val & ~TAG_MASK);
}

tag get_tag(value val)
{
	return ((uint64_t)val) & TAG_MASK;
}

///////////////////////////////////////////////////////////////////////////////

value make_int(int n)
{
	int *val;

	if (!(val = malloc(sizeof(int)))) {
		halt("make_int: Out of memory\n");
	}
	*val = n;
	return set_tag(INT_TAG, val);
}

int get_int(value val)
{
	assert(get_tag(val) == INT_TAG);
	return *(int *)(clear_tag(val));
}

#define POP_INT_INT(x, y, stk) \
	do { \
		y = pop_value(stk); \
		x = pop_value(stk); \
		if (get_tag(x) != INT_TAG || get_tag(y) != INT_TAG) { \
			fprintf(stderr, "%s: Unexpected argument: ", __func__); \
			write(x, stderr); \
			halt("\n"); \
		} \
	} while (0) \

///////////////////////////////////////////////////////////////////////////////

typedef struct {
	value car, cdr;
} cons;

value make_cons(value car, value cdr)
{
	cons *val;

	if (!(val = malloc(sizeof(cons)))) {
		halt("make_cons: Out of memory\n");
	}
	val->car = car;
	val->cdr = cdr;
	return set_tag(CONS_TAG, val);
}

value get_car(value val)
{
	assert(get_tag(val) == CONS_TAG);
	return ((cons *)(clear_tag(val)))->car;
}

value get_cdr(value val)
{
	assert(get_tag(val) == CONS_TAG);
	return ((cons *)(clear_tag(val)))->cdr;
}

#define POP_CONS(x, stk) \
	do { \
		x = pop_value(stk); \
		if (get_tag(x) != CONS_TAG) { \
			fprintf(stderr, "%s: Unexpected argument: ", __func__); \
			write(x, stderr); \
			halt("\n"); \
		} \
	} while (0) \

///////////////////////////////////////////////////////////////////////////////

#define COMMENT_CHAR    ';'
#define NEG_FIXNUM_CHAR '-'

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

value read_int(FILE *in, int sign)
{
	int c, n = 0;

	while (isdigit((c = getc(in)))) {
		n = (n * 10) + digittoint(c);
	}
	n *= sign;
	ungetc(c, in);
	return make_int(n);
}

value read(FILE *in)
{
	int c, sign = 1;

	skip_spaces(in);
	if ((c = getc(in)) == EOF) {
		printf("\n");
		exit(0);
	}
	if (isdigit(c) || (c == NEG_FIXNUM_CHAR && isdigit(peekc(in)))) {
		if (c == NEG_FIXNUM_CHAR) {
			sign = -1;
		} else {
			ungetc(c, in);
		}
		return read_int(in, sign);
	}
	haltf("read: Unexpected character: '%c'\n", c);
}

///////////////////////////////////////////////////////////////////////////////

value eval(value expr)
{
	return expr;
}

///////////////////////////////////////////////////////////////////////////////

void write(value val, FILE *out)
{
	switch (get_tag(val)) {
	case INT_TAG:
		fprintf(out, "%d", get_int(val));
		break;
	default:
		haltf("write: Unexpected tag: %d\n", get_tag(val));
	}
}

///////////////////////////////////////////////////////////////////////////////

int prog_ctr = 0;
stack *data_stk = NULL;
stack *ctrl_stk = NULL;
value *env_frm = NULL;

// LDC - load constant
void do_ldc(int n)
{
	push_value(make_int(n), data_stk);
	prog_ctr++;
}

// LD - load from environment
// TODO

// ADD - integer addition
void do_add()
{
	value x, y;

	POP_INT_INT(x, y, data_stk);
	push_value(make_int(get_int(x) + get_int(y)), data_stk);
	prog_ctr++;
}

// SUB - integer subtraction
void do_sub()
{
	value x, y;

	POP_INT_INT(x, y, data_stk);
	push_value(make_int(get_int(x) - get_int(y)), data_stk);
	prog_ctr++;
}

// MUL - integer multiplication
void do_mul()
{
	value x, y;

	POP_INT_INT(x, y, data_stk);
	push_value(make_int(get_int(x) * get_int(y)), data_stk);
	prog_ctr++;
}

// DIV - integer division
void do_div()
{
	value x, y;

	POP_INT_INT(x, y, data_stk);
	push_value(make_int(get_int(x) / get_int(y)), data_stk);
	prog_ctr++;
}

// CEQ - compare equal
void do_ceq()
{
	value x, y;

	POP_INT_INT(x, y, data_stk);
	push_value(make_int(get_int(x) == get_int(y) ? 1 : 0), data_stk);
	prog_ctr++;
}

// CGT - compare greater than
void do_cgt()
{
	value x, y;

	POP_INT_INT(x, y, data_stk);
	push_value(make_int(get_int(x) > get_int(y) ? 1 : 0), data_stk);
	prog_ctr++;
}

// CGTE - compare greater than or equal
void do_cgte()
{
	value x, y;

	POP_INT_INT(x, y, data_stk);
	push_value(make_int(get_int(x) >= get_int(y) ? 1 : 0), data_stk);
	prog_ctr++;
}

// ATOM - test if value is an integer
void do_atom()
{
	value x;

	x = pop_value(data_stk);
	push_value(make_int(get_tag(x) == INT_TAG ? 1 : 0), data_stk);
	prog_ctr++;
}

// CONS - allocate a CONS cell
void do_cons()
{
	value x, y;

	y = pop_value(data_stk);
	x = pop_value(data_stk);
	push_value(make_cons(x, y), data_stk);
	prog_ctr++;
}

// CAR - extract first element from CONS cell
void do_car()
{
	value x;

	POP_CONS(x, data_stk);
	push_value(get_car(x), data_stk);
	prog_ctr++;
}

// CDR - extract second element from CONS cell
void do_cdr()
{
	value x;

	POP_CONS(x, data_stk);
	push_value(get_cdr(x), data_stk);
	prog_ctr++;
}

// SEL - conditional branch
// JOIN - return from branch
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

void init()
{
	prog_ctr = 0;
	data_stk = make_stack();
	ctrl_stk = make_stack();
	// TODO: env_frm
}

///////////////////////////////////////////////////////////////////////////////

int main()
{
	printf("Hello, world!\n");
	init();
	while (1) {
		printf("> ");
		write(eval(read(stdin)), stdout);
		printf("\n");
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
