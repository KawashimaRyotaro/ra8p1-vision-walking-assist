#ifndef IPC_TEST_H
#define IPC_TEST_H

#include <tk/tkernel.h>
#include "hal_data.h"
#include "shared_ipc_protocol.h"


/* Initialize IPC hardware, then create and start the task from usermain. */
fsp_err_t ipc_test_init(void);
ER ipc_test_create(void);
ER ipc_test_start(void);

fsp_err_t ipc_perception_publish(
    perception_snapshot_t const * source
);


#endif
