obj-m += vfifo.o

# KDIR should point to the kernel headers. 
# On standard Linux/WSL, this is usually /lib/modules/$(shell uname -r)/build
KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
