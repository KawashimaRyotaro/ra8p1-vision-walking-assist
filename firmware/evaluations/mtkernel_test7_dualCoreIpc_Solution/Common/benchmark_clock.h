#ifndef BENCHMARK_CLOCK_H
#define BENCHMARK_CLOCK_H

#include <stdint.h>
#include <stddef.h>

#include "hal_data.h"


/*
 * GPT13 is reserved as the common dual-core benchmark clock.
 *
 * GPT12 is kept available for the camera clock used by the
 * existing camera project.
 */
#define BENCHMARK_CLOCK_GPT_CHANNEL    (13U)


/*
 * FSP calculates a GPT channel register base as:
 *
 *   GPT0 + channel * (GPT1 - GPT0)
 *
 * Use exactly the same mapping here so CPU0 and CPU1 can
 * read the same physical GPT13 GTCNT register.
 */
static inline R_GPT0_Type *
benchmark_clock_register(void)
{
    uintptr_t const base =
        (uintptr_t) R_GPT0;

    uintptr_t const stride =
        (uintptr_t) R_GPT1 -
        (uintptr_t) R_GPT0;

    return
        (R_GPT0_Type *)
        (
            base +
            ((uintptr_t)
             BENCHMARK_CLOCK_GPT_CHANNEL *
             stride)
        );
}


static inline uint32_t
benchmark_clock_now_ticks(void)
{
    return
        benchmark_clock_register()->GTCNT;
}


/*
 * CPU0 only.
 */
fsp_err_t benchmark_clock_start(void);


extern volatile fsp_err_t
    g_benchmark_clock_open_err;

extern volatile fsp_err_t
    g_benchmark_clock_start_err;

extern volatile fsp_err_t
    g_benchmark_clock_info_err;

extern volatile uint32_t
    g_benchmark_clock_hz;


#endif /* BENCHMARK_CLOCK_H */