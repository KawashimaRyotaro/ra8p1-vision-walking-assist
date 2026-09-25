#ifndef USB_LOADER_H
#define USB_LOADER_H

#include <tk/tkernel.h>
#include <stdint.h>

/* CPU0 debugger diagnostics: UART is owned by CPU1 in the GVD demo. */
typedef enum
{
    USB_LOADER_IDLE = 0,
    USB_LOADER_OPENING,
    USB_LOADER_WAIT_DEVICE,
    USB_LOADER_MEDIA_INIT,
    USB_LOADER_RAW_BENCHMARK,
    USB_LOADER_DISK_INIT,
    USB_LOADER_MOUNT,
    USB_LOADER_READ_SELECT,
    USB_LOADER_OPEN_VIDEO,
    USB_LOADER_WAIT_BUFFER,
    USB_LOADER_READ_FRAME,
    USB_LOADER_FINISHED,
    USB_LOADER_WAIT_FPS
} usb_loader_stage_t;

typedef struct
{
    usb_loader_stage_t stage;
    uint32_t failed;
    int32_t last_fsp_error;
    int32_t last_fat_error;
    uint32_t last_usb_event;
    uint32_t event_poll_count;
    uint32_t frame_index;
    uint32_t frame_count;
    uint32_t frames_published;
    /* CPU0-local diagnostics only; not part of the IPC protocol. */
    uint32_t playback_count;
    uint32_t detach_count;
    uint32_t target_fps;
    uint32_t stream_frame_index; /* Last published ID; UINT32_MAX before first frame. */
    uint32_t playback_elapsed_ms; /* First to latest publication, not mount time. */
} usb_loader_diagnostics_t;

extern volatile usb_loader_diagnostics_t g_usb_loader_diagnostics;

ER usb_loader_create(void);
ER usb_loader_start(void);

#endif /* USB_LOADER_H */
