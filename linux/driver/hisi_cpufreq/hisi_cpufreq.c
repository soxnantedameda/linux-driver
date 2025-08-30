#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/thermal.h>

static LIST_HEAD(dvfs_info_list);

struct hisi_cpu_dvfs_info {
	struct cpumask cpus;
	struct device *cpu_dev;
	struct regulator *vcpu_reg;
	struct clk *cpu_clk;
	struct list_head list_head;
};

static int hisi_cpufreq_set_target(struct cpufreq_policy *policy,
				  unsigned int index)
{
	struct cpufreq_frequency_table *freq_table = policy->freq_table;
	struct clk *cpu_clk = policy->clk;
	struct hisi_cpu_dvfs_info *info = policy->driver_data;
	struct device *cpu_dev = info->cpu_dev;
	struct dev_pm_opp *opp;
	long freq_hz;
	int vcpu_uV, ret;

	freq_hz = freq_table[index].frequency * 1000;

	opp = dev_pm_opp_find_freq_ceil(cpu_dev, &freq_hz);
	if (IS_ERR(opp)) {
		pr_err("cpu%d: failed to find OPP for %ld\n",
			policy->cpu, freq_hz);
		return PTR_ERR(opp);
	}
	vcpu_uV = dev_pm_opp_get_voltage(opp);
	dev_pm_opp_put(opp);

	ret = regulator_set_voltage(info->vcpu_reg, vcpu_uV, vcpu_uV);
	if (ret < 0)
		return ret;

	ret = clk_set_rate(cpu_clk, freq_hz);
	if (ret < 0)
		return ret;

	return 0;
}

static struct hisi_cpu_dvfs_info *hisi_cpu_dvfs_info_lookup(int cpu)
{
	struct hisi_cpu_dvfs_info *info;

	list_for_each_entry(info, &dvfs_info_list, list_head) {
		if (cpumask_test_cpu(cpu, &info->cpus))
			return info;
	}

	return NULL;
}

static int hisi_cpufreq_init(struct cpufreq_policy *policy)
{
	struct hisi_cpu_dvfs_info *info;
	struct cpufreq_frequency_table *freq_table;
	int ret;

	info = hisi_cpu_dvfs_info_lookup(policy->cpu);
	if (!info) {
		pr_err("dvfs info for cpu%d is not initialized.\n",
		       policy->cpu);
		return -EINVAL;
	}

	ret = dev_pm_opp_init_cpufreq_table(info->cpu_dev, &freq_table);
	if (ret) {
		pr_err("failed to init cpufreq table for cpu%d: %d\n",
		       policy->cpu, ret);
		return ret;
	}

	cpumask_copy(policy->cpus, &info->cpus);
	policy->freq_table = freq_table;
	policy->driver_data = info;
	policy->clk = info->cpu_clk;

	dev_pm_opp_of_register_em(info->cpu_dev, policy->cpus);

	return 0;
}

static int hisi_cpufreq_exit(struct cpufreq_policy *policy)
{
	struct hisi_cpu_dvfs_info *info = policy->driver_data;

	dev_pm_opp_free_cpufreq_table(info->cpu_dev, &policy->freq_table);

	return 0;
}

static struct cpufreq_driver hisi_cpufreq_driver = {
	.flags = CPUFREQ_STICKY | CPUFREQ_NEED_INITIAL_FREQ_CHECK |
		 CPUFREQ_HAVE_GOVERNOR_PER_POLICY |
		 CPUFREQ_IS_COOLING_DEV,
	.verify = cpufreq_generic_frequency_table_verify,
	.target_index = hisi_cpufreq_set_target,
	.get = cpufreq_generic_get,
	.init = hisi_cpufreq_init,
	.exit = hisi_cpufreq_exit,
	.name = "hisi-cpufreq",
	.attr = cpufreq_generic_attr,
};

static int hisi_cpu_dvfs_info_init(struct hisi_cpu_dvfs_info *info, int cpu)
{
	struct device *cpu_dev;
	struct regulator *vcpu_reg = ERR_PTR(-ENODEV);
	struct clk *cpu_clk = ERR_PTR(-ENODEV);
	int ret;

	cpu_dev = get_cpu_device(cpu);
	if (!cpu_dev) {
		pr_err("failed to get cpu%d device\n", cpu);
		return -ENODEV;
	}

	cpu_clk = clk_get(cpu_dev, "cpu");
	if (IS_ERR(cpu_clk)) {
		if (PTR_ERR(cpu_clk) == -EPROBE_DEFER)
			pr_warn("cpu clk for cpu%d not ready, retry.\n", cpu);
		else
			pr_err("failed to get cpu clk for cpu%d\n", cpu);

		ret = PTR_ERR(cpu_clk);
		return ret;
	}

	ret = clk_prepare_enable(cpu_clk);
	if (ret < 0)
		return ret;

	vcpu_reg = regulator_get_exclusive(cpu_dev, "vcpu");
	if (IS_ERR(vcpu_reg)) {
		if (PTR_ERR(vcpu_reg) == -EPROBE_DEFER)
			pr_warn("vcpu regulator for cpu%d not ready, retry.\n", cpu);
		else
			pr_err("failed to get vcpu regulator for cpu%d\n", cpu);

		ret = PTR_ERR(vcpu_reg);
		goto out_free_resources;
	}

	/* Get OPP-sharing information from "operating-points-v2" bindings */
	ret = dev_pm_opp_of_get_sharing_cpus(cpu_dev, &info->cpus);
	if (ret) {
		pr_err("failed to get OPP-sharing information for cpu%d\n",
		       cpu);
		goto out_free_resources;
		return ret;
	}

	ret = dev_pm_opp_of_cpumask_add_table(&info->cpus);
	if (ret) {
		pr_warn("no OPP table for cpu%d\n", cpu);
		goto out_free_resources;
	}

	info->cpu_dev = cpu_dev;
	info->cpu_clk = cpu_clk;
	info->vcpu_reg = vcpu_reg;

	return 0;

out_free_resources:
	if (!IS_ERR(vcpu_reg))
		regulator_put(vcpu_reg);
	if (!IS_ERR(cpu_clk))
		clk_put(cpu_clk);

	return ret;
}

static void hisi_cpu_dvfs_info_release(struct hisi_cpu_dvfs_info *info)
{
	if (!IS_ERR(info->vcpu_reg))
		regulator_put(info->vcpu_reg);
	if (!IS_ERR(info->cpu_clk)) {
		clk_disable_unprepare(info->cpu_clk);
		clk_put(info->cpu_clk);
	}

	dev_pm_opp_of_cpumask_remove_table(&info->cpus);
}

static int hisi_cpufreq_probe(struct platform_device *pdev)
{
	struct hisi_cpu_dvfs_info *info, *tmp;
	int cpu, ret;

	for_each_possible_cpu(cpu) {
		info = hisi_cpu_dvfs_info_lookup(cpu);
		if (info)
			continue;

		info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
		if (!info) {
			ret = -ENOMEM;
			goto release_dvfs_info_list;
		}

		ret = hisi_cpu_dvfs_info_init(info, cpu);
		if (ret) {
			dev_err(&pdev->dev,
				"failed to initialize dvfs info for cpu%d\n",
				cpu);
			goto release_dvfs_info_list;
		}

		list_add(&info->list_head, &dvfs_info_list);
	}

	ret = cpufreq_register_driver(&hisi_cpufreq_driver);
	if (ret) {
		dev_err(&pdev->dev, "failed to register hisi cpufreq driver\n");
		goto release_dvfs_info_list;
	}

	return 0;

release_dvfs_info_list:
	list_for_each_entry_safe(info, tmp, &dvfs_info_list, list_head) {
		hisi_cpu_dvfs_info_release(info);
		list_del(&info->list_head);
	}

	return ret;
}

static int hisi_cpufreq_remove(struct platform_device *pdev)
{
	struct hisi_cpu_dvfs_info *info, *tmp;

	list_for_each_entry_safe(info, tmp, &dvfs_info_list, list_head) {
		hisi_cpu_dvfs_info_release(info);
		list_del(&info->list_head);
	}

	return cpufreq_unregister_driver(&hisi_cpufreq_driver);
}

static const struct of_device_id of_hisi_cpufreq_match[] = {
	{ .compatible = "hi3519dv500", },
	{ /* end */ }
};
MODULE_DEVICE_TABLE(of, of_hisi_cpufreq_match);

static struct platform_driver hisi_cpufreq_platdrv = {
	.driver = {
		.name		= "hisi-cpufreq",
		.of_match_table = of_hisi_cpufreq_match,
	},
	.probe	= hisi_cpufreq_probe,
	.remove	= hisi_cpufreq_remove,
};
module_platform_driver(hisi_cpufreq_platdrv);

MODULE_AUTHOR("<zhiwen.liang@hollyland-tech.com>");
MODULE_DESCRIPTION("Driver for hisi cpufreq");
MODULE_LICENSE("GPL and additional rights");
