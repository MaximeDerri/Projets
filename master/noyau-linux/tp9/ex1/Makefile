KERNELDIR_PNL ?= /home/hellyiard/Cours/Master/Master1/cours/PNL/td-tp/src/kernel/linux-5.10.167
#KERNELDIR_PNL ?= /lib/modules/$(shell uname -r)/build

obj-m += one_addr.o

all : mod cpy

mod:
	make -C $(KERNELDIR_PNL) M=$$PWD modules

cpy:
	cp one_addr.ko /root/pnl/share

clean:
	make -C $(KERNELDIR_PNL) M=$$PWD clean

.PHONY: all clean mod cpy