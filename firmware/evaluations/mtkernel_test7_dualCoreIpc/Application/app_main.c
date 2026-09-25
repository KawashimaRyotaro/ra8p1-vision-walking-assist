#include <tk/tkernel.h>
#include <tm/tmonitor.h>

#include "hal_data.h"
#include "app_demo_config.h"
#include "demo_uart_owner.h"
#include "benchmark_clock.h"
#include "camera_control.h"
#include "camera_capture.h"
#include "display_task.h"
#include "ipc_test.h"
#include "motion_task.h"
#include "npu_worker.h"
#include "usb_loader.h"
#include "video_source.h"

EXPORT INT usermain(void)
{
    ER err;

    /* Preserve the early IPC startup and its existing best-effort policy. */
    if (ipc_test_init() == FSP_SUCCESS)
    {
        err = ipc_test_create();
        if (err >= E_OK)
        {
            (void) ipc_test_start();
        }
    }

#if !DEMO_UART_OWNER_CPU1
    tm_printf((UB *)"[BOOT] input=%s display=%s uart=CPU0\n",
              APP_VIDEO_SOURCE_CAMERA ? "CAMERA" : "USB",
              APP_ENABLE_DEBUG_DISPLAY ? "ON" : "OFF");
#endif
    tm_putstring((UB *)"Application start.\n");

    g_hal_init();
    tm_putstring((UB *)"FSP HAL initialized.\n");

    fsp_err_t const benchmark_clock_err = benchmark_clock_start();
    if (FSP_SUCCESS != benchmark_clock_err)
    {
        tm_printf((UB *)"ERROR: benchmark_clock_start = %d\n",
                  benchmark_clock_err);
        goto error;
    }

    tm_printf((UB *)"[BenchmarkClock] GPT13 started. hz=%u\n",
              g_benchmark_clock_hz);

#if APP_VIDEO_SOURCE_CAMERA
    fsp_err_t const camera_control_err = camera_control_init();
    if (FSP_SUCCESS != camera_control_err)
    {
        tm_printf((UB *)"[Camera] control init failed: %d\n",
                  camera_control_err);
        goto error;
    }
#endif

    video_source_init();
    tm_putstring((UB *)"[Video] SDRAM frame source initialized.\n");

    /* Create dormant tasks in the established allocation order. */
#if APP_ENABLE_DEBUG_DISPLAY
    err = display_task_create();
    if (err < E_OK)
    {
        tm_printf((UB *)"ERROR: display_task_create = %d\n", err);
        goto error;
    }
#endif

    err = motion_task_create();
    if (err < E_OK)
    {
        tm_printf((UB *)"ERROR: motion_task_create = %d\n", err);
        goto error;
    }

#if APP_VIDEO_SOURCE_CAMERA
    err = camera_source_create();
    if (err < E_OK)
    {
        tm_printf((UB *)"ERROR: camera_source_create = %d\n", err);
        goto error;
    }
#else
    err = usb_loader_create();
    if (err < E_OK)
    {
        tm_printf((UB *)"ERROR: usb_loader_create = %d\n", err);
        goto error;
    }
#endif

    /* NPU still starts first; its startup failure does not stop other tasks. */
    err = npu_worker_create();
    if (err >= E_OK)
    {
        (void) npu_worker_start();
    }

    /* Start consumers before enabling the camera or USB producer. */
#if APP_ENABLE_DEBUG_DISPLAY
    err = display_task_start();
    if (err < E_OK)
    {
        tm_printf((UB *)"ERROR: display_task_start = %d\n", err);
        goto error;
    }
#endif

    err = motion_task_start();
    if (err < E_OK)
    {
        tm_printf((UB *)"ERROR: motion_task_start = %d\n", err);
        goto error;
    }

#if APP_VIDEO_SOURCE_CAMERA
    err = camera_source_start();
    if (err < E_OK)
    {
        tm_printf((UB *)"ERROR: camera_source_start = %d\n", err);
        goto error;
    }

    fsp_err_t const camera_live_err = camera_capture_start_live();
    if (FSP_SUCCESS != camera_live_err)
    {
        tm_printf((UB *)"ERROR: camera_capture_start_live = %d\n",
                  camera_live_err);
        goto error;
    }
#else
    err = usb_loader_start();
    if (err < E_OK)
    {
        tm_printf((UB *)"ERROR: usb_loader_start = %d\n", err);
        goto error;
    }
#endif

    tm_putstring((UB *)"All application tasks started.\n");
    tk_slp_tsk(TMO_FEVR);
    return 0;

error:
    tm_putstring((UB *)"Application initialization failed.\n");
    tk_slp_tsk(TMO_FEVR);
    return 0;
}
