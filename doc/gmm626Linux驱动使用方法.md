# gmm626 Linux驱动使用方法

## 版本

| 版本 | 修订日期   | 内容     |
| ---- | ---------- | -------- |
| 1.0  | 2024.10.23 | 初始版本 |
|      |            |          |
|      |            |          |

## 概要

​		该驱动参考原厂文档参考开发，并使用Linux标准接口进行开发适配，保证驱动的通用性。

## 内核配置

```c
CONFIG_GMM626=y
```

## dts参考配置

```c
&spi_bus1 {
	/delete-node/ spidev@0;
	gmm626@0 {
		compatible = "gmm626";
		status = "okay";
		reg = <0>;
		pl022,interface = <0>; //海思spi驱动配置
		pl022,com-mode = <0>; //海思spi驱动配置
		gmm626,standby-gpio = <&aw95016_gpios 12 0>;
		gmm626,reset-gpio = <&aw95016_gpios 13 0>;
		gmm626,rate0-gpio = <&aw95016_gpios 14 0>;
		gmm626,rate1-gpio = <&aw95016_gpios 15 0>;
		spi-max-frequency = <10000000>;
	};
};
```

