#include "demo_uart_owner.h"

#if !DEMO_UART_OWNER_CPU1
/* The original BSP file is excluded as a separate Debug compilation unit. */
#include "../mtk3_bsp2/sysdepend/ra_fsp/lib/libtm/ek_ra8p1/tm_com.c"
#else
#include <tk/tkernel.h>
#include <mtkernel/lib/libtm/libtm.h>

#if !USE_TMONITOR || !TM_COM_SERIAL_DEV
#error "Keep the existing serial T-Monitor configuration; select owner in demo_uart_owner.h"
#endif

/* CPU0 must neither reinitialize nor transmit on CPU1's SCI8. */
EXPORT void tm_com_init(void) {}
EXPORT void tm_snd_dat(const UB *buf, INT size)
{
    (void) buf;
    (void) size;
}
EXPORT void tm_rcv_dat(UB *buf, INT size)
{
    while (size-- > 0) { *buf++ = 0U; }
}
#endif
