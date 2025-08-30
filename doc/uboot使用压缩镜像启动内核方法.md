# uboot使用压缩镜像启动内核方法

## 版本

| 版本 | 修订日期   | 内容            |
| ---- | ---------- | --------------- |
| 1.0  | 2024.10.12 | 初始版本        |
| 1.1  | 2025.8.18  | 1. 更新示例     |
|      |            | 2. 更新部分说明 |

## 简述

​		使用gzip对内核镜像进行压缩，减少内核对储存空间的占用，该方法理论上在uboot上是通用的。

## 要点

​		将sdk编译出的内核镜像使用gzip工具压缩，烧录到主板上后，然后在uboot中使用gzip解压。

​		能使linux内核占用大小减少50%+，对于flash(spi-nand、spi-nor)储存介质能极大减少空间占用，提升内核加载速度。

以下以spi-nand为例

启动非压缩内核命令：

```sh
mtd read spi-nand0 0x46000000 0x1c0000 0x20000;mtd read spi-nand0 0x23080000 0x200000 0xc00000;booti 0x23080000 - 0x46000000
```

启动压缩内核命令：

```sh
mtd read spi-nand0 0x46000000 0x1c0000 0x20000;mtd read spi-nand0 0x50000000 0x200000 0x600000;unzip 0x50000000 0x23080000;booti 0x23080000 - 0x46000000
```

### 命令解析

​		mtd为uboot读取flash介质的统一接口命令，使用方法本文不做详细介绍。

​		unzip为uboot解压gzip命令。

```sh
# 从flash芯片上偏移0x1c0000读取数据0x20000大小数据到内存地址0x51000000，该数据为内核dtb镜像
mtd read spi-nand0 0x46000000 0x1c0000 0x20000
```

```sh
# 从flash芯片上偏移0x200000读取数据0x600000大小数据到内存地址0x50000000，该数据为内核kernel镜像
mtd read spi-nand0 0x50000000 0x200000 0x600000
```

```sh
# 解压内存0x50000000数据到0x23080000
unzip 0x50000000 0x23080000
```

```sh
# 启动内核，加载内核和dtb镜像
booti 0x23080000 - 0x46000000
```

**注：并非所有原厂都实现了mtd命令，实际操作，依照原厂提供的文档为准，如spi-nand为nand，spi-nor为sf。**

### 提升启动速度原理

​		flash不同于mmc储存，其读取速度较慢，通常只有6M/ms左右，所以，假设一个内核大小为12M，那么从flash读取内核到内存的时间为2s左右，通过压缩内核的方法使内核减小的6M左右，就能大幅减少读取内核的时间，从而加快开启速度。

## **SDK修改**

﻿[ osdrv: add gzip kernel (cb0bd560) · Commits · C4902 / BSP · GitLab ](http://192.168.2.88/c4902/bsp/commit/cb0bd560cf3e9eb408feced4770a09c8e31bbfe9)﻿

## 扩展

uboot不单单支持gzip格式个压缩包，还支持lzma、lz4、lzo、bz2格式的压缩包解压。原生的uboot已

经支持gzip、lzma的cmd命令行，lz4、lzo、bz2需要自行实现，可参考unzip的具体实现。

参考patch:

﻿[ cmd: add lz4dec lzodec bz2 dec cmd support (dbf6d382) · Commits · C4902 / BSP · GitLab ](http://192.168.2.88/c4902/bsp/commit/dbf6d382dd5a2a7837562c5c234918ef570512cb)﻿

﻿[ ss927v100_hl_demb_emmc_defconfig: add lz4dec lzodec cmd support (e168d429) · Commits · C4902 / BSP · GitLab ](http://192.168.2.88/c4902/bsp/commit/e168d429d6cdbcbacff1ecc7c432a1ac30db865d)﻿

### gzip命令使用方法

Usage:

```
unzip srcaddr dstaddr [dstsize]
```

例

```
unzip 0x51000000 0x50000000
```

一般情况下不需要指定dst长度，api可以自行识别。

### lzma命令使用方法

Usage:

```
lzmadec srcaddr dstaddr [dstsize]
```

例

```
lzmadec 0x51000000 0x50000000
```

一般情况下不需要指定dst长度，api可以自行识别。

### lz4命令使用方法(新版uboot已实现)

Usage:

```
lz4dec srcaddr dstaddr srcsize [dstsize]
```

例

```
lz4dec 0x51000000 0x50000000 0x61ec00
```

**注：uboot新版命令为unlz4。**

### lzo命令使用方法

Usage:

```
lzodec srcaddr dstaddr srcsize [dstsize]
```

例

```
lzodec 0x51000000 0x50000000 0x61ec00
```

### bz2命令使用方法（不推荐）

使用bz2需要修改uboot配置CONFIG_SYS_MALLOC_LEN大于4M（0x400000），默认大小为(0x60000)384K，否则接口会出现报错(错误码-3)，而且在uboot上，该接口相对其他接口的解压速度明显较慢，故不推荐使用。

Usage:

```
bz2dec srcaddr dstaddr srcsize [dstsize]
```

例

```
bz2dec 0x51000000 0x50000000 0x61ec00
```

lz4、lzo、bz2一般情况下也不需要指定dst长度，api可以自行识别，但与gzip和lzma的区别在于需要指定src的长度，这个长度必须大于等于原始文件，单位byte，例如，原压缩文件为0x61ec00 byte，这个srcsize只要大于0x61ec00即可。在实际项目中，可以直接将srcsize的值指定为烧录的分区的大小。

### 格式压缩率对比

都是-9最高压缩比配置

|              |        |        |        |        |        |
| ------------ | ------ | ------ | ------ | ------ | ------ |
| 原始镜像大小 | gzip   | lzma   | lz4    | lzo    | bz2    |
| 11167KB      | 5388KB | 4048KB | 6260KB | 5893KB | 4969KB |

### 解压耗时对比

|          |       |       |      |      |        |
| -------- | ----- | ----- | ---- | ---- | ------ |
| 格式     | gzip  | lzma  | lz4  | lzo  | bz2    |
| 耗时(ms) | 141ms | 922ms | 39ms | 87ms | 2795ms |