#ifndef COMMON_UTILS_H_
#define COMMON_UTILS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <tm/tmonitor.h>

#include "hal_data.h"


#define RESET_VALUE    (0x00U)
#define MODULE_CLOSE   (0U)


#define APP_PRINT(fn_, ...) \
    tm_printf( \
        (UB *) (fn_), \
        ##__VA_ARGS__ \
    )


#define APP_ERR_PRINT(fn_, ...) \
    do \
    { \
        tm_printf( \
            (UB *) "[Camera][ERR] %s: ", \
            __FUNCTION__ \
        ); \
        tm_printf( \
            (UB *) (fn_), \
            ##__VA_ARGS__ \
        ); \
    } while (0)


#define APP_ERR_RET(con_, err_, fn_, ...) \
    do \
    { \
        if (con_) \
        { \
            APP_ERR_PRINT( \
                fn_, \
                ##__VA_ARGS__ \
            ); \
            return (err_); \
        } \
    } while (0)


#endif /* COMMON_UTILS_H_ */