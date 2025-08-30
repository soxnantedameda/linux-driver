#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/regmap.h>

#include "sh5001a.h"

static const struct regmap_config sh5001a_i2c_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int sh5001a_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct regmap *regmap;

	regmap = devm_regmap_init_i2c(client, &sh5001a_i2c_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(&client->dev, "Failed to register i2c regmap %d\n",
			(int)PTR_ERR(regmap));
		return PTR_ERR(regmap);
	}

	return sh5001a_probe(&client->dev, regmap);
}

static int sh5001a_i2c_remove(struct i2c_client *client)
{
	return sh5001a_remove(&client->dev);
}

void sh5001a_i2c_shutdown(struct i2c_client *client)
{
	sh5001a_shutdown(&client->dev);
}

static const struct of_device_id sh5001a_i2c_of_match[] = {
	{
		.compatible = "sh5001a",
	},
	{}
};
MODULE_DEVICE_TABLE(of, sh5001a_i2c_of_match);

static struct i2c_driver sh5001a_driver = {
	.driver = {
		.name = "sh5001a_i2c",
		//.pm = &sh5001a_pm_ops,
		.of_match_table = of_match_ptr(sh5001a_i2c_of_match),
	},
	.probe = sh5001a_i2c_probe,
	.remove = sh5001a_i2c_remove,
	.shutdown = sh5001a_i2c_shutdown,
};
module_i2c_driver(sh5001a_driver);

MODULE_AUTHOR("<zhiwen.liang@hollyland-tech.com>");
MODULE_DESCRIPTION("Driver for sh5001a");
MODULE_LICENSE("GPL and additional rights");
