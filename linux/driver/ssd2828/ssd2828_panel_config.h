#define DEFAULT_PANEL 2
#define PANEL_ID_MAX_COMMANDS	3

struct panel_detect_params {
	uint32_t retry_cnt;
	uint8_t address[PANEL_ID_MAX_COMMANDS];
	uint8_t expected[PANEL_ID_MAX_COMMANDS];
	mipi_send_initcode_cb_t mipi_send_initcode;
};

extern struct panel_detect_params panel_list[];
extern uint8_t get_panel_list_size(void);

