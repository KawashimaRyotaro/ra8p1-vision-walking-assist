#ifndef DEMO_CONSOLE_H
#define DEMO_CONSOLE_H
#include <stdint.h>
uint32_t demo_console_try_write(const char *data, uint32_t bytes);
extern volatile uint32_t g_cpu1_console_timeout_count;
#endif
