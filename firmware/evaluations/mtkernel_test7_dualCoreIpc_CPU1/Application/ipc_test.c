#include <tk/tkernel.h>
#include "hal_data.h"
#include "ipc_test.h"
#include "shared_ipc_protocol.h"
#include "benchmark_clock.h"
#include "gvd_output.h"

#define IPC_TEST_REQUEST   (0x12345678UL)
#define IPC_TEST_ACK       (0xA5A5A5A5UL)

volatile uint32_t g_cpu1_ipc_rx_value = 0U;
volatile uint32_t g_cpu1_ipc_rx_count = 0U;
volatile uint32_t g_cpu1_ipc_ack_count = 0U;

volatile fsp_err_t g_cpu1_ipc_init_err = FSP_SUCCESS;
volatile fsp_err_t g_cpu1_ipc_send_err = FSP_SUCCESS;

static ID s_ipc_task_id;
static volatile uint32_t s_ack_pending = 0U;
static volatile uint32_t s_perception_pending = 0U;

perception_snapshot_t g_cpu1_perception_latest;

volatile uint32_t g_cpu1_perception_rx_count = 0U;
volatile uint32_t g_cpu1_perception_last_sequence = 0U;
volatile uint32_t g_cpu1_perception_invalid_count = 0U;

/*
 * True fast-path E2E latency measured with the common GPT13:
 *
 * CPU0 Frame Cache publish
 *     -> Motion
 *     -> Perception Bank
 *     -> IPC
 *     -> CPU1 validated snapshot ready
 */
volatile uint32_t g_cpu1_e2e_latency_count = 0U;
volatile uint32_t g_cpu1_e2e_latency_last_ticks = 0U;
volatile uint64_t g_cpu1_e2e_latency_total_ticks = 0ULL;
volatile uint32_t g_cpu1_e2e_latency_min_ticks = UINT32_MAX;
volatile uint32_t g_cpu1_e2e_latency_max_ticks = 0U;
volatile uint32_t g_cpu1_e2e_latency_last_sequence = 0U;

volatile uint32_t g_cpu1_ipc_dispatch_latency_count = 0U;
volatile uint32_t g_cpu1_ipc_dispatch_latency_last_ms = 0U;
volatile uint32_t g_cpu1_ipc_dispatch_latency_total_ms = 0U;
volatile uint32_t g_cpu1_ipc_dispatch_latency_min_ms = UINT32_MAX;
volatile uint32_t g_cpu1_ipc_dispatch_latency_max_ms = 0U;
/*
 * Timestamp captured immediately when the IPC interrupt/callback arrives.
 */
static volatile uint32_t s_perception_ipc_arrival_tick_ms = 0U;

volatile uint32_t g_cpu1_shared_request = 0U;
volatile uint32_t g_cpu1_shared_owner = 0U;
volatile uint32_t g_cpu1_shared_sequence = 0U;
volatile uint32_t g_cpu1_shared_pass = 0U;

static void ipc_test_task_entry(INT stacd, void *exinf)
{
    FSP_PARAMETER_NOT_USED(stacd);
    FSP_PARAMETER_NOT_USED(exinf);

    while (1)
    {
        if (0U != s_ack_pending)
        {
            volatile shared_sdram_mailbox_t * const p_shared =
                SHARED_SDRAM_MAILBOX;

            s_ack_pending = 0U;

            #if BSP_CFG_DCACHE_ENABLED

            SCB_InvalidateDCache_by_Addr(
                (volatile void *) p_shared,
                (int32_t) sizeof(shared_sdram_mailbox_t)
            );

            #endif

            __DMB();


            g_cpu1_shared_request =
                p_shared->request_value;

            g_cpu1_shared_owner =
                p_shared->owner;

            g_cpu1_shared_sequence =
                p_shared->sequence;

            if ((SHARED_SDRAM_MAGIC == p_shared->magic) &&
                (SHARED_OWNER_CPU1 == p_shared->owner) &&
                (1U == p_shared->sequence) &&
                (SHARED_TEST_SLOT == p_shared->slot) &&
                (SHARED_TEST_REQUEST_VALUE == p_shared->request_value))
            {
                g_cpu1_shared_pass = 1U;

                p_shared->response_value =
                    SHARED_TEST_RESPONSE_VALUE;

                p_shared->sequence =
                    2U;


                /*
                * Return ownership last.
                */
                __DMB();

                p_shared->owner =
                    SHARED_OWNER_CPU0;


                #if BSP_CFG_DCACHE_ENABLED

                SCB_CleanDCache_by_Addr(
                    (volatile void *) p_shared,
                    (int32_t) sizeof(shared_sdram_mailbox_t)
                );

                #endif

                __DSB();


                g_cpu1_ipc_send_err =
                    R_IPC_MessageSend(
                        &g_ipc0_ctrl,
                        IPC_TEST_ACK
                    );


                if (FSP_SUCCESS ==
                    g_cpu1_ipc_send_err)
                {
                    g_cpu1_ipc_ack_count++;
                }
            }
        }
        
        if (0U != s_perception_pending)
        {
            s_perception_pending =
                0U;


            volatile shared_perception_bank_t * const p_bank =
                SHARED_PERCEPTION_BANK;


#if BSP_CFG_DCACHE_ENABLED

            SCB_InvalidateDCache_by_Addr(
                (volatile void *) p_bank,
                (int32_t) sizeof(shared_perception_bank_t)
            );

#endif


            __DMB();


            uint32_t const sequence_before =
                p_bank->publish_sequence;

            uint32_t const published_index =
                p_bank->published_index;


            if ((SHARED_PERCEPTION_MAGIC ==
                 p_bank->magic) &&
                (SHARED_PERCEPTION_VERSION ==
                 p_bank->version) &&
                (published_index < 2U))
            {
                perception_snapshot_t const snapshot =
                    p_bank->slot[published_index];


                __DMB();


                uint32_t const sequence_after =
                    p_bank->publish_sequence;


                if ((sequence_before ==
                    sequence_after) &&
                    (snapshot.sequence ==
                    sequence_before))
                {
                    /*
                    * Existing production behavior:
                    * publish the validated state locally on CPU1.
                    */
                    g_cpu1_perception_latest =
                        snapshot;


                    /*
                    * The state is now feedback-ready on CPU1.
                    *
                    * Read the same GPT13 counter that CPU0 used at the
                    * Frame Cache publication point.
                    *
                    * No additional IPC, task or scheduling operation is added.
                    */
                    uint32_t const ready_tick_gpt =
                        benchmark_clock_now_ticks();

                    uint32_t const latency_ticks =
                        ready_tick_gpt -
                        snapshot.temporal_source_tick_gpt;


                    g_cpu1_e2e_latency_last_ticks =
                        latency_ticks;

                    g_cpu1_e2e_latency_total_ticks +=
                        (uint64_t) latency_ticks;

                    g_cpu1_e2e_latency_count++;

                    g_cpu1_e2e_latency_last_sequence =
                        snapshot.sequence;


                    if (latency_ticks <
                        g_cpu1_e2e_latency_min_ticks)
                    {
                        g_cpu1_e2e_latency_min_ticks =
                            latency_ticks;
                    }


                    if (latency_ticks >
                        g_cpu1_e2e_latency_max_ticks)
                    {
                        g_cpu1_e2e_latency_max_ticks =
                            latency_ticks;
                    }


                    g_cpu1_perception_last_sequence =
                        snapshot.sequence;

                    g_cpu1_perception_rx_count++;

                    /*
                    * CPU1-local IPC dispatch latency:
                    *
                    * IPC callback arrival
                    *      -> validated Perception Bank snapshot acquired
                    */
                    SYSTIM system_time;

                    if (tk_get_otm(&system_time) >= E_OK)
                    {
                        uint32_t const consume_tick_ms =
                            system_time.lo;

                        uint32_t const latency_ms =
                            consume_tick_ms -
                            s_perception_ipc_arrival_tick_ms;


                        g_cpu1_ipc_dispatch_latency_last_ms =
                            latency_ms;

                        g_cpu1_ipc_dispatch_latency_total_ms +=
                            latency_ms;

                        g_cpu1_ipc_dispatch_latency_count++;


                        if (latency_ms <
                            g_cpu1_ipc_dispatch_latency_min_ms)
                        {
                            g_cpu1_ipc_dispatch_latency_min_ms =
                                latency_ms;
                        }


                        if (latency_ms >
                            g_cpu1_ipc_dispatch_latency_max_ms)
                        {
                            g_cpu1_ipc_dispatch_latency_max_ms =
                                latency_ms;
                        }
                    }

                    /*
                     * Keep demo work outside the existing latency window.
                     */
                    gvd_output_accept(&snapshot);
                }
                else
                {
                    g_cpu1_perception_invalid_count++;
                    gvd_output_invalidate();
                }
            }
            else
            {
                g_cpu1_perception_invalid_count++;
                gvd_output_invalidate();
            }
        }


        tk_dly_tsk(10);
    }
}

void ipc1_callback(ipc_callback_args_t *p_args)
{
    if (IPC_EVENT_MESSAGE_RECEIVED == p_args->event)
    {
        g_cpu1_ipc_rx_value = p_args->message;
        g_cpu1_ipc_rx_count++;

        if (IPC_TEST_REQUEST == p_args->message)
        {
            s_ack_pending = 1U;
        }
        else if (IPC_MSG_PERCEPTION_UPDATE ==
                p_args->message)
        {
            SYSTIM system_time;

            if (tk_get_otm(&system_time) >= E_OK)
            {
                s_perception_ipc_arrival_tick_ms =
                    system_time.lo;
            }

            s_perception_pending =
                1U;
        }
    }
}

fsp_err_t ipc_test_init(void)
{
    g_cpu1_ipc_init_err =
        R_IPC_Open(g_ipc0.p_ctrl, g_ipc0.p_cfg);

    if (FSP_SUCCESS != g_cpu1_ipc_init_err)
    {
        return g_cpu1_ipc_init_err;
    }

    g_cpu1_ipc_init_err =
        R_IPC_Open(g_ipc1.p_ctrl, g_ipc1.p_cfg);

    return g_cpu1_ipc_init_err;
}

ER ipc_test_create(void)
{
    const T_CTSK ctsk =
    {
        .tskatr  = TA_HLNG,
        .task    = ipc_test_task_entry,
        .itskpri = 20,
        .stksz   = 1024
    };

    ID const task_id = tk_cre_tsk(&ctsk);
    if (task_id < E_OK)
    {
        return (ER) task_id;
    }

    s_ipc_task_id = task_id;
    return E_OK;
}

ER ipc_test_start(void)
{
    if (s_ipc_task_id <= 0)
    {
        return E_ID;
    }

    return tk_sta_tsk(s_ipc_task_id, 0);
}
