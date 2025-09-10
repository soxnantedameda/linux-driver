#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/i2c.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regmap.h>

#define AW37004_MAX_REGULATOR 4

#define AW37004_ID_NUM 2
#define AW37004_ID0 0x00
#define AW37004_ID2 0x19

#define AW37004_CLMTCR 0x01
#define AW37004_AVDD2_CLMT_MASK GENMASK(7, 6)
#define AW37004_AVDD1_CLMT_MASK GENMASK(5, 4)
#define AW37004_DVDD2_CLMT_MASK GENMASK(3, 2)
#define AW37004_DVDD1_CLMT_MASK GENMASK(1, 0)

#define AW37004_AVDD2_ILIM_SHIFT 6
#define AW37004_AVDD1_ILIM_SHIFT 4
#define AW37004_DVDD2_ILIM_SHIFT 2
#define AW37004_DVDD1_ILIM_SHIFT 0

#define AW37004_DISCR 0x02
#define AW37004_DISCHG_MD_MASK BIT(7)
#define AW37004_DISCHG_MD_EN AW37004_DISCHG_MD_MASK

#define AW37004_AVDD2_DISCHG_MASK BIT(3)
#define AW37004_AVDD1_DISCHG_MASK BIT(2)
#define AW37004_DVDD2_DISCHG_MASK BIT(1)
#define AW37004_DVDD1_DISCHG_MASK BIT(0)

#define AW37004_AVDD2_DISCHG_EN AW37004_AVDD2_DISCHG_MASK
#define AW37004_AVDD1_DISCHG_EN AW37004_AVDD1_DISCHG_MASK
#define AW37004_DVDD2_DISCHG_EN AW37004_DVDD2_DISCHG_MASK
#define AW37004_DVDD1_DISCHG_EN AW37004_DVDD1_DISCHG_MASK

#define AW37004_DVDD1_VOUT 0x03
#define AW37004_DVDD2_VOUT 0x04
#define AW37004_AVDD1_VOUT 0x05
#define AW37004_AVDD2_VOUT 0x06

#define AW37004_DVDD_SEQ 0x0A
#define AW37004_AVDD_SEQ 0x0B

#define AW37004_ENCR 0x0E
#define AW37004_AVDD2_EN_MASK BIT(3)
#define AW37004_AVDD1_EN_MASK BIT(2)
#define AW37004_DVDD2_EN_MASK BIT(1)
#define AW37004_DVDD1_EN_MASK BIT(0)

#define AW37004_AVDD2_EN AW37004_AVDD2_EN_MASK
#define AW37004_AVDD1_EN AW37004_AVDD1_EN_MASK
#define AW37004_DVDD2_EN AW37004_DVDD2_EN_MASK
#define AW37004_DVDD1_EN AW37004_DVDD1_EN_MASK

#define AW37004_SEQCR 0x0F

enum {
	AW37004_DVDD1_ID,
	AW37004_DVDD2_ID,
	AW37004_AVDD1_ID,
	AW37004_AVDD2_ID,
};

struct aw37004_data {
	struct i2c_client *client;
	struct regmap *regmap;
	struct regulator_dev *rdev[AW37004_MAX_REGULATOR];
	struct gpio_desc *enable_gpiod;
};

struct aw37004_regulator {
	const struct regulator_desc desc;
	/* Current limiting */
	unsigned int n_current_limits;
	const int *current_limits;
	unsigned int limit_reg;
	unsigned int limit_mask;
	unsigned int limit_shift;
};

static const struct regmap_config aw37004_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int aw37004_set_current_limit(struct regulator_dev *rdev,
		int min, int max)
{
	struct aw37004_regulator *info = rdev_get_drvdata(rdev);
	int i;

	/* search for closest to maximum */
	for (i = 0; i < info->n_current_limits; i++) {
		if (min <= info->current_limits[i]
			&& max >= info->current_limits[i]) {
			return regmap_update_bits(rdev->regmap,
				info->limit_reg,
				info->limit_mask,
				i << info->limit_shift);
		}
	}

	return -EINVAL;
}

static int aw37004_get_current_limit(struct regulator_dev *rdev)
{
	struct aw37004_regulator *info = rdev_get_drvdata(rdev);
	unsigned int data;
	int ret;

	ret = regmap_read(rdev->regmap, info->limit_reg, &data);
	if (ret < 0)
		return ret;

	data = (data & info->limit_mask) >> info->limit_shift;

	return info->current_limits[data];
}

static const struct regulator_ops aw37004_regulator_ops = {
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.list_voltage = regulator_list_voltage_linear,
	.set_current_limit = aw37004_set_current_limit,
	.get_current_limit = aw37004_get_current_limit,
	.set_active_discharge = regulator_set_active_discharge_regmap,
};

static const unsigned int aw37004_dvdd_limits[] = {
	1300000, 1440000, 1580000, 1720000
};

static const unsigned int aw37004_avdd_limits[] = {
	480000, 620000, 760000, 900000
};

static const struct regulator_init_data aw37004_dvdd_init_data = {
	.constraints = {
		.min_uV = 600000,
		.max_uV = 2130000,
		.min_uA = 1300000,
		.max_uA = 1720000,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE |
			REGULATOR_CHANGE_CURRENT |
			REGULATOR_CHANGE_STATUS,
	},
};

static const struct regulator_init_data aw37004_avdd_init_data = {
	.constraints = {
		.min_uV = 1200000,
		.max_uV = 4387500,
		.min_uA = 480000,
		.max_uA = 900000,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE |
			REGULATOR_CHANGE_CURRENT |
			REGULATOR_CHANGE_STATUS,
	},
};

#define AW37004_LDO(ldo_name, match, min, step, max, limits_array) \
{ \
	.desc = { \
		.id = AW37004_##ldo_name##_ID, \
		.name = of_match_ptr(match), \
		.type = REGULATOR_VOLTAGE, \
		.owner = THIS_MODULE, \
		.of_match = of_match_ptr(match), \
		.ops = &aw37004_regulator_ops, \
		.n_voltages = ((max) - (min))/(step) + 1, \
		.min_uV = min, \
		.uV_step = step, \
		.vsel_reg = AW37004_##ldo_name##_VOUT, \
		.vsel_mask = 0xff, \
		.enable_reg = AW37004_ENCR, \
		.enable_mask = AW37004_##ldo_name##_EN_MASK, \
		.enable_val = AW37004_##ldo_name##_EN, \
		.disable_val = 0, \
		.active_discharge_on = AW37004_##ldo_name##_DISCHG_EN, \
		.active_discharge_off = 0, \
		.active_discharge_mask = AW37004_##ldo_name##_DISCHG_MASK, \
		.active_discharge_reg = AW37004_DISCR, \
	}, \
	.current_limits = limits_array, \
	.n_current_limits = ARRAY_SIZE(limits_array), \
	.limit_reg = AW37004_CLMTCR, \
	.limit_mask = AW37004_##ldo_name##_CLMT_MASK, \
	.limit_shift = AW37004_##ldo_name##_ILIM_SHIFT, \
}

static const struct aw37004_regulator aw37004_ldo[] = {
	AW37004_LDO(DVDD1, "dvdd1", 600000, 6000, 2130000, aw37004_dvdd_limits),
	AW37004_LDO(DVDD2, "dvdd2", 600000, 6000, 2130000, aw37004_dvdd_limits),
	AW37004_LDO(AVDD1, "avdd1", 1200000, 12500, 4387500, aw37004_avdd_limits),
	AW37004_LDO(AVDD2, "avdd2", 1200000, 12500, 4387500, aw37004_avdd_limits),
};

static int aw37004_check_id(struct i2c_client *client)
{
	u8 id_expected[AW37004_ID_NUM] = {0x00, 0x04};
	u8 id[AW37004_ID_NUM] = {0};

	id[0] = (u8)i2c_smbus_read_byte_data(client, AW37004_ID0);
	id[1] = (u8)i2c_smbus_read_byte_data(client, AW37004_ID2);

	if (memcmp(id, id_expected, sizeof(u8) * AW37004_ID_NUM)) {
		dev_err(&client->dev, "chip id mismatch\n");
		return -ENODEV;
	}

	return 0;
}

static ssize_t manuel_discharge_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct aw37004_data *aw37004 = dev_get_drvdata(dev);
	u8 value =  0;

	regmap_read(aw37004->regmap, AW37004_DISCR, (u32 *)&value);
	value &= AW37004_DISCHG_MD_MASK;

	return sprintf(buf, "%d\n", !!value);
}

static ssize_t manuel_discharge_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct aw37004_data *aw37004 = dev_get_drvdata(dev);
	bool manuel_discharge;
	int rc;

	rc = kstrtobool(buf, &manuel_discharge);
	if (rc)
		return rc;

	regmap_update_bits(aw37004->regmap, AW37004_DISCR, AW37004_DISCHG_MD_MASK,
			manuel_discharge ? AW37004_DISCHG_MD_EN : 0);

	return count;
}
static DEVICE_ATTR_RW(manuel_discharge);

static ssize_t force_enable_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct aw37004_data *aw37004 = dev_get_drvdata(dev);
	bool force_enable = false;

	if (aw37004->enable_gpiod)
		force_enable = !!gpiod_get_value_cansleep(aw37004->enable_gpiod);

	return sprintf(buf, "%d\n", force_enable);
}

static ssize_t force_enable_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct aw37004_data *aw37004 = dev_get_drvdata(dev);
	bool force_enable = false;
	int rc;

	rc = kstrtobool(buf, &force_enable);
	if (rc)
		return rc;

	if (aw37004->enable_gpiod)
		gpiod_set_value_cansleep(aw37004->enable_gpiod, !!force_enable);

	return count;
}
static DEVICE_ATTR_RW(force_enable);

static ssize_t reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct aw37004_data *aw37004 = dev_get_drvdata(dev);
	unsigned int reg_val = 0;
	unsigned int i = 0;
	ssize_t len = 0;

	for (i = 0; i <= 0xF; i++) {
		regmap_read(aw37004->regmap, i, &reg_val);
		len += snprintf(buf + len, PAGE_SIZE - len, "reg:0x%02x=0x%02x\n", i, reg_val);
	}

	return len;
}

static ssize_t reg_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t len)
{
	unsigned int databuf[2];
	struct aw37004_data *aw37004 = dev_get_drvdata(dev);

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2)
		regmap_write(aw37004->regmap, databuf[0], databuf[1]);

	return len;
}
static DEVICE_ATTR_RW(reg);

/* add your attr in here*/
static struct attribute *aw37004_attributes[] = {
	&dev_attr_force_enable.attr,
	&dev_attr_manuel_discharge.attr,
	&dev_attr_reg.attr,
	NULL
};

static struct attribute_group aw37004_attribute_group = {
	.attrs = aw37004_attributes
};

static int aw37004_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct aw37004_data *aw37004 = NULL;
	struct device *dev = &client->dev;
	struct regulator_config config = {0};
	int ret = 0;
	int i;
	bool manuel_discharge;

	aw37004 = devm_kzalloc(&client->dev, sizeof(*aw37004), GFP_KERNEL);
	if (!aw37004) {
		dev_err(&client->dev, "Failed to malloc data for aw37004\n");
		return -ENOMEM;
	}
	aw37004->client = client;
	i2c_set_clientdata(client, aw37004);

	aw37004->regmap = devm_regmap_init_i2c(client, &aw37004_regmap_config);
	if (IS_ERR(aw37004->regmap)) {
		ret = PTR_ERR(aw37004->regmap);
		dev_err(dev, "Failed to allocate register map: %d\n", ret);
		return ret;
	}

	ret = aw37004_check_id(client);
	if (ret < 0) {
		return ret;
	}

	aw37004->enable_gpiod = devm_gpiod_get_optional(dev, "aw37004,enable", GPIOD_OUT_LOW);
	if (IS_ERR(aw37004->enable_gpiod)) {
		dev_warn(dev, "Failed to get enable gpio, disable force power up control\n");
	}

	manuel_discharge = of_property_read_bool(dev->of_node, "manuel-discharge");
	if (manuel_discharge)
		regmap_update_bits(aw37004->regmap, AW37004_DISCR,
			AW37004_DISCHG_MD_MASK, AW37004_DISCHG_MD_EN);

	config.dev = dev;
	config.regmap = aw37004->regmap;
	config.of_node = dev->of_node;
	for (i = 0; i < AW37004_MAX_REGULATOR; i++) {
		(i < 2) ? (config.init_data = &aw37004_dvdd_init_data) :
			(config.init_data = &aw37004_avdd_init_data);
		config.driver_data = (void *)&aw37004_ldo[i];
		aw37004->rdev[i] = devm_regulator_register(dev, &aw37004_ldo[i].desc, &config);
		if (IS_ERR(aw37004->rdev[i])) {
			dev_err(dev, "Failed to register aw37004 %s regulator\n",
				aw37004_ldo[i].desc.name);
			return PTR_ERR(aw37004->rdev[i]);
		}
	}

	ret = devm_device_add_group(&client->dev, &aw37004_attribute_group);
	if (ret) {
		dev_err(dev, "Failed to add group attr for aw37004\n");
		return ret;
	}

	dev_info(dev, "aw37004 probe success\n");

	return 0;
}

static int aw37004_remove(struct i2c_client *client)
{
	struct aw37004_data *aw37004 = i2c_get_clientdata(client);
	int i;

	for (i = 0; i < AW37004_MAX_REGULATOR; i++) {
		regulator_disable_regmap(aw37004->rdev[i]);
	}

	return 0;
}

static void aw37004_shutdown(struct i2c_client *client)
{
	struct aw37004_data *aw37004 = i2c_get_clientdata(client);
	int i;

	for (i = 0; i < AW37004_MAX_REGULATOR; i++) {
		regulator_disable_regmap(aw37004->rdev[i]);
	}
}

static const struct of_device_id aw37004_dt_match[] = {
	{.compatible = "aw37004", },
	{},
};
MODULE_DEVICE_TABLE(of, aw37004_dt_match);

static const struct i2c_device_id aw37004_ids[] = {
	{"aw37004", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, aw37004_ids);

static struct i2c_driver aw37004_i2c_driver = {
	.probe = aw37004_probe,
	.remove = aw37004_remove,
	.shutdown = aw37004_shutdown,
	.driver = {
		.name = "aw37004_i2c_driver",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(aw37004_dt_match),
	},
	.id_table = aw37004_ids,
};
module_i2c_driver(aw37004_i2c_driver);

MODULE_AUTHOR("<897420073@qq.com>");
MODULE_DESCRIPTION("Driver for aw37004 ldo");
MODULE_LICENSE("GPL and additional rights");
