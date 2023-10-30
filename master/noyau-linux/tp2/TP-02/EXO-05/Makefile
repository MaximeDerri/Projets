CC = gcc
CFLAGS = -Wall -Wextra -g -Og

SRC = comment.c testComment.c
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