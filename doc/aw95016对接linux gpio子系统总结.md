# AW95016扩展GPIO linux驱动使用方法

## 版本

| 版本 | 修订日期   | 内容     |
| ---- | ---------- | -------- |
| 1.0  | 2024.10.23 | 初始版本 |
|      |            |          |
|      |            |          |

## 概要

​		aw95016扩展GPIO目前是基于原厂的驱动的基础上对接至linux gpio子系统，可通dts或者sysfs提供的接口直接设置gpio状态。

## 内核配置

```c
CONFIG_AW95016=y
```

## dts参考配置

```c
&i2c_bus0 {
	status = "okay";

	aw95016_gpios: aw95016_ex@20 {
		compatible = "awinic,aw95016";
		status = "okay";
		reg = <0x20>;
		#gpio-cells = <2>; //新增配置
		base = <144>; //新增配置
		reset-aw95016 = <&gpio_chip5 0 IRQ_TYPE_LEVEL_HIGH>;
		irq-aw95016 = <&gpio_chip0 1 IRQ_TYPE_LEVEL_HIGH>;
		clock-frequency = <400000>;
		aw95016,single_key_enable = <1>; //enable single key function, 0 to disable
		aw95016,matrix_key_enable = <0>; //enable matrix key function, 0 to disable
		aw95016,gpio_enable = <1>; //enable gpio function, 0 to disable
		aw95016,key{
			aw95016,wake_up_enable = <1>; //enabel key function in suspend mode,0 to disable
			aw95016,input_port_mask = <0x0003>; // 0000 1111 0000 0000 Identifies the pin port for the input. here: P1_0-P1_3
			aw95016,output_port_mask = <0x0000>; //1111 0000 0000 0000 Identifies the pin port for the output here: P1_4-P1_7
		};
		aw95016,gpio{
			aw95016,gpio_mode = <0>;
			gpio06{
				aw95016,gpio_idx = <6>; // The specific port identifier used, This is used here:P0_6
				aw95016,gpio_dir = <0>; // The specific port work in output(1) or input(0)
				aw95016,gpio_default_val = <1>;
			};
			gpio07{
				aw95016,gpio_idx = <7>; // The specific port identifier used, This is used here:P0_7
				aw95016,gpio_dir = <0>; //The specific port work in output(1) or input(0)
				aw95016,gpio_default_val = <1>;
			};
			gpio10{
				aw95016,gpio_idx = <8>; // The specific port identifier used, This is used here:P1_0
				aw95016,gpio_dir = <1>; // The specific port work in output(1) or input(0)
				aw95016,gpio_default_val = <1>;
			};
			gpio11{
				aw95016,gpio_idx = <9>; // The specific port identifier used, This is used here:P1_1
				aw95016,gpio_dir = <1>; // The specific port work in output(1) or input(0)
				aw95016,gpio_default_val = <1>;
			};
		};
	};
```

**注："#gpio-cells = <2>"、"base = <144>"为驱动新增配置，其中"#gpio-cells = <2>"为gpio子系统的必须配置，表明在dts中申请gpio时所需的参数个数，"base = <144>"为建议配置，用于表示该扩展io的起始标号，一般配置为主控的最大gpio数量，例如海思的gpio为0~143，共144可用的gpio，该扩展的扩展io为起始号为144，在gpio的sysfs中就可以相关接口申请到对应的gpio，如果不配置，该扩展gpio的标号就由linux来分配。**

## dts其他申请扩展GPIO

例

```c
&spi_bus1 {
	/delete-node/ spidev@0;
	gs2972@0 {
		compatible = "gs2972";
		status = "okay";
		reg = <0>;
		pl022,interface = <0>;
		pl022,com-mode = <0>;
		gs2972,spi_standby-gpio = <&aw95016_gpios 12 0>;
		gs2972,reset-gpio = <&aw95016_gpios 13 0>;
		gs2972,rate0-gpio = <&aw95016_gpios 14 0>;
		gs2972,rate1-gpio = <&aw95016_gpios 15 0>;
		spi-max-frequency = <10000000>;
	};
};
```

