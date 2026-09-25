#include <stdio.h>
#include "gvd_log.h"
#include "gvd_output.h"
#include "gvd_config.h"
#include "demo_console.h"
#include "demo_uart_owner.h"
#include "app_demo_config.h"

/* Preserve debugger symbols while keeping UART work in its own task. */
volatile ID g_cpu1_gvd_log_task_id;
volatile uint32_t g_cpu1_demo_risk_log_count;
volatile uint32_t g_cpu1_gvd_log_drop_count;
volatile uint32_t g_cpu1_gvd_log_coalesced_count;
volatile uint32_t g_cpu1_gvd_sync_log_count;
volatile uint32_t g_cpu1_gvd_boot_log_count;

static uint32_t now_ms(uint32_t *out)
{
    SYSTIM t;
    if (tk_get_otm(&t) < E_OK) { return 0U; }
    *out = t.lo;
    return 1U;
}

/* Presentation only: do not reclassify the input or change the LED decision.
 * Derive the estimated hazard side from the FINAL cue, so both labels use
 * the same coordinate mapping when GVD_CUE_SWAP_LR is enabled. The raw
 * activity values printed as imgL/imgR remain in image coordinates.
 * Keep the console labels ASCII. "GVS direction" is a logical cue only;
 * it does not control electrical stimulation.
 */
static const char *guidance_text(const gvd_decision_t *decision)
{
    if (decision->reason == GVD_ACTIVITY) {
        if (decision->cue == GVD_CUE_LEFT) {
            return "danger=RIGHT GVS direction=to LEFT";
        }
        if (decision->cue == GVD_CUE_RIGHT) {
            return "danger=LEFT GVS direction=to RIGHT";
        }
    }
    return "danger=UNKNOWN GVS direction=NONE";
}

static void gvd_log_task_entry(INT stacd, void *exinf)
{
    (void) stacd; (void) exinf;
    char line[GVD_LOG_BUFFER_BYTES];
    gvd_event_t event = {0};
    uint32_t length = 0U, offset = 0U, started = 0U;
    uint32_t last_log_ms = 0U, last_event = 0U, log_started = 0U;
    uint32_t sync_consumed = 0U, resync_line = 0U;
    uint32_t boot_pending = 1U, boot_line = 0U;
    while (1) {
        uint32_t at;
        if (!now_ms(&at)) { tk_dly_tsk(1); continue; }
#if !DEMO_UART_OWNER_CPU1
        tk_dly_tsk(GVD_OUTPUT_POLL_MS);
        continue;
#endif
        if (!length) {
            if (resync_line) {
                line[0] = '\r'; line[1] = '\n'; length = 2U;
            } else if (boot_pending) {
                /* Print before guidance/SYNC even if no input frame has arrived.
                 * Use the same bounded UART service as all other log lines.
                 */
                int count = snprintf(line, sizeof(line),
                    "[BOOT] input=%s display=%s uart=CPU1\r\n",
                    APP_VIDEO_SOURCE_CAMERA ? "CAMERA" : "USB",
                    APP_ENABLE_DEBUG_DISPLAY ? "ON" : "OFF");
                boot_pending = 0U;
                if (count < 0 || (uint32_t) count >= sizeof(line)) {
                    g_cpu1_gvd_log_drop_count++; tk_dly_tsk(1); continue;
                }
                length = (uint32_t) count;
                boot_line = 1U;
            } else {
                if (!sync_consumed) {
                    /* Gate logs only: LED/IPC tasks continue while we wait.
                     * Never emit the startup WAITING event before SYNC.
                     */
                    if (!gvd_output_read_sync_event(&event)) {
                        tk_dly_tsk(1); continue;
                    }
                } else {
                    if ((log_started && at - last_log_ms < GVD_LOG_INTERVAL_MS) ||
                        !gvd_output_read_event(&event) || event.event == last_event ||
                        (last_event && event.event - last_event >= 0x80000000U)) {
                        tk_dly_tsk(1); continue;
                    }
                    if (last_event && event.event > last_event + 1U) {
                        g_cpu1_gvd_log_coalesced_count += event.event - last_event - 1U;
                    }
                }
                int count;
                if (event.sync) {
                    count = snprintf(line, sizeof(line),
                        "[SYNC] event=%lu seq=%lu t=%lu pattern=ALL_2\r\n",
                        (unsigned long) event.event, (unsigned long) event.seq,
                        (unsigned long) event.at_ms);
                } else {
                    count = snprintf(line, sizeof(line),
                        "[GUIDANCE] event=%lu seq=%lu %s | risk=%s imgL=%u imgR=%u t=%lu reason=%s objects=%u sem=%lu swap=%u\r\n",
                        (unsigned long) event.event, (unsigned long) event.seq,
                        guidance_text(&event.decision), gvd_risk_name(event.decision.risk),
                        (unsigned int) event.left, (unsigned int) event.right,
                        (unsigned long) event.at_ms, gvd_reason_name(event.decision.reason),
                        (unsigned int) event.objects, (unsigned long) event.semantic_frame,
                        (unsigned int) (GVD_CUE_SWAP_LR != 0U));
                }
                if (count < 0 || (uint32_t) count >= sizeof(line)) {
                    g_cpu1_gvd_log_drop_count++; tk_dly_tsk(1); continue;
                }
                length = (uint32_t) count;
                last_event = event.event;
                last_log_ms = at;
                log_started = 1U;
            }
            offset = 0U;
            started = at;
        }

        /* Poll only within a fixed CPU budget, with interrupts enabled.
         * Both IPC and LED tasks preempt this task. No printf/DI/wait in ISR.
         * A short sleep bounds background CPU use even on a stuck UART. */
        for (uint32_t polls = 0U; polls < GVD_UART_SERVICE_POLLS && offset < length; polls++) {
            offset += demo_console_try_write(&line[offset], length - offset);
        }
        if (offset == length) {
            if (resync_line) { resync_line = 0U; }
            else if (boot_line) {
                g_cpu1_gvd_boot_log_count++;
                boot_line = 0U;
            }
            else if (event.sync) {
                sync_consumed = 1U; /* Retry SYNC after a bounded UART timeout. */
                g_cpu1_gvd_sync_log_count++;
            }
            else { g_cpu1_demo_risk_log_count++; }
            length = 0U;
        } else if (at - started >= GVD_UART_LINE_TIMEOUT_MS) {
            g_cpu1_gvd_log_drop_count++;
            length = 0U;
            boot_line = 0U;
            resync_line = 1U; /* Terminate a partial line before the next record. */
        }
        tk_dly_tsk(1);
    }
}

ER gvd_log_create(void)
{
    const T_CTSK ctsk =
    {
        .tskatr  = TA_HLNG,
        .task    = gvd_log_task_entry,
        .itskpri = GVD_LOG_PRIORITY,
        .stksz   = GVD_TASK_STACK_BYTES
    };

    g_cpu1_gvd_log_task_id = tk_cre_tsk(&ctsk);
    if (g_cpu1_gvd_log_task_id < E_OK)
    {
        g_cpu1_gvd_start_error = (ER) g_cpu1_gvd_log_task_id;
        return g_cpu1_gvd_start_error;
    }

    return E_OK;
}

ER gvd_log_start(void)
{
    if (g_cpu1_gvd_log_task_id <= 0)
    {
        g_cpu1_gvd_start_error = E_ID;
        return g_cpu1_gvd_start_error;
    }

    g_cpu1_gvd_start_error = tk_sta_tsk(g_cpu1_gvd_log_task_id, 0);
    return g_cpu1_gvd_start_error;
}
