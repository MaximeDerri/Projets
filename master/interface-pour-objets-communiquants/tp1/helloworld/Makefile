CC=bcm2708hardfp-gcc
CFLAGS=-O2 -static
CARD=22
NAME=normand-derri
CFLAGS=-W -Wall -Wextra -Wfatal-errors -O2
APP=helloworld.x

all: $(APP)
	scp -P 622$(CARD) $^ pi@peri:$(NAME)/

helloworld.x: helloworld.c
	bcm2708hardfp-gcc -o $@ $< -O2 -static

clean:
	rm $(APP)