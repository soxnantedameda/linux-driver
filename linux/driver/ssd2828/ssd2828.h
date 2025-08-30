#include <linux/types.h>
#include <linux/mutex.h>

struct spi_device;

#define MIPI_READ_FAIL 0X01
#define MIPI_READ_SUCCEED 0X00

#define LP 0
#define HS 1
#define VD 2

#define NON_BURST_PULSE (0x00 << 2) //Non burst mode with sync pulses
#define NON_BURST_EVENT (0x01 << 2) //Non burst mode with sync events
#define BURST (0x02 << 2) //Burst mode

#define VS_P_LOW (0 << 7)
#define VS_P_HIGH (1 << 7)

#define HS_P_LOW (0 << 6)
#define HS_P_HIGH (1 << 6)

#define PCLK_P_FALLING (0 << 5)
#define PCLK_P_RISING (1 << 5)

#define CBM_0 (0 << 0)
#define CBM_1 (1 << 0)  //no blanking packet

#define NVB_0 (0 << 7)
#define NVB_1 (1 << 7)
#define NVD_HS (0 << 6)
#define NVD_LP (1 << 6)

/* 0xB7 */
//Transmit Disable
#define TXD_0 (0 << 3) //0 – Transmit on
#define TXD_1 (1 << 3) //1 – Transmit halt
//Long Packet Enable
#define LPE_0 (0 << 2) //0 – Short Packet
#define LPE_1 (1 << 2) //1 – Long Packet
//EOT Packet Enable
#define EOT_0 (0 << 1) //0 – Do not send
#define EOT_1 (1 << 1) //1 – Send
//ECC CRC Check Disable
#define ECD_0 (0 << 0) //0 – Enable
#define ECD_1 (1 << 0) //1 – Disable
//Read Enable
#define REN_0 (0 << 7) // Write operation
#define REN_1 (1 << 7) // Read operation
//DCS Enable
#define DCS_0 (0 << 6) //0 – Generic packet
#define DCS_1 (1 << 6) //1 – DCS packet
//Clock Source Select
#define CSS_0 (0 << 5) //0– The clock source is tx_clk
#define CSS_1 (1 << 5) //1 – The clock source is pclk
//HS Clock Disable
#define HCLK_0 (0 << 4) //0 – HS clock is enabled
#define HCLK_1 (1 << 4) //1 – HS clock is disabled
//Video Mode Enable
#define VEN_0 (0 << 3) //0 – Video mode is disabled
#define VEN_1 (1 << 3) //1 – Video mode is enabled
//Sleep Mode Enable
#define SLP_0 (0 << 2) //0 – Sleep mode is disabled
#define SLP_1 (1 << 2) //1 – Sleep mode is enabled.
//Clock Lane Enable
#define CKE_0 (0 << 1) //0 – Clock lane will enter LP mode
#define CKE_1 (1 << 1) //– Clock lane will enter HS mode for all the cases.
//HS Mode
#define HS_0 (0 << 0) //0 – LP mode
#define HS_1 (1 << 0) //1 – HS mode

#define MIPI_VIDEO_1LANE 0X15
#define MIPI_VIDEO_2LANE 0X16
#define MIPI_VIDEO_3LANE 0X17
#define MIPI_VIDEO_4LANE 0X02
#define MIPI_VIDEO_8LANE 0X03

#define MIPI_COMMAND_1LANE 0x10
#define MIPI_COMMAND_2LANE 0x11
#define MIPI_COMMAND_3LANE 0x12
#define MIPI_COMMAND_4LANE 0x13
#define MIPI_COMMAND_8LANE 0x14

typedef void (*mipi_send_initcode_cb_t)(struct spi_device *spi);

typedef enum {
	FALLING_EDGE = 0x00,
	RISING_EDGE  = 0x01,
} RGBEdge_TypeDef;

typedef enum {
	ACTIVE_LOW  = 0X00,
	ACTIVE_HIGH = 0X01,
} RGBPolarity_TypeDef;

struct ssd2828_data {
	uint8_t IFMODE;
	uint8_t VIDEO_MODE;

	uint32_t BPP;
	uint32_t SOURCE_CLK;
	uint32_t PCLK;
	uint32_t LP_CLK;
	uint32_t XSIZE;
	uint32_t YSIZE;
	uint32_t HBPD;
	uint32_t HFPD;
	uint32_t HSPW;
	uint32_t VBPD;
	uint32_t VFPD;
	uint32_t VSPW;
	RGBEdge_TypeDef PCLK_EDGE;
	RGBPolarity_TypeDef HS_POLARITY;
	RGBPolarity_TypeDef VS_POLARITY;
	RGBPolarity_TypeDef DE_POLARITY;

	struct gpio_desc *ssd2828_enable_gpio;
	struct gpio_desc *ssd2828_reset_gpio;
	struct gpio_desc *panel_enable_gpio;
	struct gpio_desc *panel_reset_gpio;

	int ssd2828_enable;
	int ssd2828_reset;
	int panel_enable;
	int panel_reset;
	uint8_t speedmode;

	struct {
		uint32_t reset;
		uint32_t prepare;
		uint32_t enable;
		uint32_t disable;
		uint32_t unprepare;
	} delay;
	bool sleeped;
	bool prepared;
	bool enabled;
	struct mutex bus_lock;

	struct backlight_device *backlight;
	mipi_send_initcode_cb_t mipi_send_initcode_cb;
};

uint32_t ssd2828_dcs_read(struct spi_device *spi, uint8_t reg, uint16_t len, uint8_t *p);
uint32_t ssd2828_generic_read(struct spi_device *spi, uint8_t reg, uint16_t len, uint8_t *p);
void ssd2828_mipi_write(struct spi_device *spi, uint8_t DT, int data, ...);
#define MIPI_WR(ssd2828, DT, data, ...) ssd2828_mipi_write(ssd2828, DT, data, ##__VA_ARGS__, -1)

