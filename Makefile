all: docs/google-code-prettify docs/src/lc3.c docs/src/lc3-win.c docs/src/lc3-alt.cpp docs/src/lc3-alt-win.cpp docs/index.html

docs/google-code-prettify:
	srcweave-format-init docs

docs/src/lc3.c docs/src/lc3-win.c docs/src/lc3-alt.cpp docs/src/lc3-alt-win.cpp: index.lit
	srcweave --tangle ./docs/src/ $<

docs/index.html: index.lit
	srcweave --weave ./docs/ --formatter srcweave-format $<

.PHONY:
clean:
	rm -f docs/src/lc3.c
	rm -f docs/src/lc3-win.c
	rm -f docs/src/lc3-alt.cpp
	rm -f docs/src/lc3-alt-win.cpp
	rm -f docs/index.html
