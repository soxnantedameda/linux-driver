#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#define LTR381_MAIN_CTRL 0x00
#define LTR381_MAIN_CTRL_SW_RESET_MASK BIT(4)
#define LTR381_MAIN_CTRL_CS_MODE_MASK BIT(2)
#define LTR381_MAIN_CTRL_ALS_CS_ENABLE_MASK BIT(1)

#define LTR381_ALS_CS_MEAS_RATE 0x04
#define LTR381_ALS_CS_MEAS_RATE_RESOLUTION_MASK GENMASK(6, 4)
#define LTR381_ALS_CS_MEAS_RATE_MEASUREMENT_MASK GENMASK(2, 0)

#define LTR381_ALS_CS_GAIN 0x05
#define LTR381_ALS_CS_GAIN_RANGE_MASK GENMASK(2, 0)

#define LTR381_PART_ID 0x06
#define LTR381_MAIN_STATUS 0x07
#define LTR381_CS_DATA_IR_0 0x0A
#define LTR381_CS_DATA_IR_1 0x0B
#define LTR381_CS_DATA_IR_2 0x0C
#define LTR381_CS_DATA_GREEN_0 0x0D
#define LTR381_CS_DATA_GREEN_1 0x0E
#define LTR381_CS_DATA_GREEN_2 0x0F
#define LTR381_CS_DATA_RED_0 0x10
#define LTR381_CS_DATA_RED_1 0x11
#define LTR381_CS_DATA_RED_2 0x12
#define LTR381_CS_DATA_BLUE_0 0x13
#define LTR381_CS_DATA_BLUE_1 0x14
#define LTR381_CS_DATA_BLUE_2 0x15
#define LTR381_INT_CFG 0x19
#define LTR381_INT_PST 0x1A
#define LTR381_ALS_THRES_UP_0 0x21
#define LTR381_ALS_THRES_UP_1 0x22
#define LTR381_ALS_THRES_UP_2 0x23
#define LTR381_ALS_THRES_LOW_0 0x24
#define LTR381_ALS_THRES_LOW_1 0x25
#define LTR381_ALS_THRES_LOW_2 0x26

#define CCT_COEFFICEINT 3724
#define CCT_OFFSET 1525

enum ltr381_mode {
	LTR381_MODE_STANDBY,
	LTR381_MODE_ACTIVE
};

enum ltr381_cs_mode {
	LTR381_MODE_ALS,
	LTR381_MODE_CS
};

enum ltr381_led_idx {
	LTR381_LED_IR,
	LTR381_LED_RED,
	LTR381_LED_GREEN,
	LTR381_LED_BLUE,
};

struct ltr381rgb_data {
	struct iio_trigger *trig;
	struct i2c_client *client;
	struct gpio_desc *irq_gpiod;
	int irq;

	u32 hardwaredgain;
	u32 sample_rate;
	u32 resolution;

	u32 upper_threshold;
	u32 lower_threshold;

	enum ltr381_mode mode;
	enum ltr381_cs_mode cs_mode;

	int ir_factor;
	int r_factor;
	int b_factor;
	int g_factor;

	struct mutex lock;
};

#define LTR381_LIGHT_CHANNEL(_mode) { \
	.type = IIO_LIGHT, \
	.channel2 = IIO_MOD_LIGHT_##_mode, \
	.scan_index = LTR381_LED_##_mode, \
	.address = LTR381_CS_DATA_##_mode##_0, \
	.modified = 1, \
	.scan_type = { \
		.sign = 'u', \
		.realbits = 20, \
		.storagebits = 24, \
		.shift = 0, \
	}, \
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | \
			BIT(IIO_CHAN_INFO_PROCESSED) | \
			BIT(IIO_CHAN_INFO_CALIBSCALE), \
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) | \
			BIT(IIO_CHAN_INFO_PEAK) | \
			BIT(IIO_CHAN_INFO_AVERAGE_RAW) | \
			BIT(IIO_CHAN_INFO_SAMP_FREQ) | \
			BIT(IIO_CHAN_INFO_HARDWAREGAIN), \
}

static const struct iio_chan_spec ltr381rgb_channels[] = {
	{
		.type = IIO_TEMP,
		.info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),
	},
	LTR381_LIGHT_CHANNEL(IR),
	LTR381_LIGHT_CHANNEL(RED),
	LTR381_LIGHT_CHANNEL(GREEN),
	LTR381_LIGHT_CHANNEL(BLUE),
};

static int ltr381rgb_read_byte(struct i2c_client *client, uint8_t addr)
{
	return i2c_smbus_read_byte_data(client, addr);
}

static int ltr381rgb_write_byte(struct i2c_client *client, uint8_t addr, uint8_t value)
{
	return i2c_smbus_write_byte_data(client, addr, value);
}

static int ltr381rgb_set_bits(struct i2c_client *client,
	uint8_t addr, uint8_t mask, uint8_t data)
{
	uint8_t value;

	value = ltr381rgb_read_byte(client, addr);
	value &= ~mask;
	value |= (data & mask);

	return ltr381rgb_write_byte(client, addr, value);
}

static int ltr381rgb_get_raw_data(struct ltr381rgb_data *ltr381rgb, u32 addr)
{
	int raw_data[3];
	int i;

	for (i = 0; i < 3; i++) {
		raw_data[i] = ltr381rgb_read_byte(ltr381rgb->client, addr + i);
	}

	return (raw_data[2] << 16) | (raw_data[1] << 8) | raw_data[0];
}

static int ltr381rgb_set_mode(struct ltr381rgb_data *ltr381rgb,
	enum ltr381_mode mode)
{
	int ret = 0;

	if (mode == LTR381_MODE_STANDBY) {
		ret = ltr381rgb_set_bits(ltr381rgb->client, LTR381_MAIN_CTRL,
			LTR381_MAIN_CTRL_ALS_CS_ENABLE_MASK, 0);
	} else {
		ret = ltr381rgb_set_bits(ltr381rgb->client, LTR381_MAIN_CTRL,
			LTR381_MAIN_CTRL_ALS_CS_ENABLE_MASK, 0xff);
	}
	ltr381rgb->mode = mode;

	return ret;
}

static int ltr381rgb_set_cs_mode(struct ltr381rgb_data *ltr381rgb,
	enum ltr381_cs_mode cs_mode)
{
	int ret = 0;

	if (cs_mode == LTR381_MODE_ALS) {
		ret = ltr381rgb_set_bits(ltr381rgb->client, LTR381_MAIN_CTRL,
			LTR381_MAIN_CTRL_CS_MODE_MASK, 0);
	} else {
		ret = ltr381rgb_set_bits(ltr381rgb->client, LTR381_MAIN_CTRL,
			LTR381_MAIN_CTRL_CS_MODE_MASK, 0xff);
	}
	ltr381rgb->cs_mode = cs_mode;

	return ret;
}

static IIO_CONST_ATTR(hardwaregain_available, "1 3 6 9 18");

static const int ltr381rgb_gain[] = {1, 3, 6, 9, 18};

static int ltr381rgb_set_gain_range(struct ltr381rgb_data *ltr381rgb,
	int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ltr381rgb_gain); i++) {
		if (ltr381rgb_gain[i] == val) {
			return ltr381rgb_set_bits(ltr381rgb->client, LTR381_ALS_CS_GAIN,
				LTR381_ALS_CS_GAIN_RANGE_MASK, i);
		}
	}

	return -EINVAL;
}

static int ltr381rgb_get_gain_range(struct ltr381rgb_data *ltr381rgb, int *val)
{
	int reg_val;
	int i;

	reg_val = ltr381rgb_read_byte(ltr381rgb->client, LTR381_ALS_CS_GAIN);
	for (i = 0; i < ARRAY_SIZE(ltr381rgb_gain); i++) {
		if (reg_val == i) {
			*val = ltr381rgb_gain[i];
			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static IIO_CONST_ATTR(scale_available, "20 19 18 17 16");

static const int ltr381rgb_resolution[] = {20, 19, 18, 17, 16};

static int ltr381rgb_set_resolution(struct ltr381rgb_data *ltr381rgb,
	int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ltr381rgb_resolution); i++) {
		if (ltr381rgb_resolution[i] == val) {
			return ltr381rgb_set_bits(ltr381rgb->client, LTR381_ALS_CS_MEAS_RATE,
				LTR381_ALS_CS_MEAS_RATE_RESOLUTION_MASK, i << 4);
		}
	}

	return -EINVAL;
}

static int ltr381rgb_get_resolution(struct ltr381rgb_data *ltr381rgb,
	int *val)
{
	int reg_val;
	int i;

	reg_val = (ltr381rgb_read_byte(ltr381rgb->client, LTR381_ALS_CS_MEAS_RATE)
		& LTR381_ALS_CS_MEAS_RATE_RESOLUTION_MASK) >> 4;
	for (i = 0; i < ARRAY_SIZE(ltr381rgb_resolution); i++) {
		if (reg_val == i) {
			*val = ltr381rgb_resolution[i];
			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static IIO_CONST_ATTR(sampling_frequency_available, "40 20 10 5 2 1 0.5");

static const int ltr381rgb_freqs[][2] = {
	{40, 0}, {20, 0}, {10, 0}, {5, 0}, {2, 0}, {1, 0}, {0, 500000}};

static int ltr381rgb_set_measurement_rate(struct ltr381rgb_data *ltr381rgb,
	int val, int val2)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ltr381rgb_freqs); i++) {
		if (ltr381rgb_freqs[i][0] == val &&
			ltr381rgb_freqs[i][1] == val2) {
			return ltr381rgb_set_bits(ltr381rgb->client, LTR381_ALS_CS_MEAS_RATE,
				LTR381_ALS_CS_MEAS_RATE_MEASUREMENT_MASK, i);
		}
	}

	return -EINVAL;
}

static int ltr381rgb_get_measurement_rate(struct ltr381rgb_data *ltr381rgb,
	int *val, int *val2)
{
	int reg_val;
	int i;

	reg_val = ltr381rgb_read_byte(ltr381rgb->client, LTR381_ALS_CS_MEAS_RATE)
		& LTR381_ALS_CS_MEAS_RATE_MEASUREMENT_MASK;
	for (i = 0; i < ARRAY_SIZE(ltr381rgb_freqs); i++) {
		if (reg_val == i) {
			*val = ltr381rgb_freqs[i][0];
			*val2 = ltr381rgb_freqs[i][1];
			return IIO_VAL_INT_PLUS_MICRO;
		}
	}

	return -EINVAL;
}

static int ltr381rgb_get_peak(struct ltr381rgb_data *ltr381rgb, int *val)
{
	int blue, red, green;

	red = ltr381rgb_get_raw_data(ltr381rgb, LTR381_CS_DATA_RED_0);
	green = ltr381rgb_get_raw_data(ltr381rgb, LTR381_CS_DATA_GREEN_0);
	blue = ltr381rgb_get_raw_data(ltr381rgb, LTR381_CS_DATA_BLUE_0);
	*val = max3(red, green, blue);

	return IIO_VAL_INT;
}

static int ltr381rgb_get_rgb_average(struct ltr381rgb_data *ltr381rgb, int *val)
{
	int blue, red, green;

	red = ltr381rgb_get_raw_data(ltr381rgb, LTR381_CS_DATA_RED_0);
	green = ltr381rgb_get_raw_data(ltr381rgb, LTR381_CS_DATA_GREEN_0);
	blue = ltr381rgb_get_raw_data(ltr381rgb, LTR381_CS_DATA_BLUE_0);
	*val = ((red + green  + blue) / 3);

	return IIO_VAL_INT;
}

static int ltr381rgb_get_processed(struct ltr381rgb_data *ltr381rgb,
	struct iio_chan_spec const *chan, int *val)
{
	long raw;

	raw = ltr381rgb_get_raw_data(ltr381rgb, chan->address);
	switch (chan->channel2) {
	case IIO_MOD_LIGHT_RED:
		*val = raw * ltr381rgb->r_factor / 1000000;
		break;
	case IIO_MOD_LIGHT_GREEN:
		*val = raw * ltr381rgb->g_factor / 1000000;
		break;
	case IIO_MOD_LIGHT_BLUE:
		*val = raw * ltr381rgb->b_factor / 1000000;
		break;
	case IIO_MOD_LIGHT_IR:
		*val = raw * ltr381rgb->ir_factor / 1000000;
		break;
	default:
		return -EINVAL;
	}

	return IIO_VAL_INT;
}

static int ltr381rgb_set_calibscale(struct ltr381rgb_data *ltr381rgb,
	struct iio_chan_spec const *chan, int val, int val2)
{
	switch (chan->channel2) {
	case IIO_MOD_LIGHT_RED:
		ltr381rgb->r_factor = val * 1000000 + val2;
		break;
	case IIO_MOD_LIGHT_GREEN:
		ltr381rgb->g_factor = val * 1000000 + val2;
		break;
	case IIO_MOD_LIGHT_BLUE:
		ltr381rgb->b_factor = val * 1000000 + val2;
		break;
	case IIO_MOD_LIGHT_IR:
		ltr381rgb->ir_factor = val * 1000000 + val2;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ltr381rgb_get_calibscale(struct ltr381rgb_data *ltr381rgb,
	struct iio_chan_spec const *chan, int *val, int *val2)
{
	switch (chan->channel2) {
	case IIO_MOD_LIGHT_RED:
		*val = ltr381rgb->r_factor / 1000000;
		*val2 = ltr381rgb->r_factor % 1000000;
		break;
	case IIO_MOD_LIGHT_GREEN:
		*val = ltr381rgb->g_factor / 1000000;
		*val2 = ltr381rgb->g_factor % 1000000;
		break;
	case IIO_MOD_LIGHT_BLUE:
		*val = ltr381rgb->b_factor / 1000000;
		*val2 = ltr381rgb->b_factor % 1000000;
		break;
	case IIO_MOD_LIGHT_IR:
		*val = ltr381rgb->ir_factor / 1000000;
		*val2 = ltr381rgb->ir_factor % 1000000;
		break;
	default:
		return -EINVAL;
	}

	return IIO_VAL_INT_PLUS_MICRO;

}

static int ltr381rgb_get_color_temperature(struct ltr381rgb_data *ltr381rgb,
	int *val)
{
	long blue, red;
	long long tmp;

	blue = ltr381rgb_get_raw_data(ltr381rgb, LTR381_CS_DATA_BLUE_0);
	blue = blue * ltr381rgb->b_factor / 1000000;
	red = ltr381rgb_get_raw_data(ltr381rgb, LTR381_CS_DATA_RED_0);
	red = red  * ltr381rgb->r_factor / 1000000;
	tmp =  CCT_COEFFICEINT * (blue * 1000000) / red;
	*val = tmp / 1000000 + CCT_OFFSET;

	return IIO_VAL_INT;
}

static ssize_t cs_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct ltr381rgb_data *ltr381rgb = iio_priv(indio_dev);
	u32 reg;
	ssize_t count;

	reg = ltr381rgb_read_byte(ltr381rgb->client, LTR381_MAIN_CTRL) &
		LTR381_MAIN_CTRL_CS_MODE_MASK;
	if (reg) {
		count = sprintf(buf, "cs\n");
	} else {
		count = sprintf(buf, "als\n");
	}

	return count;
}

static ssize_t cs_mode_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t len)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct ltr381rgb_data *ltr381rgb = iio_priv(indio_dev);

	if (!strncmp(buf, "cs", 2)) {
		ltr381rgb_set_cs_mode(ltr381rgb, LTR381_MODE_CS);
	} else if (!strncmp(buf, "als", 3)) {
		ltr381rgb_set_cs_mode(ltr381rgb, LTR381_MODE_ALS);
	}

	return len;
}
static DEVICE_ATTR_RW(cs_mode);

static ssize_t calibration_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t len)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct ltr381rgb_data *ltr381rgb = iio_priv(indio_dev);
	int ret;
	bool cali;
	long red, green, blue, peak;

	ret = kstrtobool(buf, &cali);
	if (ret)
		return ret;

	if (cali) {
		red = ltr381rgb_get_raw_data(ltr381rgb, LTR381_CS_DATA_RED_0);
		green = ltr381rgb_get_raw_data(ltr381rgb, LTR381_CS_DATA_GREEN_0);
		blue = ltr381rgb_get_raw_data(ltr381rgb, LTR381_CS_DATA_BLUE_0);
		peak = max3(red, green, blue);
		ltr381rgb->r_factor = peak * 1000000 / red;
		ltr381rgb->g_factor = peak * 1000000 / green;
		ltr381rgb->b_factor = peak * 1000000 / blue;
	} else {
		/* Reset to default */
		ltr381rgb->r_factor = 1000000;
		ltr381rgb->g_factor = 1000000;
		ltr381rgb->b_factor = 1000000;
	}

	return len;
}
static DEVICE_ATTR_WO(calibration);

static ssize_t reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct ltr381rgb_data *ltr381rgb = iio_priv(indio_dev);
	unsigned int reg_val = 0;
	unsigned int i = 0;
	ssize_t len = 0;

	for (i = 0; i <= 0x26; i++) {
		reg_val = ltr381rgb_read_byte(ltr381rgb->client, i);
		len += snprintf(buf + len, PAGE_SIZE - len, "reg:0x%02x=0x%02x\n", i, reg_val);
	}

	return len;
}

static ssize_t reg_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t len)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct ltr381rgb_data *ltr381rgb = iio_priv(indio_dev);
	unsigned int databuf[2];

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2)
		ltr381rgb_write_byte(ltr381rgb->client, databuf[0], databuf[1]);

	return len;
}
static DEVICE_ATTR_RW(reg);

/* add your attr in here*/
static struct attribute *ltr381rgb_attributes[] = {
	&iio_const_attr_sampling_frequency_available.dev_attr.attr,
	&iio_const_attr_hardwaregain_available.dev_attr.attr,
	&iio_const_attr_scale_available.dev_attr.attr,
	&dev_attr_calibration.attr,
	&dev_attr_cs_mode.attr,
	&dev_attr_reg.attr,
	NULL
};

static struct attribute_group ltr381rgb_attribute_group = {
	.attrs = ltr381rgb_attributes
};

static int ltr381rgb_check_id(struct ltr381rgb_data *ltr381rgb)
{
	u8 id_expected = 0xc2;
	u8 id = {0};

	id = i2c_smbus_read_byte_data(ltr381rgb->client, LTR381_PART_ID);

	if (memcmp(&id, &id_expected, sizeof(u8))) {
		dev_err(&ltr381rgb->client->dev, "chip id mismatch\n");
		return -ENODEV;
	}

	return 0;
}

static int ltr381rgb_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val, int *val2, long mask)
{
	struct ltr381rgb_data *ltr381rgb = iio_priv(indio_dev);
	int ret = -EINVAL;

	mutex_lock(&ltr381rgb->lock);
	switch(chan->type) {
		/* color temperature */
		case IIO_TEMP:
			ret = ltr381rgb_get_color_temperature(ltr381rgb, val);
			break;
		case IIO_LIGHT:
			switch (mask) {
			case IIO_CHAN_INFO_RAW:
				*val = ltr381rgb_get_raw_data(ltr381rgb, chan->address);
				ret = IIO_VAL_INT;
				break;
			case IIO_CHAN_INFO_PROCESSED:
				ret = ltr381rgb_get_processed(ltr381rgb, chan, val);
				break;
			case IIO_CHAN_INFO_CALIBSCALE:
				ret = ltr381rgb_get_calibscale(ltr381rgb, chan, val, val2);
				break;
			case IIO_CHAN_INFO_AVERAGE_RAW:
				ret = ltr381rgb_get_rgb_average(ltr381rgb, val);
				break;
			case IIO_CHAN_INFO_PEAK:
				ret = ltr381rgb_get_peak(ltr381rgb, val);
				break;
			case IIO_CHAN_INFO_SCALE:
				ret = ltr381rgb_get_resolution(ltr381rgb, val);
				break;
			case IIO_CHAN_INFO_SAMP_FREQ:
				ret = ltr381rgb_get_measurement_rate(ltr381rgb, val, val2);
				break;
			case IIO_CHAN_INFO_HARDWAREGAIN:
				ret = ltr381rgb_get_gain_range(ltr381rgb, val);
				break;
			default:
				ret = -EINVAL;
				break;
			}
			break;
		default:
			ret = -EINVAL;
	}
	mutex_unlock(&ltr381rgb->lock);

	return ret;
}

static int ltr381rgb_write_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int val, int val2, long mask)
{
	struct ltr381rgb_data *ltr381rgb = iio_priv(indio_dev);
	int ret = 0;

	mutex_lock(&ltr381rgb->lock);
	switch (mask) {
	case IIO_CHAN_INFO_SCALE:
		ret = ltr381rgb_set_resolution(ltr381rgb, val);
		break;
	case IIO_CHAN_INFO_CALIBSCALE:
		ret = ltr381rgb_set_calibscale(ltr381rgb, chan, val, val2);
		break;
	case IIO_CHAN_INFO_HARDWAREGAIN:
		ret = ltr381rgb_set_gain_range(ltr381rgb, val);
		break;
	case IIO_CHAN_INFO_SAMP_FREQ:
		ret = ltr381rgb_set_measurement_rate(ltr381rgb, val, val2);
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&ltr381rgb->lock);

	return ret;
}

static const struct iio_info ltr381rgb_info = {
	.read_raw		= ltr381rgb_read_raw,
	.write_raw		= ltr381rgb_write_raw,
	.attrs			= &ltr381rgb_attribute_group,
};

static void ltr381rgb_parse_dts(struct ltr381rgb_data *ltr381rgb)
{
	struct device_node *node = ltr381rgb->client->dev.of_node;
	int ret;
	u32 val;

	ret = of_property_read_u32(node, "ltr381rgb,ir_factor", &val);
	if (!ret) {
		ltr381rgb->ir_factor = val;
	} else {
		ltr381rgb->ir_factor = 1000000;
	}

	ret = of_property_read_u32(node, "ltr381rgb,r_factor", &val);
	if (!ret) {
		ltr381rgb->r_factor = val;
	} else {
		ltr381rgb->r_factor = 1000000;
	}

	ret = of_property_read_u32(node, "ltr381rgb,g_factor", &val);
	if (!ret) {
		ltr381rgb->g_factor = val;
	} else {
		ltr381rgb->g_factor = 1000000;
	}

	ret = of_property_read_u32(node, "ltr381rgb,b_factor", &val);
	if (!ret) {
		ltr381rgb->b_factor = val;
	} else {
		ltr381rgb->b_factor = 1000000;
	}

	ret = of_property_read_u32(node, "ltr381rgb,hardwaregain", &val);
	if (!ret) {
		ltr381rgb->hardwaredgain = val;
	} else {
		ltr381rgb->hardwaredgain = 3;
	}

	ret = of_property_read_u32(node, "ltr381rgb,sample_rate", &val);
	if (!ret) {
		ltr381rgb->sample_rate = val;
	} else {
		ltr381rgb->sample_rate = 10;
	}

	ret = of_property_read_u32(node, "ltr381rgb,resolution", &val);
	if (!ret) {
		ltr381rgb->resolution = val;
	} else {
		ltr381rgb->resolution = 18;
	}
	/*
	ret = of_property_read_u32(node, "ltr381rgb,upper_threshold", &val);
	if (!ret) {
		ltr381rgb->upper_threshold = val;
	} else {
		ltr381rgb->upper_threshold = 0xfffff;
	}

	ret = of_property_read_u32(node, "ltr381rgb,lower_threshold", &val);
	if (!ret) {
		ltr381rgb->lower_threshold = val;
	} else {
		ltr381rgb->lower_threshold = 0;
	}
	*/
}

static void ltr381rgb_init(struct ltr381rgb_data *ltr381rgb)
{
	// ALS in Active Mode, CS mode = CS
	ltr381rgb_set_mode(ltr381rgb, LTR381_MODE_ACTIVE);
	ltr381rgb_set_cs_mode(ltr381rgb, LTR381_MODE_CS);
	// Resolution = 18 bit, Meas Rate = 100ms
	ltr381rgb_set_resolution(ltr381rgb, ltr381rgb->resolution);
	ltr381rgb_set_measurement_rate(ltr381rgb, ltr381rgb->sample_rate, 0);
	// Gain = 3
	ltr381rgb_set_gain_range(ltr381rgb, ltr381rgb->hardwaredgain);
}

static void ltr381rgb_deinit(struct ltr381rgb_data *ltr381rgb)
{
	ltr381rgb_set_mode(ltr381rgb, LTR381_MODE_STANDBY);
}

static int ltr381rgb_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct iio_dev *indio_dev;
	struct ltr381rgb_data *ltr381rgb = NULL;
	struct device *dev = &client->dev;
	int ret = 0;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*ltr381rgb));
	if (!indio_dev) {
		dev_err(&client->dev, "iio allocation failed!\n");
		return -ENOMEM;
	}

	ltr381rgb = iio_priv(indio_dev);
	ltr381rgb->client = client;
	i2c_set_clientdata(client, indio_dev);
	mutex_init(&ltr381rgb->lock);

	indio_dev->dev.parent = &client->dev;
	indio_dev->info = &ltr381rgb_info;
	indio_dev->name = "ltr381rgb";
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = ltr381rgb_channels;
	indio_dev->num_channels = ARRAY_SIZE(ltr381rgb_channels);

	ret = ltr381rgb_check_id(ltr381rgb);
	if (ret < 0) {
		return ret;
	}

	ltr381rgb_parse_dts(ltr381rgb);

	ltr381rgb_init(ltr381rgb);

	ret = devm_iio_device_register(dev, indio_dev);
	if (ret < 0) {
		dev_err(&client->dev, "device_register failed\n");
		return ret;
	}

	dev_info(dev, "ltr381rgb probe success\n");

	return 0;
}

static int ltr381rgb_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);
	struct ltr381rgb_data *ltr381rgb = iio_priv(indio_dev);

	ltr381rgb_deinit(ltr381rgb);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ltr381rgb_suspend(struct device *dev)
{
	struct ltr381rgb_data *ltr381rgb;

	ltr381rgb = iio_priv(i2c_get_clientdata(to_i2c_client(dev)));

	return ltr381rgb_set_mode(ltr381rgb, LTR381_MODE_STANDBY);
}

static int ltr381rgb_resume(struct device *dev)
{
	struct ltr381rgb_data *ltr381rgb;

	ltr381rgb = iio_priv(i2c_get_clientdata(to_i2c_client(dev)));

	return ltr381rgb_set_mode(ltr381rgb, LTR381_MODE_ACTIVE);
}

static SIMPLE_DEV_PM_OPS(ltr381rgb_pm_ops, ltr381rgb_suspend, ltr381rgb_resume);

#define ltr381rgb_PM_OPS (&ltr381rgb_pm_ops)
#else
#define ltr381rgb_PM_OPS NULL
#endif

static const struct of_device_id ltr381rgb_dt_match[] = {
	{.compatible = "ltr381rgb", },
	{},
};
MODULE_DEVICE_TABLE(of, ltr381rgb_dt_match);

static const struct i2c_device_id ltr381rgb_ids[] = {
	{"ltr381rgb", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, ltr381rgb_ids);

static struct i2c_driver ltr381rgb_i2c_driver = {
	.probe = ltr381rgb_probe,
	.remove = ltr381rgb_remove,
	.driver = {
		.name = "ltr381rgb_i2c_driver",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ltr381rgb_dt_match),
		.pm = ltr381rgb_PM_OPS,
	},
	.id_table = ltr381rgb_ids,
};
module_i2c_driver(ltr381rgb_i2c_driver);

MODULE_AUTHOR("<zhiwen.liang@hollyland-tech.com>");
MODULE_DESCRIPTION("Driver for ltr381rgb");
MODULE_LICENSE("GPL and additional rights");
