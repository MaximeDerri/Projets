CC = gcc
CFLAGS = -pedantic -Wall -Wextra -Wformat -Wformat-security -Werror=format-security
CPPFLAGS = -I. -I../../include
LDFLAGS = -fPIE -pie -L../../lib -Wl,-rpath,../../lib
LDLIBS = -ldsm -lpthread -lm
LDFLAGS_HARDENED = -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
CFLAGS_HARDENED = -fstack-protector-strong -D_FORTIFY_SOURCE=2 -O1  # -g -Og  |   NOTE: -g -Og is only for debug phase

SRC = master.c slave.c
OBJ = $(SRC:.c=.o)

all: master slave

master: master.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(LDFLAGS_HARDENED) $(CFLAGS_HARDENED) -o $@ $^ $(LDLIBS)

slave: slave.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(LDFLAGS_HARDENED) $(CFLAGS_HARDENED) -o $@ $^ $(LDLIBS)

%.o:%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_HARDENED) -c -o $@ $^

depend:
	$(CC) -MM $(CPPFLAGS) $(SRC) > .depends
-include .depends

clean:
	-rm -f $(OBJ) .depends

cleanall:
	-rm -f $(OBJ) .depends master slave

.PHONY = all clean cleanall