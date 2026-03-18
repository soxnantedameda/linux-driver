#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/regmap.h>

#include "qma6101t.h"

static const struct regmap_config qma6101t_i2c_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int qma6101t_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct regmap *regmap;

	regmap = devm_regmap_init_i2c(client, &qma6101t_i2c_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(&client->dev, "Failed to register i2c regmap %d\n",
			(int)PTR_ERR(regmap));
		return PTR_ERR(regmap);
	}

	return qma6101t_probe(&client->dev, regmap);
}

static void qma6101t_i2c_remove(struct i2c_client *client)
{
	qma6101t_remove(&client->dev);
}

void qma6101t_i2c_shutdown(struct i2c_client *client)
{
	qma6101t_shutdown(&client->dev);
}

static const struct of_device_id qma6101t_i2c_of_match[] = {
	{ .compatible = "qma6101t", },
	{}
};
MODULE_DEVICE_TABLE(of, qma6101t_i2c_of_match);

static DEFINE_SIMPLE_DEV_PM_OPS(qma6101t_pm_ops, qma6101t_suspend,
				qma6101t_resume);

static struct i2c_driver qma6101t_driver = {
	.driver = {
		.name = "qma6101t_i2c",
		.pm = &qma6101t_pm_ops,
		.of_match_table = of_match_ptr(qma6101t_i2c_of_match),
	},
	.probe = qma6101t_i2c_probe,
	.remove = qma6101t_i2c_remove,
	.shutdown = qma6101t_i2c_shutdown,
};
module_i2c_driver(qma6101t_driver);

MODULE_AUTHOR("<897420073@qq.com>");
MODULE_DESCRIPTION("Driver for qma6101t");
MODULE_LICENSE("GPL");
