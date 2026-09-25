#ifndef GVD_POLICY_H
#define GVD_POLICY_H
#include <stdint.h>
#include "shared_ipc_protocol.h"

typedef enum { GVD_UNKNOWN = 0, GVD_NO_RISK = 1,
               GVD_POSSIBLE = 2, GVD_HIGH = 3 } gvd_risk_t;
typedef enum { GVD_CUE_NONE = 0, GVD_CUE_LEFT, GVD_CUE_RIGHT } gvd_cue_t;
typedef enum { GVD_WAITING = 0, GVD_INVALID, GVD_STALE, GVD_UNCERTAIN,
               GVD_LOW_ACTIVITY, GVD_BALANCED, GVD_ACTIVITY } gvd_reason_t;
typedef struct {
    gvd_risk_t risk;
    gvd_cue_t cue;
    gvd_reason_t reason;
} gvd_decision_t;

gvd_decision_t gvd_decide(const perception_snapshot_t *snapshot,
                          uint32_t age_ms, uint32_t valid);
const char *gvd_risk_name(gvd_risk_t risk);
const char *gvd_cue_name(gvd_cue_t cue);
const char *gvd_reason_name(gvd_reason_t reason);
#endif
