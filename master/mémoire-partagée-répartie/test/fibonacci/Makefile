CC = gcc
CFLAGS = -pedantic -Wall -Wextra -Wformat -Wformat-security -Werror=format-security
CPPFLAGS = -I. -I../../include
LDFLAGS = -fPIE -pie -L../../lib -Wl,-rpath,../../lib
LDLIBS = -ldsm -lpthread -lm
LDFLAGS_HARDENED = -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
CFLAGS_HARDENED = -fstack-protector-strong -D_FORTIFY_SOURCE=2 -O1  # -g -Og  |   NOTE: -g -Og is only for debug phase

SRC = fibonacci.c
OBJ = $(SRC:.c=.o)

all: fibonacci

fibonacci: $(OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(LDFLAGS_HARDENED) $(CFLAGS_HARDENED) -o $@ $(OBJ) $(LDLIBS)

%.o:%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_HARDENED) -c -o $@ $^

depend:
	$(CC) -MM $(CPPFLAGS) $(SRC) > .depends
-include .depends

clean:
	-rm -f $(OBJ) .depends

cleanall:
	-rm -f $(OBJ) .depends fibonacci

.PHONY = all clean cleanall