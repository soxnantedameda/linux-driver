#include <linux/backlight.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/mutex.h>

#include "ssd2828.h"
#include "ssd2828_panel_config.h"

static uint16_t ssd2828_read_data(struct spi_device *spi)
{
	uint16_t data = 0;
	int err;

	if (spi->bits_per_word != 8) {
		spi->bits_per_word = 8;
		err = spi_setup(spi);
		if (err < 0) {
			dev_err(&spi->dev, "setup spi failed\n");
			return err;
		}
	}

	spi_read(spi, &data, 1);
	return data;
}

static uint16_t ssd2828_write_data(struct spi_device *spi, uint16_t data)
{
	int err = 0;

	if (spi->bits_per_word != 9) {
		spi->bits_per_word = 9;
		err = spi_setup(spi);
		if (err < 0) {
			dev_err(&spi->dev, "setup spi failed\n");
			return err;
		}
	}

	data |= 0x100;

	return spi_write(spi, (void *)&data, 2);
}

static uint16_t ssd2828_write_cmd(struct spi_device *spi, uint16_t data)
{
	int err;

	if (spi->bits_per_word != 9) {
		spi->bits_per_word = 9;
		err = spi_setup(spi);
		if (err < 0) {
			dev_err(&spi->dev, "setup spi failed\n");
			return err;
		}
	}

	return spi_write(spi, (void *)&data, 2);
}

static int ssd2828_write_reg(struct spi_device *spi, uint16_t cmd, uint16_t data_h, uint16_t data_l)
{
	int ret;

	ret = ssd2828_write_cmd(spi, cmd);
	ret = ssd2828_write_data(spi, data_l);
	ret = ssd2828_write_data(spi, data_h);

	return ret;
}

static uint16_t ssd2828_read_reg(struct spi_device *spi, uint16_t reg)
{
	uint16_t rd_data_h, rd_data_l;

	ssd2828_write_reg(spi, 0xd4, 0x00, 0xfa);

	ssd2828_write_cmd(spi, reg);
	ssd2828_write_cmd(spi, 0xfa);
	rd_data_l = ssd2828_read_data(spi);
	rd_data_h = ssd2828_read_data(spi) << 8;

	return rd_data_h | rd_data_l;
}

static void ssd2828_default_param(struct spi_device *spi)
{
	struct ssd2828_data *data = spi_get_drvdata(spi);

	data->IFMODE = MIPI_VIDEO_4LANE;
	data->VIDEO_MODE = NON_BURST_PULSE;
	data->BPP = 24;

	data->SOURCE_CLK = 24;
	data->PCLK = 148;
	data->LP_CLK = 10;

	data->XSIZE = 1200;
	data->HFPD = 15;
	data->HSPW = 8;
	data->HBPD = 15;
	data->YSIZE = 1920;
	data->VFPD = 33;
	data->VSPW = 12;
	data->VBPD = 32;

	data->PCLK_EDGE = FALLING_EDGE;
	data->HS_POLARITY = ACTIVE_HIGH;
	data->VS_POLARITY = ACTIVE_HIGH;
	data->DE_POLARITY = ACTIVE_HIGH;

}

static int ssd2828_config(struct spi_device *spi)
{
	struct ssd2828_data *data = spi_get_drvdata(spi);

	uint32_t video_pll_clk = 0;
	uint8_t pclk_p, hs_p, vs_p;
	uint8_t r_ba_FR, r_ba_MS = 4, r_ba_NS = 0;
	uint8_t r_bb_LPD;
	uint8_t lane_num = 4;
	uint8_t r_vpf;

	ssd2828_default_param(spi);

	if (ssd2828_read_reg(spi, 0xb0) != 0x2828) {
		dev_err(&spi->dev, "ssd2828 NG\n");
		return -ENXIO;
	}

	if (data->IFMODE == MIPI_VIDEO_1LANE) {
		video_pll_clk = data->PCLK * (data->BPP / 1);
		lane_num = 0;
	} else if (data->IFMODE == MIPI_VIDEO_2LANE) {
		video_pll_clk = data->PCLK * (data->BPP / 2);
		lane_num = 1;
	} else if (data->IFMODE == MIPI_VIDEO_3LANE) {
		video_pll_clk = data->PCLK * (data->BPP / 3);
		lane_num = 2;
	} else if((data->IFMODE == MIPI_VIDEO_4LANE) || (data->IFMODE == MIPI_VIDEO_8LANE)) {
		video_pll_clk = data->PCLK * (data->BPP / 4);
		lane_num = 3;
	}

	video_pll_clk = video_pll_clk + video_pll_clk / 50;
	if (video_pll_clk % (data->SOURCE_CLK / r_ba_MS)) {
		r_ba_NS = video_pll_clk / (data->SOURCE_CLK / r_ba_MS) + 1;
	} else {
		r_ba_NS = video_pll_clk / (data->SOURCE_CLK / r_ba_MS);
	}

	if (video_pll_clk <= 125) {
		r_ba_FR = 0x00;
	} else if (video_pll_clk <= 250) {
		r_ba_FR = 0x40;
	} else if (video_pll_clk <= 500) {
		r_ba_FR = 0x80;
	} else {
		r_ba_FR = 0xc0;
	}

	dev_info(&spi->dev, "video_pll_clk = %u, NS = %u, MS = %u\n",
		video_pll_clk, r_ba_NS, r_ba_MS);

	ssd2828_write_reg(spi, 0x00b9, 0x00, 0x00);

	ssd2828_write_reg(spi, 0x00b1, data->VSPW, data->HSPW);
	if ((data->VIDEO_MODE == NON_BURST_EVENT) || (data->VIDEO_MODE == BURST)) {
		ssd2828_write_reg(spi, 0x00b2, data->VBPD + data->VSPW, data->HBPD + data->HSPW);
		} else {
		ssd2828_write_reg(spi, 0x00b2, data->VBPD, data->HBPD);
	}
	ssd2828_write_reg(spi, 0x00b3, data->VFPD, data->HFPD);
	if ((data->IFMODE != MIPI_VIDEO_8LANE) && (data->IFMODE != MIPI_COMMAND_8LANE)) {
		ssd2828_write_reg(spi, 0xb4, (data->XSIZE >> 8) & 0xff, data->XSIZE & 0xff);
	} else {
		ssd2828_write_reg(spi, 0xb4, ((data->XSIZE / 2) >> 8) & 0xff, (data->XSIZE / 2) & 0xff);
	}
	ssd2828_write_reg(spi, 0xb5, (data->YSIZE >> 8) & 0xff, data->YSIZE & 0xff);

	if (data->PCLK_EDGE == FALLING_EDGE) {
		pclk_p = PCLK_P_FALLING;
	} else {
		pclk_p = PCLK_P_RISING;
	}

	if (data->HS_POLARITY == ACTIVE_HIGH) {
		hs_p = HS_P_HIGH;
	} else {
		hs_p = HS_P_LOW;
	}

	if (data->VS_POLARITY == ACTIVE_HIGH) {
		vs_p = VS_P_HIGH;
	} else {
		vs_p = VS_P_LOW;
	}

	if (data->BPP == 24) {
		r_vpf = 0x3;
	} else if (data->BPP == 18) {
		r_vpf = 0x2;
	} else if (data->BPP == 16) {
		r_vpf = 0x0;
	} else {
		r_vpf = 0x3;
	}
	// RGB Interface Control Register6
	ssd2828_write_reg(spi, 0x00b6, pclk_p | hs_p | vs_p, r_vpf | data->VIDEO_MODE | NVB_1);
	// PLL Configuration Register PLL= fIN X NS = xMHz
	ssd2828_write_reg(spi, 0x00ba, r_ba_FR | r_ba_MS, r_ba_NS);

	if (data->LP_CLK > 10) {
		data->LP_CLK = 10;
	}
	r_bb_LPD = data->SOURCE_CLK * r_ba_NS / (r_ba_MS * (8 * data->LP_CLK));
	r_bb_LPD -= 1;
	if (r_bb_LPD > 0x3F) {
		r_bb_LPD = 0x3F;
	}
	// 设置lp时钟分频 LP Mode CLK =PLL /(LPD+1)/8 = x MHz
	ssd2828_write_reg(spi, 0x00bb, 0x00, r_bb_LPD);
	//VC Control Register Description
	ssd2828_write_reg(spi, 0x00b8, 0x00, 0x00);

	ssd2828_write_reg(spi, 0x00c9, 0x23, 0x02);  //Delay Adjustment Register 1
	ssd2828_write_reg(spi, 0x00ca, 0x23, 0x01);  //Delay Adjustment Register 2
	ssd2828_write_reg(spi, 0x00cb, 0x05, 0x10);  //Delay Adjustment Register 3
	ssd2828_write_reg(spi, 0x00cc, 0x10, 0x05);  //Delay Adjustment Register 4

	//ssd2828_write_reg(0x00d0,0x00,0x00);

	ssd2828_write_reg(spi, 0x00de, 0x00, lane_num);  //Lane Configuration Register
	//ssd2828_write_reg(0x00d6, 0x00, 0x04);  //BGR
	ssd2828_write_reg(spi, 0x00d6, 0x00, 0x05);  //Test Register rgb
	ssd2828_write_reg(spi, 0x00c4, 0x00, 0x01);  //Line Control Register Automatically perform BTA after the next write packe
	ssd2828_write_reg(spi, 0x00eb, 0x80, 0x00);  //Video Sync Delay Register

	//EOT Packet Enable,ECC CRC Check Enable, DCS, Short packer, LP
	ssd2828_write_reg(spi, 0x00b7,0x02,0x50);

	mdelay(10);
	ssd2828_write_reg(spi, 0x00b9, 0x00, 0x01);  //PLL Control Register PLL Enable
	mdelay(120);
	//data = data_ReadReg(0xb9);

	return 0;
}

static void ssd2828_dcs_short_write(struct spi_device *spi, uint32_t len)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	if (ssd2828->speedmode == LP) {
		//EOT Packet Enable,ECC CRC Check Enable, DCS, Short packer, LP
		ssd2828_write_reg(spi, 0xb7, 0x02, 0x50);
	} else if (ssd2828->speedmode == HS) {
		//EOT Packet Enable,ECC CRC Check Enable, DCS, Short packer, HS
		ssd2828_write_reg(spi, 0x00b7, 0x02, (0x50 & 0xef) | 0x03);
	} else if (ssd2828->speedmode == VD) {
		//EOT Packet Enable,ECC CRC Check Disable, DCS, Short packer, HS Video
		ssd2828_write_reg(spi, 0x00b7, 0x02 | 0x01, (0x50 & 0xef) | 0x0b);
	}
	udelay(100);
	ssd2828_write_reg(spi, 0xbc, 0x00, len);

	//mdelay(50);
	//uint16_t data = SSD2828_ReadReg(0xbc);
	ssd2828_write_reg(spi, 0xbd, 0x00, 0x00);
	ssd2828_write_reg(spi, 0xbe, 0x00, len);

	ssd2828_write_cmd(spi, 0xbf);
}

static void ssd2828_dcs_long_write(struct spi_device *spi, uint32_t len)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	if (ssd2828->speedmode == LP) {
		//EOT Packet Enable,ECC CRC Check Enable, DCS Long Write, LP
		ssd2828_write_reg(spi, 0x00b7, 0x06, 0x50);
	} else if (ssd2828->speedmode == HS) {
		//EOT Packet Enable,ECC CRC Check Enable, DCS Long Write, HS
		ssd2828_write_reg(spi, 0x00b7, 0x06, (0x50 & 0xef) | 0x03);
	} else if (ssd2828->speedmode == VD) {
		//EOT Packet Enable,ECC CRC Check Disable, DCS Long Write, HS Video
		ssd2828_write_reg(spi, 0x00b7, 0x06 | 0x01, (0x50 & 0xef) | 0x0b);
	}
	udelay(100);
	ssd2828_write_reg(spi, 0xbc, (len) >> 8, (len));
	ssd2828_write_reg(spi, 0xbd, (len) >> 24, (len) >> 16);
	ssd2828_write_reg(spi, 0xbe, 0x0f, 0xff);
	ssd2828_write_cmd(spi, 0xbf);
}

static void ssd2828_generic_short_write(struct spi_device *spi, uint32_t len)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	if (ssd2828->speedmode == LP) {
		//Configuration Register EOT Packet Enable,ECC CRC Check Enable, Generic Short Write, LP
		ssd2828_write_reg(spi, 0x00b7, 0x02, 0x10);
	} else if (ssd2828->speedmode == HS) {
		//EOT Packet Enable,ECC CRC Check Enable, Generic Short Write, HS
		ssd2828_write_reg(spi, 0x00b7, 0x02, (0x10 & 0xef) | 0x03);
	} else if (ssd2828->speedmode == VD) {
		//EOT Packet Enable,ECC CRC Check Disable, Generic Short Write, HS Video
		ssd2828_write_reg(spi, 0x00b7, 0x02 | 0x01, (0x10 & 0xef) | 0x0b);
	}
	udelay(100);
	ssd2828_write_reg(spi, 0xbc, 0x00, len);   //Packet Size Control Register1
	ssd2828_write_reg(spi, 0xbd, 0x00, 0x00);  //Packet Size Control Register2
	ssd2828_write_reg(spi, 0xbe, 0x00, len);   //Packet Size Control Register3
	ssd2828_write_cmd(spi, 0xbf);  //通用数据包发送寄存器
}

static void ssd2828_generic_long_write(struct spi_device *spi, uint32_t len)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	if (ssd2828->speedmode == LP) {
		//EOT Packet Enable,ECC CRC Check Enable, Generic Long Write, LP;
		ssd2828_write_reg(spi, 0x00b7, 0x06, 0x10);
	} else if (ssd2828->speedmode == HS) {
		//EOT Packet Enable,ECC CRC Check Enable, Generic Long Write, HS;
		ssd2828_write_reg(spi, 0x00b7, 0x06, (0x10 & 0xef) | 0x03);
	} else if (ssd2828->speedmode == VD) {
		//EOT Packet Enable,ECC CRC Check Disable, Generic Long Write, HS Video
		ssd2828_write_reg(spi, 0x00b7, 0x06 | 0x01, (0x10 & 0xef) | 0x0b);
	}
	udelay(100);
	ssd2828_write_reg(spi, 0xbc, (len) >> 8, (len));
	ssd2828_write_reg(spi, 0xbd, (len) >> 24, (len) >> 16);
	ssd2828_write_reg(spi, 0xbe, 0x0f, 0xff);

	ssd2828_write_cmd(spi, 0xbf);
}

void ssd2828_mipi_write(struct spi_device *spi, uint8_t DT, int data, ...)
{
	va_list param_list;
	int in_data;
	uint32_t len = 0, i;
	uint16_t pack_data[256];

	va_start(param_list, data);
	pack_data[len++] = data;
	while (1) {
		in_data = va_arg(param_list, int);
		if (in_data == -1)
			break;
		pack_data[len++] = in_data;
	}

	if (len > 256)
		return;

	switch (DT) {
		case 0x39:
			ssd2828_dcs_long_write(spi, len);
			break;
		case 0x29:
			ssd2828_generic_long_write(spi, len);
			break;
		case 0x05:
		case 0x15:
			ssd2828_dcs_short_write(spi, len);
			break;
		case 0x03:
		case 0x13:
		case 0x23:
			ssd2828_generic_short_write(spi, len);
			break;
	}

	for (i = 0; i < len; i++) {
		ssd2828_write_data(spi, pack_data[i]);
	}

	va_end(param_list);
}

uint32_t ssd2828_dcs_read(struct spi_device *spi, uint8_t reg, uint16_t len, uint8_t *p)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);
	uint16_t state;
	uint32_t i;
	uint32_t timeout_cnt = 0;

	do {
		if (ssd2828->speedmode == LP) {
			ssd2828_write_reg(spi, 0x00b7, 0x03, 0xc2);  //LP DCS read
		} else if (ssd2828->speedmode == HS) {
			ssd2828_write_reg(spi, 0x00b7, 0x03, (0xc2 & 0XEF) | 0X03);
		} else if (ssd2828->speedmode == VD) {
			ssd2828_write_reg(spi, 0x00b7, 0x03, (0xc2 & 0XEF) | 0X0B);
		}

		//ssd2828_write_reg(0x00bb,0x00,8);			//PL clock
		ssd2828_write_reg(spi, 0x00c1, len >> 8, len); //Maximum Return Size
		ssd2828_write_reg(spi, 0x00c0, 0x00, 0x01);	//Operation Control Register   Cancel the current operation
		ssd2828_write_reg(spi, 0x00bc, 0x00, 0x01);	//Packet Size Control Register1
		ssd2828_write_reg(spi, 0x00bf, 0x00, reg);	//Generic Packet Drop Register 发送寄存器
		mdelay(10);
		state = ssd2828_read_reg(spi, 0xc6); //Generic Packet Drop Register中断状态寄存器

		if (state & 0x01) {
			break;
		} else if (++timeout_cnt > 10) {
			return MIPI_READ_FAIL;
		}
	} while (1);

	//Read Register
	ssd2828_write_cmd(spi, 0xff);
	for (i = 0; i < len;) {
		ssd2828_write_cmd(spi, 0xfa);
		*p++ = ssd2828_read_data(spi);
		if (++i >= len) {
			ssd2828_read_data(spi);
			break;
		}
		*p++ = ssd2828_read_data(spi);
		++i;
	}

	return MIPI_READ_SUCCEED;
}

uint32_t ssd2828_generic_read(struct spi_device *spi, uint8_t reg, uint16_t len, uint8_t *p)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);
	uint16_t state;
	uint32_t i;
	uint32_t timeout_cnt = 0;

	do {
		if (ssd2828->speedmode == LP) {
			ssd2828_write_reg(spi, 0x00b7, 0x03, 0x82);  //LP generic read
		} else if (ssd2828->speedmode == HS) {
			ssd2828_write_reg(spi, 0x00b7, 0x03, (0x82 & 0XEF) | 0X03);
		} else if (ssd2828->speedmode == VD) {
			ssd2828_write_reg(spi, 0x00b7, 0x03, (0x82 & 0XEF) | 0X0B);
		}

		//ssd2828_write_reg(0x00bb,0x00,8);
		ssd2828_write_reg(spi, 0x00c1, len >> 8, len);
		ssd2828_write_reg(spi, 0x00c0, 0x00, 0x01);
		ssd2828_write_reg(spi, 0x00bc, 0x00, 1);
		ssd2828_write_reg(spi, 0x00bf, 0x00, reg);
		mdelay(10);
		state = ssd2828_read_reg(spi, 0xc6);

		if (state & 0x01) {
			break;
		} else if (++timeout_cnt > 10) {
			return MIPI_READ_FAIL;
		}
	} while (1);

	ssd2828_write_cmd(spi, 0xff);
	for (i = 0; i < len;) {
		ssd2828_write_cmd(spi, 0xfa);
		*p++ = ssd2828_read_data(spi);
		if (++i >= len) {
			ssd2828_read_data(spi);
			break;
		}
		*p++ = ssd2828_read_data(spi);
		++i;
	}

	return MIPI_READ_SUCCEED;
}

static void ssd2828_lp(struct spi_device *spi)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	ssd2828->speedmode = LP;
}

static void ssd2828_video(struct spi_device *spi)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	ssd2828->speedmode = VD;
	ssd2828_write_reg(spi, 0x00b7, 0x03, 0x0b);
}

/*
static void ssd2828_hs(struct spi_device *spi)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	ssd2828->speedmode = HS;
}

static void ssd2828_pattern_color_bar(struct spi_device *spi)
{
	uint8_t color;

	color = ssd2828_read_reg(spi, 0xee);  //SSD2828 PLL OFF
	//printk("old color bar 0xee = 0x%x\n\r",color);
	color |= 0x600;
	ssd2828_write_reg(spi, 0xee, (color >> 8) & 0xff, color & 0xff);  //SSD2828 PLL OFF

	color = ssd2828_read_reg(spi, 0xee);  //SSD2828 PLL OFF
	//uint16_t data = ssd2828_read_reg(0xee);
	//printk("new color bar 0xee = 0x%x\n\r",color);
}
*/

static void mipi_dsi_panel_detection(struct spi_device *spi)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);
	struct panel_detect_params *params = panel_list;
	int i, panel_cnt;
	uint8_t ID[PANEL_ID_MAX_COMMANDS] = {0};

	panel_cnt = get_panel_list_size();
	for (i = 0; i < panel_cnt; i++) {
		uint32_t retry_cnt = 0;
		uint32_t status[PANEL_ID_MAX_COMMANDS] = {0};

		do {
			memset(ID, 0, sizeof(unsigned char) * PANEL_ID_MAX_COMMANDS);
			status[0] = ssd2828_dcs_read(spi, params[i].address[0], 1, &ID[0]);
			status[1] = ssd2828_dcs_read(spi, params[i].address[1], 1, &ID[1]);
			status[2] = ssd2828_dcs_read(spi, params[i].address[2], 1, &ID[2]);
			retry_cnt++;
		} while (retry_cnt < params[i].retry_cnt &&
			(status[0] != MIPI_READ_SUCCEED) &&
			(status[1] != MIPI_READ_SUCCEED) &&
			(status[2] != MIPI_READ_SUCCEED));
		if (retry_cnt < params[i].retry_cnt) {
			if (!memcmp(params[i].expected, ID, sizeof(unsigned char) * PANEL_ID_MAX_COMMANDS)) {
				ssd2828->mipi_send_initcode_cb = params[i].mipi_send_initcode;
				break;
			}
		}
		if (i == panel_cnt - 1) {
			dev_err(&spi->dev, "not found special panel, use default\n");
			ssd2828->mipi_send_initcode_cb = params[DEFAULT_PANEL].mipi_send_initcode;
		}
	}
}

static int ssd2828_display_prepare(struct spi_device *spi)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	if (ssd2828->prepared)
		return 0;

	/* ssd2828 prepare */
	if (ssd2828->ssd2828_enable_gpio) {
		gpiod_set_value_cansleep(ssd2828->ssd2828_enable_gpio, 1);
		mdelay(10);
	}

	if (ssd2828->ssd2828_reset_gpio) {
		gpiod_set_value_cansleep(ssd2828->ssd2828_reset_gpio, 1);
		mdelay(10);
		gpiod_set_value_cansleep(ssd2828->ssd2828_reset_gpio, 0);
		mdelay(10);
		gpiod_set_value_cansleep(ssd2828->ssd2828_reset_gpio, 1);
		mdelay(10);
	}

	if (ssd2828_config(spi)) {
		return -EINVAL;
	}

	/* panel prepare */
	if (ssd2828->panel_enable_gpio) {
		gpiod_set_value_cansleep(ssd2828->panel_enable_gpio, 1);
		mdelay(10);
	}

	if (ssd2828->panel_reset_gpio) {
		gpiod_set_value_cansleep(ssd2828->panel_reset_gpio, 1);
		mdelay(ssd2828->delay.reset);
		gpiod_set_value_cansleep(ssd2828->panel_reset_gpio, 0);
		mdelay(ssd2828->delay.reset);
		gpiod_set_value_cansleep(ssd2828->panel_reset_gpio, 1);
		mdelay(ssd2828->delay.reset);
	}

	if (ssd2828->delay.prepare)
		mdelay(ssd2828->delay.prepare);
	/* send initcode */
	ssd2828_lp(spi);
	mipi_dsi_panel_detection(spi);
	if (ssd2828->mipi_send_initcode_cb)
		ssd2828->mipi_send_initcode_cb(spi);
	ssd2828_video(spi);

	ssd2828->prepared = true;

	return 0;
}

static int ssd2828_display_unprepare(struct spi_device *spi)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	if (!ssd2828->prepared)
		return 0;

	if (ssd2828->delay.unprepare)
		mdelay(ssd2828->delay.unprepare);

	if (ssd2828->panel_reset_gpio) {
		gpiod_set_value_cansleep(ssd2828->panel_reset_gpio, 0);
		mdelay(ssd2828->delay.reset);
	}

	if (ssd2828->panel_enable_gpio) {
		gpiod_set_value_cansleep(ssd2828->panel_enable_gpio, 0);
	}

	if (ssd2828->ssd2828_reset_gpio) {
		gpiod_set_value_cansleep(ssd2828->ssd2828_reset_gpio, 0);
		mdelay(10);
	}

	if (ssd2828->ssd2828_enable_gpio) {
		gpiod_set_value_cansleep(ssd2828->ssd2828_enable_gpio, 0);
	}

	ssd2828->prepared = false;

	return 0;
}

static int ssd2828_display_enable(struct spi_device *spi)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	if (ssd2828->enabled)
		return 0;

	if (ssd2828->delay.enable)
		mdelay(ssd2828->delay.enable);

	if (ssd2828->backlight) {
		backlight_enable(ssd2828->backlight);
	}

	ssd2828->enabled = true;

	return 0;
}

static int ssd2828_display_disable(struct spi_device *spi)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	if (!ssd2828->enabled)
		return 0;

	if (ssd2828->backlight) {
		backlight_disable(ssd2828->backlight);
	}

	if (ssd2828->delay.disable)
		mdelay(ssd2828->delay.disable);

	ssd2828->enabled = false;

	return 0;
}

static void ssd2828_enter_sleep(struct spi_device *spi)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	mutex_lock(&ssd2828->bus_lock);
	ssd2828_display_disable(spi);

	if (!ssd2828->sleeped) {
		ssd2828_lp(spi);
		ssd2828_mipi_write(spi, 0x05, 0x28);
		mdelay(20);
		ssd2828_mipi_write(spi, 0x05, 0x10);
		mdelay(120);
		ssd2828_write_reg(spi, 0xb7, 0x03, 0x00);  //SSD2828 VIDEO MODE OFF
		ssd2828_write_reg(spi, 0xb7, 0x03, 0x04);  //SSD2828 ENTERS ULP MODE
		ssd2828_write_reg(spi, 0xb9, 0x00, 0x00);  //SSD2828 PLL OFF
		mdelay(10);
		ssd2828->sleeped = true;
	}
	mutex_unlock(&ssd2828->bus_lock);
}

static void ssd2828_exit_sleep(struct spi_device *spi)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	mutex_lock(&ssd2828->bus_lock);

	if (ssd2828->sleeped) {
		ssd2828_write_reg(spi, 0xb9, 0x00, 0x01);  //SSD2828 PLL ON
		ssd2828_write_reg(spi, 0xb7, 0x03, 0x00);  //SSD2828 LEAVES ULP MODE
		ssd2828_mipi_write(spi, 0x03, 0x11);
		mdelay(120);
		ssd2828_mipi_write(spi, 0x03, 0x29);
		mdelay(20);
		ssd2828_write_reg(spi, 0xb7, 0x03, 0x09);  //SSD2828 VIDEO MODE ON
		ssd2828_video(spi);
		ssd2828->sleeped = false;
	}
	ssd2828_display_enable(spi);

	mutex_unlock(&ssd2828->bus_lock);
}

static ssize_t ssd2828_sleep_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	return sprintf(buf, "%u\n", ssd2828->enabled);
}

static ssize_t ssd2828_sleep_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	int rc;
	bool sleep;

	rc = kstrtobool(buf, &sleep);
	if (rc)
		return rc;

	if (sleep)
		ssd2828_enter_sleep(spi);
	else
		ssd2828_exit_sleep(spi);

	return count;
}
static DEVICE_ATTR_RW(ssd2828_sleep);

static void ssd2828_display_on(struct spi_device * spi)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	mutex_lock(&ssd2828->bus_lock);
	ssd2828_display_prepare(spi);
	ssd2828_display_enable(spi);
	mutex_unlock(&ssd2828->bus_lock);
}

static void ssd2828_display_off(struct spi_device * spi)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	mutex_lock(&ssd2828->bus_lock);
	ssd2828_display_disable(spi);
	ssd2828_display_unprepare(spi);
	mutex_unlock(&ssd2828->bus_lock);
}

static ssize_t ssd2828_display_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);

	return sprintf(buf, "%u\n", ssd2828->prepared & ssd2828->enabled);
}

static ssize_t ssd2828_display_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	int rc;
	bool on;

	rc = kstrtobool(buf, &on);
	if (rc)
		return rc;

	if (on)
		ssd2828_display_on(spi);
	else
		ssd2828_display_off(spi);

	return count;
}
static DEVICE_ATTR_RW(ssd2828_display);

static ssize_t ssd2828_dump_reg_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);
	uint16_t reg_addr_start = 0xb0;
	uint16_t reg_addr_end = 0xf7;
	uint16_t reg_addr = 0, reg_value = 0;
	size_t count = 0;

	mutex_lock(&ssd2828->bus_lock);

	if (ssd2828->prepared) {
		for (reg_addr = reg_addr_start; reg_addr <= reg_addr_end; reg_addr++) {
			reg_value = ssd2828_read_reg(spi, reg_addr);
			dev_info(dev, "0x%2x : 0x%4x\n", reg_addr, reg_value);
		}
	} else {
		dev_info(dev, "ssd2828 is not prepared yet!\n");
	}
	mutex_unlock(&ssd2828->bus_lock);

	return count;
}
DEVICE_ATTR_RO(ssd2828_dump_reg);

static ssize_t ssd2828_write_reg_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);
	uint16_t reg_addr = 0, reg_value = 0, old_reg_value, new_reg_value;

	mutex_lock(&ssd2828->bus_lock);

	if (ssd2828->prepared) {
		sscanf(buf, "%hx %hx", &reg_addr, &reg_value);
		old_reg_value = ssd2828_read_reg(spi, reg_addr);
		dev_info(dev, "0x%2x old value = 0x%4x\n", reg_addr, old_reg_value);
		ssd2828_write_reg(spi, reg_addr, (reg_value >> 8) & 0xff, reg_value & 0xff);
		new_reg_value = ssd2828_read_reg(spi, reg_addr);
		dev_info(dev, "0x%2x new value = 0x%4x\n", reg_addr, new_reg_value);
	}

	mutex_unlock(&ssd2828->bus_lock);

	return count;
}
DEVICE_ATTR_WO(ssd2828_write_reg);

/* add your attr in here*/
static struct attribute *ssd2828_attributes[] = {
    &dev_attr_ssd2828_display.attr,
    &dev_attr_ssd2828_sleep.attr,
    &dev_attr_ssd2828_dump_reg.attr,
    &dev_attr_ssd2828_write_reg.attr,
    NULL
};

static struct attribute_group ssd2828_attribute_group = {
    .attrs = ssd2828_attributes
};

static int ssd2828_parse_dt(struct spi_device *spi)
{
	struct ssd2828_data *ssd2828 = spi_get_drvdata(spi);
	int err = 0;

	ssd2828->ssd2828_enable_gpio = devm_gpiod_get_optional(&spi->dev, "ssd2828-enable",
							 GPIOD_OUT_LOW);
	if (IS_ERR(ssd2828->ssd2828_enable_gpio)) {
		err = PTR_ERR(ssd2828->ssd2828_enable_gpio);
		if (err != -EPROBE_DEFER)
			dev_err(&spi->dev, "failed to request GPIO: %d\n", err);
	}

	ssd2828->ssd2828_reset_gpio = devm_gpiod_get_optional(&spi->dev, "ssd2828-reset",
							 GPIOD_OUT_HIGH);
	if (IS_ERR(ssd2828->ssd2828_reset_gpio)) {
		err = PTR_ERR(ssd2828->ssd2828_reset_gpio);
		if (err != -EPROBE_DEFER)
			dev_err(&spi->dev, "failed to request GPIO: %d\n", err);
		return err;
	}

	ssd2828->panel_enable_gpio = devm_gpiod_get_optional(&spi->dev, "panel-enable",
							 GPIOD_OUT_LOW);
	if (IS_ERR(ssd2828->panel_enable_gpio)) {
		err = PTR_ERR(ssd2828->panel_enable_gpio);
		if (err != -EPROBE_DEFER)
			dev_err(&spi->dev, "failed to request GPIO: %d\n", err);
	}

	ssd2828->panel_reset_gpio = devm_gpiod_get_optional(&spi->dev, "panel-reset",
							 GPIOD_OUT_HIGH);
	if (IS_ERR(ssd2828->panel_reset_gpio)) {
		err = PTR_ERR(ssd2828->panel_reset_gpio);
		if (err != -EPROBE_DEFER)
			dev_err(&spi->dev, "failed to request GPIO: %d\n", err);
		return err;
	}

	err = of_property_read_u32_array(spi->dev.of_node, "panel_delay", (u32*)&ssd2828->delay, 5);
	if (err) {
		dev_err(&spi->dev, "failed to get panel delay\n");
	}

	ssd2828->backlight = devm_of_find_backlight(&spi->dev);
	if (!ssd2828->backlight)
		dev_err(&spi->dev, "failed to request backlight: %d\n", err);

	return 0;
}

static int ssd2828_driver_probe(struct spi_device *spi)
{
	struct ssd2828_data *data;
	int err = 0;

	spi->mode = SPI_MODE_0;
	err = spi_setup(spi);
	if (err < 0) {
		dev_err(&spi->dev, "spi setup failed.\n");
		return err;
	}

	data = devm_kzalloc(&spi->dev, sizeof(struct ssd2828_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	data->enabled = false;
	data->prepared = false;
	spi_set_drvdata(spi, data);

	err = ssd2828_parse_dt(spi);
	if (err) {
		dev_err(&spi->dev, "ssd2828 parse dts failed\n");
		return -EINVAL;
	}

	err = devm_device_add_group(&spi->dev, &ssd2828_attribute_group);
	if (err) {
		dev_err(&spi->dev, "failed to add group for ssd2828!\n");
		return err;
	}

	/* default on */
	mutex_init(&data->bus_lock);
	ssd2828_display_on(spi);

	return 0;
}

static int ssd2828_driver_remove(struct spi_device *spi)
{
	struct ssd2828_data *data = spi_get_drvdata(spi);

	ssd2828_display_off(spi);

	if (data->backlight)
		backlight_put(data->backlight);

	devm_device_remove_group(&spi->dev, &ssd2828_attribute_group);

	return 0;
}

static void ssd2828_driver_shutdown(struct spi_device *spi)
{
	ssd2828_display_off(spi);
}

#ifdef CONFIG_PM_SLEEP
static int ssd2828_spi_suspend(struct device *dev)
{
	struct spi_device *spi = to_spi_device(dev);

	ssd2828_display_off(spi);

	return 0;
}

static int ssd2828_spi_resume(struct device *dev)
{
	struct spi_device *spi = to_spi_device(dev);

	ssd2828_display_on(spi);

	return 0;
}
#endif

static const struct dev_pm_ops ssd2828_spi_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(ssd2828_spi_suspend, ssd2828_spi_resume)
};

static const struct of_device_id ssd2828_of_match[] = {
	{
		.compatible = "ssd2828qn4",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, ssd2828_of_match);

static struct spi_driver ssd2828_spi_driver = {
	.driver = {
		.name = "ssd2828_spi_driver",
		.of_match_table = ssd2828_of_match,
		.pm = &ssd2828_spi_pm,
	},
	.probe = ssd2828_driver_probe,
	.remove = ssd2828_driver_remove,
	.shutdown = ssd2828_driver_shutdown,
};
module_spi_driver(ssd2828_spi_driver);

MODULE_AUTHOR("<897420073@qq.com>");
MODULE_DESCRIPTION("Driver for ssd2828");
MODULE_LICENSE("GPL and additional rights");

