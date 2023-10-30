CC = gcc
CFLAGS = -fPIC -pedantic -Wall -Wextra -Wformat=2 -Wformat-security -Werror=format-security
CPPFLAGS = -I. -I../include
LDLIBS = -lpthread
CFLAGS_HARDENED = -g -Og   # -fstack-protector-all -D_FORTIFY_SOURCE=2 -O2   |   NOTE: -g -Og is only for debug phase

SRC = dsm.c master.c slave.c request.c lock_demand.c#TODO
OBJ = $(SRC:.c=.o)

#MACRO
A = -DARCHI=64
B = -DBUFFER=-1

all: ../lib/libdsm.so

../lib/libdsm.so: $(OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) -shared -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_HARDENED) $(A) $(B) -c -o $@ $^ $(LDLIBS)

depend:
	$(CC) -MM $(CPPFLAGS) $(SRC) > .depends
-include .depends

clean:
	-rm -f $(OBJ) .depends

.PHONY = all clean