#ifndef SH5001A_H
#define SH5001A_H

#include <linux/device.h>

#define SH5001_ACC_XL				(0x00U)
#define SH5001_ACC_XH				(0x01U)
#define SH5001_ACC_YL				(0x02U)
#define SH5001_ACC_YH				(0x03U)
#define SH5001_ACC_ZL				(0x04U)
#define SH5001_ACC_ZH				(0x05U)
#define SH5001_GYRO_XL				(0x06U)
#define SH5001_GYRO_XH				(0x07U)
#define SH5001_GYRO_YL				(0x08U)
#define SH5001_GYRO_YH				(0x09U)
#define SH5001_GYRO_ZL				(0x0AU)
#define SH5001_GYRO_ZH				(0x0BU)
#define SH5001_TEMP_ZL				(0x0CU)
#define SH5001_TEMP_ZH				(0x0DU)
#define SH5001_TIMESTAMP_L			(0x0EU)
#define SH5001_TIMESTAMP_M			(0x0FU)
#define SH5001_TIMESTAMP_H			(0x10U)
#define SH5001_INT_STA0				(0x16U)
#define SH5001_INT_STA1				(0x17U)
#define SH5001_INT_STA2				(0x18U)
#define SH5001_INT_STA3				(0x19U)
#define SH5001_INT_STA4				(0x1AU)
#define SH5001_FIFO_STA0			(0x1BU)
#define SH5001_FIFO_STA1			(0x1CU)
#define SH5001_FIFO_DATA			(0x1DU)

#define SH5001_CHIP_ID				(0x1FU)
#define SH5001_CHIP_ID_VAL			(0xA1U)

#define SH5001_ACC_CONF0			(0x20U)
#define SH5001_ACC_CONF1			(0x21U)
#define SH5001_ACC_CONF2			(0x22U)

#define SH5001_GYRO_CONF0			(0x23U)
#define SH5001_GYRO_CONF1			(0x24U)
#define SH5001_GYRO_CONF2			(0x25U)

#define SH5001_OIS_ACC_CONF			(0x26U)
#define SH5001_OIS_GYRO_CONF		(0x27U)

#define SH5001_TEMP_CONF0			(0x28U)
#define SH5001_ACC_DOWNSAMPLE		(0x28U)
#define SH5001_TEMP_CONF1			(0x29U)
#define SH5001_TEMP_CONF2			(0x2AU)
#define SH5001_TIMESTAMP_CONF		(0x2AU)

#define SH5001_POWER_MODE			(0x30U)
#define SH5001_POWER_MODE_PWM		(0x31U)

#define SH5001_I2C_CONF				(0x34U)
#define SH5001_SPI_CONF				(0x34U)
#define SH5001_I2C_READ_LOOP		(0x34U)
#define SH5001_OIS_SPI_CONF			(0x34U)


#define SH5001_FIFO_CONF0			(0x35U)
#define SH5001_FIFO_CONF1			(0x36U)
#define SH5001_FIFO_CONF2			(0x37U)
#define SH5001_FIFO_CONF3			(0x38U)
#define SH5001_FIFO_CONF4			(0x39U)

#define SH5001_MI2C_CONF0			(0x3AU)
#define SH5001_MI2C_CONF1			(0x3BU)
#define SH5001_MI2C_CMD0			(0x3CU)
#define SH5001_MI2C_CMD1			(0x3DU)
#define SH5001_MI2C_WR				(0x3EU)
#define SH5001_MI2C_RD				(0x3FU)

#define SH5001_INT_ENABLE0			(0x40U)
#define SH5001_INT_ENABLE1			(0x41U)
#define SH5001_INT_CONF				(0x42U)
#define SH5001_INT_LIMIT			(0x43U)
#define SH5001_ORIEN_INTCONF0		(0x44U)
#define SH5001_ORIEN_INTCONF1		(0x45U)
#define SH5001_ORIEN_INT_1G5_LOW	(0x46U)
#define SH5001_ORIEN_INT_1G5_HIGH	(0x47U)
#define SH5001_ORIEN_INT_SLOPE_LOW	(0x48U)
#define SH5001_ORIEN_INT_SLOPE_HIGH	(0x49U)
#define SH5001_ORIEN_INT_HYST_LOW	(0x4AU)
#define SH5001_ORIEN_INT_HYST_HIGH	(0x4BU)

#define SH5001_FLAT_INT_CONF		(0x4CU)

#define SH5001_ACT_INACT_INT_CONF	(0x4DU)
#define SH5001_ACT_INACT_INT_LINK	(0x4EU)
#define SH5001_ACT_INT_THRESHOLDL	(0x54U)
#define SH5001_ACT_INT_THRESHOLDH	(0x55U)
#define SH5001_ACT_INT_TIME			(0x56U)

#define SH5001_INACT_INT_THRESHOLDL	(0x57U)
#define SH5001_INACT_INT_THRESHOLDH	(0x58U)
#define SH5001_INACT_INT_TIME		(0x59U)

#define SH5001_SMD_INT_THRESHOLDL	(0x5AU)
#define SH5001_SMD_INT_THRESHOLDH	(0x5BU)
#define SH5001_SMD_INT_TIME			(0x5CU)

#define SH5001_TAP_INT_CONF			(0x4EU)
#define SH5001_TAP_INT_THRESHOLDL	(0x4FU)
#define SH5001_TAP_INT_THRESHOLDH	(0x50U)
#define SH5001_TAP_INT_DURATION		(0x51U)
#define SH5001_TAP_INT_LATENCY		(0x52U)
#define SH5001_DTAP_INT_WINDOW		(0x53U)

#define SH5001_HIGHG_INT_CONF		(0x5DU)
#define SH5001_HIGHG_INT_THRESHOLDL	(0x5EU)
#define SH5001_HIGHG_INT_THRESHOLDH	(0x5FU)
#define SH5001_HIGHG_INT_TIME		(0x60U)

#define SH5001_LOWG_INT_CONF		(0x5DU)
#define SH5001_LOWG_INT_THRESHOLDL	(0x61U)
#define SH5001_LOWG_INT_THRESHOLDH	(0x62U)
#define SH5001_LOWG_INT_TIME		(0x63U)

#define SH5001_FREEFALL_INT_THRES	(0x64U)
#define SH5001_FREEFALL_INT_TIME	(0x65U)

#define SH5001_INT_PIN_MAP0			(0x66U)
#define SH5001_INT_PIN_MAP1			(0x67U)

#define SH5001_SPI_REG_ACCESS		(0x7FU)
#define SH5001_AUX_I2C_CONF			(0xFDU)

#define SH5001_PIN_PP_CONF0			(0xFDU)
#define SH5001_PIN_PP_CONF1			(0xFEU)

/******************************************************************
*	ACC Config Macro Definitions
******************************************************************/
#define SH5001_ACC_ODR_1000HZ	(0x00U)  //TODO need to set ref datasheet
#define SH5001_ACC_ODR_500HZ	(0x01U)
#define SH5001_ACC_ODR_250HZ	(0x02U)
#define SH5001_ACC_ODR_125HZ	(0x03U)

#define SH5001_ACC_ODR_2000HZ	(0x08U)
#define SH5001_ACC_ODR_4000HZ	(0x09U)
#define SH5001_ACC_ODR_8000HZ	(0x0AU)

#define SH5001_ACC_ODR_880HZ	(0x00U)  //TODO need to set ref datasheet
#define SH5001_ACC_ODR_440HZ	(0x01U)
#define SH5001_ACC_ODR_220HZ	(0x02U)
#define SH5001_ACC_ODR_110HZ	(0x03U)

#define SH5001_ACC_ODR_1760HZ	(0x08U)
#define SH5001_ACC_ODR_3520HZ	(0x09U)
#define SH5001_ACC_ODR_7040HZ	(0x0AU)

#define SH5001_ACC_RANGE_2G		(0x00U)
#define SH5001_ACC_RANGE_4G		(0x10U)
#define SH5001_ACC_RANGE_8G		(0x20U)
#define SH5001_ACC_RANGE_16G	(0x30U)
#define SH5001_ACC_RANGE_32G	(0x40U)

#define SH5001_ACC_ODRX040		(0x00U)
#define SH5001_ACC_ODRX036		(0x01U)
#define SH5001_ACC_ODRX032		(0x02U)
#define SH5001_ACC_ODRX028		(0x03U)
#define SH5001_ACC_ODRX024		(0x04U)
#define SH5001_ACC_ODRX020		(0x05U)
#define SH5001_ACC_ODRX016		(0x06U)
#define SH5001_ACC_ODRX014		(0x07U)
#define SH5001_ACC_ODRX012		(0x08U)
#define SH5001_ACC_ODRX010		(0x09U)
#define SH5001_ACC_ODRX008		(0x0aU)
#define SH5001_ACC_ODRX006		(0x0bU)
#define SH5001_ACC_ODRX004		(0x0cU)
#define SH5001_ACC_ODRX003		(0x0dU)
#define SH5001_ACC_ODRX002		(0x0eU)
#define SH5001_ACC_ODRX001		(0x0fU)

#define SH5001_ACC_FILTER_EN	(0x01U)
#define SH5001_ACC_FILTER_DIS	(0x00U)
#define SH5001_ACC_BYPASS_EN	(0x02U)
#define SH5001_ACC_BYPASS_DIS	(0x00U)


/******************************************************************
*	GYRO Config Macro Definitions
******************************************************************/
#define SH5001_GYRO_ODR_1000HZ	(0x00U)
#define SH5001_GYRO_ODR_500HZ	(0x01U)
#define SH5001_GYRO_ODR_250HZ	(0x02U)
#define SH5001_GYRO_ODR_125HZ	(0x03U)
#define SH5001_GYRO_ODR_2000HZ	(0x08U)
#define SH5001_GYRO_ODR_4000HZ	(0x09U)
#define SH5001_GYRO_ODR_8000HZ	(0x0AU)
#define SH5001_GYRO_ODR_16000HZ	(0x0BU)

#define SH5001_GYRO_ODR_880HZ	(0x00U)
#define SH5001_GYRO_ODR_440HZ	(0x01U)
#define SH5001_GYRO_ODR_220HZ	(0x02U)
#define SH5001_GYRO_ODR_110HZ	(0x03U)
#define SH5001_GYRO_ODR_1760HZ	(0x08U)
#define SH5001_GYRO_ODR_3520HZ	(0x09U)
#define SH5001_GYRO_ODR_7040HZ	(0x0AU)
#define SH5001_GYRO_ODR_14080HZ	(0x0BU)

#define SH5001_GYRO_RANGE_31	(0x00U)
#define SH5001_GYRO_RANGE_62	(0x10U)
#define SH5001_GYRO_RANGE_125	(0x20U)
#define SH5001_GYRO_RANGE_250	(0x30U)
#define SH5001_GYRO_RANGE_500	(0x40U)
#define SH5001_GYRO_RANGE_1000	(0x50U)
#define SH5001_GYRO_RANGE_2000	(0x60U)
#define SH5001_GYRO_RANGE_4000	(0x70U)

#define SH5001_GYRO_ODRX040		(0x00U)
#define SH5001_GYRO_ODRX036		(0x01U)
#define SH5001_GYRO_ODRX032		(0x02U)
#define SH5001_GYRO_ODRX028		(0x03U)
#define SH5001_GYRO_ODRX024		(0x04U)
#define SH5001_GYRO_ODRX020		(0x05U)
#define SH5001_GYRO_ODRX016		(0x06U)
#define SH5001_GYRO_ODRX014		(0x07U)
#define SH5001_GYRO_ODRX012		(0x08U)
#define SH5001_GYRO_ODRX010		(0x09U)
#define SH5001_GYRO_ODRX008		(0x0aU)
#define SH5001_GYRO_ODRX006		(0x0bU)
#define SH5001_GYRO_ODRX004		(0x0cU)
#define SH5001_GYRO_ODRX003		(0x0dU)
#define SH5001_GYRO_ODRX002		(0x0eU)
#define SH5001_GYRO_ODRX001		(0x0fU)

#define SH5001_GYRO_FILTER_EN	(0x01U)
#define SH5001_GYRO_FILTER_DIS	(0x00U)
#define SH5001_GYRO_BYPASS_EN	(0x02U)
#define SH5001_GYRO_BYPASS_DIS	(0x00U)

#define SH5001_GYRO_OFF_INACT   (0x80U)
#define SH5001_GYRO_ON_INACT    (0x00U)

/******************************************************************
*	Time Stamp Config Macro Definitions
******************************************************************/
#define SH5001_TS_ODR_25KHZ		(0x20U)
#define SH5001_TS_ODR_1KHZ		(0x00U)
#define SH5001_TS_EN			(0x40U)
#define SH5001_TS_DIS			(0x00U)

/******************************************************************
*	Temperature Config Macro Definitions
******************************************************************/
#define SH5001_TEMP_ODR_500HZ	(0x00U)
#define SH5001_TEMP_ODR_250HZ	(0x02U)
#define SH5001_TEMP_ODR_125HZ	(0x04U)
#define SH5001_TEMP_ODR_63HZ	(0x06U)

#define SH5001_TEMP_EN			(0x01U)
#define SH5001_TEMP_DIS			(0x00U)

/******************************************************************
*	INT Config Macro Definitions
******************************************************************/
#define SH5001_INT_LOWG				(0x8000U)
#define SH5001_INT_HIGHG			(0x4000U)
#define SH5001_INT_INACT			(0x2000U)
#define SH5001_INT_ACT				(0x1000U)
#define SH5001_INT_DOUBLE_TAP		(0x0800U)
#define SH5001_INT_SINGLE_TAP		(0x0400U)
#define SH5001_INT_FLAT				(0x0200U)
#define SH5001_INT_ORIENTATION		(0x0100U)
#define SH5001_INT_TAP				(0x0020U)
#define SH5001_INT_SMD				(0x0010U)
#define SH5001_INT_FIFO_WATERMARK	(0x0008U)
#define SH5001_INT_GYRO_READY		(0x0004U)
#define SH5001_INT_ACC_READY		(0x0002U)
#define SH5001_INT_FREE_FALL		(0x0001U)
#define SH5001_INT_UP_DOWN_Z		(0x0040U)

#define SH5001_INT_EN				(0x01U)
#define SH5001_INT_DIS				(0x00U)

#define SH5001_INT_MAP_INT1			(0x01U)
#define SH5001_INT_MAP_INT0			(0x00U)

#define SH5001_INT0_LEVEL_LOW		(0x80U)
#define SH5001_INT0_LEVEL_HIGH		(0x7FU)
#define SH5001_INT_NO_LATCH			(0x40U)
#define SH5001_INT_LATCH			(0xBFU)
#define SH5001_INT_CLEAR_ANY		(0x10U)
#define SH5001_INT_CLEAR_STATUS	    (0xEFU)
#define SH5001_INT1_OD				(0x08U)
#define SH5001_INT1_NORMAL			(0xF7U)
#define SH5001_INT1_OUTPUT			(0x04U)
#define SH5001_INT1_INPUT			(0xFBU)
#define SH5001_INT0_OD				(0x02U)
#define SH5001_INT0_NORMAL			(0xFDU)
#define SH5001_INT0_OUTPUT			(0x01U)
#define SH5001_INT0_INPUT			(0xFEU)


/******************************************************************
*	Orientation Blocking Config Macro Definitions
******************************************************************/
#define SH5001_ORIENT_BLOCK_MODE0	(0x00U)
#define SH5001_ORIENT_BLOCK_MODE1	(0x04U)
#define SH5001_ORIENT_BLOCK_MODE2	(0x08U)
#define SH5001_ORIENT_BLOCK_MODE3	(0x0CU)

#define SH5001_ORIENT_SYMM			(0x00U)
#define SH5001_ORIENT_HIGH_ASYMM	(0x01U)
#define SH5001_ORIENT_LOW_ASYMM		(0x02U)


/******************************************************************
*	Flat Time Config Macro Definitions
******************************************************************/
#define SH5001_FLAT_TIME_200MS		(0x00U)
#define SH5001_FLAT_TIME_400MS		(0x40U)
#define SH5001_FLAT_TIME_800MS		(0x80U)
#define SH5001_FLAT_TIME_000MS		(0xC0U)

/******************************************************************
*	ACT and INACT Int Config Macro Definitions
******************************************************************/
#define SH5001_ACT_X_INT_EN		(0x40U)
#define SH5001_ACT_X_INT_DIS	(0x00U)
#define SH5001_ACT_Y_INT_EN		(0x20U)
#define SH5001_ACT_Y_INT_DIS	(0x00U)
#define SH5001_ACT_Z_INT_EN		(0x10U)
#define SH5001_ACT_Z_INT_DIS	(0x00U)

#define SH5001_INACT_X_INT_EN	(0x04U)
#define SH5001_INACT_X_INT_DIS	(0x00U)
#define SH5001_INACT_Y_INT_EN	(0x02U)
#define SH5001_INACT_Y_INT_DIS	(0x00U)
#define SH5001_INACT_Z_INT_EN	(0x01U)
#define SH5001_INACT_Z_INT_DIS	(0x00U)


#define SH5001_LINK_PRE_STA		(0x01U)
#define SH5001_LINK_PRE_STA_NO	(0x00U)

#define SH5001_ACT_INACT_CLR_STATUS1	(0x00U)
#define SH5001_ACT_INACT_CLR_STATUS3	(0x80U)

/******************************************************************
*	TAP Int Config Macro Definitions
******************************************************************/
#define SH5001_TAP_X_INT_EN		(0x08U)
#define SH5001_TAP_X_INT_DIS	(0x00U)
#define SH5001_TAP_Y_INT_EN		(0x04U)
#define SH5001_TAP_Y_INT_DIS	(0x00U)
#define SH5001_TAP_Z_INT_EN		(0x02U)
#define SH5001_TAP_Z_INT_DIS	(0x00U)


/******************************************************************
*	HIGHG Int Config Macro Definitions
******************************************************************/
#define SH5001_HIGHG_ALL_INT_EN		(0x80U)
#define SH5001_HIGHG_ALL_INT_DIS	(0x00U)
#define SH5001_HIGHG_X_INT_EN		(0x40U)
#define SH5001_HIGHG_X_INT_DIS		(0x00U)
#define SH5001_HIGHG_Y_INT_EN		(0x20U)
#define SH5001_HIGHG_Y_INT_DIS		(0x00U)
#define SH5001_HIGHG_Z_INT_EN		(0x10U)
#define SH5001_HIGHG_Z_INT_DIS		(0x00U)


/******************************************************************
*	LOWG Int Config Macro Definitions
******************************************************************/
#define SH5001_LOWG_ALL_INT_EN		(0x01U)
#define SH5001_LOWG_ALL_INT_DIS		(0x00U)


/******************************************************************
*	SPI Interface Config Macro Definitions
******************************************************************/
#define SH5001_SPI_3_WIRE      (0x20U)
#define SH5001_SPI_4_WIRE      (0x00U)


/******************************************************************
*	I2C Interface Config Macro Definitions
******************************************************************/
#define SH5001_I2C_DIS				(0x80U)
#define SH5001_I2C_EN				(0x00U)

//Jin
#define SH5001_I2C_LP_06BYTE		(0x04U)
#define SH5001_I2C_LP_12BYTE		(0x02U)
#define SH5001_I2C_LP_14BYTE		(0x01U)
#define SH5001_I2C_LP_DISABLE		(0x00U)

/******************************************************************
*	OIS SPI Interface Config Macro Definitions
******************************************************************/
//#define SH5001_OIS_SPI_3WIRE		(0x00U)
//#define SH5001_OIS_SPI_4WIRE		(0x40U)
#define SH5001_OIS_SPI_EN			(0x10U)
#define SH5001_OIS_SPI_DIS			(0x00U)

#define SH5001_ACC_OIS_ENABLE		(0x80U)
#define SH5001_ACC_OIS_DISABLE	(0x00U)

#define SH5001_ACC_OIS_RANGE_4G		(0x10U)
#define SH5001_ACC_OIS_RANGE_8G		(0x20U)
#define SH5001_ACC_OIS_RANGE_16G	(0x30U)

#define SH5001_ACC_OIS_ODRX008		(0x0CU)
#define SH5001_ACC_OIS_ODRX004		(0x08U)
#define SH5001_ACC_OIS_ODRX002		(0x04U)
#define SH5001_ACC_OIS_ODRX000		(0x00U)  //bypass


#define SH5001_ACC_OIS_ODR_1000HZ	(0x00U)  //TODO need to set ref datasheet
#define SH5001_ACC_OIS_ODR_2000HZ	(0x01U)
#define SH5001_ACC_OIS_ODR_4000HZ	(0x02U)
#define SH5001_ACC_OIS_ODR_8000HZ	(0x03U)




#define SH5001_GYRO_OIS_ENABLE		(0x80U)
#define SH5001_GYRO_OIS_DISABLE		(0x00U)

#define SH5001_GYRO_OIS_RANGE_31	(0x00U)
#define SH5001_GYRO_OIS_RANGE_62	(0x10U)
#define SH5001_GYRO_OIS_RANGE_125	(0x20U)
#define SH5001_GYRO_OIS_RANGE_250	(0x30U)
#define SH5001_GYRO_OIS_RANGE_500	(0x40U)
#define SH5001_GYRO_OIS_RANGE_1000	(0x50U)
#define SH5001_GYRO_OIS_RANGE_2000	(0x60U)
#define SH5001_GYRO_OIS_RANGE_4000	(0x70U)

#define SH5001_GYRO_OIS_ODRX008		(0x0CU)
#define SH5001_GYRO_OIS_ODRX004		(0x08U)
#define SH5001_GYRO_OIS_ODRX002		(0x04U)
#define SH5001_GYRO_OIS_ODRX000		(0x00U)  //bypass

#define SH5001_GYRO_OIS_ODR_1000HZ	(0x00U)
#define SH5001_GYRO_OIS_ODR_2000HZ	(0x01U)
#define SH5001_GYRO_OIS_ODR_4000HZ	(0x02U)
#define SH5001_GYRO_OIS_ODR_8000HZ	(0x03U)




/******************************************************************
*	FIFO Config Macro Definitions
******************************************************************/
#define SH5001_FIFO_MODE_DIS		(0x00U)
#define SH5001_FIFO_MODE_FIFO		(0x01U)
#define SH5001_FIFO_MODE_STREAM		(0x02U)
#define SH5001_FIFO_MODE_TRIGGER	(0x03U)

#define SH5001_FIFO_ACC_DOWNS_EN	(0x80U)
#define SH5001_FIFO_ACC_DOWNS_DIS	(0x00U)
#define SH5001_FIFO_GYRO_DOWNS_EN	(0x08U)
#define SH5001_FIFO_GYRO_DOWNS_DIS	(0x00U)

#define SH5001_FIFO_FREQ_X1_2		(0x00U)
#define SH5001_FIFO_FREQ_X1_4		(0x01U)
#define SH5001_FIFO_FREQ_X1_8		(0x02U)
#define SH5001_FIFO_FREQ_X1_16		(0x03U)
#define SH5001_FIFO_FREQ_X1_32		(0x04U)
#define SH5001_FIFO_FREQ_X1_64		(0x05U)
#define SH5001_FIFO_FREQ_X1_128		(0x06U)
#define SH5001_FIFO_FREQ_X1_256		(0x07U)

#define SH5001_FIFO_TIMESTAMP_EN	(0x4000U)
#define SH5001_FIFO_EXT_Z_EN		(0x2000U)
#define SH5001_FIFO_EXT_Y_EN		(0x1000U)
#define SH5001_FIFO_EXT_X_EN		(0x0080U)
#define SH5001_FIFO_TEMPERATURE_EN	(0x0040U)
#define SH5001_FIFO_GYRO_Z_EN		(0x0020U)
#define SH5001_FIFO_GYRO_Y_EN		(0x0010U)
#define SH5001_FIFO_GYRO_X_EN		(0x0008U)
#define SH5001_FIFO_ACC_Z_EN		(0x0004U)
#define SH5001_FIFO_ACC_Y_EN		(0x0002U)
#define SH5001_FIFO_ACC_X_EN		(0x0001U)
#define SH5001_FIFO_ALL_DIS			(0x0000U)

/******************************************************************
*	AUX I2C Config Macro Definitions
******************************************************************/
#define SH5001_MI2C_NORMAL_MODE			(0x00U)
#define SH5001_MI2C_BYPASS_MODE			(0x01U)

#define SH5001_MI2C_READ_ODR_200HZ	    (0x00U)
#define SH5001_MI2C_READ_ODR_100HZ	    (0x10U)
#define SH5001_MI2C_READ_ODR_50HZ		(0x20U)
#define SH5001_MI2C_READ_ODR_25HZ		(0x30U)

#define SH5001_MI2C_FAIL				(0x20U)
#define SH5001_MI2C_SUCCESS				(0x10U)

#define SH5001_MI2C_READ_MODE_AUTO		(0x40U)
#define SH5001_MI2C_READ_MODE_MANUAL	(0x00U)


/******************************************************************
*	Fast offset compensation Macro Definitions
******************************************************************/
#define SH5001_X_UP     (0x00U)
#define SH5001_X_DOWN   (0x01U)
#define SH5001_Y_UP     (0x02U)
#define SH5001_Y_DOWN   (0x03U)
#define SH5001_Z_UP     (0x04U)
#define SH5001_Z_DOWN   (0x05U)


/******************************************************************
*	Other Macro Definitions
******************************************************************/
#define SH5001_TRUE		(0U)
#define SH5001_FALSE	(1U)

#define SH5001_CALI_REG_NUM 8
#define SH5001_POWMODE_REG_NUM_O1 11
#define SH5001_POWMODE_REG_NUM_O2 14
#define SH5001_NORMAL_MODE		(0x00)
#define SH5001_ACC_HP_MODE		(0x01)
#define SH5001_ACC_LP_MODE		(0x02)
#define SH5001_POWERDOWN_MODE	(0x03)

// FIFO Mode DIV = FIFO channel * 2;
#define SH5001_WATERMARK_DIV    (14U)
#define SH5001_FIFO_BUFFER      (1024U)

#define SH5001A_FIFO_LENGTH SH5001_WATERMARK_DIV * 10

int sh5001a_probe(struct device *dev, struct regmap *regmap);
int sh5001a_remove(struct device *dev);
void sh5001a_shutdown(struct device *dev);


#endif

