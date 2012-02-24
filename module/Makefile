obj-m += eeefsb.o
eeefsb-objs := eeefsb_main.o ec.o pll.o eeefsb_wq.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

