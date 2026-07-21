#include "FreeRTOS.h"
#include "task.h"
#include "cli_prv.h"
#include "rak3172.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern char pcCliScratchBuffer[CLI_OUTPUT_SCRATCH_BUF_LEN];

/* Command: rak-version - Get RAK3172 firmware version */
static void prvRakVersionCommand(ConsoleIO_t * const pxConsoleIO,
                                 uint32_t ulArgc,
                                 char * ppcArgv[])
{
    char version[64];
    
    pxConsoleIO->print("Getting RAK3172 version...\n");
    
    if(RAK3172_GetVersion(version, sizeof(version)) == pdPASS)
    {
        snprintf(pcCliScratchBuffer, CLI_OUTPUT_SCRATCH_BUF_LEN,
                "RAK3172 Firmware: %s\n", version);
        pxConsoleIO->print(pcCliScratchBuffer);
    }
    else
    {
        pxConsoleIO->print("ERROR: Failed to get version\n");
    }
}

const CLI_Command_Definition_t xCommandDef_rakVersion =
{
    "rak-version",
    "rak-version:\n"
    "  Get RAK3172 firmware version\n"
    "  Usage: rak-version\n\n",
    prvRakVersionCommand
};

/* Command: rak-config - Configure LoRaWAN credentials */
static void prvRakConfigCommand(ConsoleIO_t * const pxConsoleIO,
                                uint32_t ulArgc,
                                char * ppcArgv[])
{
    if(ulArgc < 4)
    {
        pxConsoleIO->print("Usage: rak-config <deveui> <appeui> <appkey>\n");
        pxConsoleIO->print("Example: rak-config 0000000000000000 0000000000000000 00000000000000000000000000000000\n");
        return;
    }
    
    const char *deveui = ppcArgv[1];
    const char *appeui = ppcArgv[2];
    const char *appkey = ppcArgv[3];
    
    pxConsoleIO->print("Configuring RAK3172...\n");
    
    if(RAK3172_SetDevEUI(deveui) == pdPASS)
    {
        pxConsoleIO->print("DevEUI set OK\n");
    }
    else
    {
        pxConsoleIO->print("ERROR: Failed to set DevEUI\n");
        return;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if(RAK3172_SetAppEUI(appeui) == pdPASS)
    {
        pxConsoleIO->print("AppEUI set OK\n");
    }
    else
    {
        pxConsoleIO->print("ERROR: Failed to set AppEUI\n");
        return;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if(RAK3172_SetAppKey(appkey) == pdPASS)
    {
        pxConsoleIO->print("AppKey set OK\n");
    }
    else
    {
        pxConsoleIO->print("ERROR: Failed to set AppKey\n");
        return;
    }
    
    pxConsoleIO->print("Configuration complete!\n");
}

const CLI_Command_Definition_t xCommandDef_rakConfig =
{
    "rak-config",
    "rak-config:\n"
    "  Configure LoRaWAN credentials\n"
    "  Usage: rak-config <deveui> <appeui> <appkey>\n"
    "  Example: rak-config 0000000000000000 0000000000000000 00000000000000000000000000000000\n\n",
    prvRakConfigCommand
};

/* Command: rak-join - Join LoRaWAN network */
static void prvRakJoinCommand(ConsoleIO_t * const pxConsoleIO,
                              uint32_t ulArgc,
                              char * ppcArgv[])
{
    pxConsoleIO->print("Joining LoRaWAN network...\n");
    pxConsoleIO->print("This may take up to 30 seconds...\n");

    if(RAK3172_Join(30000) == pdPASS)
    {
        pxConsoleIO->print("Successfully joined LoRaWAN network!\n");
    }
    else
    {
        pxConsoleIO->print("ERROR: Failed to join network\n");
    }
}

const CLI_Command_Definition_t xCommandDef_rakJoin =
{
    "rak-join",
    "rak-join:\n"
    "  Join LoRaWAN network\n"
    "  Usage: rak-join\n\n",
    prvRakJoinCommand
};

/* Command: rak-send - Send data via LoRaWAN */
static void prvRakSendCommand(ConsoleIO_t * const pxConsoleIO,
                              uint32_t ulArgc,
                              char * ppcArgv[])
{
    if(ulArgc < 3)
    {
        pxConsoleIO->print("Usage: rak-send <port> <hex_data>\n");
        pxConsoleIO->print("Example: rak-send 2 48656C6C6F\n");
        return;
    }
    
    uint8_t port = atoi(ppcArgv[1]);
    const char *hexData = ppcArgv[2];
    
    /* Convert hex string to bytes */
    uint16_t dataLen = strlen(hexData) / 2;
    uint8_t data[RAK3172_MAX_PAYLOAD];
    
    if(dataLen > RAK3172_MAX_PAYLOAD)
    {
        pxConsoleIO->print("ERROR: Data too long (max 255 bytes)\n");
        return;
    }
    
    for(uint16_t i = 0; i < dataLen; i++)
    {
        char byteStr[3] = {hexData[i*2], hexData[i*2+1], '\0'};
        data[i] = (uint8_t)strtol(byteStr, NULL, 16);
    }
    
    snprintf(pcCliScratchBuffer, CLI_OUTPUT_SCRATCH_BUF_LEN,
            "Sending %d bytes on port %d...\n", dataLen, port);
    pxConsoleIO->print(pcCliScratchBuffer);
    
    if(RAK3172_SendDataUnconfirmed(port, data, dataLen) == pdPASS)
    {
        pxConsoleIO->print("Data sent successfully!\n");
    }
    else
    {
        pxConsoleIO->print("ERROR: Failed to send data\n");
    }
}

const CLI_Command_Definition_t xCommandDef_rakSend =
{
    "rak-send",
    "rak-send:\n"
    "  Send data via LoRaWAN\n"
    "  Usage: rak-send <port> <hex_data>\n"
    "  Example: rak-send 2 48656C6C6F (sends 'Hello')\n\n",
    prvRakSendCommand
};

/* Command: rak-at - Send raw AT command */
static void prvRakATCommand(ConsoleIO_t * const pxConsoleIO,
                            uint32_t ulArgc,
                            char * ppcArgv[])
{
    if(ulArgc < 2)
    {
        pxConsoleIO->print("Usage: rak-at <command>\n");
        pxConsoleIO->print("Example: rak-at AT+VER=?\n");
        return;
    }
    
    /* Reconstruct command from all arguments */
    char cmd[256] = {0};
    for(uint32_t i = 1; i < ulArgc; i++)
    {
        strcat(cmd, ppcArgv[i]);
        if(i < ulArgc - 1)
            strcat(cmd, " ");
    }
    
    char response[512];
    if(RAK3172_SendCommand(cmd, response, 5000) == pdPASS)
    {
        pxConsoleIO->print(response);
    }
    else
    {
        pxConsoleIO->print("ERROR: Command timeout\n");
    }
}

const CLI_Command_Definition_t xCommandDef_rakAT =
{
    "rak-at",
    "rak-at:\n"
    "  Send raw AT command to RAK3172\n"
    "  Usage: rak-at <command>\n"
    "  Example: rak-at AT+VER=?\n\n",
    prvRakATCommand
};

/* Command: rak-test - Low-level connectivity diagnostic */
static void prvRakTestCommand(ConsoleIO_t * const pxConsoleIO,
                              uint32_t ulArgc,
                              char * ppcArgv[])
{
    char dump[256];
    uint32_t bytesReceived = 0;
    uint32_t baudFound = 0;

    pxConsoleIO->print("Resetting module and probing UART (115200/9600)...\n");

    BaseType_t xResult = RAK3172_SelfTest(dump, sizeof(dump),
                                          &bytesReceived, &baudFound);

    if(xResult == pdPASS)
    {
        snprintf(pcCliScratchBuffer, CLI_OUTPUT_SCRATCH_BUF_LEN,
                 "Module ALIVE: %lu bytes @ %lu baud\nRaw: %s\n",
                 (unsigned long)bytesReceived, (unsigned long)baudFound, dump);
        pxConsoleIO->print(pcCliScratchBuffer);

        if(baudFound != 115200)
        {
            pxConsoleIO->print("WARNING: module answered at a non-default baud;"
                               " update RAK3172_BAUD_RATE.\n");
        }
    }
    else
    {
        pxConsoleIO->print("No response from RAK3172.\n");
        pxConsoleIO->print("Check: BOOT0=GP7 low, RST=GP8, UART TX=GP0/RX=GP1"
                           " (not swapped), 3V3 power, common GND.\n");
    }
}

const CLI_Command_Definition_t xCommandDef_rakTest =
{
    "rak-test",
    "rak-test:\n"
    "  Low-level RAK3172 connectivity diagnostic (reset + raw AT probe)\n"
    "  Usage: rak-test\n\n",
    prvRakTestCommand
};

/* Command: rak-reset - Hardware reset RAK3172 */
static void prvRakResetCommand(ConsoleIO_t * const pxConsoleIO,
                                uint32_t ulArgc,
                                char * ppcArgv[])
{
    pxConsoleIO->print("Performing hardware reset...\n");
    
    if(RAK3172_HardwareReset() == pdPASS)
    {
        pxConsoleIO->print("Reset complete\n");
    }
    else
    {
        pxConsoleIO->print("ERROR: Reset failed\n");
    }
}

const CLI_Command_Definition_t xCommandDef_rakReset =
{
    "rak-reset",
    "rak-reset:\n"
    "  Perform hardware reset of RAK3172\n"
    "  Usage: rak-reset\n\n",
    prvRakResetCommand
};