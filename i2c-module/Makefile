SDK_DIR = /home/dar/riscv/milkv/duo-buildroot-sdk
KERN_DIR = $(SDK_DIR)/linux_5.10/build/cv1800b_milkv_duo_sd

all:
	make -C $(KERN_DIR) M=$(PWD) modules
	$(CROSS_COMPILE)gcc lsm6dsr-app.c -o lsm6dsr-app -Wall -pthread -O2

clean:
	make -C $(KERN_DIR) M=$(PWD) modules clean
	rm -rf modules.order
	rm -rf lsm6dsr-app

obj-m += i2c-lsm6dsr.o