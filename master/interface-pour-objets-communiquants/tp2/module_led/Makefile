CARD_NUMB       = 22
ROUTER          = peri
LOGIN           = normand-derri
LAB             = lab2

MODULE          = led0_ND

CROSSDIR        = /users/enseig/franck/IOC
KERNELDIR       = /dsk/l1/misc/linux-rpi-3.18.y2
CROSS_COMPILE   = $(CROSSDIR)/arm-bcm2708hardfp-linux-gnueabi/bin/bcm2708hardfp-
        
obj-m           += $(MODULE).o
all: default upload
default:;       make -C $(KERNELDIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules
clean:;         make -C $(KERNELDIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean
upload:;        scp -P622$(CARD_NUMB) $(MODULE).ko pi@$(ROUTER):$(LOGIN)/$(LAB)