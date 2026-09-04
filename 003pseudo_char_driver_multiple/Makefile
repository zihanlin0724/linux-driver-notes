obj-m := pseudo_char_driver_multiple.o
KDIR := /lib/modules/$(shell uname -r)/build
CC   := gcc-14

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules CC=$(CC)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

