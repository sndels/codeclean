# Base from http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC=gcc
CFLAGS=-Wall -pedantic

SRCDIR=src
OBJ=main.o cleaner.o mapped_file.o

%.o: $(SRCDIR)/%.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

codeclean: $(OBJ)
		gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
		rm -f *.o codeclean
