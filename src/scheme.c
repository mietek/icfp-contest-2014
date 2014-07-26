///////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

typedef enum {
	FIXNUM_TYPE,
	CONS_TYPE,
	CLOSURE_TYPE
} object_type;

typedef int32_t fixnum;

typedef struct object {
	object_type type;
	union {
		struct {
			fixnum value;
		} fixnum;
	} data;
} object;

object *alloc_object()
{
	object *obj;

	if (!(obj = malloc(sizeof(object)))) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	return obj;
}

object *make_fixnum(fixnum val)
{
	object *obj;

	obj = alloc_object();
	obj->type = FIXNUM_TYPE;
	obj->data.fixnum.value = val;
	return obj;
}

int is_fixnum(object *obj)
{
	return obj->type == FIXNUM_TYPE;
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

void skip_space(FILE *in)
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

	while (isdigit((c = getc(in)))) {
		val = (val * 10) + digittoint(c);
	}
	val *= sign;
	ungetc(c, in);
	return make_fixnum(val);
}

object *read(FILE *in)
{
	int c;
	int sign = 1;

	skip_space(in);
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
	switch (obj->type) {
	case FIXNUM_TYPE:
		printf("%d", obj->data.fixnum.value);
		break;
	case CONS_TYPE:
		break;
	case CLOSURE_TYPE:
		break;
	default:
		fprintf(stderr, "Unexpected object type %d\n", obj->type);
		exit(1);
	}
}

///////////////////////////////////////////////////////////////////////////////

int main()
{
	printf("Hello, world!\n");
	while (1) {
		printf("> ");
		write(eval(read(stdin)));
		printf("\n");
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
