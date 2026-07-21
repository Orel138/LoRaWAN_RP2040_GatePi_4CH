#ifndef CLI_H
#define CLI_H

#include "FreeRTOS.h"
#include "task.h"

/* Configuration */
#define CLI_INPUT_LINE_LEN_MAX          128
#define CLI_OUTPUT_SCRATCH_BUF_LEN      256
#define CLI_UART_RX_STREAM_LEN          128
#define CLI_UART_TX_STREAM_LEN          256
#define CLI_UART_RX_READ_SZ_10MS        32
#define CLI_UART_TX_WRITE_SZ_5MS        64

#define CLI_PROMPT_STR                  "> "
#define CLI_PROMPT_LEN                  2
#define CLI_OUTPUT_EOL                  "\n"
#define CLI_OUTPUT_EOL_LEN              1

/* Task prototype */
void Task_CLI(void *pvParameters);

/* Public API */
BaseType_t xInitConsoleUart(void);

#endif /* CLI_H */