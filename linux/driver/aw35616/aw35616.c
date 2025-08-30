#include <linux/extcon-provider.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>

#define AW35616_DEV_ID	0x01
#define AW35616_ID 0x16

#define AW35616_CTR		0x02
/* AW35616_CTR MASK */
#define ACCDIS			BIT(7)
#define TYR_MD			GENMASK(6, 5)
#define SRC_CUR_MD		GENMASK(4, 3)
#define WKMD			GENMASK(2, 1)
#define INTDIS			BIT(1)

#define AW35616_INT		0x03
/* AW35616_INT MASK */
#define WAKE_FLAG		BIT(2)
#define INTB_FLAG		GENMASK(1, 0)

#define AW35616_STATUS		0x04
/* AW35616_STATUS MASK */
#define VBUSOK			BIT(7)
#define SNK_CUR_MD		GENMASK(6, 5)
#define PLUG_ST			GENMASK(4, 2)
#define PLUG_ORI		GENMASK(1, 0)

#define AW35616_STATUS1		0x05
/* AW35616_STATUS1 MASK */
#define WAKE_ST			BIT(1)
#define ACTIVE_CABLE	BIT(0)

#define AW35616_RSTN	0x06
/* AW35616_RSTN MASK */
#define TPYEC_RSTN		BIT(1)
#define SFT_RSTN		BIT(0)

#define AW35616_USB0_VID0 0x07
#define AW35616_USB0_VID1 0x08

#define AW35616_CTR2	0x22
/* AW35616_CTR2 MASK */
#define TOG_SAVE_MD		GENMASK(2, 1)

#define USB_GPIO_DEBOUNCE_MS 20

#define AW35616_USB_HOST	BIT(3)
#define AW35616_USB_DEVICE	BIT(2)

#define AW35616_CC1		BIT(0)
#define AW35616_CC2		BIT(1)
#define AW35616_CC1_CC2 (AW35616_CC1 | AW35616_CC2)

struct aw35616_data {
	struct i2c_client *client;
	struct gpio_desc *irq_gpiod;
	struct gpio_desc *cc_switch_gpiod;
	int irq_gpio;
	struct regulator *vdd33;
	struct extcon_dev *edev;
	struct delayed_work usb_irq_work;
};

static const unsigned int aw35616_usb_extcon_cable[] = {
	EXTCON_USB,
	EXTCON_USB_HOST,
	EXTCON_NONE,
};

static int aw35616_read_byte(struct i2c_client *client, u8 addr)
{
	return i2c_smbus_read_byte_data(client, addr);
}

static int aw35616_write_byte(struct i2c_client *client, u8 addr, u8 value)
{
	return i2c_smbus_write_byte_data(client, addr, value);
}

static void aw35616_usb_irq_worker(struct work_struct *work)
{
	struct aw35616_data *aw35616 = container_of(to_delayed_work(work),
		struct aw35616_data, usb_irq_work);
	struct i2c_client *client = aw35616->client;
	u8 int_status = 0;
	u8 status = 0;
	u8 plug_oriention = 0;
	u8 attach_state = 0;
	u8 plug_st = 0;
	bool host_connected = false, device_connected = false;
	int polarity = 0;

	int_status = aw35616_read_byte(client, AW35616_INT);
	status = aw35616_read_byte(client, AW35616_STATUS);

	plug_st = status & PLUG_ST;
	if (plug_st == AW35616_USB_HOST) {
		device_connected = true;
	} else if (plug_st == AW35616_USB_DEVICE) {
		host_connected = true;
	}

	attach_state = int_status & INTB_FLAG;
	if (attach_state == 0x01) {
		/* attached */
		plug_oriention = status & PLUG_ORI;
		if (plug_oriention == AW35616_CC1) {
			/* CC1 */
			polarity = 0;
		} else if (plug_oriention == AW35616_CC2) {
			/* CC2 */
			polarity = 1;
		} else if (plug_oriention == AW35616_CC1_CC2) {
			/* CC1 & CC2 */
			/* msm not support, do nothing */
		}
		if (aw35616->cc_switch_gpiod) {
			polarity ? gpiod_set_value_cansleep(aw35616->cc_switch_gpiod, 1):
				gpiod_set_value_cansleep(aw35616->cc_switch_gpiod, 0);
		}
		extcon_set_property(aw35616->edev, EXTCON_USB,
			EXTCON_PROP_USB_TYPEC_POLARITY,
			(union extcon_property_value)polarity);
		extcon_set_property(aw35616->edev, EXTCON_USB_HOST,
			EXTCON_PROP_USB_TYPEC_POLARITY,
			(union extcon_property_value)polarity);

	} else {
		/* deattached */
	}

	extcon_set_state_sync(aw35616->edev, EXTCON_USB, device_connected);
	extcon_set_state_sync(aw35616->edev, EXTCON_USB_HOST, host_connected);

	/* clear interrupt */
	aw35616_write_byte(client, AW35616_INT, int_status);

	dev_dbg(&client->dev, "int_status = 0x%x\n", int_status);
	dev_dbg(&client->dev, "status = 0x%x\n", status);
	dev_dbg(&client->dev, "polarity = 0x%x\n", polarity);
	dev_dbg(&client->dev, "device_connected = 0x%x\n", device_connected);
	dev_dbg(&client->dev, "host_connected = 0x%x\n", host_connected);
}

static irqreturn_t aw35616_irq_handler(int irq, void *dev_id)
{
	struct aw35616_data *aw35616 = dev_id;

	schedule_delayed_work(&aw35616->usb_irq_work, msecs_to_jiffies(0));

	return IRQ_HANDLED;
}

static int aw35616_check_id(struct i2c_client *client)
{
	u8 id_expected = AW35616_ID;
	u8 id = 0;

	id = aw35616_read_byte(client, AW35616_DEV_ID);
	if (memcmp(&id, &id_expected, sizeof(u8))) {
		dev_err(&client->dev, "chip id mismatch\n");
		return -ENODEV;
	}

	/* reset chip */
	aw35616_write_byte(client, AW35616_RSTN, 0x3);

	return 0;
}

static ssize_t reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char reg_val = 0;
	unsigned char i = 0;
	ssize_t len = 0;
	struct aw35616_data *aw35616 = dev_get_drvdata(dev);
	struct i2c_client *client = aw35616->client;

	for (i = 0; i <= AW35616_USB0_VID1; i++) {
		reg_val = aw35616_read_byte(client, i);
		len += snprintf(buf + len, PAGE_SIZE - len, "reg:0x%02x=0x%02x\n", i, reg_val);
	}
	reg_val = aw35616_read_byte(client, AW35616_CTR2);
	len += snprintf(buf+len, PAGE_SIZE - len, "reg:0x%02x=0x%02x\n", AW35616_CTR2, reg_val);

	return len;
}

static ssize_t reg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned int databuf[2];
	struct aw35616_data *aw35616 = dev_get_drvdata(dev);
	struct i2c_client *client = aw35616->client;

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2)
		aw35616_write_byte(client, databuf[0], databuf[1]);

	return len;
}
static DEVICE_ATTR_RW(reg);

/* add your attr in here*/
static struct attribute *aw35616_attributes[] = {
	&dev_attr_reg.attr,
	NULL
};

static struct attribute_group aw35616_attribute_group = {
	.attrs = aw35616_attributes
};

static int aw35616_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct aw35616_data *aw35616 = NULL;
	struct device *dev = &client->dev;

	aw35616 = devm_kzalloc(&client->dev, sizeof(*aw35616), GFP_KERNEL);
	if (!aw35616) {
		dev_err(&client->dev, "failed to malloc data for aw35616\n");
		return -ENOMEM;
	}
	aw35616->client = client;
	i2c_set_clientdata(client, aw35616);

	aw35616->vdd33 = devm_regulator_get_optional(&client->dev, "vdd-3.3");
	if (IS_ERR(aw35616->vdd33)) {
		if (PTR_ERR(aw35616->vdd33) == -EPROBE_DEFER)
			return PTR_ERR(aw35616->vdd33);
		aw35616->vdd33 = NULL;
	}

	if (aw35616->vdd33) {
		ret = regulator_set_voltage(aw35616->vdd33, 3304000, 3304000);
		if (ret < 0) {
			dev_err(&client->dev, "failed to set voltage vdd33\n");
			goto put_vdd33;
		}

		ret = regulator_enable(aw35616->vdd33);
		if (ret < 0) {
			dev_err(&client->dev, "failed to enable vdd33\n");
			goto put_vdd33;
		}
	}

	ret = aw35616_check_id(client);
	if (ret < 0) {
		goto put_vdd33;
	}

	aw35616->edev = devm_extcon_dev_allocate(dev, aw35616_usb_extcon_cable);
	if (IS_ERR(aw35616->edev)) {
		dev_err(dev, "failed to allocate extcon device\n");
		ret = -ENOMEM;
		goto put_vdd33;
	}

	ret = devm_extcon_dev_register(dev, aw35616->edev);
	if (ret < 0) {
		dev_err(dev, "failed to register extcon device\n");
		goto put_vdd33;
	}

	extcon_set_property_capability(aw35616->edev, EXTCON_USB,
		EXTCON_PROP_USB_TYPEC_POLARITY);
	extcon_set_property_capability(aw35616->edev, EXTCON_USB_HOST,
		EXTCON_PROP_USB_TYPEC_POLARITY);

	INIT_DELAYED_WORK(&aw35616->usb_irq_work, aw35616_usb_irq_worker);

	aw35616->irq_gpiod = devm_gpiod_get_optional(dev, "aw35616,int", GPIOD_IN);
	if (!aw35616->irq_gpiod) {
		dev_err(dev, "failed to get irq gpio\n");
		return -ENODEV;
	}

	aw35616->cc_switch_gpiod = devm_gpiod_get_optional(dev, "aw35616,cc_switch", GPIOD_OUT_LOW);
	if (!aw35616->cc_switch_gpiod) {
		dev_dbg(dev, "failed to get irq gpio\n");
	}

	if (aw35616->irq_gpiod) {
		aw35616->irq_gpio = gpiod_to_irq(aw35616->irq_gpiod);
		if (aw35616->irq_gpio < 0) {
			dev_err(dev, "failed to IRQ for aw35616!, ret = %d\n", aw35616->irq_gpio);
			goto put_vdd33;
		}

		ret = devm_request_threaded_irq(dev, aw35616->irq_gpio, NULL,
									    aw35616_irq_handler,
									    IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
									    "aw35616-irq", aw35616);

		if (ret < 0) {
			dev_err(dev, "failed to request irq handler for aw35616\n");
			goto put_vdd33;

		}
	}
	device_set_wakeup_capable(dev, true);
	//aw35616_usb_irq_worker(&aw35616->usb_irq_work.work);

	ret = devm_device_add_group(&client->dev, &aw35616_attribute_group);
	if (ret) {
		dev_err(dev, "failed to add group attr for aw35616\n");
		goto put_vdd33;
	}

	dev_info(dev, "aw35616 probe success\n");

	return 0;

put_vdd33:
	if (aw35616->vdd33)
		regulator_disable(aw35616->vdd33);
	return ret;
}

static int aw35616_remove(struct i2c_client *client)
{
	struct aw35616_data *aw35616 = i2c_get_clientdata(client);

	cancel_delayed_work_sync(&aw35616->usb_irq_work);

	device_init_wakeup(&client->dev, false);

	if (aw35616->vdd33)
		regulator_disable(aw35616->vdd33);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int aw35616_suspend(struct device *dev)
{
	struct aw35616_data *aw35616 = dev_get_drvdata(dev);
	int ret = 0;

	if (device_may_wakeup(dev)) {
		if (aw35616->irq_gpio) {
			ret = enable_irq_wake(aw35616->irq_gpio);
			if (ret)
				return ret;
		}
	}

	/*
	 * We don't want to process any IRQs after this point
	 * as GPIOs used behind I2C subsystem might not be
	 * accessible until resume completes. So disable IRQ.
	 */
	if (aw35616->irq_gpiod)
		disable_irq(aw35616->irq_gpio);

	if (!device_may_wakeup(dev))
		pinctrl_pm_select_sleep_state(dev);

	return ret;
}

static int aw35616_resume(struct device *dev)
{
	struct aw35616_data *aw35616 = dev_get_drvdata(dev);
	int ret = 0;

	if (!device_may_wakeup(dev))
		pinctrl_pm_select_default_state(dev);

	if (device_may_wakeup(dev)) {
		if (aw35616->irq_gpiod) {
			ret = disable_irq_wake(aw35616->irq_gpio);
			if (ret)
				return ret;
		}
	}

	if (aw35616->irq_gpiod)
		enable_irq(aw35616->irq_gpio);

	cancel_delayed_work_sync(&aw35616->usb_irq_work);

	return ret;
}
#endif

static SIMPLE_DEV_PM_OPS(aw35616_pm_ops,
			 aw35616_suspend, aw35616_resume);

static const struct of_device_id aw35616_dt_match[] = {
	{.compatible = "awinic,aw35616", },
	{},
};
MODULE_DEVICE_TABLE(of, aw35616_dt_match);

static const struct i2c_device_id aw35616_ids[] = {
	{"aw35616", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, aw35616_ids);

static struct i2c_driver aw35616_i2c_driver = {
	.probe = aw35616_probe,
	.remove = aw35616_remove,
	.driver = {
		.name = "aw35616_i2c_driver",
		.owner = THIS_MODULE,
		.pm = &aw35616_pm_ops,
		.of_match_table = of_match_ptr(aw35616_dt_match),
	},
	.id_table = aw35616_ids,
};
module_i2c_driver(aw35616_i2c_driver);

MODULE_AUTHOR("<zhiwen.liang@hollyland-tech.com>");
MODULE_DESCRIPTION("Driver for aw35616");
MODULE_LICENSE("GPL");
