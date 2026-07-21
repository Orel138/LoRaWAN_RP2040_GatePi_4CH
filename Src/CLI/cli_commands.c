#include "FreeRTOS.h"
#include "task.h"
#include "cli_prv.h"
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include <stdio.h>
#include <string.h>

/* Command: ps - List all tasks */
static void prvPsCommand(ConsoleIO_t * const pxConsoleIO,
                         uint32_t ulArgc,
                         char * ppcArgv[])
{
    char pcBuffer[256];
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    unsigned long ulTotalRunTime, ulStatsAsPercentage;

    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

    if(pxTaskStatusArray != NULL)
    {
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

        if(ulTotalRunTime > 0)
        {
            pxConsoleIO->print("\nTask Name       State Prio Stack  Num   CPU%\n");
            pxConsoleIO->print("==================================================\n");

            for(x = 0; x < uxArraySize; x++)
            {
                ulStatsAsPercentage = pxTaskStatusArray[x].ulRunTimeCounter / (ulTotalRunTime / 100UL);

                char cState;
                switch(pxTaskStatusArray[x].eCurrentState)
                {
                    case eRunning:   cState = 'X'; break;
                    case eReady:     cState = 'R'; break;
                    case eBlocked:   cState = 'B'; break;
                    case eSuspended: cState = 'S'; break;
                    default:         cState = 'D'; break;
                }

                if(ulStatsAsPercentage > 0UL)
                {
                    snprintf(pcBuffer, sizeof(pcBuffer),
                            "%-15s %c     %-4u  %-5u  %-5u %3lu%%\n",
                            pxTaskStatusArray[x].pcTaskName,
                            cState,
                            (unsigned int)pxTaskStatusArray[x].uxCurrentPriority,
                            (unsigned int)pxTaskStatusArray[x].usStackHighWaterMark,
                            (unsigned int)pxTaskStatusArray[x].xTaskNumber,
                            ulStatsAsPercentage);
                }
                else
                {
                    snprintf(pcBuffer, sizeof(pcBuffer),
                            "%-15s %c     %-4u  %-5u  %-5u  <1%%\n",
                            pxTaskStatusArray[x].pcTaskName,
                            cState,
                            (unsigned int)pxTaskStatusArray[x].uxCurrentPriority,
                            (unsigned int)pxTaskStatusArray[x].usStackHighWaterMark,
                            (unsigned int)pxTaskStatusArray[x].xTaskNumber);
                }

                pxConsoleIO->print(pcBuffer);
            }
            pxConsoleIO->print("\n");
        }

        vPortFree(pxTaskStatusArray);
    }
    else
    {
        pxConsoleIO->print("Error: Unable to allocate memory for task list\n");
    }
}

const CLI_Command_Definition_t xCommandDef_ps =
{
    "ps",
    "ps:\n"
    "  List all running tasks with their status\n"
    "  Usage: ps\n\n",
    prvPsCommand
};

/* Command: heap - Show heap statistics */
static void prvHeapStatCommand(ConsoleIO_t * const pxConsoleIO,
                               uint32_t ulArgc,
                               char * ppcArgv[])
{
    char pcBuffer[512];
    HeapStats_t xHeapStats;

    vPortGetHeapStats(&xHeapStats);

    snprintf(pcBuffer, sizeof(pcBuffer),
            "\nHeap Statistics:\n"
            "  Available heap space:  %6u bytes\n"
            "  Largest free block:    %6u bytes\n"
            "  Smallest free block:   %6u bytes\n"
            "  Number of free blocks: %6u\n"
            "  Minimum ever free:     %6u bytes\n"
            "  Successful allocs:     %6u\n"
            "  Successful frees:      %6u\n\n",
            (unsigned int)xHeapStats.xAvailableHeapSpaceInBytes,
            (unsigned int)xHeapStats.xSizeOfLargestFreeBlockInBytes,
            (unsigned int)xHeapStats.xSizeOfSmallestFreeBlockInBytes,
            (unsigned int)xHeapStats.xNumberOfFreeBlocks,
            (unsigned int)xHeapStats.xMinimumEverFreeBytesRemaining,
            (unsigned int)xHeapStats.xNumberOfSuccessfulAllocations,
            (unsigned int)xHeapStats.xNumberOfSuccessfulFrees);

    pxConsoleIO->print(pcBuffer);
}

const CLI_Command_Definition_t xCommandDef_heapStat =
{
    "heap",
    "heap:\n"
    "  Display heap memory statistics\n"
    "  Usage: heap\n\n",
    prvHeapStatCommand
};

/* Command: reset - Reset the RP2040 */
static void prvResetCommand(ConsoleIO_t * const pxConsoleIO,
                            uint32_t ulArgc,
                            char * ppcArgv[])
{
    pxConsoleIO->print("Resetting RP2040...\r\n");
    
    /* Small delay to allow message to be sent */
    vTaskDelay(pdMS_TO_TICKS(100));
    
    /* Reset using watchdog */
    watchdog_enable(1, 1);
    while(1);
}

const CLI_Command_Definition_t xCommandDef_reset =
{
    "reset",
    "reset:\n"
    "  Reset the RP2040 microcontroller\n"
    "  Usage: reset\n\n",
    prvResetCommand
};

/* Command: clear - Clear the terminal screen */
static void prvClearCommand(ConsoleIO_t * const pxConsoleIO,
                           uint32_t ulArgc,
                           char * ppcArgv[])
{
    /* ANSI escape sequence to clear screen and move cursor to home */
    pxConsoleIO->print("\033[2J\033[H");
}

const CLI_Command_Definition_t xCommandDef_clear =
{
    "clear",
    "clear:\n"
    "  Clear the terminal screen\n"
    "  Usage: clear\n\n",
    prvClearCommand
};

/* Command: uptime - Show system uptime */
static void prvUptimeCommand(ConsoleIO_t * const pxConsoleIO,
                             uint32_t ulArgc,
                             char * ppcArgv[])
{
    char pcBuffer[256];
    TickType_t xUptime = xTaskGetTickCount();
    
    uint32_t ulSeconds = xUptime / configTICK_RATE_HZ;
    uint32_t ulMinutes = ulSeconds / 60;
    uint32_t ulHours = ulMinutes / 60;
    uint32_t ulDays = ulHours / 24;
    
    ulSeconds %= 60;
    ulMinutes %= 60;
    ulHours %= 24;
    
    snprintf(pcBuffer, sizeof(pcBuffer),
            "\nSystem uptime: %lu days, %lu hours, %lu minutes, %lu seconds\n"
            "Total ticks: %lu\n\n",
            ulDays, ulHours, ulMinutes, ulSeconds,
            (unsigned long)xUptime);
    
    pxConsoleIO->print(pcBuffer);
}

const CLI_Command_Definition_t xCommandDef_uptime =
{
    "uptime",
    "uptime:\n"
    "  Display system uptime\n"
    "  Usage: uptime\n\n",
    prvUptimeCommand
};
