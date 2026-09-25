#include <tk/tkernel.h>
#include "hal_data.h"
#include "gvd_output.h"
#include "gvd_policy.h"
#include "gvd_config.h"
#include "gvd_phy_led.h"

/* CPU1-local handoffs only. Each has a single writer. Short structure
 * copies use generation checks, never wait for another task or the UART.
 * All storage is static; no new shared-memory/IPC fields are introduced.
 */
typedef struct {
    perception_snapshot_t snapshot;
    uint32_t received_ms;
    uint32_t seen;
    uint32_t valid;
} gvd_input_t;

static gvd_input_t s_input;
static volatile uint32_t s_input_generation;
static gvd_event_t s_event;
static volatile uint32_t s_event_generation;
static gvd_event_t s_sync_event;
static volatile uint32_t s_sync_ready;

/* Preserve names used by the existing demo and debugger scripts. */
volatile ID g_cpu1_demo_task_id;
volatile uint32_t g_cpu1_demo_sync_count;
volatile fsp_err_t g_cpu1_demo_led_last_err;
volatile ER g_cpu1_gvd_start_error;
volatile uint32_t g_cpu1_gvd_duplicate_count;
volatile uint32_t g_cpu1_gvd_event_count;
/* Debugger-visible record is the same record consumed by LED/log paths. */
gvd_event_t g_cpu1_gvd_latest;

static uint32_t now_ms(uint32_t *out)
{
    SYSTIM t;
    if (tk_get_otm(&t) < E_OK) { return 0U; }
    *out = t.lo;
    return 1U;
}

void gvd_output_accept(const perception_snapshot_t *snapshot)
{
    uint32_t at;
    if (!now_ms(&at)) { gvd_output_invalidate(); return; }
    if (s_input.seen) {
        uint32_t seq_step = snapshot->sequence - s_input.snapshot.sequence;
        uint32_t frame_step = snapshot->temporal_frame_index -
                              s_input.snapshot.temporal_frame_index;
        /* Duplicate/replayed notifications cannot keep a stale cue alive.
         * Unsigned deltas also accept normal uint32 wraparound. */
        if (!seq_step || seq_step >= 0x80000000U ||
            !frame_step || frame_step >= 0x80000000U) {
            g_cpu1_gvd_duplicate_count++;
            return;
        }
    }
    s_input_generation++;
    __DMB();
    s_input.snapshot = *snapshot;
    s_input.received_ms = at;
    s_input.seen = 1U;
    s_input.valid = 1U;
    __DMB();
    s_input_generation++;
}

void gvd_output_invalidate(void)
{
    s_input_generation++;
    __DMB();
    s_input.valid = 0U;
    __DMB();
    s_input_generation++;
}

static uint32_t read_input(gvd_input_t *out, uint32_t *generation)
{
    uint32_t before = s_input_generation;
    if (before & 1U) { return 0U; }
    __DMB();
    gvd_input_t candidate = s_input;
    __DMB();
    if (before != s_input_generation) { return 0U; }
    *out = candidate;
    *generation = before;
    return 1U;
}

uint32_t gvd_output_read_event(gvd_event_t *out)
{
    uint32_t before = s_event_generation;
    if (before & 1U) { return 0U; }
    __DMB();
    *out = s_event;
    __DMB();
    return (before == s_event_generation && out->event != 0U);
}

/* The startup marker is published once and never overwritten. */
uint32_t gvd_output_read_sync_event(gvd_event_t *out)
{
    if (!s_sync_ready)
    {
        return 0U;
    }

    __DMB();
    *out = s_sync_event;
    return 1U;
}

static void leds_write(uint32_t mask)
{
    const bsp_io_port_pin_t pins[3] = {
        USER_LED_BLUE, USER_LED_GREEN, USER_LED_RED
    };
    fsp_err_t status = FSP_SUCCESS;
    /* Clear inactive LEDs first, then light the selected one. CPU0 already
     * configured these pins; CPU1 never reopens/reconfigures the GPIOs. */
    for (uint32_t pass = 0U; pass < 2U; pass++) {
        for (uint32_t i = 0U; i < 3U; i++) {
            uint32_t on = (mask >> i) & 1U;
            if (on != pass) { continue; }
            fsp_err_t err = R_IOPORT_PinWrite(&g_ioport_ctrl, pins[i],
                                  on ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
            if (err != FSP_SUCCESS) { status = err; }
        }
    }
    g_cpu1_demo_led_last_err = status;
}

static void publish_event(const gvd_input_t *input, gvd_decision_t decision,
                          uint32_t at, uint32_t sync)
{
    gvd_event_t event = {0};
    event.event = ++g_cpu1_gvd_event_count;
    if (!event.event) { event.event = ++g_cpu1_gvd_event_count; }
    event.seq = input->seen ? input->snapshot.sequence : 0U;
    event.at_ms = at;
    event.frame = input->seen ? input->snapshot.temporal_frame_index : UINT32_MAX;
    event.semantic_frame = input->seen ? input->snapshot.semantic_frame_index : UINT32_MAX;
    event.left = input->snapshot.activity_left;
    event.right = input->snapshot.activity_right;
    event.objects = input->snapshot.object_count;
    event.decision = decision;
    event.sync = sync;

    /* Apply the decision before exposing the SAME immutable event to logs. */
    uint32_t mask = sync ? 7U :
        (decision.risk == GVD_UNKNOWN ? 0U : (1U << ((uint32_t) decision.risk - 1U)));
    leds_write(mask);
    gvd_phy_led_set(sync ? GVD_CUE_NONE : decision.cue);
    g_cpu1_gvd_latest = event;
    if (sync) {
        s_sync_event = event;
        __DMB();
        s_sync_ready = 1U; /* Reserved: not overwritten by normal updates. */
    } else {
        s_event_generation++;
        __DMB();
        s_event = event;
        __DMB();
        s_event_generation++;
    }
}

static void gvd_output_task_entry(INT stacd, void *exinf)
{
    (void) stacd; (void) exinf;
    gvd_input_t input = {0};
    uint32_t generation = 0U, last_generation = UINT32_MAX;
    uint32_t sync_started = 0U, sync_done = 0U, sync_start_ms = 0U;
    uint32_t sync_phase = 0U;
    gvd_decision_t previous = {GVD_UNKNOWN, GVD_CUE_NONE, GVD_WAITING};
    leds_write(0U);
    (void) gvd_phy_led_init();
    while (1) {
        uint32_t at;
        (void) read_input(&input, &generation);
        if (!now_ms(&at)) {
            leds_write(0U);
            gvd_phy_led_set(GVD_CUE_NONE);
            tk_dly_tsk(GVD_OUTPUT_POLL_MS);
            continue;
        }
        gvd_decision_t decision = gvd_decide(input.seen ? &input.snapshot : NULL,
                                            at - input.received_ms, input.valid);
        if (!sync_started && input.seen && input.valid) {
            sync_started = 1U;
            sync_start_ms = at;
            publish_event(&input, decision, at, 1U);
            g_cpu1_demo_sync_count++;
        }
        if (sync_started && !sync_done) {
            uint32_t phase = (at - sync_start_ms) / GVD_SYNC_STEP_MS;
            if (phase < 4U) {
                if (phase != sync_phase) { leds_write((phase & 1U) ? 0U : 7U); }
                sync_phase = phase;
                tk_dly_tsk(GVD_OUTPUT_POLL_MS);
                continue;
            }
            sync_done = 1U;
            last_generation = UINT32_MAX; /* Reevaluate; never restore an old cue. */
        }
        if (generation != last_generation || decision.risk != previous.risk ||
            decision.cue != previous.cue || decision.reason != previous.reason) {
            publish_event(&input, decision, at, 0U);
            previous = decision;
            last_generation = generation;
        }
        tk_dly_tsk(GVD_OUTPUT_POLL_MS);
    }
}

ER gvd_output_create(void)
{
    const T_CTSK ctsk =
    {
        .tskatr  = TA_HLNG,
        .task    = gvd_output_task_entry,
        .itskpri = GVD_OUTPUT_PRIORITY,
        .stksz   = GVD_TASK_STACK_BYTES
    };

    g_cpu1_demo_task_id = tk_cre_tsk(&ctsk);
    if (g_cpu1_demo_task_id < E_OK)
    {
        g_cpu1_gvd_start_error = (ER) g_cpu1_demo_task_id;
        return g_cpu1_gvd_start_error;
    }

    return E_OK;
}

ER gvd_output_start(void)
{
    if (g_cpu1_demo_task_id <= 0)
    {
        g_cpu1_gvd_start_error = E_ID;
        return g_cpu1_gvd_start_error;
    }

    g_cpu1_gvd_start_error = tk_sta_tsk(g_cpu1_demo_task_id, 0);
    return g_cpu1_gvd_start_error;
}
