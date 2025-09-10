#include <linux/acpi.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio/consumer.h>
#include <linux/iio/buffer.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/trigger.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>

/* STK8329_ADDR */
#define STK8329_CHIP_ID		0x00
#define STK8329_XOUT1		0x02
#define STK8329_XOUT2		0x03
#define STK8329_YOUT1		0x04
#define STK8329_YOUT2		0x05
#define STK8329_ZOUT1		0x06
#define STK8329_ZOUT2		0x07
#define STK8329_INTSTS1		0x09
#define STK8329_INTSTS2		0x0A
#define STK8329_EVENTINFO1	0x0B
#define STK8329_FIFOSTS		0x0C
#define STK8329_RANGESEL	0x0F
#define STK8329_BWSEL		0x10
#define STK8329_POWMODE		0x11
#define STK8329_ODRMODE		0x12
#define STK8329_DATASETUP	0x13
#define STK8329_SWRST		0x14
#define STK8329_INTEN1		0x16
#define STK8329_INTEN2		0x17
#define STK8329_INTMAP1		0x19
#define STK8329_INTMAP2		0x1A
#define STK8329_INTCFG1		0x20
#define STK8329_INTCFG2		0x21
#define STK8329_SLOPEDLY	0x27
#define STK8329_SLOPETHD	0x28
#define STK8329_SIGMOT1		0x29
#define STK8329_SIGMOT2		0x2A
#define STK8329_SIGMOT3		0x2B
#define STK8329_PRIMIF		0x33
#define STK8329_INTFCFG		0x34
#define STK8329_LPFCFG		0x35
#define STK8329_OFSTCOMP1	0x36
#define STK8329_OFSTX		0x38
#define STK8329_OFSTY		0x39
#define STK8329_OFSTZ		0x3A
#define STK8329_FIFOCFG1	0x3D
#define STK8329_FIFOCFG2	0x3E
#define STK8329_FIFODATA	0x3F

/* MASK */
#define INTSTS1_SIG_MOT_STS		BIT(0)
#define INTSTS1_ANY_MOT_STS		BIT(2)

#define INTSTS2_DATA_STS		BIT(7)
#define INTSTS2_FWM_STS			BIT(6)
#define INTSTS2_FFULL_STS		BIT(5)

#define INTEN2_FIFO_INT_TYPE	BIT(7)
#define INTEN2_FWM_EN			BIT(6)
#define INTEN2_FFULL_EN			BIT(5)
#define INTEN2_DATA_EN			BIT(4)

#define INTMAP2_DATA2INT2		BIT(7)
#define INTMAP2_FWM2INT2		BIT(6)
#define INTMAP2_FFULL2INT2		BIT(5)
#define INTMAP2_FFULL2INT1		BIT(2)
#define INTMAP2_FWM2INT1		BIT(1)
#define INTMAP2_DATA2INT1		BIT(0)

#define INTCFG2_INT_LATCH		GENMASK(3, 0)
#define INTCFG2_INT_RST			BIT(7)

#define DATASETUP_DATA_SEL		BIT(7)
#define DATASETUP_PROTECT_DIS	BIT(6)

#define POWMODE_SUSPEND			BIT(7)
#define POWMODE_LOWPOWER		BIT(6)
#define POWMODE_SLEEP_TIMER		BIT(5)
#define POWMODE_SLEEP_DUR		GENMASK(4, 1)

#define BWSEL_BW 				GENMASK(4, 0)
#define RANGESEL_RANGE			GENMASK(3, 0)

#define OFFSET_UNIT 7810000

#define STK8329_CHANNEL_SIZE			2
#define STK8329_ALL_CHANNEL_MASK		7
#define STK8329_ALL_CHANNEL_SIZE		6

/* Used to map scan mask bits to their corresponding channel register. */
static const int stk8329_channel_table[] = {
	STK8329_XOUT1,
	STK8329_YOUT1,
	STK8329_ZOUT1
};

struct stk8329_data {
	struct i2c_client *client;
	struct iio_trigger *dready_trig;
	struct gpio_desc *irq1_gpiod;
	int irq1;
	struct mutex lock;
	int range_idx;
	u8 sample_rate_idx;
	bool dready_trigger_on;
	s16 buffer[8];
};

#define STK8329_ACCEL_CHANNEL(index, reg, axis) { \
	.type = IIO_ACCEL, \
	.address = reg, \
	.modified = 1, \
	.channel2 = IIO_MOD_##axis, \
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | \
		BIT(IIO_CHAN_INFO_PROCESSED) | \
		BIT(IIO_CHAN_INFO_OFFSET), \
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) | \
		BIT(IIO_CHAN_INFO_SAMP_FREQ), \
	.scan_index = index, \
	.scan_type = { \
		.sign = 's', \
		.realbits = 16, \
		.storagebits = 16, \
		.shift = 0, \
		.endianness = IIO_CPU, \
	}, \
}

static const struct iio_chan_spec stk8329_channels[] = {
	STK8329_ACCEL_CHANNEL(0, STK8329_XOUT1, X),
	STK8329_ACCEL_CHANNEL(1, STK8329_YOUT1, Y),
	STK8329_ACCEL_CHANNEL(2, STK8329_ZOUT1, Z),
	IIO_CHAN_SOFT_TIMESTAMP(3),
};

static int stk8329_read_byte(struct i2c_client *client, uint8_t addr)
{
	return i2c_smbus_read_byte_data(client, addr);
}

static int stk8329_write_byte(struct i2c_client *client, uint8_t addr, uint8_t value)
{
	return i2c_smbus_write_byte_data(client, addr, value);
}

static int stk8329_set_bits(struct i2c_client *client,
	uint8_t addr, uint8_t mask, uint8_t data)
{
	uint8_t value;

	value = stk8329_read_byte(client, addr);
	value &= ~mask;
	value |= (data & mask);

	return stk8329_write_byte(client, addr, value);
}

static int stk8329_set_suspend_mode(struct i2c_client *client, bool value)
{
	return stk8329_set_bits(client, STK8329_POWMODE, POWMODE_SUSPEND,
		value ? 0x80 : 0x00);
}

static int stk8329_set_asix_offset(struct stk8329_data *stk3829,
	struct iio_chan_spec const *chan, int val, int val2)
{
	int ret;
	int reg_val;
	int offset;

	offset = val * 1000000 + val2;
	reg_val = offset / OFFSET_UNIT;
	reg_val = sign_extend32(reg_val, 7);
	switch(chan->channel2) {
	case IIO_MOD_X:
		ret = stk8329_write_byte(stk3829->client, STK8329_OFSTX, reg_val);
		break;
	case IIO_MOD_Y:
		ret = stk8329_write_byte(stk3829->client, STK8329_OFSTY, reg_val);
		break;
	case IIO_MOD_Z:
		ret = stk8329_write_byte(stk3829->client, STK8329_OFSTZ, reg_val);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int stk8329_get_asix_offset(struct stk8329_data *stk3829,
	struct iio_chan_spec const *chan, int *val, int *val2)
{
	int offset;
	int reg_val = 0;

	switch(chan->channel2) {
	case IIO_MOD_X:
		reg_val = stk8329_read_byte(stk3829->client, STK8329_OFSTX);
		break;
	case IIO_MOD_Y:
		reg_val = stk8329_read_byte(stk3829->client, STK8329_OFSTY);
		break;
	case IIO_MOD_Z:
		reg_val = stk8329_read_byte(stk3829->client, STK8329_OFSTZ);
		break;
	default:
		return -EINVAL;
	}
	offset = sign_extend32(reg_val, 7) * OFFSET_UNIT;
	*val = offset / 1000000;
	*val2 = offset % 1000000;

	return IIO_VAL_INT_PLUS_MICRO;
}

static IIO_CONST_ATTR_SAMP_FREQ_AVAIL("7.81 15.63 31.25 62.5 125 250 500 1000");

static const struct {
	u8 reg_val;
	u16 samp_freq;
} stk8329_samp_freq_table[] = {
	{0x08, 7}, {0x09, 15}, {0x0A, 31}, {0x0B, 62},
	{0x0C, 125}, {0x0D, 250}, {0x0E, 500}, {0x0F, 1000}
};

static int stk8329_set_asix_sampel_rate(struct stk8329_data *stk3829, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(stk8329_samp_freq_table); i++) {
		if (stk8329_samp_freq_table[i].samp_freq == val) {
			stk3829->sample_rate_idx = i;

			return stk8329_set_bits(stk3829->client, STK8329_BWSEL,
				BWSEL_BW, stk8329_samp_freq_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int stk8329_get_asix_sampel_rate(struct stk8329_data *stk3829, int *val)
{
	int reg_val;
	int i;

	reg_val = stk8329_read_byte(stk3829->client, STK8329_BWSEL) & BWSEL_BW;
	for(i = 0; i < ARRAY_SIZE(stk8329_samp_freq_table); i++) {
		if (stk8329_samp_freq_table[i].reg_val == reg_val) {
			*val = stk8329_samp_freq_table[i].samp_freq;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static IIO_CONST_ATTR(in_accel_scale_available, "2 4 8 16");

static const struct {
	u8 reg_val;
	u8 range;
	u32 scale_val;
} stk8329_scale_table[] = {
	{0x3, 2, 61035}, {0x5, 4, 122070}, {0x8, 8, 244140}, {0xc, 16, 488281}
};

static int stk8329_set_asix_scale(struct stk8329_data *stk8329, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(stk8329_scale_table); i++) {
		if (stk8329_scale_table[i].range == val) {
			stk8329->range_idx = i;

			return stk8329_set_bits(stk8329->client, STK8329_RANGESEL,
				RANGESEL_RANGE, stk8329_scale_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int stk8329_get_asix_scale(struct stk8329_data *stk8329, int *val)
{
	int reg_val;
	int i;

	reg_val = stk8329_read_byte(stk8329->client, STK8329_RANGESEL) &
		RANGESEL_RANGE;
	for (i = 0; i < ARRAY_SIZE(stk8329_scale_table); i++) {
		if (stk8329_scale_table[i].reg_val == reg_val) {
			*val = stk8329_scale_table[i].range;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static int stk8329_read_asix_raw_data(struct stk8329_data *stk8329, u32 addr)
{
	int asix_data;

	i2c_smbus_read_i2c_block_data(stk8329->client, addr,
		STK8329_CHANNEL_SIZE, (u8 *)&asix_data);

	return asix_data;
}

static int stk8329_read_asix_processed(struct stk8329_data *stk8329,
	struct iio_chan_spec const *chan, int *val, int *val2)
{
	int reg_val = 0;

	reg_val = stk8329_read_asix_raw_data(stk8329, chan->address);
	reg_val = sign_extend32(reg_val, 15);
	reg_val = reg_val * stk8329_scale_table[stk8329->range_idx].scale_val;
	*val = reg_val / 1000000;
	*val2 = reg_val % 1000000;

	return IIO_VAL_INT_PLUS_MICRO;
}

static int stk8329_read_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     int *val, int *val2, long mask)
{
	struct stk8329_data *stk8329 = iio_priv(indio_dev);
	int ret = 0;
	int asix = 0;

	mutex_lock(&stk8329->lock);
	switch(chan->type) {
	case IIO_ACCEL:
		switch(mask) {
		case IIO_CHAN_INFO_RAW:
			asix = stk8329_read_asix_raw_data(stk8329, chan->address);
			*val = sign_extend32(asix, 15);
			ret = IIO_VAL_INT;
			break;
		case IIO_CHAN_INFO_PROCESSED:
			ret = stk8329_read_asix_processed(stk8329, chan, val, val2);
			break;
		case IIO_CHAN_INFO_SCALE:
			ret = stk8329_get_asix_scale(stk8329, val);
			break;
		case IIO_CHAN_INFO_OFFSET:
			ret = stk8329_get_asix_offset(stk8329, chan, val, val2);
			break;
		case IIO_CHAN_INFO_SAMP_FREQ:
			ret = stk8329_get_asix_sampel_rate(stk8329, val);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&stk8329->lock);

	return ret;
}

static int stk8329_write_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int val, int val2, long mask)
{
	struct stk8329_data *stk8329 = iio_priv(indio_dev);
	int ret = 0;

	mutex_lock(&stk8329->lock);
	stk8329_set_suspend_mode(stk8329->client, 1);
	switch (mask) {
	case IIO_CHAN_INFO_SCALE:
		ret = stk8329_set_asix_scale(stk8329, val);
		break;
	case IIO_CHAN_INFO_OFFSET:
		ret = stk8329_set_asix_offset(stk8329, chan, val, val2);
		break;
	case IIO_CHAN_INFO_SAMP_FREQ:
		ret = stk8329_set_asix_sampel_rate(stk8329, val);
		break;
	default:
		ret = -EINVAL;
	}
	stk8329_set_suspend_mode(stk8329->client, 0);
	mutex_unlock(&stk8329->lock);

	return ret;
}

static irqreturn_t stk8329_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct stk8329_data *stk8329 = iio_priv(indio_dev);
	int bit, ret, i = 0;

	mutex_lock(&stk8329->lock);
	/*
	 * Do a bulk read if all channels are requested,
	 * from 0x02 (XOUT1) to 0x07 (ZOUT2)
	 */
	if (*(indio_dev->active_scan_mask) == STK8329_ALL_CHANNEL_MASK) {
		ret = i2c_smbus_read_i2c_block_data(stk8329->client,
						    STK8329_XOUT1,
						    STK8329_ALL_CHANNEL_SIZE,
						    (u8 *)stk8329->buffer);

		if (ret < STK8329_ALL_CHANNEL_SIZE) {
			dev_err(&stk8329->client->dev, "register read failed\n");
			goto err;
		}
	} else {
		for_each_set_bit(bit, indio_dev->active_scan_mask,
				 indio_dev->masklength) {
			ret = stk8329_read_asix_raw_data(stk8329,
					stk8329_channel_table[bit]);
			if (ret < 0)
				goto err;

			stk8329->buffer[i++] = ret;
		}
	}
	iio_push_to_buffers_with_timestamp(indio_dev, stk8329->buffer,
					   pf->timestamp);
err:
	mutex_unlock(&stk8329->lock);
	iio_trigger_notify_done(indio_dev->trig);

	return IRQ_HANDLED;
}

static void stk8329_init(struct stk8329_data *stk8329)
{
	stk8329_set_suspend_mode(stk8329->client, 1);
	/* Map new data interrupt to INT1 */
	stk8329_set_bits(stk8329->client, STK8329_INTMAP2, INTMAP2_DATA2INT1, 0x1);
	stk8329_set_asix_scale(stk8329, 2);
	stk8329_set_asix_sampel_rate(stk8329, 15);
	stk8329_set_suspend_mode(stk8329->client, 0);
}

static ssize_t reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct stk8329_data *stk8329 = iio_priv(indio_dev);
	unsigned int reg_val = 0;
	unsigned int i = 0;
	ssize_t len = 0;

	for (i = 0; i <= 0x26; i++) {
		reg_val = stk8329_read_byte(stk8329->client, i);
		len += snprintf(buf + len, PAGE_SIZE - len, "reg:0x%02x=0x%02x\n", i, reg_val);
	}

	return len;
}

static ssize_t reg_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t len)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct stk8329_data *stk8329 = iio_priv(indio_dev);
	unsigned int databuf[2];

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2)
		stk8329_write_byte(stk8329->client, databuf[0], databuf[1]);

	return len;
}
static DEVICE_ATTR_RW(reg);

static struct attribute *stk8329_attributes[] = {
	&iio_const_attr_in_accel_scale_available.dev_attr.attr,
	&iio_const_attr_sampling_frequency_available.dev_attr.attr,
	&dev_attr_reg.attr,
	NULL,
};

static const struct attribute_group stk8329_attribute_group = {
	.attrs = stk8329_attributes
};

static const struct iio_info stk8329_info = {
	.read_raw		= stk8329_read_raw,
	.write_raw		= stk8329_write_raw,
	.attrs			= &stk8329_attribute_group,
};

static int stk8329_data_rdy_trigger_set_state(struct iio_trigger *trig,
					       bool state)
{
	struct iio_dev *indio_dev = iio_trigger_get_drvdata(trig);
	struct stk8329_data *stk8329 = iio_priv(indio_dev);
	int ret;

	if (state)
		ret = stk8329_set_bits(stk8329->client, STK8329_INTEN2, INTEN2_DATA_EN, 0x10);

	else
		ret = stk8329_set_bits(stk8329->client, STK8329_INTEN2, INTEN2_DATA_EN, 0x00);

	if (ret < 0)
		dev_err(&stk8329->client->dev, "failed to set trigger state\n");
	else
		stk8329->dready_trigger_on = state;

	return ret;
}

static irqreturn_t stk8329_data_rdy_trig_poll(int irq, void *private)
{
	struct iio_dev *indio_dev = private;
	struct stk8329_data *stk8329 = iio_priv(indio_dev);

	if (stk8329->dready_trigger_on)
		iio_trigger_poll(stk8329->dready_trig);

	return IRQ_HANDLED;
}

static const struct iio_buffer_setup_ops stk8329_buffer_setup_ops = {
	.postenable  = iio_triggered_buffer_postenable,
	.predisable  = iio_triggered_buffer_predisable,
};

static const struct iio_trigger_ops stk8329_trigger_ops = {
	.set_trigger_state = stk8329_data_rdy_trigger_set_state,
};

static int stk8329_check_id(struct i2c_client *client)
{
	uint8_t id_expected = 0x25;
	uint8_t id = {0};

	id = stk8329_read_byte(client, STK8329_CHIP_ID);
	if (memcmp(&id, &id_expected, sizeof(uint8_t))) {
		dev_err(&client->dev, "chip id mismatch\n");
		return -ENODEV;
	}
	/* Reset all the registers to default value */
	stk8329_write_byte(client, STK8329_SWRST, 0xB6);

	return 0;
}

static int stk8329_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	int ret;
	struct iio_dev *indio_dev;
	struct stk8329_data *stk8329;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*stk8329));
	if (!indio_dev) {
		dev_err(&client->dev, "iio allocation failed!\n");
		return -ENOMEM;
	}

	stk8329 = iio_priv(indio_dev);
	stk8329->client = client;
	i2c_set_clientdata(client, indio_dev);
	mutex_init(&stk8329->lock);

	indio_dev->dev.parent = &client->dev;
	indio_dev->info = &stk8329_info;
	indio_dev->name = "stk8329";
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = stk8329_channels;
	indio_dev->num_channels = ARRAY_SIZE(stk8329_channels);

	ret = stk8329_check_id(client);
	if (ret < 0) {
		return ret;
	}
	stk8329_init(stk8329);

	stk8329->irq1_gpiod = devm_gpiod_get_optional(&client->dev, "stk8329,irq1", GPIOD_IN);
	if (!stk8329->irq1_gpiod) {
		dev_err(&client->dev, "failed to get irq1 gpio\n");
	}
	if (stk8329->irq1_gpiod) {
		stk8329->irq1 = gpiod_to_irq(stk8329->irq1_gpiod);
		if (stk8329->irq1 < 0) {
			dev_err(&client->dev, "failed to irq1 for stk8329!\n");
		}
	}

	if (stk8329->irq1 > 0) {
		ret = devm_request_threaded_irq(&client->dev, stk8329->irq1,
						stk8329_data_rdy_trig_poll, NULL,
						IRQF_TRIGGER_RISING | IRQF_ONESHOT,
						"stk8329",
						indio_dev);
		if (ret < 0) {
			dev_err(&client->dev, "request irq1 %d failed\n",
				client->irq);
			return ret;
		}

		stk8329->dready_trig = devm_iio_trigger_alloc(&client->dev,
							   "%s-dev%d",
							   indio_dev->name,
							   indio_dev->id);
		if (!stk8329->dready_trig) {
			ret = -ENOMEM;
			return ret;
		}

		stk8329->dready_trig->dev.parent = &client->dev;
		stk8329->dready_trig->ops = &stk8329_trigger_ops;
		iio_trigger_set_drvdata(stk8329->dready_trig, indio_dev);
		ret = devm_iio_trigger_register(&client->dev, stk8329->dready_trig);
		if (ret) {
			dev_err(&client->dev, "iio trigger register failed\n");
			return ret;
		}

		ret = devm_iio_triggered_buffer_setup(&client->dev,
						indio_dev,
						iio_pollfunc_store_time,
						stk8329_trigger_handler,
						&stk8329_buffer_setup_ops);
		if (ret < 0) {
			dev_err(&client->dev, "iio triggered buffer setup failed\n");
			return -EINVAL;
		}
	}

	ret = devm_iio_device_register(&client->dev, indio_dev);
	if (ret < 0) {
		dev_err(&client->dev, "device_register failed\n");
		return ret;
	}

	dev_info(&client->dev, "stk8329 probe success\n");

	return 0;
}

static int stk8329_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);
	struct stk8329_data *stk8329 = iio_priv(indio_dev);

	return stk8329_set_suspend_mode(stk8329->client, 0);
}

#ifdef CONFIG_PM_SLEEP
static int stk8329_suspend(struct device *dev)
{
	struct stk8329_data *stk8329;

	stk8329 = iio_priv(i2c_get_clientdata(to_i2c_client(dev)));

	return stk8329_set_suspend_mode(stk8329->client, 1);
}

static int stk8329_resume(struct device *dev)
{
	struct stk8329_data *stk8329;

	stk8329 = iio_priv(i2c_get_clientdata(to_i2c_client(dev)));

	return stk8329_set_suspend_mode(stk8329->client, 0);
}

static SIMPLE_DEV_PM_OPS(stk8329_pm_ops, stk8329_suspend, stk8329_resume);

#define stk8329_PM_OPS (&stk8329_pm_ops)
#else
#define stk8329_PM_OPS NULL
#endif

static const struct i2c_device_id stk8329_i2c_id[] = {
	{"stk8329", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, stk8329_i2c_id);

static const struct acpi_device_id stk8329_acpi_id[] = {
	{"stk8329", 0},
	{}
};
MODULE_DEVICE_TABLE(acpi, stk8329_acpi_id);

static struct i2c_driver stk8329_driver = {
	.driver = {
		.name = "stk8329",
		.pm = stk8329_PM_OPS,
		.acpi_match_table = ACPI_PTR(stk8329_acpi_id),
	},
	.probe =  stk8329_probe,
	.remove = stk8329_remove,
	.id_table = stk8329_i2c_id,
};
module_i2c_driver(stk8329_driver);

MODULE_AUTHOR("<897420073@qq.com>");
MODULE_DESCRIPTION("Driver for stk8329");
MODULE_LICENSE("GPL and additional rights");
