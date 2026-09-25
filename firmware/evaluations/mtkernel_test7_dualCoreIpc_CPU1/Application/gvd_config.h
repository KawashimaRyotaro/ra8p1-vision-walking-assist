#ifndef GVD_CONFIG_H
#define GVD_CONFIG_H

/* DEMO ONLY: activity sums, NOT calibrated physical risk or distance.
 * Each side contains 80 blocks; 64/256 mean 0.8/3.2 residual pixels per
 * block. Start with these provisional thresholds and record calibration.
 * Low activity does not rule out stationary obstacles.
 */
#define GVD_POSSIBLE_ACTIVITY_MIN (64U)
#define GVD_HIGH_ACTIVITY_MIN     (256U)
#define GVD_DIRECTION_DIFF_MIN    (32U)
#define GVD_DIRECTION_DIFF_PC     (25U)
#define GVD_STALE_MS              (500U)

/* L/R in the log remain original image halves. Swap the final cue for both
 * console and LED6/7 after confirming camera/USB orientation in the scene.
 */
#ifndef GVD_CUE_SWAP_LR
#define GVD_CUE_SWAP_LR           (0U)
#endif

#define GVD_OUTPUT_PRIORITY      (25)
#define GVD_LOG_PRIORITY         (26)
#define GVD_TASK_STACK_BYTES     (2048)
#define GVD_OUTPUT_POLL_MS       (10)
#define GVD_LOG_INTERVAL_MS      (500U)
#define GVD_SYNC_STEP_MS         (150U)
#define GVD_UART_LINE_TIMEOUT_MS (500U)
#define GVD_UART_CHUNK_BYTES     (32U)
#define GVD_UART_SERVICE_POLLS   (65536U)
#define GVD_UART_BOOT_POLL_LIMIT (10000U)
#define GVD_LOG_BUFFER_BYTES     (256U)

#if GVD_POSSIBLE_ACTIVITY_MIN >= GVD_HIGH_ACTIVITY_MIN
#error "GVD activity thresholds must be increasing"
#endif
#if GVD_DIRECTION_DIFF_PC > 100U || GVD_LOG_INTERVAL_MS == 0U
#error "Invalid GVD timing or direction threshold"
#endif
#endif
