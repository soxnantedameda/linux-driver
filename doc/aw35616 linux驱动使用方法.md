# Linux Extcon（外部连接器）子系统详解

## 版本

| 版本 | 修订日期  | 内容                       |
| ---- | --------- | -------------------------- |
| 1.0  | 2025.5.7  | 初始版本                   |
| 1.1  | 2025.5.25 | 新增Linux extcon子系统说明 |
|      |           |                            |

## 简介

​		Extcon（External Connector）是Linux内核中用于管理外部连接器状态的子系统，主要用于处理USB接口、音频插孔、HDMI等外部连接设备的状态监测和通知。

## 核心功能

1. **状态管理**：
   - 检测连接/断开状态
   - 维护连接器当前状态
   - 支持多种连接类型（USB、HDMI、音频等）
2. **事件通知**：
   - 当连接状态变化时通知相关驱动
   - 使用内核通知链(notifier chain)机制
3. **统一抽象**：
   - 为不同类型的连接器提供统一接口
   - 简化驱动开发

## 典型应用场景

1. **USB接口管理**：
   - 区分USB主机(host)/设备(device)模式
   - 处理USB Type-C的双角色切换
2. **音频插孔检测**：
   - 耳机插入/拔出检测
   - 区分耳机和麦克风
3. **充电器检测**：
   - 识别充电器类型（标准/快速充电）

## 关键API接口

```c
// 注册extcon设备
struct extcon_dev *extcon_dev_allocate(const unsigned int *cable);
int extcon_dev_register(struct extcon_dev *edev);

// 状态设置与通知
int extcon_set_state(struct extcon_dev *edev, unsigned int id, bool state);
int extcon_set_state_sync(struct extcon_dev *edev, unsigned int id, bool state);

// 通知注册
int extcon_register_notifier(struct extcon_dev *edev, unsigned int id,
                            struct notifier_block *nb);
// 能力设置
int extcon_set_property_capability(struct extcon_dev *edev, unsigned int id,
					unsigned int prop)
```

## 设备树配置示例

```c
extcon_usb: extcon-usb {
    compatible = "linux,extcon-usb-gpio";
    id-gpio = <&gpio0 12 GPIO_ACTIVE_HIGH>;
    vbus-gpio = <&gpio0 11 GPIO_ACTIVE_HIGH>;
};
```

## 开发建议

1. 优先使用内核已支持的extcon驱动
2. 对于自定义硬件，可以实现extcon接口
3. 注意处理并发和同步问题
4. 合理使用状态同步(_sync)接口

Extcon子系统简化了外部连接器管理，使各驱动可以专注于自身功能而不必直接处理连接状态变化。

## 基于extcon子系统的AW35616驱动使用方法

​		AW35616是一款usb协议芯片，目前对接至linux extcon子系统，可通dts配置实现支持drd的usb主控实现otg功能和正反插功能。

## 内核配置

内核需要将USB DWC3的DRD配置打开

```c
CONFIG_USB_DWC3_DUAL_ROLE=y
```

开启EXTCON框架支持

```c
CONFIG_EXTCON=y
```

AW35616驱动配置打开

```c
CONFIG_AW35616=y
```

## dts配置

```c
//AW35616芯片配置
&qupv3_se15_i2c {
	status = "okay";
	extcon_aw35616: aw35616@68 {
		compatible = "awinic,aw35616";
		status = "okay";
		reg = <0x68>;
		aw35616,int-gpio = <&tlmm 3 0>; //中断io
        aw35616,cc_switch-gpio = <&tlmm 11 0>; //正反插控制引脚，可选
        vdd-3.3-supply = <&L11C>; //vdd3.3供电,可选
	};
};
//usb主控配置
&usb0 {
	extcon = <&extcon_aw35616>; //配置aw35616外挂芯片
};
```
