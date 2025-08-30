# LTR381 Linux驱动用户空间接口使用方法

## 版本

| 版本 | 修订日期   | 内容                       |
| ---- | ---------- | -------------------------- |
| 1.0  | 2024.10.17 | 初始版本                   |
| 1.1  | 2025.10.25 | 增加白平衡校准功能         |
| 1.2  | 2025.10.26 | intensity替换为illuminance |

## 概要

​		LTR381亮度传感器目前对接至linux iio子系统，在用户空间提供统一的访问接口，用户无需关心具体的驱动实现。

​		当前LTR381驱动提供了色温(color temp)、红外(ir)、红光(red)、绿光(green)、蓝光(blue)强度查询，白平衡校正，硬件增益调节，采样速录调节，分辨率调节功能。

## 接口说明

​		linux iio子系统通过过sysfs提供查询节点。

位置：

```sh
/sys/bus/iio/devices/iio\:deviceX/
```

X:为数字，按照注册顺序从0递增，一下说明都按0展示。

## 色温(color temp)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_temp_raw
```

查询色温：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_temp_raw
```

## 原始值

### 红外原始值(ir)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_ir_raw
```

查询红外强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_ir_raw
```

### 红光原始值(red)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_red_raw
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_red_raw
```

### 绿光原始值(green)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_green_raw
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_green_raw
```

### 蓝光原始值(blue)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_blue_raw
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_blue_raw
```

## 校正值

为原始值x校正系数

### 红外校正值(ir)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_ir_input
```

查询红外强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_ir_input
```

### 红光校正值(red)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_red_input
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_red_input
```

### 绿光校正值(green)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_green_input
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_green_input
```

### 蓝光校正值(blue)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_blue_input
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_blue_input
```

## 校正系数

用于设置校正系数，默认为1.000000，可通过dts配置默认值

### 红外校正系数(ir)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_ir_calibscale
```

查询红外强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_ir_calibscale
```

### 红光校正系数(red)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_red_calibscale
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_red_calibscale
```

### 绿光校正系数(green)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_green_calibscale
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_green_calibscale
```

### 蓝光校正系数(blue)

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_blue_calibscale
```

查询红光强度：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_blue_calibscale
```

## 白平衡校正

节点：

```sh
/sys/bus/iio/devices/iio\:device0/calibscale
```

执行一次白平衡校正

```sh
echo 1 > /sys/bus/iio/devices/iio\:device0/calibscale
```

**注：白平衡校正需要在标准白色光源下进行**

## 硬件增益调节

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_hardwaregain
```

查询当前增益值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_hardwaregain
3
```

设置增益值：

```sh
echo 3 > cat /sys/bus/iio/devices/iio\:device0/in_illuminance_hardwaregain
```

查询可用值节点：

```sh
/sys/bus/iio/devices/iio\:device0/hardwaregain_available
```

使用示例：

```sh
cat /sys/bus/iio/devices/iio\:device0/hardwaregain_available
1 3 6 9 18
```

## 白点峰值

用与白平衡校正

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_peak_raw
```

查询当前白点峰值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_peak_raw
```

## RGB原始数据平均值

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_mean_raw
```

查询当前RGB平均值：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_mean_raw
```

## 采样速率调节

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_sampling_frequency
```

查询当前速率调节：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_sampling_frequency
10
```

设置速率调节：

```sh
echo 10 > cat /sys/bus/iio/devices/iio\:device0/in_illuminance_sampling_frequency
```

查询可用值节点：

```sh
/sys/bus/iio/devices/iio\:device0/sampling_frequency_available
```

使用示例：

```sh
cat /sys/bus/iio/devices/iio\:device0/sampling_frequency_available
40 20 10 5 2 1 0.5
```

单位：hz

## 分辨率调节

节点：

```sh
/sys/bus/iio/devices/iio\:device0/in_illuminance_scale
```

查询当前速率调节：

```sh
cat /sys/bus/iio/devices/iio\:device0/in_illuminance_scale
18
```

设置分辨率调节：

```sh
echo 18 > cat /sys/bus/iio/devices/iio\:device0/in_illuminance_scale
```

查询可用值节点：

```sh
/sys/bus/iio/devices/iio\:device0/scale_available
```

使用示例：

```sh
cat /sys/bus/iio/devices/iio\:device0/scale_available
20 19 18 17 16
```

单位：位深度(bit)

## DTS配置

例

```c
&i2c_bus0 {
	ltr381rgbwa@53 {
        compatible = "ltr381rgbwa";
        status = "okay";
        reg = <0x53>;
        //ltr381rgbwa,irq-gpio = <&gpio_chip0 1 0>;
        // 5000k白光下白平衡校准
        //ltr381rgb,g_factor = <1000000>; //1.000000 默认值
        ltr381rgb,r_factor = <1408369>; //1.408368
        ltr381rgb,b_factor = <1547730>; //1.547730
        clock-frequency = <400000>;
	};
};
```

## 调试节点

### reg

用于直接修改寄存器

```sh
/sys/bus/iio/devices/iio\:device0/reg
```

查询寄存器值

```sh
cat /sys/bus/iio/devices/iio\:device0/reg
```

修改寄存器值

```sh
echo 0x00 0x00 > /sys/bus/iio/devices/iio\:device0/reg
```

### cs_mode

修改芯片工作方式

```
/sys/bus/iio/devices/iio\:device0/cs_mode
```

查询当前工作方式

```sh
cat /sys/bus/iio/devices/iio\:device0/cs_mode
```

修改当前工作方式

有两种模式als、cs，默认cs模式

```sh
echo als > /sys/bus/iio/devices/iio\:device0/cs_mode
echo cs > /sys/bus/iio/devices/iio\:device0/cs_mode
```

## 内核配置

```c
CONFIG_LTR381RGB=y
```

## dts配置

```c
&i2c_bus0 {
	status = "okay";
	ltr381rgb@53 {
		compatible = "ltr381rgb";
		status = "okay";
		reg = <0x53>;
		ltr381rgb,r_factor = <1414446>;
		ltr381rgb,b_factor = <1543854>;
		clock-frequency = <400000>;
	};
};
```

