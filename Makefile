all: docs/src/lc3.c docs/src/lc3-win.c docs/src/lc3-alt.cpp docs/src/lc3-alt-win.cpp docs/index.html

docs/src/lc3.c docs/src/lc3-win.c docs/src/lc3-alt.cpp docs/src/lc3-alt-win.cpp: index.lit
	lit --tangle --out-dir ./docs/src/ $<

docs/index.html: index.lit main.css
	lit --weave --out-dir ./docs/ $<

.PHONY:
clean:
	rm -f docs/src/lc3.c
	rm -f docs/src/lc3-win.c
	rm -f docs/src/lc3-alt.cpp
	rm -f docs/src/lc3-alt-win.cpp
	rm -f docs/index.html
