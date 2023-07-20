all: sgfs

# This second rule lists the dependencies of the mysimulator binary
# How this dependencies are linked is described in an implicit rule below
sgfs: sgfs.o

# These third give the dependencies of the each source file
sgfs.o: sgfs.cpp # list every .h that you use

# Some configuration
CC = g++
WARNING = -Wshadow -Wcast-align -Waggregate-return -Wmissing-prototypes \
	-Wmissing-declarations -Wstrict-prototypes -Wmissing-prototypes \
	-Wmissing-declarations -Wmissing-noreturn -Wredundant-decls \
	-Wnested-externs -Wpointer-arith -Wwrite-strings -finline-functions

CFLAGS=-g -O2 $(WARNINGS) $(shell pkg-config --libs --cflags simgrid fuse3)

%: %.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *~
.PHONY: clean
