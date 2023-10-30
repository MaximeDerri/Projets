all: comp

comp:
	cd tree_code/src && $(MAKE)
	-mkdir graph_code/src/.result
	-rm -f graph_code/src/.result/*

install:
	cd tree_code/lib/BigInt-0.5.0-dev  && $(MAKE) release

cleanup: clean
	-cd tree_code/lib/BigInt-0.5.0-dev && $(MAKE) clean
	rm -f tree_code/lib/BigInt-0.5.0-dev/release/*


clean:
	rm -f analyzer
	cd tree_code/src && $(MAKE) clean

.PHONY: all install cleanup clean