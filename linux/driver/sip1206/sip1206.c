#include <linux/bitfield.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/regmap.h>
#include <linux/gpio/consumer.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/trigger.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/buffer.h>

#define SIP1206_DRV_NAME "sip1206"

#define SIP1206_CLKCTRL 0x02
#define SIP1206_LOWPOWER_EN	BIT(6)
#define SIP1206_CLK_FLK_EN	BIT(5)
#define SIP1206_CLK_ALS_EN	BIT(4)
#define SIP1206_CLK_EFUSE_EN	BIT(1)
#define SIP1206_CLK_DIG_EN	BIT(0)

#define SIP1206_ID	0x3
#define SIP1206_ID_VAL	0xE

#define SIP1206_CTRL	0x05
#define SIP1206_SOFT_RST_N	BIT(0)
#define SIP1206_OSC_EN		BIT(1)

#define SIP1206_INTCTRL	0x06
#define SIP1206_INT_EN		BIT(0)
#define SIP1206_INT_POLARITY	BIT(1)
#define SIP1206_INT_CLR_MODE	BIT(2)

#define SIP1206_ALSENABLE	0x50
#define SIP1206_ALS_EN		BIT(0)
#define SIP1206_ALS_R_EN	BIT(1)
#define SIP1206_ALS_G_EN	BIT(2)
#define SIP1206_ALS_B_EN	BIT(3)
#define SIP1206_ALS_IR_EN	BIT(4)
#define SIP1206_ALS_FLK_EN	BIT(5)

#define SIP1206_ALS_CTRL0	0x51
#define SIP1206_ALSPERIOD_EN	BIT(0)

#define SIP1206_ALS_CTRL1	0x52
#define SIP1206_ALS_PERSIST_NUM	GENMASK(7, 4)
#define SIP1206_ALS_TRIG_MODE	GENMASK(1, 0)

#define SIP1206_ALSINT_EN	0x53
#define SIP1206_ALS_CH_INT_SEL		GENMASK(7, 6)
#define SIP1206_ALS_ERR_INT_EN		BIT(3)
#define SIP1206_ALS_ANA_SAT_INT_EN	BIT(2)
#define SIP1206_ALS_DIG_SAT_INT_EN	BIT(1)
#define SIP1206_ALS_INT_EN		BIT(0)

#define SIP1206_ALS_THLOW_H	0x54
#define SIP1206_ALS_THLOW_L	0x55
#define SIP1206_ALS_THHIGH_H	0x56
#define SIP1206_ALS_THHIGH_L	0x57

#define SIP1206_ALSINTE_TIME_H	0x58
#define SIP1206_ALS_INTE_TIME_H	GENMASK(1, 0)
#define SIP1206_ALSINTE_TIME_M	0x59
#define SIP1206_ALSINTE_TIME_L	0x5A

#define SIP1206_ALSPERIOD_STEP	0x5B
#define SIP1206_ALSPERIOD_TIME	0x5C
#define SIP1206_ALS_RST_NUM	0x5D
#define SIP1206_ALSAZ_CTRL	0x5E
#define SIP1206_ALSAZ_EN	0x5F

#define SIP1206_ALS_GAIN_1_0	0x61
#define SIP1206_ALS1_GAIN_MASK	GENMASK(7, 4)
#define SIP1206_ALS0_GAIN_MASK	GENMASK(3, 0)

#define SIP1206_ALS_GAIN_3_2	0x62
#define SIP1206_ALS3_GAIN_MASK	GENMASK(7, 4)
#define SIP1206_ALS2_GAIN_MASK	GENMASK(3, 0)

#define SIP1206_ALS_GAIN_4_N	0x63
#define SIP1206_ALS4_GAIN_MASK	GENMASK(3, 0)

#define SIP1206_ALS_GAIN_0P25	0x0
#define SIP1206_ALS_GAIN_0P5	0x1
#define SIP1206_ALS_GAIN_1	0x2
#define SIP1206_ALS_GAIN_2	0x3
#define SIP1206_ALS_GAIN_4	0x4
#define SIP1206_ALS_GAIN_8	0x5
#define SIP1206_ALS_GAIN_16	0x6
#define SIP1206_ALS_GAIN_32	0x7
#define SIP1206_ALS_GAIN_64	0x8
#define SIP1206_ALS_GAIN_128	0x9
#define SIP1206_ALS_GAIN_256	0xA
#define SIP1206_ALS_GAIN_512	0xB
#define SIP1206_ALS_GAIN_1024	0xC

#define SIP1206_FLKINTE_TIME_H	0x73
#define SIP1206_FLKINTE_TIME_M	0x74
#define SIP1206_FLKINTE_TIME_L	0x75

#define SIP1206_ALSINT_STATUS	0x81
#define SIP1206_FIFO_INT	BIT(7)
#define SIP1206_SYNC_CHG_INT	BIT(6)
#define SIP1206_SYNC_LOST_INT	BIT(5)
#define SIP1206_FLK_SAT_INT	BIT(4)
#define SIP1206_ALS_ERR_INT	BIT(3)
#define SIP1206_ALS_ANA_SAT_INT	BIT(2)
#define SIP1206_ALS_DIG_SAT_INT	BIT(1)
#define SIP1206_ALS_INT		BIT(0)

#define SIP1206_DATA_VALID	0x84
#define SIP1206_ALS_DATA_VALID	BIT(2)

#define SIP1206_ALS_RDATA_H	0x90
#define SIP1206_ALS_RDATA_L	0x91
#define SIP1206_ALS_GDATA_H	0x92
#define SIP1206_ALS_GDATA_L	0x93
#define SIP1206_ALS_BDATA_H	0x94
#define SIP1206_ALS_BDATA_L	0x95
#define SIP1206_ALS_IRDATA_H	0x96
#define SIP1206_ALS_IRDATA_L	0x97

/* FIFO */
#define SIP1206_FDATA	0x9F
#define SIP1206_FSTATUS	0xA0
#define SIP1206_FIFO_LVL_L	GENMASK(7, 5)
#define SIP1206_FIFO_FULL_INT	BIT(4)
#define SIP1206_FIFO_EMPTY_INT	BIT(3)
#define SIP1206_FIFO_AF_INT	BIT(2)
#define SIP1206_FIFO_AE_INT	BIT(1)

#define SIP1206_FLVL	0xA1
#define SIP1206_FIFO_LVL_H	GENMASK(6, 0)
#define SIP1206_FIFO_LVL_MASK	GENMASK(9, 0)

#define SIP1206_F_THRESH	0xA2
#define SIP1206_FIFO_FULL_TH	GENMASK(7, 5)
#define SIP1206_FIFO_EMPTY_TH	GENMASK(4, 2)
#define SIP1206_FIFO_INTE_ALMOST_FULL_VALID	BIT(1)
#define SIP1206_FIFO_INTE_ALMOST_EMPTY_VALID	BIT(0)

#define SIP1206_FMODE0	0xA3
#define SIP1206_FIFO_AL_FLAG_STICKY_EN	BIT(6)
#define SIP1206_FIFO_INT_EN	BIT(5)
#define SIP1206_FIFO_CLEAR	BIT(4)
#define SIP1206_FIFO_MODE	BIT(3)

#define SIP1206_FMODE1	0xA4
#define SIP1206_SAVE_DATA_TO_FIFO	BIT(4)
#define SIP1206_ALS_FIFO_DATA_SRC_SEL	GENMASK(3, 0)

#define SIP1206_SYNC_CTRL	0xE0
#define SIP1206_SYNC_LOST_FSM_EN	BIT(4)
#define SIP1206_SYNC_FREQ_INT_EN	BIT(3)
#define SIP1206_SYNC_LOST_INT_EN	BIT(2)
#define SIP1206_SYNC_POL		BIT(1)
#define SIP1206_WDT_EN			BIT(0)

#define SIP1206_SYNC_DLY_CNT_H	0xE2
#define SIP1206_SYNC_DLY_CNT_L	0xE3

#define SIP1206_SYNCTRIG_CNT	0xE4

#define SIP1206_SSYNCWDT_CNT_H	0xE8
#define SIP1206_SSYNCWDT_CNT_L	0xE9

#define SIP1206_SYNC_FRQCHG_THRESH_H	0xEA
#define SIP1206_SYNC_FRQCHG_THRESH_L	0xEB

#define SIP1206_SYNCPERIOD_H	0xEC
#define SIP1206_SYNCPERIOD_M	0xED
#define SIP1206_SYNCPERIOD_L	0xEE

#define SIP1206_FIFO_LEN 512

#define SIP1206_ALL_CHANNEL_MASK GENMASK(4, 0)
#define SIP1206_ALL_CHANNEL_SIZE 10

struct sip1206_data {
	struct i2c_client *client;
	struct regmap *regmap;
	struct mutex lock;

	u8 chn0_gain_idx;
	u8 chn1_gain_idx;
	u8 chn2_gain_idx;
	u8 chn3_gain_idx;
	u8 chn4_gain_idx;
	u8 fifo_mode;

	s16 buffer[8];
};

#define SIP1206_LIGHT_CHANNEL(index, reg, axis) { \
	.type = IIO_LIGHT, \
	.address = reg, \
	.modified = 1, \
	.channel2 = IIO_MOD_LIGHT_##axis, \
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | \
		BIT(IIO_CHAN_INFO_HARDWAREGAIN), \
	.scan_index = index, \
	.scan_type = { \
		.sign = 'u', \
		.realbits = 16, \
		.storagebits = 16, \
		.shift = 0, \
		.endianness = IIO_CPU, \
	}, \
}

static const struct iio_chan_spec sip1206_channels[] = {
	SIP1206_LIGHT_CHANNEL(0, SIP1206_ALS_RDATA_H, RED),
	SIP1206_LIGHT_CHANNEL(1, SIP1206_ALS_GDATA_H, GREEN),
	SIP1206_LIGHT_CHANNEL(2, SIP1206_ALS_BDATA_H, BLUE),
	SIP1206_LIGHT_CHANNEL(3, SIP1206_ALS_IRDATA_H, IR),
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
			BIT(IIO_CHAN_INFO_HARDWAREGAIN),
		.scan_index = 4,
		.scan_type = {
			.sign = 'u',
			.realbits = 16,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_CPU,
		},
	},
	IIO_CHAN_SOFT_TIMESTAMP(5)
};

static const struct {
	u8 reg_val;
	u16 gain;
} sip1206_als_gain_table[] = {
	{SIP1206_ALS_GAIN_1, 1},
	{SIP1206_ALS_GAIN_2, 2},
	{SIP1206_ALS_GAIN_4, 4},
	{SIP1206_ALS_GAIN_8, 8},
	{SIP1206_ALS_GAIN_16, 16},
	{SIP1206_ALS_GAIN_32, 32},
	{SIP1206_ALS_GAIN_64, 64},
	{SIP1206_ALS_GAIN_128, 128},
	{SIP1206_ALS_GAIN_256, 256},
	{SIP1206_ALS_GAIN_512, 512},
	{SIP1206_ALS_GAIN_1024, 1024},
};

static IIO_CONST_ATTR(in_illuminance_gain_available, "1024 512 256 128 64 32 16 8 4 2 1");

static int sip1206_set_chn0_gain(struct sip1206_data *sip1206, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sip1206_als_gain_table); i++) {
		if (sip1206_als_gain_table[i].gain == val) {
			sip1206->chn0_gain_idx = i;

			return regmap_update_bits(sip1206->regmap, SIP1206_ALS_GAIN_1_0,
				SIP1206_ALS0_GAIN_MASK, sip1206_als_gain_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int sip1206_get_chn0_gain(struct sip1206_data *sip1206, int *val)
{
	int reg_val;
	int i;

	regmap_read(sip1206->regmap, SIP1206_ALS_GAIN_1_0, &reg_val);
	reg_val &= SIP1206_ALS0_GAIN_MASK;
	for (i = 0; i < ARRAY_SIZE(sip1206_als_gain_table); i++) {
		if (reg_val == sip1206_als_gain_table[i].reg_val) {
			*val = sip1206_als_gain_table[i].gain;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static int sip1206_set_chn1_gain(struct sip1206_data *sip1206, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sip1206_als_gain_table); i++) {
		if (sip1206_als_gain_table[i].gain == val) {
			sip1206->chn1_gain_idx = i;

			return regmap_update_bits(sip1206->regmap, SIP1206_ALS_GAIN_1_0,
				SIP1206_ALS1_GAIN_MASK, sip1206_als_gain_table[i].reg_val << 4);
		}
	}

	return -EINVAL;
}

static int sip1206_get_chn1_gain(struct sip1206_data *sip1206, int *val)
{
	int reg_val;
	int i;

	regmap_read(sip1206->regmap, SIP1206_ALS_GAIN_1_0, &reg_val);
	reg_val &= SIP1206_ALS1_GAIN_MASK;
	reg_val = (reg_val >> 4);
	for (i = 0; i < ARRAY_SIZE(sip1206_als_gain_table); i++) {
		if (reg_val == sip1206_als_gain_table[i].reg_val) {
			*val = sip1206_als_gain_table[i].gain;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static int sip1206_set_chn2_gain(struct sip1206_data *sip1206, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sip1206_als_gain_table); i++) {
		if (sip1206_als_gain_table[i].gain == val) {
			sip1206->chn2_gain_idx = i;

			return regmap_update_bits(sip1206->regmap, SIP1206_ALS_GAIN_3_2,
				SIP1206_ALS2_GAIN_MASK, sip1206_als_gain_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int sip1206_get_chn2_gain(struct sip1206_data *sip1206, int *val)
{
	int reg_val;
	int i;

	regmap_read(sip1206->regmap, SIP1206_ALS_GAIN_3_2, &reg_val);
	reg_val &= SIP1206_ALS2_GAIN_MASK;
	for (i = 0; i < ARRAY_SIZE(sip1206_als_gain_table); i++) {
		if (reg_val == sip1206_als_gain_table[i].reg_val) {
			*val = sip1206_als_gain_table[i].gain;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static int sip1206_set_chn3_gain(struct sip1206_data *sip1206, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sip1206_als_gain_table); i++) {
		if (sip1206_als_gain_table[i].gain == val) {
			sip1206->chn3_gain_idx = i;

			return regmap_update_bits(sip1206->regmap, SIP1206_ALS_GAIN_3_2,
				SIP1206_ALS3_GAIN_MASK, sip1206_als_gain_table[i].reg_val << 4);
		}
	}

	return -EINVAL;
}

static int sip1206_get_chn3_gain(struct sip1206_data *sip1206, int *val)
{
	int reg_val;
	int i;

	regmap_read(sip1206->regmap, SIP1206_ALS_GAIN_3_2, &reg_val);
	reg_val &= SIP1206_ALS3_GAIN_MASK;
	reg_val = (reg_val >> 4);
	for (i = 0; i < ARRAY_SIZE(sip1206_als_gain_table); i++) {
		if (reg_val == sip1206_als_gain_table[i].reg_val) {
			*val = sip1206_als_gain_table[i].gain;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static int sip1206_set_chn4_gain(struct sip1206_data *sip1206, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sip1206_als_gain_table); i++) {
		if (sip1206_als_gain_table[i].gain == val) {
			sip1206->chn4_gain_idx = i;

			return regmap_update_bits(sip1206->regmap, SIP1206_ALS_GAIN_4_N,
				SIP1206_ALS4_GAIN_MASK, sip1206_als_gain_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int sip1206_get_chn4_gain(struct sip1206_data *sip1206, int *val)
{
	int reg_val;
	int i;

	regmap_read(sip1206->regmap, SIP1206_ALS_GAIN_4_N, &reg_val);
	reg_val &= SIP1206_ALS4_GAIN_MASK;
	for (i = 0; i < ARRAY_SIZE(sip1206_als_gain_table); i++) {
		if (reg_val == sip1206_als_gain_table[i].reg_val) {
			*val = sip1206_als_gain_table[i].gain;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static int sip1206_set_chn_gain(struct sip1206_data *sip1206,
	struct iio_chan_spec const *chan, int val)
{
	int ret = -EINVAL;

	switch(chan->channel2) {
	case IIO_MOD_LIGHT_RED:
		ret = sip1206_set_chn0_gain(sip1206, val);
		break;
	case IIO_MOD_LIGHT_GREEN:
		ret = sip1206_set_chn1_gain(sip1206, val);
		break;
	case IIO_MOD_LIGHT_BLUE:
		ret = sip1206_set_chn2_gain(sip1206, val);
		break;
	case IIO_MOD_LIGHT_IR:
		ret = sip1206_set_chn3_gain(sip1206, val);
		break;
	default:
		ret = sip1206_set_chn4_gain(sip1206, val);
	}

	return ret;
}

static int sip1206_get_chn_gain(struct sip1206_data *sip1206,
	struct iio_chan_spec const *chan, int *val)
{
	int ret;

	switch(chan->channel2) {
	case IIO_MOD_LIGHT_RED:
		ret = sip1206_get_chn0_gain(sip1206, val);
		break;
	case IIO_MOD_LIGHT_GREEN:
		ret = sip1206_get_chn1_gain(sip1206, val);
		break;
	case IIO_MOD_LIGHT_BLUE:
		ret = sip1206_get_chn2_gain(sip1206, val);
		break;
	case IIO_MOD_LIGHT_IR:
		ret = sip1206_get_chn3_gain(sip1206, val);
		break;
	default:
		ret = sip1206_get_chn4_gain(sip1206, val);
	}

	return ret;
}

static int sip1206_get_chn_raw_data(struct sip1206_data *sip1206, u32 addr, int *val)
{
	u16 data;

	regmap_bulk_read(sip1206->regmap, addr, &data, 2);
	*val =  be16_to_cpu(data);

	return IIO_VAL_INT;
}

static ssize_t reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct sip1206_data *sip1206 = iio_priv(indio_dev);
	u32 reg_val = 0;
	u32 i = 0;
	ssize_t len = 0;

	mutex_lock(&sip1206->lock);
	for (i = 0; i <= SIP1206_SYNCPERIOD_L; i++) {
		regmap_read(sip1206->regmap, i, &reg_val);
		len += snprintf(buf + len, PAGE_SIZE - len, "reg:0x%02x=0x%02x\n", i, reg_val);
	}
	mutex_unlock(&sip1206->lock);

	return len;
}

static ssize_t reg_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t len)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct sip1206_data *sip1206 = iio_priv(indio_dev);
	u32 databuf[2];

	mutex_lock(&sip1206->lock);
	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2)
		regmap_write(sip1206->regmap, databuf[0], databuf[1]);
	mutex_unlock(&sip1206->lock);

	return len;
}

static IIO_DEVICE_ATTR_RW(reg,   0);

static IIO_CONST_ATTR(fifo_mode_available, "als flicker");

static ssize_t fifo_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct sip1206_data *sip1206 = iio_priv(indio_dev);
	ssize_t len = 0;
	int reg_val;

	mutex_lock(&sip1206->lock);
	regmap_read(sip1206->regmap, SIP1206_FMODE1, &reg_val);
	reg_val &= SIP1206_SAVE_DATA_TO_FIFO;
	mutex_unlock(&sip1206->lock);

	if (reg_val) {
		len = snprintf(buf, PAGE_SIZE - len, "als\n");
	} else {
		len = snprintf(buf, PAGE_SIZE - len, "flicker\n");
	}

	return len;
}

static ssize_t fifo_mode_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t len)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct sip1206_data *sip1206 = iio_priv(indio_dev);

	mutex_lock(&sip1206->lock);
	if (!strncmp(buf, "als", 3)) {
		regmap_update_bits(sip1206->regmap, SIP1206_FMODE0,
			SIP1206_FIFO_MODE, 0x00);
		regmap_update_bits(sip1206->regmap, SIP1206_FMODE1,
			SIP1206_SAVE_DATA_TO_FIFO, 0xff);
		sip1206->fifo_mode = 1;
	} else if (!strncmp(buf, "flicker", 7)) {
		regmap_update_bits(sip1206->regmap, SIP1206_FMODE0,
			SIP1206_FIFO_MODE, 0xff);
		regmap_update_bits(sip1206->regmap, SIP1206_FMODE1,
			SIP1206_SAVE_DATA_TO_FIFO, 0);
		sip1206->fifo_mode = 0;
	} else {
		dev_err(&sip1206->client->dev, "Invalid mode\n");
	}
	mutex_unlock(&sip1206->lock);

	return len;
}

static IIO_DEVICE_ATTR_RW(fifo_mode,   0);

static struct attribute *sip1206_attributes[] = {
	&iio_const_attr_in_illuminance_gain_available.dev_attr.attr,
	&iio_const_attr_fifo_mode_available.dev_attr.attr,
	&iio_dev_attr_fifo_mode.dev_attr.attr,
	&iio_dev_attr_reg.dev_attr.attr,
	NULL,
};

static const struct attribute_group sip1206_attribute_group = {
	.attrs = sip1206_attributes,
};

static int sip1206_read_raw(struct iio_dev *indio_dev,
			   struct iio_chan_spec const *chan, int *val,
			   int *val2, long mask)
{
	struct sip1206_data *sip1206 = iio_priv(indio_dev);
	int ret = -EINVAL;

	mutex_lock(&sip1206->lock);
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if (chan->type == IIO_LIGHT)
			ret = sip1206_get_chn_raw_data(sip1206, chan->address, val);
		break;
	case IIO_CHAN_INFO_HARDWAREGAIN:
		ret = sip1206_get_chn_gain(sip1206, chan, val);
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&sip1206->lock);

	return ret;
}

static int sip1206_write_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan, int val,
			    int val2, long mask)
{
	struct sip1206_data *sip1206 = iio_priv(indio_dev);
	int ret;

	mutex_lock(&sip1206->lock);
	switch (mask) {
	case IIO_CHAN_INFO_HARDWAREGAIN:
		ret = sip1206_set_chn_gain(sip1206, chan, val);
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&sip1206->lock);

	return ret;
}

static const struct iio_info sip1206_info = {
	.read_raw	= sip1206_read_raw,
	.write_raw	= sip1206_write_raw,
	.attrs		= &sip1206_attribute_group,
};

static u16 sip1206_read_fifo_level(struct sip1206_data *sip1206)
{
	u16 fifo_level = 0;

	regmap_bulk_read(sip1206->regmap, SIP1206_FSTATUS, &fifo_level, 2);

	return (fifo_level >> 5) & SIP1206_FIFO_LVL_MASK;
}

static irqreturn_t sip1206_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *i_dev = pf->indio_dev;
	struct sip1206_data *sip1206 = iio_priv(i_dev);
	int temp;
	int i = 0;

	mutex_lock(&sip1206->lock);
	if (sip1206->fifo_mode) {
		for_each_set_bit(i, i_dev->active_scan_mask, i_dev->masklength) {
			const struct iio_chan_spec *chan = &sip1206_channels[i];

			if (i != 4)
				sip1206_get_chn_raw_data(sip1206, chan->address, &temp);
			sip1206->buffer[i++] = temp;
		}
		iio_push_to_buffers_with_timestamp(i_dev, sip1206->buffer, pf->timestamp);
	} else {
		s16 fifo_level = 0;
		u16 fifo_buff[SIP1206_FIFO_LEN / 2];

		fifo_level = sip1206_read_fifo_level(sip1206);
		regmap_raw_read(sip1206->regmap, SIP1206_FDATA, fifo_buff, fifo_level);
		while (fifo_level > 0) {
			sip1206->buffer[0] = be16_to_cpu(fifo_buff[i++]);
			fifo_level -= 2;
			iio_push_to_buffers_with_timestamp(i_dev, sip1206->buffer, pf->timestamp);
		}
	}
	mutex_unlock(&sip1206->lock);

	iio_trigger_notify_done(i_dev->trig);

	return IRQ_HANDLED;
}

static int sip1206_init(struct sip1206_data *sip1206)
{
	regmap_write(sip1206->regmap, SIP1206_CTRL, 0x7);
	regmap_write(sip1206->regmap, SIP1206_ALSENABLE, 0x3F);

	/* default 32x gain */
	sip1206_set_chn0_gain(sip1206, 32);
	sip1206_set_chn1_gain(sip1206, 32);
	sip1206_set_chn2_gain(sip1206, 32);
	sip1206_set_chn3_gain(sip1206, 32);
	sip1206_set_chn4_gain(sip1206, 32);

	regmap_update_bits(sip1206->regmap, SIP1206_FMODE0, SIP1206_FIFO_MODE, 0xff);
	regmap_update_bits(sip1206->regmap, SIP1206_FMODE0, SIP1206_FIFO_CLEAR, 0xff);

	return 0;
}

static void sip1206_deinit(struct sip1206_data *sip1206)
{
	regmap_write(sip1206->regmap, SIP1206_CTRL, 0);
}

static const struct regmap_config sip1206_i2c_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};
static int sip1206_check_id(struct sip1206_data *sip1206)
{
	int expected = 0;

	regmap_read(sip1206->regmap, SIP1206_ID, &expected);
	if (expected != SIP1206_ID_VAL) {
		dev_err(&sip1206->client->dev, "unexpected id 0x%0x\n", expected);
		return -EINVAL;
	}

	return 0;
}

static int sip1206_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct sip1206_data *sip1206;
	struct iio_dev *indio_dev;
	int ret;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*sip1206));
	if (!indio_dev)
		return -ENOMEM;

	sip1206 = iio_priv(indio_dev);
	i2c_set_clientdata(client, indio_dev);
	sip1206->client = client;
	mutex_init(&sip1206->lock);

	indio_dev->info = &sip1206_info;
	indio_dev->name = SIP1206_DRV_NAME;
	indio_dev->channels = sip1206_channels;
	indio_dev->num_channels = ARRAY_SIZE(sip1206_channels);
	indio_dev->modes = INDIO_DIRECT_MODE;
	ret = devm_iio_triggered_buffer_setup(&client->dev, indio_dev,
		iio_pollfunc_store_time, sip1206_trigger_handler, NULL);
	if (ret)
		return ret;

	sip1206->regmap = devm_regmap_init_i2c(client, &sip1206_i2c_regmap_config);
	if (IS_ERR(sip1206->regmap)) {
		dev_err(&client->dev, "Failed to register i2c regmap %d\n",
			(int)PTR_ERR(sip1206->regmap));
		return PTR_ERR(sip1206->regmap);
	}

	ret = sip1206_check_id(sip1206);
	if (ret < 0) {
		return ret;
	}

	sip1206_init(sip1206);
	ret = devm_iio_device_register(&client->dev, indio_dev);
	if (ret < 0) {
		dev_err(&client->dev, "device register failed\n");
		return ret;
	}

	dev_info(&client->dev, "sip1206 probe success\n");

	return 0;
}

void sip1206_remove(struct i2c_client *client)
{
	struct sip1206_data *sip1206 = i2c_get_clientdata(client);

	sip1206_deinit(sip1206);
}

static const struct i2c_device_id sip1206_id[] = {
	{SIP1206_DRV_NAME, },
	{}
};
MODULE_DEVICE_TABLE(i2c, sip1206_id);

static const struct of_device_id sip1206_of_match[] = {
	{ .compatible = "silicon,sip1206", },
	{},
};
MODULE_DEVICE_TABLE(of, sip1206_of_match);

static struct i2c_driver sip1206_driver = {
	.driver = {
		.name = SIP1206_DRV_NAME,
		.of_match_table = sip1206_of_match,
	},
	.probe		= sip1206_probe,
	.remove		= sip1206_remove,
	.id_table	= sip1206_id,
};
module_i2c_driver(sip1206_driver);

MODULE_AUTHOR("<897420073@qq.com>");
MODULE_DESCRIPTION("Driver for sip1206");
MODULE_LICENSE("GPL");
