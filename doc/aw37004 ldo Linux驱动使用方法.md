# aw37004 ldo linux驱动使用方法

## 版本

| 版本 | 修订日期   | 内容     |
| ---- | ---------- | -------- |
| 1.0  | 2024.10.23 | 初始版本 |
|      |            |          |
|      |            |          |

## 概要

​		aw37004 ldo对接至linux regulator子系统，可通dts或者regulator子系统提供的接口直接设置所需的电压、电流。

## 内核配置

```c
CONFIG_AW37004=y
```

## dts参考配置

```c
&i2c_bus0 {
	status = "okay";
	#address-cells = <1>;
	#size-cells = <0>;

	aw37004@28 {
		compatible = "aw37004";
		status = "okay";
		reg = <0x28>;
		clock-frequency = <400000>;
		dvdd1 {
			regulator-compatible = "dvdd1";  //名称
			regulator-min-microvolt = <600000>; //最小电压
			regulator-max-microvolt = <2130000>; //最大电压
			regulator-min-microamp = <1300000>; //最小负载电流
			regulator-max-microamp = <1720000>; //最大负载电流
			//regulator-boot-on; //开机自启
			//regulator-always-on; //一直开启
		};
	
		dvdd2 {
			regulator-compatible = "dvdd2";
			regulator-min-microvolt = <600000>;
			regulator-max-microvolt = <2130000>;
			regulator-min-microamp = <1300000>;
			regulator-max-microamp = <1720000>;
			//regulator-boot-on;
			//regulator-always-on;
		};
	
		avdd1 {
			regulator-compatible = "avdd1";
			regulator-min-microvolt = <1200000>;
			regulator-max-microvolt = <4387500>;
			regulator-min-microamp = <480000>;
			regulator-max-microamp = <900000>;
			//regulator-boot-on;
			//regulator-always-on;
	
		};
	
		avdd2 {
			regulator-compatible = "avdd2";
			regulator-min-microvolt = <1200000>;
			regulator-max-microvolt = <4387500>;：
			regulator-min-microamp = <480000>;
			regulator-max-microamp = <900000>;
			//regulator-boot-on;
			//regulator-always-on;
		};
	};
};
```

**注：dvdd、avdd的电压和电流是按挡位划分的，dvdd为6000mV为一个电压步进，140000mA一个电流步进；avdd为12500mV一个电压步进，140000mA一个电流步进。如果设置的参数错误则可能导致ldo无法正常工作**。

## 内核空间使用参考方法

```c
	struct regulator *dvdd1 = NULL;

	//获取ddvd1对象
	dvdd1 = devm_regulator_get(dev, "dvdd1");
	if (dvdd1) {
		dev_err(dev, "xxx");
	}
	//设置ddvd1电压828000mV
	ret = regulator_set_voltage(dvdd1, 828000, 828000);
	if (ret) {
        dev_err(dev, "xxx");
    }
	//设置dvdd1负载电流1580000mA
	ret = regulator_set_current_limit(dvdd1, 1580000, 1580000);
	if (ret) {
        dev_err(dev, "xxx");
    }
	//开启ddvd1
	ret = regulator_enable(dvdd1);
	if (ret) {
        dev_err(dev, "xxx");
    }
```

## 特殊情况

​		对于一些特殊平台，即不使用linux内核配置配置外设的，如海思平台，则可以直接在dts设置所需的电压和电流，并设置"regulator-boot-on"，"regulator-always-on"，标识，使其在开机时自启并保持开启状态。

参考配置：

```c
&i2c_bus0 {
	status = "okay";
	#address-cells = <1>;
	#size-cells = <0>;
	
	aw37004@28 {
		compatible = "aw37004";
		status = "okay";
		reg = <0x28>;
		clock-frequency = <400000>;
		dvdd1 {
			regulator-compatible = "dvdd1";
			regulator-min-microvolt = <828000>;
			regulator-max-microvolt = <828000>;
			regulator-min-microamp = <1580000>;
			regulator-max-microamp = <1580000>;
			regulator-boot-on;
			regulator-always-on;
		};
	
		dvdd2 {
			regulator-compatible = "dvdd2";
			regulator-min-microvolt = <1200000>;
			regulator-max-microvolt = <1200000>;
			regulator-min-microamp = <1300000>;
			regulator-max-microamp = <1300000>;
			regulator-boot-on;
			regulator-always-on;
		};
	
		avdd1 {
			regulator-compatible = "avdd1";
			regulator-min-microvolt = <1800000>;
			regulator-max-microvolt = <1800000>;
			regulator-min-microamp = <480000>;
			regulator-max-microamp = <480000>;
			regulator-boot-on;
			regulator-always-on;
	
		};
	
		avdd2 {
			regulator-compatible = "avdd2";
			regulator-min-microvolt = <2900000>;
			regulator-max-microvolt = <2900000>;
			regulator-min-microamp = <900000>;
			regulator-max-microamp = <900000>;
			regulator-boot-on;
			regulator-always-on;
		};
	};
};
```

