# If KERNELRELEASE is defined, we've been invoked from the
# # kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
	obj-m := kprobe_mm_fault.o
# # Otherwise we were called directly from the command
# # line; invoke the kernel build system.
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	gcc change_pid.c -o change_pid
	gcc read_book.c -o read_book
install:
	sudo insmod kprobe_mm_fault.ko 
	sudo chmod a+rw /dev/kprobe_mm_fault
clean:
	rm -f *.o
	rm -f *.ko
	rm -f *.mod.c
	rm -rf .tmp*
	rm -f *.symvers
	rm -f *.order
	rm -f .kprobe*
	rm -f change_pid
	rm -f read_book
endif
