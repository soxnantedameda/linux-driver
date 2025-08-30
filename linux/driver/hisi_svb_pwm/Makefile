#obj-$(CONFIG_HISI_SVB_PWM) += hisi_svb_pwm.o

obj-m = hisi_svb_pwm.o

export ARCH=arm64
export CROSS_COMPILE=aarch64-v01c01-linux-gnu-
KDIR := ../../../../linux-5.10.y
PWD := $(shell pwd)
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean