// SPDX-License-Identifier: GPL-2.0

enum pwm_pre_div {
	PWM_PRE_DIV_1 = 0,
	PWM_PRE_DIV_2,
	PWM_PRE_DIV_4,
	PWM_PRE_DIV_8,
	PWM_PRE_DIV_16,
	PWM_PRE_DIV_32,
	PWM_PRE_DIV_64,
	PWM_PRE_DIV_128,
	PWM_PRE_DIV_256,
};

enum pwm_align {
	PWM_ALIGN_RIGHT = 0,
	PWM_ALIGN_LEFT,
	PWM_ALIGN_MIDDLE,
};

typedef enum {
	PWM_CONTROLLER_0 = 0,
	PWM_CONTROLLER_1,
} pwm_controller_index;

typedef enum {
	PWM_CHN_0 = 0,
	PWM_CHN_1,
	PWM_CHN_2,
	PWM_CHN_3,
	PWM_CHN_4,
	PWM_CHN_5,
	PWM_CHN_6,
	PWM_CHN_7,
	PWM_CHN_8,
	PWM_CHN_9,
	PWM_CHN_10,
	PWM_CHN_11,
	PWM_CHN_12,
	PWM_CHN_13,
	PWM_CHN_14,
	PWM_CHN_15,
} pwm_chn_index;

enum pwm_polarity {
	PWM_POLARITY_NORMAL,
	PWM_POLARITY_INVERSED,
};

struct pwm_args {
	unsigned int period;
	enum pwm_polarity polarity;
};

enum {
	PWMF_REQUESTED = 1 << 0,
	PWMF_EXPORTED = 1 << 1,
};

struct pwm_state {
	unsigned int period;
	unsigned int duty_cycle;
	unsigned int duty_cycle1;
	unsigned int duty_cycle2;
	enum pwm_polarity polarity;
	bool enabled;
};

int bsp_pwm_apply(pwm_controller_index controller_index, pwm_chn_index chn_index,
				struct pwm_state *state);
int bsp_pwm_init(pwm_controller_index controller_index);
int bsp_pwm_deinit(pwm_controller_index controller_index);
