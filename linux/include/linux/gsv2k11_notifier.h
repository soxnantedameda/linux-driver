
#ifndef __GSV2K11_NOTIFIER_H__
#define __GSV2K11_NOTIFIER_H__

#include <linux/device.h>

typedef enum {
    VFMT_CEA_NULL                        = 0,
    VFMT_CEA_01_640x480P_60HZ            = 1,
    VFMT_CEA_02_720x480P_60HZ            = 2,
    VFMT_CEA_03_720x480P_60HZ            = 3, //include
    VFMT_CEA_04_1280x720P_60HZ           = 4, //include
    VFMT_CEA_05_1920x1080I_60HZ          = 5, //include
    VFMT_CEA_06_720x480I_60HZ            = 6,
    VFMT_CEA_07_720x480I_60HZ            = 7, //include
    VFMT_CEA_08_720x240P_60HZ            = 8,
    VFMT_CEA_09_720x240P_60HZ            = 9,
    VFMT_CEA_10_720x480I_60HZ            = 10,
    VFMT_CEA_11_720x480I_60HZ            = 11,
    VFMT_CEA_12_720x240P_60HZ            = 12,
    VFMT_CEA_13_720x240P_60HZ            = 13,
    VFMT_CEA_14_1440x480P_60HZ           = 14,
    VFMT_CEA_15_1440x480P_60HZ           = 15,
    VFMT_CEA_16_1920x1080P_60HZ          = 16, //include
    VFMT_CEA_17_720x576P_50HZ            = 17,
    VFMT_CEA_18_720x576P_50HZ            = 18, //include
    VFMT_CEA_19_1280x720P_50HZ           = 19, //include
    VFMT_CEA_20_1920x1080I_50HZ          = 20, //include
    VFMT_CEA_21_720x576I_50HZ            = 21,
    VFMT_CEA_22_720x576I_50HZ            = 22, //include
    VFMT_CEA_23_720x288P_50HZ            = 23,
    VFMT_CEA_24_720x288P_50HZ            = 24,
    VFMT_CEA_25_720x576I_50HZ            = 25,
    VFMT_CEA_26_720x576I_50HZ            = 26,
    VFMT_CEA_27_720x288P_50HZ            = 27,
    VFMT_CEA_28_720x288P_50HZ            = 28,
    VFMT_CEA_29_1440x576P_50HZ           = 29,
    VFMT_CEA_30_1440x576P_50HZ           = 30,
    VFMT_CEA_31_1920x1080P_50HZ          = 31, //include
    VFMT_CEA_32_1920x1080P_24HZ          = 32, //include
    VFMT_CEA_33_1920x1080P_25HZ          = 33, //include
    VFMT_CEA_34_1920x1080P_30HZ          = 34, //include
    VFMT_CEA_35_2880x480P_60HZ           = 35,
    VFMT_CEA_36_2880x480P_60HZ           = 36,
    VFMT_CEA_37_2880x576P_50HZ           = 37,
    VFMT_CEA_38_2880x576P_50HZ           = 38,
    VFMT_CEA_39_1920x1080I_50HZ          = 39,

    VFMT_CEA_60_1280x720P_24HZ           = 60,
    VFMT_CEA_61_1280x720P_25HZ           = 61,
    VFMT_CEA_62_1280x720P_30HZ           = 62,
    VFMT_CEA_63_1280x720P_120HZ          = 63,
    VFMT_CEA_64_1280x720P_100HZ          = 64,
    VFMT_CEA_65_1280x720P_24HZ           = 65,
    VFMT_CEA_66_1280x720P_25HZ           = 66,
    VFMT_CEA_67_1280x720P_30HZ           = 67,
    VFMT_CEA_68_1280x720P_50HZ           = 68,
    VFMT_CEA_69_1280x720P_60HZ           = 69,
    VFMT_CEA_70_1280x720P_100HZ          = 70,
    VFMT_CEA_71_1280x720P_120HZ          = 71,
    VFMT_CEA_72_1920x1080P_24HZ          = 72,
    VFMT_CEA_73_1920x1080P_25HZ          = 73,
    VFMT_CEA_74_1920x1080P_23HZ          = 74,
    VFMT_CEA_75_1920x1080P_50HZ          = 75,
    VFMT_CEA_76_1920x1080P_60HZ          = 76,

    VFMT_CEA_93_3840x2160P_24HZ          = 93, //include
    VFMT_CEA_94_3840x2160P_25HZ          = 94, //include
    VFMT_CEA_95_3840x2160P_30HZ          = 95, //include
    VFMT_CEA_96_3840x2160P_50HZ          = 96, //include
    VFMT_CEA_97_3840x2160P_60HZ          = 97, //include
    VFMT_CEA_98_4096x2160P_24HZ          = 98, //include
    VFMT_CEA_99_4096x2160P_25HZ          = 99, //include
    VFMT_CEA_100_4096x2160P_30HZ         = 100, //include
    VFMT_CEA_101_4096x2160P_50HZ         = 101, //include
    VFMT_CEA_102_4096x2160P_60HZ         = 102, //include

    VFMT_CEA_103_3840x2160P_24HZ         = 103,
    VFMT_CEA_104_3840x2160P_25HZ         = 104,
    VFMT_CEA_105_3840x2160P_30HZ         = 105,
    VFMT_CEA_106_3840x2160P_50HZ         = 106,
    VFMT_CEA_107_3840x2160P_60HZ         = 107,
} CEA_VIDEOFORMAT_E;

#ifdef CONFIG_GSV2K11
extern int gsv2k11_notifier_register(struct notifier_block *nb);
extern int gsv2k11_notifier_unregister(struct notifier_block *nb);
#else
static inline int gsv2k11_notifier_register(struct notifier_block *nb)
{
	return -ENODEV;
}

static inline int gsv2k11_notifier_unregister(struct notifier_block *nb)
{
	return -ENODEV;
}
#endif

#endif
