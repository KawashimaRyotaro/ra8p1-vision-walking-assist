#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <tk/tkernel.h>
#include <tm/tmonitor.h>

#include "hal_data.h"
#include "motion_task.h"
#include "video_source.h"
#include "motion_global.h"
#include "npu_worker.h"
#include "ipc_test.h"


/*
 * Emergency temporal path must run before display/USB/NPU tasks,
 * but CP1 only performs a short 112x84 conversion before yielding.
 *
 * Existing priorities:
 *   Display = 10
 *   USB     = 11
 *   NPU     = 12
 */
#define MOTION_TASK_PRIORITY       (9)
#define MOTION_TASK_STACK_SIZE     (2048)


uint8_t g_motion_gray_buffers
    [MOTION_GRAY_BUFFER_COUNT]
    [MOTION_GRAY_PIXELS];

volatile uint32_t g_motion_gray_frame_count =
    0U;

volatile uint32_t g_motion_gray_last_frame_index =
    UINT32_MAX;

volatile uint32_t g_motion_gray_last_buffer =
    0U;

volatile uint32_t g_motion_gray_consecutive_count =
    0U;

volatile uint32_t g_motion_gray_gap_count =
    0U;

volatile uint32_t g_motion_gray_last_checksum =
    0U;

volatile uint32_t g_motion_temporal_latency_count =
    0U;

volatile uint32_t g_motion_temporal_latency_last_ms =
    0U;

volatile uint32_t g_motion_temporal_latency_total_ms =
    0U;

volatile uint32_t g_motion_temporal_latency_min_ms =
    UINT32_MAX;

volatile uint32_t g_motion_temporal_latency_max_ms =
    0U;


/*
 * CP2 latest 16x12 motion-vector field.
 *
 * 192 vectors × 4 bytes = 768 bytes.
 */
motion_vector_t g_motion_vectors
    [MOTION_VECTOR_COUNT];

volatile uint32_t g_motion_vector_frame_count =
    0U;

volatile uint32_t g_motion_vector_last_frame_index =
    UINT32_MAX;


/*
 * Debug-display exact-frame history.
 *
 * 8 frames × 192 vectors × 4 bytes
 * = about 6 KB.
 */
#define MOTION_VECTOR_HISTORY_COUNT    (8U)

static motion_vector_t
    s_motion_vector_history
        [MOTION_VECTOR_HISTORY_COUNT]
        [MOTION_VECTOR_COUNT];

static volatile uint32_t
    s_motion_vector_history_frame
        [MOTION_VECTOR_HISTORY_COUNT] =
{
    UINT32_MAX,
    UINT32_MAX,
    UINT32_MAX,
    UINT32_MAX,
    UINT32_MAX,
    UINT32_MAX,
    UINT32_MAX,
    UINT32_MAX
};


static ID s_motion_task_id =
    0;

static uint8_t s_write_buffer =
    0U;

/*
 * CP3-C:
 * Workspace for continuous integrated Perception Bank snapshots.
 */
static motion_vector_t
    s_perception_motion_vectors[MOTION_VECTOR_COUNT];

static motion_global_result_t
    s_perception_global_result;

static Detection_t
    s_perception_detections[
        SHARED_PERCEPTION_OBJECT_MAX
    ];


static void motion_task_entry(
    INT stacd,
    void * exinf);


static T_CTSK s_motion_task_ctsk =
{
    .tskatr  = TA_HLNG | TA_RNG3,
    .task    = motion_task_entry,
    .itskpri = MOTION_TASK_PRIORITY,
    .stksz   = MOTION_TASK_STACK_SIZE,
};


static uint32_t motion_task_activity(
    motion_vector_t const * vector)
{
    int32_t const dx =
        vector->dx;

    int32_t const dy =
        vector->dy;

    uint32_t const abs_dx =
        (uint32_t)
        (
            (dx < 0) ?
            -dx :
            dx
        );

    uint32_t const abs_dy =
        (uint32_t)
        (
            (dy < 0) ?
            -dy :
            dy
        );

    return
        abs_dx +
        abs_dy;
}


static void motion_task_fill_semantic_snapshot(
    perception_snapshot_t * snapshot)
{
    if (NULL == snapshot)
    {
        return;
    }


    uint32_t semantic_frame_index =
        UINT32_MAX;


    int32_t const detection_count =
        npu_worker_copy_latest_detections(
            s_perception_detections,
            SHARED_PERCEPTION_OBJECT_MAX,
            &semantic_frame_index
        );


    /*
     * YOLOX has not completed even one inference yet.
     */
    if (detection_count < 0)
    {
        snapshot->semantic_frame_index =
            UINT32_MAX;

        snapshot->object_count =
            0U;

        return;
    }


    snapshot->semantic_frame_index =
        semantic_frame_index;


    uint32_t object_count =
        (uint32_t) detection_count;


    if (object_count >
        SHARED_PERCEPTION_OBJECT_MAX)
    {
        object_count =
            SHARED_PERCEPTION_OBJECT_MAX;
    }


    snapshot->object_count =
        (uint16_t) object_count;


    for (uint32_t i = 0U;
         i < object_count;
         i++)
    {
        Detection_t const * const detection =
            &s_perception_detections[i];


        float score =
            detection->score;


        if (score < 0.0F)
        {
            score =
                0.0F;
        }

        if (score > 1.0F)
        {
            score =
                1.0F;
        }


        float x1 =
            detection->x1;

        float y1 =
            detection->y1;

        float x2 =
            detection->x2;

        float y2 =
            detection->y2;


        float const max_x =
            (float)
            (VIDEO_SOURCE_WIDTH - 1U);

        float const max_y =
            (float)
            (VIDEO_SOURCE_HEIGHT - 1U);


        if (x1 < 0.0F)
        {
            x1 = 0.0F;
        }

        if (x1 > max_x)
        {
            x1 = max_x;
        }

        if (y1 < 0.0F)
        {
            y1 = 0.0F;
        }

        if (y1 > max_y)
        {
            y1 = max_y;
        }

        if (x2 < 0.0F)
        {
            x2 = 0.0F;
        }

        if (x2 > max_x)
        {
            x2 = max_x;
        }

        if (y2 < 0.0F)
        {
            y2 = 0.0F;
        }

        if (y2 > max_y)
        {
            y2 = max_y;
        }


        if (x2 < x1)
        {
            x2 = x1;
        }

        if (y2 < y1)
        {
            y2 = y1;
        }


        snapshot->objects[i].class_id =
            (uint16_t)
            detection->cls_id;

        snapshot->objects[i].confidence_x1000 =
            (uint16_t)
            (
                score * 1000.0F +
                0.5F
            );

        snapshot->objects[i].center_x =
            (uint16_t)
            (
                (x1 + x2) *
                0.5F
            );

        snapshot->objects[i].center_y =
            (uint16_t)
            (
                (y1 + y2) *
                0.5F
            );

        snapshot->objects[i].width =
            (uint16_t)
            (
                x2 - x1
            );

        snapshot->objects[i].height =
            (uint16_t)
            (
                y2 - y1
            );

        /*
         * Metric/non-metric distance estimation is not
         * implemented yet.
         */
        snapshot->objects[i].proximity_x1000 =
            0U;
    }
}


static void motion_task_publish_perception(
    uint32_t frame_index,
    uint32_t source_tick_ms,
    uint32_t source_tick_gpt)
{
    /*
     * Derive the temporal path from the current real Motion field.
     */
    motion_global_compensate(
        g_motion_vectors,
        s_perception_motion_vectors,
        &s_perception_global_result
    );


    uint32_t activity_left =
        0U;

    uint32_t activity_right =
        0U;

    uint32_t active_blocks =
        0U;

    uint32_t considered_blocks =
        0U;


    /*
     * Keep the same spatial definition used by the existing
     * Activity debug view:
     *
     * ignore top and bottom block rows.
     */
    for (uint32_t block_y = 1U;
         block_y < (MOTION_GRID_ROWS - 1U);
         block_y++)
    {
        for (uint32_t block_x = 0U;
             block_x < MOTION_GRID_COLS;
             block_x++)
        {
            uint32_t const vector_index =
                block_y *
                MOTION_GRID_COLS +
                block_x;


            uint32_t const activity =
                motion_task_activity(
                    &s_perception_motion_vectors[
                        vector_index
                    ]
                );


            if (block_x <
                (MOTION_GRID_COLS / 2U))
            {
                activity_left +=
                    activity;
            }
            else
            {
                activity_right +=
                    activity;
            }


            if (0U != activity)
            {
                active_blocks++;
            }


            considered_blocks++;
        }
    }


    perception_snapshot_t snapshot =
    {
        0
    };


    snapshot.temporal_frame_index =
        frame_index;

    snapshot.temporal_source_tick_gpt =
        source_tick_gpt;

    /*
     * UINT32_MAX means:
     * no completed semantic result is available yet.
     */
    snapshot.semantic_frame_index =
        UINT32_MAX;


    SYSTIM system_time;


    if (tk_get_otm(&system_time) >= E_OK)
    {
        snapshot.producer_tick_ms =
            system_time.lo;


        uint32_t const latency_ms =
            snapshot.producer_tick_ms -
            source_tick_ms;


        g_motion_temporal_latency_last_ms =
            latency_ms;

        g_motion_temporal_latency_total_ms +=
            latency_ms;

        g_motion_temporal_latency_count++;


        if (latency_ms <
            g_motion_temporal_latency_min_ms)
        {
            g_motion_temporal_latency_min_ms =
                latency_ms;
        }


        if (latency_ms >
            g_motion_temporal_latency_max_ms)
        {
            g_motion_temporal_latency_max_ms =
                latency_ms;
        }
    }


    snapshot.activity_left =
        (uint16_t)
        activity_left;

    snapshot.activity_right =
        (uint16_t)
        activity_right;


    if (0U != considered_blocks)
    {
        snapshot.activity_coverage_x1000 =
            (uint16_t)
            (
                (active_blocks * 1000U) /
                considered_blocks
            );
    }


    snapshot.global_dx =
        s_perception_global_result.dx;

    snapshot.global_dy =
        s_perception_global_result.dy;

    snapshot.global_valid =
        s_perception_global_result.valid;

    snapshot.motion_uncertain =
        (0U ==
         s_perception_global_result.valid) ?
        1U :
        0U;


    /*
     * Add the latest COMPLETED asynchronous YOLOX result.
     *
     * The semantic frame does not need to equal the
     * current Motion frame.
     */
    motion_task_fill_semantic_snapshot(
        &snapshot
    );


    fsp_err_t const publish_err =
        ipc_perception_publish(
            &snapshot
        );


    /*
     * Do not print every frame.
     * That would perturb real-time behavior.
     */
    if ((FSP_SUCCESS != publish_err) ||
        (1U == frame_index) ||
        (100U == frame_index) ||
        (200U == frame_index) ||
        (299U == frame_index))
    {
        tm_printf(
            (UB *)"[Perception] "
                  "seq_frame=%u "
                  "semantic_frame=%u "
                  "left=%u right=%u "
                  "coverage=%u/1000 "
                  "global=(%d,%d) valid=%u "
                  "uncertain=%u "
                  "objects=%u "
                  "publish_err=%d\n",
            frame_index,
            snapshot.semantic_frame_index,
            activity_left,
            activity_right,
            snapshot.activity_coverage_x1000,
            snapshot.global_dx,
            snapshot.global_dy,
            snapshot.global_valid,
            snapshot.motion_uncertain,
            snapshot.object_count,
            publish_err
        );
    }
}


static uint32_t motion_gray_checksum(
    const uint8_t * gray)
{
    /*
     * FNV-1a: used only for sparse CP1 debug output.
     */
    uint32_t hash =
        2166136261UL;

    for (uint32_t i = 0U;
         i < MOTION_GRAY_PIXELS;
         i++)
    {
        hash ^=
            gray[i];

        hash *=
            16777619UL;
    }

    return hash;
}


static void motion_gray_print_checkpoint(
    uint32_t frame_index,
    uint32_t buffer_index)
{
    uint8_t const * const gray =
        g_motion_gray_buffers[buffer_index];

    uint32_t sum =
        0U;

    uint32_t min_value =
        255U;

    uint32_t max_value =
        0U;

    for (uint32_t i = 0U;
         i < MOTION_GRAY_PIXELS;
         i++)
    {
        uint32_t const value =
            gray[i];

        sum +=
            value;

        if (value < min_value)
        {
            min_value =
                value;
        }

        if (value > max_value)
        {
            max_value =
                value;
        }
    }

    uint32_t const checksum =
        motion_gray_checksum(
            gray
        );

    g_motion_gray_last_checksum =
        checksum;

    tm_printf(
        (UB *)"[Motion][CP1] "
              "frame=%u buf=%u gray=%ux%u "
              "min=%u max=%u mean=%u "
              "checksum=0x%08X "
              "count=%u consecutive=%u gaps=%u\n",
        frame_index,
        buffer_index,
        MOTION_GRAY_WIDTH,
        MOTION_GRAY_HEIGHT,
        min_value,
        max_value,
        sum / MOTION_GRAY_PIXELS,
        checksum,
        g_motion_gray_frame_count,
        g_motion_gray_consecutive_count,
        g_motion_gray_gap_count
    );
}


static void motion_vectors_print_checkpoint(
    uint32_t frame_index)
{
    uint32_t nonzero_count =
        0U;

    uint32_t sum_abs_dx =
        0U;

    uint32_t sum_abs_dy =
        0U;

    uint32_t sum_sad =
        0U;


    for (uint32_t i = 0U;
         i < MOTION_VECTOR_COUNT;
         i++)
    {
        int32_t const dx =
            g_motion_vectors[i].dx;

        int32_t const dy =
            g_motion_vectors[i].dy;


        if ((0 != dx) ||
            (0 != dy))
        {
            nonzero_count++;
        }


        sum_abs_dx +=
            (uint32_t)
            (
                (dx < 0) ?
                -dx :
                dx
            );

        sum_abs_dy +=
            (uint32_t)
            (
                (dy < 0) ?
                -dy :
                dy
            );

        sum_sad +=
            g_motion_vectors[i].sad;
    }


    /*
     * Approximately central block:
     * row=6, col=8.
     */
    uint32_t const center_index =
        6U * MOTION_GRID_COLS +
        8U;


    tm_printf(
        (UB *)"[Motion][CP2] "
              "frame=%u vectors=%u pair_count=%u "
              "nonzero=%u abs_dx_sum=%u abs_dy_sum=%u "
              "avg_sad=%u "
              "center=(%d,%d,sad=%u)\n",
        frame_index,
        MOTION_VECTOR_COUNT,
        g_motion_vector_frame_count,
        nonzero_count,
        sum_abs_dx,
        sum_abs_dy,
        sum_sad / MOTION_VECTOR_COUNT,
        g_motion_vectors[center_index].dx,
        g_motion_vectors[center_index].dy,
        g_motion_vectors[center_index].sad
    );
}


uint8_t motion_task_copy_vectors_for_frame(
    uint32_t frame_index,
    motion_vector_t out_vectors[MOTION_VECTOR_COUNT])
{
    if (NULL == out_vectors)
    {
        return 0U;
    }


    uint32_t const history_slot =
        frame_index %
        MOTION_VECTOR_HISTORY_COUNT;


    __DMB();


    if (s_motion_vector_history_frame[history_slot] !=
        frame_index)
    {
        return 0U;
    }


    for (uint32_t i = 0U;
         i < MOTION_VECTOR_COUNT;
         i++)
    {
        out_vectors[i] =
            s_motion_vector_history
                [history_slot][i];
    }


    __DMB();


    /*
     * Ensure producer did not overwrite this slot
     * while Display was copying it.
     */
    if (s_motion_vector_history_frame[history_slot] !=
        frame_index)
    {
        return 0U;
    }


    return 1U;
}


ER motion_task_create(void)
{
    ID const task_id =
        tk_cre_tsk(
            &s_motion_task_ctsk
        );

    if (task_id < E_OK)
    {
        return
            (ER) task_id;
    }

    s_motion_task_id =
        task_id;

    return
        E_OK;
}


ER motion_task_start(void)
{
    if (s_motion_task_id <= 0)
    {
        return
            E_ID;
    }

    return
        tk_sta_tsk(
            s_motion_task_id,
            0
        );
}


static void motion_task_entry(
    INT stacd,
    void * exinf)
{
    (void) stacd;
    (void) exinf;

    tm_printf(
        (UB *)"[Motion] task started. block_match_mve=%u\n",
        MOTION_BLOCK_MATCH_MVE_ENABLED
    );

    /*
     * From this point every newly published raw frame also has a
     * motion-consumer reference.
     */
    video_source_motion_consumer_enable();

    while (1)
    {
        uint32_t slot =
            VIDEO_SOURCE_INVALID_SLOT;

        uint32_t frame_index =
            0U;

        const uint8_t * const frame =
            video_source_acquire_motion_buffer(
                &slot,
                &frame_index
            );


        if (NULL == frame)
        {
            tk_dly_tsk(1);
            continue;
        }


        uint32_t source_tick_ms =
            0U;

        (void)
        video_source_get_publish_tick_ms(
            slot,
            frame_index,
            &source_tick_ms
        );


        uint32_t source_tick_gpt =
            0U;

        (void)
        video_source_get_publish_tick_gpt(
            slot,
            frame_index,
            &source_tick_gpt
        );

        if (0U == frame_index)
        {
            g_motion_vector_frame_count =
                0U;

            g_motion_vector_last_frame_index =
                UINT32_MAX;


            for (uint32_t i = 0U;
                i < MOTION_VECTOR_HISTORY_COUNT;
                i++)
            {
                s_motion_vector_history_frame[i] =
                    UINT32_MAX;
            }

            __DMB();


            s_write_buffer =
                0U;

            g_motion_gray_last_buffer =
                0U;

            g_motion_gray_consecutive_count =
                0U;

            g_motion_gray_gap_count =
                0U;

            g_motion_gray_last_checksum =
                0U;

            g_motion_vector_frame_count =
                0U;

            g_motion_vector_last_frame_index =
                UINT32_MAX;
        }


        uint32_t const buffer_index =
            s_write_buffer;


        /*
         * CP1:
         * 224x168 RGB565 / 2048-byte stride
         *     -> 112x84 uint8 grayscale.
         */
        motion_preprocess_rgb565_to_gray_half(
            frame,
            VIDEO_SOURCE_STRIDE_BYTES,
            g_motion_gray_buffers[buffer_index]
        );


        /*
         * Raw frame ownership ends immediately after grayscale copy.
         * Later block matching operates only on these compact buffers.
         */
        video_source_release_motion_buffer(
            slot
        );


        /*
        * CP2:
        *
        * Match only truly consecutive grayscale frames.
        *
        * buffer_index     = current
        * buffer_index ^ 1 = previous
        */
        uint8_t const has_previous_frame =
            (UINT32_MAX !=
            g_motion_gray_last_frame_index) &&
            (frame_index ==
            (g_motion_gray_last_frame_index + 1U));


        if (0U != has_previous_frame)
        {
            uint32_t const previous_buffer_index =
                buffer_index ^ 1U;


            motion_block_match(
                g_motion_gray_buffers[
                    previous_buffer_index
                ],
                g_motion_gray_buffers[
                    buffer_index
                ],
                g_motion_vectors
            );


            /*
            * Publish exact-frame vector history.
            */
            uint32_t const history_slot =
                frame_index %
                MOTION_VECTOR_HISTORY_COUNT;


            /*
            * Invalidate slot while it is being updated.
            */
            s_motion_vector_history_frame[history_slot] =
                UINT32_MAX;

            __DMB();


            for (uint32_t i = 0U;
                i < MOTION_VECTOR_COUNT;
                i++)
            {
                s_motion_vector_history
                    [history_slot][i] =
                        g_motion_vectors[i];
            }


            __DMB();


            /*
            * Publish frame index last.
            */
            s_motion_vector_history_frame[history_slot] =
                frame_index;

            __DMB();


            g_motion_vector_frame_count++;

            g_motion_vector_last_frame_index =
                frame_index;

            /*
            * CP3-C:
            * Current Motion result + newest completed YOLOX result
            * -> one integrated Perception Bank snapshot.
            */
            motion_task_publish_perception(
                frame_index,
                source_tick_ms,
                source_tick_gpt
            );


            if ((1U == frame_index) ||
                (0U == (frame_index % 100U)) ||
                (299U == frame_index))
            {
                motion_vectors_print_checkpoint(
                    frame_index
                );
            }
        }


        if (UINT32_MAX !=
            g_motion_gray_last_frame_index)
        {
            if (frame_index ==
                (g_motion_gray_last_frame_index + 1U))
            {
                g_motion_gray_consecutive_count++;
            }
            else
            {
                g_motion_gray_gap_count++;
            }
        }


        g_motion_gray_last_buffer =
            buffer_index;

        g_motion_gray_last_frame_index =
            frame_index;

        g_motion_gray_frame_count++;


        /*
         * Sparse logging only.
         */
        if ((0U == frame_index) ||
            (1U == frame_index) ||
            (0U == (frame_index % 100U)) ||
            (299U == frame_index))
        {
            motion_gray_print_checkpoint(
                frame_index,
                buffer_index
            );
        }


        s_write_buffer ^=
            1U;


        /*
         * Motion runs above Display in priority, but a backlog can keep a
         * higher-priority task continuously runnable.  Briefly block after
         * completing one full temporal result so the lower-priority debug
         * observer can compose and present its pending raw frame.
         *
         * The emergency path still preempts Display as soon as this delay
         * expires; no wait is inserted between frame acquisition and CP1/CP2.
         */
        tk_dly_tsk(1);
    }
}
