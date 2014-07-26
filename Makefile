all: test

.PHONY: build run test clean

build: build/scheme

build/scheme: src/scheme.c
	@mkdir -p build
	gcc -Wall -O2 -o build/scheme -std=c99 src/scheme.c

run: build/scheme
	build/scheme

test: build/scheme
	build/scheme <test/test.s >build/test.out
	diff -u test/test.out build/test.out

clean:
	rm -rf build
