all: docs/src/lc3.c docs/src/lc3-alt.cpp docs/index.html

docs/src/lc3.c docs/src/lc3-alt.cpp: index.lit main.css
	lit --tangle $<
	mv lc3.c docs/src/
	mv lc3-alt.cpp docs/src/

docs/index.html: index.lit
	lit --weave $<
	mv index.html docs/
	cp main.css docs/main.css

clean:
	rm -f docs/src/lc3.c
	rm -f docs/src/lc3-alt.cpp
	rm -f docs/index.html
	rm -f docs/main.css

