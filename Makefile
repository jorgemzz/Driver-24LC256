ifeq ($(KERNELRELEASE),)
$(info Buiding from command line)
$(info )

KERNELDIR ?= $(HOME)/Documents/linux-kernel-labs/linux
PWD := $(shell pwd)


modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *.cmd *.ko *.mod.c .tmp_versions

.PHONY: modules modules_install clean

else

$(info Building with KERNELRELEASE = ${KERNELRELEASE})
$(info )
obj-m:=ee24lc256.o

endif
