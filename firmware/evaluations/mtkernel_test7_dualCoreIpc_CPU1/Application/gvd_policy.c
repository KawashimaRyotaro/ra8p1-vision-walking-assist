#include <stddef.h>
#include "gvd_policy.h"
#include "gvd_config.h"

gvd_decision_t gvd_decide(const perception_snapshot_t *s,
                          uint32_t age_ms, uint32_t valid)
{
    gvd_decision_t d = {GVD_UNKNOWN, GVD_CUE_NONE, GVD_WAITING};
    if (NULL == s) { return d; }
    if (!valid || s->object_count > SHARED_PERCEPTION_OBJECT_MAX ||
        s->activity_coverage_x1000 > 1000U) {
        d.reason = GVD_INVALID;
        return d;
    }
    if (age_ms >= GVD_STALE_MS) { d.reason = GVD_STALE; return d; }
    if (s->motion_uncertain || !s->global_valid) {
        d.reason = GVD_UNCERTAIN;
        return d;
    }

    uint32_t l = s->activity_left;
    uint32_t r = s->activity_right;
    uint32_t peak = (l > r) ? l : r;
    uint32_t diff = (l > r) ? (l - r) : (r - l);
    if (peak < GVD_POSSIBLE_ACTIVITY_MIN) {
        d.risk = GVD_NO_RISK;
        d.reason = GVD_LOW_ACTIVITY;
        return d;
    }
    d.risk = (peak >= GVD_HIGH_ACTIVITY_MIN) ? GVD_HIGH : GVD_POSSIBLE;
    if (diff < GVD_DIRECTION_DIFF_MIN ||
        diff * 100U < (l + r) * GVD_DIRECTION_DIFF_PC) {
        d.reason = GVD_BALANCED;
        return d;
    }
    d.reason = GVD_ACTIVITY;
    /* Higher activity side is the DEMO hazard-side proxy, not the cue. */
    d.cue = (l > r) ? GVD_CUE_RIGHT : GVD_CUE_LEFT;
#if GVD_CUE_SWAP_LR
    d.cue = (d.cue == GVD_CUE_LEFT) ? GVD_CUE_RIGHT : GVD_CUE_LEFT;
#endif
    return d;
}

const char *gvd_risk_name(gvd_risk_t risk)
{
    switch (risk) {
    case GVD_NO_RISK: return "NONE";
    case GVD_POSSIBLE: return "POSSIBLE";
    case GVD_HIGH: return "HIGH";
    default: return "UNKNOWN";
    }
}
const char *gvd_cue_name(gvd_cue_t cue)
{
    switch (cue) {
    case GVD_CUE_LEFT: return "LEFT";
    case GVD_CUE_RIGHT: return "RIGHT";
    default: return "NONE";
    }
}
const char *gvd_reason_name(gvd_reason_t reason)
{
    switch (reason) {
    case GVD_INVALID: return "INVALID";
    case GVD_STALE: return "STALE";
    case GVD_UNCERTAIN: return "UNCERTAIN";
    case GVD_LOW_ACTIVITY: return "LOW_ACTIVITY";
    case GVD_BALANCED: return "BALANCED";
    case GVD_ACTIVITY: return "ACTIVITY";
    default: return "WAITING";
    }
}
