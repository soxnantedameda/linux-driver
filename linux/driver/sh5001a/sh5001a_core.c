#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/regmap.h>
#include <linux/bitfield.h>
#include <linux/gpio/consumer.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/trigger.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/buffer.h>

#include "sh5001a.h"

#define SH5001A_ALL_CHANNEL_MASK GENMASK(6, 0)
#define SH5001A_ALL_CHANNEL_SIZE 14

struct sh5001a_data {
	struct device *dev;
	struct iio_trigger *dready_trig;
	struct iio_trigger *fifo_trig;
	struct regmap *regmap;
	struct gpio_desc *irq1_gpiod;
	int irq1;
	struct mutex lock;
	bool dready_trigger_on;
	bool fifo_trigger_on;
	int room_temp;

	u8 acc_range_idx;
	u8 acc_sample_idx;
	u8 acc_lpf_idx;
	u8 gyro_range_idx;
	u8 gyro_sample_idx;
	u8 gyro_lpf_idx;
	u8 temp_sample_idx;
	s16 buffer[16];
	s16 fifo_buffer[SH5001_FIFO_BUFFER];
	u32 watermark;

	unsigned char O1_switchpower;
	short v3_acc[3];
	unsigned char use_otp_comp;
};

/* Used to map scan mask bits to their corresponding channel register. */
static const int sh5001a_channel_table[] = {
	SH5001_ACC_XL,
	SH5001_ACC_YL,
	SH5001_ACC_ZL,
	SH5001_GYRO_XL,
	SH5001_GYRO_YL,
	SH5001_GYRO_ZL,
	SH5001_TEMP_ZL
};

#define SH5001A_ACCEL_CHANNEL(index, reg, axis) { \
	.type = IIO_ACCEL, \
	.address = reg, \
	.modified = 1, \
	.channel2 = IIO_MOD_##axis, \
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | \
		BIT(IIO_CHAN_INFO_PROCESSED), \
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) | \
		BIT(IIO_CHAN_INFO_SAMP_FREQ) | \
		BIT(IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY), \
	.scan_index = index, \
	.scan_type = { \
		.sign = 's', \
		.realbits = 16, \
		.storagebits = 16, \
		.shift = 0, \
		.endianness = IIO_CPU, \
	}, \
}

#define SH5001A_ANGL_VEL_CHANNEL(index, reg, axis) { \
	.type = IIO_ANGL_VEL, \
	.address = reg, \
	.modified = 1, \
	.channel2 = IIO_MOD_##axis, \
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | \
		BIT(IIO_CHAN_INFO_PROCESSED), \
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) | \
		BIT(IIO_CHAN_INFO_SAMP_FREQ) | \
		BIT(IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY), \
	.scan_index = index, \
	.scan_type = { \
		.sign = 's', \
		.realbits = 16, \
		.storagebits = 16, \
		.shift = 0, \
		.endianness = IIO_CPU, \
	}, \
}

static const struct iio_chan_spec sh5001a_channels[] = {
	SH5001A_ACCEL_CHANNEL(0, SH5001_ACC_XL, X),
	SH5001A_ACCEL_CHANNEL(1, SH5001_ACC_YL, Y),
	SH5001A_ACCEL_CHANNEL(2, SH5001_ACC_ZL, Z),
	SH5001A_ANGL_VEL_CHANNEL(3, SH5001_GYRO_XL, X),
	SH5001A_ANGL_VEL_CHANNEL(4, SH5001_GYRO_YL, Y),
	SH5001A_ANGL_VEL_CHANNEL(5, SH5001_GYRO_ZL, Z),
	{
		.type = IIO_TEMP,
		.address = SH5001_TEMP_ZL,
		.scan_index = 6,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
			BIT(IIO_CHAN_INFO_PROCESSED),
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SAMP_FREQ),
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_CPU,
		},
	},
	IIO_CHAN_SOFT_TIMESTAMP(7)
};

#define SH5001_ACC_RANGE_MASK GENMASK(6, 4)

static IIO_CONST_ATTR(in_accel_scale_available, "2 4 8 16 32");

static const struct {
	u8 reg_val;
	u16 range;
	u32 uints;
} sh5001a_acc_range_table[] = {
	{SH5001_ACC_RANGE_2G, 2, 61035},
	{SH5001_ACC_RANGE_4G, 4, 122070},
	{SH5001_ACC_RANGE_8G, 8, 244140},
	{SH5001_ACC_RANGE_16G, 16, 488281},
	{SH5001_ACC_RANGE_32G, 32, 976562},
};

static int sh5001a_set_acc_scale(struct sh5001a_data *sh5001a, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sh5001a_acc_range_table); i++) {
		if (sh5001a_acc_range_table[i].range == val) {
			sh5001a->acc_range_idx = i;

			return regmap_update_bits(sh5001a->regmap, SH5001_ACC_CONF1,
				SH5001_ACC_RANGE_MASK, sh5001a_acc_range_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int sh5001a_get_acc_scale(struct sh5001a_data *sh5001a, int *val)
{
	u32 reg_val;
	int i;

	regmap_read(sh5001a->regmap, SH5001_ACC_CONF1, &reg_val);
	reg_val &= SH5001_ACC_RANGE_MASK;
	for (i = 0; i < ARRAY_SIZE(sh5001a_acc_range_table); i++) {
		if (sh5001a_acc_range_table[i].reg_val == reg_val) {
			*val = sh5001a_acc_range_table[i].range;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static const struct {
	u8 reg_val;
	u16 samp_freq;
} sh5001a_acc_samp_freq_table[] = {
	{SH5001_ACC_ODR_125HZ, 125},
	{SH5001_ACC_ODR_250HZ, 250},
	{SH5001_ACC_ODR_500HZ, 500},
	{SH5001_ACC_ODR_1000HZ, 1000},
	{SH5001_ACC_ODR_2000HZ, 2000},
	{SH5001_ACC_ODR_4000HZ, 4000},
	{SH5001_ACC_ODR_8000HZ, 8000}
};

#define SH5001_ACC_ODR_MASK GENMASK(3, 0)

static IIO_CONST_ATTR(in_accel_sampling_frequency_available,
	"125 250 500 1000 2000 4000 8000");

static int sh5001a_set_acc_sampel_rate(struct sh5001a_data *sh5001a, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sh5001a_acc_samp_freq_table); i++) {
		if (sh5001a_acc_samp_freq_table[i].samp_freq == val) {
			sh5001a->acc_sample_idx = i;

			return regmap_update_bits(sh5001a->regmap, SH5001_ACC_CONF1,
				SH5001_ACC_ODR_MASK, sh5001a_acc_samp_freq_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int sh5001a_get_acc_sampel_rate(struct sh5001a_data *sh5001a, int *val)
{
	u32 reg_val;
	int i;

	regmap_read(sh5001a->regmap, SH5001_ACC_CONF1, &reg_val);
	reg_val &= SH5001_ACC_ODR_MASK;
	for (i = 0; i < ARRAY_SIZE(sh5001a_acc_samp_freq_table); i++) {
		if (sh5001a_acc_samp_freq_table[i].reg_val == reg_val) {
			*val = sh5001a_acc_samp_freq_table[i].samp_freq;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

#define SH5001_ACC_ODRX_MASK GENMASK(3, 0)

static IIO_CONST_ATTR(in_accel_filter_low_pass_3db_frequency_available,
	"0.40 0.36 0.32 0.28 0.24 0.20 0.16 0.14 0.12 0.10 0.08 0.06 0.04 0.03 0.02 0.01");

static const struct {
	u8 reg_val;
	u32 odrx_factor;
} sh5001a_acc_lpf_table[] = {
	{SH5001_ACC_ODRX040, 400000},
	{SH5001_ACC_ODRX036, 360000},
	{SH5001_ACC_ODRX032, 320000},
	{SH5001_ACC_ODRX028, 280000},
	{SH5001_ACC_ODRX024, 240000},
	{SH5001_ACC_ODRX020, 200000},
	{SH5001_ACC_ODRX016, 160000},
	{SH5001_ACC_ODRX014, 140000},
	{SH5001_ACC_ODRX012, 120000},
	{SH5001_ACC_ODRX010, 100000},
	{SH5001_ACC_ODRX008, 80000},
	{SH5001_ACC_ODRX006, 60000},
	{SH5001_ACC_ODRX004, 40000},
	{SH5001_ACC_ODRX003, 30000},
	{SH5001_ACC_ODRX002, 20000},
	{SH5001_ACC_ODRX001, 10000}
};

static int sh5001a_set_acc_low_pass_filter(struct sh5001a_data *sh5001a, int val2)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sh5001a_acc_lpf_table); i++) {
		if (sh5001a_acc_lpf_table[i].odrx_factor == val2) {
			sh5001a->acc_lpf_idx = i;

			return regmap_update_bits(sh5001a->regmap, SH5001_ACC_CONF2,
				SH5001_ACC_ODRX_MASK, sh5001a_acc_lpf_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int sh5001a_get_acc_low_pass_filter(struct sh5001a_data *sh5001a, int *val2)
{
	u32 reg_val;
	int i;

	regmap_read(sh5001a->regmap, SH5001_ACC_CONF2, &reg_val);
	reg_val &= SH5001_ACC_ODRX_MASK;
	for (i = 0; i < ARRAY_SIZE(sh5001a_acc_lpf_table); i++) {
		if (sh5001a_acc_lpf_table[i].reg_val == reg_val) {
			*val2 = sh5001a_acc_lpf_table[i].odrx_factor;

			return IIO_VAL_INT_PLUS_MICRO;
		}
	}

	return -EINVAL;
}

#define SH5001_GYRO_ODR_MASK GENMASK(3, 0)

static IIO_CONST_ATTR(in_anglvel_sampling_frequency_available,
	"125 250 500 1000 2000 4000 8000 16000");

static const struct {
	u8 reg_val;
	u16 samp_freq;
} sh5001a_gyro_samp_freq_table[] = {
	{SH5001_GYRO_ODR_125HZ, 125},
	{SH5001_GYRO_ODR_250HZ, 250},
	{SH5001_GYRO_ODR_500HZ, 500},
	{SH5001_GYRO_ODR_1000HZ, 1000},
	{SH5001_GYRO_ODR_2000HZ, 2000},
	{SH5001_GYRO_ODR_4000HZ, 4000},
	{SH5001_GYRO_ODR_8000HZ, 8000},
	{SH5001_GYRO_ODR_16000HZ, 16000}
};

static int sh5001a_set_gyro_sampel_rate(struct sh5001a_data *sh5001a, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sh5001a_gyro_samp_freq_table); i++) {
		if (sh5001a_gyro_samp_freq_table[i].samp_freq == val) {
			sh5001a->gyro_sample_idx = i;

			return regmap_update_bits(sh5001a->regmap, SH5001_GYRO_CONF1,
				SH5001_GYRO_ODR_MASK, sh5001a_gyro_samp_freq_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int sh5001a_get_gyro_sampel_rate(struct sh5001a_data *sh5001a, int *val)
{
	u32 reg_val;
	int i;

	regmap_read(sh5001a->regmap, SH5001_GYRO_CONF1, &reg_val);
	reg_val &= SH5001_GYRO_ODR_MASK;
	for (i = 0; i < ARRAY_SIZE(sh5001a_gyro_samp_freq_table); i++) {
		if (sh5001a_gyro_samp_freq_table[i].reg_val == reg_val) {
			*val = sh5001a_gyro_samp_freq_table[i].samp_freq;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

#define SH5001_GYRO_RANGE_MASK GENMASK(6, 4)

static IIO_CONST_ATTR(in_anglvel_scale_available, "31 62 125 250 500 1000 2000 4000");

static const struct {
	u8 reg_val;
	u16 range;
	u32 uints;
} sh5001a_gyro_range_table[] = {
	{SH5001_GYRO_RANGE_31, 31, 954},
	{SH5001_GYRO_RANGE_62, 62, 1908},
	{SH5001_GYRO_RANGE_125, 125, 3816},
	{SH5001_GYRO_RANGE_250, 250, 7633},
	{SH5001_GYRO_RANGE_500, 500, 15267},
	{SH5001_GYRO_RANGE_1000, 1000, 30534},
	{SH5001_GYRO_RANGE_2000, 2000, 61068},
	{SH5001_GYRO_RANGE_4000, 4000, 122136},
};

static int sh5001a_set_gyro_scale(struct sh5001a_data *sh5001a, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sh5001a_gyro_range_table); i++) {
		if (sh5001a_gyro_range_table[i].range == val) {
			sh5001a->gyro_range_idx = i;

			return regmap_update_bits(sh5001a->regmap, SH5001_GYRO_CONF1,
				SH5001_GYRO_RANGE_MASK, sh5001a_gyro_range_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int sh5001a_get_gyro_scale(struct sh5001a_data *sh5001a, int *val)
{
	u32 reg_val;
	int i;

	regmap_read(sh5001a->regmap, SH5001_GYRO_CONF1, &reg_val);
	reg_val &= SH5001_GYRO_RANGE_MASK;
	for (i = 0; i < ARRAY_SIZE(sh5001a_gyro_range_table); i++) {
		if (sh5001a_gyro_range_table[i].reg_val == reg_val) {
			*val = sh5001a_gyro_range_table[i].range;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

#define SH5001_GYRO_ODRX_MASK GENMASK(3, 0)

static IIO_CONST_ATTR(in_anglvel_filter_low_pass_3db_frequency_available,
	"0.40 0.36 0.32 0.28 0.24 0.20 0.16 0.14 0.12 0.10 0.08 0.06 0.04 0.03 0.02 0.01");

static const struct {
	u8 reg_val;
	u32 odrx_factor;
} sh5001a_gyro_lpf_table[] = {
	{SH5001_GYRO_ODRX040, 400000},
	{SH5001_GYRO_ODRX036, 360000},
	{SH5001_GYRO_ODRX032, 320000},
	{SH5001_GYRO_ODRX028, 280000},
	{SH5001_GYRO_ODRX024, 240000},
	{SH5001_GYRO_ODRX020, 200000},
	{SH5001_GYRO_ODRX016, 160000},
	{SH5001_GYRO_ODRX014, 140000},
	{SH5001_GYRO_ODRX012, 120000},
	{SH5001_GYRO_ODRX010, 100000},
	{SH5001_GYRO_ODRX008, 80000},
	{SH5001_GYRO_ODRX006, 60000},
	{SH5001_GYRO_ODRX004, 40000},
	{SH5001_GYRO_ODRX003, 30000},
	{SH5001_GYRO_ODRX002, 20000},
	{SH5001_GYRO_ODRX001, 10000}
};

static int sh5001a_set_gyro_low_pass_filter(struct sh5001a_data *sh5001a, int val2)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sh5001a_gyro_lpf_table); i++) {
		if (sh5001a_gyro_lpf_table[i].odrx_factor == val2) {
			sh5001a->gyro_lpf_idx = i;

			return regmap_update_bits(sh5001a->regmap, SH5001_GYRO_CONF2,
				SH5001_GYRO_ODRX_MASK, sh5001a_gyro_lpf_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int sh5001a_get_gyro_low_pass_filter(struct sh5001a_data *sh5001a, int *val2)
{
	u32 reg_val;
	int i;

	regmap_read(sh5001a->regmap, SH5001_GYRO_CONF2, &reg_val);
	reg_val &= SH5001_GYRO_ODRX_MASK;
	for (i = 0; i < ARRAY_SIZE(sh5001a_gyro_lpf_table); i++) {
		if (sh5001a_gyro_lpf_table[i].reg_val == reg_val) {
			*val2 = sh5001a_gyro_lpf_table[i].odrx_factor;

			return IIO_VAL_INT_PLUS_MICRO;
		}
	}

	return -EINVAL;
}

#define SH5001_TEMP_ZL_MASK GENMASK(7, 0)
#define SH5001_TEMP_ZH_MASK GENMASK(3, 0)
#define SH5001_TEMP_MASK GENMASK(11, 0)
#define SH5001_TEMP_ODR_MASK GENMASK(3, 1)

static IIO_CONST_ATTR(in_temp_sampling_frequency_available, "63 125 250 500");

static const struct {
	u8 reg_val;
	u16 samp_freq;
} sh5001a_temp_samp_freq_table[] = {
	{SH5001_TEMP_ODR_63HZ, 63},
	{SH5001_TEMP_ODR_125HZ, 125},
	{SH5001_TEMP_ODR_250HZ, 250},
	{SH5001_TEMP_ODR_500HZ, 500},
};

static int sh5001a_set_temp_sampel_rate(struct sh5001a_data *sh5001a, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sh5001a_temp_samp_freq_table); i++) {
		if (sh5001a_temp_samp_freq_table[i].samp_freq == val) {
			sh5001a->temp_sample_idx = i;

			return regmap_update_bits(sh5001a->regmap, SH5001_TEMP_CONF0,
				SH5001_TEMP_ODR_MASK, sh5001a_temp_samp_freq_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int sh5001a_get_temp_sampel_rate(struct sh5001a_data *sh5001a, int *val)
{
	u32 reg_val;
	int i;

	regmap_read(sh5001a->regmap, SH5001_TEMP_CONF0, &reg_val);
	reg_val &= SH5001_TEMP_ODR_MASK;
	for (i = 0; i < ARRAY_SIZE(sh5001a_temp_samp_freq_table); i++) {
		if (sh5001a_temp_samp_freq_table[i].reg_val == reg_val) {
			*val = sh5001a_temp_samp_freq_table[i].samp_freq;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}


static int sh5001a_get_real_temp(struct sh5001a_data *sh5001a, int *val)
{
	u32 room_val[2] = {0};
	u32 temp_val[2] = {0};
	u32 temp_data = 0;

	if (!sh5001a->room_temp) {
		regmap_read(sh5001a->regmap, SH5001_TEMP_CONF1, &room_val[0]);
		regmap_read(sh5001a->regmap, SH5001_TEMP_CONF2, &room_val[1]);
		sh5001a->room_temp = ((room_val[1] & SH5001_TEMP_ZH_MASK) << 8) | room_val[0];
	}

	regmap_read(sh5001a->regmap, SH5001_TEMP_ZL, &temp_val[0]);
	regmap_read(sh5001a->regmap, SH5001_TEMP_ZH, &temp_val[1]);
	temp_data = ((temp_val[1] & SH5001_TEMP_ZH_MASK) << 8) | temp_val[0];

	*val = ((int)(temp_data - sh5001a->room_temp)) / 14 + 25;

	return IIO_VAL_INT;
}

static int sh5001a_get_raw_data(struct sh5001a_data *sh5001a, u32 addr, int *val)
{
	regmap_bulk_read(sh5001a->regmap, addr, val, 2);

	return IIO_VAL_INT;
}

static int sh5001a_get_acc_processed(struct sh5001a_data *sh5001a,
	struct iio_chan_spec const *chan, int *val, int *val2)
{
	s64 reg_val;

	sh5001a_get_raw_data(sh5001a, chan->address, (s32 *)&reg_val);
	reg_val = sign_extend64(reg_val, 15);
	reg_val = reg_val * sh5001a_acc_range_table[sh5001a->acc_range_idx].uints;
	*val = reg_val / 1000000;
	*val2 = reg_val % 1000000;

	return IIO_VAL_INT_PLUS_MICRO;
}

static int sh5001a_get_gyro_processed(struct sh5001a_data *sh5001a,
	struct iio_chan_spec const *chan, int *val, int *val2)
{
	s64 reg_val;

	sh5001a_get_raw_data(sh5001a, chan->address, (s32 *)&reg_val);
	reg_val = sign_extend64(reg_val, 15);
	reg_val = reg_val * sh5001a_gyro_range_table[sh5001a->acc_range_idx].uints;
	*val = reg_val / 1000000;
	*val2 = reg_val % 1000000;

	return IIO_VAL_INT_PLUS_MICRO;
}

static int sh5001a_read_raw(struct iio_dev *indio_dev,
	struct iio_chan_spec const *chan, int *val, int *val2, long mask)
{
	struct sh5001a_data *sh5001a = iio_priv(indio_dev);
	int ret = 0;

	mutex_lock(&sh5001a->lock);
	switch(mask) {
	case IIO_CHAN_INFO_RAW:
		ret = sh5001a_get_raw_data(sh5001a, chan->address, val);
		switch(chan->type) {
		case IIO_ACCEL:
		case IIO_ANGL_VEL:
			*val = sign_extend32(*val, 15);
			break;
		case IIO_TEMP:
			*val &= SH5001_TEMP_MASK;
			break;
		default:
			ret = -EINVAL;
		}
		break;
	case IIO_CHAN_INFO_PROCESSED:
		switch(chan->type) {
		case IIO_ACCEL:
			ret = sh5001a_get_acc_processed(sh5001a, chan, val, val2);
			break;
		case IIO_ANGL_VEL:
			ret = sh5001a_get_gyro_processed(sh5001a, chan, val, val2);
			break;
		case IIO_TEMP:
			ret = sh5001a_get_real_temp(sh5001a, val);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	case IIO_CHAN_INFO_SCALE:
		switch(chan->type) {
		case IIO_ACCEL:
			ret = sh5001a_get_acc_scale(sh5001a, val);
			break;
		case IIO_ANGL_VEL:
			ret = sh5001a_get_gyro_scale(sh5001a, val);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	case IIO_CHAN_INFO_SAMP_FREQ:
		switch(chan->type) {
		case IIO_ACCEL:
			ret = sh5001a_get_acc_sampel_rate(sh5001a, val);
			break;
		case IIO_ANGL_VEL:
			ret = sh5001a_get_gyro_sampel_rate(sh5001a, val);
			break;
		case IIO_TEMP:
			ret = sh5001a_get_temp_sampel_rate(sh5001a, val);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	case IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY:
		switch(chan->type) {
		case IIO_ACCEL:
			*val = 0;
			ret = sh5001a_get_acc_low_pass_filter(sh5001a, val2);
			break;
		case IIO_ANGL_VEL:
			*val = 0;
			ret = sh5001a_get_gyro_low_pass_filter(sh5001a, val2);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&sh5001a->lock);

	return ret;
}

static int sh5001a_write_raw(struct iio_dev *indio_dev,
	struct iio_chan_spec const *chan, int val, int val2, long mask)
{
	struct sh5001a_data *sh5001a = iio_priv(indio_dev);
	int ret = -EINVAL;

	mutex_lock(&sh5001a->lock);
	switch(mask) {
	case IIO_CHAN_INFO_SCALE:
		switch(chan->type) {
		case IIO_ACCEL:
			ret = sh5001a_set_acc_scale(sh5001a, val);
			break;
		case IIO_ANGL_VEL:
			ret = sh5001a_set_gyro_scale(sh5001a, val);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	case IIO_CHAN_INFO_SAMP_FREQ:
		switch(chan->type) {
		case IIO_ACCEL:
			ret = sh5001a_set_acc_sampel_rate(sh5001a, val);
			break;
		case IIO_ANGL_VEL:
			ret = sh5001a_set_gyro_sampel_rate(sh5001a, val);
			break;
		case IIO_TEMP:
			ret = sh5001a_set_temp_sampel_rate(sh5001a, val);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	case IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY:
		switch(chan->type) {
		case IIO_ACCEL:
			if (val == 0)
				ret = sh5001a_set_acc_low_pass_filter(sh5001a, val2);
			break;
		case IIO_ANGL_VEL:
			if (val == 0)
				ret = sh5001a_set_gyro_low_pass_filter(sh5001a, val2);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&sh5001a->lock);

	return ret;
}

static ssize_t reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct sh5001a_data *sh5001a = iio_priv(indio_dev);
	u32 reg_val = 0;
	u32 i = 0;
	ssize_t len = 0;

	for (i = 0; i <= 0xff; i++) {
		regmap_read(sh5001a->regmap, i, &reg_val);
		len += snprintf(buf + len, PAGE_SIZE - len, "reg:0x%02x=0x%02x\n", i, reg_val);
	}

	return len;
}

static ssize_t reg_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t len)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct sh5001a_data *sh5001a = iio_priv(indio_dev);
	u32 databuf[2];

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2)
		regmap_write(sh5001a->regmap, databuf[0], databuf[1]);

	return len;
}
static DEVICE_ATTR_RW(reg);

static struct attribute *sh5001a_attributes[] = {
	&iio_const_attr_in_accel_sampling_frequency_available.dev_attr.attr,
	&iio_const_attr_in_accel_scale_available.dev_attr.attr,
	&iio_const_attr_in_accel_filter_low_pass_3db_frequency_available.dev_attr.attr,
	&iio_const_attr_in_anglvel_sampling_frequency_available.dev_attr.attr,
	&iio_const_attr_in_anglvel_scale_available.dev_attr.attr,
	&iio_const_attr_in_anglvel_filter_low_pass_3db_frequency_available.dev_attr.attr,
	&iio_const_attr_in_temp_sampling_frequency_available.dev_attr.attr,
	&dev_attr_reg.attr,
	NULL,
};

static const struct attribute_group sh5001a_attribute_group = {
	.attrs = sh5001a_attributes
};

static ssize_t sh5001a_get_fifo_watermark(struct device *dev,
					       struct device_attribute *attr,
					       char *buf)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct sh5001a_data *sh5001a = iio_priv(indio_dev);
	int wm;

	mutex_lock(&sh5001a->lock);
	wm = sh5001a->watermark;
	mutex_unlock(&sh5001a->lock);

	return sprintf(buf, "%d\n", wm);
}

static IIO_CONST_ATTR(hwfifo_watermark_min, "1");
static IIO_CONST_ATTR(hwfifo_watermark_max,
		      __stringify(SH5001_FIFO_BUFFER));
static IIO_DEVICE_ATTR(hwfifo_watermark, S_IRUGO,
		       sh5001a_get_fifo_watermark, NULL, 0);

static const struct attribute *sh5001a_fifo_attributes[] = {
	&iio_const_attr_hwfifo_watermark_min.dev_attr.attr,
	&iio_const_attr_hwfifo_watermark_max.dev_attr.attr,
	&iio_dev_attr_hwfifo_watermark.dev_attr.attr,
	NULL,
};

static int sh5001a_set_watermark(struct iio_dev *indio_dev, unsigned val)
{
	struct sh5001a_data *sh5001a = iio_priv(indio_dev);

	if (val > SH5001_FIFO_BUFFER)
		val = SH5001_FIFO_BUFFER;

	mutex_lock(&sh5001a->lock);
	sh5001a->watermark = val;
	mutex_unlock(&sh5001a->lock);

	return 0;
}

static const struct iio_info sh5001a_info = {
	.read_raw		= sh5001a_read_raw,
	.write_raw		= sh5001a_write_raw,
	.attrs			= &sh5001a_attribute_group,
	.hwfifo_set_watermark	= sh5001a_set_watermark,
};

static void sh5001a_soft_reset(struct sh5001a_data *sh5001a)
{
	u32 regData = 0;

	regData = 0x01;
	regmap_write(sh5001a->regmap, 0x2b, regData);
	regData = 0x73;
	regmap_write(sh5001a->regmap, 0x00, regData);
	mdelay(100);
}

static void sh5001a_drive_start(struct sh5001a_data *sh5001a)
{
	u32 regData = 0;

	regData = 0x01;
	regmap_write(sh5001a->regmap, 0x2b, regData);
	mdelay(2);
	regData = 0x00;
	regmap_write(sh5001a->regmap, 0x2b, regData);
	mdelay(1);
}

static void sh5001a_adc_reset(struct sh5001a_data *sh5001a)
{
	u32 regData = 0;

	regData = 0x08;
	regmap_write(sh5001a->regmap, 0x30, regData);
	regData = 0x00;
	regmap_write(sh5001a->regmap, 0xd2, regData);
	regData = 0x6B;
	regmap_write(sh5001a->regmap, 0xd1, regData);
	regData = 0x02;
	regmap_write(sh5001a->regmap, 0xd5, regData);
	mdelay(5);
	regData = 0x68;
	regmap_write(sh5001a->regmap, 0xd1, regData);
	mdelay(2);
	regData = 0x00;
	regmap_write(sh5001a->regmap, 0xd5, 0x00);
	regData = 0x00;
	regmap_write(sh5001a->regmap, 0x30, 0x00);
	mdelay(50);
}

static void sh5001a_cva_reset(struct sh5001a_data *sh5001a)
{

	u32 regData = 0;
	u32 regDEData = 0;

	regmap_write(sh5001a->regmap, 0xDE, regDEData);

	regData = regDEData & 0xC7;
	regmap_write(sh5001a->regmap, 0xDE, regData);
	mdelay(5);

	regData = regDEData | 0x38;
	regmap_write(sh5001a->regmap, 0xDE, regData);
	mdelay(5);

	regmap_write(sh5001a->regmap, 0xDE, regDEData);
	mdelay(5);

	regData = 0x12;
	regmap_write(sh5001a->regmap, 0xCD, regData);
	regData = 0x12;
	regmap_write(sh5001a->regmap, 0xCE, regData);
	regData = 0x12;
	regmap_write(sh5001a->regmap, 0xCF, regData);

	mdelay(1);

	regData = 0x2;
	regmap_write(sh5001a->regmap, 0xCD, regData);
	regData = 0x2;
	regmap_write(sh5001a->regmap, 0xCE, regData);
	regData = 0x2;
	regmap_write(sh5001a->regmap, 0xCF, regData);
}

static void sh5001a_module_reset(struct sh5001a_data *sh5001a)
{
	sh5001a_soft_reset(sh5001a);
	sh5001a_drive_start(sh5001a);
	sh5001a_adc_reset(sh5001a);
	sh5001a_cva_reset(sh5001a);
	mdelay(200);
}

static void sh5001a_osc_freq(struct sh5001a_data *sh5001a)
{
	u32 reg_val;

	regmap_read(sh5001a->regmap, 0xda, &reg_val);
	if ((reg_val & 0x07) != 0x07) {
		sh5001a->O1_switchpower = 1;
		reg_val |= 0x07;
		regmap_write(sh5001a->regmap, 0xda, reg_val);
	} else if((reg_val & 0x07) == 0x07) {
		sh5001a->O1_switchpower = 0;
	}
	dev_info(sh5001a->dev, "Chip is %s\r\n", sh5001a->O1_switchpower ?
		"O1":"O2");
}

static void sh5001a_acc_config(struct sh5001a_data *sh5001a,
	u16 accODR, u16 accRange, u32 accCutOffFreq, u8 accFilter, u8 accByPass)
{
	u32 reg_val;

	// enable acc digital filter and bypass or not
	regmap_read(sh5001a->regmap, SH5001_ACC_CONF0, &reg_val);
	reg_val = (reg_val & 0xFCU) | accFilter | accByPass;
	regmap_write(sh5001a->regmap, SH5001_ACC_CONF0, reg_val);

	sh5001a_set_acc_sampel_rate(sh5001a, accODR);
	sh5001a_set_acc_scale(sh5001a, accRange);
	sh5001a_set_acc_low_pass_filter(sh5001a, accCutOffFreq);
}

static void sh5001a_gyro_config(struct sh5001a_data *sh5001a,
	u16 gyroODR, u16 gyroRange, u32 gyroCutOffFreq, u8 gyroFilter, u8 gyroByPass)
{
	u32 reg_val;

	// enable gyro digital filter
	regmap_read(sh5001a->regmap, SH5001_GYRO_CONF0, &reg_val);
	reg_val = (reg_val & 0x7CU) | gyroFilter | gyroByPass;
	regmap_write(sh5001a->regmap, SH5001_GYRO_CONF0, reg_val);

	sh5001a_set_gyro_sampel_rate(sh5001a, gyroODR);
	sh5001a_set_gyro_scale(sh5001a, gyroRange);
	sh5001a_set_gyro_low_pass_filter(sh5001a, gyroCutOffFreq);
}

static void sh5001a_imu_data_stablize(struct sh5001a_data *sh5001a)
{
	u32 reg_val;

	regmap_read(sh5001a->regmap, SH5001_ACC_CONF0, &reg_val);
	reg_val &= 0xFE;
	regmap_write(sh5001a->regmap, SH5001_ACC_CONF0, reg_val);
	reg_val |= 0x1;
	regmap_write(sh5001a->regmap, SH5001_ACC_CONF0, reg_val);

	//gyro
	regmap_read(sh5001a->regmap, SH5001_GYRO_CONF0, &reg_val);
	reg_val &= 0xFE;
	regmap_write(sh5001a->regmap, SH5001_GYRO_CONF0, reg_val);
	reg_val |= 0x1;
	regmap_write(sh5001a->regmap, SH5001_GYRO_CONF0, reg_val);
}

static void sh5001a_temp_config(struct sh5001a_data *sh5001a,
	u8 tempODR, u8 tempEnable)
{
	u32 reg_val;

	// enable temperature, set ODR
	regmap_read(sh5001a->regmap, SH5001_TEMP_CONF0, &reg_val);
	reg_val = (reg_val & 0xF8U) | tempEnable;
	regmap_write(sh5001a->regmap, SH5001_TEMP_CONF0, reg_val);
	sh5001a_set_temp_sampel_rate(sh5001a, tempODR);
}

static void sh5001a_int_config(struct sh5001a_data *sh5001a,
	u8 intLevel, u8 intLatch, u8 intClear, u8 intTime,
	u8 int1Mode, u8 int0Mode, u8 int1OE, u8 int0OE)
{
	u32 reg_val = 0;

	//regmap_read(sh5001a->regmap, SH5001_INT_CONF, 1, &reg_val);

	reg_val = (intLevel == SH5001_INT0_LEVEL_LOW) ?
		(reg_val | SH5001_INT0_LEVEL_LOW) : (reg_val & SH5001_INT0_LEVEL_HIGH);

	reg_val = (intLatch == SH5001_INT_NO_LATCH) ?
		(reg_val | SH5001_INT_NO_LATCH) : (reg_val & SH5001_INT_LATCH);

	reg_val = (intClear == SH5001_INT_CLEAR_ANY) ?
		(reg_val | SH5001_INT_CLEAR_ANY) : (reg_val & SH5001_INT_CLEAR_STATUS);

	reg_val = (int1Mode == SH5001_INT1_OD) ?
		(reg_val | SH5001_INT1_OD) : (reg_val & SH5001_INT1_NORMAL);

	reg_val = (int1OE == SH5001_INT1_OUTPUT) ?
		(reg_val | SH5001_INT1_OUTPUT) : (reg_val & SH5001_INT1_INPUT);

	reg_val = (int0Mode == SH5001_INT0_OD) ?
		(reg_val | SH5001_INT0_OD) : (reg_val & SH5001_INT0_NORMAL);

	reg_val = (int0OE == SH5001_INT0_OUTPUT) ?
		(reg_val | SH5001_INT0_OUTPUT) : (reg_val & SH5001_INT0_INPUT);

	regmap_write(sh5001a->regmap, SH5001_INT_CONF, reg_val);

	if(intLatch == SH5001_INT_NO_LATCH) {
		reg_val = intTime;
		regmap_write(sh5001a->regmap, SH5001_INT_LIMIT, reg_val);
	}
}

static void sh5001a_int_enable(struct sh5001a_data *sh5001a,
	u8 intType,  u8 intEnable, u8 intPinSel)
{
	u32 regData[2] = {0};
	u32 u32IntVal = 0;

	// Z axis change between UP to DOWN
	if((intType & 0x0040U) == SH5001_INT_UP_DOWN_Z) {
		regmap_read(sh5001a->regmap, SH5001_ORIEN_INTCONF0, &regData[0]);
		regData[0] = (intEnable == SH5001_INT_EN) ?
			(regData[0] & 0xBFU) : (regData[0] | 0x40U);
		regmap_write(sh5001a->regmap, SH5001_ORIEN_INTCONF0, regData[0]);
	}

	if(intType & 0xFF3FU) {
		// enable or disable INT
		regmap_read(sh5001a->regmap, SH5001_INT_ENABLE0, &regData[0]);
		regmap_read(sh5001a->regmap, SH5001_INT_ENABLE1, &regData[1]);

		u32IntVal = (regData[0] << 8U) | regData[1];

		u32IntVal = u32IntVal & 0xFF1FU;
		u32IntVal = (intEnable == SH5001_INT_EN) ?
			(u32IntVal | intType) : (u32IntVal & ~intType);

		regData[0] = (u8)(u32IntVal >> 8U);
		regData[1] = (u8)(u32IntVal);
		regmap_write(sh5001a->regmap, SH5001_INT_ENABLE0, regData[0]);
		regmap_write(sh5001a->regmap, SH5001_INT_ENABLE1, regData[1]);

		// mapping interrupt to INT0 pin or INT1 pin
		regmap_read(sh5001a->regmap, SH5001_INT_PIN_MAP0, &regData[0]);
		regmap_read(sh5001a->regmap, SH5001_INT_PIN_MAP1, &regData[1]);
		u32IntVal = (regData[0] << 8U) | regData[1];

		u32IntVal = u32IntVal & 0xFF3FU;
		u32IntVal = (intPinSel == SH5001_INT_MAP_INT1) ?
			(u32IntVal | intType) : (u32IntVal & ~intType);

		regData[0] = (u8)(u32IntVal >> 8U);
		regData[1] = (u8)(u32IntVal);
		regmap_write(sh5001a->regmap, SH5001_INT_PIN_MAP0, regData[0]);
		regmap_write(sh5001a->regmap, SH5001_INT_PIN_MAP1, regData[1]);
	}
}

static void sh5001a_init_data_ready_int(struct sh5001a_data *sh5001a)
{
	///u32 reg_val;
	sh5001a_int_config(sh5001a,
					SH5001_INT0_LEVEL_HIGH,
					SH5001_INT_NO_LATCH, //SH5001_INT_NO_LATCH, SH5001_INT_LATCH
					SH5001_INT_CLEAR_STATUS,  //SH5001_INT_CLEAR_ANY, SH5001_INT_CLEAR_STATUS
					1,
					SH5001_INT1_NORMAL,
					SH5001_INT0_NORMAL,
					SH5001_INT1_OUTPUT,
					SH5001_INT0_OUTPUT);

	//SH5001_INT_GYRO_READY SH5001_INT_ACC_READY
	sh5001a_int_enable(sh5001a, SH5001_INT_GYRO_READY, SH5001_INT_EN, SH5001_INT_MAP_INT0);
}

void sh5001a_fifo_reset(struct sh5001a_data *sh5001a)
{
	u32 regData = 0;

	regmap_read(sh5001a->regmap, SH5001_FIFO_CONF0, &regData);
	regData |= 0x80;
	regmap_write(sh5001a->regmap, SH5001_FIFO_CONF0, regData);
}

void sh5001a_fifo_freq_config(struct sh5001a_data *sh5001a,
								u8 fifoAccDownSampleEnDis,
								u8 fifoAccFreq,
								u8 fifoGyroDownSampleEnDis,
								u8 fifoGyroFreq)
{
	u8 regData = 0;

	regData |= fifoAccDownSampleEnDis | fifoGyroDownSampleEnDis;
	regData |= (fifoAccFreq << 4) | fifoGyroFreq;
	regmap_write(sh5001a->regmap, SH5001_FIFO_CONF4, regData);
}

void sh5001a_fifo_data_config(struct sh5001a_data *sh5001a,
								u32 fifoMode,
								u32 fifoWaterMarkLevel)
{
	u32 regData = 0;

	if(fifoWaterMarkLevel > SH5001_FIFO_BUFFER) {
		fifoWaterMarkLevel = SH5001_FIFO_BUFFER;
	}

	regmap_read(sh5001a->regmap, SH5001_FIFO_CONF2, &regData);
	regData = (regData & 0x88U) | ((u8)(fifoMode >> 8U) & 0x70U) |
		(((u8)(fifoWaterMarkLevel >> 8U)) & 0x07U);
	regmap_write(sh5001a->regmap, SH5001_FIFO_CONF2, regData);

	regData = (u8)fifoMode;
	regmap_write(sh5001a->regmap, SH5001_FIFO_CONF3, regData);

	regData = (u8)fifoWaterMarkLevel;
	regmap_write(sh5001a->regmap, SH5001_FIFO_CONF1, regData);
}

void sh5001a_fifo_mode_set(struct sh5001a_data *sh5001a, u32 fifoMode)
{
	u32 regData = 0;

	regmap_read(sh5001a->regmap, SH5001_FIFO_CONF0, &regData);
	regData &= 0x7F;
	regmap_write(sh5001a->regmap, SH5001_FIFO_CONF0, regData);

	regData = fifoMode & 0x03;
	regmap_write(sh5001a->regmap, SH5001_FIFO_CONF0, regData);
}

void sh5001a_init_fifo(struct sh5001a_data *sh5001a)
{
	sh5001a_int_config(sh5001a, SH5001_INT0_LEVEL_HIGH,
						SH5001_INT_LATCH, //SH5001_INT_NO_LATCH, SH5001_INT_LATCH
						SH5001_INT_CLEAR_STATUS,  //SH5001_INT_CLEAR_ANY, SH5001_INT_CLEAR_STATUS
						1,
						SH5001_INT1_NORMAL,
						SH5001_INT0_NORMAL,
						SH5001_INT1_OUTPUT,
						SH5001_INT0_OUTPUT);

	sh5001a_fifo_reset(sh5001a);

	sh5001a_fifo_freq_config(sh5001a,
							SH5001_FIFO_ACC_DOWNS_DIS,
							SH5001_FIFO_FREQ_X1_32,
							SH5001_FIFO_GYRO_DOWNS_DIS,
							SH5001_FIFO_FREQ_X1_32);

	sh5001a_fifo_data_config(sh5001a,SH5001_FIFO_ACC_X_EN |
							SH5001_FIFO_ACC_Y_EN |
							SH5001_FIFO_ACC_Z_EN |
							SH5001_FIFO_GYRO_X_EN |
							SH5001_FIFO_GYRO_Y_EN |
							SH5001_FIFO_GYRO_Z_EN |
							SH5001_FIFO_TEMPERATURE_EN,
							sh5001a->watermark);

	sh5001a_fifo_mode_set(sh5001a, SH5001_FIFO_MODE_FIFO);

	sh5001a_int_enable(sh5001a, SH5001_INT_FIFO_WATERMARK, SH5001_INT_EN, SH5001_INT_MAP_INT0);
}

static void sh5001a_adjust_Cf(struct sh5001a_data *sh5001a)
{
	u8 reg_addr[15] = {
		0x8C, 0x8D, 0x8E, 0x8F, 0x98, 0x99, 0x9A, 0x9B,
		0xA4, 0xA5, 0xA6, 0xA7, 0xC4, 0xC5, 0xC6};
	u8 regDataold[15] = {0};
	u8 regDatanew[15] = {0};
	u16 OldData[6] = {0};
	u16 NewData[6] = {0};
	int i = 0;

	regmap_read(sh5001a->regmap, reg_addr[12], (u32 *)&regDataold[12]);
	dev_info(sh5001a->dev, "Adjust read %x= %x\r\n", reg_addr[12], regDataold[12]);
	if(regDataold[12] != 0x06)
		return;

	for(i = 0; i < 15; i++) {
		regmap_read(sh5001a->regmap, reg_addr[i], (u32 *)&regDataold[i]);
		dev_info(sh5001a->dev, "Adjust read = %x\r\n", regDataold[i]);
	}

	OldData[0] = (u16)(regDataold[1] << 8 | regDataold[0]);
	OldData[1] = (u16)(regDataold[3] << 8 | regDataold[2]);
	OldData[2] = (u16)(regDataold[5] << 8 | regDataold[4]);
	OldData[3] = (u16)(regDataold[7] << 8 | regDataold[6]);
	OldData[4] = (u16)(regDataold[9] << 8 | regDataold[8]);
	OldData[5] = (u16)(regDataold[11] << 8 | regDataold[10]);

	NewData[0] = OldData[0] * 1693 / 1000;
	NewData[1] = (OldData[1] * 588 - 91000) / 1000;
	NewData[2] = (OldData[2] * 1693) / 1000;
	NewData[3] = (OldData[3] * 585 - 313000) / 1000;
	NewData[4] = OldData[4] * 1679 / 1000;
	NewData[5] = (OldData[5] * 590 - 143000) / 1000;

	dev_info(sh5001a->dev, "OldData %d %d %d %d %d %d \r\n",
		OldData[0], OldData[1], OldData[2], OldData[3], OldData[4], OldData[5]);
	dev_info(sh5001a->dev, "NewData %d %d %d %d %d %d \r\n",
		NewData[0], NewData[1], NewData[2], NewData[3], NewData[4], NewData[5]);

	regDatanew[0] = NewData[0] & 0xFF;
	regDatanew[1] = (NewData[0] >> 8) & 0xFF;
	regDatanew[2] = NewData[1] & 0xFF;
	regDatanew[3] = (NewData[1] >> 8) & 0xFF;
	regDatanew[4] = NewData[2] & 0xFF;
	regDatanew[5] = (NewData[2] >> 8) & 0xFF;
	regDatanew[6] = NewData[3] & 0xFF;
	regDatanew[7] = (NewData[3] >> 8) & 0xFF;
	regDatanew[8] = NewData[4] & 0xFF;
	regDatanew[9] = (NewData[4] >> 8) & 0xFF;
	regDatanew[10] = NewData[5] & 0xFF;
	regDatanew[11] = (NewData[5] >> 8) & 0xFF;
	regDatanew[12] = 0x04;
	regDatanew[13] = 0x04;
	regDatanew[14] = 0x04;

	for(i = 0; i < 15; i++) {
		regmap_write(sh5001a->regmap, reg_addr[i], (u32)regDatanew[i]);
	}
}

static int sh5001a_init(struct sh5001a_data *sh5001a)
{
	//sh5001a_module_reset(sh5001a);
	sh5001a_soft_reset(sh5001a);

	sh5001a_osc_freq(sh5001a);

	// 500Hz, 16G, cut off Freq(BW)=500*0.25Hz=125Hz, enable filter;
	sh5001a_acc_config(sh5001a, 125, 16, 400000,
		SH5001_ACC_FILTER_EN, SH5001_ACC_BYPASS_EN);
	// 800Hz, X\Y\Z 2000deg/s, cut off Freq(BW)=291Hz, enable filter;
	sh5001a_gyro_config(sh5001a, 125, 2000, 400000,
		SH5001_GYRO_FILTER_EN, SH5001_GYRO_BYPASS_EN);
	//sh5001a_imu_data_stablize(sh5001a);

	// temperature ODR is 125Hz, enable temperature measurement
	sh5001a_temp_config(sh5001a, 125, SH5001_TEMP_EN);

	sh5001a_adjust_Cf(sh5001a);

	return 0;
}

static irqreturn_t sh5001a_data_trig_poll(int irq, void *private)
{
	struct iio_dev *indio_dev = private;
	struct sh5001a_data *sh5001a = iio_priv(indio_dev);

	if (sh5001a->dready_trigger_on)
		iio_trigger_poll(sh5001a->dready_trig);
	else if (sh5001a->fifo_trigger_on)
		iio_trigger_poll(sh5001a->fifo_trig);

	return IRQ_HANDLED;
}

static int sh5001a_data_rdy_trigger_set_state(struct iio_trigger *trig, bool state)
{
	struct iio_dev *indio_dev = iio_trigger_get_drvdata(trig);
	struct sh5001a_data *sh5001a = iio_priv(indio_dev);

	if (state) {
		sh5001a_init_data_ready_int(sh5001a);
	} else {
		sh5001a_int_enable(sh5001a, SH5001_INT_GYRO_READY, SH5001_INT_DIS, SH5001_INT_MAP_INT0);
	}

	sh5001a->dready_trigger_on = state;

	return 0;
}

static int sh5001a_fifo_trigger_set_state(struct iio_trigger *trig, bool state)
{
	struct iio_dev *indio_dev = iio_trigger_get_drvdata(trig);
	struct sh5001a_data *sh5001a = iio_priv(indio_dev);

	if (state) {
		sh5001a_init_fifo(sh5001a);
	} else {
		sh5001a_int_enable(sh5001a, SH5001_INT_FIFO_WATERMARK, SH5001_INT_DIS, SH5001_INT_MAP_INT0);
	}

	sh5001a->fifo_trigger_on = state;

	return 0;
}

static const struct iio_trigger_ops sh5001a_dready_trigger_ops = {
	.set_trigger_state = sh5001a_data_rdy_trigger_set_state,
	.validate_device = iio_trigger_validate_own_device,
};

static const struct iio_trigger_ops sh5001a_fifo_trigger_ops = {
	.set_trigger_state = sh5001a_fifo_trigger_set_state,
	.validate_device = iio_trigger_validate_own_device,
};

static int sh5001a_check_chip_id(struct sh5001a_data *sh5001a)
{
	u32 id_expected = SH5001_CHIP_ID_VAL;
	u32 id = 0;

	regmap_read(sh5001a->regmap, SH5001_CHIP_ID, &id);
	if (memcmp(&id, &id_expected, sizeof(u32))) {
		dev_err(sh5001a->dev, "chip id mismatch, id = 0x%x\n", id);
		return -ENODEV;
	}

	return 0;
}

u16 sh5001a_int_read_status01(struct sh5001a_data *sh5001a)
{
	u8 regData[3] = {0};

	regmap_bulk_read(sh5001a->regmap, SH5001_INT_STA0, &regData[0], 3);

	return (((u16)(regData[1] & 0x3FU) << 8U) | regData[0]);
}

u8 sh5001a_fifo_read_status(struct sh5001a_data *sh5001a, u16 *fifoEntriesCount)
{
	u8 regData[2] = {0};

	regmap_bulk_read(sh5001a->regmap, SH5001_FIFO_STA0, regData, 2);

	*fifoEntriesCount = ((u16)(regData[1] & 0x0FU) << 8U) | regData[0];

	return (regData[1] & 0x70U);
}

void sh5001a_fifo_read_data(struct sh5001a_data *sh5001a, u8* fifo_data, u16 data_cnt)
{
	if (data_cnt == 0)
		return;

	if (data_cnt > SH5001_FIFO_BUFFER)
		data_cnt = SH5001_FIFO_BUFFER;

	regmap_raw_read(sh5001a->regmap, SH5001_FIFO_DATA, fifo_data, data_cnt);
}

static irqreturn_t sh5001a_trigger_handler_thread(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct sh5001a_data *sh5001a = iio_priv(indio_dev);
	int bit, tmp, i = 0;

	mutex_lock(&sh5001a->lock);
	if (sh5001a->dready_trigger_on) {
		if (*(indio_dev->active_scan_mask) == SH5001A_ALL_CHANNEL_MASK) {
			regmap_bulk_read(sh5001a->regmap, SH5001_ACC_XL, sh5001a->buffer, SH5001A_ALL_CHANNEL_SIZE);
		} else {
			for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->masklength) {
				sh5001a_get_raw_data(sh5001a, sh5001a_channel_table[bit], &tmp);
				sh5001a->buffer[i++] = (s16)tmp;
			}
		}
		iio_push_to_buffers_with_timestamp(indio_dev, sh5001a->buffer, pf->timestamp);
	} else if (sh5001a->fifo_trigger_on) {
		s16 fifoEntriesCount = 0;
		s16 *p_buffer = NULL;

		sh5001a_fifo_read_status(sh5001a, &fifoEntriesCount);

		sh5001a_fifo_read_data(sh5001a, (u8 *)sh5001a->fifo_buffer, fifoEntriesCount);

		sh5001a_fifo_mode_set(sh5001a, SH5001_FIFO_MODE_DIS);
		p_buffer = sh5001a->fifo_buffer;
		while (fifoEntriesCount > SH5001_WATERMARK_DIV) {
			if (*(indio_dev->active_scan_mask) == SH5001A_ALL_CHANNEL_MASK) {
				memcpy(sh5001a->buffer, p_buffer, SH5001_WATERMARK_DIV);
			} else {
				i = 0;

				for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->masklength) {
					if (bit == 7)
						break;
					sh5001a->buffer[i++] = p_buffer[bit];
				}
			}
			iio_push_to_buffers_with_timestamp(indio_dev, sh5001a->buffer, pf->timestamp);
			p_buffer += (SH5001_WATERMARK_DIV / 2);
			fifoEntriesCount -= SH5001_WATERMARK_DIV;
		}
		sh5001a_fifo_mode_set(sh5001a, SH5001_FIFO_MODE_FIFO);
		/* clear INT */
		sh5001a_int_read_status01(sh5001a);
	}
	mutex_unlock(&sh5001a->lock);

	iio_trigger_notify_done(indio_dev->trig);

	return IRQ_HANDLED;
}

int sh5001a_probe(struct device *dev, struct regmap *regmap)
{
	int ret;
	struct iio_dev *indio_dev;
	struct sh5001a_data *sh5001a;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*sh5001a));
	if (!indio_dev) {
		dev_err(dev, "iio allocation failed!\n");
		return -ENOMEM;
	}
	sh5001a = iio_priv(indio_dev);
	sh5001a->dev = dev;
	sh5001a->regmap = regmap;
	mutex_init(&sh5001a->lock);
	dev_set_drvdata(dev, sh5001a);

	indio_dev->dev.parent = dev;
	indio_dev->info = &sh5001a_info;
	indio_dev->name = "sh5001a";
	indio_dev->modes = INDIO_DIRECT_MODE | INDIO_BUFFER_TRIGGERED;
	indio_dev->channels = sh5001a_channels;
	indio_dev->num_channels = ARRAY_SIZE(sh5001a_channels);

	ret = sh5001a_check_chip_id(sh5001a);
	if (ret < 0) {
		return ret;
	}

	sh5001a_init(sh5001a);

	sh5001a->irq1_gpiod = devm_gpiod_get_optional(dev, "sh5001a,irq1", GPIOD_IN);
	if (!sh5001a->irq1_gpiod) {
		dev_err(dev, "failed to get irq1 gpio\n");
	}
	if (sh5001a->irq1_gpiod) {
		sh5001a->irq1 = gpiod_to_irq(sh5001a->irq1_gpiod);
		if (sh5001a->irq1 < 0) {
			dev_err(dev, "failed to irq1 for sh5001a\n");
		} else {
			ret = devm_request_threaded_irq(dev, sh5001a->irq1,
				sh5001a_data_trig_poll, NULL,
				IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				"sh5001a", indio_dev);
			if (ret) {
				dev_err(dev, "request irq1 %d failed\n", sh5001a->irq1);
				return ret;
			}
			/* dready mode */
			sh5001a->dready_trig = devm_iio_trigger_alloc(dev,
				"%s-dev%d-dready", indio_dev->name, indio_dev->id);
			if (!sh5001a->dready_trig) {
				ret = -ENOMEM;
				return ret;
			}

			sh5001a->dready_trig->dev.parent = dev;
			sh5001a->dready_trig->ops = &sh5001a_dready_trigger_ops;
			iio_trigger_set_drvdata(sh5001a->dready_trig, indio_dev);
			ret = devm_iio_trigger_register(dev, sh5001a->dready_trig);
			if (ret) {
				dev_err(dev, "iio trigger register failed\n");
				return ret;
			}
			/* fifo mode */
			sh5001a->fifo_trig = devm_iio_trigger_alloc(dev,
				"%s-dev%d-fifo", indio_dev->name, indio_dev->id);
			if (!sh5001a->fifo_trig) {
				ret = -ENOMEM;
				return ret;
			}

			sh5001a->fifo_trig->dev.parent = dev;
			sh5001a->fifo_trig->ops = &sh5001a_fifo_trigger_ops;
			/* default watermark */
			sh5001a->watermark = SH5001A_FIFO_LENGTH;
			iio_trigger_set_drvdata(sh5001a->fifo_trig, indio_dev);
			ret = devm_iio_trigger_register(dev, sh5001a->fifo_trig);
			if (ret) {
				dev_err(dev, "iio trigger register failed\n");
				return ret;
			}

			ret = devm_iio_triggered_buffer_setup(dev, indio_dev,
					iio_pollfunc_store_time,
					sh5001a_trigger_handler_thread,
					NULL);
			if (ret < 0) {
				dev_err(dev, "iio triggered buffer setup failed\n");
				return -EINVAL;
			}
			iio_buffer_set_attrs(indio_dev->buffer,
					sh5001a_fifo_attributes);
		}
	}

	ret = devm_iio_device_register(dev, indio_dev);
	if (ret < 0) {
		dev_err(dev, "device_register failed\n");
		return ret;
	}

	dev_info(dev, "sh5001a probe success\n");

	return 0;
}

int sh5001a_remove(struct device *dev)
{
	struct sh5001a_data *sh5001a = dev_get_drvdata(dev);

	if (sh5001a->dready_trigger_on)
		sh5001a_int_enable(sh5001a, SH5001_INT_GYRO_READY, SH5001_INT_DIS, SH5001_INT_MAP_INT0);

	if (sh5001a->fifo_trigger_on)
		sh5001a_int_enable(sh5001a, SH5001_INT_FIFO_WATERMARK, SH5001_INT_DIS, SH5001_INT_MAP_INT0);

	return 0;
}

void sh5001a_shutdown(struct device *dev)
{
	sh5001a_remove(dev);
}

MODULE_AUTHOR("<zhiwen.liang@hollyland-tech.com>");
MODULE_DESCRIPTION("Driver for sh5001a");
MODULE_LICENSE("GPL and additional rights");
