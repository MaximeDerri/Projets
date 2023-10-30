KERNELDIR_PNL ?= /home/hellyiard/Cours/Master/Master1/cours/PNL/td-tp/src/kernel/linux-5.10.167
#KERNELDIR_PNL ?= /lib/modules/$(shell uname -r)/build

obj-m += helloioctl.o

all : mod cpy

mod:
	make -C $(KERNELDIR_PNL) M=$$PWD modules

cpy:
	cp helloioctl.ko /root/pnl/share
	cp usr_ioctl.c /root/pnl/share
	cp interface.h /root/pnl/share

clean:
	make -C $(KERNELDIR_PNL) M=$$PWD clean

.PHONY: all clean mod cpy