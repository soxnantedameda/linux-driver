
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/reset.h>

/* addr offset */
#define SVB_PWM_DUTY GENMASK(25, 16)
#define SVB_PWM_PERIOD GENMASK(13, 4)
#define SVB_PWM_LOAD BIT(2)
#define SVB_PWM_INV BIT(1)
#define SVB_PWM_ENABLE BIT(0)

#define SVB_PWM_PERIOD_HZ 1000000000

struct hisi_svb_pwm_data {
	struct pwm_chip	chip;
	struct clk *clk;
	void __iomem *base;
	struct reset_control *rstc;
};

static void hisi_svb_pwm_set_bits(void __iomem *base, u32 offset,
					u32 mask, u32 data)
{
	void __iomem *address = base + offset;
	u32 value;

	value = readl(address);
	value &= ~mask;
	value |= (data & mask);
	writel(value, address);
}

static inline struct hisi_svb_pwm_data *to_hisi_svb_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct hisi_svb_pwm_data, chip);
}

static void hisi_svb_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
	struct pwm_state *state)
{
	struct hisi_svb_pwm_data *hisi_svb_pwm = to_hisi_svb_pwm_chip(chip);
	void __iomem *base = hisi_svb_pwm->base + pwm->hwpwm * 0x4;
	u64 rate, freq;
	u32 reg_val, svb_pwm_period, svb_pwm_duty;

	rate = clk_get_rate(hisi_svb_pwm->clk);
	reg_val = readl(base);
	svb_pwm_period = ((reg_val & SVB_PWM_PERIOD) >> 4) + 1;
	svb_pwm_duty = ((reg_val & SVB_PWM_DUTY) >> 16) + 1;

	freq = div_u64(rate, svb_pwm_period);
	state->period = SVB_PWM_PERIOD_HZ / freq;
	state->duty_cycle = div_u64(state->period * svb_pwm_duty, svb_pwm_period);
	state->polarity = (reg_val & SVB_PWM_INV) ? PWM_POLARITY_INVERSED : PWM_POLARITY_NORMAL;
	state->enabled = (reg_val & SVB_PWM_ENABLE) ? true : false;
}

static void hisi_svb_pwm_set_polarity(struct pwm_chip *chip,
					struct pwm_device *pwm,
					enum pwm_polarity polarity)
{
	struct hisi_svb_pwm_data *hisi_svb_pwm = to_hisi_svb_pwm_chip(chip);

	if (polarity == PWM_POLARITY_INVERSED)
		hisi_svb_pwm_set_bits(hisi_svb_pwm->base, pwm->hwpwm * 0x4,
			SVB_PWM_INV, 0x1 << 1);
	else
		hisi_svb_pwm_set_bits(hisi_svb_pwm->base, pwm->hwpwm * 0x4,
			SVB_PWM_INV, 0x0);
}

static void hisi_svb_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
					const struct pwm_state *state)
{
	struct hisi_svb_pwm_data *hisi_svb_pwm = to_hisi_svb_pwm_chip(chip);
	u32 offset = pwm->hwpwm * 0x4;
	u64 rate, freq;
	u32 svb_pwm_period, svb_pwm_duty;

	rate = clk_get_rate(hisi_svb_pwm->clk);
	freq = SVB_PWM_PERIOD_HZ / state->period;
	svb_pwm_period = (u32)rate / freq - 1;
	svb_pwm_duty = (rate / freq) * state->duty_cycle / state->period - 1;

	hisi_svb_pwm_set_bits(hisi_svb_pwm->base, offset, SVB_PWM_PERIOD, svb_pwm_period << 4);
	hisi_svb_pwm_set_bits(hisi_svb_pwm->base, offset, SVB_PWM_DUTY, svb_pwm_duty << 16);
	hisi_svb_pwm_set_bits(hisi_svb_pwm->base, offset, SVB_PWM_LOAD, 0x4);
}

static void hisi_svb_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm ,bool en)
{
	struct hisi_svb_pwm_data *hisi_svb_pwm = to_hisi_svb_pwm_chip(chip);

	if (en)
		hisi_svb_pwm_set_bits(hisi_svb_pwm->base, pwm->hwpwm * 0x4, SVB_PWM_ENABLE, 0x1);
	else
		hisi_svb_pwm_set_bits(hisi_svb_pwm->base, pwm->hwpwm * 0x4, SVB_PWM_ENABLE, 0x0);
}

static int hisi_svb_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
	const struct pwm_state *state)
{
	if (state->polarity != pwm->state.polarity) {
		hisi_svb_pwm_set_polarity(chip, pwm, state->polarity);
	}

	if (state->period != pwm->state.period ||
		state->duty_cycle != pwm->state.duty_cycle)
		hisi_svb_pwm_config(chip, pwm, state);

	if (state->enabled != pwm->state.enabled) {
		hisi_svb_pwm_enable(chip, pwm, state->enabled);
	}

	return 0;
}

static const struct pwm_ops hisi_svb_pwm_ops = {
	.get_state = hisi_svb_pwm_get_state,
	.apply = hisi_svb_pwm_apply,
	.owner = THIS_MODULE,
};

static int hisi_svb_pwm_probe(struct platform_device *pdev)
{
	struct hisi_svb_pwm_data *hisi_svb_pwm = NULL;
	struct resource *res = NULL;
	int ret = 0;

	hisi_svb_pwm = devm_kzalloc(&pdev->dev, sizeof(*hisi_svb_pwm), GFP_KERNEL);
	if (!hisi_svb_pwm) {
		return -ENOMEM;
	}

	hisi_svb_pwm->chip.ops = &hisi_svb_pwm_ops;
	hisi_svb_pwm->chip.dev = &pdev->dev;
	hisi_svb_pwm->chip.base = -1;
	hisi_svb_pwm->chip.npwm = 3;
	hisi_svb_pwm->chip.of_xlate = of_pwm_xlate_with_flags;
	hisi_svb_pwm->chip.of_pwm_n_cells = 3;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hisi_svb_pwm->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(hisi_svb_pwm->base))
		return PTR_ERR(hisi_svb_pwm->base);

	hisi_svb_pwm->clk = devm_clk_get(&pdev->dev, "svb_pwm");
	if (IS_ERR(hisi_svb_pwm->clk)) {
		dev_err(&pdev->dev, "getting clock failed with %ld\n",
				PTR_ERR(hisi_svb_pwm->clk));
		return PTR_ERR(hisi_svb_pwm->clk);
	}

	ret = clk_set_rate(hisi_svb_pwm->clk, 50000000);
	if (ret < 0)
		return ret;

	ret = clk_prepare_enable(hisi_svb_pwm->clk);
	if (ret < 0)
		return ret;

	hisi_svb_pwm->rstc = devm_reset_control_get_exclusive(&pdev->dev, "svb_pwm");
	if (IS_ERR(hisi_svb_pwm->rstc)) {
		clk_disable_unprepare(hisi_svb_pwm->clk);
		return PTR_ERR(hisi_svb_pwm->rstc);
	}

	ret = pwmchip_add(&hisi_svb_pwm->chip);
	if (ret < 0) {
		clk_disable_unprepare(hisi_svb_pwm->clk);
		return ret;
	}

	reset_control_assert(hisi_svb_pwm->rstc);
	msleep(30);
	reset_control_deassert(hisi_svb_pwm->rstc);

	platform_set_drvdata(pdev, hisi_svb_pwm);

	dev_info(&pdev->dev, "hisi svb pwm probe success\n");

	return 0;
}

static int hisi_svb_pwm_remove(struct platform_device *pdev)
{
	struct hisi_svb_pwm_data *hisi_svb_pwm = platform_get_drvdata(pdev);

	reset_control_assert(hisi_svb_pwm->rstc);
	msleep(30);
	reset_control_deassert(hisi_svb_pwm->rstc);

	clk_disable_unprepare(hisi_svb_pwm->clk);
	pwmchip_remove(&hisi_svb_pwm->chip);

	return 0;
}

static const struct of_device_id of_hisi_svb_pwm_match[] = {
	{
		.compatible = "hisi,svb_pwm",
	},
	{ /* end */ }
};
MODULE_DEVICE_TABLE(of, of_hisi_svb_pwm_match);

static struct platform_driver hisi_svb_pwm_driver = {
	.driver = {
		.name		= "hisi_svb_pwm",
		.of_match_table = of_hisi_svb_pwm_match,
	},
	.probe	= hisi_svb_pwm_probe,
	.remove	= hisi_svb_pwm_remove,
};
module_platform_driver(hisi_svb_pwm_driver);

MODULE_AUTHOR("<897420073@qq.com>");
MODULE_DESCRIPTION("Driver for hisi svb pwm");
MODULE_LICENSE("GPL and additional rights");
