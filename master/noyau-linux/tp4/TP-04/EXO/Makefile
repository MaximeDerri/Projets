KERNELDIR_PNL ?= /home/hellyiard/Cours/Master/Master1/cours/PNL/td-tp/src/kernel/linux-5.10.167
#/lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m += hideModule.o #update_sb.oshow_sb.o uname.ohelloWorldParam.o helloWorld.o

all :
	make -C $(KERNELDIR_PNL) M=$(PWD) modules

clean:
	make -C $(KERNELDIR_PNL) M=$(PWD) clean
#cp helloWorld.ko /root/pnl/share/

.PHONY = all clean