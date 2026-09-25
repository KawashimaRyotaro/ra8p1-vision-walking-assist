#include <stdint.h>

#include <tm/tmonitor.h>

#include "camera_control.h"
#include "camera_sensor.h"
#include "i2c_control.h"
#include "switch_init.h"


#define CAMERA_MIPI_SWITCH_PIN    (6U)


volatile fsp_err_t
    g_camera_i2c_init_err =
        FSP_SUCCESS;

volatile fsp_err_t
    g_camera_switch_err =
        FSP_SUCCESS;

volatile fsp_err_t
    g_camera_open_err =
        FSP_SUCCESS;

volatile uint32_t
    g_camera_control_ready =
        0U;


fsp_err_t camera_control_init(void)
{
    g_camera_control_ready =
        0U;


    tm_putstring(
        (UB *) "[Camera] control init start.\n"
    );


    g_camera_i2c_init_err =
        i2c_control_init();


    if (FSP_SUCCESS !=
        g_camera_i2c_init_err)
    {
        tm_printf(
            (UB *) "[Camera] I2C init failed: %d\n",
            g_camera_i2c_init_err
        );

        return
            g_camera_i2c_init_err;
    }


    /*
     * Route the board camera connector to the
     * MIPI CSI path.
     */
    g_camera_switch_err =
        set_switch_state(
            CAMERA_MIPI_SWITCH_PIN,
            HIGH_STATE
        );


    if (FSP_SUCCESS !=
        g_camera_switch_err)
    {
        tm_printf(
            (UB *) "[Camera] MIPI switch failed: %d\n",
            g_camera_switch_err
        );

        return
            g_camera_switch_err;
    }


    /*
     * camera_open() restores the I2C slave address
     * from the board switch (0x43) to the OV5640
     * address (0x3C), starts GPT12 XCLK, resets the
     * sensor and writes the OV5640 configuration.
     */
    g_camera_open_err =
        camera_open();


    if (FSP_SUCCESS !=
        g_camera_open_err)
    {
        tm_printf(
            (UB *) "[Camera] OV5640 init failed: %d\n",
            g_camera_open_err
        );

        return
            g_camera_open_err;
    }


    g_camera_control_ready =
        1U;


    tm_putstring(
        (UB *) "[Camera] OV5640 control ready.\n"
    );


    return
        FSP_SUCCESS;
}