SOURCE = main.cpp
OBJECT = main.obj
BINARY = main.exe
RESULT = hist.bin
PYTHON = python
LDFLAGS = /nologo /out:
CFLAGS =  /nologo /EHsc /Fo:
CC = cl.exe
LL = link.exe

build: ${BINARY}

${RESULT}: ${BINARY}
	${BINARY}

${OBJECT}: ${SOURCE}
	${CC} ${SOURCE} ${CFLAGS}${OBJECT}

${BINARY}: ${OBJECT}
	${LL} ${OBJECT} ${LDFLAGS}${BINARY}

link: ${OBJECT}
run: ${RESULT}

clean:
	del ${BINARY}
	del ${OBJECT}

plot: ${RESULT} plot.py
	${PYTHON} plot.py

.PHONY: link build run plot ${RESULT}
