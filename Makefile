CC=gcc
C-FLAGS=-O3
CPP=g++
CPP-FLAGS=-std=c++14 -O3

all: lc3 lc3-alt lc3.html

lc3.c lc3.cpp: lc3.lit
	lit --tangle $^

lc3.html: lc3.lit
	lit --weave $^

lc3-alt: lc3-alt.cpp
	${CPP} ${CPP-FLAGS} $^ -o $@

lc3: lc3.c
	${CC} ${C-FLAGS} $^ -o $@

clean:
	rm -f *.c
	rm -f *.cpp
	rm -f *.html
	rm -f lc3
	rm -f lc3-alt


