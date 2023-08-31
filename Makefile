OBJECT = graphics.obj point.obj main.obj 
BINARY = main.exe
RESULT = velocity.bin energy.bin
PYTHON = python
CC = cl.exe
CFLAGS = /nologo /c /O2 /Ot /Ox /EHsc /Fo:
LD = link.exe
LDFLAGS = /nologo d2d1.lib user32.lib ole32.lib /OUT:

build: ${BINARY}

main.obj: main.cpp graphics.hpp
	${CC} $< ${CFLAGS}$@

graphics.obj: graphics.cpp graphics.hpp point.hpp
	${CC} $< ${CFLAGS}$@

point.obj: point.cpp point.hpp
	${CC} $< ${CFLAGS}$@

${BINARY}: ${OBJECT}
	${LD} ${OBJECT} ${LDFLAGS}${BINARY}

run: ${BINARY}
	${BINARY}

clean:
	rm ${BINARY} || true
	rm ${RESULT} || true
	rm ${OBJECT} || true

plot: run plot.py
	${PYTHON} plot.py

.PHONY: link build run plot ${RESULT}
