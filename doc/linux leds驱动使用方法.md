# linux leds驱动使用方法

## 版本

| 版本 | 修订日期   | 内容     |
| ---- | ---------- | -------- |
| 1.0  | 2024.11.27 | 初始版本 |
|      |            |          |
|      |            |          |

## 概要

​		linux leds子系统为linux为了统一管理系统的leds设备而构建的子系统，在内核提供统一的控制接口，并在sysfs提供统一的操作的节点。

## 内核配置

```c
CONFIG_NEW_LEDS=y //必须
CONFIG_LEDS_CLASS=y //必须
CONFIG_LEDS_GPIO=y //必须，使用gpio控制led
CONFIG_INPUT_LEDS=y //按需
CONFIG_LEDS_TRIGGERS=y //按需
CONFIG_LEDS_TRIGGER_TIMER=y //按需
```

## dts配置

```c
leds: gpio-leds {
        status = "disabled";
        compatible = "gpio-leds";
};

&leds {
        status = "ok";

        led_yellow {
                label = "yellow"; //标签名
                gpios = <&gpio_chip9 6 0>;
                default-state = "on"; //默认状态
                linux,default-trigger = "timer"; //触发器，用于闪烁控制
        };
};
```

## 操作方法

节点

```sh
ls /sys/class/leds/
mmc0::/  yellow/
```

所有的leds类设备都在该节点下。

例**黄灯**节点

```sh
ls /sys/class/leds/yellow/
brightness      device          subsystem
delay_off       max_brightness  trigger
delay_on        power           uevent
```

**注：trigger需要配置CONFIG_LEDS_TRIGGERS的配置才会出现。**

**注：delay_on、delay_off需要绑定timer触发器才会生成。**

### 亮度控制

节点：

```sh
/sys/class/leds/yellow/brightness
```

打开led

```sh
echo 1 > /sys/class/leds/yellow/brightness
```

对于gpio控制的led来说，只要是非零值就是打开，值范围1~255。

关闭led

```sh
echo 0 > /sys/class/leds/yellow/brightness
```

### 触发器配置

节点：

```sh
/sys/class/leds/yellow/trigger
```

查看当前绑定的触发器

```sh
cat /sys/class/leds/yellow/trigger
none kbd-scrolllock kbd-numlock kbd-capslock kbd-kanalock kbd-shiftlock kbd-altgrlock kbd-ctrllock kbd-altlock kbd-shiftllock kbd-shiftrlock kbd-ctrlllock kbd-ctrlrlock mmc0 [timer]
```

**注："[]"框住的为当前绑定的触发器，如上[timer]表示当前绑定的触发器为timer。**

绑定指定触发器

```sh
echo timer > /sys/class/leds/yellow/trigger
```

**注：如果值为"none"，则表示解绑触发器。**

## timer触发器配置

​		timer一共有两个节点配置delay_on和delay_off。

### delay_on

​		用于控制led开启时长，单位ms

查看当前开启时长

```sh
cat /sys/class/leds/yellow/delay_on
500
```

配置开启时长

```sh
echo 500 > /sys/class/leds/yellow/delay_on
```

### delay_off

​		用于控制led关闭时长，单位ms

查看当前关闭时长

```sh
cat /sys/class/leds/yellow/delay_off
500
```

配置关闭时长

```sh
echo 500 > /sys/class/leds/yellow/delay_off
```

**注：若值为0，表示常量后者常灭。**

## 其他触发器...（待补充）