KERNELDIR_PNL ?= /home/hellyiard/Cours/Master/Master1/cours/PNL/td-tp/src/kernel/linux-5.10.167
#KERNELDIR_PNL ?= /lib/modules/$(shell uname -r)/build

obj-m += kcpustat.o

all :
	$(info obj-m : $(obj-m))
	make -C $(KERNELDIR_PNL) M=$$PWD modules

clean:
	make -C $(KERNELDIR_PNL) M=$$PWD clean
