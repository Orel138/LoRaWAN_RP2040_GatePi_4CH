#include "FreeRTOS.h"
#include "task.h"
#include "cli_prv.h"
#include "relay.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern char pcCliScratchBuffer[CLI_OUTPUT_SCRATCH_BUF_LEN];

/* Command: relay - Control the on-board relays */
static void prvRelayCommand(ConsoleIO_t * const pxConsoleIO,
                            uint32_t ulArgc,
                            char * ppcArgv[])
{
    if(ulArgc < 3)
    {
        pxConsoleIO->print("Usage: relay <1-4|all> <on|off>\n");
        pxConsoleIO->print("Example: relay 1 on / relay all off\n");
        return;
    }

    const char *pcTarget = ppcArgv[1];
    const char *pcState = ppcArgv[2];
    bool on;

    if(strcmp(pcState, "on") == 0)
    {
        on = true;
    }
    else if(strcmp(pcState, "off") == 0)
    {
        on = false;
    }
    else
    {
        pxConsoleIO->print("Error: state must be 'on' or 'off'\n");
        return;
    }

    if(strcmp(pcTarget, "all") == 0)
    {
        Relay_SetAll(on);
        snprintf(pcCliScratchBuffer, CLI_OUTPUT_SCRATCH_BUF_LEN,
                 "All relays %s\n", on ? "on" : "off");
        pxConsoleIO->print(pcCliScratchBuffer);
        return;
    }

    int relay = atoi(pcTarget);
    if((relay < 1) || (relay > (int)RELAY_COUNT))
    {
        pxConsoleIO->print("Error: relay must be 1-4 or 'all'\n");
        return;
    }

    (void)Relay_Set((uint8_t)relay, on);
    snprintf(pcCliScratchBuffer, CLI_OUTPUT_SCRATCH_BUF_LEN,
             "Relay %d %s\n", relay, on ? "on" : "off");
    pxConsoleIO->print(pcCliScratchBuffer);
}

const CLI_Command_Definition_t xCommandDef_relay =
{
    "relay",
    "relay:\n"
    "  Control the on-board relays (GP18-GP21)\n"
    "  Usage: relay <1-4|all> <on|off>\n"
    "  Example: relay 1 on / relay 4 off / relay all on\n\n",
    prvRelayCommand
};
