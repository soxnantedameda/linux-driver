# gsv2k11 linux驱动使用方法

## 版本

| 版本 | 修订日期   | 内容     |
| ---- | ---------- | -------- |
| 1.0  | 2024.10.23 | 初始版本 |
|      |            |          |
|      |            |          |

## 概要

​		gsv2k11Linux驱动移植至原厂MCU驱动，在此基础上对接了linux标准的i2c的驱动，并实现了linux内核消息链框架，可实现根据分辨率配置其他外设驱动的功能。

## 内核配置

```c
CONFIG_GSV2K11=y
```

## dts参考配置

```c
&i2c_bus0 {
	status = "okay";
    	gsv2k11: gsv2k11@58 {
		compatible = "gsv2k11";
		status = "okay";
		reg = <0x58>;
		gsv2k11,reset-gpio = <&aw95016_gpios 11 0>;
		gsv2k11,mute-gpio = <&aw95016_gpios 10 0>;
		clock-frequency = <400000>;
	};
};
```

## 使用消息链控制其他外设

例，在gs2972驱动中

```c
#include <linux/gsv2k11_notifier.h> //包含消息链必要头文件
...

static int gs2972_mode_notifier(struct notifier_block *nb,
     unsigned long event, void *data)
{
    //根据不同分辨做相应处理，event为CEA规范的VIC
    ...
}

static int gs2972_driver_probe(struct spi_device *spi)
{
  	struct gs2972_data *gs2972;
    ...
    //注册gsv2k11的消息链的回调函数
    gs2972->mode_nb.notifier_call = gs2972_mode_notifier;
	gsv2k11_notifier_register(&gs2972->mode_nb);
	....
    return 0;
}
```

