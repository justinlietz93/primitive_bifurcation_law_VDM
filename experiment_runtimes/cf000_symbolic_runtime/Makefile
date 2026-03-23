CC=gcc
CFLAGS=-std=c11 -O2 -Iinclude

SRC=\
	src/cf000_state.c \
	src/cf000_core.c \
	src/cf000_observer.c \
	src/main.c

all: cf000_run

cf000_run:
	$(CC) $(CFLAGS) $(SRC) -o cf000_run

clean:
	rm -f cf000_run
