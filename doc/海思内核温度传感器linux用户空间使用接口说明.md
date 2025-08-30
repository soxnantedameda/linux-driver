# 海思内核温度传感器linux用户空间使用接口说明

## 版本

| 版本 | 修订日期  | 内容     |
| ---- | --------- | -------- |
| 1.0  | 2024.11.5 | 初始版本 |
|      |           |          |
|      |           |          |

## 概要

​		海思芯片内部集成了3个温度传感器，可用来查看芯片的温度，为此开发了linux对应的thermal驱动，可直接获取芯片当前温度。

## 接口说明

​		linux通过thermal子系统通过sysfs提供温度的查询节点。

```sh
# 传感器0
/sys/class/thermal/thermal_zone0/
# 传感器1
/sys/class/thermal/thermal_zone1/
# 传感器2
/sys/class/thermal/thermal_zone2/
```

## 温度

节点：

```
/sys/class/thermal/thermal_zone0/temp
```

查村温度：

```sh
cat /sys/class/thermal/thermal_zone0/temp
```

## patch修改

[hollyland: add hisi Tsensor driver (3424ae6c) · Commits · C4902 / BSP · GitLab](http://192.168.2.88/c4902/bsp/commit/3424ae6c38dc300c74330411d0bdd8a639e8588f)