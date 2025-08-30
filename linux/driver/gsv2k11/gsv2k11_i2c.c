#include <linux/gpio/consumer.h>
#include <linux/of_gpio.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/workqueue.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/notifier.h>
#include <linux/gsv2k11_notifier.h>

#include "kapi/kapi.h"  /* this file includes kernal APIs */
#include "av_uart_cmd.h" /* accept command */
#include "av_key_cmd.h" /* accept key */
#include "av_irda_cmd.h" /* accept ir */
#include "av_edid_manage.h" /* edid manage */
#include "av_event_handler.h" /* routing and event */

#include "global_var.h"

#include "av_user_config_input.h"

#if GSV1K
#include "uapi/gsv1k_device.h"
#endif
#if GSV2K0
#include "uapi/gsv2k0_device.h"
#endif
#if GSV2K11
#include "uapi/gsv2k11_device.h"
#endif

static BLOCKING_NOTIFIER_HEAD(gsv2k11_notifier_head);

#define BusConfig 16

extern uint8 EdidHdmi2p0;
extern uint8 LogicOutputSel;

static struct i2c_client *g_i2c_client = NULL;

struct gsv2k11_data {
	struct i2c_client *client;

	struct gpio_desc *reset_gpiod;
	struct gpio_desc *mute_gpiod;

	struct workqueue_struct *gsv2k11_wq;
	struct delayed_work gsv2k11_delayed_work;

	struct timer_list gsv2k11_timer;

	AvDevice devices[1];
	Gsv2k11Device gsv2k11_0;
	AvPort gsv2k11Ports[9];

	uint8 cur_vic;

	bool debug;
};

static AvRet gsv2k11_I2cRead(uint32 devAddress, uint32 regAddress, uint8 *data, uint16 count)
{
	AvRet ret = AvOk;
	uint8 *rd_buf = NULL;
	uint8 addr_buf[2] = {0};
	struct i2c_msg msgs[2] = {
		{
			.addr	= g_i2c_client->addr,
			.flags	= 0,
			.len	= 2,
			.buf	= addr_buf,
		},
		{
			.addr	= g_i2c_client->addr,
			.flags	= I2C_M_RD,
			.len	= count,
		},
	};

	regAddress = (uint32)((AvGetRegAddress(devAddress) << 8) | AvGetRegAddress(regAddress));
	addr_buf[0] = (regAddress >> 8) & 0xff;
	addr_buf[1] = regAddress & 0xff;

	rd_buf = kmalloc(count, GFP_KERNEL);
	if (rd_buf == NULL)
		return  -ENOMEM;

	msgs[1].buf = rd_buf;

	ret = i2c_transfer(g_i2c_client->adapter, msgs, 2);
	if (ret < 0) {
		dev_err(&g_i2c_client->dev, "i2c read error: %d\n", ret);
		ret = AvError;
	} else
		ret = AvOk;

	if (data != NULL)
		memcpy(data, rd_buf, count);
	kfree(rd_buf);

	return ret;
}

static AvRet gsv2k11_I2cWrite(uint32 devAddress, uint32 regAddress, uint8 *data, uint16 count)
{
	AvRet ret = AvOk;
	uint8 *wr_buf = NULL;
	wr_buf = kmalloc(count + 2, GFP_KERNEL);
	if (wr_buf == NULL)
		return -ENOMEM;

	regAddress = (uint32)((AvGetRegAddress(devAddress) << 8) | AvGetRegAddress(regAddress));
	wr_buf[0] = (regAddress >> 8) & 0xff;
	wr_buf[1] = regAddress & 0xff;
	memcpy(&wr_buf[2], data, count);

	ret = i2c_master_send(g_i2c_client, wr_buf, count + 2);
	if (ret < 0) {
		dev_err(&g_i2c_client->dev, "i2c master send error, ret = %d\n", ret);
		ret = AvError;
	} else
		ret = AvOk;
	kfree(wr_buf);

	return ret;
}

static AvRet gsv2k11_GetMilliSecond(uint32 *ms)
{
	AvRet ret = AvOk;

	*ms = ktime_to_ms(ktime_get()) ;

	return ret;
}
/* 1: mute, 0: unmute */
static void  gsv2k11_mute(struct i2c_client *client, bool mute)
{
	struct gsv2k11_data *gsv2k11 = i2c_get_clientdata(client);

	gpiod_set_value_cansleep(gsv2k11->mute_gpiod, !!mute);
}

static void gsv2k11_reset(struct i2c_client *client)
{
	struct gsv2k11_data *gsv2k11 = i2c_get_clientdata(client);

	/* Reset gsv2k11 chip */
	gpiod_set_value_cansleep(gsv2k11->reset_gpiod, 1);
	mdelay(10);
	gpiod_set_value_cansleep(gsv2k11->reset_gpiod, 0);
	mdelay(10);
	gpiod_set_value_cansleep(gsv2k11->reset_gpiod, 1);
	mdelay(10);
}

static void gsv2k11_work(struct work_struct *work)
{
	struct gsv2k11_data *gsv2k11 = container_of(to_delayed_work(work),
		struct gsv2k11_data, gsv2k11_delayed_work);
	AvPort *port = gsv2k11->devices[0].port;
	uint8 NewVic = 0x61;
	uint16 PixelFreq = 0;
	uint8 CommonBusConfig = BusConfig;

	AvApiUpdate();
	AvPortConnectUpdate(&gsv2k11->devices[0]);
	/* 4.1 switch Vic based on frequency */
	if((LogicOutputSel == 0) && (gsv2k11->gsv2k11Ports[7].content.lvrx->Lock == 1)) {
		PixelFreq = gsv2k11->gsv2k11Ports[7].content.video->info.TmdsFreq;
		if((ParallelConfigTable[CommonBusConfig*3 + 2] & 0x40) == 0x40) {
			if((ParallelConfigTable[CommonBusConfig*3 + 2] & 0x01) == 0x00)
				PixelFreq = PixelFreq / 2;
		} else {
			if((ParallelConfigTable[CommonBusConfig*3 + 2] & 0x01) == 0x00)
				PixelFreq = PixelFreq * 2;
		}
		if(PixelFreq > 590)
			NewVic = 0x61;
		else if(PixelFreq > 290)
			NewVic = 0x5F;
		else if(PixelFreq > 145)
			NewVic = 0x10;
		else if(PixelFreq > 70)
			NewVic = 0x04;
		else
			NewVic = 0x02;
		if(NewVic != gsv2k11->gsv2k11Ports[7].content.video->timing.Vic) {
			gsv2k11->gsv2k11Ports[7].content.video->timing.Vic = NewVic;
			gsv2k11->gsv2k11Ports[7].content.lvrx->Update = 1;
		}
	}

	if (gsv2k11->cur_vic != port->content.video->timing.Vic) {
		gsv2k11_mute(gsv2k11->client, 1);
		gsv2k11->cur_vic = port->content.video->timing.Vic;
		blocking_notifier_call_chain(&gsv2k11_notifier_head, gsv2k11->cur_vic, NULL);
		dev_info(&gsv2k11->client->dev, "Vic = %d\n", gsv2k11->cur_vic);

		if (port->content.video->timing.Vic != 0) {
			gsv2k11_mute(gsv2k11->client, 0);

			if (gsv2k11->debug) {
				dev_info(&gsv2k11->client->dev, "Vic = %d\n", port->content.video->timing.Vic);
				dev_info(&gsv2k11->client->dev, "HPolarity = %d\n", port->content.video->timing.HPolarity);
				dev_info(&gsv2k11->client->dev, "VPolarity = %d\n", port->content.video->timing.VPolarity);
				dev_info(&gsv2k11->client->dev, "Interlaced = %d\n", port->content.video->timing.Interlaced);
				dev_info(&gsv2k11->client->dev, "HActive = %d\n", port->content.video->timing.HActive);
				dev_info(&gsv2k11->client->dev, "VActive = %d\n", port->content.video->timing.VActive);
				dev_info(&gsv2k11->client->dev, "HTotal = %d\n", port->content.video->timing.HTotal);
				dev_info(&gsv2k11->client->dev, "VTotal = %d\n", port->content.video->timing.VTotal);
				dev_info(&gsv2k11->client->dev, "FrameRate = %d\n", port->content.video->timing.FrameRate);
				dev_info(&gsv2k11->client->dev, "VSync = %d\n", port->content.video->timing.VSync);
				dev_info(&gsv2k11->client->dev, "VSync = %d\n", port->content.video->timing.VBack);
				dev_info(&gsv2k11->client->dev, "HSync = %d\n", port->content.video->timing.HSync);
				dev_info(&gsv2k11->client->dev, "HBack = %d\n", port->content.video->timing.HBack);
			}
		} else {
			/* mute */
			gsv2k11_mute(gsv2k11->client, 1);
		}
	}

	mod_timer(&gsv2k11->gsv2k11_timer, jiffies + msecs_to_jiffies(500));
}

static void gsv2k11_timer_handler(struct timer_list *timer)
{
	struct gsv2k11_data *gsv2k11 = container_of(timer, struct gsv2k11_data, gsv2k11_timer);

	queue_delayed_work(gsv2k11->gsv2k11_wq,
		&gsv2k11->gsv2k11_delayed_work, msecs_to_jiffies(0));
}

static ssize_t mute_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct gsv2k11_data *gsv2k11 = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", gpiod_get_value_cansleep(gsv2k11->mute_gpiod));
}

static ssize_t mute_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	bool mute;
	int rc;

	rc = kstrtobool(buf, &mute);
	if (rc)
		return rc;

	gsv2k11_mute(client, mute);

	return count;
}
static DEVICE_ATTR_RW(mute);

static ssize_t debug_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct gsv2k11_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", data->debug);
}

static ssize_t debug_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct gsv2k11_data *data = i2c_get_clientdata(client);
	bool debug;
	int rc;

	rc = kstrtobool(buf, &debug);
	if (rc)
		return rc;

	data->debug = debug;

	return count;
}
static DEVICE_ATTR_RW(debug);

/* add your attr in here*/
static struct attribute *gsv2k11_attributes[] = {
	&dev_attr_mute.attr,
	&dev_attr_debug.attr,
	NULL
};

static struct attribute_group gsv2k11_attribute_group = {
	.attrs = gsv2k11_attributes
};

static int gsv2k11_i2c_check(struct i2c_client *client)
{
	uint8 ret = 0;

	return gsv2k11_I2cWrite(0, 0, &ret, 1) | gsv2k11_I2cRead(0, 0, &ret, 1);
}

static int gsv2k11_init(struct i2c_client *client)
{
	struct gsv2k11_data *gsv2k11 = i2c_get_clientdata(client);
	uint8 CommonBusConfig = BusConfig;
	int ret = 0;

	/* 1. Low Level Hardware Level Initialization */
	/* 1.1 init bsp support (user speficic) */
	ret = gsv2k11_i2c_check(client);
	if (ret) {
		dev_err(&client->dev, "failed to find gsv2k11\n");
		return ret;
	}

	/* 1.2 init software package and hookup user's bsp functions */
	AvApiInit();
	AvApiHookBspFunctions(&gsv2k11_I2cRead, &gsv2k11_I2cWrite,
						  NULL, NULL,
						  &gsv2k11_GetMilliSecond,
						  NULL, NULL);
	AvApiHookUserFunctions(&ListenToKeyCommand, &ListenToUartCommand, &ListenToIrdaCommand);

	/* 2.2 specific devices and ports */
	/* they must be able to be linked to the device in 1. */

	/* 2.3 init device address in 2.2 */
	gsv2k11->gsv2k11_0.DeviceAddress = AvGenerateDeviceAddress(0x00, 0x01, client->addr << 1, 0x00);

	/* 2.4 connect devices to device declaration */
	AvApiAddDevice(&gsv2k11->devices[0], Gsv2k11, 0,
		(void *)&gsv2k11->gsv2k11_0, (void *)&gsv2k11->gsv2k11Ports[0],  NULL);

	/* 3. Port Level Declaration */
	/* 3.1 init devices and port structure, must declare in number order */
	/* 0-3 HdmiRx, 4-7 HdmiTx, 8-9 TTLTx, 10-11 TTLRx,
	   20-23 Scaler, 24-27 Color, 28 VideoGen, 30 VideoIn, 32 VideoOut,
	   34 AudioGen, 36 ClockGen */

	AvApiAddPort(&gsv2k11->devices[0],&gsv2k11->gsv2k11Ports[0] ,0 ,HdmiRx);
	AvApiAddPort(&gsv2k11->devices[0],&gsv2k11->gsv2k11Ports[1] ,5 ,HdmiTx);
	AvApiAddPort(&gsv2k11->devices[0],&gsv2k11->gsv2k11Ports[2] ,32,LogicVideoTx);
	AvApiAddPort(&gsv2k11->devices[0],&gsv2k11->gsv2k11Ports[3] ,8 ,LogicAudioTx);
	AvApiAddPort(&gsv2k11->devices[0],&gsv2k11->gsv2k11Ports[4] ,20,VideoScaler);
	AvApiAddPort(&gsv2k11->devices[0],&gsv2k11->gsv2k11Ports[5] ,24,VideoColor);
	AvApiAddPort(&gsv2k11->devices[0],&gsv2k11->gsv2k11Ports[6] ,28,VideoGen);
	AvApiAddPort(&gsv2k11->devices[0],&gsv2k11->gsv2k11Ports[7] ,30,LogicVideoRx);
	AvApiAddPort(&gsv2k11->devices[0],&gsv2k11->gsv2k11Ports[8] ,10,LogicAudioRx);

	/* 3.2 initialize port content */
#if AvEnableCecFeature
	gsv2k11->gsv2k11Ports[1].content.cec->CecEnable = 1;
	if(AudioStatus == 0)
		gsv2k11->gsv2k11Ports[1].content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_DISABLE;
	else {
		gsv2k11->gsv2k11Ports[1].content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
		gsv2k11->gsv2k11Ports[1].content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;
	}
	Cec_Tx_Audio_Status.Volume = 30;
	Cec_Tx_Audio_Status.Mute   = 0;	/*  */
	Cec_Tx_Audio_Status.AudioMode = 1; /* Audio Mode is ON to meet ARC */
	Cec_Tx_Audio_Status.AudioRate = 1; /* 100% rate */
	Cec_Tx_Audio_Status.AudioFormatCode = AV_AUD_FORMAT_LINEAR_PCM; /* Follow Spec */
	Cec_Tx_Audio_Status.MaxNumberOfChannels = 2; /* Max Channels */
	Cec_Tx_Audio_Status.AudioSampleRate = 0x07; /* 32KHz/44.1KHz/48KHz */
	Cec_Tx_Audio_Status.AudioBitLen = 0x01;  /* 16-bit only */
	Cec_Tx_Audio_Status.MaxBitRate	= 0;  /* default */
	Cec_Tx_Audio_Status.ActiveSource = 0; /* default */
#endif
	/* 3.3 init fsms */

	AvApiInitDevice(&gsv2k11->devices[0]);

	AvApiPortStart();

	/* 3.4 routing */
	/* connect the port by video using AvConnectVideo */
	/* connect the port by audio using AvConnectAudio */
	/* connect the port by video and audio using AvConnectAV */

	/* 3.4.1 video routing */
	/* CHIP1 setting */
	/* case 1: default routing RxA->TxB */
	if(LogicOutputSel == 1) {
		AvApiConnectPort(&gsv2k11->gsv2k11Ports[0], &gsv2k11->gsv2k11Ports[1], AvConnectAV);
		AvApiConnectPort(&gsv2k11->gsv2k11Ports[0], &gsv2k11->gsv2k11Ports[2], AvConnectVideo);
		AvApiConnectPort(&gsv2k11->gsv2k11Ports[0], &gsv2k11->gsv2k11Ports[3], AvConnectAudio);
	} else {
	/* case 2: routing of LogicTx/Rx->TxB */
		AvApiConnectPort(&gsv2k11->gsv2k11Ports[7], &gsv2k11->gsv2k11Ports[1], AvConnectVideo);
		//AvApiConnectPort(&gsv2k11->gsv2k11Ports[8], &gsv2k11->gsv2k11Ports[1], AvConnectAudio);
	}

	LogicLedOut(LogicOutputSel);

	/* 3.4.2 ARC Connection, set after rx port connection to avoid conflict */
#if AvEnableCecFeature
	if(AudioStatus == 1) {
		AvApiConnectPort(&gsv2k11->gsv2k11Ports[0], &gsv2k11->gsv2k11Ports[1], AvConnectAudio);
	}
#endif

	/* 3.4.3 Internal Video Generator*/
#if AvEnableInternalVideoGen
	gsv2k11->gsv2k11Ports[6].content.video->timing.Vic = 0x10; /* 1080p60 */
	gsv2k11->gsv2k11Ports[6].content.video->AvailableVideoPackets = AV_BIT_AV_INFO_FRAME;
	gsv2k11->gsv2k11Ports[6].content.video->Cd         = AV_CD_24;
	gsv2k11->gsv2k11Ports[6].content.video->Y          = AV_Y2Y1Y0_RGB;
	gsv2k11->gsv2k11Ports[6].content.vg->Pattern       = AV_PT_COLOR_BAR;
#endif

	/* 3.4.4 Audio Insertion */
#if AvEnableAudioTTLInput
	gsv2k11->gsv2k11Ports[8].content.audio->AudioMute    = 0;
	gsv2k11->gsv2k11Ports[8].content.audio->AudFormat    = AV_AUD_I2S;
	gsv2k11->gsv2k11Ports[8].content.audio->AudType      = AV_AUD_TYPE_ASP;
	gsv2k11->gsv2k11Ports[8].content.audio->AudCoding    = AV_AUD_FORMAT_LINEAR_PCM;
	gsv2k11->gsv2k11Ports[8].content.audio->AudMclkRatio = AV_MCLK_256FS;
	gsv2k11->gsv2k11Ports[8].content.audio->Layout       = 1;    /* 2 channel Layout = 0 */
	gsv2k11->gsv2k11Ports[8].content.audio->Consumer     = 0;    /* Consumer */
	gsv2k11->gsv2k11Ports[8].content.audio->Copyright    = 0;    /* Copyright asserted */
	gsv2k11->gsv2k11Ports[8].content.audio->Emphasis     = 0;    /* No Emphasis */
	gsv2k11->gsv2k11Ports[8].content.audio->CatCode      = 0;    /* Default */
	gsv2k11->gsv2k11Ports[8].content.audio->SrcNum       = 0;    /* Refer to Audio InfoFrame */
	gsv2k11->gsv2k11Ports[8].content.audio->ChanNum      = 8;    /* Audio Channel Count */
	gsv2k11->gsv2k11Ports[8].content.audio->SampFreq     = AV_AUD_FS_48KHZ; /* Sample Frequency */
	gsv2k11->gsv2k11Ports[8].content.audio->ClkAccur     = 0;    /* Level 2 */
	gsv2k11->gsv2k11Ports[8].content.audio->WordLen      = 0x0B; /* 24-bit word length */
#endif

	/* 3.4.5 Video Parallel Bus Input */
	/* CommonBusConfig = 0 to disable, CommonBusConfig = 1~64 for feature setting */

	gsv2k11->gsv2k11Ports[2].content.lvtx->Config		= CommonBusConfig;
	/* 3.4.5.1 LogicVideoTx Port's Y and InCS
	   = AV_Y2Y1Y0_INVALID/AV_CS_AUTO to do no 2011 color processing,
	   = Dedicated Color for internal Color/Scaler Processing */
	if((ParallelConfigTable[CommonBusConfig*3 + 1] & 0x04) != 0) {
		gsv2k11->gsv2k11Ports[2].content.video->Y           = AV_Y2Y1Y0_YCBCR_422;
		gsv2k11->gsv2k11Ports[2].content.video->InCs        = AV_CS_LIM_YUV_709;
	} else {
		gsv2k11->gsv2k11Ports[2].content.video->Y           = AV_Y2Y1Y0_INVALID;
		gsv2k11->gsv2k11Ports[2].content.video->InCs        = AV_CS_AUTO;
	}
	/* 3.4.5.2 LogicVideoTx Port's Limited Highest Pixel Clock Frequency
	  = 600 to output HDMI 2.0 on Parallel bus,
	  = 300 to output HDMI 1.4 on Parallel bus,
	  = 150 to output 1080p on Parallel bus */
	gsv2k11->gsv2k11Ports[2].content.video->info.TmdsFreq   = 150;

	/* 3.4.6 Video Parallel Bus Input */
	gsv2k11->gsv2k11Ports[7].content.video->timing.Vic = 0x61; /* 4K60 */
	gsv2k11->gsv2k11Ports[7].content.video->AvailableVideoPackets = AV_BIT_GC_PACKET | AV_BIT_AV_INFO_FRAME;
	gsv2k11->gsv2k11Ports[7].content.video->Cd = AV_CD_24;
	if((ParallelConfigTable[CommonBusConfig*3 + 1] & 0x04) != 0) {
		gsv2k11->gsv2k11Ports[7].content.video->Y    = AV_Y2Y1Y0_YCBCR_422;
		gsv2k11->gsv2k11Ports[7].content.video->InCs = AV_CS_LIM_YUV_709;
	} else {
		gsv2k11->gsv2k11Ports[7].content.video->Y    = AV_Y2Y1Y0_RGB;
		gsv2k11->gsv2k11Ports[7].content.video->InCs = AV_CS_RGB;
	}
	gsv2k11->gsv2k11Ports[7].content.lvrx->Config = CommonBusConfig;

	/* 3.4.7 Video Parallel Bus Config */
	gsv2k11->gsv2k11Ports[7].content.rx->VideoEncrypted = 0;
	if(LogicOutputSel == 1) {
		gsv2k11->gsv2k11Ports[2].content.lvtx->Update = 1;
	} else {
		gsv2k11->gsv2k11Ports[7].content.lvrx->Update = 1;
	}

	timer_setup(&gsv2k11->gsv2k11_timer, gsv2k11_timer_handler, 0);

	return 0;
}

int gsv2k11_notifier_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&gsv2k11_notifier_head, nb);
}
EXPORT_SYMBOL(gsv2k11_notifier_register);

int gsv2k11_notifier_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&gsv2k11_notifier_head, nb);
}
EXPORT_SYMBOL(gsv2k11_notifier_unregister);

static int gsv2k11_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct gsv2k11_data *gsv2k11 = NULL;
	struct device *dev = &client->dev;

	gsv2k11 = devm_kzalloc(&client->dev, sizeof(*gsv2k11), GFP_KERNEL);
	if (!gsv2k11) {
		dev_err(&client->dev, "failed to malloc gsv2k11 for gsv2k11\n");
		return -ENOMEM;
	}
	gsv2k11->client = client;
	g_i2c_client = client;
	i2c_set_clientdata(client, gsv2k11);

	gsv2k11->reset_gpiod = devm_gpiod_get_optional(dev, "gsv2k11,reset", GPIOD_OUT_HIGH);
	if (!gsv2k11->reset_gpiod) {
		dev_err(dev, "failed to get reset gpio\n");
		return -ENODEV;
	}

	gsv2k11->mute_gpiod = devm_gpiod_get_optional(dev, "gsv2k11,mute", GPIOD_OUT_HIGH);
	if (!gsv2k11->mute_gpiod) {
		dev_err(dev, "failed to get mute gpio\n");
		return -ENODEV;
	}

	gsv2k11->gsv2k11_wq = create_singlethread_workqueue("gsv2k11-wq");
	if (!gsv2k11->gsv2k11_wq) {
		dev_err(dev, "failed to create workqueue for gsv2k11\n");
		return -EINVAL;
	}
	INIT_DELAYED_WORK(&gsv2k11->gsv2k11_delayed_work, gsv2k11_work);

	gsv2k11_reset(client);
	ret = gsv2k11_init(client);
	if (ret) {
		dev_err(dev, "failed to probe gsv2k11, ret = %d\n", ret);
		goto err;
	}

	queue_delayed_work(gsv2k11->gsv2k11_wq, &gsv2k11->gsv2k11_delayed_work, msecs_to_jiffies(0));

	ret = devm_device_add_group(&client->dev, &gsv2k11_attribute_group);
	if (ret) {
		dev_err(dev, "failed to add group attr for gsv2k11\n");
		goto err;
	}

	dev_info(dev, "gsv2k11 probe success\n");

	return 0;
err:
	destroy_workqueue(gsv2k11->gsv2k11_wq);
	return ret;
}

static int gsv2k11_remove(struct i2c_client *client)
{
	struct gsv2k11_data *gsv2k11 = i2c_get_clientdata(client);

	devm_device_remove_group(&client->dev, &gsv2k11_attribute_group);

	del_timer_sync(&gsv2k11->gsv2k11_timer);

	cancel_delayed_work_sync(&gsv2k11->gsv2k11_delayed_work);

	if (gsv2k11->gsv2k11_wq) {
		destroy_workqueue(gsv2k11->gsv2k11_wq);
	}

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int gsv2k11_suspend(struct device *dev)
{
	struct gsv2k11_data *gsv2k11 = dev_get_drvdata(dev);
	int ret = 0;

	del_timer_sync(&gsv2k11->gsv2k11_timer);
	cancel_delayed_work_sync(&gsv2k11->gsv2k11_delayed_work);

	return ret;
}

static int gsv2k11_resume(struct device *dev)
{
	struct gsv2k11_data *gsv2k11 = dev_get_drvdata(dev);
	int ret = 0;

	queue_delayed_work(gsv2k11->gsv2k11_wq,
		&gsv2k11->gsv2k11_delayed_work, msecs_to_jiffies(0));

	return ret;
}
#endif

static SIMPLE_DEV_PM_OPS(gsv2k11_pm_ops,
			 gsv2k11_suspend, gsv2k11_resume);

static const struct of_device_id gsv2k11_dt_match[] = {
	{.compatible = "gsv2k11", },
	{},
};
MODULE_DEVICE_TABLE(of, gsv2k11_dt_match);

static const struct i2c_device_id gsv2k11_ids[] = {
	{"gsv2k11", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, gsv2k11_ids);

static struct i2c_driver gsv2k11_i2c_driver = {
	.probe = gsv2k11_probe,
	.remove = gsv2k11_remove,
	.driver = {
		.name = "gsv2k11_i2c_driver",
		.owner = THIS_MODULE,
		.pm = &gsv2k11_pm_ops,
		.of_match_table = of_match_ptr(gsv2k11_dt_match),
	},
	.id_table = gsv2k11_ids,
};
module_i2c_driver(gsv2k11_i2c_driver);

MODULE_AUTHOR("<zhiwen.liang@hollyland-tech.com>");
MODULE_DESCRIPTION("Driver for gsv2k11");
MODULE_LICENSE("GPL and additional rights");
