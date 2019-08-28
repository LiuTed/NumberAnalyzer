CC ?= gcc
CFLAG := -O2

.PHONY: all clean
all: analyzer analyzer.so

analyzer: main.c analyzer.h
	$(CC) main.c -o analyzer -ldl $(CFLAG) -Wl,-rpath=.

analyzer.so: default.c analyzer.h
	$(CC) default.c -fPIC -shared -o analyzer.so

clean:
	-rm analyzer analyzer.so
