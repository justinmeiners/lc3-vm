
lc3: lc3.c
	gcc $^ -I /usr/local/include -L /usr/local/lib -l termbox -o $@
