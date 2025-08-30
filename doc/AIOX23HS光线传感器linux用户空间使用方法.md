# AIOX23HS Linux驱动用户空间接口使用方法

## 版本

| 版本 | 修订日期 | 内容     |
| ---- | -------- | -------- |
| 1.0  | 2025.7.7 | 初始版本 |
|      |          |          |
|      |          |          |

## 概要

​		AIOX23HS亮度传感器目前对接至linux iio子系统，在用户空间提供统一的访问接口，用户无需关心具体的驱动实现。

​		当前AIOX23HS驱动提供了色温(color temp)、照度(lux)、红外(ir)、红光(red)、绿光(green)、蓝光(blue)强度查询。

## 接口说明

​		linux iio子系统通过过sysfs提供查询节点。

位置：

```sh
/sys/bus/iio/devices/iio\:deviceX/
```

X:为数字，按照注册顺序从0递增，一下说明都按0展示。

## 原始值

​		该数值为从寄存器直接读回的32位浮点值，在应用空间需要转换成浮点数。

例：

```c
int u_value = 1170835724; //该值为读到的32位int数据。
float f_value = (float)u_value;
```

### 色温(color temp)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_cct_raw
```

查询色温：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_cct_raw
```

### 照度(lux)

节点：

```sh
/sys/bus/iio/devices/iio\:device1/in_illuminance_raw
```

查询色温：

```sh
cat /sys/bus/iio/devices/iio\:device1/in_illuminance_raw
```

### 红外原始值(ir)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_intensity_ir_raw
```

查询红外强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_intensity_ir_raw
```

### 红光原始值(red)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_intensity_red_raw
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_intensity_red_raw
```

### 绿光原始值(green)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_intensity_green_raw
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_intensity_green_raw
```

### 蓝光原始值(blue)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_intensity_blue_raw
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_intensity_blue_raw
```

## 处理值

​		该数值位经过处理的值，可以直接使用，但是丢失了小数位，精度相对于原始值略差。

### 色温(color temp)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_cct_input
```

查询色温：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_cct_input
```

### 照度(lux)

节点：

```sh
/sys/bus/iio/devices/iio\:device1/in_illuminance_input
```

查询色温：

```sh
cat /sys/bus/iio/devices/iio\:device1/in_illuminance_input
```

### 红外原始值(ir)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_intensity_ir_input
```

查询红外强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_intensity_ir_input
```

### 红光原始值(red)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_intensity_red_input
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_intensity_red_input
```

### 绿光原始值(green)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_intensity_green_input
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_intensity_green_input
```

### 蓝光原始值(blue)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_intensity_blue_input
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_intensity_blue_input
```

## 调试节点

### reg

用于读取寄存器

```sh
/sys/bus/iio/devices/iio\:device0/reg
```

查询寄存器值

```sh
cat /sys/bus/iio/devices/iio\:device0/reg
```

## DTS配置

例

```c
&i2c_bus0 {
        aiox23hs@39 {
                compatible = "aiox23hs";
                status = "okay";
                reg = <0x39>;
                clock-frequency = <100000>;
        };
};
```

## 内核编译配置

```c
CONFIG_AIOX23HS=y
```
