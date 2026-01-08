
obj-m += hid-powera.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	sudo cp hid-powera.ko /lib/modules/$(shell uname -r)/kernel/drivers/hid/
	sudo depmod -a
	@echo "Driver installed.  Run 'sudo modprobe hid-powera' to load it."

uninstall:
	sudo rm -f /lib/modules/$(shell uname -r)/kernel/drivers/hid/hid-powera.ko
	sudo depmod -a
	@echo "Driver uninstalled."

load:
	sudo modprobe hid-powera

unload:
	sudo modprobe -r hid-powera

reload:  unload load

test:
	@echo "Testing driver..."
	lsmod | grep hid_powera || echo "Driver not loaded"
	dmesg | tail -20 | grep -i powera || echo "No kernel messages"
