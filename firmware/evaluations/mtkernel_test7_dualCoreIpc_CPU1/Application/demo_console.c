/* SCI8 configuration/register definitions derived from the existing BSP2
 * sysdepend/ra_fsp/lib/libtm/ek_ra8p1/tm_com.c:
 * Copyright (C) 2026 by Ken Sakamura. T-License 2.2.
 * This application-owned adapter replaces that compilation unit in Debug.
 * Same UART/pins/baud; no FSP or kernel changes. CPU0 owns the pin mux.
 */
#include <tk/tkernel.h>
#include <mtkernel/lib/libtm/libtm.h>
#include "bsp_api.h"
#include "demo_console.h"
#include "demo_uart_owner.h"
#include "gvd_config.h"

#if !USE_TMONITOR || !TM_COM_SERIAL_DEV
#error "Keep the existing serial T-Monitor configuration; select owner in demo_uart_owner.h"
#endif

#define SCI8_BASE   (0x40358800U)
#define SCI8_RDR    (SCI8_BASE + 0x00U)
#define SCI8_TDR    (SCI8_BASE + 0x04U)
#define SCI8_CCR0   (SCI8_BASE + 0x08U)
#define SCI8_CCR1   (SCI8_BASE + 0x0CU)
#define SCI8_CCR2   (SCI8_BASE + 0x10U)
#define SCI8_CSR    (SCI8_BASE + 0x48U)
#define CSR_TDRE    (1UL << 29)
#define CSR_RDRF    (1UL << 31)
#define MSTPCRB     (0x40203004U)

volatile uint32_t g_cpu1_console_timeout_count;

uint32_t demo_console_try_write(const char *data, uint32_t bytes)
{
    uint32_t written = 0U;
#if DEMO_UART_OWNER_CPU1
    /* Bounded work; never spin waiting for the UART. */
    while (written < bytes && written < GVD_UART_CHUNK_BYTES &&
           (in_w(SCI8_CSR) & CSR_TDRE)) {
        out_b(SCI8_TDR, (UB) data[written]);
        written++;
    }
#else
    (void) data;
    (void) bytes;
#endif
    return written;
}

EXPORT void tm_com_init(void)
{
#if DEMO_UART_OWNER_CPU1
    out_w(MSTPCRB, in_w(MSTPCRB) & ~(1UL << 23));
    out_w(SCI8_CCR0, 0U);
    out_w(SCI8_CCR1, 0x00000010U);
    out_w(SCI8_CCR2, 0x80004010U); /* Existing BSP: 115200 baud */
    out_w(SCI8_CCR0, (1UL << 4) | (1UL << 0));
#endif
}

EXPORT void tm_snd_dat(const UB *buf, INT size)
{
#if DEMO_UART_OWNER_CPU1
    /* Kernel startup/exception messages only. GVD uses try_write instead.
     * Bound the entire call even when callers have disabled interrupts. */
    uint32_t budget = GVD_UART_BOOT_POLL_LIMIT;
    while (size > 0 && budget > 0U) {
        budget--;
        if (in_w(SCI8_CSR) & CSR_TDRE) {
            out_b(SCI8_TDR, *buf++);
            size--;
        }
    }
    if (size > 0) { g_cpu1_console_timeout_count++; }
#else
    (void) buf;
    (void) size;
#endif
}

EXPORT void tm_rcv_dat(UB *buf, INT size)
{
    /* No interactive input is used by this demo (USB selects SELECT.TXT). */
    while (size-- > 0) {
        *buf = 0U;
#if DEMO_UART_OWNER_CPU1
        if (in_w(SCI8_CSR) & CSR_RDRF) { *buf = in_b(SCI8_RDR); }
#endif
        buf++;
    }
}
