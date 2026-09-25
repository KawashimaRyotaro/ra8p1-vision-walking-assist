#ifndef APP_DEMO_CONFIG_H
#define APP_DEMO_CONFIG_H

/* Shared build settings: rebuild and download BOTH CPU projects after edits.
 * CPU0 selects the tasks; the UART owner prints the same settings at startup.
 * The banner reports configuration, not successful peripheral initialization.
 */
#define APP_VIDEO_SOURCE_CAMERA     (1U) /* 1: camera, 0: USB replay */
#define APP_ENABLE_DEBUG_DISPLAY    (1U) /* 1: Display task enabled */

#if APP_VIDEO_SOURCE_CAMERA != 0 && APP_VIDEO_SOURCE_CAMERA != 1
#error "APP_VIDEO_SOURCE_CAMERA must be 0 or 1"
#endif
#if APP_ENABLE_DEBUG_DISPLAY != 0 && APP_ENABLE_DEBUG_DISPLAY != 1
#error "APP_ENABLE_DEBUG_DISPLAY must be 0 or 1"
#endif

#endif /* APP_DEMO_CONFIG_H */
