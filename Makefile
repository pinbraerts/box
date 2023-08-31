OBJECT = graphics.obj point.obj main.obj 
BINARY = main.exe
RESULT = velocity.bin energy.bin
PYTHON = python
CC = cl.exe
CFLAGS = /nologo /c /O2 /Ot /Ox /EHsc /Fo:
LD = link.exe
LDFLAGS = /nologo d2d1.lib user32.lib ole32.lib /OUT:

build: ${BINARY}

main.obj: main.cpp
	${CC} $< ${CFLAGS}$@

graphics.obj: graphics.cpp
	${CC} $< ${CFLAGS}$@

point.obj: point.cpp
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
