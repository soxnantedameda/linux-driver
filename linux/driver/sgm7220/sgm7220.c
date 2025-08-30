#include <linux/extcon-provider.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>

#define SGM7220_ID_NUM 8
#define SGM7220_ID0 0x00
#define SGM7220_ID1 0x01
#define SGM7220_ID2 0x02
#define SGM7220_ID3 0x03
#define SGM7220_ID4 0x04
#define SGM7220_ID5 0x05
#define SGM7220_ID6 0x06
#define SGM7220_ID7 0x07

#define SGM7220_ADDR1 0x08
#define SGM7220_ADDR2 0x09
#define SGM7220_ADDR3 0x0a
#define SGM7220_ADDR4 0x45
/* SGM7220_ADDR1 MASK */
#define CURRENT_MODE_ADVERTISE	0xc0
#define CURRENT_MODE_DETECT		0x30
#define ACCESSORY_CONNECTED		0x0e
#define ACTIVE_CABLE_DETECTION	0x01
/* SGM7220_ADDR2 MASK */
#define ATTACHED_STATE			0xc0
#define CABLE_DIR				0x20
#define INTERRUPT_STATUS		0x10
#define DRP_DUTY_CYCLE			0x06
#define DISABLE_UFP_ACCESSORY	0x01
/* SGM7220_ADDR3 MASK */
#define DEBOUNCE		0xc0
#define MODE_SELECT		0x30
#define I2C_SOFT_RESET	0x08
#define SOURCE_PREF		0x06
#define DISABLE_TERM	0x01
/* SGM7220_ADDR4 MASK */
#define DISABLE_RD_RP	0x04

#define USB_HOST		BIT(6)
#define USB_DEVICE		BIT(7)
#define USB_ACCESSORY	(USB_HOST | USB_DEVICE)

#define USB_GPIO_DEBOUNCE_MS 20

struct sgm7220_data {
	struct i2c_client *client;

	struct gpio_desc *irq_gpiod;
	struct gpio_desc *switch3_0_gpiod;  /* used for usb3.0 */
	struct gpio_desc *switch2_0_gpiod; /* used for usb2.0 */
	struct gpio_desc *id_gpiod;
	int irq_gpio;

	struct extcon_dev *edev;

	struct workqueue_struct *sgm7220_wq;
	struct delayed_work usb_irq_work;

	bool debug_flag;
};

static const unsigned int sgm7220_usb_extcon_cable[] = {
	EXTCON_USB,
	EXTCON_USB_HOST,
	EXTCON_NONE,
};

static int sgm7220_read_byte(struct i2c_client *client, uint8_t addr)
{
	return i2c_smbus_read_byte_data(client, addr);
}

static int sgm7220_write_byte(struct i2c_client *client, uint8_t addr, uint8_t value)
{
	return i2c_smbus_write_byte_data(client, addr, value);
}

static void sgm7220_usb_irq_worker(struct work_struct *work)
{
	struct sgm7220_data *data = container_of(to_delayed_work(work),
		struct sgm7220_data, usb_irq_work);
	struct i2c_client *client = data->client;
	uint8_t value = 0;
	uint8_t cable_dir = 0;
	uint8_t attach_state = 0;
	int id = 0;

	value = sgm7220_read_byte(client, SGM7220_ADDR2);
	attach_state = value & ATTACHED_STATE;
	cable_dir = value & CABLE_DIR;


	if (attach_state == USB_ACCESSORY) {
		/* To do: accessory action */
		dev_info(&client->dev, "Accessory mode!\n");
	} else if (attach_state) {
		id = attach_state & USB_DEVICE;
	} else {
		if (data->id_gpiod)
			id = gpiod_get_value(data->id_gpiod);
		else
			id = USB_DEVICE;
	}

	if (data->switch3_0_gpiod)
		gpiod_set_value(data->switch3_0_gpiod, !!cable_dir ? 0 : 1);

	if (data->switch2_0_gpiod)
		gpiod_set_value(data->switch2_0_gpiod, !!attach_state ? 0 : 1);

	if (data->edev)
		extcon_set_state_sync(data->edev, EXTCON_USB_HOST, !!id ? false : true);
	/* clear irq */
	sgm7220_write_byte(client, SGM7220_ADDR2, value);

	if (data->debug_flag) {
		dev_info(&client->dev, "attach_state = 0x%x\n", attach_state);
		dev_info(&client->dev, "cable_dir = 0x%x\n", !!cable_dir);
		dev_info(&client->dev, "id = 0x%x\n", id);
	}
}

static irqreturn_t sgm7220_irq_handler(int irq, void *dev_id)
{
	struct sgm7220_data *data = dev_id;

	queue_delayed_work(data->sgm7220_wq, &data->usb_irq_work, msecs_to_jiffies(0));

	return IRQ_HANDLED;
}

static int sgm7220_check_id(struct i2c_client *client)
{
	int i = 0;
	uint8_t id_expected[SGM7220_ID_NUM] = {
		0x30, 0x32, 0x33, 0x42,
		0x53, 0x55, 0x54, 0x00};
	uint8_t id[SGM7220_ID_NUM] = {0};

	for (i = 0; i < SGM7220_ID_NUM; i++) {
		id[i] = sgm7220_read_byte(client, SGM7220_ID0 + i);
	}
	if (memcmp(id, id_expected, sizeof(uint8_t) * SGM7220_ID_NUM)) {
		dev_err(&client->dev, "chip id mismatch\n");
		return -ENODEV;
	}

	return 0;
}

static ssize_t reg_show(struct device *cd, struct device_attribute *attr, char *buf)
{
	unsigned char reg_val = 0;
	unsigned char i = 0;
	ssize_t len = 0;
	struct sgm7220_data *data = dev_get_drvdata(cd);
	struct i2c_client *client = data->client;

	for (i = 0; i <= 0xA; i++) {
		reg_val = sgm7220_read_byte(client, i);
		len += snprintf(buf + len, PAGE_SIZE - len, "reg:0x%02x=0x%02x\n", i, reg_val);
	}
	reg_val = sgm7220_read_byte(client, 0x45);
	len += snprintf(buf+len, PAGE_SIZE - len, "reg:0x%02x=0x%02x\n", 0x45, reg_val);

	return len;
}

static ssize_t
reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned int databuf[2];
	struct sgm7220_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2)
		sgm7220_write_byte(client, databuf[0], databuf[1]);

	return len;
}
static DEVICE_ATTR_RW(reg);

static ssize_t debug_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sgm7220_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "0x%x\n", data->debug_flag);
}

static ssize_t debug_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sgm7220_data *data = i2c_get_clientdata(client);
	bool debug_flag;
	int rc;

	rc = kstrtobool(buf, &debug_flag);
	if (rc)
		return rc;

	data->debug_flag = debug_flag;

	return count;
}
static DEVICE_ATTR_RW(debug);

/* add your attr in here*/
static struct attribute *sgm7220_attributes[] = {
	&dev_attr_reg.attr,
	&dev_attr_debug.attr,
	NULL
};

static struct attribute_group sgm7220_attribute_group = {
	.attrs = sgm7220_attributes
};

static int sgm7220_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct sgm7220_data *data = NULL;
	struct device *dev = &client->dev;

	ret = sgm7220_check_id(client);
	if (ret < 0) {
		return ret;
	}

	data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_err(&client->dev, "failed to malloc data for sgm7220\n");
		return -ENOMEM;
	}
	data->client = client;
	i2c_set_clientdata(client, data);

	data->id_gpiod = devm_gpiod_get_optional(dev, "sgm7220,id", GPIOD_IN);
	if (!data->id_gpiod) {
		dev_info(dev, "failed to get id gpio\n");
	}
	if (data->id_gpiod)
		gpiod_set_debounce(data->id_gpiod, USB_GPIO_DEBOUNCE_MS * 1000);

	data->irq_gpiod = devm_gpiod_get_optional(dev, "sgm7220,irq", GPIOD_IN);
	if (!data->irq_gpiod) {
		dev_err(dev, "failed to get irq gpio\n");
		return -ENODEV;
	}

	data->switch3_0_gpiod = devm_gpiod_get_optional(dev, "sgm7220,switch3_0", GPIOD_OUT_LOW);
	if (!data->switch3_0_gpiod) {
		dev_err(dev, "failed to get switch gpio\n");
		return -ENODEV;
	}

	data->switch2_0_gpiod = devm_gpiod_get_optional(dev, "sgm7220,switch2_0", GPIOD_OUT_LOW);
	if (!data->switch2_0_gpiod) {
		dev_err(dev, "failed to get switch gpio\n");
		return -ENODEV;
	}

	data->edev = devm_extcon_dev_allocate(dev, sgm7220_usb_extcon_cable);
	if (IS_ERR(data->edev)) {
		dev_err(dev, "failed to allocate extcon device\n");
		return -ENOMEM;
	}
	ret = devm_extcon_dev_register(dev, data->edev);
	if (ret < 0) {
		dev_err(dev, "failed to register extcon device\n");
		return ret;
	}

	data->sgm7220_wq = create_singlethread_workqueue("sgm7220-wq");
	if (!data->sgm7220_wq) {
		dev_err(dev, "failed to create workqueue for sgm7220\n");
		return -EINVAL;
	}
	INIT_DELAYED_WORK(&data->usb_irq_work, sgm7220_usb_irq_worker);

	if (data->irq_gpiod) {
		data->irq_gpio = gpiod_to_irq(data->irq_gpiod);
		if (data->irq_gpio < 0) {
			dev_err(dev, "failed to IRQ for sgm 7220!\n");
			return data->irq_gpio;
		}

		ret = devm_request_threaded_irq(dev, data->irq_gpio, NULL,
									    sgm7220_irq_handler,
									    IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
									    "sgm7220-irq", data);
		if (ret < 0) {
			dev_err(dev, "failed to request irq handler for sgm7220\n");
			return ret;
		}
	}
	device_set_wakeup_capable(dev, true);
	/* DRP will perform Try.SNK */
	sgm7220_write_byte(client, SGM7220_ADDR3, SOURCE_PREF & 0x2);
	queue_delayed_work(data->sgm7220_wq, &data->usb_irq_work, msecs_to_jiffies(0));

	ret = devm_device_add_group(&client->dev, &sgm7220_attribute_group);
	if (ret) {
		dev_err(dev, "failed to add group attr for sgm7220\n");
		return ret;
	}

	dev_info(dev, "sgm7220 probe success\n");

	return 0;
}

static int sgm7220_remove(struct i2c_client *client)
{
	struct sgm7220_data *data = i2c_get_clientdata(client);

	devm_device_remove_group(&client->dev, &sgm7220_attribute_group);

	cancel_delayed_work_sync(&data->usb_irq_work);

	free_irq(data->irq_gpio, data);

	if (data->sgm7220_wq) {
		destroy_workqueue(data->sgm7220_wq);
	}

	device_init_wakeup(&client->dev, false);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int sgm7220_suspend(struct device *dev)
{
	struct sgm7220_data *data = dev_get_drvdata(dev);
	int ret = 0;

	if (device_may_wakeup(dev)) {
		if (data->irq_gpio) {
			ret = enable_irq_wake(data->irq_gpio);
			if (ret)
				return ret;
		}
	}

	/*
	 * We don't want to process any IRQs after this point
	 * as GPIOs used behind I2C subsystem might not be
	 * accessible until resume completes. So disable IRQ.
	 */
	if (data->irq_gpiod)
		disable_irq(data->irq_gpio);

	if (!device_may_wakeup(dev))
		pinctrl_pm_select_sleep_state(dev);

	return ret;
}

static int sgm7220_resume(struct device *dev)
{
	struct sgm7220_data *data = dev_get_drvdata(dev);
	int ret = 0;

	if (!device_may_wakeup(dev))
		pinctrl_pm_select_default_state(dev);

	if (device_may_wakeup(dev)) {
		if (data->irq_gpiod) {
			ret = disable_irq_wake(data->irq_gpio);
			if (ret)
				return ret;
		}
	}

	if (data->irq_gpiod)
		enable_irq(data->irq_gpio);

	cancel_delayed_work_sync(&data->usb_irq_work);

	return ret;
}
#endif

static SIMPLE_DEV_PM_OPS(sgm7220_pm_ops,
			 sgm7220_suspend, sgm7220_resume);

static const struct of_device_id sgm7220_dt_match[] = {
	{.compatible = "sgm7220", },
	{},
};
MODULE_DEVICE_TABLE(of, sgm7220_dt_match);

static const struct i2c_device_id sgm7220_ids[] = {
	{"sgm7220", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, sgm7220_ids);

static struct i2c_driver sgm7220_i2c_driver = {
	.probe = sgm7220_probe,
	.remove = sgm7220_remove,
	.driver = {
		.name = "sgm7220_i2c_driver",
		.owner = THIS_MODULE,
		.pm = &sgm7220_pm_ops,
		.of_match_table = of_match_ptr(sgm7220_dt_match),
	},
	.id_table = sgm7220_ids,
};
module_i2c_driver(sgm7220_i2c_driver);

MODULE_AUTHOR("<zhiwen.liang@hollyland-tech.com>");
MODULE_DESCRIPTION("Driver for sgm7220");
MODULE_LICENSE("GPL and additional rights");
