#ifndef GVD_OUTPUT_H
#define GVD_OUTPUT_H

#include <tk/tkernel.h>
#include "shared_ipc_protocol.h"
#include "gvd_policy.h"

/* CPU1-local record shared by the LED and console paths; not an IPC type. */
typedef struct {
    uint32_t event;
    uint32_t seq;
    uint32_t at_ms;
    uint32_t frame;
    uint32_t semantic_frame;
    uint16_t left, right, objects;
    gvd_decision_t decision;
    uint32_t sync;
} gvd_event_t;

ER gvd_output_create(void);
ER gvd_output_start(void);

/* Called only from the CPU1 IPC task, AFTER existing latency measurements. */
void gvd_output_accept(const perception_snapshot_t *snapshot);
void gvd_output_invalidate(void);

/* Called by the log task with a valid destination.
 * A zero return means no coherent event is available; ignore the copy.
 */
uint32_t gvd_output_read_event(gvd_event_t *out);
uint32_t gvd_output_read_sync_event(gvd_event_t *out);

/* Existing debugger names are retained for startup diagnostics. */
extern volatile ID g_cpu1_demo_task_id;
extern volatile ER g_cpu1_gvd_start_error;
extern gvd_event_t g_cpu1_gvd_latest;

#endif /* GVD_OUTPUT_H */
