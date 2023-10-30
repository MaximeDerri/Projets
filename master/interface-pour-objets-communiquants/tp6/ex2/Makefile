CC = gcc
CFLAGS = -Wall -Wextra

all: serv cli

serv:
	$(CC) $(CFLAGS) -o server server.c

cli:
	$(CC) $(CFLAGS) -o client client.c

clean:
	-@rm -f *.o client server

.PHONY = all serv cli clean