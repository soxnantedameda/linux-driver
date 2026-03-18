#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>

#include "qma6101t.h"

static int qma6101t_regmap_spi_write(void *context, const void *data, size_t count)
{
	struct spi_device *spi = context;

	/* Write register is same as generic SPI */
	return spi_write(spi, data, count);
}

static int qma6101t_regmap_spi_read(void *context, const void *reg,
				size_t reg_size, void *val, size_t val_size)
{
	struct spi_device *spi = context;
	u8 addr[2];

	addr[0] = *(u8 *)reg;
	addr[0] |= BIT(7); /* Set RW = '1' */
	addr[1] = 0; /* Read requires a dummy byte transfer */

	return spi_write_then_read(spi, addr, sizeof(addr), val, val_size);
}

const struct regmap_config qma6101t_regmap_conf = {
	.reg_bits = 8,
	.val_bits = 8,
	.cache_type = REGCACHE_RBTREE,
};

static struct regmap_bus qma6101t_regmap_bus = {
	.write = qma6101t_regmap_spi_write,
	.read = qma6101t_regmap_spi_read,
};

static int qma6101t_spi_probe(struct spi_device *spi)
{
	struct regmap *regmap;

	regmap = devm_regmap_init(&spi->dev, &qma6101t_regmap_bus,
			spi, &qma6101t_regmap_conf);

	if (IS_ERR(regmap)) {
		dev_err(&spi->dev, "Failed to initialize spi regmap\n");
		return PTR_ERR(regmap);
	}

	return qma6101t_probe(&spi->dev, regmap);
}

static void qma6101t_spi_remove(struct spi_device *spi)
{
	qma6101t_remove(&spi->dev);
}

void qma6101t_spi_shutdown(struct spi_device *spi)
{
	qma6101t_shutdown(&spi->dev);
}

static const struct of_device_id qma6101t_of_match[] = {
	{ .compatible = "qma6101t" },
	{}
};
MODULE_DEVICE_TABLE(of, qma6101t_of_match);

static DEFINE_SIMPLE_DEV_PM_OPS(qma6101t_pm_ops, qma6101t_suspend,
				qma6101t_resume);

static struct spi_driver qma6101t_driver = {
	.driver = {
		.name	= "qma6101t_spi",
		.pm	= pm_ptr(&qma6101t_pm_ops),
		.of_match_table = qma6101t_of_match,
	},
	.probe		= qma6101t_spi_probe,
	.remove		= qma6101t_spi_remove,
	.shutdown	= qma6101t_spi_remove,
};
module_spi_driver(qma6101t_driver);

MODULE_AUTHOR("<897420073@qq.com>");
MODULE_DESCRIPTION("Driver for qma6101t");
MODULE_LICENSE("GPL");
