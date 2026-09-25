#include "app_main.h"

#include "ipc_test.h"
#include "gvd_output.h"
#include "gvd_log.h"

volatile UW g_cpu1_heartbeat = 0U;

EXPORT INT usermain(void)
{
    ER err;

    /* Preserve the startup order: IPC -> GVD decision/LED -> console.
     * An IPC hardware-open failure skips all three, as before.
     */
    if (ipc_test_init() == FSP_SUCCESS)
    {
        err = ipc_test_create();
        if (err >= E_OK)
        {
            (void) ipc_test_start();
        }

        /* As before, an IPC task error does not suppress GVD diagnostics. */
        err = gvd_output_create();
        if (err >= E_OK)
        {
            err = gvd_output_start();
        }
        if (err >= E_OK)
        {
            err = gvd_log_create();
        }
        if (err >= E_OK)
        {
            (void) gvd_log_start();
        }
    }

    /* usermain runs in the kernel's initial task; keep its heartbeat. */
    while (1)
    {
        g_cpu1_heartbeat++;
        tk_dly_tsk(100);
    }

    return 0;
}
