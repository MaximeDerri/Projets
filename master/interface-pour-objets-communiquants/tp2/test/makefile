NAME = normand-derri # nom1_nom2
RPI ?= 22 # card_number

CROSS_COMPILE ?= bcm2708hardfp-

CFLAGS=-Wall -O2 
LDFLAGS=-static -I../led -lpthread -D_REENTRANT 

help:
	@echo ""
	@echo "make [RPI=num] target"
	@echo ""
	@echo "      target :	help	this message"
	@echo " 		all	clean all and recompile"
	@echo " 		dep	compile if required"
	@echo " 		clean	clean all except sources"
	@echo " 		copy	compile then copy to card $(RPI) in directory $(NAME)"
	@echo ""

all: clean dep copy

dep : test.c

copy: test.x
	scp -P 622$(RPI) $^ pi@peri:$(NAME)

%.x: %.c
	$(CROSS_COMPILE)gcc -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o *.x *~ *.a