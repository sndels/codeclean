# Base from http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC=gcc
CFLAGS= -g -Wall -pedantic
LDFLAGS= -g

SRCDIR=src
OBJ=main.o cleaner.o mapped_file.o

%.o: $(SRCDIR)/%.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

codeclean: $(OBJ)
		gcc -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
		rm -f *.o codeclean
