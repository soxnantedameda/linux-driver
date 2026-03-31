#include <linux/bitfield.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/gpio/consumer.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/trigger.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/buffer.h>

#define MS5110S_DRV_NAME "ms5110s"

/* CONFIG BIT */
#define MS5110S_ST_DRDY BIT(7)
#define MS5110S_SC	BIT(4)
#define MS5110S_DR1	BIT(3)
#define MS5110S_DR0	BIT(2)
#define MS5110S_PGA1	BIT(1)
#define MS5110S_PGA0	BIT(0)

#define MS5110S_DR_MASK		GENMASK(3, 2)
#define MS5110S_240SPS_12BIT	0
#define MS5110S_60SPS_14BIT	MS5110S_DR0
#define MS5110S_30SPS_15BIT	MS5110S_DR1
#define MS5110S_15SPS_16BIT	MS5110S_DR0 | MS5110S_DR1

#define MS5110S_PGA_MASK	GENMASK(1, 0)
#define MS5110S_GAIN_1X		0
#define MS5110S_GAIN_2X		MS5110S_PGA0
#define MS5110S_GAIN_4X		MS5110S_PGA1
#define MS5110S_GAIN_8X		MS5110S_PGA0 | MS5110S_PGA1

struct ms5110s_data {
	struct i2c_client *client;
	struct mutex lock;

	u8 sample_scale_idx;
	u8 gain_idx;
	u8 sign_bit;
	u8 mode;
};

static const struct iio_chan_spec ms5110s_channels[] = {
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
			BIT(IIO_CHAN_INFO_SCALE) |
			BIT(IIO_CHAN_INFO_SAMP_FREQ) |
			BIT(IIO_CHAN_INFO_HARDWAREGAIN),
		.scan_type = {
			.sign = 's',
			.realbits = 16,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_CPU,
		},
	},
	IIO_CHAN_SOFT_TIMESTAMP(1)
};

static int ms5110s_read(struct ms5110s_data *ms5110s, void *val, u8 len)
{
	struct i2c_msg msgs;
	int ret;

	if (len > 3)
		return -EINVAL;

	msgs.addr = ms5110s->client->addr;
	msgs.flags = I2C_M_RD;
	msgs.len = len;
	msgs.buf = val;

	ret = i2c_transfer(ms5110s->client->adapter, &msgs, 1);
	if (ret != 1)
		return -EIO;

	return 0;
}

static int ms5110s_write(struct ms5110s_data *ms5110s, s8 val)
{
	return i2c_master_send(ms5110s->client, &val, 1);
}

static int ms5110s_update_bit(struct ms5110s_data *ms5110s, u8 mask, u8 val)
{
	u32 orig;
	u32 tmp;
	int ret;

	ms5110s_read(ms5110s, &orig, 3);
	orig = (orig >> 16) & 0xff;
	tmp = orig & ~mask;
	tmp |= val & mask;

	ret = ms5110s_write(ms5110s, (u8)tmp);
	if (ret < 0)
		return ret;

	return 0;
}

static const struct {
	u8 reg_val;
	u16 gain;
} ms5110s_gain_table[] = {
	{MS5110S_GAIN_1X, 1},
	{MS5110S_GAIN_2X, 2},
	{MS5110S_GAIN_4X, 4},
	{MS5110S_GAIN_8X, 8},
};

static IIO_CONST_ATTR(in_voltage_hardwaregain_available, "8 4 2 1");

static int ms5110s_set_gain(struct ms5110s_data *ms5110s, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ms5110s_gain_table); i++) {
		if (ms5110s_gain_table[i].gain == val) {
			ms5110s->gain_idx = i;

			return ms5110s_update_bit(ms5110s,
				MS5110S_PGA_MASK,
				ms5110s_gain_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int ms5110s_get_gain(struct ms5110s_data *ms5110s, int *val)
{
	int reg_val;
	int i;

	ms5110s_read(ms5110s, &reg_val, 3);
	reg_val = (reg_val >> 16) & MS5110S_PGA_MASK;
	for (i = 0; i < ARRAY_SIZE(ms5110s_gain_table); i++) {
		if (reg_val == ms5110s_gain_table[i].reg_val) {
			*val = ms5110s_gain_table[i].gain;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static const struct {
	u8 reg_val;
	u8 sample;
	u8 scale;
} ms5110s_sample_scale_table[] = {
	{MS5110S_240SPS_12BIT, 240, 12},
	{MS5110S_60SPS_14BIT, 60, 14},
	{MS5110S_30SPS_15BIT, 30, 15},
	{MS5110S_15SPS_16BIT, 15, 16},
};

static IIO_CONST_ATTR(in_voltage_sampling_frequency_available, "240 60 30 15");
static IIO_CONST_ATTR(in_voltage_scale_available, "12 14 15 16");

static int ms5110s_set_sample(struct ms5110s_data *ms5110s, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ms5110s_sample_scale_table); i++) {
		if (ms5110s_sample_scale_table[i].sample == val) {
			ms5110s->sample_scale_idx = i;

			return ms5110s_update_bit(ms5110s,
				MS5110S_DR_MASK,
				ms5110s_sample_scale_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int ms5110s_get_sample(struct ms5110s_data *ms5110s, int *val)
{
	int reg_val;
	int i;

	ms5110s_read(ms5110s, &reg_val, 3);
	reg_val = (reg_val >> 16) & MS5110S_DR_MASK;
	for (i = 0; i < ARRAY_SIZE(ms5110s_sample_scale_table); i++) {
		if (reg_val == ms5110s_sample_scale_table[i].reg_val) {
			*val = ms5110s_sample_scale_table[i].sample;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static int ms5110s_set_scale(struct ms5110s_data *ms5110s, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ms5110s_sample_scale_table); i++) {
		if (ms5110s_sample_scale_table[i].scale == val) {
			ms5110s->sample_scale_idx = i;

			return ms5110s_update_bit(ms5110s,
				MS5110S_DR_MASK,
				ms5110s_sample_scale_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int ms5110s_get_scale(struct ms5110s_data *ms5110s, int *val)
{
	int reg_val;
	int i;

	ms5110s_read(ms5110s, &reg_val, 3);
	reg_val = (reg_val >> 16) & MS5110S_DR_MASK;
	for (i = 0; i < ARRAY_SIZE(ms5110s_sample_scale_table); i++) {
		if (reg_val == ms5110s_sample_scale_table[i].reg_val) {
			*val = ms5110s_sample_scale_table[i].scale;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static ssize_t mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct ms5110s_data *ms5110s = iio_priv(indio_dev);
	ssize_t len = 0;
	int reg_val;

	mutex_lock(&ms5110s->lock);
	ms5110s_read(ms5110s, &reg_val ,3);
	reg_val = (reg_val >> 16) & MS5110S_SC;
	mutex_unlock(&ms5110s->lock);

	if (reg_val) {
		len = snprintf(buf, PAGE_SIZE - len, "single\n");
	} else {
		len = snprintf(buf, PAGE_SIZE - len, "continue\n");
	}

	return len;
}

static IIO_CONST_ATTR(mode_available, "single continue");

static ssize_t mode_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t len)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct ms5110s_data *ms5110s = iio_priv(indio_dev);

	mutex_lock(&ms5110s->lock);
	if (!strncmp(buf, "single", 6)) {
		ms5110s_update_bit(ms5110s, MS5110S_SC, 0xff);
		ms5110s->mode = 1;
	} else if (!strncmp(buf, "continue", 8)) {
		ms5110s_update_bit(ms5110s, MS5110S_SC, 0);
		ms5110s->mode = 0;
	} else {
		dev_err(&ms5110s->client->dev, "Invalid mode\n");
	}
	mutex_unlock(&ms5110s->lock);

	return len;
}

static IIO_DEVICE_ATTR_RW(mode,   0);

static ssize_t reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct ms5110s_data *ms5110s = iio_priv(indio_dev);
	u32 reg_val = 0;
	ssize_t len = 0;

	mutex_lock(&ms5110s->lock);
	ms5110s_read(ms5110s, &reg_val, 3);
	len += snprintf(buf + len, PAGE_SIZE - len, "0x%02x\n", reg_val);
	mutex_unlock(&ms5110s->lock);

	return len;
}

static ssize_t reg_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t len)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct ms5110s_data *ms5110s = iio_priv(indio_dev);
	u32 databuf;

	mutex_lock(&ms5110s->lock);
	if (sscanf(buf, "%x", &databuf) == 1)
		ms5110s_write(ms5110s, databuf);
	mutex_unlock(&ms5110s->lock);

	return len;
}

static IIO_DEVICE_ATTR_RW(reg,   0);

static struct attribute *ms5110s_attributes[] = {
	&iio_const_attr_in_voltage_hardwaregain_available.dev_attr.attr,
	&iio_const_attr_in_voltage_sampling_frequency_available.dev_attr.attr,
	&iio_const_attr_in_voltage_scale_available.dev_attr.attr,
	&iio_const_attr_mode_available.dev_attr.attr,
	&iio_dev_attr_mode.dev_attr.attr,
	&iio_dev_attr_reg.dev_attr.attr,
	NULL,
};

static const struct attribute_group ms5110s_attribute_group = {
	.attrs = ms5110s_attributes,
};

static s16 com2ori_s16(u16 val, int nbit)
{
	u16 mask = GENMASK(nbit, 0);
	u16 sign = BIT(nbit) & val;
	u16 uval = val & mask;

	if (sign == 0) {
		return uval;
	} else {
		uval = (~uval & mask) + 1;
		return -uval;
	}
}

static int ms5110s_read_data(struct ms5110s_data *ms5110s, s16 *data)
{
	u16 tmp;
	int sign_bit = ms5110s_sample_scale_table[ms5110s->sample_scale_idx].scale - 1;

	ms5110s_read(ms5110s, &tmp, 2);
	tmp = be16_to_cpu(tmp);
	*data = com2ori_s16(tmp, sign_bit);

	return IIO_VAL_INT;
}

static int ms5110s_read_raw(struct iio_dev *indio_dev,
			   struct iio_chan_spec const *chan, int *val,
			   int *val2, long mask)
{
	struct ms5110s_data *ms5110s = iio_priv(indio_dev);
	int ret = -EINVAL;
	s16 tmp = 0;

	mutex_lock(&ms5110s->lock);
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = ms5110s_read_data(ms5110s, &tmp);
		*val = sign_extend32(tmp, 15);
		break;
	case IIO_CHAN_INFO_SCALE:
		ret = ms5110s_get_scale(ms5110s, val);
		break;
	case IIO_CHAN_INFO_SAMP_FREQ:
		ret = ms5110s_get_sample(ms5110s, val);
		break;
	case IIO_CHAN_INFO_HARDWAREGAIN:
		ret = ms5110s_get_gain(ms5110s, val);
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&ms5110s->lock);

	return ret;
}

static int ms5110s_write_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan, int val,
				int val2, long mask)
{
	struct ms5110s_data *ms5110s = iio_priv(indio_dev);
	int ret;

	mutex_lock(&ms5110s->lock);
	switch (mask) {
	case IIO_CHAN_INFO_HARDWAREGAIN:
		ret = ms5110s_set_gain(ms5110s, val);
		break;
	case IIO_CHAN_INFO_SCALE:
		ret = ms5110s_set_scale(ms5110s, val);
		break;
	case IIO_CHAN_INFO_SAMP_FREQ:
		ret = ms5110s_set_sample(ms5110s, val);
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&ms5110s->lock);

	return ret;
}

static const struct iio_info ms5110s_info = {
	.read_raw	= ms5110s_read_raw,
	.write_raw	= ms5110s_write_raw,
	.attrs		= &ms5110s_attribute_group,
};

static irqreturn_t ms5110s_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *i_dev = pf->indio_dev;
	struct ms5110s_data *ms5110s = iio_priv(i_dev);
	s16 temp;

	mutex_lock(&ms5110s->lock);
	ms5110s_read_data(ms5110s, &temp);
	iio_push_to_buffers_with_timestamp(i_dev, &temp, pf->timestamp);
	mutex_unlock(&ms5110s->lock);

	iio_trigger_notify_done(i_dev->trig);

	return IRQ_HANDLED;
}

static int ms5110s_init(struct ms5110s_data *ms5110s)
{
	int ret;

	ret = ms5110s_set_sample(ms5110s, 240);
	ret |= ms5110s_set_gain(ms5110s, 8);

	return ret;
}

static int ms5110s_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct ms5110s_data *ms5110s;
	struct iio_dev *indio_dev;
	int ret;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*ms5110s));
	if (!indio_dev)
		return -ENOMEM;

	ms5110s = iio_priv(indio_dev);
	i2c_set_clientdata(client, indio_dev);
	ms5110s->client = client;
	mutex_init(&ms5110s->lock);

	indio_dev->info = &ms5110s_info;
	indio_dev->name = MS5110S_DRV_NAME;
	indio_dev->channels = ms5110s_channels;
	indio_dev->num_channels = ARRAY_SIZE(ms5110s_channels);
	indio_dev->modes = INDIO_DIRECT_MODE;
	ret = devm_iio_triggered_buffer_setup(&client->dev, indio_dev,
		iio_pollfunc_store_time, ms5110s_trigger_handler, NULL);
	if (ret)
		return ret;

	ret = ms5110s_init(ms5110s);
	if (ret < 0) {
		return ret;
	}

	ret = devm_iio_device_register(&client->dev, indio_dev);
	if (ret < 0) {
		dev_err(&client->dev, "device register failed\n");
		return ret;
	}

	dev_info(&client->dev, "ms5110s probe success\n");

	return 0;
}

static const struct i2c_device_id ms5110s_id[] = {
	{MS5110S_DRV_NAME, },
	{}
};
MODULE_DEVICE_TABLE(i2c, ms5110s_id);

static const struct of_device_id ms5110s_of_match[] = {
	{ .compatible = "relmon,ms5110s", },
	{},
};
MODULE_DEVICE_TABLE(of, ms5110s_of_match);

static struct i2c_driver ms5110s_driver = {
	.driver = {
		.name = MS5110S_DRV_NAME,
		.of_match_table = ms5110s_of_match,
	},
	.probe		= ms5110s_probe,
	.id_table	= ms5110s_id,
};
module_i2c_driver(ms5110s_driver);

MODULE_AUTHOR("<897420073@qq.com>");
MODULE_DESCRIPTION("Driver for relmon ms5110s");
MODULE_LICENSE("GPL");
