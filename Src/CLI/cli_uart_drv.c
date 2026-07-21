#include "FreeRTOS.h"
#include "task.h"
#include "cli.h"
#include "cli_prv.h"
#include "stream_buffer.h"
#include "semphr.h"

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#include <string.h>
#include <stdio.h>

/* UART Configuration - utilise USB CDC par défaut via stdio */
#define USE_USB_CDC  1  // Set to 1 to use USB CDC, 0 to use hardware UART

static SemaphoreHandle_t xUartTxSem = NULL;
static volatile BaseType_t xPartialCommand = pdFALSE;

StreamBufferHandle_t xUartRxStream = NULL;
StreamBufferHandle_t xUartTxStream = NULL;

static char pcInputBuffer[CLI_INPUT_LINE_LEN_MAX] = {0};
static volatile uint32_t ulInBufferIdx = 0;

static BaseType_t xExitFlag = pdFALSE;

static TaskHandle_t xRxThreadHandle = NULL;
static TaskHandle_t xTxThreadHandle = NULL;

static void vTxThread(void *pvParameters);
static void vRxThread(void *pvParameters);

BaseType_t xInitConsoleUart(void)
{
    printf("Initializing CLI UART...\n");
    printf("Free heap: %u bytes\n", xPortGetFreeHeapSize());
    
    xUartTxSem = xSemaphoreCreateBinary();
    
    if(xUartTxSem == NULL)
    {
        printf("ERROR: Failed to create UART TX semaphore\n");
        return pdFAIL;
    }
    printf("TX Semaphore created, heap: %u bytes\n", xPortGetFreeHeapSize());

    xUartRxStream = xStreamBufferCreate(CLI_UART_RX_STREAM_LEN, 1);
    if(xUartRxStream == NULL)
    {
        printf("ERROR: Failed to create RX stream buffer\n");
        return pdFAIL;
    }
    printf("RX Stream created, heap: %u bytes\n", xPortGetFreeHeapSize());
    
    xUartTxStream = xStreamBufferCreate(CLI_UART_TX_STREAM_LEN, 8);
    if(xUartTxStream == NULL)
    {
        printf("ERROR: Failed to create TX stream buffer\n");
        return pdFAIL;
    }
    printf("TX Stream created, heap: %u bytes\n", xPortGetFreeHeapSize());

    printf("Creating UART RX task...\n");
    printf("Stack size: %u words (%u bytes)\n", 256, 256 * sizeof(StackType_t));
    
    /* Désactiver temporairement les interruptions pour debug */
    taskENTER_CRITICAL();
    
    BaseType_t xResult;
    
    xResult = xTaskCreate(vRxThread, "uartRx", 384, NULL, 3, &xRxThreadHandle);
    
    taskEXIT_CRITICAL();
    
    printf("xTaskCreate returned: %d\n", xResult);
    printf("Heap after RX task: %u bytes\n", xPortGetFreeHeapSize());
    
    if(xResult != pdPASS)
    {
        printf("ERROR: Failed to create RX thread (result=%d)\n", xResult);
        return pdFAIL;
    }
    printf("RX task created successfully\n");
    
    printf("Creating UART TX task...\n");
    xResult = xTaskCreate(vTxThread, "uartTx", 384, NULL, 2, &xTxThreadHandle);
    
    printf("xTaskCreate TX returned: %d\n", xResult);
    printf("Heap after TX task: %u bytes\n", xPortGetFreeHeapSize());
    
    if(xResult != pdPASS)
    {
        printf("ERROR: Failed to create TX thread (result=%d)\n", xResult);
        return pdFAIL;
    }
    printf("TX task created\n");

    xSemaphoreGive(xUartTxSem);
    
    printf("CLI UART initialized successfully\n");

    return pdTRUE;
}

static void vRxThread(void *pvParameters)
{
    char c;
    
    printf("RX Thread started\n");
    
    // Petit délai pour permettre au système de se stabiliser
    vTaskDelay(pdMS_TO_TICKS(100));
    
    printf("RX Thread entering main loop\n");
    
    while(!xExitFlag)
    {
        // Utiliser getchar_timeout_us avec un timeout COURT
        // et yield entre chaque tentative
        int ch = getchar_timeout_us(1000); // 1ms timeout seulement
        
        if(ch != PICO_ERROR_TIMEOUT && ch != EOF)
        {
            c = (char)ch;
            size_t sent = xStreamBufferSend(xUartRxStream, &c, 1, 0);
            if(sent == 0)
            {
                // Buffer plein, on attend un peu
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }
        else
        {
            // Pas de données, donner du temps aux autres tâches
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    
    printf("RX Thread exiting\n");
}

static void vTxThread(void *pvParameters)
{
    uint8_t pucTxBuffer[CLI_UART_TX_WRITE_SZ_5MS] = {0};
    size_t xBytes = 0;

    printf("TX Thread started\n");
    
    // Petit délai pour permettre au système de se stabiliser
    vTaskDelay(pdMS_TO_TICKS(100));
    
    printf("TX Thread entering main loop\n");

    while(!xExitFlag)
    {
        xBytes = xStreamBufferReceive(xUartTxStream,
                                      pucTxBuffer,
                                      CLI_UART_TX_WRITE_SZ_5MS,
                                      pdMS_TO_TICKS(50));  // Timeout de 50ms

        if(xBytes > 0)
        {
            // Écrire caractère par caractère pour éviter les problèmes
            for(size_t i = 0; i < xBytes; i++)
            {
                putchar_raw(pucTxBuffer[i]);
            }
            stdio_flush();
        }
        else
        {
            // Pas de données à envoyer, yield
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    
    printf("TX Thread exiting\n");
}

static void uart_write(const void * const pvOutputBuffer, uint32_t xOutputBufferLen)
{
    size_t xBytesSent = 0;
    const uint8_t * const pcBuffer = (uint8_t * const) pvOutputBuffer;

    if((pvOutputBuffer != NULL) && (xOutputBufferLen > 0))
    {
        while(xBytesSent < xOutputBufferLen)
        {
            xBytesSent += xStreamBufferSend(xUartTxStream,
                                           (const void *) &(pcBuffer[xBytesSent]),
                                           xOutputBufferLen - xBytesSent,
                                           portMAX_DELAY);
        }
    }

    configASSERT(xBytesSent == xOutputBufferLen);
}

static int32_t uart_read(char * const pcInputBuffer, uint32_t xInputBufferLen)
{
    int32_t ulBytesRead = 0;

    if((pcInputBuffer != NULL) && (xInputBufferLen > 0))
    {
        ulBytesRead = xStreamBufferReceive(xUartRxStream,
                                          pcInputBuffer,
                                          xInputBufferLen,
                                          portMAX_DELAY);
    }

    return ulBytesRead;
}

static int32_t uart_read_timeout(char * const pcInputBuffer,
                                 uint32_t xInputBufferLen,
                                 TickType_t xTimeout)
{
    int32_t ulBytesRead = 0;

    if((pcInputBuffer != NULL) && (xInputBufferLen > 0))
    {
        ulBytesRead = xStreamBufferReceive(xUartRxStream,
                                          pcInputBuffer,
                                          xInputBufferLen,
                                          xTimeout);
    }

    return ulBytesRead;
}

static void uart_print(const char * const pcString)
{
    if(pcString != NULL)
    {
        size_t xLength = strlen(pcString);
        uart_write(pcString, xLength);
    }
}

static int32_t uart_readline(char ** const ppcInputBuffer)
{
    int32_t lBytesWritten = 0;
    ulInBufferIdx = 0;
    BaseType_t xFoundEOL = pdFALSE;

    *ppcInputBuffer = pcInputBuffer;
    pcInputBuffer[CLI_INPUT_LINE_LEN_MAX - 1] = '\0';

    uart_write(CLI_PROMPT_STR, CLI_PROMPT_LEN);
    xPartialCommand = pdTRUE;

    while(ulInBufferIdx < CLI_INPUT_LINE_LEN_MAX && xFoundEOL == pdFALSE)
    {
        // IMPORTANT : Utiliser un timeout court au lieu de portMAX_DELAY
        if(uart_read_timeout(&(pcInputBuffer[ulInBufferIdx]), 1, pdMS_TO_TICKS(100)))  // 100ms timeout
        {
            switch(pcInputBuffer[ulInBufferIdx])
            {
                case '\n':
                case '\r':
                    if(ulInBufferIdx > 0)
                    {
                        pcInputBuffer[ulInBufferIdx] = '\0';
                        lBytesWritten = ulInBufferIdx;
                        xFoundEOL = pdTRUE;

                        if(xSemaphoreTake(xUartTxSem, pdMS_TO_TICKS(100)) == pdTRUE)
                        {
                            uart_write(CLI_OUTPUT_EOL, CLI_OUTPUT_EOL_LEN);
                            xPartialCommand = pdFALSE;
                            xSemaphoreGive(xUartTxSem);
                        }
                    }
                    else
                    {
                        if(xSemaphoreTake(xUartTxSem, pdMS_TO_TICKS(100)) == pdTRUE)
                        {
                            pcInputBuffer[ulInBufferIdx] = '\0';
                            ulInBufferIdx = 0;
                            xSemaphoreGive(xUartTxSem);
                        }
                    }
                    break;

                case '\b':
                case '\x7F':
                    if(ulInBufferIdx > 0)
                    {
                        if(xSemaphoreTake(xUartTxSem, pdMS_TO_TICKS(100)) == pdTRUE)
                        {
                            pcInputBuffer[ulInBufferIdx] = '\0';
                            
                            if(ulInBufferIdx > 0)
                            {
                                pcInputBuffer[ulInBufferIdx - 1] = '\0';
                            }
                            
                            ulInBufferIdx--;
                            uart_print("\b \b");
                            xSemaphoreGive(xUartTxSem);
                        }
                    }
                    break;

                case '\x03':  // Ctrl+C
                    if(xSemaphoreTake(xUartTxSem, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        ulInBufferIdx = 0;
                        uart_write(CLI_OUTPUT_EOL CLI_PROMPT_STR, CLI_OUTPUT_EOL_LEN + CLI_PROMPT_LEN);
                        xSemaphoreGive(xUartTxSem);
                    }
                    break;

                default:
                    if(xSemaphoreTake(xUartTxSem, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        uart_write(&(pcInputBuffer[ulInBufferIdx]), 1);
                        ulInBufferIdx++;
                        xSemaphoreGive(xUartTxSem);
                    }
                    break;
            }
        }
        // Si timeout, on continue la boucle (permet aux autres tâches de s'exécuter)
    }

    return lBytesWritten;
}

static void uart_lock(void)
{
    xSemaphoreTake(xUartTxSem, portMAX_DELAY);
}

static void uart_unlock(void)
{
    xSemaphoreGive(xUartTxSem);
}

const ConsoleIO_t xConsoleIO =
{
    .read         = uart_read,
    .write        = uart_write,
    .lock         = uart_lock,
    .unlock       = uart_unlock,
    .read_timeout = uart_read_timeout,
    .print        = uart_print,
    .readline     = uart_readline
};