// SPDX-License-Identifier: GPL-2.0

s32 gpio_direction_input(u32 gpio);
s32 gpio_direction_output(u32 gpio, int value);
void gpio_set_value(u32 gpio, int value);
s32 gpio_get_value(u32 gpio);
