# changer les valeurs par dÃ©faut de RPI et DIR
RPI?=22
DIR?=normand_derri
APP=testIoctl

CFLAGS=-Wall -Wfatal-errors -O2
LDFLAGS=-static -L$(LIB) -lgpio

KERNELDIR       = /dsk/l1/misc/linux-rpi-3.18.y
CROSSDIR        = /users/enseig/franck/IOC
CROSS_COMPILE   = $(CROSSDIR)/arm-bcm2708hardfp-linux-gnueabi/bin/bcm2708hardfp-

all:  $(APP).x upload

$(APP).x: $(APP).c
	$(CROSS_COMPILE)gcc -g3 -o $@ $< -I$(INC) -O2 -static

upload: 
	scp -P622$(RPI) $(APP).x pi@peri:$(DIR)

clean:
	rm -f *.o *.x *~