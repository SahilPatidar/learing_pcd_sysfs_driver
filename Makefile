
obj-m := pcd_sysfs.o
pcd_sysfs-objs += pcd_platform_driver_dt_sysfs.o pcd_syscalls.o

HOST_KERN_DIR = /lib/modules/$(shell uname -r)/build/

host:
	make -C $(HOST_KERN_DIR) M=$(PWD) modules

clean:
	make -C $(HOST_KERN_DIR) M=$(PWD) clean

copy-dtb:
	scp /home/sahil/linux/arch/arm/boot/dts/am355x-boneblack.dtb debain@192
	
copy-drv:
	scp *.ko debain@192 
