#ifndef CAMERA_CONTROL_H
#define CAMERA_CONTROL_H

#include <stdint.h>

#include "hal_data.h"


extern volatile fsp_err_t
    g_camera_i2c_init_err;

extern volatile fsp_err_t
    g_camera_switch_err;

extern volatile fsp_err_t
    g_camera_open_err;

extern volatile uint32_t
    g_camera_control_ready;


fsp_err_t camera_control_init(void);


#endif /* CAMERA_CONTROL_H */