
all:
	(cd src; $(MAKE))

allclean:
	(cd src; $(MAKE))
	(cd src; $(MAKE) clean)

clean:
	(cd src; $(MAKE) clean)

cleanall:
	(cd src; $(MAKE) clean)
	-rm -f lib/*.so

.PHONY = all allclean clean cleanall