#include "hal_data.h"
#include "benchmark_clock.h"


volatile fsp_err_t
    g_benchmark_clock_open_err =
        FSP_SUCCESS;

volatile fsp_err_t
    g_benchmark_clock_start_err =
        FSP_SUCCESS;

volatile fsp_err_t
    g_benchmark_clock_info_err =
        FSP_SUCCESS;

volatile uint32_t
    g_benchmark_clock_hz =
        0U;


fsp_err_t benchmark_clock_start(void)
{
    timer_info_t info =
    {
        0
    };


    g_benchmark_clock_open_err =
        R_GPT_Open(
            &g_benchmark_timer_ctrl,
            &g_benchmark_timer_cfg
        );


    if (FSP_SUCCESS !=
        g_benchmark_clock_open_err)
    {
        return
            g_benchmark_clock_open_err;
    }


    g_benchmark_clock_info_err =
        R_GPT_InfoGet(
            &g_benchmark_timer_ctrl,
            &info
        );


    if (FSP_SUCCESS !=
        g_benchmark_clock_info_err)
    {
        return
            g_benchmark_clock_info_err;
    }


    /*
     * Actual counter frequency after the configured divider.
     * Do not hard-code the GPT frequency.
     */
    g_benchmark_clock_hz =
        info.clock_frequency;


    g_benchmark_clock_start_err =
        R_GPT_Start(
            &g_benchmark_timer_ctrl
        );


    return
        g_benchmark_clock_start_err;
}