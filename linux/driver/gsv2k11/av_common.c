#include "av_config.h"

#if AvEnableCecFeature /* CEC Related */
uchar  DevicePowerStatus = 0;
char   DeviceName[20] = "GSV Default";
uchar  AudioStatus = 0;
CEC_AUDIO_STATUS Cec_Tx_Audio_Status;
#endif

const uint8 PktSize[20] ={31, /*AV_PKT_AV_INFO_FRAME */
                          14, /*AV_PKT_AUDIO_INFO_FRAME*/
                          31, /*AV_PKT_ACP_PACKET*/
                          31, /*AV_PKT_SPD_PACKET*/
                          31, /*AV_PKT_ISRC1_PACKET*/
                          31, /*AV_PKT_ISRC2_PACKET*/
                          31, /*AV_PKT_GMD_PACKET*/
                          32, /*AV_PKT_GC_PACKET*/
                          17, /*AV_PKT_MPEG_PACKET*/
                          31, /*AV_PKT_VS_PACKET*/
                          5,  /*AV_PKT_AUDIO_CHANNEL_STATUS*/
                          32, /*AV_PKT_AUDIO_SAMPLE_PACKET*/
                          32, /*AV_PKT_ACR_PACKET*/
                          32, /*AV_PKT_EXT_AV_INFO_FRAME*/
                          31, /*AV_PKT_HDR_PACKET*/
                          31, /*AV_PKT_EMP_PACKET*/
                          31, /*AV_PKT_SPARE3_PACKET*/
                          31, /*AV_PKT_SPARE4_PACKET*/
                          32, /*AV_PKT_UNKNOWN_PACKET*/
                          32};/*AV_PKT_ALL_PACKETS*/

                       /*  32    44    48     88     96    176    192    768   Undefined */
const uint32 NTable[9] ={4096, 6272, 6144, 12544, 12288, 25088, 24576, 98304,  4096};
const uchar  NIdx[16]  ={ 1,  8,  2,  0,  8,  8,  8,  8,
                                    3,  7,  4,  8,  5,  8,  6,  8};
/* i*3+2 = MCLK ratio, 1 = 256Fs, 0 = 128Fs, 3 = 512Fs */
const uchar ChannelStatusSfTable[] = {
    (uchar)AV_AUD_FS_32KHZ,     3,    1,
    (uchar)AV_AUD_FS_44KHZ,     0,    1,
    (uchar)AV_AUD_FS_48KHZ,     2,    1,
    (uchar)AV_AUD_FS_88KHZ,     8,    1,
    (uchar)AV_AUD_FS_96KHZ,    10,    1,
    (uchar)AV_AUD_FS_176KHZ,   12,    1,
    (uchar)AV_AUD_FS_192KHZ,   14,    1,
    (uchar)AV_AUD_FS_HBR,       9,    1,
    (uchar)AV_AUD_FS_FROM_STRM, 0,    1,
    0xff,                    0xff, 0xff
};

#if AvEnableCecFeature /* CEC Related */

/* Size is FLEX_OP_CODES */
const uchar FlexOpCodes[] = {
/* opcode; standard parameters length, max parameters length, min parameters length   */
    AV_CEC_MSG_REQUEST_SHORT_AUDIO_DESCRIPTOR, 4,5,2
};

/* Size is CEC_OP_CODES */
const uint8 CecOpCodes[] = {
        AV_CEC_MSG_STANDBY, 2,         AV_CEC_MSG_ROUTE_CHANGE, 6,
        AV_CEC_MSG_ROUTE_INFO, 4,      AV_CEC_MSG_ACTIVE_SRC, 4,
        AV_CEC_MSG_GIVE_PHYS_ADDR, 2,  AV_CEC_MSG_REPORT_PHYS_ADDR, 5,
        AV_CEC_MSG_SET_STRM_PATH, 4,   AV_CEC_MSG_ABORT, 2,
        AV_CEC_MSG_FEATURE_ABORT, 4,   AV_CEC_MSG_INITIATE_ARC , 2,
        AV_CEC_MSG_REPORT_ARC_INITIATED, 2, AV_CEC_MSG_REPORT_ARC_TERMINATED, 2,
        AV_CEC_MSG_REQUEST_ARC_INITIATION, 2, AV_CEC_MSG_REQUEST_ARC_TERMINATION, 2,
        AV_CEC_MSG_TERMINATE_ARC, 2,            AV_CEC_MSG_IMAGE_VIEW_ON, 2,
        AV_CEC_MSG_TUNER_STEP_INC, 2,           AV_CEC_MSG_TUNER_STEP_DEC, 2,
        AV_CEC_MSG_GIVE_TUNER_DEV_STATUS, 3,    AV_CEC_MSG_RECORD_STATUS, 3,
        AV_CEC_MSG_RECORD_OFF, 2,           AV_CEC_MSG_TEXT_VIEW_ON, 2,
        AV_CEC_MSG_RECORD_TV_SCREEN, 2,     AV_CEC_MSG_GIVE_DECK_STATUS, 3,
        AV_CEC_MSG_DECK_STATUS, 3,          AV_CEC_MSG_SET_MENU_LANGUAGE, 5,
        AV_CEC_MSG_CLR_ANALOGUE_TIMER, 13,  AV_CEC_MSG_SET_ANALOGUE_TIMER, 13,
        AV_CEC_MSG_PLAY, 3,                 AV_CEC_MSG_DECK_CONTROL, 3,
        AV_CEC_MSG_TIMER_CLEARED_STATUS, 3, AV_CEC_MSG_USER_CONTROL_RELEASED, 2,
        AV_CEC_MSG_GIVE_OSD_NAME, 2,   AV_CEC_MSG_GIVE_SYSTEM_AUDIO_MODE_STATUS, 2,
        AV_CEC_MSG_GIVE_AUDIO_STATUS, 2,    AV_CEC_MSG_SET_SYSTEM_AUDIO_MODE, 3,
        AV_CEC_MSG_REPORT_AUDIO_STATUS, 3,  AV_CEC_MSG_SYSTEM_AUDIO_MODE_STATUS, 3,
        AV_CEC_MSG_REQ_ACTIVE_SRC, 2,       AV_CEC_MSG_DEVICE_VENDOR_ID, 5,
        AV_CEC_MSG_VENDOR_REMOTE_BTN_UP, 2, AV_CEC_MSG_GET_VENDOR_ID, 2,
        AV_CEC_MSG_MENU_REQUEST, 3,         AV_CEC_MSG_MENU_STATUS, 3,
        AV_CEC_MSG_GIVE_PWR_STATUS, 2,      AV_CEC_MSG_REPORT_PWR_STATUS, 3,
        AV_CEC_MSG_GET_MENU_LANGUAGE, 2,    AV_CEC_MSG_SEL_ANALOGUE_SERVICE, 6,
        AV_CEC_MSG_SEL_DIGITAL_SERVICE, 9,  AV_CEC_MSG_SET_DIGITAL_TIMER, 16,
        AV_CEC_MSG_CLR_DIGITAL_TIMER, 16,   AV_CEC_MSG_SET_AUDIO_RATE, 3,
        AV_CEC_MSG_INACTIVE_SOURCE, 4,      AV_CEC_MSG_CEC_VERSION, 3,
        AV_CEC_MSG_GET_CEC_VERSION, 2,      AV_CEC_MSG_CLR_EXTERNAL_TIMER, 13,
        AV_CEC_MSG_SET_EXTERNAL_TIMER, 13,
        AV_CEC_MSG_REPORT_SHORT_AUDIO_DESCRIPTOR, 5,
        AV_CEC_MSG_REQUEST_SHORT_AUDIO_DESCRIPTOR, 4,
        AV_CEC_MSG_USER_CONTROL_PRESSED,           3
};
#endif

const uint8 ChanCount[32] =
{
        1, 2, 2, 3, 2, 3, 3, 4,
        3, 4, 4, 5, 4, 5, 5, 6,
        5, 6, 6, 7, 3, 4, 4, 5,
        4, 5, 5, 6, 5, 6, 6, 7
};

const AvVideoAspectRatio ARTable[108] =
{
    AV_AR_NOT_INDICATED,  AV_AR_4_3,  AV_AR_4_3,  AV_AR_16_9, AV_AR_16_9, AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, /* VIC 0 ~ 7 */
    AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, /* VIC 8 ~ 15 */
    AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_16_9, AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  /* VIC 16 ~ 23 */
    AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_16_9, /* VIC 24 ~ 31 */
    AV_AR_16_9, AV_AR_16_9, AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_16_9, /* VIC 32 ~ 39 */
    AV_AR_16_9, AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_16_9, AV_AR_16_9, /* VIC 40 ~ 47 */
    AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, /* VIC 48 ~ 55 */
    AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_16_9, AV_AR_16_9, AV_AR_16_9, AV_AR_16_9, /* VIC 56 ~ 63 */
    AV_AR_16_9,                                                                                     /* VIC 64 */
    AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27,/* VIC 65 ~ 72 */
    AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27,/* VIC 73 ~ 80 */
    AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27,/* VIC 81 ~ 88 */
    AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27,                                             /* VIC 89 ~ 92 */
    AV_AR_16_9,  AV_AR_16_9,  AV_AR_16_9,  AV_AR_16_9,  AV_AR_16_9,                                 /* VIC 93 ~ 97 */
    AV_AR_256_135, AV_AR_256_135, AV_AR_256_135, AV_AR_256_135, AV_AR_256_135,                      /* VIC 98 ~ 102 */
    AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27                                 /* VIC 103 ~ 107 */
};

#if AvEnableInternalVideoGen
const uchar VideoGenVicTable[] = {
    0x10, 0x10, 0x04, 0x00, 0x00, 0x00, 0x00, /* 1080p60 */
    0x61, 0x5F, 0x01, 0x00, 0x00, 0x00, 0x00, /* 4K60  */
    0x5F, 0x5F, 0x02, 0x00, 0x00, 0x00, 0x00, /* 4K30  */
    0x60, 0x5E, 0x01, 0x00, 0x00, 0x00, 0x00, /* 4K50  */
    0x5E, 0x5E, 0x02, 0x00, 0x00, 0x00, 0x00, /* 4K25  */
    0x5D, 0x5D, 0x02, 0x00, 0x00, 0x00, 0x00, /* 4K24  */
    0x62, 0x62, 0x02, 0x00, 0x00, 0x00, 0x00, /* 4KS24 */
    0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, /* 720p  */
    0x02, 0x03, 0x16, 0x00, 0x00, 0x00, 0x00, /* 480p  */
    0x11, 0x11, 0x16, 0x00, 0x00, 0x00, 0x00, /* 576p  */
    0x06, 0x06, 0x16, 0x00, 0x00, 0x00, 0x00, /* 480i  */
    0x15, 0x15, 0x16, 0x00, 0x00, 0x00, 0x00, /* 576i  */
    0x05, 0x05, 0x08, 0x00, 0x00, 0x00, 0x00, /* 1080i60 */
    0x14, 0x14, 0x08, 0x00, 0x00, 0x00, 0x00, /* 1080i50 */
    0x27, 0x27, 0x08, 0x00, 0x00, 0x00, 0x00, /* 1080i50s */
    0xFF
};
#endif

const uint8 ParallelConfigTable[] = {
    0x00, 0x00, 0x00,    // 0: Invalid Setting for disabled Parallel bus
    0x04, 0x03, 0x82,    // 1: Index 21, 4x single pixel 12-bit 444
    0x14, 0x01, 0x82,    // 2: Index 61, 4x dual pixel 8-bit 444
    0x04, 0x05, 0xC1,    // 3: Index 58-1, HIS mode in DDR mode
    0x14, 0x01, 0xA2,    // 4: 4x dual pixel UDP mode using LVDS
    0x01, 0x03, 0x01,    // 5: TTL 36-bit, DDR mode, YCbCr 444
    0x01, 0x15, 0x10,    // 6: TTL BT.1120 16-bit, SDR mode, YCbCr 422
    0x11, 0x01, 0x01,    // 7: TTL 48-bit, DDR mode
    0x01, 0x15, 0x11,    // 8: TTL BT.1120 16-bit, DDR mode, YCbCr 422
    0x11, 0x01, 0x20,    // 9: dual pixel UDP mode using TTL, SDR
    0x01, 0x15, 0x00,    // 10: TTL BT.1120 16-bit, SDR mode, YCbCr 422, separate sync
    0x17, 0x81, 0x82,    // 11: LVDS VESA configuration, dual pixel, 8-bit
    0x42, 0x11, 0x00,    // 12: TTL x2 12-bit, SDR mode, separate sync
    0x04, 0x01, 0xA2,    // 13: 4x single pixel UDP mode using LVDS
    0x14, 0x05, 0x92,    // 14: 4x dual pixel embedd-sync using LVDS
    0x04, 0x06, 0xC1,    // 15: HIS mode 10-bit color depth using DDR mode, 8-lane
    0x01, 0x05, 0x10,    // 16: TTL BT.1120 16-bit, SDR mode, YCbCr 422
    0xFF, 0xFF, 0xFF
};
