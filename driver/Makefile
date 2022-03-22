obj-m += multiflow-driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	insmod multiflow-driver.ko

uninstall:
	rmmod multiflow-driver

%:
	@echo Available targets:
	@echo "  help"
	@echo "  all"
	@echo "  clean"
	@echo "  install"
	@echo "  uninstall"
	@echo "  install/uninstall must be run as root (use sudo)"
