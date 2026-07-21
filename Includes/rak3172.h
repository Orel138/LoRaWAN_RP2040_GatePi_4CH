#ifndef RAK3172_H
#define RAK3172_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include <stdint.h>
#include <stdbool.h>

/* Debug configuration */
#define RAK3172_DEBUG_LOGS      0

#if RAK3172_DEBUG_LOGS
    #define RAK_DEBUG(fmt, ...) printf("[RAK3172] " fmt, ##__VA_ARGS__)
#else
    #define RAK_DEBUG(fmt, ...)
#endif

/* Configuration */
#define RAK3172_UART_ID         uart0
#define RAK3172_TX_PIN          0
#define RAK3172_RX_PIN          1
#define RAK3172_BAUD_RATE       115200

/* GP8 drives the RAK3172 reset line (active low), matching the vendor
 * MicroPython examples. GP7 drives the module BOOT0 line: it MUST be held low
 * so the RAK3172 boots its application (AT) firmware instead of the STM32
 * system bootloader, otherwise the module never answers AT commands. */
#define RAK3172_RST_PIN         8
#define RAK3172_BOOT0_PIN       7

#define RAK3172_RX_BUFFER_SIZE  512
#define RAK3172_RESPONSE_TIMEOUT_MS  2000
#define RAK3172_MAX_PAYLOAD     255



/* Event types */
typedef enum {
    RAK3172_EVENT_NONE,
    RAK3172_EVENT_OK,
    RAK3172_EVENT_ERROR,
    RAK3172_EVENT_JOIN_SUCCESS,
    RAK3172_EVENT_JOIN_FAILED,
    RAK3172_EVENT_TX_SUCCESS,
    RAK3172_EVENT_TX_FAILED,
    RAK3172_EVENT_RX_DATA,
    RAK3172_EVENT_RESPONSE
} RAK3172_Event_t;

/* Structure for received LoRa data */
typedef struct {
    uint8_t port;
    uint8_t data[RAK3172_MAX_PAYLOAD];
    uint16_t length;
    int16_t rssi;
    int8_t snr;
} RAK3172_RxData_t;

/* Event structure */
typedef struct {
    RAK3172_Event_t type;
    char response[RAK3172_RX_BUFFER_SIZE];
    RAK3172_RxData_t rxData;
} RAK3172_EventData_t;

/* Callback type for received data */
typedef void (*RAK3172_RxCallback_t)(const RAK3172_RxData_t *data);

/* Public API */
BaseType_t RAK3172_Init(void);
BaseType_t RAK3172_HardwareReset(void);
BaseType_t RAK3172_SendCommand(const char *cmd, char *response, uint32_t timeout_ms);
BaseType_t RAK3172_GetVersion(char *version, size_t max_len);
BaseType_t RAK3172_Join(uint32_t timeout_ms);
BaseType_t RAK3172_SendData(uint8_t port, const uint8_t *data, uint16_t length);
BaseType_t RAK3172_SendDataUnconfirmed(uint8_t port, const uint8_t *data, uint16_t length);
BaseType_t RAK3172_SetDevEUI(const char *deveui);
BaseType_t RAK3172_SetAppEUI(const char *appeui);
BaseType_t RAK3172_SetAppKey(const char *appkey);
BaseType_t RAK3172_SetRegion(const char *region);
BaseType_t RAK3172_SetNetworkMode(uint8_t mode);   /* AT+NWM: 0 = P2P, 1 = LoRaWAN */
BaseType_t RAK3172_SetJoinMode(uint8_t mode);       /* AT+NJM: 0 = ABP, 1 = OTAA */
BaseType_t RAK3172_SetClass(char deviceClass);      /* AT+CLASS: 'A', 'B' or 'C' */
BaseType_t RAK3172_RegisterRxCallback(RAK3172_RxCallback_t callback);

/* Low-level diagnostic: resets the module then sends "AT" over the raw UART
 * (bypassing the queue/mutex), trying 115200 then 9600 baud. Fills `dump` with
 * a hex trace of every received byte, reports how many bytes came back and at
 * which baud. Returns pdPASS if any byte was received. */
BaseType_t RAK3172_SelfTest(char *dump, size_t dumpLen,
                            uint32_t *bytesReceived, uint32_t *baudFound);

/* Task */
void Task_RAK3172(void *pvParameters);

#endif /* RAK3172_H */