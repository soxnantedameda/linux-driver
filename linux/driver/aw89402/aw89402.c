#include <linux/clk.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <linux/regmap.h>

#define AW89402_IDH 0x00
#define AW89402_IDL 0x01
#define AW89402_SYSST 0x02
#define AW89402_SYSINT 0x03
#define AW89402_SYSINTM 0x04

#define AW89402_SYSCTRL1 0x06
#define VOL_CZBYP_MASK BIT(7)
#define INTN_MASK BIT(5)
#define I2S_EN_MASK BIT(4)
#define SYSCE_MASK BIT(1)
#define PWDN_MASK BIT(0)

#define AW89402_SYSCTRL2 0x07
#define SYSCTRL2_I2C_WEN_MASK GENMASK(7, 6)
#define CH4_EN_MASK BIT(5)
#define CH3_EN_MASK BIT(4)
#define CH2_EN_MASK BIT(3)
#define CH1_EN_MASK BIT(2)
#define ALL_CH_MASK GENMASK(5, 2)

#define AW89402_CHCTRL1 0x08
#define AW89402_CHCTRL2 0x09
#define AW89402_CHCTRL3 0x0A
#define AW89402_CHCTRL4 0x0B
#define CH_HAGCE_MASK BIT(7)
#define CH_HDCCE_MASK BIT(6)
#define CH_HMUTE_MASK BIT(5)

#define AW89402_VOLCTRL1 0x0C
#define AW89402_VOLCTRL2 0x0D
#define AW89402_VOLCTRL3 0x0E
#define AW89402_VOLCTRL4 0x0F
#define AW89402_VOLCTRL5 0x10
#define AW89402_VOLCTRL6 0x11
#define AW89402_VOLCTRL7 0x12
#define AW89402_VOLCTRL8 0x13

#define AW89402_I2SCTRL1 0x20
#define I2S_CLK_MODE_MASK BIT(7)
#define I2SSR_MASK GENMASK(4, 0)

#define AW89402_I2SCTRL2 0x21
#define AW89402_I2SCTRL3 0x22
#define AW89402_I2SCTRL4 0x23
#define I2S_WORD_WIDTH_MASK GENMASK(6, 0)

#define AW89402_I2SCTRL5 0x24
#define I2S_BIT_DEPTH_MASK GENMASK(4, 0)

#define AW89402_I2SCTRL6 0x25
#define AW89402_I2SCTRL7 0x26
#define AW89402_I2SCTRL8 0x27
#define AW89402_I2SCTRL9 0x28
#define AW89402_I2SCTRL10 0x29

#define AW89402_PDMCTRL 0x2E
#define AW89402_ADC1CTRL2 0x31
#define AW89402_ADC2CTRL2 0x33
#define AW89402_ADC3CTRL2 0x35
#define AW89402_ADC4CTRL2 0x37
#define PGA1_GAINSEL_MASK GENMASK(5, 0)

#define AW89402_ADCSCTRL5 0x3C
#define SDM_PDIREF_MASK BIT(2)
#define AW89402_ADCSCTRL6 0x3D
#define VCM_FSTUP_MASK BIT(2)
#define VCM_EN_MASK BIT(1)
#define AW89402_ADCSCTRL7 0x3E
#define CH12_ADC_CLKEN_MASK BIT(0)
#define AW89402_ADCSCTRL8 0x3F
#define CH34_ADC_CLKEN_MASK BIT(0)

#define AW89402_MICBIAS12CTRL1 0x40
#define MICB12_SEL_MASK GENMASK(5, 2)
#define MICB12_FST_MASK BIT(1)
#define MICB12_PWD_MASK BIT(0)

#define AW89402_MICBIAS34CTRL1 0x42
#define MICB34_SEL_MASK GENMASK(5, 2)
#define MICB34_FST_MASK BIT(1)
#define MICB34_PWD_MASK BIT(0)

#define AW89402_PLL_CTRL2 0x45
#define PLL_PD_MASK BIT(6)
#define AW89402_PLL_CTRL6 0x49

#define AW89402_MISCCTRL2 0x4C
#define LDO_LP_EN_MASK BIT(3)
#define BG_V2I_EN_MASK BIT(2)
#define AW89402_ANA_SYSCTRL 0x4F

#define AW89402_CH1_DLY 0x50
#define AW89402_CH2_DLY 0x51
#define AW89402_CH3_DLY 0x52
#define AW89402_CH4_DLY 0x53

#define AW89402_HDCC_COEF1 0x54
#define AW89402_HDCC_COEF2 0x55

#define AW89402_ANASTA1 0x5A
#define AW89402_ANASTA2 0x5B
#define AW89402_ANASTA3 0x5C
#define AW89402_ANASTA4 0x5D

#define AW89402_AGC_CTRL1 0x60
#define AW89402_AGC_CTRL2 0x61
#define AW89402_AGC_CTRL3 0x62
#define AW89402_AGC_CTRL4 0x63
#define AW89402_AGC_CTRL6 0x64
#define AW89402_AGC_CTRL8 0x65
#define AW89402_AGC_CTRL9 0x66
#define AW89402_AGC_CTRL10 0x67
#define AW89402_AGC_CTRL11 0x68
#define AW89402_AGC_CTRL12 0x69

#define AW89402_IOCTRL2 0x6F

struct aw89402_priv {
	struct regmap *regmap;
	struct i2c_client *i2c;
	struct regulator *supply;
	struct gpio_desc *enable_gpiod;

	u8 pdm_dmic_1_enable;
	u8 pdm_dmic_2_enable;
	u8 pdm_dmic_3_enable;
	u8 pdm_dmic_4_enable;

	u8 pll_enabled;
	u8 core_enabled;

	struct clk *mclk;
	unsigned int sysclk;
	const struct snd_pcm_hw_constraint_list *sysclk_constraints;
	const int *mclk_ratios;
	u8 tdm_mode;

	bool sclkinv;
	u8 mastermode;
};

typedef struct {
	u8 addr;
	u8 data;
} reg_tab;

const reg_tab init_reg_tab[] = {
	{0x04, 0xff},
	{0x06, 0x12},
	{0x07, 0xbf},
	{0x08, 0x40},
	{0x09, 0x40},
	{0x0a, 0x40},
	{0x0b, 0x40},
	{0x0c, 0x20},
	{0x0d, 0x3f},
	{0x0e, 0x20},
	{0x0f, 0x3f},
	{0x10, 0x20},
	{0x11, 0x3f},
	{0x12, 0x20},
	{0x13, 0x3f},
	{0x14, 0x00},
	{0x15, 0x00},
	{0x20, 0x08},
	{0x21, 0x34},
	{0x22, 0x01},
	{0x23, 0x0f},
	{0x24, 0x0f},
	{0x25, 0x1a},
	{0x26, 0x00},
	{0x27, 0x01},
	{0x28, 0x10},
	{0x29, 0x11},
	{0x2a, 0x00},
	{0x2c, 0x00},
	{0x2d, 0xc3},
	{0x2e, 0x02},
	{0x30, 0x05},
	{0x31, 0x9d},
	{0x32, 0x04},
	{0x33, 0x1d},
	{0x34, 0x04},
	{0x35, 0x86},
	{0x36, 0x04},
	{0x37, 0x86},
	{0x38, 0xf7},
	{0x39, 0xe1},
	{0x3a, 0x00},
	{0x3b, 0x05},
	{0x3c, 0x71},
	{0x3d, 0x62},
	{0x3e, 0x01},
	{0x3f, 0x01},
	{0x40, 0x88},
	{0x41, 0x20},
	{0x42, 0x88},
	{0x43, 0x20},
	{0x44, 0xa2},
	{0x45, 0x00},
	{0x46, 0x00},
	{0x47, 0x03},
	{0x48, 0x00},
	{0x49, 0x10},
	{0x4a, 0x00},
	{0x4b, 0x00},
	{0x4c, 0x84},
	{0x4d, 0x40},
	{0x4e, 0x00},
	{0x4f, 0x00},
	{0x50, 0x00},
	{0x51, 0x00},
	{0x52, 0x00},
	{0x53, 0x00},
	{0x54, 0x44},
	{0x55, 0xc4},
	{0x56, 0x13},
	{0x57, 0x45},
	{0x58, 0x04},
	{0x5a, 0x1d},
	{0x5b, 0x1d},
	{0x5c, 0x06},
	{0x5d, 0x06},
	{0x5f, 0x54},
	{0x60, 0x6b},
	{0x61, 0x08},
	{0x62, 0x09},
	{0x63, 0x05},
	{0x64, 0x01},
	{0x65, 0x06},
	{0x66, 0x02},
	{0x67, 0x83},
	{0x68, 0x27},
	{0x69, 0x11},
	{0x6f, 0x1f},
	{0x71, 0x00},
	{0x72, 0x00},
	{0x73, 0x00},
	{0x7d, 0x32},
	{0x7f, 0x00}
};

static const struct regmap_config aw89402_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x6F,
	.cache_type = REGCACHE_RBTREE,
};

static const unsigned int rates_12288[] = {
	8000, 11000, 12000, 16000, 22000, 24000,
	32000, 44000, 48000, 64000, 88000, 96000,
};

static const int ratios_12288[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

static const struct snd_pcm_hw_constraint_list constraints_12288 = {
	.count	= ARRAY_SIZE(rates_12288),
	.list	= rates_12288,
};



static int aw89402_pcm_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(component);

	printk("%s %d\n", __func__, __LINE__);
	if (aw89402->mastermode && aw89402->sysclk_constraints)
		snd_pcm_hw_constraint_list(substream->runtime, 0,
			SNDRV_PCM_HW_PARAM_RATE, aw89402->sysclk_constraints);

	return 0;
}

static int aw89402_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *hw_params, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(component);

	printk("%s %d\n", __func__, __LINE__);

	switch (params_format(hw_params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		regmap_write_bits(aw89402->regmap, AW89402_I2SCTRL5, I2S_BIT_DEPTH_MASK, 0xF);
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		regmap_write_bits(aw89402->regmap, AW89402_I2SCTRL5, I2S_BIT_DEPTH_MASK, 0x13);
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		regmap_write_bits(aw89402->regmap, AW89402_I2SCTRL5, I2S_BIT_DEPTH_MASK, 0x17);
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		regmap_write_bits(aw89402->regmap, AW89402_I2SCTRL5, I2S_BIT_DEPTH_MASK, 0x1F);
		break;
	default:
		regmap_write_bits(aw89402->regmap, AW89402_I2SCTRL5, I2S_BIT_DEPTH_MASK, 0x1F);
	}

	return 0;
}

static int aw89402_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_component *component = dai->component;
	struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(component);

	printk("%s %d\n", __func__, __LINE__);
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		/* Master serial port mode, with BCLK generated automatically */
		snd_soc_component_update_bits(component, AW89402_I2SCTRL1,
			I2S_CLK_MODE_MASK, 1 << 7);
		aw89402->mastermode = true;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		/* Slave serial port mode */
		snd_soc_component_update_bits(component, AW89402_I2SCTRL1,
			I2S_CLK_MODE_MASK, 0 << 7);
		aw89402->mastermode = false;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int aw89402_set_dai_sysclk(struct snd_soc_dai *dai,
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_component *component = dai->component;
	struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(component);

	printk("%s %d\n", __func__, __LINE__);

	switch (freq) {
	case 0:
		aw89402->sysclk_constraints = NULL;
		aw89402->sysclk = freq;
		aw89402->mclk_ratios = NULL;
		break;
	case 12288000:
		aw89402->sysclk_constraints = &constraints_12288;
		aw89402->sysclk = freq;
		aw89402->mclk_ratios = ratios_12288;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int aw89402_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_component *component = dai->component;
	struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(component);

	printk("%s %d\n", __func__, __LINE__);
	if (mute) {
		regmap_write_bits(aw89402->regmap, AW89402_CHCTRL1, CH_HMUTE_MASK, 1 << 5);
		regmap_write_bits(aw89402->regmap, AW89402_CHCTRL2, CH_HMUTE_MASK, 1 << 5);
		regmap_write_bits(aw89402->regmap, AW89402_CHCTRL3, CH_HMUTE_MASK, 1 << 5);
		regmap_write_bits(aw89402->regmap, AW89402_CHCTRL4, CH_HMUTE_MASK, 1 << 5);
	} else {
		regmap_write_bits(aw89402->regmap, AW89402_CHCTRL1, CH_HMUTE_MASK, 0 << 5);
		regmap_write_bits(aw89402->regmap, AW89402_CHCTRL2, CH_HMUTE_MASK, 0 << 5);
		regmap_write_bits(aw89402->regmap, AW89402_CHCTRL3, CH_HMUTE_MASK, 0 << 5);
		regmap_write_bits(aw89402->regmap, AW89402_CHCTRL4, CH_HMUTE_MASK, 0 << 5);
	}

	return 0;
}

static struct snd_soc_dai_ops aw89402_ops = {
	.startup = aw89402_pcm_startup,
	.hw_params = aw89402_pcm_hw_params,
	.set_fmt = aw89402_set_dai_fmt,
	.set_sysclk = aw89402_set_dai_sysclk,
	.digital_mute = aw89402_mute,
};

#define AW89402_RATES SNDRV_PCM_RATE_8000_192000

#define AW89402_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
	SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_driver aw89402_dai = {
	.name = "AW89402 4CH ADC",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 4,
		.rates = AW89402_RATES,
		.formats = AW89402_FORMATS,
	},
	.ops = &aw89402_ops,
	.symmetric_rates = 1,
};

static int aw89402_config(struct aw89402_priv *aw89402,
	const reg_tab* tab, unsigned int array_cnt)
{
	int ret = 0;
	int i = 0;
	u8 addr = 0;
	u8 data = 0;

	regmap_write_bits(aw89402->regmap, AW89402_SYSCTRL2, SYSCTRL2_I2C_WEN_MASK, 2 << 6);

	for (i = 0; i < array_cnt; i++) {
		addr = tab[i].addr;
		data = tab[i].data;

		if (addr == AW89402_SYSCTRL2) {
			data &= ~SYSCTRL2_I2C_WEN_MASK;
			data |= 2 << 6;
		} else if (addr == AW89402_SYSCTRL1) {
			data &= ~PWDN_MASK;
			data |= 1 << 0;
		} else if (addr == AW89402_PLL_CTRL2) {
			aw89402->pll_enabled = (data & PLL_PD_MASK) == (0 << 6);
			data &= ~PLL_PD_MASK;
			data |= 1 << 6;
		} else if (addr == 0x4D) {
			data &= ~(((1 << 1) - 1) << 7);
			data |= 1 << 7;
		} else if (addr == AW89402_MISCCTRL2) {
			data &= ~BG_V2I_EN_MASK;
			data |= 0 << 2;
		} else if (addr == AW89402_ADCSCTRL5) {
			aw89402->core_enabled = (data & SDM_PDIREF_MASK) == (0 << 2);
			data &= ~SDM_PDIREF_MASK;
			data |= 1 << 2;
		} else if (addr == AW89402_ADCSCTRL6) {
			data &= ~VCM_EN_MASK;
			data |= 0 << 1;
		}

		regmap_write(aw89402->regmap, addr, data);
	}

	regmap_read(aw89402->regmap, 0x6A, (u32 *)&data);

	regmap_write_bits(aw89402->regmap, AW89402_SYSCTRL2, SYSCTRL2_I2C_WEN_MASK, 0 << 6);

	if ((data & (((1 << 1) - 1) << 4)) != (1 << 4) && aw89402->pll_enabled) {
		ret = -EINVAL;
	}

	return ret;
}

static int aw89xxx_volume(struct aw89402_priv *aw89402, int channel, int vol)
{
	int ret = 0;
	u8 gain_table[] = {AW89402_ADC1CTRL2, AW89402_ADC2CTRL2,
		AW89402_ADC3CTRL2, AW89402_ADC4CTRL2};

	if (channel < 0 || channel > 3) {
		return -EINVAL;
	}

	if (vol < -6 || vol > 36) {
		return -EINVAL;
	}

	ret = regmap_write_bits(aw89402->regmap,
		gain_table[channel], PGA1_GAINSEL_MASK, (vol + 6));

	return ret;
}

static int aw89402_init_codec(struct aw89402_priv *aw89402)
{
	/* enable I2C Write */
	regmap_write_bits(aw89402->regmap, AW89402_SYSCTRL2, SYSCTRL2_I2C_WEN_MASK, 2 << 6);

	/* bias power up */
	regmap_write_bits(aw89402->regmap, AW89402_MISCCTRL2, BG_V2I_EN_MASK, 1 << 2);

	/* osc power up */
	regmap_write_bits(aw89402->regmap, 0x4D, BIT(7), 0 << 7);

	/* fast upe enable */
	regmap_write_bits(aw89402->regmap, AW89402_ADCSCTRL6, BG_V2I_EN_MASK, 0 << 2);

	/* vcm enable */
	regmap_write_bits(aw89402->regmap, AW89402_ADCSCTRL6, VCM_EN_MASK, 1 << 1);

	mdelay(2);

	/* vcm fastup disable */
	regmap_write_bits(aw89402->regmap, 0x3D, BG_V2I_EN_MASK, 1 << 2);

	/* ldo low power close */
	regmap_write_bits(aw89402->regmap, AW89402_MISCCTRL2, LDO_LP_EN_MASK, 0 << 3);

	/* mic bias power up */
	regmap_write_bits(aw89402->regmap, AW89402_MICBIAS12CTRL1, MICB12_PWD_MASK, 0 << 0);
	regmap_write_bits(aw89402->regmap, AW89402_MICBIAS34CTRL1, MICB34_PWD_MASK, 0 << 0);

	/* core power up */
	if (aw89402->core_enabled) {
		regmap_write_bits(aw89402->regmap, AW89402_ADCSCTRL5, BG_V2I_EN_MASK, 0 << 2);
		regmap_write(aw89402->regmap, AW89402_ANA_SYSCTRL, 0);
	}

	/* enable digital ch en */
	regmap_write_bits(aw89402->regmap, AW89402_SYSCTRL2, ALL_CH_MASK, 0x0F << 2);

	/* pll power up */
	if (aw89402->pll_enabled)
		regmap_write_bits(aw89402->regmap, AW89402_PLL_CTRL2, PLL_PD_MASK, 0 << 6);

	/* power up */
	regmap_write_bits(aw89402->regmap, AW89402_SYSCTRL1, PWDN_MASK, 0 << 0);

	mdelay(2);

	/* core clk enable */
	if (aw89402->core_enabled) {
		regmap_write(aw89402->regmap, 0x3E, 1);
		regmap_write(aw89402->regmap, 0x3F, 1);
	}

	mdelay(1);

	/* ch hmute disable */
	regmap_write_bits(aw89402->regmap, AW89402_CHCTRL1, CH_HMUTE_MASK, 0 << 5);
	regmap_write_bits(aw89402->regmap, AW89402_CHCTRL2, CH_HMUTE_MASK, 0 << 5);
	regmap_write_bits(aw89402->regmap, AW89402_CHCTRL3, CH_HMUTE_MASK, 0 << 5);
	regmap_write_bits(aw89402->regmap, AW89402_CHCTRL4, CH_HMUTE_MASK, 0 << 5);

	/* default volume */
	aw89xxx_volume(aw89402, 0, 23);
	aw89xxx_volume(aw89402, 1, 23);

	/* disable I2C Write */
	//regmap_write_bits(aw89402->regmap, AW89402_SYSCTRL2, SYSCTRL2_I2C_WEN_MASK, 0x0);

	return 0;
}

static int aw89402_deinit_codec(struct aw89402_priv *aw89402)
{
	/* enable I2C Write */
	regmap_write_bits(aw89402->regmap, AW89402_SYSCTRL2, SYSCTRL2_I2C_WEN_MASK, 2 << 6);

	/* hmute enable */
	regmap_write_bits(aw89402->regmap, AW89402_CHCTRL1, CH_HMUTE_MASK, 1 << 5);
	regmap_write_bits(aw89402->regmap, AW89402_CHCTRL2, CH_HMUTE_MASK, 1 << 5);
	regmap_write_bits(aw89402->regmap, AW89402_CHCTRL3, CH_HMUTE_MASK, 1 << 5);
	regmap_write_bits(aw89402->regmap, AW89402_CHCTRL4, CH_HMUTE_MASK, 1 << 5);

	/* mic bias power down */
	regmap_write_bits(aw89402->regmap, AW89402_MICBIAS12CTRL1, MICB12_PWD_MASK, 1 << 0);
	regmap_write_bits(aw89402->regmap, AW89402_MICBIAS34CTRL1, MICB34_PWD_MASK, 1 << 0);

	/* core power down */
	regmap_write_bits(aw89402->regmap, AW89402_ADCSCTRL5, BG_V2I_EN_MASK, 1 << 2);
	regmap_write(aw89402->regmap, AW89402_ANA_SYSCTRL, 0xFF);

	/* disable vcm */
	regmap_write_bits(aw89402->regmap, AW89402_ADCSCTRL6, VCM_EN_MASK, 0 << 1);

	/* core clk disable */
	regmap_write(aw89402->regmap, AW89402_ADCSCTRL7, 0);
	regmap_write(aw89402->regmap, AW89402_ADCSCTRL8, 0);

	/* osc power down */
	regmap_write_bits(aw89402->regmap, 0x4D, BIT(7), 1 << 7);

	/* bias power down */
	regmap_write_bits(aw89402->regmap, AW89402_MISCCTRL2, BG_V2I_EN_MASK, 0 << 2);

	/* pwd down enable */
	regmap_write_bits(aw89402->regmap, AW89402_SYSCTRL1, PWDN_MASK, 1 << 0);

	/* Pll power down enable */
	regmap_write_bits(aw89402->regmap, AW89402_PLL_CTRL2, PLL_PD_MASK, 1 << 6);

	/* disable ch en */
	regmap_write_bits(aw89402->regmap, AW89402_SYSCTRL2, ALL_CH_MASK, 0 << 2);

	/* enable ldo low power mode */
	regmap_write_bits(aw89402->regmap, AW89402_MISCCTRL2, LDO_LP_EN_MASK, 1 << 3);

	/* disable I2C Write */
	//regmap_write_bits(aw89402->regmap, 0x07, SYSCTRL2_I2C_WEN_MASK, 0 << 6);

	return 0;
}

static int aw89402_probe(struct snd_soc_component *c)
{
	//struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(c);

	printk("%s %d\n", __func__, __LINE__);
	//aw89402_init_codec(aw89402);


	return 0;
}

static void aw89402_remove(struct snd_soc_component *c)
{
	//struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(c);

	printk("%s %d\n", __func__, __LINE__);
	//aw89402_deinit_codec(aw89402);

}

static int aw89402_suspend(struct snd_soc_component *c)
{
	printk("%s %d\n", __func__, __LINE__);
	return 0;
}

static int aw89402_resume(struct snd_soc_component *c)
{
	printk("%s %d\n", __func__, __LINE__);
	return 0;
}

static int aw89402_set_bias_level(struct snd_soc_component *component,
	enum snd_soc_bias_level level)
{
	printk("%s %d\n", __func__, __LINE__);
	return 0;
}

static const DECLARE_TLV_DB_SCALE(pga_gain_tlv, -600, 100, 0);

static int aw89402_pga1_gainsel_set(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(component);

	regmap_write_bits(aw89402->regmap, AW89402_ADC1CTRL2, PGA1_GAINSEL_MASK,
		ucontrol->value.integer.value[0] & 0x3f);

	return 0;
}

static int aw89402_pga1_gainsel_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(component);
	u8 val;

	regmap_read(aw89402->regmap, AW89402_ADC1CTRL2, (u32 *)&val);
	ucontrol->value.integer.value[0] = val & 0x3f;

	return 0;
}

static int aw89402_pga2_gainsel_set(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(component);

	regmap_write_bits(aw89402->regmap, AW89402_ADC2CTRL2, PGA1_GAINSEL_MASK,
		ucontrol->value.integer.value[0] & 0x3f);

	return 0;
}

static int aw89402_pga2_gainsel_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(component);
	u8 val;

	regmap_read(aw89402->regmap, AW89402_ADC2CTRL2, (u32 *)&val);
	ucontrol->value.integer.value[0] = val & 0x3f;

	return 0;
}

static int aw89402_pga3_gainsel_set(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(component);

	regmap_write_bits(aw89402->regmap, AW89402_ADC3CTRL2, PGA1_GAINSEL_MASK,
		ucontrol->value.integer.value[0] & 0x3f);

	return 0;
}

static int aw89402_pga3_gainsel_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(component);
	u8 val;

	regmap_read(aw89402->regmap, AW89402_ADC3CTRL2, (u32 *)&val);
	ucontrol->value.integer.value[0] = val & 0x3f;

	return 0;
}

static int aw89402_pga4_gainsel_set(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(component);

	regmap_write_bits(aw89402->regmap, AW89402_ADC4CTRL2, PGA1_GAINSEL_MASK,
		ucontrol->value.integer.value[0] & 0x3f);

	return 0;
}

static int aw89402_pga4_gainsel_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct aw89402_priv *aw89402 = snd_soc_component_get_drvdata(component);
	u8 val;

	regmap_read(aw89402->regmap, AW89402_ADC4CTRL2, (u32 *)&val);
	ucontrol->value.integer.value[0] = val & 0x3f;

	return 0;
}

static const struct snd_kcontrol_new aw89402_snd_controls[] = {
	SOC_SINGLE_EXT_TLV("PGA1_GAINSEL", AW89402_ADC1CTRL2, 0, 0x2A, 0,
			aw89402_pga1_gainsel_get,
			aw89402_pga1_gainsel_set,
			pga_gain_tlv),
	SOC_SINGLE_EXT_TLV("PGA2_GAINSEL", AW89402_ADC2CTRL2, 0, 0x2A, 0,
			aw89402_pga2_gainsel_get,
			aw89402_pga2_gainsel_set,
			pga_gain_tlv),
	SOC_SINGLE_EXT_TLV("PGA3_GAINSEL", AW89402_ADC3CTRL2, 0, 0x2A, 0,
			aw89402_pga3_gainsel_get,
			aw89402_pga3_gainsel_set,
			pga_gain_tlv),
	SOC_SINGLE_EXT_TLV("PGA4_GAINSEL", AW89402_ADC4CTRL2, 0, 0x2A, 0,
			aw89402_pga4_gainsel_get,
			aw89402_pga4_gainsel_set,
			pga_gain_tlv),
};

static const struct snd_soc_dapm_widget aw89402_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("INPUT"),
	SND_SOC_DAPM_ADC("ADC", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("SDOUT", "I2S Capture", 0, SND_SOC_NOPM, 0, 0),
};

static const struct snd_soc_dapm_route aw89402_dapm_routes[] = {
	{"ADC", NULL, "INPUT"},
	{"SDOUT", NULL, "ADC"},
};

static struct snd_soc_component_driver soc_codec_dev_aw89402 = {
	.probe = aw89402_probe,
	.remove = aw89402_remove,
	.suspend = aw89402_suspend,
	.resume = aw89402_resume,
	.set_bias_level = aw89402_set_bias_level,
	.idle_bias_on = 1,

	.controls = aw89402_snd_controls,
	.num_controls = ARRAY_SIZE(aw89402_snd_controls),
	.dapm_widgets = aw89402_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(aw89402_dapm_widgets),
	.dapm_routes = aw89402_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(aw89402_dapm_routes),
};

static ssize_t reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct aw89402_priv *aw89402 = dev_get_drvdata(dev);
	unsigned int reg_val = 0;
	unsigned int i = 0;
	ssize_t len = 0;

	for (i = 0; i <= 0x6F; i++) {
		regmap_read(aw89402->regmap, i, &reg_val);
		len += snprintf(buf + len, PAGE_SIZE - len, "reg:0x%02x=0x%02x\n", i, reg_val);
	}

	return len;
}

static ssize_t reg_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t len)
{
	unsigned int databuf[2];
	struct aw89402_priv *aw89402 = dev_get_drvdata(dev);

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2)
		regmap_write(aw89402->regmap, databuf[0], databuf[1]);

	return len;
}
static DEVICE_ATTR_RW(reg);

/* add your attr in here*/
static struct attribute *aw89402_attributes[] = {
	&dev_attr_reg.attr,
	NULL
};

static struct attribute_group aw89402_attribute_group = {
	.attrs = aw89402_attributes
};


static int aw89402_check_id(struct aw89402_priv *aw89402)
{
	u8 id_expected[2] = {0x22, 0x16};
	u8 id[2] = {0};

	id[0] = i2c_smbus_read_byte_data(aw89402->i2c, AW89402_IDH);
	id[1] = i2c_smbus_read_byte_data(aw89402->i2c, AW89402_IDL);

	if (memcmp(id, id_expected, sizeof(u8) * 2)) {
		dev_err(&aw89402->i2c->dev, "chip id mismatch\n");
		return -ENODEV;
	}


	return 0;
}

static void aw89402_enable(struct aw89402_priv *aw89402, bool en)
{
	if (aw89402->enable_gpiod) {
		if (en) {
			gpiod_direction_output(aw89402->enable_gpiod, 1);
		} else {
			gpiod_direction_output(aw89402->enable_gpiod, 0);
		}
	}

	mdelay(1);
}

static int aw89402_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct aw89402_priv *aw89402 = NULL;
	struct device *dev = &client->dev;
	int ret;

	aw89402 = devm_kzalloc(dev, sizeof(*aw89402), GFP_KERNEL);
	if (!aw89402) {
		dev_err(&client->dev, "Failed to malloc data for aw89402\n");
		return -ENOMEM;
	}

	aw89402->enable_gpiod = devm_gpiod_get_optional(dev, "aw89402,enable", GPIOD_OUT_LOW);
	if (IS_ERR(aw89402->enable_gpiod)) {
		dev_warn(dev, "Failed to get enable gpio, disable force power up control\n");
	}

	aw89402->i2c = client;
	i2c_set_clientdata(client, aw89402);

	aw89402->regmap = devm_regmap_init_i2c(client, &aw89402_regmap_config);
	if (IS_ERR(aw89402->regmap)) {
		ret = PTR_ERR(aw89402->regmap);
		dev_err(dev, "Failed to allocate register map: %d\n", ret);
		return ret;
	}

	/* enable aw89402 */
	aw89402_enable(aw89402, true);
	ret = aw89402_check_id(aw89402);
	if (ret)
		return ret;

	/* soft rest */
	regmap_write_bits(aw89402->regmap, AW89402_SYSCTRL2, SYSCTRL2_I2C_WEN_MASK, 2 << 6);
	regmap_write(aw89402->regmap, AW89402_IDL, 0xAA);
	regmap_write_bits(aw89402->regmap, AW89402_SYSCTRL2, SYSCTRL2_I2C_WEN_MASK, 0 << 6);
	mdelay(2);
	aw89402_config(aw89402, &init_reg_tab[0], sizeof(init_reg_tab) / sizeof(reg_tab));
	aw89402_init_codec(aw89402);

	ret = devm_snd_soc_register_component(dev, &soc_codec_dev_aw89402, &aw89402_dai, 1);
	if (ret) {
		dev_err(dev, "Failed to register snd soc component, ret = %d\n", ret);
		return ret;
	}

	ret = devm_device_add_group(&client->dev, &aw89402_attribute_group);
	if (ret) {
		dev_err(dev, "Failed to add group attr for aw89402\n");
		return ret;
	}

	dev_info(dev, "aw89402 probe success\n");

	return 0;
}

static int aw89402_i2c_remove(struct i2c_client *client)
{
	struct aw89402_priv *aw89402 = i2c_get_clientdata(client);

	aw89402_deinit_codec(aw89402);
	aw89402_enable(aw89402, false);

	return 0;
}

static const struct of_device_id aw89402_dt_match[] = {
	{.compatible = "aw89402",},
	{},
};
MODULE_DEVICE_TABLE(of, aw89402_dt_match);

static const struct i2c_device_id aw89402_ids[] = {
	{"aw89402", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, aw89402_ids);

static struct i2c_driver aw89402_i2c_driver = {
	.probe = aw89402_i2c_probe,
	.remove = aw89402_i2c_remove,
	.driver = {
		.name = "aw89402_i2c_driver",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(aw89402_dt_match),
	},
	.id_table = aw89402_ids,
};
module_i2c_driver(aw89402_i2c_driver);

MODULE_AUTHOR("<zhiwen.liang@hollyland-tech.com>");
MODULE_DESCRIPTION("Driver for aw89402 adc");
MODULE_LICENSE("GPL and additional rights");

