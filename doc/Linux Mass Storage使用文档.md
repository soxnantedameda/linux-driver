# Linux Mass Storage使用文档

## 版本

| 版本 | 修订日期   | 内容     |
| ---- | ---------- | -------- |
| 1.0  | 2024.12.31 | 初始版本 |
|      |            |          |
|      |            |          |

## 概要

​		Linux Mass Storage(Linux 大容量储存设备)，为usb其中一种协议，U盘使用的就是这种协议。本文要介绍的的是，利用该协议将移动嵌入式开发板上的储存设备配置成U盘，能让电脑直接操作开发版上的储存，简而言之，就是把开发板当成U盘使用，以便快速地传输文件。

## 内核配置

```c
CONFIG_USB_GADGET=y
CONFIG_USB_CONFIGFS=y
CONFIG_USB_F_MASS_STORAGE=y
```

## Demo配置脚本

### 初始化脚本

```sh
export VID="0x18D1" 
export PID="0x0001" 
export MANUFACTURER="Google" 
export PRODUCT="hollyland" 
export SERIALNUMBER="lzw123lzw" 
#挂载分区位置,需要先格式化
export MEMORY="/dev/mmcblk0p5"

#挂载configfs(只需执行一次)
mount -t configfs none /sys/kernel/config/

cd /sys/kernel/config/usb_gadget/
mkdir storage
cd storage
#创建mass_storage功能
mkdir functions/mass_storage.0 
#指定挂载的内存块
echo $MEMORY > functions/mass_storage.0/lun.0/file
#配置usb信息
echo $VID > idVendor
echo $PID > idProduct
mkdir strings/0x409
echo $MANUFACTURER > strings/0x409/manufacturer
echo $PRODUCT > strings/0x409/product
echo $SERIALNUMBER > strings/0x409/serialnumber

mkdir configs/c.1/
echo "0xC0" > configs/c.1/bmAttributes
echo "1" > configs/c.1/MaxPower
mkdir configs/c.1/strings/0x409/
echo "Mass Storage" > configs/c.1/strings/0x409/configuration
ln -s functions/mass_storage.0/ configs/c.1/

echo "$(ls /sys/class/udc/)" > UDC
```

### 反初始化脚本

```sh
cd /sys/kernel/config/usb_gadget/storage
#解绑UDC功能
echo > UDC
rm configs/c.1/mass_storage.0
rmdir configs/c.1/strings/0x409
rmdir functions/mass_storage.0
rmdir configs/c.1
rmdir strings/0x409
cd ../
rmdir storage
cd /root/
umount /sys/kernel/config/
```

## 特殊场景1，使用ddr作为临时储存

​		对于使用小储存介质的产品(如spi nand 128M)，需要临时将升级包复制到本地进行固件升级的场景，可利用**Mass Storage**和**tmpfs**将DDR临时划分出一部分做为临时储存

### 参考脚本

```sh
export VID="0x18D1"
export PID="0x0001"
export MANUFACTURER="Google"
export PRODUCT="hollyland"
export SERIALNUMBER="lzw123lzw"
export MEMORY="/mnt/tmpfs/u_disk"

#创建tmpfs 大小64M
mkdir /mnt/tmpfs
mount -t tmpfs -o size=64M tmpfs /mnt/tmpfs/
#创建一个60M块文件
dd if=/dev/zero of=$MEMORY bs=1M count=60
#格式化内存
mkfs.vfat $MEMORY

mount -t configfs none /sys/kernel/config/

cd /sys/kernel/config/usb_gadget/
mkdir storage
cd storage

mkdir functions/mass_storage.0
echo $MEMORY > functions/mass_storage.0/lun.0/file

echo $VID > idVendor
echo $PID > idProduct
mkdir strings/0x409
echo $MANUFACTURER > strings/0x409/manufacturer
echo $PRODUCT > strings/0x409/product
echo $SERIALNUMBER > strings/0x409/serialnumber

mkdir configs/c.1/
echo "0xC0" > configs/c.1/bmAttributes
echo "1" > configs/c.1/MaxPower
mkdir configs/c.1/strings/0x409/
echo "Mass Storage" > configs/c.1/strings/0x409/configuration
ln -s functions/mass_storage.0/ configs/c.1/

echo "$(ls /sys/class/udc/)" > UDC
```

注：电脑访问时，实际访问的是"/mnt/tmpfs/u_disk"这个块内存，linux需要将"/mnt/tmpfs/u_disk"mount到一个临时目录下，才能访问内存块里的内容，如"**mount /mnt/tmpfs/u_disk /tmp/**"

### 反初始化脚本

```sh
cd /sys/kernel/config/usb_gadget/storage
echo > UDC
rm configs/c.1/mass_storage.0
rmdir configs/c.1/strings/0x409
rmdir functions/mass_storage.0
rmdir configs/c.1
rmdir strings/0x409
cd ../
rmdir storage
cd /root/
umount /sys/kernel/config/
#删除ddr临时分区
rm /mnt/tmpfs/u_disk
umount /mnt/tmpfs/
rm -rf /mnt/tmpfs
```

