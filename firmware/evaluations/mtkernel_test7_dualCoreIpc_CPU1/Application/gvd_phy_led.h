#ifndef GVD_PHY_LED_H
#define GVD_PHY_LED_H

#include <stdint.h>
#include "gvd_policy.h"

/* CPU1 only. LED6 = evade LEFT, LED7 = evade RIGHT. */
uint32_t gvd_phy_led_init(void);
void gvd_phy_led_set(gvd_cue_t cue);

extern volatile uint32_t g_cpu1_led67_ready;
extern volatile uint32_t g_cpu1_led67_error;
extern volatile uint32_t g_cpu1_led67_phy_address;
extern volatile uint32_t g_cpu1_led67_mask;

#endif
