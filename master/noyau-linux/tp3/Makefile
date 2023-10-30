.PHONY : all clean check
.ONESHELL:

CFLAGS = -pedantic -Wall -Wextra -fPIC -fno-gnu-unique
LDLIBS = -lfunc
LDFLAGS = -fPIE -pie -L. -Wl,-rpath,.
LDFLAGS_HARDENED = -Wl,-z,relro -Wl,-z,now

CHECK_PATCH=../../../../biblio/eudyptula/torvalds/scripts/checkpatch.pl

all : cron_func libread.so libhackfunc.so

%.o : %.c
	$(CC) $(CFLAGS) -c $<

hack.o: hack.c
	$(CC) $(CFLAGS) -c $< -ldl

libread.so: hack.o
	$(CC) $(CFLAGS) -shared -o $@ $<

libhackfunc.so: hack_func.o
	$(CC) $(CFLAGS) -shared -o $@ $<

libfunc.so: nothing.o
	$(CC) $(CFLAGS) -shared -o $@ $<

cron_func : cron_func.o libfunc.so
	$(CC) $(CFLAGS) $(LDFLAGS_HARDENED) $(LDFLAGS) -o $@ $^ $(LDLIBS)

check :
	for f in *.c *.h ; do
		$(CHECK_PATCH) -f $$f
	done

clean :
	rm -rf *.o cron_func
