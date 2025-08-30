// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <asm/io.h>

#include "hisi_pwm.h"

/* pwm reg */
#define pwm0_reg_base              (0x110B0000)
#define pwm1_reg_base              (0x1102D000)

/* clk reg */
#define pwm0_clk_reg_base          (0x11014588)
#define pwm1_clk_reg_base          (0x11014590)

#define pwm_srst_req               BIT(0)
#define pwm_cken                   BIT(4)

/* reg addr of the xth chn. */
#define pwm_period_cfg_addr(x)     (0x0000 + (0x100 * (x)))
#define pwm_duty0_cfg_addr(x)      (0x0004 + (0x100 * (x)))
#define pwm_duty1_cfg_addr(x)      (0x0008 + (0x100 * (x)))
#define pwm_duty2_cfg_addr(x)      (0x000C + (0x100 * (x)))
#define pwm_num_cfg_addr(x)        (0x0010 + (0x100 * (x)))
#define pwm_ctrl_addr(x)           (0x0014 + (0x100 * (x)))
#define pwm_dt_value_cfg_addr(x)   (0x0020 + (0x100 * (x)))
#define pwm_dt_ctrl_cfg_addr(x)    (0x0024 + (0x100 * (x)))
#define pwm_sync_cfg_addr(x)       (0x0030 + (0x100 * (x)))
#define pwm_sync_delay_cfg_addr(x) (0x0034 + (0x100 * (x)))
#define pwm_period_addr(x)         (0x0040 + (0x100 * (x)))
#define pwm_duty0_addr(x)          (0x0044 + (0x100 * (x)))
#define pwm_duty1_addr(x)          (0x0048 + (0x100 * (x)))
#define pwm_duty2_addr(x)          (0x004C + (0x100 * (x)))
#define pwm_num_addr(x)            (0x0050 + (0x100 * (x)))
#define pwm_ctrl_st_addr(x)        (0x0054 + (0x100 * (x)))
#define pwm_dt_value_addr(x)       (0x0060 + (0x100 * (x)))
#define pwm_dt_ctrl_addr(x)        (0x0064 + (0x100 * (x)))
#define pwm_sync_delay_addr(x)     (0x0074 + (0x100 * (x)))

#define PWM_SYNC_START_ADDR        0x0ff0

#define PWM_ALIGN_MODE_SHIFT      4
#define PWM_ALIGN_MODE_MASK       GENMASK(5, 4)

#define PWM_PRE_DIV_SEL_SHIFT     8
#define PWM_PRE_DIV_SEL_MASK      GENMASK(11, 8)

/* pwm dt value */
#define PWM_DT_A_SHIFT      0
#define PWM_DT_A_MASK       GENMASK(31, 16)

#define PWM_DT_B_SHIFT      16
#define PWM_DT_B_MASK       GENMASK(15, 0)

/* pwm dt ctrl */
#define PWM_DTS_OUT_0P_SHIFT      0
#define PWM_DTS_OUT_0P_MASK       BIT(0)

#define PWM_DTS_OUT_0N_SHIFT      1
#define PWM_DTS_OUT_0N_MASK       BIT(1)

#define PWM_DTS_OUT_1P_SHIFT      2
#define PWM_DTS_OUT_1P_MASK       BIT(2)

#define PWM_DTS_OUT_1N_SHIFT      3
#define PWM_DTS_OUT_1N_MASK       BIT(3)

#define PWM_DTS_OUT_2P_SHIFT      4
#define PWM_DTS_OUT_2P_MASK       BIT(4)

#define PWM_DTS_OUT_2N_SHIFT      5
#define PWM_DTS_OUT_2N_MASK       BIT(5)

/* pwm ctrl */
#define PWM_ENABLE_SHIFT    0
#define PWM_ENABLE_MASK     BIT(0)

#define PWM_POLARITY_SHIFT  1
#define PWM_POLARITY_MASK   BIT(1)

#define PWM_KEEP_SHIFT      2
#define PWM_KEEP_MASK       BIT(2)

/* pwm period */
#define PWM_PERIOD_MASK     GENMASK(31, 0)

/* pwm duty */
#define PWM_DUTY_MASK       GENMASK(31, 0)

struct bsp_pwm_soc {
	u64 reg_base;
	u64 clk_reg_base;
	u64 clk;
	u32 num_pwms;
};

#define CHIP_PWM_NUM 2

static const struct bsp_pwm_soc pwm_soc[CHIP_PWM_NUM] = {
	{
		.reg_base = pwm0_reg_base,
		.clk_reg_base = pwm0_clk_reg_base,
		.clk = 200000000,
		.num_pwms = 16
	},
	{
		.reg_base = pwm1_reg_base,
		.clk_reg_base = pwm1_clk_reg_base,
		.clk = 200000000,
		.num_pwms = 16
	},
};

static inline u64 div_u64_rem(u64 dividend, u32 divisor, u32 *remainder)
{
	*remainder = dividend % divisor;
	return dividend / divisor;
}

static inline u64 div_u64(u64 dividend, u32 divisor)
{
	u32 remainder;
	return div_u64_rem(dividend, divisor, &remainder);
}

static void bsp_pwm_set_bits(u64 base, u32 offset,
					u32 mask, u32 data)
{
	u64 address = base + offset;
	u32 value;

	value = readl(address);
	value &= ~mask;
	value |= (data & mask);
	writel(value, address);
}

static void bsp_pwm_enable(pwm_controller_index controller_index, pwm_chn_index chn_index)
{
	u64 reg_base;

	if (controller_index > CHIP_PWM_NUM) {
		printf("invalid pwm chip num\n");
		return;
	}

	reg_base = pwm_soc[controller_index].reg_base;
	if (chn_index > pwm_soc[controller_index].num_pwms) {
		printf("invalid pwm channel num\n");
		return;
	}

	bsp_pwm_set_bits(reg_base, pwm_ctrl_addr(chn_index),
			PWM_ENABLE_MASK, 0x1);
}

static void bsp_pwm_disable(pwm_controller_index controller_index, pwm_chn_index chn_index)
{
	u64 reg_base;

	if (controller_index > CHIP_PWM_NUM) {
		printf("invalid pwm chip num\n");
		return;
	}

	reg_base = pwm_soc[controller_index].reg_base;
	if (chn_index > pwm_soc[controller_index].num_pwms) {
		printf("invalid pwm channel num\n");
		return;
	}

	bsp_pwm_set_bits(reg_base, pwm_ctrl_addr(chn_index),
			PWM_ENABLE_MASK, 0x0);
}

static bool bsp_pwm_is_complementary_chn(pwm_controller_index controller_index, pwm_chn_index chn_index)
{
	if (((controller_index == PWM_CONTROLLER_0) && (chn_index == PWM_CHN_15)) ||
		((controller_index == PWM_CONTROLLER_1) &&
		((chn_index == PWM_CHN_0) || (chn_index == PWM_CHN_1)))) {
		return 1;
	}
	return 0;
}

static void bsp_pwm_config(pwm_controller_index controller_index, pwm_chn_index chn_index,
					const struct pwm_state *state, u32 period_ns)
{
	u64 freq, period, duty, duty1, duty2;
	u64 reg_base;

	if (controller_index > CHIP_PWM_NUM) {
		printf("invalid pwm chip num\n");
		return;
	}

	reg_base = pwm_soc[controller_index].reg_base;
	if (chn_index > pwm_soc[controller_index].num_pwms) {
		printf("invalid pwm channel num\n");
		return;
	}

	freq = div_u64(pwm_soc[controller_index].clk, 1000000);

	period = div_u64(freq * period_ns, 1000);
	duty = div_u64(period * state->duty_cycle, period_ns);
	duty1 = div_u64(period * state->duty_cycle1, period_ns);
	duty2 = div_u64(period * state->duty_cycle2, period_ns);

	bsp_pwm_set_bits(reg_base, pwm_ctrl_addr(chn_index),
		PWM_PRE_DIV_SEL_MASK, (PWM_PRE_DIV_1 << PWM_PRE_DIV_SEL_SHIFT));

	bsp_pwm_set_bits(reg_base, pwm_period_cfg_addr(chn_index),
			PWM_PERIOD_MASK, period);

	bsp_pwm_set_bits(reg_base, pwm_duty0_cfg_addr(chn_index),
			PWM_DUTY_MASK, duty);

	if (bsp_pwm_is_complementary_chn(controller_index, chn_index) == 1) {
		bsp_pwm_set_bits(reg_base, pwm_duty1_cfg_addr(chn_index),
				PWM_DUTY_MASK, duty1);
		bsp_pwm_set_bits(reg_base, pwm_duty2_cfg_addr(chn_index),
				PWM_DUTY_MASK, duty2);
	}
}

static void bsp_pwm_set_polarity(pwm_controller_index controller_index,
					pwm_chn_index chn_index,
					enum pwm_polarity polarity)
{
	u64 reg_base;

	if (controller_index > CHIP_PWM_NUM) {
		printf("invalid pwm chip num\n");
		return;
	}

	reg_base = pwm_soc[controller_index].reg_base;
	if (chn_index > pwm_soc[controller_index].num_pwms) {
		printf("invalid pwm channel num\n");
		return;
	}

	if (polarity == PWM_POLARITY_INVERSED)
		bsp_pwm_set_bits(reg_base, pwm_ctrl_addr(chn_index),
				PWM_POLARITY_MASK, (0x1 << PWM_POLARITY_SHIFT));
	else
		bsp_pwm_set_bits(reg_base, pwm_ctrl_addr(chn_index),
				PWM_POLARITY_MASK, (0x0 << PWM_POLARITY_SHIFT));
}

static void bsp_pwm_get_state(pwm_controller_index controller_index, pwm_chn_index chn_index,
				struct pwm_state *state)
{
	u64 reg_base;
	u32 freq, value;

	if (controller_index > CHIP_PWM_NUM) {
		printf("invalid pwm chip num\n");
		return;
	}

	reg_base = pwm_soc[controller_index].reg_base;
	if (chn_index > pwm_soc[controller_index].num_pwms) {
		printf("invalid pwm channel num\n");
		return;
	}

	freq = div_u64(pwm_soc[controller_index].clk, 1000000);
	value = readl(reg_base + pwm_period_cfg_addr(chn_index));
	state->period = div_u64(value * 1000, freq);

	value = readl(reg_base + pwm_duty0_cfg_addr(chn_index));
	state->duty_cycle = div_u64(value * 1000, freq);

	if (bsp_pwm_is_complementary_chn(controller_index, chn_index) == 1) {
		value = readl(reg_base + pwm_duty1_cfg_addr(chn_index));
		state->duty_cycle1 = div_u64(value * 1000, freq);

		value = readl(reg_base + pwm_duty2_cfg_addr(chn_index));
		state->duty_cycle2 = div_u64(value * 1000, freq);
	}

	value = readl(reg_base + pwm_ctrl_addr(chn_index));
	state->enabled = (PWM_ENABLE_MASK & value);
}

int bsp_pwm_apply(pwm_controller_index controller_index, pwm_chn_index chn_index,
				struct pwm_state *state)
{
	struct pwm_state old_state = {0};

	bsp_pwm_get_state(controller_index, chn_index, &old_state);

	if (state->polarity != old_state.polarity)
		bsp_pwm_set_polarity(controller_index, chn_index, state->polarity);

	if (state->period != old_state.period ||
		state->duty_cycle != old_state.duty_cycle ||
		state->duty_cycle1 != old_state.duty_cycle1 ||
		state->duty_cycle2 != old_state.duty_cycle2)
		bsp_pwm_config(controller_index, chn_index, state, state->period);

	if (state->enabled != old_state.enabled) {
		if (state->enabled)
			bsp_pwm_enable(controller_index, chn_index);
		else
			bsp_pwm_disable(controller_index, chn_index);
	}

	return 0;
}

int bsp_pwm_init(pwm_controller_index controller_index)
{
	u64 reg_base;
	u64 clk_reg_base;
	u32 reg_value;
	u32 mask;
	int i;

	if (controller_index > CHIP_PWM_NUM) {
		printf("invalid pwm chip num\n");
		return -1;
	}
	reg_base = pwm_soc[controller_index].reg_base;
	clk_reg_base = pwm_soc[controller_index].clk_reg_base;

	/* enable clk */
	mask = pwm_cken;
	reg_value = readl(clk_reg_base);
	reg_value |= mask;
	writel(reg_value, clk_reg_base);

	/* reset pwm */
	mask = pwm_srst_req;
	reg_value = readl(clk_reg_base);
	reg_value |= mask;
	writel(reg_value, clk_reg_base);
	mdelay(30);
	reg_value = readl(clk_reg_base);
	reg_value &= ~mask;
	writel(reg_value, clk_reg_base);

	for (i = 0; i < pwm_soc[controller_index].num_pwms; i++) {
		bsp_pwm_set_bits(reg_base, pwm_ctrl_addr(i),
				PWM_KEEP_MASK, (0x1 << PWM_KEEP_SHIFT));
	}

	return 0;
}

int bsp_pwm_deinit(pwm_controller_index controller_index)
{
	u64 clk_reg_base;
	u32 reg_value;
	u32 mask;

	if (controller_index > CHIP_PWM_NUM) {
		printf("invalid pwm chip num\n");
		return -1;
	}

	clk_reg_base = pwm_soc[controller_index].reg_base;

	mask = pwm_srst_req;
	reg_value = readl(clk_reg_base);
	reg_value |= mask;
	writel(reg_value, clk_reg_base);
	mdelay(30);
	reg_value = readl(clk_reg_base);
	reg_value &= ~pwm_srst_req;
	writel(reg_value, clk_reg_base);
	mask = pwm_cken;
	reg_value = readl(clk_reg_base);
	reg_value &= ~mask;
	writel(reg_value, clk_reg_base);

	return 0;
}

