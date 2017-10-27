# Base from http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC=gcc
CFLAGS= -g -Wall -pedantic
LDFLAGS= -g

SRCDIR=src
OBJ=cleaner.o mapped_file.o filelock.o

all: codeclean test

%.o: $(SRCDIR)/%.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

codeclean: $(OBJ) main.o
		gcc -o $@ $^ $(LDFLAGS)

test: $(OBJ) test_main.o
		gcc -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
		rm -f *.o
