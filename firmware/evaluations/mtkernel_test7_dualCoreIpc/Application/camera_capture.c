#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include <tk/tkernel.h>
#include <tm/tmonitor.h>

#include "hal_data.h"
#include "common_data.h"

#include "camera_capture.h"
#include "camera_sensor.h"
#include "i2c_control.h"
#include "switch_init.h"
#include "video_source.h"

#define CAMERA_CAPTURE_WIDTH         (224U)
#define CAMERA_CAPTURE_HEIGHT        (168U)
#define CAMERA_ACTIVE_LINE_BYTES     (CAMERA_CAPTURE_WIDTH * 2U)


enum
{
    CAMERA_MIPI_TRACE_AFTER_VIN_OPEN = 0,
    CAMERA_MIPI_TRACE_AFTER_CAPTURE_START,
    CAMERA_MIPI_TRACE_BEFORE_STREAM_ON,
    CAMERA_MIPI_TRACE_AFTER_STREAM_ON,
    CAMERA_MIPI_TRACE_AFTER_1_MS,
    CAMERA_MIPI_TRACE_AFTER_10_MS,
    CAMERA_MIPI_TRACE_AFTER_100_MS,
    CAMERA_MIPI_TRACE_COUNT
};

typedef struct st_camera_mipi_trace_snapshot
{
    uint32_t checkpoint;

    uint32_t phy_dphymdc;
    uint32_t phy_refcr;
    uint32_t phy_pwrcr;
    uint32_t phy_ocr;
    uint32_t phy_sfr;

    uint32_t csi_mct0;
    uint32_t csi_mct2;
    uint32_t csi_mct3;
    uint32_t csi_rxst;
    uint32_t csi_dlst0;
    uint32_t csi_dlst1;
    uint32_t csi_mist;
    uint32_t csi_vcst0;
    uint32_t csi_pmst;

    uint32_t vin_mc;
    uint32_t vin_fc;
    uint32_t vin_ms;
    uint32_t vin_lc;

    uint32_t p411_pfs;
    uint32_t p501_pfs;
    uint32_t p511_pfs;
    uint32_t p512_pfs;
    uint32_t p709_pfs;
} camera_mipi_trace_snapshot_t;


volatile camera_mipi_trace_snapshot_t
    g_camera_mipi_trace[CAMERA_MIPI_TRACE_COUNT];

volatile uint32_t g_camera_mipi_trace_magic = 0x4D495031U;
volatile uint32_t g_camera_mipi_trace_valid_mask = 0U;
volatile uint32_t g_camera_mipi_trace_complete = 0U;


static vin_extended_cfg_t s_vin_extend;
static capture_cfg_t s_vin_cfg;

#define CAMERA_SOURCE_TASK_PRIORITY    (8)
#define CAMERA_SOURCE_TASK_STACK_SIZE  (2048)

static ID s_camera_source_task_id = 0;

static volatile uint8_t
    s_camera_live_enabled = 0U;

static volatile uintptr_t
    s_camera_live_completed_buffer = (uintptr_t) 0U;


/*
 * Camera -> Frame Cache diagnostics.
 */
volatile uint32_t g_camera_source_irq_count = 0U;
volatile uint32_t g_camera_source_publish_count = 0U;
volatile uint32_t g_camera_source_drop_count = 0U;
volatile uint32_t g_camera_source_copy_count = 0U;

volatile uint32_t g_camera_source_loop_count = 0U;
volatile uint32_t g_camera_source_idle_count = 0U;
volatile uint32_t g_camera_source_new_irq_count = 0U;
volatile uint32_t g_camera_source_retry_count = 0U;
volatile uint32_t g_camera_source_acquire_attempt_count = 0U;

volatile uint32_t g_camera_source_last_frame_index =
    UINT32_MAX;

volatile uintptr_t g_camera_source_last_vin_buffer =
    (uintptr_t) 0U;

volatile uint32_t g_camera_source_last_cache_slot =
    VIDEO_SOURCE_INVALID_SLOT;


static void camera_source_task_entry(
    INT stacd,
    void * exinf
);


static T_CTSK s_camera_source_task_ctsk =
{
    .tskatr  = TA_HLNG | TA_RNG3,
    .task    = camera_source_task_entry,
    .itskpri = CAMERA_SOURCE_TASK_PRIORITY,
    .stksz   = CAMERA_SOURCE_TASK_STACK_SIZE,
};

volatile uint32_t g_camera_vin_frame_count = 0U;
volatile uintptr_t g_camera_vin_last_buffer = (uintptr_t) 0U;
volatile uint32_t g_camera_vin_error_count = 0U;
volatile uint32_t g_camera_vin_checksum = 0U;
volatile fsp_err_t g_camera_vin_open_err = FSP_SUCCESS;
volatile fsp_err_t g_camera_vin_start_err = FSP_SUCCESS;
volatile fsp_err_t g_camera_vin_stream_err = FSP_SUCCESS;
volatile uint32_t g_camera_vin_probe_stage = 0U;

volatile uint32_t g_camera_mipi_event_count = 0U;
volatile uint32_t g_camera_mipi_last_event = 0U;

volatile uint32_t g_camera_vin_callback_count = 0U;
volatile uint32_t g_camera_vin_last_interrupt_status = 0U;
volatile uint32_t g_camera_vin_last_event_status = 0U;

volatile uint32_t g_camera_vin_buf1_before = 0U;
volatile uint32_t g_camera_vin_buf1_after  = 0U;

volatile uint32_t g_camera_vin_buf2_before = 0U;
volatile uint32_t g_camera_vin_buf2_after  = 0U;

volatile uint32_t g_camera_vin_buf3_before = 0U;
volatile uint32_t g_camera_vin_buf3_after  = 0U;

volatile uint32_t g_camera_vin_buffer_write_detected = 0U;

volatile uint32_t g_camera_hw_phy_pwrsen = 0U;
volatile uint32_t g_camera_hw_phy_dphyen = 0U;
volatile uint32_t g_camera_hw_phy_status = 0U;

volatile uint32_t g_camera_hw_csi_rxen = 0U;
volatile uint32_t g_camera_hw_csi_rxst = 0U;
volatile uint32_t g_camera_hw_csi_ractdet = 0U;

volatile uint32_t g_camera_hw_vin_me = 0U;
volatile uint32_t g_camera_hw_vin_cc = 0U;
volatile uint32_t g_camera_hw_vin_ms = 0U;
volatile uint32_t g_camera_hw_vin_ca = 0U;
volatile uint32_t g_camera_hw_vin_lc = 0U;

volatile fsp_err_t g_camera_xclk_info_err =
    FSP_SUCCESS;

volatile uint32_t g_camera_xclk_clock_hz = 0U;
volatile uint32_t g_camera_xclk_period_counts = 0U;
volatile uint32_t g_camera_xclk_output_hz = 0U;

volatile fsp_err_t g_camera_sensor_readback_err =
    FSP_SUCCESS;

volatile uint32_t g_camera_sensor_readback_fail_reg = 0U;

volatile uint32_t g_camera_sensor_reg_3008 = 0U;
volatile uint32_t g_camera_sensor_reg_4202 = 0U;
volatile uint32_t g_camera_sensor_reg_300e = 0U;
volatile uint32_t g_camera_sensor_reg_3035 = 0U;
volatile uint32_t g_camera_sensor_reg_3036 = 0U;
volatile uint32_t g_camera_sensor_reg_3037 = 0U;
volatile uint32_t g_camera_sensor_reg_4814 = 0U;

volatile uint32_t g_camera_reset_pin_level = 0U;

volatile uint32_t g_camera_gpt12_gtcr  = 0U;
volatile uint32_t g_camera_gpt12_gtior = 0U;
volatile uint32_t g_camera_gpt12_gtpr  = 0U;
volatile uint32_t g_camera_p501_pfs    = 0U;

volatile uint32_t g_camera_cmp_phy_dphymdc = 0U;
volatile uint32_t g_camera_cmp_phy_refcr   = 0U;

volatile uint32_t g_camera_cmp_phy_tim1 = 0U;
volatile uint32_t g_camera_cmp_phy_tim2 = 0U;
volatile uint32_t g_camera_cmp_phy_tim3 = 0U;
volatile uint32_t g_camera_cmp_phy_tim4 = 0U;
volatile uint32_t g_camera_cmp_phy_tim5 = 0U;
volatile uint32_t g_camera_cmp_phy_tim6 = 0U;

volatile uint32_t g_camera_cmp_csi_mct0 = 0U;
volatile uint32_t g_camera_cmp_csi_mct2 = 0U;
volatile uint32_t g_camera_cmp_csi_mct3 = 0U;
volatile uint32_t g_camera_cmp_csi_epct = 0U;
volatile uint32_t g_camera_cmp_csi_emct = 0U;
volatile uint32_t g_camera_cmp_csi_dtel = 0U;
volatile uint32_t g_camera_cmp_csi_dteh = 0U;
volatile uint32_t g_camera_cmp_csi_gsct = 0U;

volatile uint32_t g_camera_cmp_vin_mc = 0U;
volatile uint32_t g_camera_cmp_vin_fc = 0U;

volatile uint32_t g_camera_cmp_csi_dlst0 = 0U;
volatile uint32_t g_camera_cmp_csi_dlst1 = 0U;
volatile uint32_t g_camera_cmp_csi_mist  = 0U;
volatile uint32_t g_camera_cmp_csi_vcst0 = 0U;
volatile uint32_t g_camera_cmp_csi_pmst  = 0U;

volatile uint32_t g_camera_xclk_seen_low = 0U;
volatile uint32_t g_camera_xclk_seen_high = 0U;
volatile uint32_t g_camera_xclk_transitions = 0U;

volatile fsp_err_t g_camera_switch_addr_diag_err    = FSP_SUCCESS;
volatile fsp_err_t g_camera_switch_dir_diag_err     = FSP_SUCCESS;
volatile fsp_err_t g_camera_switch_state_diag_err   = FSP_SUCCESS;
volatile fsp_err_t g_camera_switch_enable_diag_err  = FSP_SUCCESS;
volatile fsp_err_t g_camera_switch_restore_diag_err = FSP_SUCCESS;

volatile uint32_t g_camera_switch_dir_diag    = 0U;
volatile uint32_t g_camera_switch_state_diag  = 0U;
volatile uint32_t g_camera_switch_enable_diag = 0U;

volatile uint32_t g_camera_tx_300e = 0U;
volatile uint32_t g_camera_tx_3019 = 0U;
volatile uint32_t g_camera_tx_4800 = 0U;
volatile uint32_t g_camera_tx_4801 = 0U;
volatile uint32_t g_camera_tx_4805 = 0U;
volatile uint32_t g_camera_tx_4814 = 0U;
volatile uint32_t g_camera_tx_4837 = 0U;

volatile fsp_err_t g_camera_tx_read_err = FSP_SUCCESS;

volatile int32_t g_camera_rx_irq = -1;
volatile uint32_t g_camera_rx_ipl = 0U;
volatile uint32_t g_camera_rx_nvic_enabled = 0U;
volatile uint32_t g_camera_rx_nvic_pending = 0U;
volatile uint32_t g_camera_basepri = 0U;

static void
camera_diag_read_sensor_reg(
    uint16_t reg,
    volatile uint32_t * destination
);

static void
camera_mipi_trace_save(
    uint32_t checkpoint
);

/*
 * Build a runtime copy of the generated VIN
 * configuration.
 *
 * The generated VIN buffer keeps:
 *
 *   image stride = 1024 pixels
 *   bytes/line   = 2048 bytes
 *
 * while VIN scaling reduces the active image to
 * 224 x 168 RGB565.
 */
static fsp_err_t
camera_capture_prepare_config(void)
{
    if (NULL ==
        g_vin0_cfg.p_extend)
    {
        return
            FSP_ERR_INVALID_ARGUMENT;
    }


    vin_extended_cfg_t const * const
        base_extend =
            (vin_extended_cfg_t const *)
            g_vin0_cfg.p_extend;


    s_vin_extend =
        *base_extend;


    uint32_t const old_v =
        (uint32_t)
        base_extend
            ->conversion_data
            .uds_clipping_bits
            .cl_vsize;

    uint32_t const old_h =
        (uint32_t)
        base_extend
            ->conversion_data
            .uds_clipping_bits
            .cl_hsize;

    uint16_t const old_vm =
        (uint16_t)
        base_extend
            ->conversion_data
            .uds_scale_bits
            .vertical_mask;

    uint16_t const old_hm =
        (uint16_t)
        base_extend
            ->conversion_data
            .uds_scale_bits
            .horizontal_mask;


    if ((0U == old_v) ||
        (0U == old_h))
    {
        return
            FSP_ERR_INVALID_ARGUMENT;
    }


    /*
     * Same runtime scaling calculation used by the
     * verified Renesas camera_lcd project.
     */
    s_vin_extend
        .conversion_data
        .uds_scale_bits
        .vertical_mask =
            (uint16_t)
            (
                ((uint32_t) old_vm *
                 old_v) /
                CAMERA_CAPTURE_HEIGHT
            );


    s_vin_extend
        .conversion_data
        .uds_scale_bits
        .horizontal_mask =
            (uint16_t)
            (
                ((uint32_t) old_hm *
                 old_h) /
                CAMERA_CAPTURE_WIDTH
            );


    s_vin_extend
        .conversion_data
        .uds_clipping_bits
        .cl_vsize =
            CAMERA_CAPTURE_HEIGHT;


    s_vin_extend
        .conversion_data
        .uds_clipping_bits
        .cl_hsize =
            CAMERA_CAPTURE_WIDTH;


    s_vin_extend
        .input_ctrl
        .cfg_bits
        .scaling_enable =
            true;


    /*
     * Copy all generated VIN settings including
     * callbacks and buffer configuration.
     */
    s_vin_cfg =
        g_vin0_cfg;


    s_vin_cfg.p_extend =
        &s_vin_extend;


    return
        FSP_SUCCESS;
}


/*
 * VIN interrupt callback generated in common_data.h.
 *
 * Do not process image data inside the ISR.
 */
void vin0_callback(
    capture_callback_args_t * p_args)
{
    if (NULL == p_args)
    {
        return;
    }

    g_camera_vin_callback_count++;

    g_camera_vin_last_interrupt_status =
        p_args->interrupt_status;

    g_camera_vin_last_event_status =
        p_args->event_status;


    vin_module_status_t const module_status =
        (vin_module_status_t)
        p_args->event_status;

    vin_interrupt_status_t const interrupt_status =
        (vin_interrupt_status_t)
        p_args->interrupt_status;

    FSP_PARAMETER_NOT_USED(module_status);

    switch (p_args->event)
    {
        case VIN_EVENT_NOTIFY:
        {
            if (interrupt_status.bits.frame_complete)
            {
                g_camera_vin_last_buffer =
                    (uintptr_t) p_args->p_buffer;

                g_camera_vin_frame_count++;


                /*
                * Live mode:
                * ISRではcompleted bufferの通知だけを行う。
                * Frame Cacheへのコピーはtask側で実行する。
                */
                if (0U != s_camera_live_enabled)
                {
                    s_camera_live_completed_buffer =
                        (uintptr_t) p_args->p_buffer;

                    __DMB();

                    g_camera_source_irq_count++;
                }
            }

            break;
        }

        case VIN_EVENT_ERROR:
        {
            g_camera_vin_error_count++;
            break;
        }

        default:
        {
            break;
        }
    }
}


/*
 * MIPI callback required by the generated FSP stack.
 * VIN is responsible for completed-frame handling.
 */
void mipi_csi0_callback(
    mipi_csi_callback_args_t * p_args)
{
    if (NULL == p_args)
    {
        return;
    }

    g_camera_mipi_event_count++;

    g_camera_mipi_last_event =
        (uint32_t) p_args->event;
}


static uint32_t
camera_capture_checksum(
    uint8_t const * const frame)
{
    if (NULL ==
        frame)
    {
        return 0U;
    }


    uint32_t checksum =
        2166136261U;


    for (uint32_t y = 0U;
         y < CAMERA_CAPTURE_HEIGHT;
         y++)
    {
        uint8_t const * const
            row =
                frame +
                (
                    y *
                    VIN_CFG_BYTES_PER_LINE
                );


        for (uint32_t x = 0U;
             x < CAMERA_ACTIVE_LINE_BYTES;
             x++)
        {
            checksum ^=
                row[x];

            checksum *=
                16777619U;
        }
    }


    return
        checksum;
}


static uint32_t
camera_full_buffer_checksum(
    uint8_t const * const buffer)
{
    uint32_t checksum =
        2166136261U;

    for (uint32_t i = 0U;
         i < VIN_BYTES_PER_FRAME;
         i++)
    {
        checksum ^=
            buffer[i];

        checksum *=
            16777619U;
    }

    return checksum;
}


static void
camera_mipi_trace_save(
    uint32_t checkpoint
)
{
    if (checkpoint >=
        (uint32_t) CAMERA_MIPI_TRACE_COUNT)
    {
        return;
    }


    volatile camera_mipi_trace_snapshot_t * const snapshot =
        &g_camera_mipi_trace[checkpoint];


    snapshot->checkpoint =
        checkpoint;

    snapshot->phy_dphymdc =
        R_MIPI_PHY->DPHYMDC;

    snapshot->phy_refcr =
        R_MIPI_PHY->DPHYREFCR;

    snapshot->phy_pwrcr =
        R_MIPI_PHY->DPHYPWRCR;

    snapshot->phy_ocr =
        R_MIPI_PHY->DPHYOCR;

    snapshot->phy_sfr =
        R_MIPI_PHY->DPHYSFR;

    snapshot->csi_mct0 =
        R_MIPI_CSI->MCT0;

    snapshot->csi_mct2 =
        R_MIPI_CSI->MCT2;

    snapshot->csi_mct3 =
        R_MIPI_CSI->MCT3;

    snapshot->csi_rxst =
        R_MIPI_CSI->RXST;

    snapshot->csi_dlst0 =
        R_MIPI_CSI->DLST0;

    snapshot->csi_dlst1 =
        R_MIPI_CSI->DLST1;

    snapshot->csi_mist =
        R_MIPI_CSI->MIST;

    snapshot->csi_vcst0 =
        R_MIPI_CSI->VCST0;

    snapshot->csi_pmst =
        R_MIPI_CSI->PMST;

    snapshot->vin_mc =
        R_VIN->MC;

    snapshot->vin_fc =
        R_VIN->FC;

    snapshot->vin_ms =
        R_VIN->MS;

    snapshot->vin_lc =
        R_VIN->LC;

    snapshot->p411_pfs =
        R_PFS->PORT[4].PIN[11].PmnPFS;

    snapshot->p501_pfs =
        R_PFS->PORT[5].PIN[1].PmnPFS;

    snapshot->p511_pfs =
        R_PFS->PORT[5].PIN[11].PmnPFS;

    snapshot->p512_pfs =
        R_PFS->PORT[5].PIN[12].PmnPFS;

    snapshot->p709_pfs =
        R_PFS->PORT[7].PIN[9].PmnPFS;


    g_camera_mipi_trace_valid_mask |=
        (1UL << checkpoint);
}


fsp_err_t camera_capture_start_live(void)
{
    fsp_err_t err;


    /*
     * Build the verified 224 x 168 runtime VIN configuration.
     */
    err =
        camera_capture_prepare_config();

    if (FSP_SUCCESS != err)
    {
        return err;
    }


    /*
     * Ensure a known sensor state.
     */
    err =
        camera_stream_off();

    if (FSP_SUCCESS != err)
    {
        return err;
    }


    err =
        write_reg_16bit(
            SYS_CTRL0_REG,
            SYS_CTRL0_SW_PWDN
        );

    if (FSP_SUCCESS != err)
    {
        return err;
    }


    /*
     * Normally closed here because probe mode is no longer used
     * in the live-camera startup path.
     */
    if (0U != g_vin0_ctrl.open)
    {
        (void)
        R_VIN_Close(
            &g_vin0_ctrl
        );
    }


    g_camera_vin_frame_count = 0U;
    g_camera_vin_error_count = 0U;
    g_camera_vin_last_buffer = (uintptr_t) 0U;

    g_camera_source_irq_count = 0U;
    g_camera_source_publish_count = 0U;
    g_camera_source_drop_count = 0U;
    g_camera_source_copy_count = 0U;

    g_camera_source_last_frame_index =
        UINT32_MAX;

    g_camera_source_last_vin_buffer =
        (uintptr_t) 0U;

    g_camera_source_last_cache_slot =
        VIDEO_SOURCE_INVALID_SLOT;

    s_camera_live_completed_buffer =
        (uintptr_t) 0U;


    g_camera_vin_open_err =
        R_VIN_Open(
            &g_vin0_ctrl,
            &s_vin_cfg
        );

    if (FSP_SUCCESS !=
        g_camera_vin_open_err)
    {
        return
            g_camera_vin_open_err;
    }


    g_camera_vin_start_err =
        R_VIN_CaptureStart(
            &g_vin0_ctrl,
            NULL
        );

    if (FSP_SUCCESS !=
        g_camera_vin_start_err)
    {
        (void)
        R_VIN_Close(
            &g_vin0_ctrl
        );

        return
            g_camera_vin_start_err;
    }


    /*
     * Enable ISR -> camera task notification before stream starts.
     */
    s_camera_live_enabled =
        1U;

    __DMB();


    g_camera_vin_stream_err =
        camera_stream_on();

    if (FSP_SUCCESS !=
        g_camera_vin_stream_err)
    {
        s_camera_live_enabled =
            0U;

        (void)
        R_VIN_Close(
            &g_vin0_ctrl
        );

        return
            g_camera_vin_stream_err;
    }


    tm_putstring(
        (UB *)"[Camera] live capture started: "
               "224x168 RGB565 stride=2048.\n"
    );


    return
        FSP_SUCCESS;
}


fsp_err_t camera_capture_stop_live(void)
{
    s_camera_live_enabled =
        0U;

    __DMB();


    fsp_err_t const stream_err =
        camera_stream_off();


    if (0U != g_vin0_ctrl.open)
    {
        (void)
        R_VIN_Close(
            &g_vin0_ctrl
        );
    }


    return
        stream_err;
}


ER camera_source_create(void)
{
    ID const task_id =
        tk_cre_tsk(
            &s_camera_source_task_ctsk
        );

    if (task_id < E_OK)
    {
        return
            (ER) task_id;
    }


    s_camera_source_task_id =
        task_id;


    return
        E_OK;
}


ER camera_source_start(void)
{
    if (s_camera_source_task_id <= 0)
    {
        return
            E_ID;
    }


    return
        tk_sta_tsk(
            s_camera_source_task_id,
            0
        );
}


static void camera_source_task_entry(
    INT stacd,
    void * exinf)
{
    (void) stacd;
    (void) exinf;


    uint32_t consumed_irq_count =
        0U;


    tm_putstring(
        (UB *)"[CameraSource] task started.\n"
    );


    while (1)
    {
        g_camera_source_loop_count++;

        uint32_t const irq_count =
            g_camera_source_irq_count;


        if (irq_count ==
            consumed_irq_count)
        {
            g_camera_source_idle_count++;

            tk_dly_tsk(1);
            continue;
        }

        g_camera_source_new_irq_count++;


        /*
         * More than one camera frame arrived before this task
         * handled the previous one.  Older frames are intentionally
         * discarded: Camera is a real-time latest-frame producer.
         */
        if (irq_count >
            (consumed_irq_count + 1U))
        {
            g_camera_source_drop_count +=
                irq_count -
                consumed_irq_count -
                1U;
        }


        __DMB();


        uintptr_t const vin_buffer_address =
            s_camera_live_completed_buffer;


        /*
         * If ISR updated the completed buffer while sampling it,
         * retry and use the newest frame.
         */
        if (irq_count !=
            g_camera_source_irq_count)
        {
            continue;
        }


        consumed_irq_count =
            irq_count;


        if (irq_count !=
            g_camera_source_irq_count)
        {
            g_camera_source_retry_count++;
            continue;
        }


        uint32_t slot =
            VIDEO_SOURCE_INVALID_SLOT;


        g_camera_source_acquire_attempt_count++;

        uint8_t * const cache_buffer =
            video_source_acquire_write_buffer(
                &slot
            );


        /*
         * No back-pressure for a physical camera.
         *
         * If all Frame Cache slots are still owned by consumers,
         * discard this input frame and continue receiving.
         */
        if (NULL ==
            cache_buffer)
        {
            g_camera_source_drop_count++;
            continue;
        }


        uint8_t const * const vin_buffer =
            (uint8_t const *)
            vin_buffer_address;


        /*
         * VIN is a DMA writer.
         * Remove stale CPU cache lines before CPU reads the frame.
         */
#if BSP_CFG_DCACHE_ENABLED

        SCB_InvalidateDCache_by_Addr(
            (void *) vin_buffer,
            (int32_t) VIDEO_SOURCE_FRAME_BYTES
        );

#endif


        /*
         * VIN and Frame Cache have the same physical layout:
         *
         * active = 448 bytes/line
         * stride = 2048 bytes
         * height = 168
         *
         * Padding does not contain image data and is not copied.
         */
        for (uint32_t y = 0U;
             y < VIDEO_SOURCE_HEIGHT;
             y++)
        {
            memcpy(
                cache_buffer +
                    (y * VIDEO_SOURCE_STRIDE_BYTES),

                vin_buffer +
                    (y * VIDEO_SOURCE_STRIDE_BYTES),

                VIDEO_SOURCE_ACTIVE_LINE_BYTES
            );
        }


        uint32_t const frame_index =
            irq_count - 1U;


        video_source_publish_write_buffer(
            slot,
            frame_index
        );


        g_camera_source_copy_count++;
        g_camera_source_publish_count++;

        g_camera_source_last_frame_index =
            frame_index;

        g_camera_source_last_vin_buffer =
            vin_buffer_address;

        g_camera_source_last_cache_slot =
            slot;
    }
}


fsp_err_t camera_capture_probe(void)
{
    fsp_err_t err;


    g_camera_mipi_trace_magic =
        0x4D495031U;

    g_camera_mipi_trace_valid_mask =
        0U;

    g_camera_mipi_trace_complete =
        0U;


    g_camera_vin_probe_stage =
        10U;


    g_camera_vin_frame_count =
        0U;

    g_camera_vin_last_buffer =
        (uintptr_t) 0U;

    g_camera_vin_error_count =
        0U;

    g_camera_vin_checksum =
        0U;


    tm_putstring(
        (UB *) "[Camera][VIN] probe start.\n"
    );


    /*
     * Stage 20:
     * camera_stream_off() entry.
     */
    g_camera_vin_probe_stage =
        20U;


    err =
        camera_stream_off();


    /*
     * Stage 21:
     * camera_stream_off() returned.
     */
    g_camera_vin_probe_stage =
        21U;


    if (FSP_SUCCESS != err)
    {
        tm_printf(
            (UB *) "[Camera][VIN] stream off failed: %d\n",
            err
        );

        return err;
    }

    /*
    * Match the known-good camera_lcd startup sequence exactly.
    *
    * camera_stream_off() only disables streaming through 0x4202.
    * The Renesas reference additionally places OV5640 into
    * software power-down before VIN/MIPI initialization.
    */
    g_camera_vin_probe_stage =
        25U;


    err =
        write_reg_16bit(
            SYS_CTRL0_REG,
            SYS_CTRL0_SW_PWDN
        );


    if (FSP_SUCCESS != err)
    {
        tm_printf(
            (UB *)"[Camera][VIN] sensor power-down failed: %d\n",
            err
        );

        return err;
    }


    g_camera_vin_probe_stage =
        26U;

    /*
    * CP-CAM2-A
    *
    * First verify the exact Renesas reference path:
    *
    * OV5640 1024x600
    * -> MIPI CSI
    * -> VIN
    * -> SDRAM
    *
    * Do NOT apply runtime scaling yet.
    */
    g_camera_vin_probe_stage =
        30U;


    g_camera_vin_probe_stage =
        40U;


    g_camera_vin_open_err =
        R_VIN_Open(
            &g_vin0_ctrl,
            &g_vin0_cfg
        );


    g_camera_vin_probe_stage =
        41U;


    if (FSP_SUCCESS !=
        g_camera_vin_open_err)
    {
        tm_printf(
            (UB *)"[Camera][VIN] open failed: %d\n",
            g_camera_vin_open_err
        );

        return
            g_camera_vin_open_err;
    }


    camera_mipi_trace_save(
        CAMERA_MIPI_TRACE_AFTER_VIN_OPEN
    );


    /*
     * Stage 41:
     * R_VIN_Open() returned.
     */
    g_camera_vin_probe_stage =
        41U;


    if (FSP_SUCCESS !=
        g_camera_vin_open_err)
    {
        tm_printf(
            (UB *) "[Camera][VIN] open failed: %d\n",
            g_camera_vin_open_err
        );

        return
            g_camera_vin_open_err;
    }


    /*
     * Stage 50:
     * entering R_VIN_CaptureStart().
     */
    g_camera_vin_probe_stage =
        50U;
    

    g_camera_vin_buf1_before =
        camera_full_buffer_checksum(
            vin_image_buffer_1
        );

    g_camera_vin_buf2_before =
        camera_full_buffer_checksum(
            vin_image_buffer_2
        );

    g_camera_vin_buf3_before =
        camera_full_buffer_checksum(
            vin_image_buffer_3
        );


    g_camera_vin_start_err =
        R_VIN_CaptureStart(
            &g_vin0_ctrl,
            NULL
        );


    /*
     * Stage 51:
     * capture start returned.
     */
    g_camera_vin_probe_stage =
        51U;


    if (FSP_SUCCESS !=
        g_camera_vin_start_err)
    {
        tm_printf(
            (UB *) "[Camera][VIN] capture start failed: %d\n",
            g_camera_vin_start_err
        );

        (void)
        R_VIN_Close(
            &g_vin0_ctrl
        );

        return
            g_camera_vin_start_err;
    }


    camera_mipi_trace_save(
        CAMERA_MIPI_TRACE_AFTER_CAPTURE_START
    );


    /*
     * Stage 60:
     * entering camera_stream_on().
     */
    g_camera_vin_probe_stage =
        60U;


    camera_mipi_trace_save(
        CAMERA_MIPI_TRACE_BEFORE_STREAM_ON
    );


    g_camera_vin_stream_err =
        camera_stream_on();

    camera_mipi_trace_save(
        CAMERA_MIPI_TRACE_AFTER_STREAM_ON
    );

    g_camera_rx_irq =
        (int32_t)
        g_mipi_csi0_cfg
            .interrupt_cfg
            .receive_cfg
            .irq;

    g_camera_rx_ipl =
        (uint32_t)
        g_mipi_csi0_cfg
            .interrupt_cfg
            .receive_cfg
            .ipl;

    g_camera_rx_nvic_enabled =
        NVIC_GetEnableIRQ(
            g_mipi_csi0_cfg
                .interrupt_cfg
                .receive_cfg
                .irq
        );

    g_camera_rx_nvic_pending =
        NVIC_GetPendingIRQ(
            g_mipi_csi0_cfg
                .interrupt_cfg
                .receive_cfg
                .irq
        );

    g_camera_basepri =
        __get_BASEPRI();


    /*
     * Stage 61:
     * sensor streaming started.
     */
    g_camera_vin_probe_stage =
        61U;


    if (FSP_SUCCESS !=
        g_camera_vin_stream_err)
    {
        tm_printf(
            (UB *) "[Camera][VIN] sensor stream failed: %d\n",
            g_camera_vin_stream_err
        );

        (void)
        R_VIN_Close(
            &g_vin0_ctrl
        );

        return
            g_camera_vin_stream_err;
    }

    /*
     * Preserve the original 100 ms total delay while recording
     * when MIPI activity first becomes visible to the receiver.
     */
    R_BSP_SoftwareDelay(
        1U,
        BSP_DELAY_UNITS_MILLISECONDS
    );

    camera_mipi_trace_save(
        CAMERA_MIPI_TRACE_AFTER_1_MS
    );

    R_BSP_SoftwareDelay(
        9U,
        BSP_DELAY_UNITS_MILLISECONDS
    );

    camera_mipi_trace_save(
        CAMERA_MIPI_TRACE_AFTER_10_MS
    );

    R_BSP_SoftwareDelay(
        90U,
        BSP_DELAY_UNITS_MILLISECONDS
    );

    camera_mipi_trace_save(
        CAMERA_MIPI_TRACE_AFTER_100_MS
    );

    g_camera_mipi_trace_complete =
        1U;

    /*
    * Check the actual OV5640 state after stream-on.
    */
    g_camera_sensor_readback_err =
        FSP_SUCCESS;

    g_camera_sensor_readback_fail_reg =
        0U;


    camera_diag_read_sensor_reg(
        0x3008U,
        &g_camera_sensor_reg_3008
    );

    camera_diag_read_sensor_reg(
        0x4202U,
        &g_camera_sensor_reg_4202
    );

    camera_diag_read_sensor_reg(
        0x300EU,
        &g_camera_sensor_reg_300e
    );

    camera_diag_read_sensor_reg(
        0x3035U,
        &g_camera_sensor_reg_3035
    );

    camera_diag_read_sensor_reg(
        0x3036U,
        &g_camera_sensor_reg_3036
    );

    camera_diag_read_sensor_reg(
        0x3037U,
        &g_camera_sensor_reg_3037
    );

    camera_diag_read_sensor_reg(
        0x4814U,
        &g_camera_sensor_reg_4814
    );

    (void)
    camera_sensor_trace_readback();

    g_camera_cmp_phy_dphymdc =
        R_MIPI_PHY->DPHYMDC;

    g_camera_cmp_phy_refcr =
        R_MIPI_PHY->DPHYREFCR;

    g_camera_cmp_phy_tim1 =
        R_MIPI_PHY->DPHYTIM1;

    g_camera_cmp_phy_tim2 =
        R_MIPI_PHY->DPHYTIM2;

    g_camera_cmp_phy_tim3 =
        R_MIPI_PHY->DPHYTIM3;

    g_camera_cmp_phy_tim4 =
        R_MIPI_PHY->DPHYTIM4;

    g_camera_cmp_phy_tim5 =
        R_MIPI_PHY->DPHYTIM5;

    g_camera_cmp_phy_tim6 =
        R_MIPI_PHY->DPHYTIM6;


    g_camera_cmp_csi_mct0 =
        R_MIPI_CSI->MCT0;

    g_camera_cmp_csi_mct2 =
        R_MIPI_CSI->MCT2;

    g_camera_cmp_csi_mct3 =
        R_MIPI_CSI->MCT3;

    g_camera_cmp_csi_epct =
        R_MIPI_CSI->EPCT;

    g_camera_cmp_csi_emct =
        R_MIPI_CSI->EMCT;

    g_camera_cmp_csi_dtel =
        R_MIPI_CSI->DTEL;

    g_camera_cmp_csi_dteh =
        R_MIPI_CSI->DTEH;

    g_camera_cmp_csi_gsct =
        R_MIPI_CSI->GSCT;

    g_camera_cmp_vin_mc =
            R_VIN->MC;

    g_camera_cmp_vin_fc =
        R_VIN->FC;
    
    g_camera_cmp_csi_dlst0 =
        R_MIPI_CSI->DLST0;

    g_camera_cmp_csi_dlst1 =
        R_MIPI_CSI->DLST1;

    g_camera_cmp_csi_mist =
        R_MIPI_CSI->MIST;

    g_camera_cmp_csi_vcst0 =
        R_MIPI_CSI->VCST0;

    g_camera_cmp_csi_pmst =
        R_MIPI_CSI->PMST;

    uint32_t previous =
        R_PFS->PORT[5]
             .PIN[1]
             .PmnPFS_b.PIDR;

    for (uint32_t i = 0U;
         i < 8192U;
         i++)
    {
        uint32_t const current =
            R_PFS->PORT[5]
                 .PIN[1]
                 .PmnPFS_b.PIDR;

        if (0U == current)
        {
            g_camera_xclk_seen_low = 1U;
        }
        else
        {
            g_camera_xclk_seen_high = 1U;
        }

        if (current != previous)
        {
            g_camera_xclk_transitions++;
        }

        previous = current;
    }

    uint8_t value = 0U;


    g_camera_switch_addr_diag_err =
        R_IIC_MASTER_SlaveAddressSet(
            &g_i2c_master_for_peripheral_ctrl,
            SWITCH_ADDR,
            I2C_MASTER_ADDR_MODE_7BIT
        );


    if (FSP_SUCCESS ==
        g_camera_switch_addr_diag_err)
    {
        g_camera_switch_dir_diag_err =
            read_reg_8bit(
                PIN_DIR_REG,
                &value
            );

        if (FSP_SUCCESS ==
            g_camera_switch_dir_diag_err)
        {
            g_camera_switch_dir_diag =
                value;
        }


        value = 0U;

        g_camera_switch_state_diag_err =
            read_reg_8bit(
                OUTPUT_STATE_REG,
                &value
            );

        if (FSP_SUCCESS ==
            g_camera_switch_state_diag_err)
        {
            g_camera_switch_state_diag =
                value;
        }


        value = 0U;

        g_camera_switch_enable_diag_err =
            read_reg_8bit(
                OUTPUT_ENABLE_REG,
                &value
            );

        if (FSP_SUCCESS ==
            g_camera_switch_enable_diag_err)
        {
            g_camera_switch_enable_diag =
                value;
        }
    }


    g_camera_switch_restore_diag_err =
        R_IIC_MASTER_SlaveAddressSet(
            &g_i2c_master_for_peripheral_ctrl,
            REG_CAM_I2C_SLAVE_ADDR,
            I2C_MASTER_ADDR_MODE_7BIT
        );

    g_camera_tx_read_err = read_reg_16bit(0x300EU, &value);
    if (FSP_SUCCESS == g_camera_tx_read_err)
    {
        g_camera_tx_300e = value;
    }

    value = 0U;
    g_camera_tx_read_err = read_reg_16bit(0x3019U, &value);
    if (FSP_SUCCESS == g_camera_tx_read_err)
    {
        g_camera_tx_3019 = value;
    }

    value = 0U;
    g_camera_tx_read_err = read_reg_16bit(0x4800U, &value);
    if (FSP_SUCCESS == g_camera_tx_read_err)
    {
        g_camera_tx_4800 = value;
    }

    value = 0U;
    g_camera_tx_read_err = read_reg_16bit(0x4801U, &value);
    if (FSP_SUCCESS == g_camera_tx_read_err)
    {
        g_camera_tx_4801 = value;
    }

    value = 0U;
    g_camera_tx_read_err = read_reg_16bit(0x4805U, &value);
    if (FSP_SUCCESS == g_camera_tx_read_err)
    {
        g_camera_tx_4805 = value;
    }

    value = 0U;
    g_camera_tx_read_err = read_reg_16bit(0x4814U, &value);
    if (FSP_SUCCESS == g_camera_tx_read_err)
    {
        g_camera_tx_4814 = value;
    }

    value = 0U;
    g_camera_tx_read_err = read_reg_16bit(0x4837U, &value);
    if (FSP_SUCCESS == g_camera_tx_read_err)
    {
        g_camera_tx_4837 = value;
    }


    /*
    * OV5640 reset pin.
    */
    g_camera_reset_pin_level =
        R_BSP_PinRead(
            PIN_CAM_RESET
        );


    /*
    * GPT12 actual hardware registers.
    */
    uintptr_t const gpt_base =
        (uintptr_t) R_GPT0;

    uintptr_t const gpt_stride =
        (uintptr_t) R_GPT1 -
        (uintptr_t) R_GPT0;

    R_GPT0_Type * const gpt12 =
        (R_GPT0_Type *)
        (
            gpt_base +
            (
                12U *
                gpt_stride
            )
        );


    g_camera_gpt12_gtcr =
        gpt12->GTCR;

    g_camera_gpt12_gtior =
        gpt12->GTIOR;

    g_camera_gpt12_gtpr =
        gpt12->GTPR;


    /*
    * P501 = CAM_XCLK.
    */
    g_camera_p501_pfs =
        R_PFS
            ->PORT[5]
            .PIN[1]
            .PmnPFS;

    /*
    * CAM_XCLK actual timer configuration.
    */
    timer_info_t xclk_info =
    {
        0
    };


    g_camera_xclk_info_err =
        R_GPT_InfoGet(
            &g_timer_periodic_ctrl,
            &xclk_info
        );


    if (FSP_SUCCESS ==
        g_camera_xclk_info_err)
    {
        g_camera_xclk_clock_hz =
            xclk_info.clock_frequency;

        g_camera_xclk_period_counts =
            xclk_info.period_counts;


        if (0U !=
            xclk_info.period_counts)
        {
            g_camera_xclk_output_hz =
                xclk_info.clock_frequency /
                xclk_info.period_counts;
        }
    }


    /*
    * MIPI D-PHY actual state.
    */
    g_camera_hw_phy_pwrsen =
        R_MIPI_PHY->DPHYPWRCR_b.PWRSEN;

    g_camera_hw_phy_dphyen =
        R_MIPI_PHY->DPHYOCR_b.DPHYEN;

    g_camera_hw_phy_status =
        R_MIPI_PHY->DPHYSFR;


    /*
    * MIPI CSI actual state.
    *
    * Do not call R_MIPI_CSI_StatusGet() here because it
    * clears the RX-active-detect flag. Read the register
    * directly for this diagnostic checkpoint.
    */
    g_camera_hw_csi_rxen =
        R_MIPI_CSI->MCT3_b.RXEN;

    g_camera_hw_csi_rxst =
        R_MIPI_CSI->RXST;

    g_camera_hw_csi_ractdet =
        R_MIPI_CSI->RXST_b.RACTDET;


    /*
    * VIN actual state.
    */
    g_camera_hw_vin_me =
        R_VIN->MC_b.ME;

    g_camera_hw_vin_cc =
        R_VIN->FC_b.CC;

    g_camera_hw_vin_ms =
        R_VIN->MS;

    g_camera_hw_vin_ca =
        R_VIN->MS_b.CA;

    g_camera_hw_vin_lc =
        R_VIN->LC;


    g_camera_vin_probe_stage =
        65U;


    /*
     * Stage 70:
     * waiting for VIN frames.
     */
    g_camera_vin_probe_stage =
        70U;


    for (uint32_t elapsed_ms = 0U;
        (elapsed_ms < 1000U) &&
        (g_camera_vin_frame_count < 5U);
        elapsed_ms += 10U)
    {
        R_BSP_SoftwareDelay(
            10U,
            BSP_DELAY_UNITS_MILLISECONDS
        );
    }


    g_camera_vin_probe_stage =
        80U;


    err =
        camera_stream_off();
    
    R_BSP_SoftwareDelay(
        20U,
        BSP_DELAY_UNITS_MILLISECONDS
    );


    /*
    * VIN writes SDRAM directly.
    * CPU0 D-cache is enabled, so discard potentially stale
    * cache lines before reading the DMA-written buffers.
    */
    #if BSP_CFG_DCACHE_ENABLED

    SCB_InvalidateDCache_by_Addr(
        (void *) vin_image_buffer_1,
        (int32_t) VIN_BYTES_PER_FRAME
    );

    SCB_InvalidateDCache_by_Addr(
        (void *) vin_image_buffer_2,
        (int32_t) VIN_BYTES_PER_FRAME
    );

    SCB_InvalidateDCache_by_Addr(
        (void *) vin_image_buffer_3,
        (int32_t) VIN_BYTES_PER_FRAME
    );

    #endif


    g_camera_vin_buf1_after =
        camera_full_buffer_checksum(
            vin_image_buffer_1
        );

    g_camera_vin_buf2_after =
        camera_full_buffer_checksum(
            vin_image_buffer_2
        );

    g_camera_vin_buf3_after =
        camera_full_buffer_checksum(
            vin_image_buffer_3
        );


    if ((g_camera_vin_buf1_before !=
        g_camera_vin_buf1_after) ||

        (g_camera_vin_buf2_before !=
        g_camera_vin_buf2_after) ||

        (g_camera_vin_buf3_before !=
        g_camera_vin_buf3_after))
    {
        g_camera_vin_buffer_write_detected =
            1U;
    }
    else
    {
        g_camera_vin_buffer_write_detected =
            0U;
    }


    if (FSP_SUCCESS != err)
    {
        tm_printf(
            (UB *) "[Camera][VIN] final stream off failed: %d\n",
            err
        );
    }


    tk_dly_tsk(20);


    g_camera_vin_probe_stage =
        90U;


    if (0U !=
        g_vin0_ctrl.open)
    {
        (void)
        R_VIN_Close(
            &g_vin0_ctrl
        );
    }


    if ((0U ==
         g_camera_vin_frame_count) ||
        ((uintptr_t) 0U ==
         g_camera_vin_last_buffer))
    {
        tm_printf(
            (UB *) "[Camera][VIN] no frame. "
                   "count=%u errors=%u\n",
            g_camera_vin_frame_count,
            g_camera_vin_error_count
        );

        return
            FSP_ERR_TIMEOUT;
    }


    g_camera_vin_checksum =
        camera_capture_checksum(
            (uint8_t const *)
            g_camera_vin_last_buffer
        );


    g_camera_vin_probe_stage =
        100U;


    tm_printf(
        (UB *) "[Camera][VIN] OK "
               "frames=%u "
               "buffer=0x%08X "
               "checksum=0x%08X "
               "errors=%u\n",
        g_camera_vin_frame_count,
        (uint32_t)
        g_camera_vin_last_buffer,
        g_camera_vin_checksum,
        g_camera_vin_error_count
    );


    return
        FSP_SUCCESS;
}


static void
camera_diag_read_sensor_reg(
    uint16_t const reg,
    volatile uint32_t * const destination)
{
    uint8_t value = 0U;

    fsp_err_t const err =
        read_reg_16bit(
            reg,
            &value
        );

    if (FSP_SUCCESS == err)
    {
        *destination =
            (uint32_t) value;
    }
    else if (0U ==
             g_camera_sensor_readback_fail_reg)
    {
        g_camera_sensor_readback_err =
            err;

        g_camera_sensor_readback_fail_reg =
            (uint32_t) reg;
    }
}
