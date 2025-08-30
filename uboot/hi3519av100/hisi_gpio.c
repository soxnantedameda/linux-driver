// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <asm/io.h>

#define GPIO_REG_BASE 0x11090000

#define GPIO_DATA 0x0
#define GPIO_DIR 0x400

s32 gpio_direction_input(u32 gpio)
{
    u64 group = gpio / 8;
    u64 offset = gpio % 8;
    u64 reg_base = GPIO_REG_BASE + group * 0x1000;
    u64 dir_reg  = reg_base + GPIO_DIR;

    writel(readl(dir_reg) | ~(0x1 << offset), dir_reg);
    return 0;
}

s32 gpio_direction_output(u32 gpio, int value)
{
    u64 group = gpio / 8;
    u64 offset = gpio % 8;
    u64 reg_base = GPIO_REG_BASE + group * 0x1000;
    u64 dir_reg  = reg_base + GPIO_DIR;
    u64 data_reg = reg_base + (0x1 << (offset + 2));

    writel(readl(dir_reg) | (0x1 << offset), dir_reg);
    writel(value ? 0xff : 0x00, data_reg);
    return 0;
}

void gpio_set_value(u32 gpio, int value)
{
    u64 group = gpio / 8;
    u64 offset = gpio % 8;
    u64 reg_base = GPIO_REG_BASE + group * 0x1000;
    u64 data_reg = reg_base + (0x1 << (offset + 2));

    writel(value ? 0xff : 0x00, data_reg);
}

s32 gpio_get_value(u32 gpio)
{
    u64 group = gpio / 8;
    u64 offset = gpio % 8;
    u64 reg_base = GPIO_REG_BASE + group * 0x1000;
    u64 data_reg = reg_base + (0x1 << (offset + 2));

    return !!readl(data_reg);
}

