CC=gcc
CFLAGS=-std=c11 -O2 -Iinclude
SRC=\
	src/am1_term.c \
	src/am1_core.c \
	src/am1_observer.c \
	src/main.c
all: am1_rewrite_run
am1_rewrite_run:
	$(CC) $(CFLAGS) $(SRC) -o am1_rewrite_run
clean:
	rm -f am1_rewrite_run
