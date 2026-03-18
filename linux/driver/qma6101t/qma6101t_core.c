#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
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

#include "qma6101t.h"

#define QMA6101T_ALL_CHANNEL_MASK GENMASK(2, 0)
#define QMA6101T_ALL_CHANNEL_SIZE 6

struct qma6101t_data {
	struct device *dev;
	struct iio_trigger *dready_trig;
	struct iio_trigger *fifo_trig;
	struct regmap *regmap;
	struct gpio_desc *irq1_gpiod;
	int irq1;
	struct mutex lock;
	bool dready_trigger_on;
	bool fifo_trigger_on;

	u8 acc_range_idx;
	u8 acc_sample_idx;
	u8 acc_lpf_idx;

	u32 frame_len;
	u32 watermark;
	s16 buffer[8];
	u8 fifo_buffer[QMA6101T_FIFO_LEN];
};

static const int qma6101t_channel_table[] = {
	QMA6101T_X_OUT_L,
	QMA6101T_Y_OUT_L,
	QMA6101T_Z_OUT_L,
};

static const int qma6101t_fifo_en_table[] = {
	QMA6101T_FIFO_EN_X,
	QMA6101T_FIFO_EN_Y,
	QMA6101T_FIFO_EN_Z,
};

#define QMA6101T_ACCEL_CHANNEL(index, reg, axis) { \
	.type = IIO_ACCEL, \
	.address = reg, \
	.modified = 1, \
	.channel2 = IIO_MOD_##axis, \
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | \
		BIT(IIO_CHAN_INFO_PROCESSED), \
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

static const struct iio_chan_spec qma6101t_channels[] = {
	QMA6101T_ACCEL_CHANNEL(0, QMA6101T_X_OUT_L, X),
	QMA6101T_ACCEL_CHANNEL(1, QMA6101T_Y_OUT_L, Y),
	QMA6101T_ACCEL_CHANNEL(2, QMA6101T_Z_OUT_L, Z),
	IIO_CHAN_SOFT_TIMESTAMP(3)
};

static int qma6101t_get_raw_data(struct qma6101t_data *qma6101t, u32 addr, int *val)
{
	regmap_bulk_read(qma6101t->regmap, addr, val, 2);

	return IIO_VAL_INT;
}

static IIO_CONST_ATTR(in_accel_scale_available, "2 4 8");

static const struct {
	u8 reg_val;
	u16 range;
	u32 uints;
} qma6101t_acc_range_table[] = {
	{QMA6101T_RANGE_2G, 2, 61},
	{QMA6101T_RANGE_4G, 4, 122},
	{QMA6101T_RANGE_8G, 8, 244},
};

static int qma6101t_set_acc_scale(struct qma6101t_data *qma6101t, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(qma6101t_acc_range_table); i++) {
		if (qma6101t_acc_range_table[i].range == val) {
			qma6101t->acc_range_idx = i;

			return regmap_update_bits(qma6101t->regmap, QMA6101T_RANGE,
				QMA6101T_RANGE_MASK, qma6101t_acc_range_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int qma6101t_get_acc_scale(struct qma6101t_data *qma6101t, int *val)
{
	u32 reg_val;
	int i;

	regmap_read(qma6101t->regmap, QMA6101T_RANGE, &reg_val);
	reg_val &= QMA6101T_RANGE_MASK;
	for (i = 0; i < ARRAY_SIZE(qma6101t_acc_range_table); i++) {
		if (qma6101t_acc_range_table[i].reg_val == reg_val) {
			*val = qma6101t_acc_range_table[i].range;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static int qma6101t_get_acc_processed(struct qma6101t_data *qma6101t,
	struct iio_chan_spec const *chan, int *val, int *val2)
{
	s64 reg_val;

	qma6101t_get_raw_data(qma6101t, chan->address, (s32 *)&reg_val);
	reg_val = sign_extend64(reg_val, 15);
	reg_val = reg_val * qma6101t_acc_range_table[qma6101t->acc_range_idx].uints;
	*val = reg_val / 1000000;
	*val2 = reg_val % 1000000;

	return IIO_VAL_INT_PLUS_MICRO;
}

static const struct {
	u8 reg_val;
	u16 samp_freq;
} qma6101t_acc_samp_freq_table[] = {
	{QMA6101T_ODR_1600HZ, 1600},
	{QMA6101T_ODR_800HZ, 800},
	{QMA6101T_ODR_400HZ, 400},
	{QMA6101T_ODR_200HZ, 200},
	{QMA6101T_ODR_100HZ, 100},
	{QMA6101T_ODR_50HZ, 50},
	{QMA6101T_ODR_25HZ, 25},
	{QMA6101T_ODR_12P5HZ, 12},
	{QMA6101T_ODR_6P2HZ, 6},
	{QMA6101T_ODR_3P1HZ, 3},
	{QMA6101T_ODR_1P5HZ, 1},
	{QMA6101T_ODR_0P7HZ, 0},
};

static IIO_CONST_ATTR(in_accel_sampling_frequency_available,
	"1600 800 400 200 100 50 25 12 6 3 1 0");

static void qma6101t_set_mode(struct qma6101t_data *qma6101t, bool en)
{
	if (en) {
		regmap_update_bits(qma6101t->regmap, QMA6101T_PM, QMA6101T_MODE, 0xff);
	} else {
		regmap_update_bits(qma6101t->regmap, QMA6101T_PM, QMA6101T_MODE, 0);
	}
}

static int qma6101t_set_acc_sample_rate(struct qma6101t_data *qma6101t, int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(qma6101t_acc_samp_freq_table); i++) {
		if (qma6101t_acc_samp_freq_table[i].samp_freq == val) {
			qma6101t->acc_sample_idx = i;

			return regmap_update_bits(qma6101t->regmap, QMA6101T_PM,
				QMA6101T_ODR_MASK, qma6101t_acc_samp_freq_table[i].reg_val);
		}
	}

	return -EINVAL;
}

static int qma6101t_get_acc_sample_rate(struct qma6101t_data *qma6101t, int *val)
{
	u32 reg_val;
	int i;

	regmap_read(qma6101t->regmap, QMA6101T_PM, &reg_val);
	reg_val &= QMA6101T_ODR_MASK;
	for (i = 0; i < ARRAY_SIZE(qma6101t_acc_samp_freq_table); i++) {
		if (qma6101t_acc_samp_freq_table[i].reg_val == reg_val) {
			*val = qma6101t_acc_samp_freq_table[i].samp_freq;

			return IIO_VAL_INT;
		}
	}

	return -EINVAL;
}

static int qma6101t_read_raw(struct iio_dev *indio_dev,
	struct iio_chan_spec const *chan, int *val, int *val2, long mask)
{
	struct qma6101t_data *qma6101t = iio_priv(indio_dev);
	int ret = 0;

	mutex_lock(&qma6101t->lock);
	switch(mask) {
	case IIO_CHAN_INFO_RAW:
		ret = qma6101t_get_raw_data(qma6101t, chan->address, val);
		switch(chan->type) {
		case IIO_ACCEL:
			*val = sign_extend32(*val, 15);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	case IIO_CHAN_INFO_PROCESSED:
		switch(chan->type) {
		case IIO_ACCEL:
			ret = qma6101t_get_acc_processed(qma6101t, chan, val, val2);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	case IIO_CHAN_INFO_SCALE:
		switch(chan->type) {
		case IIO_ACCEL:
			ret = qma6101t_get_acc_scale(qma6101t, val);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	case IIO_CHAN_INFO_SAMP_FREQ:
		switch(chan->type) {
		case IIO_ACCEL:
			ret = qma6101t_get_acc_sample_rate(qma6101t, val);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&qma6101t->lock);

	return ret;
}

static int qma6101t_write_raw(struct iio_dev *indio_dev,
	struct iio_chan_spec const *chan, int val, int val2, long mask)
{
	struct qma6101t_data *qma6101t = iio_priv(indio_dev);
	int ret = -EINVAL;

	mutex_lock(&qma6101t->lock);
	switch(mask) {
	case IIO_CHAN_INFO_SCALE:
		switch(chan->type) {
		case IIO_ACCEL:
			ret = qma6101t_set_acc_scale(qma6101t, val);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	case IIO_CHAN_INFO_SAMP_FREQ:
		switch(chan->type) {
		case IIO_ACCEL:
			ret = qma6101t_set_acc_sample_rate(qma6101t, val);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&qma6101t->lock);

	return ret;
}

static ssize_t reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct qma6101t_data *qma6101t = iio_priv(indio_dev);
	u32 reg_val = 0;
	u32 i = 0;
	ssize_t len = 0;

	for (i = 0; i <= QMA6101T_TST1_ANA; i++) {
		regmap_read(qma6101t->regmap, i, &reg_val);
		len += snprintf(buf + len, PAGE_SIZE - len, "reg:0x%02x=0x%02x\n", i, reg_val);
	}

	return len;
}

static ssize_t reg_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t len)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct qma6101t_data *qma6101t = iio_priv(indio_dev);
	u32 databuf[2];

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2)
		regmap_write(qma6101t->regmap, databuf[0], databuf[1]);

	return len;
}

static IIO_DEVICE_ATTR_RW(reg, 0);

static struct attribute *qma6101t_attributes[] = {
	&iio_const_attr_in_accel_sampling_frequency_available.dev_attr.attr,
	&iio_const_attr_in_accel_scale_available.dev_attr.attr,
	&iio_dev_attr_reg.dev_attr.attr,
	NULL,
};

static const struct attribute_group qma6101t_attribute_group = {
	.attrs = qma6101t_attributes
};

static int qma6101t_set_watermark(struct iio_dev *indio_dev, unsigned val)
{
	struct qma6101t_data *qma6101t = iio_priv(indio_dev);

	if (val > QMA6101T_FIFO_LEN)
		val = QMA6101T_FIFO_LEN;

	qma6101t->watermark = val;

	return 0;
}

static const struct iio_info qma6101t_info = {
	.read_raw		= qma6101t_read_raw,
	.write_raw		= qma6101t_write_raw,
	.attrs			= &qma6101t_attribute_group,
	.hwfifo_set_watermark	= qma6101t_set_watermark,
};

static u8 qma6101t_read_fifo_status(struct qma6101t_data *qma6101t)
{
	u8 frame_cnt = 0;

	regmap_read(qma6101t->regmap, QMA6101T_FIFO_FRAME_COUNTER, (int *)&frame_cnt);
	frame_cnt &= QMA6101T_FIFO_FRAME_COUNTER_MASK;

	return frame_cnt;
}

static void qma6101t_fifo_read_data(struct qma6101t_data *qma6101t, void *data, u32 cnt)
{
	regmap_raw_read(qma6101t->regmap, QMA6101T_FIFO_DATA, data, cnt);
}

static u8 qma6101t_read_data_int_status(struct qma6101t_data *qma6101t)
{
	u8 int_status = 0;

	regmap_read(qma6101t->regmap, QMA6101T_DATA_INT_STATUS, (int *)&int_status);

	return int_status;
}

static irqreturn_t qma6101t_trigger_handler_thread(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct qma6101t_data *qma6101t = iio_priv(indio_dev);

	mutex_lock(&qma6101t->lock);
	if (qma6101t->dready_trigger_on) {
		int bit, tmp, i = 0;

		if (*(indio_dev->active_scan_mask) == QMA6101T_ALL_CHANNEL_MASK) {
			regmap_bulk_read(qma6101t->regmap, QMA6101T_X_OUT_L, qma6101t->buffer,
				QMA6101T_ALL_CHANNEL_SIZE);
		} else {
			for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->masklength) {
				qma6101t_get_raw_data(qma6101t, qma6101t_channel_table[bit], &tmp);
				qma6101t->buffer[i++] = (s16)tmp;
			}
		}
		iio_push_to_buffers_with_timestamp(indio_dev, qma6101t->buffer, pf->timestamp);
		qma6101t_read_data_int_status(qma6101t);
	} else if (qma6101t->fifo_trigger_on) {
		u16 frame_cnt = 0;
		u32 total_frame_len = 0;
		u8 *p_buffer = NULL;

		frame_cnt = qma6101t_read_fifo_status(qma6101t);
		total_frame_len = frame_cnt * qma6101t->frame_len;
		qma6101t_fifo_read_data(qma6101t, qma6101t->fifo_buffer, total_frame_len);
		p_buffer = qma6101t->fifo_buffer;
		while (total_frame_len) {
			memcpy(qma6101t->buffer, p_buffer, qma6101t->frame_len);
			iio_push_to_buffers_with_timestamp(indio_dev, qma6101t->buffer, pf->timestamp);
			p_buffer += qma6101t->frame_len;
			total_frame_len -= qma6101t->frame_len;
		}
		qma6101t_read_data_int_status(qma6101t);
	}
	mutex_unlock(&qma6101t->lock);

	iio_trigger_notify_done(indio_dev->trig);

	return IRQ_HANDLED;
}

static irqreturn_t qma6101t_data_trig_poll(int irq, void *private)
{
	struct iio_dev *indio_dev = private;
	struct qma6101t_data *qma6101t = iio_priv(indio_dev);

	if (qma6101t->dready_trigger_on)
		iio_trigger_poll(qma6101t->dready_trig);
	else if (qma6101t->fifo_trigger_on)
		iio_trigger_poll(qma6101t->fifo_trig);

	return IRQ_HANDLED;
}

static int qma6101t_reset(struct qma6101t_data *qma6101t)
{
	u32 val = 0;
	u32 i = 5;

	qma6101t_set_mode(qma6101t, 1);

	regmap_write(qma6101t->regmap, QMA6101T_SW_RESET, QMA6101T_SW_RESET_VAL);
	while (i) {
		regmap_read(qma6101t->regmap, QMA6101T_NVM, &val);
		val = val & 0x5;
		if (val == (QMA6101T_NVM_LOAD_DONE | QMA6101T_NVM_RDY))
			break;
		usleep_range(1000, 1100);
		i--;
	}
	if (val == 0)
		return -EINVAL;
	regmap_write(qma6101t->regmap, QMA6101T_OSR_LPF_ASLP,0x60);
	regmap_write(qma6101t->regmap, QMA6101T_RANGE, 0x01);
	regmap_write(qma6101t->regmap, 0x57, 0x07);
	regmap_write(qma6101t->regmap, QMA6101T_RSV1, 0x04);
	regmap_write(qma6101t->regmap, QMA6101T_PM, 0xC4);

	return 0;
}

static int qma6101t_dready_trigger_set_state(struct iio_trigger *trig, bool state)
{
	struct iio_dev *indio_dev = iio_trigger_get_drvdata(trig);
	struct qma6101t_data *qma6101t = iio_priv(indio_dev);

	if (state) {
		regmap_write(qma6101t->regmap, QMA6101T_INT1_MAP_CTRL1, QMA6101T_INT1_DRDY);
		regmap_update_bits(qma6101t->regmap, QMA6101T_DATA_INT_CTRL, QMA6101T_DRDY, 0xff);
	} else {
		regmap_update_bits(qma6101t->regmap, QMA6101T_DATA_INT_CTRL, QMA6101T_DRDY, 0x0);
		regmap_write(qma6101t->regmap, QMA6101T_INT1_MAP_CTRL1, 0);
	}

	qma6101t->dready_trigger_on = state;

	return 0;
}

static const struct iio_trigger_ops qma6101t_dready_trigger_ops = {
	.set_trigger_state = qma6101t_dready_trigger_set_state,
	.validate_device = iio_trigger_validate_own_device,
};

static int qma6101t_fifo_trigger_set_state(struct iio_trigger *trig, bool state)
{
	struct iio_dev *indio_dev = iio_trigger_get_drvdata(trig);
	struct qma6101t_data *qma6101t = iio_priv(indio_dev);

	if (state) {
		u32 bit = 0;
		u8 hw_watermark = 0;

		regmap_write(qma6101t->regmap, QMA6101T_INT1_MAP_CTRL1, QMA6101T_INT1_FIFO_WATERMARK);
		regmap_update_bits(qma6101t->regmap, QMA6101T_FIFO_CFG, QMA6101T_FIFO_MODE_MASK, QMA6101T_FIFO);
		if (*(indio_dev->active_scan_mask) == QMA6101T_ALL_CHANNEL_MASK) {
			qma6101t->frame_len = 6;
			regmap_update_bits(qma6101t->regmap, QMA6101T_FIFO_CFG, QMA6101T_FIFO_EN_ALL, 0xff);
		} else {
			regmap_update_bits(qma6101t->regmap, QMA6101T_FIFO_CFG, QMA6101T_FIFO_EN_ALL, 0);
			for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->masklength) {
				regmap_update_bits(qma6101t->regmap, QMA6101T_FIFO_CFG, qma6101t_fifo_en_table[bit], 0xff);
				qma6101t->frame_len += 2;
			}
		}

		hw_watermark = qma6101t->watermark / qma6101t->frame_len;
		regmap_write(qma6101t->regmap, QMA6101T_FIFO_WM_LVL, hw_watermark);
		regmap_update_bits(qma6101t->regmap, QMA6101T_DATA_INT_CTRL, QMA6101T_FIFO_WM, 0xff);
	} else {
		regmap_update_bits(qma6101t->regmap, QMA6101T_DATA_INT_CTRL, QMA6101T_FIFO_WM, 0);
		regmap_write(qma6101t->regmap, QMA6101T_INT1_MAP_CTRL1, 0);
	}

	qma6101t->fifo_trigger_on = state;

	return 0;
}

static const struct iio_trigger_ops qma6101t_fifo_trigger_ops = {
	.set_trigger_state = qma6101t_fifo_trigger_set_state,
	.validate_device = iio_trigger_validate_own_device,
};

static int qma6101t_check_chip_id(struct qma6101t_data *qma6101t)
{
	u32 id_expected = QMA6101T_CHIP_ID_VAL;
	u8 id = 0;

	regmap_read(qma6101t->regmap, QMA6101T_CHIP_ID, (u32 *)&id);
	id &= 0xF0;
	if (memcmp(&id, &id_expected, sizeof(u8))) {
		dev_err(qma6101t->dev, "chip id mismatch, id = 0x%x\n", id);
		return -ENODEV;
	}

	return 0;
}

int qma6101t_suspend(struct device *dev)
{
	struct qma6101t_data *qma6101t = dev_get_drvdata(dev);

	qma6101t_set_mode(qma6101t, true);

	return 0;
}

int qma6101t_resume(struct device *dev)
{
	struct qma6101t_data *qma6101t = dev_get_drvdata(dev);

	qma6101t_set_mode(qma6101t, false);

	return 0;
}

int qma6101t_probe(struct device *dev, struct regmap *regmap)
{
	struct iio_dev *indio_dev;
	struct qma6101t_data *qma6101t;
	int ret;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*qma6101t));
	if (!indio_dev) {
		dev_err(dev, "iio allocation failed!\n");
		return -ENOMEM;
	}
	qma6101t = iio_priv(indio_dev);
	qma6101t->dev = dev;
	qma6101t->regmap = regmap;
	mutex_init(&qma6101t->lock);
	dev_set_drvdata(dev, qma6101t);

	indio_dev->dev.parent = dev;
	indio_dev->info = &qma6101t_info;
	indio_dev->name = "qma6101t";
	indio_dev->modes = INDIO_DIRECT_MODE | INDIO_BUFFER_TRIGGERED;
	indio_dev->channels = qma6101t_channels;
	indio_dev->num_channels = ARRAY_SIZE(qma6101t_channels);

	ret = qma6101t_check_chip_id(qma6101t);
	if (ret < 0) {
		return ret;
	}

	qma6101t_reset(qma6101t);

	qma6101t->irq1_gpiod = devm_gpiod_get_optional(dev, "qma6101t,irq1", GPIOD_IN);
	if (!qma6101t->irq1_gpiod) {
		dev_err(dev, "failed to get irq1 gpio\n");
	}
	if (qma6101t->irq1_gpiod) {
		qma6101t->irq1 = gpiod_to_irq(qma6101t->irq1_gpiod);
		if (qma6101t->irq1 < 0) {
			dev_err(dev, "failed to irq1 for qma6101t\n");
		} else {
			ret = devm_request_threaded_irq(dev, qma6101t->irq1,
				qma6101t_data_trig_poll, NULL,
				IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				"qma6101t", indio_dev);
			if (ret) {
				dev_err(dev, "request irq1 %d failed\n", qma6101t->irq1);
				return ret;
			}
			/* dready mode */
			qma6101t->dready_trig = devm_iio_trigger_alloc(dev,
				"%s-dev%d-dready", indio_dev->name, iio_device_id(indio_dev));
			if (!qma6101t->dready_trig) {
				ret = -ENOMEM;
				return ret;
			}

			qma6101t->dready_trig->dev.parent = dev;
			qma6101t->dready_trig->ops = &qma6101t_dready_trigger_ops;
			iio_trigger_set_drvdata(qma6101t->dready_trig, indio_dev);
			ret = devm_iio_trigger_register(dev, qma6101t->dready_trig);
			if (ret) {
				dev_err(dev, "iio trigger register failed\n");
				return ret;
			}
			/* fifo mode */
			qma6101t->fifo_trig = devm_iio_trigger_alloc(dev,
				"%s-dev%d-fifo", indio_dev->name, iio_device_id(indio_dev));
			if (!qma6101t->fifo_trig) {
				ret = -ENOMEM;
				return ret;
			}

			qma6101t->fifo_trig->dev.parent = dev;
			qma6101t->fifo_trig->ops = &qma6101t_fifo_trigger_ops;
			iio_trigger_set_drvdata(qma6101t->fifo_trig, indio_dev);
			ret = devm_iio_trigger_register(dev, qma6101t->fifo_trig);
			if (ret) {
				dev_err(dev, "iio trigger register failed\n");
				return ret;
			}

			ret = devm_iio_triggered_buffer_setup(dev, indio_dev,
				iio_pollfunc_store_time,
				qma6101t_trigger_handler_thread,
				NULL);

			if (ret < 0) {
				dev_err(dev, "iio triggered buffer setup failed\n");
				return -EINVAL;
			}
		}
	}

	ret = devm_iio_device_register(dev, indio_dev);
	if (ret < 0) {
		dev_err(dev, "device_register failed\n");
		return ret;
	}

	dev_info(dev, "qma6101t probe success\n");

	return 0;
}

void qma6101t_remove(struct device *dev)
{
	struct qma6101t_data *qma6101t = dev_get_drvdata(dev);

	if (qma6101t->dready_trigger_on)
		regmap_update_bits(qma6101t->regmap, QMA6101T_DATA_INT_CTRL, QMA6101T_DRDY, 0x0);

	if (qma6101t->fifo_trigger_on)
		regmap_update_bits(qma6101t->regmap, QMA6101T_DATA_INT_CTRL, QMA6101T_FIFO_WM, 0x0);

	qma6101t_set_mode(qma6101t, false);
}

void qma6101t_shutdown(struct device *dev)
{
	qma6101t_remove(dev);
}

MODULE_AUTHOR("<897420073@qq.com>");
MODULE_DESCRIPTION("Driver for qma6101t");
MODULE_LICENSE("GPL");
