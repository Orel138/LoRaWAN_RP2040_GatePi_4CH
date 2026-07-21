#include "relay_report.h"
#include "relay.h"
#include "rak3172.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>

/* LoRaWAN port used for the relay status uplink */
#define RELAY_REPORT_PORT           2U

/* Lower bound on the interval to avoid flooding the network */
#define RELAY_REPORT_MIN_INTERVAL_S 5U

static TaskHandle_t xReportTaskHandle = NULL;
static volatile bool xReportEnabled = false;
static volatile uint32_t xReportIntervalMs = 0U;

/* Pack the relay states into one byte: bit i set means relay (i + 1) is on. */
static uint8_t prvBuildStatusByte(void)
{
    uint8_t status = 0U;

    for(uint8_t i = 0U; i < RELAY_COUNT; i++)
    {
        if(Relay_Get((uint8_t)(i + 1U)))
        {
            status |= (uint8_t)(1U << i);
        }
    }

    return status;
}

static void prvSendStatus(void)
{
    uint8_t status = prvBuildStatusByte();

    if(RAK3172_SendDataUnconfirmed(RELAY_REPORT_PORT, &status, 1U) == pdPASS)
    {
        printf("[Report] Relay status uplink sent: 0x%02X\n", status);
    }
    else
    {
        printf("[Report] Relay status uplink failed\n");
    }
}

static void prvReportTask(void *pvParameters)
{
    (void)pvParameters;

    for(;;)
    {
        if(!xReportEnabled)
        {
            /* Idle with no CPU cost until the routine is (re)started */
            (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            continue;
        }

        prvSendStatus();

        /* Wait for the configured interval, but wake immediately if the
         * routine is stopped or reconfigured through RelayReport_Start/Stop. */
        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(xReportIntervalMs));
    }
}

void RelayReport_Init(void)
{
    if(xReportTaskHandle == NULL)
    {
        (void)xTaskCreate(prvReportTask, "RelayReport", 1024, NULL, 1,
                          &xReportTaskHandle);
    }
}

bool RelayReport_Start(uint32_t interval_s)
{
    if(xReportTaskHandle == NULL)
    {
        return false;
    }

    if(interval_s < RELAY_REPORT_MIN_INTERVAL_S)
    {
        interval_s = RELAY_REPORT_MIN_INTERVAL_S;
    }

    xReportIntervalMs = interval_s * 1000U;
    xReportEnabled = true;
    xTaskNotifyGive(xReportTaskHandle);   /* wake the task now */

    return true;
}

void RelayReport_Stop(void)
{
    xReportEnabled = false;

    if(xReportTaskHandle != NULL)
    {
        xTaskNotifyGive(xReportTaskHandle);   /* break the interval wait */
    }
}

bool RelayReport_IsRunning(void)
{
    return xReportEnabled;
}

uint32_t RelayReport_GetIntervalS(void)
{
    return xReportIntervalMs / 1000U;
}

bool RelayReport_ProcessLoRaPayload(const char *payload, uint16_t length)
{
    if((payload == NULL) || (length < 2U))
    {
        return false;
    }

    /* Report commands are marked by a leading 'F' nibble, which cannot collide
     * with the relay commands (those start with '0' or '1'). */
    if((payload[0] != 'F') && (payload[0] != 'f'))
    {
        return false;
    }

    char stateChar = payload[1];

    if(stateChar == '0')
    {
        RelayReport_Stop();
        return true;
    }

    if(stateChar != '1')
    {
        return false;
    }

    /* "F1" optionally followed by 4 hex chars = 16-bit interval in seconds.
     * Without an explicit interval, reuse the current one (or a default). */
    uint32_t interval_s = RelayReport_GetIntervalS();

    if(length >= 6U)
    {
        char hex[5] = { payload[2], payload[3], payload[4], payload[5], '\0' };
        long value = strtol(hex, NULL, 16);
        if(value > 0)
        {
            interval_s = (uint32_t)value;
        }
    }

    if(interval_s == 0U)
    {
        interval_s = 60U;   /* default when no interval was ever configured */
    }

    return RelayReport_Start(interval_s);
}
