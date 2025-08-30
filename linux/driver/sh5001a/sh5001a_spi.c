#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/regmap.h>

#include "sh5001a.h"

static int sh5001a_spi_reg_read(void *context, unsigned int reg, unsigned int *val)
{
	struct spi_device *spi = to_spi_device(context);
	u16 head_buf = 0;
	u16 rx_buf = 0;
	int ret = 0;
	struct spi_transfer t[2] = {
		{
			.tx_buf = &head_buf,
			.len = 2,
		},
		{
			.tx_buf = &reg,
			.rx_buf = &rx_buf,
			.len = 2,
		},
	};

	head_buf = SH5001_SPI_REG_ACCESS << 8;
	head_buf = (reg > 0x7f) ? (head_buf | 0x01) : (head_buf | 0x00);
	reg |= 0x80;
	reg |= reg << 8;

	ret = spi_sync_transfer(spi, t, 2);
	if (ret) {
		dev_err(&spi->dev, "ret = %d, %s %d\n", ret, __func__, __LINE__);
	}
	*val = rx_buf & 0xff;

	return ret;
}

static int sh5001a_spi_reg_write(void *context, unsigned int reg, unsigned int val)
{
	struct spi_device *spi = to_spi_device(context);
	u16 head_buf = 0;
	u16 tx_buf = 0;
	int ret = 0;
	struct spi_transfer t[2] = {
		{
			.tx_buf = &head_buf,
			.len = 2,
		},
		{
			.tx_buf = &tx_buf,
			.len = 2,
		},
	};

	head_buf = SH5001_SPI_REG_ACCESS << 8;
	head_buf = (reg > 0x7f) ? (head_buf | 0x01) : (head_buf | 0x00);
	tx_buf = ((reg & 0x7f) << 8) | val;

	ret = spi_sync_transfer(spi, t, 2);
	if (ret) {
		dev_err(&spi->dev, "ret = %d, %s %d\n", ret, __func__, __LINE__);
	}

	return ret;
}

static const struct regmap_config sh5001a_spi_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.reg_read = sh5001a_spi_reg_read,
	.reg_write = sh5001a_spi_reg_write,
};

static int sh5001a_spi_probe(struct spi_device *spi)
{
	struct regmap *regmap;
	int ret = 0;

	spi->bits_per_word = 16;
	ret = spi_setup(spi);
	if (ret) {
		dev_err(&spi->dev, "set spi failed, ret = %d\n", ret);
		return ret;
	}

	regmap = devm_regmap_init(&spi->dev, NULL, &spi->dev, &sh5001a_spi_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(&spi->dev, "Failed to register spi regmap %d\n",
			(int)PTR_ERR(regmap));
		return PTR_ERR(regmap);
	}

	return sh5001a_probe(&spi->dev, regmap);
}

static int sh5001a_spi_remove(struct spi_device *spi)
{
	return sh5001a_remove(&spi->dev);
}

static void sh5001a_spi_shutdown(struct spi_device *spi)
{
	sh5001a_shutdown(&spi->dev);
}

static const struct of_device_id sh5001a_spi_of_match[] = {
	{
		.compatible = "sh5001a",
	},
	{}
};
MODULE_DEVICE_TABLE(of, sh5001a_spi_of_match);

static struct spi_driver sh5001a_spi_driver = {
	.driver = {
		.name = "sh5001a_spi",
		//.pm = &sh5001a_pm_ops,
		.of_match_table = of_match_ptr(sh5001a_spi_of_match),
	},
	.probe = sh5001a_spi_probe,
	.remove = sh5001a_spi_remove,
	.shutdown = sh5001a_spi_shutdown,
};
module_spi_driver(sh5001a_spi_driver);

MODULE_AUTHOR("<zhiwen.liang@hollyland-tech.com>");
MODULE_DESCRIPTION("Driver for sh5001a");
MODULE_LICENSE("GPL and additional rights");
