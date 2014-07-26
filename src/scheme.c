///////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define halt(s)       do { fprintf(stderr, s); exit(1); } while (0)
#define haltf(s, ...) do { fprintf(stderr, s, __VA_ARGS__); exit(0); } while (0)

///////////////////////////////////////////////////////////////////////////////

typedef enum {
	NO_TYPE     = 0,
	FIXNUM_TYPE = 1
} object_type;

typedef int32_t fixnum;

typedef struct {
	object_type type;
	union {
		struct {
			fixnum value;
		} fixnum;
	} data;
} object;

object *make_object()
{
	object *obj;

	if (!(obj = malloc(sizeof(object)))) {
		halt("Out of memory\n");
	}
	obj->type = NO_TYPE;
	return obj;
}

object *make_fixnum(fixnum val)
{
	object *obj;

	obj = make_object();
	obj->type = FIXNUM_TYPE;
	obj->data.fixnum.value = val;
	return obj;
}

object_type get_type(object *obj)
{
	assert(obj->type != NO_TYPE);
	return obj->type;
}

int is_fixnum(object *obj)
{
	return get_type(obj) == FIXNUM_TYPE;
}

fixnum get_fixnum(object *obj)
{
	assert(is_fixnum(obj));
	return obj->data.fixnum.value;
}

///////////////////////////////////////////////////////////////////////////////

typedef void *tagged_object;

typedef enum {
	NO_TAG     = 0x00,
	FIXNUM_TAG = 0x01
} object_tag;

#define OBJECT_TAG_MASK 0x07

tagged_object tag_object(object_tag tag, void *obj)
{
	assert((((uint64_t)obj) & OBJECT_TAG_MASK) == 0);
	return (tagged_object)((((uint64_t)obj) & ~OBJECT_TAG_MASK) | tag);
}

void *untag_object(tagged_object obj)
{
	return (void *)((uint64_t)obj & ~OBJECT_TAG_MASK);
}

object_tag get_tag(tagged_object obj)
{
	return ((uint64_t)obj) & OBJECT_TAG_MASK;
}

///////////////////////////////////////////////////////////////////////////////

#define INIT_STACK_SIZE 1024

typedef struct {
	size_t size;
	tagged_object *base;
	tagged_object *top;
	tagged_object *limit;
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
	if (!(stk->base = malloc(stk->size * sizeof(object *)))) {
		halt("Out of memory\n");
	}
	stk->top = stk->base;
	stk->limit = stk->base + stk->size;
	return stk;
}

void push_object(stack *stk, tagged_object obj)
{
	if (stk->top == stk->limit) {
		stk->size *= 2;
		if (!(stk->base = realloc(stk->base, stk->size * sizeof(object *)))) {
			halt("Out of memory\n");
		}
		stk->top = stk->base + stk->size / 2;
		stk->limit = stk->base + stk->size;
	}
	*(stk->top)++ = obj;
}

void push_fixnum(stack *stk, object *obj)
{
	assert(is_fixnum(obj));
	push_object(stk, tag_object(FIXNUM_TAG, obj));
}

tagged_object pop_object(stack *stk)
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

object *read_fixnum(FILE *in, int sign)
{
	int c;
	fixnum val = 0;
	object *obj;

	while (isdigit((c = getc(in)))) {
		val = (val * 10) + digittoint(c);
	}
	val *= sign;
	ungetc(c, in);
	obj = make_fixnum(val);
	push_fixnum(data_stack, obj);
	return obj;
}

object *read(FILE *in)
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
		return read_fixnum(in, sign);
	}
	fprintf(stderr, "Unexpected character '%c'\n", c);
	exit(1);
}

///////////////////////////////////////////////////////////////////////////////

object *eval(object *expr)
{
	return expr;
}

///////////////////////////////////////////////////////////////////////////////

void write(object *obj)
{
	switch (get_type(obj)) {
	case FIXNUM_TYPE:
		printf("%d", get_fixnum(obj));
		break;
	default:
		fprintf(stderr, "Unexpected object type %d\n", get_type(obj));
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
