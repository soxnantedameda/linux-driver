
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/regmap.h>

#define AW87559_ID 0x00
#define AW87559_SYSCTRL 0x01
#define AW87559_BATSAFE 0x02
#define AW87559_BSTOVR 0x03
#define AW87559_BSTCPR2 0x05
#define AW87559_PAGR 0x06
#define AW87559_PAGC3OPR 0x07
#define AW87559_PAGC3PR 0x08
#define AW87559_PAGC2OPR 0x09
#define AW87559_PAGC2PR 0x0A
#define AW87559_PAGC1PR 0x0B
#define AW87559_ADP_MODE 0x0C
#define AW87559_ADPBST_TIME1 0x0D
#define AW87559_ADPBST_TIME2 0x0E
#define AW87559_ADPBST_VTH 0x0F

struct aw87559_priv {
	struct regmap *regmap;
	struct i2c_client *i2c;
	struct regulator *supply;
	struct gpio_desc *reset_gpiod;
};

static const struct regmap_config aw87559_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x0F,
	.cache_type = REGCACHE_RBTREE,
};

static ssize_t reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct aw87559_priv *aw87559 = dev_get_drvdata(dev);
	unsigned int reg_val = 0;
	unsigned int i = 0;
	ssize_t len = 0;

	for (i = 0; i <= AW87559_ADPBST_VTH; i++) {
		regmap_read(aw87559->regmap, i, &reg_val);
		len += snprintf(buf + len, PAGE_SIZE - len, "reg:0x%02x=0x%02x\n", i, reg_val);
	}

	return len;
}

static ssize_t reg_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t len)
{
	unsigned int databuf[2];
	struct aw87559_priv *aw87559 = dev_get_drvdata(dev);

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2)
		regmap_write(aw87559->regmap, databuf[0], databuf[1]);

	return len;
}
static DEVICE_ATTR_RW(reg);

/* add your attr in here*/
static struct attribute *aw87559_attributes[] = {
	&dev_attr_reg.attr,
	NULL
};

static struct attribute_group aw87559_attribute_group = {
	.attrs = aw87559_attributes
};


static int aw87559_check_id(struct aw87559_priv *aw87559)
{
	u8 id_expected = 0x5a;
	u8 id = 0;

	id = i2c_smbus_read_byte_data(aw87559->i2c, AW87559_ID);

	if (memcmp(&id, &id_expected, sizeof(u8))) {
		dev_err(&aw87559->i2c->dev, "chip id mismatch\n");
		return -ENODEV;
	}


	return 0;
}

static void aw87559_reset(struct aw87559_priv *aw87559, bool en)
{
	if (aw87559->reset_gpiod) {
		if (en) {
			gpiod_direction_output(aw87559->reset_gpiod, 1);
		} else {
			gpiod_direction_output(aw87559->reset_gpiod, 0);
		}
	}

	mdelay(1);
}

static int aw87559_init_speaker(struct aw87559_priv *aw87559)
{
	regmap_write(aw87559->regmap, AW87559_SYSCTRL, 0x7c);
	regmap_write(aw87559->regmap, AW87559_BATSAFE, 0x09);
	regmap_write(aw87559->regmap, AW87559_BSTOVR, 0x0A);
	regmap_write(aw87559->regmap, AW87559_BSTCPR2, 0x08);
	regmap_write(aw87559->regmap, AW87559_PAGR, 0x0);
	regmap_write(aw87559->regmap, AW87559_PAGC3OPR, 0x93);
	regmap_write(aw87559->regmap, AW87559_PAGC3PR, 0x4E);
	regmap_write(aw87559->regmap, AW87559_PAGC2OPR, 0x0B);
	regmap_write(aw87559->regmap, AW87559_PAGC2PR, 0x08);
	regmap_write(aw87559->regmap, AW87559_PAGC1PR, 0x4B);
	regmap_write(aw87559->regmap, AW87559_ADP_MODE, 0x0);
	regmap_write(aw87559->regmap, AW87559_ADPBST_TIME1, 0x77);
	regmap_write(aw87559->regmap, AW87559_ADPBST_VTH, 0x51);

	return 0;
}

static int aw87559_deinit_speaker(struct aw87559_priv *aw87559)
{
	return 0;
}

static int aw87559_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct aw87559_priv *aw87559 = NULL;
	struct device *dev = &client->dev;
	int ret;

	aw87559 = devm_kzalloc(dev, sizeof(*aw87559), GFP_KERNEL);
	if (!aw87559) {
		dev_err(&client->dev, "Failed to malloc data for aw87559\n");
		return -ENOMEM;
	}

	aw87559->reset_gpiod = devm_gpiod_get_optional(dev, "aw87559,reset", GPIOD_OUT_LOW);
	if (IS_ERR(aw87559->reset_gpiod)) {
		dev_warn(dev, "Failed to get enable gpio, disable force power up control\n");
		return -ENODEV;
	}

	aw87559->i2c = client;
	i2c_set_clientdata(client, aw87559);

	aw87559->regmap = devm_regmap_init_i2c(client, &aw87559_regmap_config);
	if (IS_ERR(aw87559->regmap)) {
		ret = PTR_ERR(aw87559->regmap);
		dev_err(dev, "Failed to allocate register map: %d\n", ret);
		return ret;
	}

	/* enable aw87559 */
	aw87559_reset(aw87559, true);
	ret = aw87559_check_id(aw87559);
	if (ret)
		return ret;

	aw87559_init_speaker(aw87559);

	ret = devm_device_add_group(&client->dev, &aw87559_attribute_group);
	if (ret) {
		dev_err(dev, "Failed to add group attr for aw87559\n");
		return ret;
	}

	dev_info(dev, "aw87559 probe success\n");

	return 0;
}

static int aw87559_i2c_remove(struct i2c_client *client)
{
	struct aw87559_priv *aw87559 = i2c_get_clientdata(client);

	aw87559_deinit_speaker(aw87559);
	aw87559_reset(aw87559, false);

	return 0;
}

static const struct of_device_id aw87559_dt_match[] = {
	{.compatible = "aw87559",},
	{},
};
MODULE_DEVICE_TABLE(of, aw87559_dt_match);

static const struct i2c_device_id aw87559_ids[] = {
	{"aw87559", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, aw87559_ids);

static struct i2c_driver aw87559_i2c_driver = {
	.probe = aw87559_i2c_probe,
	.remove = aw87559_i2c_remove,
	.driver = {
		.name = "aw87559_i2c_driver",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(aw87559_dt_match),
	},
	.id_table = aw87559_ids,
};
module_i2c_driver(aw87559_i2c_driver);

MODULE_AUTHOR("<897420073@qq.com>");
MODULE_DESCRIPTION("Driver for aw87559 speaker");
MODULE_LICENSE("GPL and additional rights");

