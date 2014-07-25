all: build

.PHONY: build run clean

build: build/lisp

build/lisp: src/lisp/lisp.c
	@mkdir -p build
	gcc -Wall -O2 -o build/lisp -std=c99 src/lisp/lisp.c
	cp src/lisp/lispinit build/lispinit

run: build/lisp
	cd build && ./lisp

clean:
	rm -rf build
