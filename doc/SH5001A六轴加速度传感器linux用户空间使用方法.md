# SH5001A六轴加速度传感器linux用户空间使用方法

## 版本

| 版本  | 修订日期   | 内容                                  |
| ----- | ---------- | :------------------------------------ |
| 1.0   | 2024.11.22 | 初始版本                              |
| 1.1   | 2024.11.28 | 添加缓冲区读取数据方法                |
| 1.1.1 | 2024.12.23 | 更新trigge命名                        |
| 1.2   | 2025.7.28  | 1. 新增fifo触发器 2. 新增配置示例脚本 |

## 概要

​		SH5001A六轴传感器目前对接至linux iio子系统，在用户空间提供统一的访问接口，用户无需关心具体的驱动实现。

​		当前SH5001A驱动提供了x、y、z轴加速度角速度原始值、处理值、查询、设置功能，以及量程、采样率，温度设置、查询功能。

## 接口说明

​		linux iio子系统通过过sysfs提供查询节点。

位置：

```sh
/sys/bus/iio/devices/iio\:deviceX/
```

X:为数字，按照注册顺序从0递增，一下说明都按0展示。

## 加速度传感器

### x轴加速度元数据

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_x_raw
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_x_raw
```

### y轴加速度元数据

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_y_raw
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_y_raw
```

### z轴加速度元数据

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_z_raw
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_z_raw
```

### x轴加速度处理值

与元数据的区别为，该值为元数据经过处理转换后得到的具体加速度的值，单位mg

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_x_input
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_x_input
```

### y轴加速度处理值

与元数据的区别为，该值为元数据经过处理转换后得到的具体加速度的值，单位mg

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_y_input
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_y_input
```

### z轴加速度处理值

与元数据的区别为，该值为元数据经过处理转换后得到的具体加速度的值，单位mg

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_z_input
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_z_input
```

### 量程范围查询

用于查询可用的量程范围查询，分别+-2g，+-4g，+-8g，+-16g，+-32g。

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_scale_available
```

查询：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_scale_available
2 4 8 16 32
```

### 设置加速度量程范围

用于设置量程范围，分别+-2g，+-4g，+-8g，+-16g，+-32g。

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_scale
```

查询当前量程：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_scale
2
```

设置当前量程：

```sh
echo 2 > /sys/bus/iio/devices/iio\:device0/in_accel_scale
```

### 加速度采样率查询

用于查询当前可用采样率，当前可用值为125、250、500、1000、2000、4000、8000。

节点：

```sh
/sys/bus/iio/devices/iio\:device0/sampling_frequency_available
```

用于查询可用采样率

```sh
cat /sys/bus/iio/devices/iio\:device0/sampling_frequency_available
125 250 500 1000 2000 4000 8000
```

### 加速度采样率设置

用于设置当前采样率

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_sampling_frequency
```

查询当前采样率

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_sampling_frequency
```

设置采样率

```sh
echo 500 > /sys/bus/iio/devices/iio\:device0/in_accel_sampling_frequency
```

### 截止频率设置系数查询

用于查询截止频率系数

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_filter_low_pass_3db_frequency_available
```

查询可用参数：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_filter_low_pass_3db_frequency_available
0.40 0.36 0.32 0.28 0.24 0.20 0.16 0.14 0.12 0.10 0.08 0.06 0.04 0.03 0.02 0.01
```

### 截止频率设置

用于设置截止频率，该设置与采样率相关，截止频率 = 采样率 * 截止频率系数，所以该节点为截止频率系数的设置

节点：

```说
/sys/bus/iio/devices/iio\:device0/in_accel_filter_low_pass_3db_frequency
```

查询当前截止系数：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_filter_low_pass_3db_frequency
0.40
```

设置截止系数：

```sh
echo 0.40 > /sys/bus/iio/devices/iio\:device0/in_accel_filter_low_pass_3db_frequency
```

## 角速度传感器

### x轴角速度元数据

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_anglvel_x_raw
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_anglvel_x_raw
```

### y轴角速度元数据

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_anglvel_y_raw
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_anglvel_y_raw
```

### z轴角速度元数据

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_anglvel_z_raw
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_anglvel_z_raw
```

### x轴角速度处理值

与元数据的区别为，该值为元数据经过处理转换后得到的具体加速度的值，单位dps

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_anglvel_x_input
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_anglvel_x_input
```

### y轴角速度处理值

与元数据的区别为，该值为元数据经过处理转换后得到的具体加速度的值，单位dps

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_anglvel_y_input
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_anglvel_y_input
```

### z轴角速度处理值

与元数据的区别为，该值为元数据经过处理转换后得到的具体加速度的值，单位dps

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_anglvel_z_input
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_anglvel_z_input
```

### 量程范围查询

用于查询可用的量程范围查询，31、62、125、250、500 、1000、2000 、4000，单位dps

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_anglvel_scale_available
```

查询：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_anglvel_scale_available
31 62 125 250 500 1000 2000 4000
```

### 设置量程范围

用于设置量程范围，分别31、62、125、250、500 、1000、2000 、4000，单位dps。

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_anglvel_scale
```

查询当前量程：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_anglvel_scale
125
```

设置当前量程：

```sh
echo 125 > /sys/bus/iio/devices/iio\:device0/in_anglvel_scale
```

### 采样率查询

用于查询当前可用采样率，当前可用值为125、250、500、1000、2000、4000、8000、16000。

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_anglvel_sampling_frequency_available
```

用于查询可用采样率

```sh
cat /sys/bus/iio/devices/iio\:device0/in_anglvel_sampling_frequency_available
125 250 500 1000 2000 4000 8000 16000
```

### 采样率设置

用于设置当前采样率

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_anglvel_sampling_frequency
```

查询当前采样率

```sh
cat /sys/bus/iio/devices/iio\:device0/in_anglvel_sampling_frequency
```

设置采样率

```sh
echo 500 > /sys/bus/iio/devices/iio\:device0/in_anglvel_sampling_frequency
```

### 截止频率设置系数查询

用于查询截止频率系数

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_anglvel_filter_low_pass_3db_frequency_available
```

查询可用参数：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_anglvel_filter_low_pass_3db_frequency_available
0.40 0.36 0.32 0.28 0.24 0.20 0.16 0.14 0.12 0.10 0.08 0.06 0.04 0.03 0.02 0.01
```

### 截止频率设置

用于设置截止频率，该设置与采样率相关，截止频率 = 采样率 * 截止频率系数，所以该节点为截止频率系数的设置

节点：

```说
/sys/bus/iio/devices/iio\:device0/in_anglvel_filter_low_pass_3db_frequency
```

查询当前截止系数：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_anglvel_filter_low_pass_3db_frequency
```

设置截止系数：

```sh
echo 0.40 > /sys/bus/iio/devices/iio\:device0/in_anglvel_filter_low_pass_3db_frequency
```

## 温度传感器

### 温度原始值

用于查询温度传感器原始值，

**换算公式Temperature = (TEMP DATA - ROOM TEMP)/14 + 25**

**ROOM TEMP=2232**

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_temp_raw
```

查询温度原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_temp_raw
```

### 温度处理值

用于查询温度传感器处理值，单位℃

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_temp_input
```

查询温度:

```sh
cat /sys/bus/iio/devices/iio\:device0/in_temp_input
```

### 温度采样率查询

用于查询温度采样率可用值，63、125、250、500。

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_temp_sampling_frequency_available
63 125 250 500
```

查询可用参数：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_temp_sampling_frequency_available
```

### 温度采样率

用于设置温度采样率

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_temp_sampling_frequency
```

查询当前采样率：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_temp_sampling_frequency
```

设置采样率：

```sh
echo 63 > /sys/bus/iio/devices/iio\:device0/in_temp_sampling_frequency
```

## kernel相关配置

​		sh5001a同时支持SPI和I2C通信方式，所以代码也做了兼容.

### SPI配置

```sh
CONFIG_SH5001A=y
CONFIG_SH5001A_SPI=y
```

### SPI DTS配置

```sh
&spi_bus1{
        status = "okay";
        sh5001a@0 {
                compatible = "sh5001a";
                status = "okay";
                reg = <0>;
                pl022,interface = <0>;
                pl022,com-mode = <0>;
                spi-max-frequency = <10000000>;
                sh5001a,irq1-gpio = <&gpio_chip1 1 1>;
        };
};
```

### I2C配置

```sh
CONFIG_SH5001A=y
CONFIG_SH5001A_I2C=y
```

### I2C DTS配置

```sh
&i2c_bus2{
        status = "okay";
        sh5001a@37 {
                compatible = "sh5001a";
                status = "okay";
                reg = <0x37>;
                sh5001a,irq1-gpio = <&gpio_chip1 1 1>;
                clock-frequency = <400000>;
        };
};
```

# 通过IIO触发缓冲区读取数据方法

## 简介

​		触发缓冲区就是基于某种信号来触发数据采集，并将数据填入缓冲区，用户可在缓冲区快速去读取数据。

​		这种信号通常为传感器数据就绪**硬件中断**、**周期性中断(高精度计时器)**

​		数据采集到以后会填充到缓冲区里面，最终会以字符设备提供给用户空间，用户空间直接读取缓冲文件即可。

## 触发器区别

​		dready触发器一般命名为sh5001a-devx-dready，dready的中断触发频率等于采样率，所以实时性比较好，但资源暂用也较大；适合作实时姿态检测。fifo触发器一般命名为sh5001a-devx-fifo，中断触发频率由fifo的watermark决定(默认为140bytes，也就是产生10组数据，才会触发一次中断)，所以资源占用较小，且数据在时间戳是连续的，但实时性较差。

## 触发器配置

​		sh5001a驱动会注册两个个硬件中断触发器，一个为dready触发器，一个为fifo触发器，使用触发器需要进行一些必要配置。以下以deady触发器进行配置介绍。

路径地址：

```sh
ls /sys/bus/iio/devices/trigger0/
name       power      subsystem  uevent
```

获取触发器名字：

```sh
cat /sys/bus/iio/devices/trigger0/name
sh5001a-dev0-dready
```

绑定触发器至sh5001a驱动：

```sh
echo "sh5001a-dev0-dready" > /sys/bus/iio/devices/iio\:device0/trigger/current_trigger
```

解绑触发器：

```sh
echo "" > /sys/bus/iio/devices/iio\:device0/trigger/current_trigger
```

查看当前绑定的触发器：

```
cat /sys/bus/iio/devices/iio\:device0/trigger/current_trigger
sh5001a-dev0
```

### 配置扫描元素

扫描元素路径：

```sh
ls /sys/bus/iio/devices/iio\:device0/scan_elements/
in_accel_x_en       in_accel_z_type     in_anglvel_z_index
in_accel_x_index    in_anglvel_x_en     in_anglvel_z_type
in_accel_x_type     in_anglvel_x_index  in_temp_en
in_accel_y_en       in_anglvel_x_type   in_temp_index
in_accel_y_index    in_anglvel_y_en     in_temp_type
in_accel_y_type     in_anglvel_y_index  in_timestamp_en
in_accel_z_en       in_anglvel_y_type   in_timestamp_index
in_accel_z_index    in_anglvel_z_en     in_timestamp_type
```

开启加速度x通道的采集

```sh
echo 1 > /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_x_en
```

开启加速度y通道的采集

```sh
echo 1 > /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_y_en
```

开启加速度z通道的采集

```sh
echo 1 > /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_z_en
```

开启角速度x通道的采集

```sh
echo 1 > /sys/bus/iio/devices/iio\:device0/scan_elements/in_anglvel_x_en
```

开启角速度y通道的采集

```sh
echo 1 > /sys/bus/iio/devices/iio\:device0/scan_elements/in_anglvel_y_en
```

开启角速度z通道的采集

```sh
echo 1 > /sys/bus/iio/devices/iio\:device0/scan_elements/in_anglvel_z_en
```

开启温度数据通道的采集

```sh
echo 1 > /sys/bus/iio/devices/iio\:device0/scan_elements/in_temp_en
```

开启时间戳

```sh
echo 1 > /sys/bus/iio/devices/iio\:device0/scan_elements/in_timestamp_en
```

注：写0，则为关闭该通道的扫描

### 通道信息查询

查询加速度x数据类型：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_x_type
le:s16/16>>0
```

查询加速度y数据类型：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_y_type
le:s16/16>>0
```

查询加速度z数据类型：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_y_type
le:s16/16>>0
```

查询角速度x数据类型：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_anglvel_x_type
le:s16/16>>0
```

查询角速度y数据类型：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_anglvel_y_type
le:s16/16>>0
```

查询角速度z数据类型：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_anglvel_y_type
le:s16/16>>0
```

查询温度数据类型：

```
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_temp_type
le:u12/16>>0
```

释义：小段、有效数16位，储存位数16位，右移0位。

查询加速度x数据通道号：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_x_index
0
```

查询加速度y数据通道号：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_y_index
1
```

查询加速度z数据通道号：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_z_index
2
```

查询角速度x数据通道号：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_anglvel_x_index
3
```

查询角速度y数据通道号：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_anglvel_y_index
4
```

查询角速度z数据通道号：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_anglvel_z_index
5
```

温度通道号：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_temp_in_temp_index
6
```

时间戳通道号：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_timestamp_index
7
```

## 配置缓冲区

配置路径：

```sh
ls /sys/bus/iio/devices/iio\:device0/buffer/
data_available  enable          length          watermark
```

查看当前可读数据长度

```sh
cat /sys/bus/iio/devices/iio\:device0/buffer/data_available
8
```

配置缓冲区buffer池长度:

```sh
echo 8 > /sys/bus/iio/devices/iio\:device0/buffer/length
```

查看当前缓冲区buffer长度：

```sh
cat /sys/bus/iio/devices/iio\:device0/buffer/length
8
```

配置水位：

```sh
echo 240 > /sys/bus/iio/devices/iio\:device0/buffer/watermark
```

查看水位：

```sh
cat /sys/bus/iio/devices/iio\:device0/buffer/watermark
240
```

**注：只在fifo mode时生效，配置fifo中断触发阈值。**建议配置为14的倍数

开启触发器：

```sh
echo 1 > /sys/bus/iio/devices/iio\:device0/buffer/enable
```

注：写0为关闭触发器。开启触发器后，即可通过设备"/dev/iio\:device0"读取缓冲区设备

例：

```sh
root@busybox:~$ hexdump /dev/iio\:device0
0000000 0392 07b9 0050 ffdf 0090 0026 07e7 0000
0000010 517e cb6a 0397 0000 038f 07b9 0077 ffe9
0000020 008e 0020 07e7 0000 b981 cb79 0397 0000
0000030 0393 07b7 0053 ffdf 0088 0022 07e7 0000
0000040 493a cb89 0397 0000 038f 07b2 003b ffe4
0000050 0086 0026 07e7 0000 e0c3 cb98 0397 0000
0000060 038c 07bb 0055 ffe8 0083 001d 07e7 0000
0000070 6a4d cba8 0397 0000 0392 07bd 0056 ffe2
0000080 0082 001a 07e7 0000 f9dd cbb7 0397 0000
0000090 0391 07b8 0060 ffe0 0083 001b 07e7 0000
00000a0 8aba cbc7 0397 0000 039e 07b7 0057 ffde
00000b0 007b 001a 07e7 0000 1ac7 cbd7 0397 0000
...
```

**注：若开启全部通道，一组完整的数据长度为2(acc_x)+2(acc_y)+2(acc_z)+2(gyro_x)+2(gyro_y)+2(gyro_z)+2(temp)+2(pad)+8(timestamp) = 24byte，其中时间戳会以8byte进行数据对齐。例，假设只开启acc_x的通道和时间戳，那么一段完整数据长度为2(acc_x)+6(pad)+8(timestamp)=16byte**

```sh
root@busybox:~$ hexdump /dev/iio\:device0
0000000 02e1 0000 0000 0000 d92f c36c 0517 0000
0000010 02c7 0000 0000 0000 4eb4 c37c 0517 0000
0000020 02c8 0000 0000 0000 d0c1 c38b 0517 0000
0000030 02f3 0000 0000 0000 60ce c39b 0517 0000
0000040 0314 0000 0000 0000 f181 c3aa 0517 0000
0000050 033f 0000 0000 0000 81e1 c3ba 0517 0000
0000060 0358 0000 0000 0000 1559 c3ca 0517 0000
0000070 035c 0000 0000 0000 a46b c3d9 0517 0000
```

## 参考配置脚本

```sh
#!/bin/sh

# 检查是否以root权限运行
if [ "$(id -u)" -ne 0 ]; then
    echo "请使用root权限运行此脚本!"
    exit 1
fi

# 默认配置参数
TRIGGER_NAME="sh5001a-dev1-dready"
# TRIGGER_NAME="sh5001a-dev1-fifo"  # 触发器名称
SAMPLE_RATE=1000           # 采样率(Hz)
BUFFER_SIZE=256           # 缓冲区大小
WATERMARK=140              # 水位(仅fifo模式下生效)
DEVICE_NAME="sh5001a"      # 根据实际设备修改
ENABLE_ALL_CHANNELS=false          # 默认不启用所有通道
CHANNEL_LIST="in_accel_x,in_accel_y,in_accel_z,in_anglvel_x,in_anglvel_y,in_anglvel_z,in_temp,in_timestamp"                # 默认启用的通道数组

# 显示用法信息
usage() {
    echo "用法: $0 [选项]"
    echo "选项:"
    echo "  -d <设备名>    指定IIO设备名称(默认: $DEVICE_NAME)"
    echo "  -t <触发器名>  指定触发器名称(默认: $TRIGGER_NAME)"
    echo "  -r <采样率>    设置采样率(Hz)(默认: $SAMPLE_RATE)"
    echo "  -b <缓冲区大小> 设置缓冲区大小(默认: $BUFFER_SIZE)"
    echo "  -a             启用所有可用通道"
    echo "  -c <通道列表>   指定要启用的通道(逗号分隔)"
    echo "                 例如: accel_x,accel_y,gyro_z"
    echo "  -h             显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0 -d mpu6050 -r 200 -b 512 -c in_accel_x,in_accel_y,in_accel_z,in_temp,in_timestamp"
    echo "  $0 -a -r 100"
    exit 0
}

# 解析命令行参数
while getopts "d:t:r:b:ac:h" opt; do
    case $opt in
        d) DEVICE_NAME="$OPTARG" ;;
        t) TRIGGER_NAME="$OPTARG" ;;
        r) SAMPLE_RATE="$OPTARG" ;;
        b) BUFFER_SIZE="$OPTARG" ;;
        a) ENABLE_ALL_CHANNELS=true ;;
        c) ENABLED_CHANNELS="$OPTARG" ;;
        h) usage ;;
        *) echo "无效选项: -$OPTARG" >&2; usage ;;
    esac
done

# 查找IIO设备
find_iio_device() {
    for dir in /sys/bus/iio/devices/iio:device*; do
        if grep -q "$DEVICE_NAME" "$dir/name" 2>/dev/null; then
            echo $(basename "$dir")
            return 0
        fi
    done
    echo ""
    return 1
}

find_trigger() {
    for dir in /sys/bus/iio/devices/trigger*; do
        if grep -q "$TRIGGER_NAME" "$dir/name" 2>/dev/null; then
            echo $(basename "$dir")
            return 0
        fi
    done
    echo ""
    return 1
}

# 启用指定通道
enable_channels() {
    device=$1
    
    if [ "$ENABLE_ALL_CHANNELS" = true ]; then
        echo "启用所有可用通道..."
        for chan in /sys/bus/iio/devices/$device/scan_elements/*_en; do
            echo 1 > "$chan" 2>/dev/null && echo "已启用: $(basename "$chan" "_en")"
        done
    elif [ -n "$CHANNEL_LIST" ]; then
        echo "启用指定通道: $CHANNEL_LIST"
        old_IFS="$IFS"
        IFS=','
        set -- $CHANNEL_LIST
        IFS="$old_IFS"
        
        for chan do
            chan_en="/sys/bus/iio/devices/$device/scan_elements/${chan}_en"
            if [ -f "$chan_en" ]; then
                echo 1 > "$chan_en" && echo "已启用: $chan"
            else
                echo "警告: 通道 $chan 不存在或不可用!"
            fi
        done
    else
        echo "警告: 未指定要启用的通道，将只配置触发器不启用任何通道!"
    fi
}

# 显示当前通道状态
show_channel_status() {
    local device=$1
    echo "当前通道状态:"
    for chan in /sys/bus/iio/devices/$device/scan_elements/*_en; do
        if [ -f "$chan" ]; then
            local chan_name=$(basename "$chan" "_en")
            local status=$(cat "$chan")
            echo "  $chan_name: $([ "$status" -eq 1 ] && echo "启用" || echo "禁用")"
        fi
    done
}

# 主函数
main() {
    echo "正在配置六轴传感器IIO触发器..."
    
    # 查找设备
    DEVICE=$(find_iio_device)
    if [ -z "$DEVICE" ]; then
        echo "错误: 未找到 $DEVICE_NAME 设备!"
        echo "可用的IIO设备:"
        ls /sys/bus/iio/devices/iio:device*/name | xargs -I{} sh -c 'echo "  $(basename $(dirname {})): $(cat {})"'
        exit 1
    fi
    echo "找到设备: $DEVICE"
    
    # 查找设备
    TRIGGER=$(find_trigger)
    if [ -z "$TRIGGER" ]; then
        echo "未找到 $TRIGGER_NAME 设备!"
        exit 1
    fi
    echo "找到触发器: $TRIGGER"
    
    # 将设备与触发器关联
    echo "将设备 $DEVICE 绑定到触发器..."
    echo "$TRIGGER_NAME" > /sys/bus/iio/devices/$DEVICE/trigger/current_trigger
    if [ $? -ne 0 ]; then
        echo "设备绑定触发器失败!"
        exit 1
    fi
    
    # 启用指定通道
    enable_channels "$DEVICE"
    
    # 配置缓冲区
    echo "配置缓冲区大小为 $BUFFER_SIZE..."
    echo "$BUFFER_SIZE" > "/sys/bus/iio/devices/$DEVICE/buffer/length"
    if [ $? -ne 0 ]; then
        echo "错误: 缓冲区配置失败!"
        exit 1
    fi

    # 配置watermark
    echo "配置水位为 $WATERMARK..."
    echo "$WATERMARK" > "/sys/bus/iio/devices/$DEVICE/buffer/watermark"
        if [ $? -ne 0 ]; then
        echo "错误: 缓冲区配置失败!"
        exit 1
    fi

    # 启用缓冲区
    echo "启用缓冲区..."
    echo 1 > "/sys/bus/iio/devices/$DEVICE/buffer/enable"
    if [ $? -ne 0 ]; then
        echo "错误: 缓冲区启用失败!"
        exit 1
    fi
    
    # 显示配置摘要
    echo ""
    echo "===== 配置摘要 ====="
    echo "设备: $DEVICE"
    echo "触发器: $TRIGGER"
    echo "accel采样率: $(cat /sys/bus/iio/devices/$DEVICE/in_accel_sampling_frequency) Hz"
    echo "anglvel采样率: $(cat /sys/bus/iio/devices/$DEVICE/in_anglvel_sampling_frequency) Hz"
    echo "缓冲区大小: $(cat "/sys/bus/iio/devices/$DEVICE/buffer/length")"
    show_channel_status "$DEVICE"
    echo ""
    echo "配置完成!"
}

main
```

## 应用参考c代码

```c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <poll.h>
#include <linux/iio/types.h>

#define NUM_AXES 6  // 3轴加速度计 + 3轴陀螺仪
#define SAMPLE_RATE 1000  // 采样率(Hz)
#define BUFFER_SIZE 1000  // 缓冲区大小
#define IIO_DIR "/sys/bus/iio/devices/"
#define DEVICE_PREFIX "iio:device"
#define MAX_DEVICES 10
#define DEVICE_NAME "sh5001a"

// 若全部通道开启，一组组数为24bytes
struct imu_data {
    int16_t accel_data[3];
    int16_t gyro_data[3];
    int16_t temp;
    int64_t timestamp;
};

volatile sig_atomic_t stop = 0;

void handle_signal(int sig) {
    stop = 1;
}

// 查找IIO设备
static int find_iio_device(const char *device_name) {
    DIR *dir;
    struct dirent *ent;
    int device_num = -1;
    char *name;
    FILE *namefp;
    char filename[PATH_MAX];

    dir = opendir(IIO_DIR);
    if (!dir) {
        perror("无法打开IIO目录");
        return -1;
    }

    while ((ent = readdir(dir)) != NULL) {
        if (strstr(ent->d_name, DEVICE_PREFIX) != NULL) {
            int tmp_num;
            if (sscanf(ent->d_name, "iio:device%d", &tmp_num) != 1)
                continue;

            snprintf(filename, sizeof(filename), 
                     IIO_DIR "%s/name", ent->d_name);
            namefp = fopen(filename, "r");
            if (!namefp)
                continue;

            if (fscanf(namefp, "%ms", &name) != 1) {
                fclose(namefp);
                continue;
            }
            fclose(namefp);

            if (strstr(name, device_name) != NULL) {
                device_num = tmp_num;
                free(name);
                break;
            }
            free(name);
        }
    }
    closedir(dir);

    return device_num;
}

// 打开IIO设备缓冲区
static int open_iio_buffer(int device_num) {
    char dev_name[PATH_MAX];
    int fd;

    snprintf(dev_name, sizeof(dev_name),
             "/dev/iio:device%d", device_num);

    fd = open(dev_name, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("无法打开IIO设备");
        return -1;
    }

    return fd;
}

int main(int argc, char **argv) {
    int device_num;
    int iio_fd;
    uint32_t buf_size;
    char raw_buffer[BUFFER_SIZE];
    struct pollfd pfd;
    
    // 设置信号处理
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    //查找指定设备
    device_num = find_iio_device(DEVICE_NAME);
    if (device_num < 0) {
        fprintf(stderr, "未找到六轴传感器设备 %s\n", DEVICE_NAME);
        return EXIT_FAILURE;
    }
    
    printf("找到传感器设备: iio:device%d\n", device_num);
    
    // 打开设备缓冲区
    iio_fd = open_iio_buffer(device_num);
    if (iio_fd < 0) {
        return EXIT_FAILURE;
    }
    
    printf("开始采集数据 (按Ctrl+C停止)...\n");
    printf("Accel_X, Accel_Y, Accel_Z, Gyro_X, Gyro_Y, Gyro_Z， temp, timestamp\n");
    
    // 配置poll
    pfd.fd = iio_fd;
    pfd.events = POLLIN;
    
    // 主循环
    while (!stop) {
        struct imu_data imu_data_t = {0};
        char *p_buf = NULL;
        ssize_t bytes_read;
        
        // 等待数据可用
        int ret = poll(&pfd, 1, 1000);
        if (ret < 0) {
            perror("poll失败");
            break;
        } else if (ret == 0) {
            continue; // 超时
        }
        // 读取缓冲区数据
        bytes_read = read(iio_fd, raw_buffer, sizeof(raw_buffer));
        //printf("bytes_read = %d\n", bytes_read);
        if (bytes_read < 0) {
            if (errno == EAGAIN) {
                break; // 无数据可用
            }
            perror("读取失败");
            break;
        }
        buf_size = sizeof(imu_data_t);
        p_buf = raw_buffer;
        while (bytes_read) {
            //整块拷贝，推荐使用
            if (bytes_read % buf_size != 0) {
                   fprintf(stderr, "缓冲区数据不完整\n");
                continue;
            }
            memcpy(&imu_data_t, p_buf, buf_size);
            p_buf += buf_size;
            bytes_read -= buf_size;
            printf("%6d, %6d, %6d, %6d, %6d, %6d, %6d, %8lld\n",
                imu_data_t.accel_data[0], imu_data_t.accel_data[1], imu_data_t.accel_data[2],
                imu_data_t.gyro_data[0], imu_data_t.gyro_data[1], imu_data_t.gyro_data[2],
                imu_data_t.temp & 0xfff, imu_data_t.timestamp);
        }
    }
    
    // 清理
    close(iio_fd);
    
    printf("\n采集结束\n");
    return EXIT_SUCCESS;
}
```

