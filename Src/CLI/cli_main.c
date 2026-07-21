#include "FreeRTOS.h"
#include "task.h"
#include "cli.h"
#include "cli_prv.h"
#include "stream_buffer.h"
#include <string.h>

#include <stdio.h>
#include "hardware/uart.h"

#define CLI_MAX_ARGS 10

typedef struct xCOMMAND_INPUT_LIST
{
    const CLI_Command_Definition_t * pxCommandLineDefinition;
    struct xCOMMAND_INPUT_LIST * pxNext;
} CLI_Definition_List_Item_t;

extern ConsoleIO_t xConsoleIO;
extern BaseType_t xInitConsoleUart(void);

char pcCliScratchBuffer[CLI_OUTPUT_SCRATCH_BUF_LEN];

static void prvHelpCommand(ConsoleIO_t * const pxConsoleIO,
                           uint32_t ulArgc,
                           char * ppcArgv[]);

static uint32_t prvGetNumberOfArgs(const char * pcCommandString);

/* Help command definition */
static const CLI_Command_Definition_t xHelpCommand =
{
    "help",
    "help:\n"
    "  List available commands and their arguments.\n"
    "  Usage:\n\n"
    "  help\n"
    "    Print help for all recognized commands\n\n"
    "  help <command>\n"
    "    Print help text for a specific command\n\n",
    prvHelpCommand
};

static CLI_Definition_List_Item_t xRegisteredCommands =
{
    &xHelpCommand,
    NULL
};

BaseType_t FreeRTOS_CLIRegisterCommand(const CLI_Command_Definition_t * const pxCommandToRegister)
{
    static CLI_Definition_List_Item_t * pxLastCommandInList = &xRegisteredCommands;
    CLI_Definition_List_Item_t * pxNewListItem;
    BaseType_t xReturn = pdFAIL;

    configASSERT(pxCommandToRegister);

    pxNewListItem = (CLI_Definition_List_Item_t *) pvPortMalloc(sizeof(CLI_Definition_List_Item_t));
    configASSERT(pxNewListItem);

    if(pxNewListItem != NULL)
    {
        taskENTER_CRITICAL();
        {
            pxNewListItem->pxCommandLineDefinition = pxCommandToRegister;
            pxNewListItem->pxNext = NULL;
            pxLastCommandInList->pxNext = pxNewListItem;
            pxLastCommandInList = pxNewListItem;
        }
        taskEXIT_CRITICAL();

        xReturn = pdPASS;
    }

    return xReturn;
}

static const CLI_Definition_List_Item_t * prvFindMatchingCommand(const char * const pcCommandInput)
{
    const CLI_Definition_List_Item_t * pxCommand = &xRegisteredCommands;

    while(pxCommand != NULL)
    {
        const char * pcRegisteredCommandString = pxCommand->pxCommandLineDefinition->pcCommand;
        size_t xCommandStringLength = strlen(pcRegisteredCommandString);

        if((strncmp(pcCommandInput, pcRegisteredCommandString, xCommandStringLength) == 0) &&
           ((pcCommandInput[xCommandStringLength] == ' ') || (pcCommandInput[xCommandStringLength] == '\x00')))
        {
            break;
        }
        else
        {
            pxCommand = pxCommand->pxNext;
        }
    }

    return pxCommand;
}

void FreeRTOS_CLIProcessCommand(ConsoleIO_t * const pxCIO, char * pcCommandInput)
{
    const CLI_Definition_List_Item_t * pxCommand;

    pxCommand = prvFindMatchingCommand(pcCommandInput);

    if(pxCommand != NULL)
    {
        uint32_t ulArgC = prvGetNumberOfArgs(pcCommandInput);
        
        // Vérification de sécurité
        if(ulArgC == 0 || ulArgC > CLI_MAX_ARGS)
        {
            pxCIO->print("Error: Invalid number of arguments\r\n");
            return;
        }
        
        // Utiliser un tableau de taille fixe au lieu d'un VLA
        char * pcArgv[CLI_MAX_ARGS];
        memset(pcArgv, 0, sizeof(pcArgv));
        
        char * pcTokenizerCtx = NULL;

        pcArgv[0] = strtok_r(pcCommandInput, " ", &pcTokenizerCtx);

        for(uint32_t i = 1; i < ulArgC; i++)
        {
            pcArgv[i] = strtok_r(NULL, " ", &pcTokenizerCtx);
            if(pcArgv[i] == NULL)
            {
                pxCIO->print("Error: Failed to parse arguments\r\n");
                return;
            }
        }

        // Call the callback function
        pxCommand->pxCommandLineDefinition->pxCommandInterpreter(pxCIO, ulArgC, pcArgv);
    }
    else
    {
        pxCIO->print("Command not recognized. Enter 'help' to view a list of available commands.\r\n");
    }
}

const char * FreeRTOS_CLIGetParameter(const char * pcCommandString,
                                      UBaseType_t uxWantedParameter,
                                      BaseType_t * pxParameterStringLength)
{
    UBaseType_t uxParametersFound = 0;
    const char * pcReturn = NULL;

    configASSERT(pxParameterStringLength);

    *pxParameterStringLength = 0;

    while(uxParametersFound < uxWantedParameter)
    {
        while(((*pcCommandString) != 0x00) && ((*pcCommandString) != ' '))
        {
            pcCommandString++;
        }

        while(((*pcCommandString) != 0x00) && ((*pcCommandString) == ' '))
        {
            pcCommandString++;
        }

        if(*pcCommandString != 0x00)
        {
            uxParametersFound++;

            if(uxParametersFound == uxWantedParameter)
            {
                pcReturn = pcCommandString;

                while(((*pcCommandString) != 0x00) && ((*pcCommandString) != ' '))
                {
                    (*pxParameterStringLength)++;
                    pcCommandString++;
                }

                if(*pxParameterStringLength == 0)
                {
                    pcReturn = NULL;
                }

                break;
            }
        }
        else
        {
            break;
        }
    }

    return pcReturn;
}

static void prvHelpCommand(ConsoleIO_t * const pxConsoleIO,
                           uint32_t ulArgc,
                           char * ppcArgv[])
{
    static const CLI_Definition_List_Item_t * pxCommand = NULL;

    if((ulArgc > 1) && (ppcArgv[1] != NULL))
    {
        BaseType_t xFound = pdFALSE;
        pxCommand = &xRegisteredCommands;

        while(pxCommand != NULL && xFound == pdFALSE)
        {
            if(strncmp(pxCommand->pxCommandLineDefinition->pcCommand,
                       ppcArgv[1],
                       strlen(pxCommand->pxCommandLineDefinition->pcCommand)) == 0)
            {
                xFound = pdTRUE;
            }
            else
            {
                pxCommand = pxCommand->pxNext;
            }
        }
    }

    if(pxCommand != NULL)
    {
        pxConsoleIO->print(pxCommand->pxCommandLineDefinition->pcHelpString);
    }
    else
    {
        pxCommand = &xRegisteredCommands;

        while(pxCommand != NULL)
        {
            pxConsoleIO->print(pxCommand->pxCommandLineDefinition->pcHelpString);
            pxCommand = pxCommand->pxNext;
        }
    }
}

static uint32_t prvGetNumberOfArgs(const char * pcCommandString)
{
    uint32_t luArgCount = 0;
    const char * pcCurrentChar = pcCommandString;

    while(*pcCurrentChar != '\x00')
    {
        if((pcCurrentChar[0] != ' ') &&
           ((pcCurrentChar[1] == ' ') || (pcCurrentChar[1] == '\x00')))
        {
            luArgCount++;
        }

        pcCurrentChar++;
    }

    return luArgCount;
}

void Task_CLI(void *pvParameters)
{
    (void) pvParameters;
    
    printf("CLI Task started\n");
    printf("Free heap in CLI task: %u bytes\n", xPortGetFreeHeapSize());
    
    // Attendre que le système soit stable
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* Register commands */
    FreeRTOS_CLIRegisterCommand(&xCommandDef_ps);
    FreeRTOS_CLIRegisterCommand(&xCommandDef_heapStat);
    FreeRTOS_CLIRegisterCommand(&xCommandDef_reset);
    FreeRTOS_CLIRegisterCommand(&xCommandDef_clear);
    FreeRTOS_CLIRegisterCommand(&xCommandDef_uptime);

    /* Register relay command */
    FreeRTOS_CLIRegisterCommand(&xCommandDef_relay);

    /* Register RAK3172 commands */
    FreeRTOS_CLIRegisterCommand(&xCommandDef_rakVersion);
    FreeRTOS_CLIRegisterCommand(&xCommandDef_rakConfig);
    FreeRTOS_CLIRegisterCommand(&xCommandDef_rakJoin);
    FreeRTOS_CLIRegisterCommand(&xCommandDef_rakSend);
    FreeRTOS_CLIRegisterCommand(&xCommandDef_rakAT);
    FreeRTOS_CLIRegisterCommand(&xCommandDef_rakReset);
    FreeRTOS_CLIRegisterCommand(&xCommandDef_rakTest);

    printf("Commands registered\n");
    
    char * pcCommandBuffer = NULL;

    if(xInitConsoleUart() == pdTRUE)
    {
        printf("Console UART initialized, entering command loop\n");
        
        for(;;)
        {
            int32_t lLen = xConsoleIO.readline(&pcCommandBuffer);

            if((pcCommandBuffer != NULL) && (lLen > 0))
            {
                FreeRTOS_CLIProcessCommand(&xConsoleIO, pcCommandBuffer);
            }
        }
    }
    else
    {
        printf("Failed to initialize UART console.\n");
        vTaskDelete(NULL);
    }
}