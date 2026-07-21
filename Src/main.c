#include "main.h"

#include <stdio.h>

#include "hardware/uart.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"

#include <stdlib.h>          // for rand()
#include "pico/stdlib.h"

#include "hardware/gpio.h"

// FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// CLI
#include "cli.h"

#include "rak3172.h"
#include "relay.h"

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart0
#define BAUD_RATE 115200

// Use pins 0 and 1 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 0
#define UART_RX_PIN 1

static uint32_t ulHighFrequencyTimerTicks = 0;

void lcd_task(void *params) {
    while (1) {

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* Callback invoked from the RAK3172 task when a LoRa/LoRaWAN payload is
 * received. The payload drives the on-board relays. */
static void prvLoRaRxCallback(const RAK3172_RxData_t *pxData)
{
    if(pxData == NULL)
    {
        return;
    }

    printf("[LoRaWAN] Downlink payload: %s\n", (const char *)pxData->data);

    if(Relay_ProcessLoRaPayload((const char *)pxData->data, pxData->length))
    {
        printf("[LoRaWAN] Relay command applied\n");
    }
    else
    {
        printf("[LoRaWAN] Payload ignored (invalid relay command)\n");
    }
}

/* One-shot task: configure the RAK3172 for OTAA and join the network at
 * start-up. Once joined it deletes itself; downlinks are then handled by the
 * RAK3172 task through prvLoRaRxCallback. */
static void lorawan_startup_task(void *params)
{
    (void)params;

    /* Let the RAK3172 finish booting after its hardware reset */
    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("[LoRaWAN] Configuring OTAA credentials...\n");
    RAK3172_SetNetworkMode(1);              /* LoRaWAN mode */
    vTaskDelay(pdMS_TO_TICKS(1000));        /* NWM change may reset the module */
    RAK3172_SetJoinMode(1);                 /* OTAA */
    RAK3172_SetRegion(LORAWAN_BAND);
    RAK3172_SetDevEUI(LORAWAN_DEVEUI);
    RAK3172_SetAppEUI(LORAWAN_APPEUI);
    RAK3172_SetAppKey(LORAWAN_APPKEY);
    RAK3172_SetClass(LORAWAN_CLASS);

    printf("[LoRaWAN] Joining network...\n");
    for(uint8_t attempt = 1U; attempt <= LORAWAN_JOIN_ATTEMPTS; attempt++)
    {
        if(RAK3172_Join(30000) == pdPASS)
        {
            printf("[LoRaWAN] Joined successfully, waiting for downlinks\n");
            vTaskDelete(NULL);
            return;
        }

        printf("[LoRaWAN] Join attempt %u/%u failed\n",
               (unsigned int)attempt, (unsigned int)LORAWAN_JOIN_ATTEMPTS);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    printf("[LoRaWAN] Could not join the network\n");
    vTaskDelete(NULL);
}

// void uart_task(void *params) {
//     while (1) {
//         printf("Hello, world!\n");
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

// void watchdog_task(void *params)
// {
//     while(1)
//     {
//         // Afficher l'état des tâches toutes les 5 secondes
//         printf("\n=== System Status ===\n");
//         printf("Free heap: %u bytes\n", xPortGetFreeHeapSize());
//         printf("Tasks running: %u\n", uxTaskGetNumberOfTasks());
        
//         vTaskDelay(pdMS_TO_TICKS(5000));
//     }
// }

int main()
{
    // Initialiser stdio en premier
    stdio_init_all();

    // Délai pour stabiliser USB CDC
    sleep_ms(2000);

    // Set up our UART
    //uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    //gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    //gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Use some the various UART functions to send out data
    // In a default system, printf will also output via the default UART
       
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart

    printf("Free heap before task creation: %u bytes\n", xPortGetFreeHeapSize());

    /* Initialize on-board relays (all off) */
    Relay_Init();

    /* Initialize RAK3172 */
    RAK3172_Init();

    /* Route received LoRa/LoRaWAN payloads to the relay handler */
    RAK3172_RegisterRxCallback(prvLoRaRxCallback);

    BaseType_t xResult;

    xResult = xTaskCreate(lcd_task, "LCD_Task", 512, NULL, 1, NULL);
    if(xResult != pdPASS)
    {
        printf("ERROR: Failed to create LCD task\n");
    }

    // xTaskCreate(uart_task, "UART_Task", 256, NULL, 1, NULL);

    xResult = xTaskCreate(Task_CLI, "CLI_Task", 768, NULL, 1, NULL);
    if(xResult != pdPASS)
    {
        printf("ERROR: Failed to create CLI task\n");
    }

    xResult = xTaskCreate(lorawan_startup_task, "LoRaWAN_Init", 1024, NULL, 1, NULL);
    if(xResult != pdPASS)
    {
        printf("ERROR: Failed to create LoRaWAN startup task\n");
    }

    // xResult = xTaskCreate(watchdog_task, "Watchdog", 256, NULL, 1, NULL);

    printf("Starting FreeRTOS scheduler...\n");
    vTaskStartScheduler();

    printf("ERROR: Scheduler failed to start!\n");
    while (true) {

    }
}

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    // Désactiver les interruptions pour éviter d'autres problèmes
    taskDISABLE_INTERRUPTS();
    
    printf("\n\n!!! STACK OVERFLOW DETECTED !!!\n");
    printf("Task: %s\n", pcTaskName);
    printf("Handle: %p\n", xTask);
    
    // Afficher l'état de la heap
    printf("Free heap: %u bytes\n", xPortGetFreeHeapSize());
    
    // Bloquer ici
    while(1)
    {
        tight_loop_contents();
    }
}
#endif

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    
    printf("\n\n!!! MALLOC FAILED !!!\n");
    printf("Out of heap memory!\n");
    printf("Free heap: %u bytes\n", xPortGetFreeHeapSize());
    
    while(1)
    {
        tight_loop_contents();
    }
}

// Idle hook pour détecter si le système tourne
void vApplicationIdleHook(void)
{
    static uint32_t ulIdleCount = 0;
    ulIdleCount++;
    
    // Toutes les 100000 itérations, clignoter la LED intégrée si disponible
    if((ulIdleCount % 100000) == 0)
    {
        // Optionnel : toggle LED pour indiquer que le système tourne
        // gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
    }
}

void vConfigureTimerForRunTimeStats(void)
{
    ulHighFrequencyTimerTicks = 0;
}

uint32_t ulGetRunTimeCounterValue(void)
{
    return time_us_32() / 100;  // Retourne en centièmes de microsecondes
}
