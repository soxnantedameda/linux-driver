
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/thermal.h>
#include <linux/interrupt.h>

/* addr offset */
#define TSENSOR_CTRL0(chan) (0x0 + 0x100 * chan)
#define TSENSOR_CTRL1(chan) (0x4 + 0x100 * chan)
#define TSENSOR_CTRL2(chan) (0x8 + 0x100 * chan)
#define TSENSOR_CTRL3(chan) (0xc + 0x100 * chan)
#define TSENSOR_CTRL4(chan) (0x14 + 0x100 * chan)
#define TSENSOR_CTRL5(chan) (0x20 + 0x100 * chan)
#define TSENSOR_INT_MASK(chan) (0x20 + 0x100 * chan)
#define TSENSOR_INT_CLR(chan) (0x24 + 0x100 * chan)
#define TSENSOR_INT_RAW(chan) (0x28 + 0x100 * chan)
#define TSENSOR_INT_STAT(chan) (0x2c + 0x100 * chan)
/* TSENSOR_CTRL0 MASk */
#define TSENSOR_EN BIT(31)
#define TSENSOR_MONITOR_EN BIT(30)
#define TSENSOR_MONITOR_PERIOD GENMASK(27, 20)
#define TSENSOR_UPLIMIT GENMASK(19, 10)
#define TSENSOR_LOWLIMIT GENMASK(9, 0)
/* TSENSOR_CTRL1 MASk */
#define TSENSOR_TEMP_CT_SEL GENMASK(3, 2)
#define TSENSOR_LOW_WARNING BIT(1)
#define TSENSOR_UP_WARNING BIT(0)
/* TSENSOR_TEMP MASk */
#define TSENSOR_RESULT1 GENMASK(25, 16)
#define TSENSOR_RESULT0 GENMASK(9, 0)
/* TSENSOR_INT_MASK */
#define TSENSOR0_LOW_WARNING_INT_MASK BIT(1)
#define TSENSOR0_UP_WARNING_INT_MASK BIT(0)
/* TSENSOR_INT_CLR */
#define TSENSOR0_LOW_WARNING_INT_CLR BIT(1)
#define TSENSOR0_UP_WARNING_INT_CLR BIT(0)
/* TSENSOR_INT_RAW */
#define TSENSOR0_LOW_WARNING_INT_RAW BIT(1)
#define TSENSOR0_UP_WARNING_INT_RAW BIT(0)
/* TSENSOR_INT_STAT */
#define TSENSOR0_LOW_WARNING_INT BIT(1)
#define TSENSOR0_UP_WARNING_INT BIT(0)

#define TSENSOR_NUM 3

struct hisi_thermal_sensor {
	struct thermal_zone_device *tzd;
	void __iomem *regs;
	int irq;

	u8 id;
	u32 period;
	int uplimit;
	int lowlimit;
};

struct hisi_tsensor_data {
	struct platform_device *pdev;
	void __iomem *regs;

	struct hisi_thermal_sensor sensor[TSENSOR_NUM];
};

static void hisi_tsensor_set_bits(void __iomem *base, u32 offset,
					u32 mask, u32 data)
{
	void __iomem *address = base + offset;
	u32 value;

	value = readl(address);
	value &= ~mask;
	value |= (data & mask);
	writel(value, address);
}

static inline u32 hisi_tsensor_temp_to_reg(int temp)
{
	return ((temp + 40000) * 718) / 165 / 1000 + 146;
}

static inline int hisi_tsensor_reg_to_temp(u32 reg)
{
	return (((reg - 146) * 1000) * 165 / 718) - 40000;
}

static inline void hisi_tsensor_set_uplimit(struct hisi_thermal_sensor *sensor, int temp)
{
	u32 reg_val;

	reg_val = hisi_tsensor_temp_to_reg(temp);
	hisi_tsensor_set_bits(sensor->regs,
		TSENSOR_CTRL0(sensor->id), TSENSOR_UPLIMIT, reg_val << 10);
}

static inline void hisi_tsensor_set_lowlimit(struct hisi_thermal_sensor *sensor, int temp)
{
	u32 reg_val;

	reg_val = hisi_tsensor_temp_to_reg(temp);
	hisi_tsensor_set_bits(sensor->regs,
		TSENSOR_CTRL0(sensor->id), TSENSOR_LOWLIMIT, reg_val);
}

static inline void hisi_tsensor_alarm_clear(struct hisi_thermal_sensor *sensor, u32 value)
{
	writel(value, sensor->regs + TSENSOR_INT_CLR(sensor->id));
}

static inline void hisi_tsensor_alarm_enable(struct hisi_thermal_sensor *sensor, u32 value)
{
	writel(value, sensor->regs + TSENSOR_INT_MASK(sensor->id));
}

static void hisi_tsensor_disable(struct hisi_thermal_sensor *sensor)
{
	hisi_tsensor_set_bits(sensor->regs, TSENSOR_CTRL0(sensor->id), TSENSOR_EN, 0);
}

static void hisi_tsensor_enable(struct hisi_thermal_sensor *sensor)
{
	/* Set T-Sensor loop mode */
	hisi_tsensor_set_bits(sensor->regs, TSENSOR_CTRL0(sensor->id), TSENSOR_MONITOR_EN, BIT(30));
	/* Enable T-Sensor */
	hisi_tsensor_set_bits(sensor->regs, TSENSOR_CTRL0(sensor->id), TSENSOR_EN, BIT(31));
}

static int hisi_tsensor_get_temp(void *__data, int *temp)
{
	struct hisi_thermal_sensor *sensor = __data;
	u32 reg_val;

	reg_val = readl(sensor->regs + TSENSOR_CTRL2(sensor->id)) & TSENSOR_RESULT0;
	*temp = hisi_tsensor_reg_to_temp(reg_val);

	return 0;
}

static const struct thermal_zone_of_device_ops hisi_tsensor_ops = {
	.get_temp = hisi_tsensor_get_temp,
};

static irqreturn_t hisi_tsensor_alarm_irq_thread(int irq, void *dev)
{
	struct hisi_thermal_sensor *sensor = dev;
	int temp = 0;
	int reg;

	hisi_tsensor_get_temp(sensor, &temp);

	reg = readl(sensor->regs + TSENSOR_INT_RAW(sensor->id));
	if (reg & TSENSOR0_UP_WARNING_INT_RAW) {
		hisi_tsensor_alarm_enable(sensor, TSENSOR0_LOW_WARNING_INT_MASK);
		hisi_tsensor_alarm_clear(sensor, TSENSOR0_UP_WARNING_INT_CLR);
		thermal_zone_device_update(sensor->tzd, THERMAL_EVENT_UNSPECIFIED);
		dev_info(&sensor->tzd->device, "THERMAL UP WARNING: %d > %d\n",
			temp, sensor->uplimit);
	} else if (reg & TSENSOR0_LOW_WARNING_INT_RAW) {
		hisi_tsensor_alarm_enable(sensor, TSENSOR0_UP_WARNING_INT_MASK);
		hisi_tsensor_alarm_clear(sensor, TSENSOR0_LOW_WARNING_INT_CLR);
		dev_info(&sensor->tzd->device, "THERMAL LOW WARNING: %d < %d\n",
			temp, sensor->lowlimit);
	}

	return IRQ_HANDLED;
}

static int hisi_tsensor_probe(struct platform_device *pdev)
{
	struct hisi_tsensor_data *hisi_tsensor;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int uplimit, lowlimit;
	int ret;
	int i;

	hisi_tsensor = devm_kzalloc(dev, sizeof(*hisi_tsensor), GFP_KERNEL);
	if (!hisi_tsensor) {
		return -ENOMEM;
	}

	hisi_tsensor->pdev = pdev;
	platform_set_drvdata(pdev, hisi_tsensor);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hisi_tsensor->regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(hisi_tsensor->regs)) {
		dev_err(dev, "failed to get io address\n");
		return PTR_ERR(hisi_tsensor->regs);
	}

	ret = of_property_read_s32(dev->of_node, "uplimit", &uplimit);
	if (ret) {
		dev_dbg(dev, "failed to get uplimit, default to 80000\n");
		uplimit = 75000;
	}
	ret = of_property_read_s32(dev->of_node, "lowlimit", &lowlimit);
	if (ret) {
		dev_dbg(dev, "failed to get lowlimit, default to 5000\n");
		lowlimit = 5000;
	}

	for (i = 0; i < TSENSOR_NUM; i++) {
		hisi_tsensor->sensor[i].regs = hisi_tsensor->regs;
		hisi_tsensor->sensor[i].id = i;
		hisi_tsensor->sensor[i].tzd = devm_thermal_zone_of_sensor_register(dev,
			i, &hisi_tsensor->sensor[i], &hisi_tsensor_ops);
		hisi_tsensor->sensor[i].uplimit = uplimit;
		hisi_tsensor->sensor[i].lowlimit = lowlimit;

		if (IS_ERR(hisi_tsensor->sensor[i].tzd)) {
			dev_err(dev, "failed to register sensor id = %d, ret = %ld\n",
				i, PTR_ERR(hisi_tsensor->sensor[i].tzd));
			return PTR_ERR(hisi_tsensor->sensor[i].tzd);
		}

		hisi_tsensor->sensor[i].irq = platform_get_irq(pdev, i);
		if (hisi_tsensor->sensor[i].irq < 0)
			return hisi_tsensor->sensor[i].irq;

		hisi_tsensor_set_uplimit(&hisi_tsensor->sensor[i], hisi_tsensor->sensor[i].uplimit);
		hisi_tsensor_set_lowlimit(&hisi_tsensor->sensor[i], hisi_tsensor->sensor[i].lowlimit);

		ret = devm_request_threaded_irq(dev,
			hisi_tsensor->sensor[i].irq, NULL,
			hisi_tsensor_alarm_irq_thread,
			IRQF_ONESHOT, "hisi_tsensor",
			&hisi_tsensor->sensor[i]);
		if (ret < 0) {
			dev_err(dev, "failed to register irq for sensor %d, ret = %d\n", i, ret);
			return ret;
		}

		hisi_tsensor_alarm_clear(&hisi_tsensor->sensor[i], TSENSOR0_UP_WARNING_INT_CLR);
		hisi_tsensor_alarm_clear(&hisi_tsensor->sensor[i], TSENSOR0_LOW_WARNING_INT_CLR);
		hisi_tsensor_alarm_enable(&hisi_tsensor->sensor[i],
			TSENSOR0_UP_WARNING_INT_MASK |
			TSENSOR0_LOW_WARNING_INT_MASK);

		hisi_tsensor_enable(&hisi_tsensor->sensor[i]);
	}

	return 0;
}

static int hisi_tsensor_remove(struct platform_device *pdev)
{
	struct hisi_tsensor_data *hisi_tsensor = platform_get_drvdata(pdev);
	int i;

	for(i = 0; i < TSENSOR_NUM; i++) {
		hisi_tsensor_disable(&hisi_tsensor->sensor[i]);
	}

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int hisi_tsensor_suspend(struct device *dev)
{
	struct hisi_tsensor_data *hisi_tsensor = dev_get_drvdata(dev);
	int i;

	for(i = 0; i < TSENSOR_NUM; i++) {
		hisi_tsensor_disable(&hisi_tsensor->sensor[i]);
	}

	return 0;
}

static int hisi_tsensor_resume(struct device *dev)
{
	struct hisi_tsensor_data *hisi_tsensor = dev_get_drvdata(dev);
	int i;

	for(i = 0; i < TSENSOR_NUM; i++) {
		hisi_tsensor_enable(&hisi_tsensor->sensor[i]);
	}

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(hisi_tsensor_pm_ops,
			 hisi_tsensor_suspend, hisi_tsensor_resume);

static const struct of_device_id of_hisi_tsensor_match[] = {
	{
		.compatible = "hisi,tsensor",
	},
	{ /* end */ }
};
MODULE_DEVICE_TABLE(of, of_hisi_tsensor_match);

static struct platform_driver hisi_tsensor_driver = {
	.driver = {
		.name		= "hisi_tsensor",
		.pm		= &hisi_tsensor_pm_ops,
		.of_match_table = of_hisi_tsensor_match,
	},
	.probe	= hisi_tsensor_probe,
	.remove	= hisi_tsensor_remove,
};
module_platform_driver(hisi_tsensor_driver);

MODULE_AUTHOR("<897420073@qq.com>");
MODULE_DESCRIPTION("Driver for hisi Tsensor");
MODULE_LICENSE("GPL and additional rights");
