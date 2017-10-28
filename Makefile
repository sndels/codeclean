# Base from http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC=gcc
CFLAGS= -Wall -pedantic

SRCDIR=src
OBJ=cleaner.o filelock.o mapped_file.o

all: codeclean test

%.o: $(SRCDIR)/%.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

codeclean: $(OBJ) main.o
		gcc -o $@ $^

test: $(OBJ) test_main.o
		gcc -o $@ $^

.PHONY: clean

clean:
		rm -f *.o
