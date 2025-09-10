#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/notifier.h>
#include <linux/gsv2k11_notifier.h>

/* gmm626 video formate regs */
#define REG_VIDEO_FORMAT_352_OUT_WORD_1    0x000A
#define REG_VIDEO_FORMAT_352_OUT_WORD_2    0x000B
#define REG_VIDEO_FORMAT_352_OUT_WORD_3    0x000C
#define REG_VIDEO_FORMAT_352_OUT_WORD_4    0x000D
#define REG_IOPROC                         0x0000

typedef enum {
	payloadID_default = 0,
	payloadID_1080p60,
	payloadID_1080p59_94,
	payloadID_1080p50,
	payloadID_1080p30,
	payloadID_1080p29_98,
	payloadID_1080p25,
	payloadID_1080p24,
	payloadID_1080p23_98,
	payloadID_1080i60,
	payloadID_1080i59_94,
	payloadID_1080i50,
	payloadID_720P60,
	payloadID_720P59_94,
	payloadID_720p50,
	payloadID_max
} video_mode_t;

struct gmm626_data {
	struct spi_device *spi;
	struct gpio_desc *reset_gpiod;
	struct gpio_desc *rate0_gpiod;
	struct gpio_desc *rate1_gpiod;
	struct gpio_desc *standby_gpiod;

	bool rate0;
	bool rate1;

	u16 mode;
	u16 max_mode;

	struct notifier_block mode_nb;
};

struct payload_id {
	const char name[32];
	u16 payload[4];
	bool rate0;
	bool rate1;
};

/* payloadID */
static struct payload_id g_payload_id[] = {
	{"default",				{0x0000, 0x0000, 0x0000, 0x0000}, 0, 0},
	{"1080p60level A",		{0xCB89, 0x0180, 0x0000, 0x0000}, 0, 1},
	{"1080p59.94level A",	{0xCA89, 0x0180, 0x0000, 0x0000}, 0, 1},
	{"1080p50level A",		{0xC989, 0x0180, 0x0000, 0x0000}, 0, 1},
	{"1080p30",				{0xC785, 0x0180, 0x0000, 0x0000}, 0, 0},
	{"1080p29.97",			{0xC685, 0x0180, 0x0000, 0x0000}, 0, 0},
	{"1080p25",				{0xC585, 0x0180, 0x0000, 0x0000}, 0, 0},
	{"1080p24",				{0xC385, 0x0180, 0x0000, 0x0000}, 0, 0},
	{"1080p23.98",			{0xC285, 0x0180, 0x0000, 0x0000}, 0, 0},
	{"1080i60",				{0x0785, 0x0180, 0x0000, 0x0000}, 0, 0},
	{"1080i59.94",			{0x0685, 0x0180, 0x0000, 0x0000}, 0, 0},
	{"1080i50",				{0x0585, 0x0180, 0x0000, 0x0000}, 0, 0},
	{"720p60",				{0x4B84, 0x0180, 0x0000, 0x0000}, 0, 0},
	{"720p59.94",			{0x4A84, 0x0180, 0x0000, 0x0000}, 0, 0},
	{"720p50",				{0x4984, 0x0180, 0x0000, 0x0000}, 0, 0},
};

static int gmm626_gspi_read(struct spi_device *spi, const u16 reg_addr, u16 *word)
{
	u16 wr_buf = 0;
	struct spi_transfer t[2] = {
		{
			.len = 2,
		},
		{
			.rx_buf = (void *)word,
			.len = 2,
		},
	};

	wr_buf = 0x8000 | reg_addr;
	t[0].tx_buf = &wr_buf;

	return spi_sync_transfer(spi, t, 2);
}

static int gmm626_gspi_write(struct spi_device *spi, u16 reg_addr, u16 word)
{
	u16 wr_buf[2] = {0};
	struct spi_transfer t[2] = {
		{
			.len = 2,
		},
		{
			.len = 2,
		},
	};

	wr_buf[0] = 0x7FFF & reg_addr;
	wr_buf[1] = word;
	t[0].tx_buf = &wr_buf[0];
	t[1].tx_buf = &wr_buf[1];

	return spi_sync_transfer(spi, t, 2);
}

static int gmm626_read_data(struct spi_device *spi, u16 reg_addr, u16 *data)
{
	int ret = 0;

	ret = gmm626_gspi_read(spi, reg_addr, data);
	if (ret) {
		dev_err(&spi->dev, "gspi read error, ret = %d\n", ret);
		return ret;
	}

	return 0;
}

static int gmm626_write_data(struct spi_device *spi, u16 reg_addr, u16 data)
{
	int ret = 0;

	ret = gmm626_gspi_write(spi, reg_addr, data);
	if (ret) {
		dev_err(&spi->dev, "gspi read error, ret = %d\n", ret);
		return ret;
	}

	return 0;
}

static void gmm626_insert_payload_id(struct spi_device *spi, u16 *payload)
{
	gmm626_write_data(spi, REG_VIDEO_FORMAT_352_OUT_WORD_1, payload[0]);
	gmm626_write_data(spi, REG_VIDEO_FORMAT_352_OUT_WORD_2, payload[1]);
	gmm626_write_data(spi, REG_VIDEO_FORMAT_352_OUT_WORD_3, payload[2]);
	gmm626_write_data(spi, REG_VIDEO_FORMAT_352_OUT_WORD_4, payload[3]);
}

static void gmm626_set_rate0(struct spi_device *spi, int rate0)
{
	struct gmm626_data *gmm626 = spi_get_drvdata(spi);

	gmm626->rate0 = !!rate0;
	gpiod_set_value_cansleep(gmm626->rate0_gpiod, gmm626->rate0);
}

static void gmm626_set_rate1(struct spi_device *spi, int rate1)
{
	struct gmm626_data *gmm626 = spi_get_drvdata(spi);

	gmm626->rate1 = !!rate1;
	gpiod_set_value_cansleep(gmm626->rate1_gpiod, gmm626->rate1);
}

static void gmm626_set_rate_status(struct spi_device *spi, int rate0, int rate1)
{
	gmm626_set_rate0(spi, rate0);
	gmm626_set_rate1(spi, rate1);
}

static void gmm626_set_standby(struct spi_device *spi, bool standby)
{
	struct gmm626_data *gmm626 = spi_get_drvdata(spi);

	gpiod_set_value_cansleep(gmm626->standby_gpiod, standby);
}

static void gmm626_set_mode(struct spi_device *spi, u16 mode)
{
	struct gmm626_data *gmm626 = spi_get_drvdata(spi);

	gmm626_set_standby(gmm626->spi, 1);
	mdelay(1);
	if (mode < gmm626->max_mode) {
		gmm626->mode = mode;
		gmm626_insert_payload_id(spi, g_payload_id[mode].payload);
		gmm626_set_rate_status(spi, g_payload_id[mode].rate0, g_payload_id[mode].rate1);
		dev_info(&spi->dev, "Switch to mode: %s\n", g_payload_id[mode].name);
	}
	gmm626_set_standby(gmm626->spi, 0);
}

static int gmm626_mode_notifier(struct notifier_block *nb,
			     unsigned long event, void *data)
{
	struct gmm626_data *gmm626 = container_of(nb, struct gmm626_data, mode_nb);
	u16 mode = payloadID_default;

	if (event) {
		switch (event) {
		case VFMT_CEA_03_720x480P_60HZ:
			mode = payloadID_default;
			break;
		case VFMT_CEA_04_1280x720P_60HZ:
			mode = payloadID_720P60;
			break;
		case VFMT_CEA_05_1920x1080I_60HZ:
			mode = payloadID_1080i60;
			break;
		case VFMT_CEA_07_720x480I_60HZ:
			break;
		case VFMT_CEA_16_1920x1080P_60HZ:
		case VFMT_CEA_97_3840x2160P_60HZ:
		case VFMT_CEA_102_4096x2160P_60HZ:
			mode = payloadID_1080p60;
			break;
		case VFMT_CEA_18_720x576P_50HZ:
			break;
		case VFMT_CEA_19_1280x720P_50HZ:
			mode = payloadID_720p50;
			break;
		case VFMT_CEA_20_1920x1080I_50HZ:
			mode = payloadID_1080i50;
			break;
		case VFMT_CEA_22_720x576I_50HZ:
			break;
		case VFMT_CEA_31_1920x1080P_50HZ:
		case VFMT_CEA_96_3840x2160P_50HZ:
		case VFMT_CEA_101_4096x2160P_50HZ:
			mode = payloadID_1080p50;
			break;
		case VFMT_CEA_32_1920x1080P_24HZ:
		case VFMT_CEA_93_3840x2160P_24HZ:
		case VFMT_CEA_98_4096x2160P_24HZ:
			mode = payloadID_1080p24;
			break;
		case VFMT_CEA_33_1920x1080P_25HZ:
		case VFMT_CEA_94_3840x2160P_25HZ:
		case VFMT_CEA_99_4096x2160P_25HZ:
			mode = payloadID_1080p25;
			break;
		case VFMT_CEA_34_1920x1080P_30HZ:
		case VFMT_CEA_95_3840x2160P_30HZ:
		case VFMT_CEA_100_4096x2160P_30HZ:
			mode = payloadID_1080p30;
			break;
		default:
			mode = payloadID_default;
		}
		gmm626_set_mode(gmm626->spi, mode);
	}else {
		gmm626_set_standby(gmm626->spi, 1);
	}

	return NOTIFY_DONE;
}

static ssize_t mode_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct gmm626_data *gmm626 = spi_get_drvdata(spi);

	return sprintf(buf, "%d\n", gmm626->mode);
}

static ssize_t mode_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	int rc;
	u16 mode = 0;

	rc = kstrtou16(buf, 10, &mode);
	if (rc)
		return rc;

	gmm626_set_mode(spi, mode);

	return count;
}
static DEVICE_ATTR_RW(mode);

static ssize_t list_mode_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct gmm626_data *gmm626 = spi_get_drvdata(spi);
	size_t count = 0;
	int i;

	for (i = 0; i < gmm626->max_mode; i++)
		count += snprintf(buf + count, PAGE_SIZE - count, "%d:%s\n", i, g_payload_id[i].name);

	return count;
}
static DEVICE_ATTR_RO(list_mode);

static ssize_t payload_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	size_t count = 0;
	u16 payload[4] = {0};
	u16 ioproc_value;

	gmm626_write_data(spi, REG_IOPROC, 0x0200);
	gmm626_read_data(spi, REG_IOPROC, &ioproc_value);
	gmm626_read_data(spi, REG_VIDEO_FORMAT_352_OUT_WORD_1, &payload[0]);
	gmm626_read_data(spi, REG_VIDEO_FORMAT_352_OUT_WORD_2, &payload[1]);
	gmm626_read_data(spi, REG_VIDEO_FORMAT_352_OUT_WORD_3, &payload[2]);
	gmm626_read_data(spi, REG_VIDEO_FORMAT_352_OUT_WORD_4, &payload[3]);

	count += snprintf(buf + count, PAGE_SIZE - count, "IOPROC:0x%04x\n", ioproc_value);
	count += snprintf(buf + count, PAGE_SIZE - count, "PAYLOAD1:0x%04x\n", payload[0]);
	count += snprintf(buf + count, PAGE_SIZE - count, "PAYLOAD2:0x%04x\n", payload[1]);
	count += snprintf(buf + count, PAGE_SIZE - count, "PAYLOAD3:0x%04x\n", payload[2]);
	count += snprintf(buf + count, PAGE_SIZE - count, "PAYLOAD4:0x%04x\n", payload[3]);

	return count;
}
static DEVICE_ATTR_RO(payload);

static ssize_t rate0_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct gmm626_data *gmm626 = spi_get_drvdata(spi);

	gmm626->rate0 = gpiod_get_value_cansleep(gmm626->rate0_gpiod);

	return sprintf(buf, "%d\n", gmm626->rate0);
}

static ssize_t rate0_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	bool rate0;
	int rc;

	rc = kstrtobool(buf, &rate0);
	if (rc)
		return rc;

	gmm626_set_rate0(spi, rate0);

	return count;
}
static DEVICE_ATTR_RW(rate0);

static ssize_t rate1_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct gmm626_data *gmm626 = spi_get_drvdata(spi);

	gmm626->rate1 = gpiod_get_value_cansleep(gmm626->rate1_gpiod);

	return sprintf(buf, "%d\n", gmm626->rate1);
}

static ssize_t rate1_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	bool rate1;
	int rc;

	rc = kstrtobool(buf, &rate1);
	if (rc)
		return rc;

	gmm626_set_rate1(spi, rate1);

	return count;
}
static DEVICE_ATTR_RW(rate1);

static ssize_t standby_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct gmm626_data *gmm626 = spi_get_drvdata(spi);

	return sprintf(buf, "%d\n", gpiod_get_value_cansleep(gmm626->rate1_gpiod));
}

static ssize_t standby_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	bool standby;
	int rc;

	rc = kstrtobool(buf, &standby);
	if (rc)
		return rc;

	gmm626_set_standby(spi, standby);

	return count;
}
static DEVICE_ATTR_RW(standby);

static ssize_t reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u16 reg_val = 0;
	u16 i = 0;
	ssize_t len = 0;
	struct spi_device *spi = to_spi_device(dev);

	for (i = 0; i <= 0x15; i++) {
		gmm626_read_data(spi, i, &reg_val);
		len += snprintf(buf + len, PAGE_SIZE - len, "0x%04x:0x%04x\n", i, reg_val);
	}
	/* Add more here */

	return len;
}

static ssize_t
reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	u16 databuf[2];
	struct spi_device *spi = to_spi_device(dev);

	if (sscanf(buf, "%hx %hx", &databuf[0], &databuf[1]) == 2)
		gmm626_write_data(spi, databuf[0], databuf[1]);

	return len;
}
static DEVICE_ATTR_RW(reg);

/* add your attr in here*/
static struct attribute *gmm626_attributes[] = {
	&dev_attr_rate0.attr,
	&dev_attr_rate1.attr,
	&dev_attr_standby.attr,
	&dev_attr_mode.attr,
	&dev_attr_list_mode.attr,
	&dev_attr_payload.attr,
	&dev_attr_reg.attr,
	NULL
};

static struct attribute_group gmm626_attribute_group = {
	.attrs = gmm626_attributes
};

static void gmm626_reset(struct spi_device *spi)
{
	struct gmm626_data *gmm626 = spi_get_drvdata(spi);

	gpiod_set_value_cansleep(gmm626->reset_gpiod, 0);
	mdelay(1);
	gpiod_set_value_cansleep(gmm626->reset_gpiod, 1);
	mdelay(10);
	gpiod_set_value_cansleep(gmm626->standby_gpiod, 1);
	mdelay(10);
	gpiod_set_value_cansleep(gmm626->standby_gpiod, 0);
	mdelay(200);
	gpiod_set_value_cansleep(gmm626->standby_gpiod, 1);
	mdelay(1);
	gpiod_set_value_cansleep(gmm626->standby_gpiod, 0);
}

static int gmm626_parse_dt(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct gmm626_data *gmm626 = spi_get_drvdata(spi);

	gmm626->reset_gpiod = devm_gpiod_get_optional(dev, "gmm626,reset", GPIOD_OUT_HIGH);
	if (!gmm626->reset_gpiod) {
		dev_err(dev, "failed to get reset gpio\n");
		return -ENODEV;
	}

	gmm626->standby_gpiod = devm_gpiod_get_optional(dev, "gmm626,standby", GPIOD_OUT_HIGH);
	if (!gmm626->standby_gpiod) {
		dev_err(dev, "failed to get rate0 gpio\n");
		return -ENODEV;
	}

	gmm626->rate0_gpiod = devm_gpiod_get_optional(dev, "gmm626,rate0", GPIOD_OUT_LOW);
	if (!gmm626->rate0_gpiod) {
		dev_err(dev, "failed to get rate0 gpio\n");
		return -ENODEV;
	}

	gmm626->rate1_gpiod = devm_gpiod_get_optional(dev, "gmm626,rate1", GPIOD_OUT_HIGH);
	if (!gmm626->rate1_gpiod) {
		dev_err(dev, "failed to get rate1 gpio\n");
		return -ENODEV;
	}

	gmm626->rate0 = of_property_read_bool(dev->of_node, "rate0");
	gmm626->rate1 = of_property_read_bool(dev->of_node, "rate1");

	return 0;
}

static int gmm626_driver_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct gmm626_data *gmm626;
	int err = 0;

	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 16;
	err = spi_setup(spi);
	if (err < 0) {
		dev_err(&spi->dev, "spi setup failed.\n");
		return err;
	}

	gmm626 = devm_kzalloc(&spi->dev, sizeof(struct gmm626_data), GFP_KERNEL);
	if (!gmm626)
		return -ENOMEM;
	spi_set_drvdata(spi, gmm626);
	gmm626->spi = spi;

	err = gmm626_parse_dt(spi);
	if (err) {
		dev_err(dev, "failed to parse dts for gmm626\n");
		return err;
	}

	err = devm_device_add_group(&spi->dev, &gmm626_attribute_group);
	if (err) {
		dev_err(&spi->dev, "failed to add group for gmm626!\n");
		return err;
	}

	gmm626->max_mode = payloadID_max;
	gmm626_reset(spi);
	gmm626_set_rate_status(spi, gmm626->rate0, gmm626->rate1);

	gmm626->mode_nb.notifier_call = gmm626_mode_notifier;
	gsv2k11_notifier_register(&gmm626->mode_nb);

	dev_info(dev, "gmm626 probe success\n");

	return 0;
}

static int gmm626_driver_remove(struct spi_device *spi)
{
	struct gmm626_data *gmm626 = spi_get_drvdata(spi);

	gsv2k11_notifier_unregister(&gmm626->mode_nb);

	devm_device_remove_group(&spi->dev, &gmm626_attribute_group);

	gpiod_set_value_cansleep(gmm626->reset_gpiod, 0);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int gmm626_spi_suspend(struct device *dev)
{
	struct spi_device *spi = to_spi_device(dev);
	struct gmm626_data *gmm626 = spi_get_drvdata(spi);

	gpiod_set_value(gmm626->reset_gpiod, 0);

	return 0;
}

static int gmm626_spi_resume(struct device *dev)
{
	struct spi_device *spi = to_spi_device(dev);

	gmm626_reset(spi);

	return 0;
}
#endif

static const struct dev_pm_ops gmm626_spi_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(gmm626_spi_suspend, gmm626_spi_resume)
};

static const struct of_device_id gmm626_of_match[] = {
	{
		.compatible = "gmm626",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, gmm626_of_match);

static struct spi_driver gmm626_spi_driver = {
	.driver = {
		.name = "gmm626_spi_driver",
		.of_match_table = gmm626_of_match,
		.pm = &gmm626_spi_pm,
	},
	.probe = gmm626_driver_probe,
	.remove = gmm626_driver_remove,
};
module_spi_driver(gmm626_spi_driver);

MODULE_AUTHOR("<897420073@qq.com>");
MODULE_DESCRIPTION("Driver for gmm626");
MODULE_LICENSE("GPL and additional rights");
