# STK8329三轴加速度传感器linux用户空间使用方法

## 版本

| 版本 | 修订日期   | 内容                       |
| ---- | ---------- | -------------------------- |
| 1.0  | 2024.10.23 | 初始版本                   |
| 1.1  | 2024.11.13 | 增加通过缓冲区去读数据方法 |
| 1.2  | 2024.11.22 | 修改量程配置方式           |

## 概要

​		STK8329亮度传感器目前对接至linux iio子系统，在用户空间提供统一的访问接口，用户无需关心具体的驱动实现。

​		当前STK8329驱动提供了x、y、z轴加速度原始值、处理值、偏移量的直接查询、设置功能，以及量程、采样率设置查询功能。

## 接口说明

​		linux iio子系统通过过sysfs提供查询节点。

位置：

```sh
/sys/bus/iio/devices/iio\:deviceX/
```

X:为数字，按照注册顺序从0递增，一下说明都按0展示。

## x轴加速度元数据

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_x_raw
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_x_raw
```

## y轴加速度元数据

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_y_raw
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_y_raw
```

## z轴加速度元数据

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_z_raw
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_z_raw
```

## x轴加速度处理值

与元数据的区别为，该值为元数据经过处理转换后得到的具体加速度的值，单位mg

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_x_input
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_x_input
```

## y轴加速度处理值

与元数据的区别为，该值为元数据经过处理转换后得到的具体加速度的值，单位mg

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_y_input
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_y_input
```

## z轴加速度处理值

与元数据的区别为，该值为元数据经过处理转换后得到的具体加速度的值，单位mg

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_z_input
```

查询x轴原始值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_z_input
```

## x轴加偏移量设置

用于设置x轴加速的偏移量，修正测量值，单位mg，步进为7.81mg。设置后会直接作用到x轴加速度的测量值

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_x_offset
```

查询x轴偏移值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_x_offset
0.000000
```

设置偏移值

```sh
echo -42.883000 > /sys/bus/iio/devices/iio\:device0/in_accel_x_offset
cat /sys/bus/iio/devices/iio\:device0/in_accel_x_offset
-39.050000
```

由于为7.81mg为一个步进，所以设置会向下取整。

## y轴加偏移量设置

用于设置y轴加速的偏移量，修正测量值，单位mg，步进为7.81mg。设置后会直接作用到y轴加速度的测量值

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_y_offset
```

查询x轴偏移值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_y_offset
0.000000
```

设置偏移值

```sh
echo -42.883000 > /sys/bus/iio/devices/iio\:device0/in_accel_y_offset
cat /sys/bus/iio/devices/iio\:device0/in_accel_y_offset
-39.050000
```

由于为7.81mg为一个步进，所以设置会向下取整。

## z轴加偏移量设置

用于设置z轴加速的偏移量，修正测量值，单位mg，步进为7.81mg。设置后会直接作用到z轴加速度的测量值

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_z_offset
```

查询x轴偏移值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_z_offset
0.000000
```

设置偏移值

```sh
echo -42.883000 > /sys/bus/iio/devices/iio\:device0/in_accel_z_offset
cat /sys/bus/iio/devices/iio\:device0/in_accel_z_offset
-39.050000
```

由于为7.81mg为一个步进，所以设置会向下取整。

## 量程范围查询

用于查询可用的量程范围查询，分别+-2g，+-4g，+-8g，+-16g。

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_accel_scale_available
```

查询：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_accel_scale_available
2 4 8 16
```

## 设置量程范围

用于设置量程范围，分别+-2g，+-4g，+-8g，+-16g。

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

## 采样率查询

用于查询当前可用采样率，当前可用值为7.81、15.63、31.25、62.5、125、250、500、1000。

节点：

```sh
/sys/bus/iio/devices/iio\:device0/sampling_frequency_available
```

用于查询可用采样率

```sh
cat /sys/bus/iio/devices/iio\:device0/sampling_frequency_available
7.81 15.63 31.25 62.5 125 250 500 1000
```

## 采样率设置

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

# 通过IIO触发缓冲区读取数据方法

## 简介

​		触发缓冲区就是基于某种信号来触发数据采集，并将数据填入缓冲区，用户可在缓冲区快速去读取数据。

​		这种信号通常为传感器数据就绪**硬件中断**、**周期性中断(高精度计时器)**

​		数据采集到以后会填充到缓冲区里面，最终会以字符设备提供给用户空间，用户空间直接读取缓冲文件即可。

## 触发器配置

​		stk8329驱动会注册一个硬件中断触发器，使用触发器需要进行一些必要配置。

路径地址：

```sh
ls /sys/bus/iio/devices/trigger0/
name       power      subsystem  uevent
```

获取触发器名字：

```sh
cat /sys/bus/iio/devices/trigger0/name
stk8329-dev0
```

绑定触发器至stk8329驱动：

```sh
echo "stk8329-dev0" > /sys/bus/iio/devices/iio\:device0/trigger/current_trigger
```

解绑触发器：

```sh
echo "" > /sys/bus/iio/devices/iio\:device0/trigger/current_trigger
```

查看当前绑定的触发器：

```
cat /sys/bus/iio/devices/iio\:device0/trigger/current_trigger
stk8329-dev0
```

### 配置扫描元素

扫描元素路径：

```sh
ls /sys/bus/iio/devices/iio\:device0/scan_elements/
in_accel_x_en       in_accel_y_index    in_accel_z_type
in_accel_x_index    in_accel_y_type     in_timestamp_en
in_accel_x_type     in_accel_z_en       in_timestamp_index
in_accel_y_en       in_accel_z_index    in_timestamp_type
```

开启x通道的采集

```sh
echo 1 > /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_x_en
```

开启y通道的采集

```sh
echo 1 > /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_y_en
```

开启z通道的采集

```sh
echo 1 > /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_z_en
```

注：写0，则为关闭该通道的扫描

### 通道信息查询

查询x数据类型：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_x_type
le:s16/16>>0
```

查询y数据类型：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_y_type
le:s16/16>>0
```

查询z数据类型：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_y_type
le:s16/16>>0
```

释义：小段、有效数16位，储存位数16位，右移0位。

查询x数据通道号：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_x_index
0
```

查询y数据通道号：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_y_index
1
```

查询z数据通道号：

```sh
cat /sys/bus/iio/devices/iio\:device0/scan_elements/in_accel_z_index
2
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
6
```

配置缓冲区buffer池长度:

```sh
echo 6 > /sys/bus/iio/devices/iio\:device0/buffer/length
```

查看当前缓冲区buffer长度：

```sh
cat /sys/bus/iio/devices/iio\:device0/buffer/length
6
```

配置水位：

```sh
echo 8 > /sys/bus/iio/devices/iio\:device0/buffer/watermark
```

查看水位：

```sh
cat /sys/bus/iio/devices/iio\:device0/buffer/watermark
8
```

注：一般不需要位置水位。

开启触发器：

```sh
echo 1 > /sys/bus/iio/devices/iio\:device0/buffer/enable
```

注：写0为关闭触发器。开启触发器后，即可通过设备"/dev/iio\:device0"读取缓冲区设备

例：

```
hexdump /dev/iio\:device0
0000000 0010 fa31 be17 ffcd fa4e be17 ffd7 f9d2
0000010 bcd9 ff85 f97d be3d ff53 f9d2 bcae ff4e
0000020 f9c3 bcf7 fea9 f9cd bd3b ff84 f9cd bdd6
0000030 ffa2 fa0b bd4f ffbe f9b3 bd6f ff38 fa77
...
```

