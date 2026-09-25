#ifndef SHARED_IPC_PROTOCOL_H
#define SHARED_IPC_PROTOCOL_H

#include <stdint.h>

/*
 * EK-RA8P1 SDRAM:
 *   0x68000000 - 0x6FFFFFFF
 *
 * Reserve the last 64 bytes for the dual-core ownership test.
 */
#define SHARED_SDRAM_MAILBOX_ADDR      (0x6FFFFFC0UL)

#define SHARED_SDRAM_MAGIC             (0x5344524DUL)

#define SHARED_OWNER_CPU0              (0U)
#define SHARED_OWNER_CPU1              (1U)

#define SHARED_TEST_SLOT               (2U)
#define SHARED_TEST_REQUEST_VALUE      (0x13579BDFUL)
#define SHARED_TEST_RESPONSE_VALUE     (0x2468ACE0UL)

typedef struct
{
    uint32_t magic;
    uint32_t sequence;
    uint32_t owner;
    uint32_t slot;

    uint32_t request_value;
    uint32_t response_value;

    uint32_t reserved[10];
} shared_sdram_mailbox_t;

#define SHARED_SDRAM_MAILBOX \
    ((volatile shared_sdram_mailbox_t *) SHARED_SDRAM_MAILBOX_ADDR)


/*
 * Shared Perception Bank
 *
 * Address range:
 *   0x6FFFFE00 ...
 *
 * Existing IPC test mailbox remains at:
 *   0x6FFFFFC0 ...
 *
 * The bank uses two snapshots so CPU0 can update one while CPU1
 * consumes the previously published one.
 */
#define SHARED_PERCEPTION_BANK_ADDR      (0x6FFFFE00UL)

#define SHARED_PERCEPTION_MAGIC          (0x5042434BUL)
#define SHARED_PERCEPTION_VERSION        (1U)

#define SHARED_PERCEPTION_OBJECT_MAX     (8U)

#define IPC_MSG_PERCEPTION_UPDATE        (0x50455243UL)

typedef struct
{
    uint16_t class_id;
    uint16_t confidence_x1000;

    uint16_t center_x;
    uint16_t center_y;

    uint16_t width;
    uint16_t height;

    /*
     * Temporary proximity representation.
     *
     * 0    = unknown / far
     * 1000 = very near
     *
     * This is NOT a metric distance.
     */
    uint16_t proximity_x1000;

    uint16_t reserved;

} perception_object_t;


/*
 * Exactly 160 bytes:
 *   metadata = 32 bytes
 *   objects  = 8 x 16 = 128 bytes
 */
typedef struct
{
    uint32_t sequence;

    uint32_t temporal_frame_index;
    uint32_t semantic_frame_index;
    uint32_t producer_tick_ms;

    uint16_t activity_left;
    uint16_t activity_right;

    uint16_t activity_coverage_x1000;

    int8_t global_dx;
    int8_t global_dy;

    uint8_t global_valid;
    uint8_t motion_uncertain;

    uint16_t object_count;

    /*
    * GPT13 counter value captured immediately before the
    * temporal source frame becomes PUBLISHED in Frame Cache.
    *
    * GPT13 is the common free-running benchmark clock shared
    * by CPU0 and CPU1.
    */
    uint32_t temporal_source_tick_gpt;


    perception_object_t
        objects[SHARED_PERCEPTION_OBJECT_MAX];

} perception_snapshot_t;


/*
 * Header = 32 bytes.
 *
 * slot[0] = 160 bytes
 * slot[1] = 160 bytes
 *
 * Total = 352 bytes.
 *
 * All boundaries are 32-byte aligned for CPU0 D-cache maintenance.
 */
typedef struct
{
    uint32_t magic;
    uint32_t version;

    uint32_t published_index;
    uint32_t publish_sequence;

    uint32_t reserved[4];

    perception_snapshot_t slot[2];

} shared_perception_bank_t;


#define SHARED_PERCEPTION_BANK \
    ((volatile shared_perception_bank_t *) \
     SHARED_PERCEPTION_BANK_ADDR)


#endif