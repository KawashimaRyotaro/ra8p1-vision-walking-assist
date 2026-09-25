#ifndef DEMO_UART_OWNER_H
#define DEMO_UART_OWNER_H

/* Rebuild BOTH Debug projects after changing this switch.
 * 1: submission demo -- CPU1 owns SCI8, CPU0 diagnostics remain in code
 *    and counters, but their T-Monitor transport is silent.
 * 0: CPU0 diagnostic profile -- original CPU0 transport, silent CPU1.
 * Pin mux remains owned by CPU0 in either profile. No extra IPC is used.
 */
#define DEMO_UART_OWNER_CPU1 (1)

#if DEMO_UART_OWNER_CPU1 != 0 && DEMO_UART_OWNER_CPU1 != 1
#error "Select exactly one SCI8 owner"
#endif

#endif
