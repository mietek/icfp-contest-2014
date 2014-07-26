///////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define halt(s)       do { fprintf(stderr, s); exit(1); } while (0)
#define haltf(s, ...) do { fprintf(stderr, s, __VA_ARGS__); exit(0); } while (0)

///////////////////////////////////////////////////////////////////////////////

typedef void *value;

typedef enum {
	NO_TAG  = 0x00,
	INT_TAG = 0x01
} value_tag;

#define VALUE_TAG_MASK 0x07

value set_tag(value_tag tag, void *val)
{
	assert((((uint64_t)val) & VALUE_TAG_MASK) == 0);
	return (value)((((uint64_t)val) & ~VALUE_TAG_MASK) | tag);
}

void *clear_tag(value val)
{
	return (void *)((uint64_t)val & ~VALUE_TAG_MASK);
}

value_tag get_tag(value val)
{
	return ((uint64_t)val) & VALUE_TAG_MASK;
}

///////////////////////////////////////////////////////////////////////////////

value make_int(int n)
{
	int *val;

	if (!(val = malloc(sizeof(int)))) {
		halt("Out of memory\n");
	}
	*val = n;
	return set_tag(INT_TAG, val);
}

int get_int(value val)
{
	assert(get_tag(val) == INT_TAG);
	return *(int *)(clear_tag(val));
}

///////////////////////////////////////////////////////////////////////////////

#define INIT_STACK_SIZE 1024

typedef struct {
	size_t size;
	value *base;
	value *top;
	value *limit;
} stack;

stack *data_stack;
stack *control_stack;

stack *make_stack()
{
	stack *stk;

	if (!(stk = malloc(sizeof(stack)))) {
		halt("Out of memory\n");
	}
	stk->size = INIT_STACK_SIZE;
	if (!(stk->base = malloc(stk->size * sizeof(value)))) {
		halt("Out of memory\n");
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
			halt("Out of memory\n");
		}
		stk->top = stk->base + stk->size / 2;
		stk->limit = stk->base + stk->size;
	}
	*(stk->top)++ = val;
}

value pop_value(stack *stk)
{
	if (stk->top == stk->base) {
		halt("Stack underflow\n");
	}
	return *(stk->top)--;
}

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
	int c;
	int n = 0;
	value val;

	while (isdigit((c = getc(in)))) {
		n = (n * 10) + digittoint(c);
	}
	n *= sign;
	ungetc(c, in);
	val = make_int(n);
	push_value(val, data_stack);
	return val;
}

value read(FILE *in)
{
	int c;
	int sign = 1;

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
	fprintf(stderr, "Unexpected character '%c'\n", c);
	exit(1);
}

///////////////////////////////////////////////////////////////////////////////

value eval(value expr)
{
	return expr;
}

///////////////////////////////////////////////////////////////////////////////

void write(value val)
{
	switch (get_tag(val)) {
	case INT_TAG:
		printf("%d", get_int(val));
		break;
	default:
		fprintf(stderr, "Unexpected value tag %d\n", get_tag(val));
		exit(1);
	}
}

///////////////////////////////////////////////////////////////////////////////

int main()
{
	printf("Hello, world!\n");
	data_stack = make_stack();
	control_stack = make_stack();
	while (1) {
		printf("> ");
		write(eval(read(stdin)));
		printf("\n");
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
