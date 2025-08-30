# SGM7220 linux驱动使用方法

## 版本

| 版本 | 修订日期   | 内容     |
| ---- | ---------- | -------- |
| 1.0  | 2024.10.23 | 初始版本 |
|      |            |          |
|      |            |          |

## 概要

​		sgm7220是一款实现usb otg功能的外挂芯片，目前对接至linux extcon子系统，可通dts配置实现支持drd的usb主控实现otg功能和正反插功能。

## 内核配置

内核需要将USB DWC3的DRD配置打开

```c
CONFIG_USB_DWC3_DUAL_ROLE=y
```

开启EXTCON框架支持

```c
CONFIG_EXTCON=y
```

sgm7220驱动配置打开

```c
CONFIG_SGM7220=y
```

## dts配置

```c
//sgm7220芯片配置
&i2c_bus0 {
	status = "okay";
	sgm7220: sgm7220@47 {
		compatible = "sgm7220";
		status = "okay";
		reg = <0x47>;
		sgm7220,irq-gpio = <&gpio_chip5 3 0>; //中断io
		sgm7220,switch3_0-gpio = <&gpio_chip12 1 0>; //usb3.0链路切换io
		sgm7220,switch2_0-gpio = <&gpio_chip16 1 0>; //usb2.0链路切换io
		sgm7220,id-gpio = <&gpio_chip4 6 0>; //主从检车io，可以不配
		clock-frequency = <10000>;
	};
};
//usb主控配置
&bspdwc3 {
	dr_mode = "otg"; //配置为otg模式
	extcon = <&sgm7220>; //配置sgm7220外挂芯片
};
```

