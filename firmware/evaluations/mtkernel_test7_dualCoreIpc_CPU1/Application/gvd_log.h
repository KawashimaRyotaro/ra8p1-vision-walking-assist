#ifndef GVD_LOG_H
#define GVD_LOG_H

#include <tk/tkernel.h>
#include <stdint.h>

/* Format and transmit events already decided by the GVD output task. */
ER gvd_log_create(void);
ER gvd_log_start(void);

extern volatile ID g_cpu1_gvd_log_task_id;
extern volatile uint32_t g_cpu1_gvd_boot_log_count;

#endif /* GVD_LOG_H */
