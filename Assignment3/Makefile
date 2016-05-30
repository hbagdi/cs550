# If KERNELRELEASE is defined, we've been invoked from the
# # kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
	obj-m := pipe_drv.o
# # Otherwise we were called directly from the command
# # line; invoke the kernel build system.
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	
	gcc producer.c -o producer
	gcc consumer.c -o consumer
install:
	sudo insmod pipe_drv.ko 
	sudo chmod a+rw /dev/pipe-file
clean:
	rm -f *.o
	rm -f *.ko
	rm -f *.mod.c
	rm -f *.symvers
	rm -f *.order
	rm -f producer
	rm -f consumer
endif
