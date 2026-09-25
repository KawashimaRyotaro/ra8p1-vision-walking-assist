#ifndef CAMERA_CAPTURE_H
#define CAMERA_CAPTURE_H

#include <stdint.h>
#include <stddef.h>
#include <tk/tkernel.h>

#include "hal_data.h"


extern volatile uint32_t g_camera_vin_frame_count;
extern volatile uintptr_t g_camera_vin_last_buffer;
extern volatile uint32_t g_camera_vin_error_count;
extern volatile uint32_t g_camera_vin_checksum;
extern volatile fsp_err_t g_camera_vin_open_err;
extern volatile fsp_err_t g_camera_vin_start_err;
extern volatile fsp_err_t g_camera_vin_stream_err;
extern volatile uint32_t g_camera_vin_probe_stage;

extern volatile uint32_t g_camera_mipi_event_count;
extern volatile uint32_t g_camera_mipi_last_event;

extern volatile uint32_t g_camera_vin_callback_count;
extern volatile uint32_t g_camera_vin_last_interrupt_status;
extern volatile uint32_t g_camera_vin_last_event_status;

extern volatile uint32_t g_camera_vin_buf1_before;
extern volatile uint32_t g_camera_vin_buf1_after;

extern volatile uint32_t g_camera_vin_buf2_before;
extern volatile uint32_t g_camera_vin_buf2_after;

extern volatile uint32_t g_camera_vin_buf3_before;
extern volatile uint32_t g_camera_vin_buf3_after;

extern volatile uint32_t g_camera_vin_buffer_write_detected;

extern volatile uint32_t g_camera_hw_phy_pwrsen;
extern volatile uint32_t g_camera_hw_phy_dphyen;
extern volatile uint32_t g_camera_hw_phy_status;

extern volatile uint32_t g_camera_hw_csi_rxen;
extern volatile uint32_t g_camera_hw_csi_rxst;
extern volatile uint32_t g_camera_hw_csi_ractdet;

extern volatile uint32_t g_camera_hw_vin_me;
extern volatile uint32_t g_camera_hw_vin_cc;
extern volatile uint32_t g_camera_hw_vin_ms;
extern volatile uint32_t g_camera_hw_vin_ca;
extern volatile uint32_t g_camera_hw_vin_lc;

extern volatile fsp_err_t g_camera_xclk_info_err;
extern volatile uint32_t g_camera_xclk_clock_hz;
extern volatile uint32_t g_camera_xclk_period_counts;
extern volatile uint32_t g_camera_xclk_output_hz;

extern volatile fsp_err_t g_camera_sensor_readback_err;
extern volatile uint32_t g_camera_sensor_readback_fail_reg;

extern volatile uint32_t g_camera_sensor_reg_3008;
extern volatile uint32_t g_camera_sensor_reg_4202;
extern volatile uint32_t g_camera_sensor_reg_300e;
extern volatile uint32_t g_camera_sensor_reg_3035;
extern volatile uint32_t g_camera_sensor_reg_3036;
extern volatile uint32_t g_camera_sensor_reg_3037;
extern volatile uint32_t g_camera_sensor_reg_4814;

extern volatile uint32_t g_camera_reset_pin_level;

extern volatile uint32_t g_camera_gpt12_gtcr;
extern volatile uint32_t g_camera_gpt12_gtior;
extern volatile uint32_t g_camera_gpt12_gtpr;
extern volatile uint32_t g_camera_p501_pfs;


extern volatile uint32_t g_camera_source_irq_count;
extern volatile uint32_t g_camera_source_publish_count;
extern volatile uint32_t g_camera_source_drop_count;
extern volatile uint32_t g_camera_source_copy_count;

extern volatile uint32_t g_camera_source_last_frame_index;
extern volatile uintptr_t g_camera_source_last_vin_buffer;
extern volatile uint32_t g_camera_source_last_cache_slot;


ER camera_source_create(void);
ER camera_source_start(void);

fsp_err_t camera_capture_start_live(void);
fsp_err_t camera_capture_stop_live(void);


fsp_err_t camera_capture_probe(void);


#endif /* CAMERA_CAPTURE_H */