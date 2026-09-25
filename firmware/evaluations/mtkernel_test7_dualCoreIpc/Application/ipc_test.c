#include <tk/tkernel.h>
#include "hal_data.h"
#include "ipc_test.h"
#include "shared_ipc_protocol.h"

#define IPC_TEST_REQUEST   (0x12345678UL)
#define IPC_TEST_ACK       (0xA5A5A5A5UL)

volatile uint32_t g_cpu0_ipc_ack_value = 0U;
volatile uint32_t g_cpu0_ipc_ack_count = 0U;
volatile fsp_err_t g_cpu0_ipc_init_err = FSP_SUCCESS;
volatile fsp_err_t g_cpu0_ipc_send_err = FSP_SUCCESS;

volatile uint32_t g_cpu0_shared_response = 0U;
volatile uint32_t g_cpu0_shared_owner = 0U;
volatile uint32_t g_cpu0_shared_sequence = 0U;
volatile uint32_t g_cpu0_shared_pass = 0U;

volatile uint32_t g_cpu0_perception_publish_count = 0U;
volatile uint32_t g_cpu0_perception_last_sequence = 0U;

static ID s_ipc_task_id;
static volatile uint32_t s_ack_received = 0U;


fsp_err_t ipc_perception_publish(
    perception_snapshot_t const * source)
{
    static uint32_t s_publish_sequence = 0U;

    if (NULL == source)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }


    volatile shared_perception_bank_t * const p_bank =
        SHARED_PERCEPTION_BANK;


    uint32_t const new_sequence =
        s_publish_sequence + 1U;

    /*
     * Alternate between slot 0 and slot 1.
     */
    uint32_t const write_index =
        new_sequence & 1U;


    perception_snapshot_t snapshot =
        *source;

    snapshot.sequence =
        new_sequence;


    /*
    * Write only the inactive/new slot first.
    */
    p_bank->slot[write_index] =
        snapshot;


#if BSP_CFG_DCACHE_ENABLED

    /*
     * CPU1 must see the actual SDRAM contents, not CPU0's dirty
     * D-cache line.
     */
    SCB_CleanDCache_by_Addr(
        (volatile void *)
        &p_bank->slot[write_index],
        (int32_t) sizeof(perception_snapshot_t)
    );

#endif


    __DMB();


    /*
     * Publish the slot only after the complete payload is visible.
     */
    p_bank->magic =
        SHARED_PERCEPTION_MAGIC;

    p_bank->version =
        SHARED_PERCEPTION_VERSION;

    p_bank->published_index =
        write_index;

    p_bank->publish_sequence =
        new_sequence;


#if BSP_CFG_DCACHE_ENABLED

    SCB_CleanDCache_by_Addr(
        (volatile void *) p_bank,
        32
    );

#endif


    __DMB();


    fsp_err_t const err =
        R_IPC_MessageSend(
            &g_ipc1_ctrl,
            IPC_MSG_PERCEPTION_UPDATE
        );


    if (FSP_SUCCESS == err)
    {
        s_publish_sequence =
            new_sequence;

        g_cpu0_perception_last_sequence =
            new_sequence;

        g_cpu0_perception_publish_count++;
    }


    return err;
}


static void ipc_test_task_entry(INT stacd, void *exinf)
{
    FSP_PARAMETER_NOT_USED(stacd);
    FSP_PARAMETER_NOT_USED(exinf);

    /*
     * CPU1 is started before CPU0 enters μT-Kernel.
     * Give CPU1 enough time to initialize its IPC instances.
     */
    tk_dly_tsk(500);

    volatile shared_sdram_mailbox_t * const p_shared =
        SHARED_SDRAM_MAILBOX;


    /*
    * Build the complete request first.
    */
    p_shared->owner          = SHARED_OWNER_CPU0;
    p_shared->magic          = SHARED_SDRAM_MAGIC;
    p_shared->sequence       = 1U;
    p_shared->slot           = SHARED_TEST_SLOT;
    p_shared->request_value  = SHARED_TEST_REQUEST_VALUE;
    p_shared->response_value = 0U;


    /*
    * Publish ownership last.
    */
    __DMB();

    p_shared->owner =
        SHARED_OWNER_CPU1;


    /*
    * CPU0 D-cache is enabled.
    * Make the complete mailbox visible in SDRAM before IPC notification.
    */
    #if BSP_CFG_DCACHE_ENABLED

    SCB_CleanDCache_by_Addr(
        (volatile void *) p_shared,
        (int32_t) sizeof(shared_sdram_mailbox_t)
    );

    #endif

    __DMB();


    /*
    * Notify CPU1 only after the shared payload is visible.
    */
    g_cpu0_ipc_send_err =
        R_IPC_MessageSend(
            &g_ipc1_ctrl,
            IPC_TEST_REQUEST
        );

    while (1)
    {
        if (0U != s_ack_received)
        {
            s_ack_received = 0U;


        #if BSP_CFG_DCACHE_ENABLED

            /*
            * CPU1 has modified this SDRAM region.
            * Discard CPU0's stale cached copy before reading it.
            */
            SCB_InvalidateDCache_by_Addr(
                (volatile void *) p_shared,
                (int32_t) sizeof(shared_sdram_mailbox_t)
            );

        #endif

            __DSB();


            g_cpu0_shared_response =
                p_shared->response_value;

            g_cpu0_shared_owner =
                p_shared->owner;

            g_cpu0_shared_sequence =
                p_shared->sequence;

            if ((SHARED_TEST_RESPONSE_VALUE == p_shared->response_value) &&
                (SHARED_OWNER_CPU0 == p_shared->owner) &&
                (2U == p_shared->sequence))
            {
                g_cpu0_shared_pass = 1U;
            }
        }

        tk_dly_tsk(10);
    }
}

void ipc0_callback(ipc_callback_args_t *p_args)
{
    if (IPC_EVENT_MESSAGE_RECEIVED ==
        p_args->event)
    {
        g_cpu0_ipc_ack_value =
            p_args->message;

        g_cpu0_ipc_ack_count++;


        if (IPC_TEST_ACK ==
            p_args->message)
        {
            s_ack_received =
                1U;
        }
    }
}

fsp_err_t ipc_test_init(void)
{
    g_cpu0_ipc_init_err =
        R_IPC_Open(g_ipc0.p_ctrl, g_ipc0.p_cfg);

    if (FSP_SUCCESS != g_cpu0_ipc_init_err)
    {
        return g_cpu0_ipc_init_err;
    }

    g_cpu0_ipc_init_err =
        R_IPC_Open(g_ipc1.p_ctrl, g_ipc1.p_cfg);

    return g_cpu0_ipc_init_err;
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
