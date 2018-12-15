all: ../lc3.c ../lc3-alt.cpp ../docs/index.html

../lc3.c ../lc3-alt.cpp: index.lit main.css
	lit --tangle $<
	mv lc3.c ../
	mv lc3-alt.cpp ../

index.html: index.lit
	lit --weave $<
	mv index.html ../docs/
	cp main.css ../docs/main.css

