all: test

.PHONY: build run test clean

build: build/lisp

build/lisp: src/lisp/lisp.c
	@mkdir -p build
	gcc -Wall -O2 -o build/lisp -std=c99 src/lisp/lisp.c
	cp src/lisp/lispinit build/lispinit

run: build/lisp
	cd build; ./lisp

test: build/lisp
	cd build; ./lisp < ../test/lisp/fib.in > fib.out
	diff test/lisp/fib.out build/fib.out

clean:
	rm -rf build
