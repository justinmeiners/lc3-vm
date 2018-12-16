CC=gcc
C-FLAGS=-O3
CPP=g++
CPP-FLAGS=-std=c++14 -O3

all: lc3 lc3-alt

lc3-alt: lc3-alt.cpp
	${CPP} ${CPP-FLAGS} $^ -o $@

lc3: lc3.c
	${CC} ${C-FLAGS} $^ -o $@

.PHONY:
clean:
	rm -f lc3
	rm -f lc3-alt
