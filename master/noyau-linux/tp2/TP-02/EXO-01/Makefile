CC = gcc
CFLAGS = -Wall -Wextra -g -Og

SRC = testHistory.c commit.c version.c history.c list.h
OBJ = $(SRC:.c=.o)

all: test

test: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

depend:
	$(CC) -MM $(CFLAGS) $(SRC) > .depends
-include .depends

clean:
	-rm -f *.o test .depends

.PHONY: all depend clean