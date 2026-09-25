/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_gpt.h"
#include "r_timer_api.h"
#include "r_ipc.h"
#include "r_dmac.h"
#include "r_transfer_api.h"
#include "r_ospi_b.h"
#include "r_spi_flash_api.h"
#include "r_iic_master.h"
#include "r_i2c_master_api.h"
#include "r_adc_b.h"
#include "r_adc_api.h"
FSP_HEADER
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer_periodic;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer_periodic_ctrl;
extern const timer_cfg_t g_timer_periodic_cfg;

#ifndef NULL
void NULL(timer_callback_args_t *p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_benchmark_timer;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_benchmark_timer_ctrl;
extern const timer_cfg_t g_benchmark_timer_cfg;

#ifndef NULL
void NULL(timer_callback_args_t *p_args);
#endif
/** IPC Instance. */
extern const ipc_instance_t g_ipc1;

/** Access the IPC instance using these structures when calling API functions directly
 (::p_api is not used). */
extern ipc_instance_ctrl_t g_ipc1_ctrl;
extern const ipc_cfg_t g_ipc1_cfg;

#ifndef NULL
void NULL(ipc_callback_args_t *p_args);
#endif
/** IPC Instance. */
extern const ipc_instance_t g_ipc0;

/** Access the IPC instance using these structures when calling API functions directly
 (::p_api is not used). */
extern ipc_instance_ctrl_t g_ipc0_ctrl;
extern const ipc_cfg_t g_ipc0_cfg;

#ifndef ipc0_callback
void ipc0_callback(ipc_callback_args_t *p_args);
#endif
/* Transfer on DMAC Instance. */
extern const transfer_instance_t g_transfer0;

/** Access the DMAC instance using these structures when calling API functions directly (::p_api is not used). */
extern dmac_instance_ctrl_t g_transfer0_ctrl;
extern const transfer_cfg_t g_transfer0_cfg;

#ifndef NULL
void NULL(transfer_callback_args_t *p_args);
#endif
#if OSPI_B_CFG_DMAC_SUPPORT_ENABLE
    #include "r_dmac.h"
#endif
#if OSPI_CFG_DOTF_SUPPORT_ENABLE
    #include "r_sce_if.h"
#endif

extern const spi_flash_instance_t g_ospi;
extern ospi_b_instance_ctrl_t g_ospi_ctrl;
extern const spi_flash_cfg_t g_ospi_cfg;
/* I2C Master on IIC Instance. */
extern const i2c_master_instance_t g_i2c_master_for_peripheral;

/** Access the I2C Master instance using these structures when calling API functions directly (::p_api is not used). */
extern iic_master_instance_ctrl_t g_i2c_master_for_peripheral_ctrl;
extern const i2c_master_cfg_t g_i2c_master_for_peripheral_cfg;

#ifndef g_i2c_master_for_peripheral_callback
void g_i2c_master_for_peripheral_callback(i2c_master_callback_args_t *p_args);
#endif
/** ADC on ADC_B instance. */
extern const adc_instance_t g_adc0;

/** Access the ADC_B instance using these structures when calling API functions directly (::p_api is not used). */
extern adc_b_instance_ctrl_t g_adc0_ctrl;
extern const adc_cfg_t g_adc0_cfg;
extern const adc_b_scan_cfg_t g_adc0_scan_cfg;

#ifndef NULL
void NULL(adc_callback_args_t *p_args);
#endif
void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
